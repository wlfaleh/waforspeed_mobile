#!/usr/bin/env bash
# WFS-277 : message « recette prête » à la fin des bancs du matin (matin.yml), dans l'issue fixe « 🧪 Recette QA du matin »
# (créée et épinglée la première fois). Il mentionne @wlfaleh (notification sur le téléphone) et dit à la QA ce qu'elle recette :
# commit photographié, résultat de chaque job, tickets fusionnés depuis la recette précédente, liens des captures et de l'émulateur.
# Ne fait jamais échouer le run : chaque erreur donne un ::warning:: et le script finit à 0.
# Entrées : GH_REPO, RUN_ID, RUN_NUMBER, GITHUB_SHA, GITHUB_SERVER_URL, RESULTATS (lignes « job : résultat »).
set -u
TITRE_ISSUE="🧪 Recette QA du matin"
SERVEUR="${GITHUB_SERVER_URL:-https://github.com}"
COURT="${GITHUB_SHA::7}"
MARQUE="<!-- wfs-recette-matin-$RUN_NUMBER -->"
avert() { echo "::warning::recette du matin : $*"; exit 0; }

# tickets fusionnés (titres « WFS-XX : … (#NNN) ») depuis le dernier run de matin.yml qui a tourné jusqu'au bout
base=$(gh api "repos/$GH_REPO/actions/workflows/matin.yml/runs?branch=main&status=completed&per_page=30" \
         --jq "[.workflow_runs[] | select(.id != $RUN_ID and (.conclusion == \"success\" or .conclusion == \"failure\"))][0].head_sha // empty" 2>/dev/null || true)
tickets=""
if [ -n "$base" ]; then
    tickets=$(gh api "repos/$GH_REPO/compare/$base...$GITHUB_SHA" --jq '.commits[].commit.message | split("\n")[0]' 2>/dev/null \
              | grep -E '\(#[0-9]+\)' | grep -oE '^WFS-[0-9]+' | sort -u -t- -k2,2n | tr '\n' ' ' | sed 's/ $//' || true)
fi
[ -n "$tickets" ] || tickets="voir qa/Check.md (premier run, ou comparaison illisible)"
verdict="✅ tout est vert"
echo "$RESULTATS" | grep -v "(information)" | grep -qvE ': (success|skipped)$' && verdict="❌ au moins un job n'est pas vert : la QA le signale au PO (ticket de correction), et recette le reste"

num=$(gh api "repos/$GH_REPO/issues?state=open&per_page=100" --jq "[.[] | select(.pull_request == null and .title == \"$TITRE_ISSUE\")][0].number // empty" 2>/dev/null) \
    || avert "liste des issues illisible"
if [ -z "$num" ]; then
    num=$(gh api -X POST "repos/$GH_REPO/issues" -f title="$TITRE_ISSUE" \
            -f body="Chaque matin, la CI (matin.yml, 04:00 heure de Paris) écrit ici quand la recette QA peut commencer (WFS-277). La QA commence à 07:00, seulement si le message du jour est là." \
            --jq .number 2>/dev/null) || avert "création de l'issue impossible"
    noeud=$(gh api "repos/$GH_REPO/issues/$num" --jq .node_id 2>/dev/null || true)
    [ -n "$noeud" ] && { gh api graphql -f query='mutation($id:ID!){pinIssue(input:{issueId:$id}){issue{number}}}' -f id="$noeud" > /dev/null 2>&1 \
        || echo "::warning::issue #$num créée mais non épinglée"; }
fi
if gh api "repos/$GH_REPO/issues/$num/comments?per_page=100&sort=created&direction=desc" --jq '.[].body' 2>/dev/null | grep -qF "$MARQUE"; then
    echo "Recette du matin déjà annoncée (run $RUN_NUMBER)"; exit 0
fi
message="@wlfaleh 🧪 **Recette QA prête** — $(TZ=Europe/Paris date '+%d/%m/%Y %H:%M') (heure de Paris), commit \`$COURT\`
- Verdict des bancs : $verdict
$(echo "$RESULTATS" | sed 's/^/  - /')
- Tickets fusionnés depuis la recette précédente : $tickets
- Captures du banc d'essai PC : $SERVEUR/$GH_REPO/releases/tag/apercu
- Émulateur Android (parcours mvp) : $SERVEUR/$GH_REPO/releases/tag/simulateur-matin
- Run : $SERVEUR/$GH_REPO/actions/runs/$RUN_ID
$MARQUE"
gh api -X POST "repos/$GH_REPO/issues/$num/comments" -f body="$message" --silent 2>/dev/null || avert "commentaire impossible dans l'issue #$num"
echo "Recette du matin annoncée : issue #$num, commit $COURT"
