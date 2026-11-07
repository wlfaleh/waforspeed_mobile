#!/usr/bin/env bash
# WaForSpeed — WFS-250 (RG-04) : vérifie, après décompression, l'archive du job « compilation » de build.yml.
# Usage : tools/qa/verifier_compilation.sh <dossier de construction>
# Rouge (code 1) si wfs_bench manque, si un test de la liste écrite à la compilation manque ou n'est pas exécutable,
# ou si le nombre de build/test_* n'est pas celui de la liste : jamais un test sauté en silence.
set -u
B=${1:-build}
echec() { echo "::error::archive de compilation : $*"; exit 1; }
[ -x "$B/wfs_bench" ] || echec "$B/wfs_bench absent ou non exécutable"
[ -s "$B/liste-tests.txt" ] || echec "$B/liste-tests.txt absente ou vide"
attendus=0
while IFS= read -r t; do
  [ -n "$t" ] || continue
  attendus=$(( attendus + 1 ))
  [ -f "$B/$t" ] && [ -x "$B/$t" ] || echec "$t absent ou non exécutable"
done < "$B/liste-tests.txt"
trouves=$(ls -1 "$B"/test_* 2>/dev/null | wc -l)
[ "$trouves" -eq "$attendus" ] || echec "$trouves test_* trouvés pour $attendus attendus"
echo "Archive de compilation vérifiée : wfs_bench et $attendus tests ($(du -sh "$B" | cut -f1))"
