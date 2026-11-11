#!/usr/bin/env bash
# WaForSpeed — tous les tests CPU (WFS-117) : lance chaque exécutable build/test_* (déclaré par add_executable dans
# CMakeLists.txt), triés par nom ; un nouveau test est donc lancé par la CI sans rien ajouter ici (RG-07).
# Usage : tools/qa/tests_cpu.sh <dossier de construction>   (à lancer depuis la racine du dépôt)
# Sortie : « [OK] test_xxx (n s) » ou « [ÉCHEC] test_xxx » (+ 30 dernières lignes de son journal), puis un résumé.
# WFS-412 : tous les tests tournent, même après un échec ; le résumé final donne la liste des tests en échec.
# Code de sortie : 1 s'il y a au moins un échec ou si aucun test n'est trouvé, 0 sinon.
set -u
BUILD=${1:-build}

# RG-02 : arguments des tests qui en ont besoin (les autres sont lancés sans argument)
arguments() {
  case "$1" in
    test_traffic) echo "tests/trafic_web_ref.txt" ;;
    test_cartes)  echo "tests/cartes_web_ref.txt" ;;
    test_course)  echo "tests/course_web_ref.txt" ;;
    test_audio|test_roues|test_tangentes) echo "assets" ;;   # WFS-149 : test_tangentes (Golf, M-02)
    *) echo "" ;;
  esac
}
# RG-05 : tests à ne pas lancer (chacun avec son commentaire et le ticket qui le répare) — aucun pour l'instant
IGNORES=""

n=0; echecs=""; nb_echecs=0; debut=$(date +%s)
for exe in $(ls -1 "$BUILD"/test_* 2>/dev/null | sort); do
  [ -f "$exe" ] && [ -x "$exe" ] || continue
  nom=$(basename "$exe")
  case " $IGNORES " in *" $nom "*) echo "[IGNORÉ] $nom"; continue ;; esac
  t0=$(date +%s)
  # shellcheck disable=SC2046
  if ! "$exe" $(arguments "$nom") > "${TMPDIR:-/tmp}/$nom.log" 2>&1; then
    echo "[ÉCHEC] $nom"
    tail -n 30 "${TMPDIR:-/tmp}/$nom.log"
    echecs="$echecs $nom"; nb_echecs=$((nb_echecs + 1))
    continue   # WFS-412 : on passe au test suivant
  fi
  echo "[OK] $nom ($(( $(date +%s) - t0 )) s)"
  n=$((n + 1))
done
if [ "$n" -eq 0 ] && [ "$nb_echecs" -eq 0 ]; then echo "tests CPU : aucun test trouvé dans $BUILD"; exit 1; fi
if [ "$nb_echecs" -gt 0 ]; then
  echo "tests CPU : $n test(s) réussi(s), $nb_echecs échec(s) en $(( $(date +%s) - debut )) s"
  echo "tests CPU : tests en échec :$echecs"
  exit 1
fi
echo "tests CPU : $n test(s) réussi(s) en $(( $(date +%s) - debut )) s"
