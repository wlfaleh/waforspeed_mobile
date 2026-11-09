#!/usr/bin/env bash
# WaForSpeed — captures d'un groupe du banc d'essai PC (WFS-277) : le même script sert à build.yml (groupes lancés à chaque envoi)
# et à matin.yml (tous les groupes, chaque matin à 07:00 heure de Paris). Usage, depuis la racine du dépôt, jeu compilé dans build/ :
#   bash tools/qa/banc_groupe.sh <groupe>      → captures/ (artefact captures-<groupe>)
# Code de sortie : celui du banc (3 = un écart mesuré) ; groupe inconnu = 2. Contenu repris tel quel de build.yml (WFS-69 à WFS-256).
set -e
GROUPE=${1:?groupe manquant}
mkdir -p captures
case "$GROUPE" in
  partie1a|partie1b|partie2a|partie2b|partie2c|vitrines|nuit)   # vitrines : WFS-13, 16 voitures ajoutées ; nuit : WFS-09
    WFS_BANC=$GROUPE ./build/wfs_bench captures assets/
    # WFS-136 (accroche) : tracé 2 forcé sur les 4 cartes (rond-point, feu rouge, épingle, autoroute) ; un écart = rouge
    if [ "$GROUPE" = vitrines ]; then mkdir -p trace && WFS_BANC=trace ./build/wfs_bench trace assets/ && cp trace/trace-*.png captures/; fi ;;
  texte)
    # planche de test du texte (WFS-26) en 1280x720 et 2400x1080
    mkdir -p texte && WFS_BANC=texte WFS_TEXTE_ESSAI=1 ./build/wfs_bench texte assets/ > /dev/null && cp texte/course-cam0-0.png captures/texte-essai-1280.png
    WFS_BANC=texte WFS_TEXTE_ESSAI=1 WFS_TAILLE=2400x1080 ./build/wfs_bench texte assets/ > /dev/null && cp texte/course-cam0-0.png captures/texte-essai-2400.png
    mkdir -p sauv && WFS_BANC=sauvegarde ./build/wfs_bench sauv assets/ && WFS_BANC=sauvegarde ./build/wfs_bench sauv assets/ && cp sauv/fin-record.png sauv/sauvegarde-relue.png captures/   # WFS-24 : deux lancements
    # WFS-22 : manette (course pilotée par des entrées injectées, focus des menus, pause) ; journal du banc gardé
    mkdir -p manette && WFS_BANC=manette ./build/wfs_bench manette assets/ 2>&1 | grep -i "manette" | tee captures/manette-journal.txt && cp manette/manette-*.png captures/
    WFS_BANC=vie ./build/wfs_bench captures assets/   # WFS-71 (job vie jusqu'à WFS-106)
    # WFS-183 (accroche, RG-02, ANO-94) : parcours du duel en 1280x720 (salon-derniers, duel-resultat-4, salon-hors-ligne)
    mkdir -p menu1280 && WFS_BANC=menu2 ./build/wfs_bench menu1280 assets/ > /dev/null && cp menu1280/salon-derniers.png menu1280/duel-resultat-4.png menu1280/salon-hors-ligne.png captures/
    WFS_BANC=course5 ./build/wfs_bench captures assets/ ;;   # WFS-110 : modes-choix, course5-grille, course5-course, course5-fin
  ecrans-2400)
    # WFS-35 / WFS-49 : écran de chargement en 2400x1080, pris dans un lancement sans planche de texte (ANO-10)
    mkdir -p chargement && WFS_BANC=chargement WFS_TAILLE=2400x1080 ./build/wfs_bench chargement assets/ > /dev/null
    for f in chargement/chargement-*.png; do cp "$f" "captures/$(basename "$f" .png)-2400.png"; done
    mkdir -p hud && WFS_BANC=hud WFS_TAILLE=2400x1080 ./build/wfs_bench hud assets/ && cp hud/hud-course.png captures/hud-2400.png && cp hud/encoche-hud.png captures/encoche-hud-2400.png   # WFS-27, WFS-52
    for f in hud/minicarte-*.png; do cp "$f" "captures/$(basename "$f" .png)-2400.png"; done   # WFS-173 : minicarte-checkpoint-2400, minicarte-ligne-2400
    mkdir -p fluidite && WFS_BANC=fluidite ./build/wfs_bench fluidite assets/ && cp fluidite/fluidite.txt fluidite/fluidite-*-0[0-5].png captures/   # WFS-221 (accroche) : à-coups avant / après (1280x720)
    mkdir -p camera && WFS_BANC=camera WFS_TAILLE=2400x1080 ./build/wfs_bench camera assets/ && for f in camera/camera-*.png; do cp "$f" "captures/$(basename "$f" .png)-2400.png"; done ;;   # WFS-66
  menu1-2400)   # WFS-106 : 1re moitié du groupe menu en 2400x1080 (menu, appuis, pause, garage, terrain)
    mkdir -p menu && WFS_BANC=menu1 WFS_TAILLE=2400x1080 ./build/wfs_bench menu assets/ && cp menu/menu-0.png captures/menu-1.png   # WFS-37
    for f in menu/garage-*.png menu/terrain-0.png menu/terrain-1.png menu/terrain-rouler.png menu/parcours-*.png; do cp "$f" "captures/$(basename "$f" .png)-2400.png"; done   # WFS-11, WFS-41 ; WFS-135 (ANO-55) : parcours-mode / -terrain / -course
    # WFS-135 (ANO-55) : parcours complet (WFS-126) jusqu'à la fin et au retour à l'accueil ; un écart = job rouge
    mkdir -p parcours && WFS_BANC=parcours ./build/wfs_bench parcours assets/ && cp parcours/parcours-fin.png parcours/parcours-retour-menu.png captures/ ;;
  menu2-2400)   # WFS-106 : 2e moitié du groupe menu en 2400x1080 (réglages, salon du duel)
    mkdir -p menu && WFS_BANC=menu2 WFS_TAILLE=2400x1080 ./build/wfs_bench menu assets/
    for f in menu/reglages-menu.png menu/reglages-pause.png menu/menu-duel.png menu/salon-*.png menu/duel-reseau-course.png menu/duel-resultat-4.png; do cp "$f" "captures/$(basename "$f" .png)-2400.png"; done ;;   # WFS-68, WFS-83 ; WFS-122 (accroche) : duel-resultat-4
  conduite-chocs)   # WFS-252 : bancs de conduite (WFS-241) et des chocs (WFS-256) sortis d'ecrans-2400 (≈ 65 min au run #864, limite 75 min)
    mkdir -p conduite && WFS_BANC=conduite ./build/wfs_bench conduite assets/ && cp conduite/conduite.txt conduite/conduite-*.png captures/   # WFS-241 (accroche) : banc de conduite, vidéos épingle / rond-point (1280x720)
    mkdir -p chocs && WFS_BANC=chocs ./build/wfs_bench chocs assets/ && cp chocs/chocs.txt chocs/chocs-*.png captures/   # WFS-256 (accroche) : chocs avec le décor des 4 cartes
    ;;
  realisme-1|realisme-2)   # WFS-148 ; WFS-251 : une partie du banc ; la texture d'essai KTX2 n'est lue que par la partie 2
    partie=${GROUPE#realisme-}
    if [ "$partie" = 2 ]; then bash tools/textures/ci_textures.sh captures || exit 1; fi   # WFS-149 : KTX2 d'essai, rapport PSNR
    WFS_BANC=realisme WFS_REALISME_PARTIE=$partie ./build/wfs_bench captures assets/ ;;   # morceaux : captures/realisme-partie-N.morceaux
  complements-a|complements-b|complements-c)   # WFS-352 (RG-03) : bancs absents des bancs du matin jusque-là ; un écart (code 3) = rouge
    case "$GROUPE" in complements-a) liste="qualite" ;; complements-b) liste="lod perf" ;; *) liste="cuisson cgu paquets virages" ;; esac   # essai local : cuisson 10 min, cgu 3, paquets 3, virages > 9
    for b in $liste; do
      mkdir -p "comp-$b"
      code=0; WFS_BANC=$b ./build/wfs_bench "comp-$b" assets/ > "captures/$b-journal.txt" 2>&1 || code=$?
      grep -E "ÉCART|ERREUR|écart|erreur" "captures/$b-journal.txt" | tail -20 || true
      # images et rapports du banc, préfixés par son nom (aucune collision entre bancs) ; sous-dossiers aplatis
      find "comp-$b" -type f \( -name '*.png' -o -name '*.txt' \) | while read -r f; do
        rel=${f#"comp-$b"/}; cp "$f" "captures/$b-${rel//\//-}"
      done
      echo "banc $b : code $code"
      [ "$code" = 0 ] || exit "$code"
    done ;;
  *) echo "::error::groupe du banc inconnu : $GROUPE"; exit 2 ;;
esac
ls -la captures
