#!/usr/bin/env bash
# WFS-238 : notification en temps réel de chaque APK de main publié.
# Ajoute un commentaire qui mentionne @wlfaleh dans l'issue fixe « 📱 Nouvel APK de main » (créée et épinglée une seule fois) :
# l'application GitHub du téléphone envoie la notification à la seconde. Aucun envoi hors de GitHub, jeton GITHUB_TOKEN seulement.
# Ne fait jamais échouer la publication : chaque erreur donne un ::warning:: et le script finit à 0 (RG-07).
# Entrées : GH_REPO, RUN_ID, RUN_NUMBER, GITHUB_SHA, GITHUB_SERVER_URL, COMMITS (JSON de l'envoi), HEAD_MSG.
set -u
TITRE_ISSUE="📱 Nouvel APK de main"
QUI="@wlfaleh"
SERVEUR="${GITHUB_SERVER_URL:-https://github.com}"
LIEN_APK="$SERVEUR/$GH_REPO/releases/download/apercu/WaForSpeed.apk"
LIEN_RUN="$SERVEUR/$GH_REPO/actions/runs/$RUN_ID"
COURT="${GITHUB_SHA::7}"
MARQUE="<!-- wfs-apk-construction-$RUN_NUMBER -->"   # RG-03 : un seul commentaire par construction
avert() { echo "::warning::notification de l'APK : $*"; exit 0; }

# RG-02 : seulement si CE run a publié l'APK (le titre de la Release porte la construction et le commit de l'APK en ligne)
titre=$(gh api "repos/$GH_REPO/releases/tags/apercu" --jq .name 2>/dev/null) || avert "Release apercu illisible"
attendu="Aperçu — construction $RUN_NUMBER ($COURT)"
if [ "$titre" != "$attendu" ]; then
    echo "Notification : l'APK en ligne n'est pas celui de ce run (« $titre ») : rien à envoyer"; exit 0
fi

# RG-04 : tickets du run : tous les commits depuis le dernier run de main qui a tourné (comme le job résumé, WFS-237), sinon ceux de l'envoi
liste=""
base=$(gh api "repos/$GH_REPO/actions/workflows/build.yml/runs?branch=main&event=push&status=completed&per_page=30" \
         --jq "[.workflow_runs[] | select(.id != $RUN_ID and .conclusion != \"cancelled\")][0].head_sha // empty" 2>/dev/null || true)
[ -n "$base" ] && liste=$(gh api "repos/$GH_REPO/compare/$base...$GITHUB_SHA" --jq '.commits[].commit.message' 2>/dev/null || true)
tickets=$(printf '%s\n%s\n%s\n' "${COMMITS:-}" "${HEAD_MSG:-}" "$liste" | grep -oE 'WFS-[0-9]+' | sort -u -t- -k2,2n | tr '\n' ' ' | sed 's/ $//' || true)
[ -n "$tickets" ] || tickets="voir le résumé du run"

# RG-05 : l'issue fixe, retrouvée par son titre exact ; créée et épinglée une seule fois si elle n'existe pas
num=$(gh api "repos/$GH_REPO/issues?state=open&per_page=100" --jq "[.[] | select(.pull_request == null and .title == \"$TITRE_ISSUE\")][0].number // empty" 2>/dev/null) \
    || avert "liste des issues illisible"
if [ -z "$num" ]; then
    corps="Chaque nouvel APK de \`main\` est annoncé ici par la CI (WFS-238), avec une mention qui envoie une notification sur le téléphone.
Lien fixe de l'APK : $LIEN_APK"
    num=$(gh api -X POST "repos/$GH_REPO/issues" -f title="$TITRE_ISSUE" -f body="$corps" --jq .number 2>/dev/null) || avert "création de l'issue impossible"
    noeud=$(gh api "repos/$GH_REPO/issues/$num" --jq .node_id 2>/dev/null || true)
    if [ -n "$noeud" ]; then
        gh api graphql -f query='mutation($id:ID!){pinIssue(input:{issueId:$id}){issue{number}}}' -f id="$noeud" > /dev/null 2>&1 \
            || echo "::warning::issue #$num créée mais non épinglée (à épingler à la main)"
    fi
    echo "Issue #$num « $TITRE_ISSUE » créée"
fi

# RG-03 : pas de second commentaire pour la même construction (job relancé)
if gh api "repos/$GH_REPO/issues/$num/comments?per_page=100&sort=created&direction=desc" --jq '.[].body' 2>/dev/null | grep -qF "$MARQUE"; then
    echo "Notification : construction $RUN_NUMBER déjà annoncée dans l'issue #$num"; exit 0
fi

message="$QUI 📱 **Nouvel APK de main disponible** : construction $RUN_NUMBER (\`$COURT\`)
- Tickets : $tickets
- Télécharger : $LIEN_APK
- Run : $LIEN_RUN
$MARQUE"
gh api -X POST "repos/$GH_REPO/issues/$num/comments" -f body="$message" --silent 2>/dev/null || avert "commentaire impossible dans l'issue #$num"
echo "Notification envoyée : issue #$num, construction $RUN_NUMBER ($COURT), tickets : $tickets"
