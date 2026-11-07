#!/usr/bin/env bash
# WaForSpeed — banc de la chauffe (WFS-123) lancé par le job « endurance » de la CI, la nuit et à la demande (WFS-126, RG-07).
# Usage : tools/qa/chauffe.sh <wfs_bench> <dossier des captures> <assets>
# Écrit <dossier>/chauffe-journal.txt (lignes CHAUFFE / chauffe / IPS) et copie chauffe-bandeau, chauffe-fin, chauffe-retabli.
# Échec (code ≠ 0) : banc en échec, plus de LIMITE s, ou ligne « CHAUFFE rétrogradation » / « CHAUFFE rétabli » absente.
# WFS-220 : limite réglable (CHAUFFE_LIMITE) ; valeur à fixer avec 25 % de marge au-dessus de la durée mesurée sur la CI (RG-03).
set -u
BENCH=$1; OUT=$2; ASSETS=$3
LIMITE=${CHAUFFE_LIMITE:-600}
mkdir -p "$OUT/chauffe"
debut=$(date +%s)
WFS_BANC=chauffe timeout "$LIMITE" "$BENCH" "$OUT/chauffe" "$ASSETS" > "$OUT/chauffe-brut.txt" 2>&1
CODE=$?
duree=$(( $(date +%s) - debut ))
{
  grep -E "CHAUFFE|chauffe|IPS plafond|\[ERREUR\]" "$OUT/chauffe-brut.txt"
  echo "---"
  echo "code de sortie du banc : $CODE ; durée : $duree s (au plus $LIMITE)"
} > "$OUT/chauffe-journal.txt"
cat "$OUT/chauffe-journal.txt"
for f in chauffe-bandeau chauffe-fin chauffe-retabli; do cp "$OUT/chauffe/$f.png" "$OUT/" 2>/dev/null || echo "::warning::chauffe : capture $f absente"; done
ECHEC=0
if [ "$CODE" = 124 ]; then echo "::error::chauffe : banc arrêté après $LIMITE s"; ECHEC=1
elif [ "$CODE" != 0 ]; then echo "::error::chauffe : le banc sort en échec (code $CODE)"; ECHEC=1; fi
for motif in "CHAUFFE rétrogradation" "CHAUFFE rétabli"; do
  grep -q "$motif" "$OUT/chauffe-journal.txt" || { echo "::error::chauffe : « $motif » absent du journal"; ECHEC=1; }
done
exit $ECHEC
