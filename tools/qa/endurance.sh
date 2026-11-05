#!/usr/bin/env bash
# WaForSpeed — banc d'endurance (WFS-91) avec les couches de validation Vulkan, lancé par le job « endurance » de la CI.
# Usage : tools/qa/endurance.sh <wfs_bench> <dossier des captures> <assets>
# Écrit <dossier>/endurance-journal.txt (lignes « endurance : », RSS, cycles, erreurs de validation).
# Échec (code ≠ 0) : plantage, fuite de mémoire (code 3 du banc) ou erreur de validation absente de
# tools/qa/validation_connues.txt (une ligne = un identifiant VUID ou un morceau exact du message, # = commentaire).
set -u
BENCH=$1; OUT=$2; ASSETS=$3
DIR=$(cd "$(dirname "$0")" && pwd)
CONNUES="$DIR/validation_connues.txt"
mkdir -p "$OUT"
LOG="$OUT/endurance-brut.txt"
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation WFS_BANC=endurance "$BENCH" "$OUT" "$ASSETS" > "$LOG" 2>&1
CODE=$?
# erreurs de validation (les avertissements de performance ne comptent pas, RG-04)
grep -E "Validation Error" "$LOG" > "$OUT/validation-erreurs.txt" || true
TOTAL=$(wc -l < "$OUT/validation-erreurs.txt")
MOTIFS=$(grep -vE '^\s*(#|$)' "$CONNUES" 2>/dev/null | grep -v '^fuite-connue' || true)
if [ -n "$MOTIFS" ]; then
  grep -vF -f <(printf '%s\n' "$MOTIFS") "$OUT/validation-erreurs.txt" > "$OUT/validation-inconnues.txt" || true
else
  cp "$OUT/validation-erreurs.txt" "$OUT/validation-inconnues.txt"
fi
INCONNUES=$(wc -l < "$OUT/validation-inconnues.txt")
FUITE_CONNUE=$(grep -c '^fuite-connue' "$CONNUES" 2>/dev/null || true)
{
  grep -E "endurance :|\[err" "$LOG"
  echo "---"
  echo "code de sortie du banc : $CODE"
  echo "erreurs de validation : $TOTAL (dont $((TOTAL - INCONNUES)) connues, $INCONNUES inconnues)"
  sort "$OUT/validation-inconnues.txt" | sed -E 's/^.*(VUID-[A-Za-z0-9_-]+).*$/\1/' | sort | uniq -c | sort -rn | head -20
} > "$OUT/endurance-journal.txt"
cat "$OUT/endurance-journal.txt"
ECHEC=0
if [ "$CODE" = 3 ] && [ "${FUITE_CONNUE:-0}" -gt 0 ]; then echo "::warning::endurance : fuite de mémoire connue (tools/qa/validation_connues.txt)"
elif [ "$CODE" != 0 ]; then echo "::error::endurance : le banc sort en échec (code $CODE)"; ECHEC=1; fi
if [ "$INCONNUES" -gt 0 ]; then echo "::error::endurance : $INCONNUES erreur(s) de validation Vulkan inconnue(s)"; ECHEC=1; fi
exit $ECHEC
