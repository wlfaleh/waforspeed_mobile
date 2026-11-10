#!/usr/bin/env bash
# WaForSpeed — bancs du matin partis après 07:00 heure de Paris (réveil de GitHub en retard) : message dans l'issue
# « 🧪 Recette QA du matin » pour que la QA sache, avant 07:00 ou dès son passage, que la recette du jour n'est pas prête,
# quand elle le sera et où sont les captures en attendant (WFS-384, RG-06). Ce message ne contient jamais « Recette QA prête ».
# Ne fait jamais échouer le run : chaque erreur donne un ::warning:: et le script finit à 0.
# Entrées : GH_REPO, RUN_ID, RUN_NUMBER, GITHUB_SERVER_URL ; MAINTENANT (essais).
set -u
TITRE_ISSUE="🧪 Recette QA du matin"
SERVEUR="${GITHUB_SERVER_URL:-https://github.com}"
ICI=$(cd "$(dirname "$0")" && pwd)
maintenant="${MAINTENANT:-$(date +%s)}"
MARQUE="<!-- wfs-retard-matin-$RUN_NUMBER -->"
avert() { echo "::warning::bancs du matin en retard : $*"; exit 0; }

num=$(gh api "repos/$GH_REPO/issues?state=open&per_page=100" --jq "[.[] | select(.pull_request == null and .title == \"$TITRE_ISSUE\")][0].number // empty" 2>/dev/null) \
    || avert "liste des issues illisible"
[ -n "$num" ] || avert "issue « $TITRE_ISSUE » introuvable (recette_prete.sh la créera à la fin des bancs)"
etat=$(bash "$ICI/etat_captures_matin.sh")
message="@wlfaleh ⏳ **Bancs du matin en retard** — $(TZ=Europe/Paris date -d "@$maintenant" '+%d/%m/%Y %H:%M') (heure de Paris)
- GitHub a réveillé la CI en retard : les bancs du jour partent seulement maintenant (run $RUN_NUMBER), au lieu de 04:00.
- **La recette QA du jour n'est pas prête** : attendre le message de fin des bancs, vers $(TZ=Europe/Paris date -d "@$(( maintenant + 5400 ))" '+%H:%M') (≈ 1 h 30).
- Captures en ce moment : $etat.
- Run : $SERVEUR/$GH_REPO/actions/runs/$RUN_ID
$MARQUE"
if gh api "repos/$GH_REPO/issues/$num/comments?per_page=100&sort=created&direction=desc" --jq '.[].body' 2>/dev/null | grep -qF "$MARQUE"; then
    echo "Retard déjà annoncé (run $RUN_NUMBER)"; exit 0
fi
gh api -X POST "repos/$GH_REPO/issues/$num/comments" -f body="$message" --silent 2>/dev/null || avert "commentaire impossible dans l'issue #$num"
echo "Retard annoncé : issue #$num"
