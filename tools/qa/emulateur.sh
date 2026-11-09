#!/usr/bin/env bash
# WaForSpeed — QA automatique sur émulateur Android (lancé par la CI).
# Installe l'APK, lance le jeu en paysage, joue un scénario, prend des captures et récupère le journal.
# Échoue si l'appli plante ou ne démarre pas le moteur.
# usage : emulateur.sh <apk> [dossier] [scénario]
#   conduite (par défaut) : accélérer, nitro, braquer, freiner (captures 01-demarrage … 07-fin)
#   mvp (WFS-56, WFS-58)  : parcours du MVP par appuis : chargement → menu → Course solo → décompte → course (HUD)
#                           → fin → Rejouer → fin → Menu (captures 01-chargement … 10-retour-menu) ; chaque étape attend
#                           son événement dans le journal du jeu (logcat) avec un délai max, et échoue franchement sinon
set -u
APK="$1"; OUT="${2:-qa-sortie}"; SCENARIO="${3:-conduite}"
# chaque commande adb est limitée dans le temps : le test ne peut plus rester bloqué
ADB="$(command -v adb)"
adb() { timeout 60 "$ADB" "$@"; }
mkdir -p "$OUT"
PKG=com.waforspeed.game
log() { echo "[QA] $*" | tee -a "$OUT/deroulement.txt"; }
shot() { adb exec-out screencap -p > "$OUT/$1.png" && log "capture $1"; }
alive() { adb shell pidof $PKG > /dev/null 2>&1; }

timeout 120 "$ADB" wait-for-device
adb shell settings put system accelerometer_rotation 0
adb shell settings put system user_rotation 1           # paysage
adb shell wm size > "$OUT/ecran.txt"
log "installation"; adb install -r -g "$APK" >> "$OUT/deroulement.txt" 2>&1 || { log "ÉCHEC installation"; exit 1; }
adb logcat -c
# scénario mvp : pas de bulle Android « Viewing full screen » (elle prend le focus : le jeu ne se redessine plus et
# les appuis n'arrivent pas au jeu)
[ "$SCENARIO" = mvp ] && adb shell settings put secure immersive_mode_confirmations confirmed
if [ "$SCENARIO" = mvp ]; then
  # WFS-58 RG-05 : pas de réduction d'écran (`wm size`) : avec ou sans changement de densité, Android passe l'appli en
  # mode de compatibilité et pose la bulle « Tap to restart this app for a better view » sur la moitié droite de l'écran
  # (essais 2 et 3, QA WFS58-1). Le rendu allégé du GPU logiciel vient de WFS-59 (rendu à 25 %).
  log "écran : $(adb shell wm size | tail -1 | tr -d '\r') (taille d'origine, pas de wm size)"
  # journal du jeu lu en continu (WFS-58 RG-01)
  LIVE="$OUT/journal-jeu.txt"; : > "$LIVE"
  (timeout 2400 "$ADB" logcat -v time -s WaForSpeed:I > "$LIVE" 2>/dev/null &)
  log "lancement (scénario $SCENARIO, extra qa_simu)"
  adb shell am start -n $PKG/.JeuActivity --ez qa_simu true >> "$OUT/deroulement.txt" 2>&1
else
  log "lancement (scénario $SCENARIO)"; adb shell am start -n $PKG/.JeuActivity >> "$OUT/deroulement.txt" 2>&1
fi
if [ "$SCENARIO" = mvp ]; then
  # ---------------------------------------------------------------- scénario « parcours MVP » (WFS-56, WFS-58)
  # Chaque étape attend sa ligne du journal du jeu (motifs exacts des WFS_LOG : game.cpp « Écran : a → b »,
  # menu.cpp « menu : « Course solo » », course_system.cpp « course : nouvelle course » / « course : temps écoulé »,
  # android_main.cpp « QA pret » / « QA ips=… vitesse=… » toutes les 5 s environ), jamais une durée fixe.
  MARQUE=0; ECHEC=""
  # attendre <nom> <motif grep -E> <délai max s> : cherche le motif après la dernière marque ; la marque passe sur la ligne trouvée
  # 4e argument facultatif : motif d'échec ; s'il apparaît avant le motif attendu, l'étape échoue tout de suite
  # (WFS-50 : « temps écoulé » avant une vitesse > 0 = le gaz n'a pas été reçu, l'avance automatique de fin ne compte pas)
  attendre() {
    local nom="$1" motif="$2" max="$3" contre="${4:-}" t=0 n e mort=0
    while [ $t -lt "$max" ]; do
      n=$(tail -n +$((MARQUE + 1)) "$LIVE" | grep -nE "$motif" | head -1 | cut -d: -f1)
      if [ -n "$contre" ]; then
        e=$(tail -n +$((MARQUE + 1)) "$LIVE" | grep -nE "$contre" | head -1 | cut -d: -f1)
        if [ -n "$e" ] && { [ -z "$n" ] || [ "$e" -lt "$n" ]; }; then
          log "ÉCHEC étape $nom : « $contre » trouvé avant « $motif » — ligne $((MARQUE + e)) du journal du jeu : $(sed -n "$((MARQUE + e))p" "$LIVE" | tr -d '\r')"
          ECHEC="$nom"; return 1
        fi
      fi
      if [ -n "$n" ]; then
        MARQUE=$((MARQUE + n))
        # heure de l'appareil et ligne du jeu trouvée : recoupement direct avec journal-jeu.txt (QA WFS58-2)
        log "étape $nom atteinte après ${t} s — attendu « $motif » — ligne $MARQUE du journal du jeu : $(sed -n "${MARQUE}p" "$LIVE" | tr -d '\r')"
        return 0
      fi
      # appli arrêtée = 10 s de suite sans processus (juste après « am start », le processus n'existe pas encore)
      if alive; then mort=0; else mort=$((mort + 1)); fi
      [ $mort -ge 10 ] && { log "ÉCHEC étape $nom : l'appli ne tourne plus"; ECHEC="$nom"; return 1; }
      sleep 1; t=$((t + 1))
    done
    log "ÉCHEC étape $nom (délai ${max} s)"; ECHEC="$nom"; return 1
  }
  # images : après l'événement, 2 nouvelles lignes « QA ips= » (au moins une image entière présentée), au plus 30 s
  images() {
    local t=0 n
    while [ $t -lt 30 ]; do
      n=$(tail -n +$((MARQUE + 1)) "$LIVE" | grep -c "QA ips=")
      [ "$n" -ge 2 ] && return 0
      sleep 1; t=$((t + 1))
    done
    sleep 2
  }
  # capture <nom> : capture ; deux captures consécutives identiques (même md5) = étape notée FIGÉE (RG-02)
  PREC=""
  capture() {
    shot "$1"; local m; m=$(md5sum < "$OUT/$1.png" | cut -d' ' -f1)
    [ "$m" = "$PREC" ] && log "FIGÉE : $1 identique à la capture précédente"; PREC="$m"
  }
  # étape <nom> <motif> <délai> <capture> : attente, images présentées, capture ; échec = capture de preuve puis arrêt
  etape() { if attendre "$1" "$2" "$3" "${5:-}"; then images; capture "$4"; else capture "$4"; return 1; fi; }
  # premier <délai s> <motif 1> <motif 2> … : numéro du motif trouvé le premier après la marque (0 = aucun dans le délai) ;
  # la marque ne bouge pas (l'étape suivante cherche elle-même sa ligne)
  premier() {
    local max="$1" t=0 i n meilleur best; shift
    while [ $t -lt "$max" ]; do
      best=0; meilleur=""
      i=0; for m in "$@"; do i=$((i + 1))
        n=$(tail -n +$((MARQUE + 1)) "$LIVE" | grep -nE "$m" | head -1 | cut -d: -f1)
        if [ -n "$n" ] && { [ -z "$meilleur" ] || [ "$n" -lt "$meilleur" ]; }; then meilleur=$n; best=$i; fi
      done
      [ "$best" != 0 ] && { echo "$best"; return 0; }
      sleep 1; t=$((t + 1))
    done
    echo 0
  }
  # taille : W, H de la fenêtre du jeu (paysage ; taille logique annoncée par le jeu si elle est là) et densité (pixels par dp)
  taille() {
    read W H < <(adb shell wm size | tail -1 | sed -E 's/.* ([0-9]+)x([0-9]+).*/\1 \2/' | awk '{ if ($1 < $2) print $2, $1; else print $1, $2 }')
    local fen; fen=$(grep -oE "Cibles de rendu : [0-9]+x[0-9]+ \(logique\)" "$LIVE" | tail -1 | grep -oE "[0-9]+x[0-9]+")
    [ -n "$fen" ] && { W=${fen%x*}; H=${fen#*x}; }
    DPI=$(adb shell wm density | grep -oE "[0-9]+" | tail -1); [ -n "$DPI" ] || DPI=420
    log "fenêtre du jeu ${W}x${H}, densité ${DPI} dpi"
  }
  # WFS-351 : points d'appui des CGU (src/ui/ecrans/cgu.cpp) et de l'étape des paquets (src/ui/ecrans/paquets.cpp), calculés
  # comme leur mise en page (marges de l'écran supposées nulles en paysage ; le point visé est loin des bords de chaque zone) :
  #   d = dpi/160 ; petit écran si H/d ≤ 520 ; k = min(d, H/400, W/800) (petit écran) sinon H/720 ; kb = H/720 ;
  #   CGU : pw = min(W − 32k, 1040k), px = (W − pw)/2, py = (8 ou 24)k, ph = H − 2·py, bh = 50kb, by = py + ph − 14k − bh ;
  #         case : centre du carré (px + 24k + 14k, by + bh/2) ; « Accepter » : à 30kb du bord droit (px + pw − 24k − 30kb, by + bh/2)
  #   paquets : pw = min(W − 32k, 760k), px = (W − pw)/2, ph = (300 ou 340)k, py = max(8k, (H − ph)/2), by = py + ph − 22k − bh ;
  #         « Installer » : (px + pw − 28k − 30kb, by + bh/2)
  points_cgu() {
    read CX CY AX AY IX IY < <(awk -v W="$W" -v H="$H" -v dpi="$DPI" 'BEGIN{
      d=dpi/160; cp=(H/d<=520); if (cp) { k=d; if (H/400<k) k=H/400; if (W/800<k) k=W/800 } else k=H/720; kb=H/720; bh=50*kb;
      pw=W-32*k; if (1040*k<pw) pw=1040*k; px=(W-pw)/2; py=(cp?8:24)*k; ph=H-2*py; by=py+ph-14*k-bh;
      cx=px+24*k+14*k; ax=px+pw-24*k-30*kb;
      qw=W-32*k; if (760*k<qw) qw=760*k; qx=(W-qw)/2; qh=(cp?300:340)*k; qy=(H-qh)/2; if (qy<8*k) qy=8*k; qby=qy+qh-22*k-bh;
      printf "%d %d %d %d %d %d\n", cx, by+bh/2, ax, by+bh/2, qx+qw-28*k-30*kb, qby+bh/2 }')
  }
  # cgu : la case « J'ai lu et j'accepte », puis « Accepter », comme un joueur ; chaque appui vérifié dans le journal du jeu
  cgu() {
    taille; points_cgu
    appui cgu-case $CX $CY
    attendre cgu-case "CGU \(WFS-308\) : case .* cochée" 15 || { capture 00-cgu-case; log "ÉCHEC : case des CGU non cochée (appui ${CX},${CY})"; return 1; }
    capture 00-cgu-case-cochee
    appui cgu-accepter $AX $AY
    attendre cgu-accepter "CGU \(WFS-308\) : « Accepter » \(version" 15 || { capture 00-cgu-accepter; log "ÉCHEC : « Accepter » des CGU non reçu (appui ${AX},${AY})"; return 1; }
  }
  # paquets : étape « Mise à jour et installation des paquets » (WFS-306) : proposition → « Installer » (vrai téléchargement
  # depuis la Release « paquets », WFS-302) ; paquet déjà présent ou pas de liste → l'étape finit seule
  paquets() {
    local m0=$MARQUE; MARQUE=$BASE_PAQUETS   # l'étape peut commencer avant la ligne « écran complet » du chargement
    case "$(premier 120 "paquets \(WFS-306\) : proposition" "paquets \(WFS-306\) : fin de l'étape" "Écran : course → menu")" in
      1) images; capture 01b-paquets-proposition; taille; points_cgu; appui paquets-installer $IX $IY
         etape paquets "paquets \(WFS-306\) : fin de l'étape" 300 01c-paquets-fin "paquets \(WFS-306\) : échec" || return 1 ;;
      2) log "paquets : l'étape a fini sans proposition (paquet présent ou pas de liste)"; MARQUE=$m0 ;;
      3) log "paquets : menu atteint sans étape des paquets"; MARQUE=$m0 ;;
      *) log "paquets : aucune ligne de l'étape des paquets en 120 s (on continue vers le menu)"; MARQUE=$m0 ;;
    esac
  }
  parcours() {
    # délais max : 1 440 s d'étapes + 9 × 30 s d'images = 1 710 s ≈ 28,5 min ≤ 30 min (RG-07) ; jeu ≥ 1 image/s attendu avec WFS-59
    # WFS-351 (ANO-199) : premier lancement, écran des CGU (WFS-308) avant le chargement : case puis « Accepter », comme un joueur
    # (aucun raccourci dans le jeu) ; lancement suivant (CGU déjà acceptées) : l'écran n'apparaît pas
    case "$(premier 120 "CGU \(WFS-308\) : écran ouvert" "écran complet \(police\)")" in
      1) attendre cgu "CGU \(WFS-308\) : écran ouvert" 5; images; capture 00-cgu; cgu || return 1 ;;
      2) log "CGU déjà acceptées : pas d'écran des CGU" ;;
      *) log "ni écran des CGU ni écran de chargement en 120 s" ;;
    esac
    # 1re image du jeu (RG-04) : l'écran de chargement complet (texte et barre) est écrit par chargement.cpp
    # (« chargement : première image … écran complet (police) … ») ; avant, seul le rail de la barre est dessiné (QA WFS58-5).
    # Jamais l'écran de démarrage Android ; pas de ligne « QA ips » pendant le chargement, d'où 1 s d'attente fixe.
    BASE_PAQUETS=$MARQUE
    if attendre chargement "écran complet \(police\)" 120; then sleep 1; capture 01-chargement; else capture 01-chargement; return 1; fi
    paquets || return 1                                                       # WFS-351 : étape des paquets (WFS-306)
    etape menu "Écran : course → menu" 180 02-menu || return 1               # chargement fini, menu (WFS-37)
    read W H < <(adb shell wm size | tail -1 | sed -E 's/.* ([0-9]+)x([0-9]+).*/\1 \2/' | awk '{ if ($1 < $2) print $2, $1; else print $1, $2 }')
    log "écran ${W}x${H} (paysage)"
    # WFS-50 : la fenêtre du jeu peut être plus petite que l'écran (barre de navigation de l'émulateur affichée, ex.
    # 2400x954 sur 2400x1080) ; les points d'appui suivent la taille logique annoncée par le jeu (« Cibles de rendu :
    # WxH (logique) », dernière ligne), fenêtre posée en haut à gauche de l'écran paysage. Sans cela, le gaz (0,92 H
    # de l'écran) tombait sous la fenêtre et la voiture restait à 0 km/h.
    local fen; fen=$(grep -oE "Cibles de rendu : [0-9]+x[0-9]+ \(logique\)" "$LIVE" | tail -1 | grep -oE "[0-9]+x[0-9]+")
    if [ -n "$fen" ]; then W=${fen%x*}; H=${fen#*x}; log "fenêtre du jeu ${W}x${H} (taille logique, points d'appui calculés dessus)"; fi
    coordonnees
    appui course-solo $SX $SY
    # 03 : juste après l'appui reçu (sans attendre d'images), 04 : le décompte une fois des images présentées (WFS58-3)
    if attendre course-solo "menu : « Course solo »" 60; then capture 03-appui-course-solo; else capture 03-appui-course-solo; return 1; fi
    etape decompte "course : nouvelle course" 60 04-decompte || return 1     # remise à zéro : décompte 3-2-1 (WFS-12)
    # « GO » n'est pas journalisé : gaz tenu, la voiture ne roule qu'après GO → première ligne « QA ips » à vitesse > 0
    log "gaz tenu ${gx},${gy}"; adb shell input motionevent DOWN $gx $gy
    etape go "QA ips=.* vitesse=[1-9]" 300 05-go "course : temps écoulé" || { adb shell input motionevent UP $gx $gy; return 1; }
    images; capture 06-course-hud; adb shell input motionevent UP $gx $gy
    etape fin "course : temps écoulé" 300 07-fin || return 1                # l'écran de fin s'affiche 0,9 s (temps de jeu) après
    appui rejouer $RX $RY
    etape rejouer "course : nouvelle course" 60 08-rejouer-decompte || return 1
    etape fin-2 "course : temps écoulé" 300 09-fin-2 || return 1
    appui menu $MX $MY
    etape retour-menu "Écran : course → menu" 60 10-retour-menu || return 1
  }
  # coordonnees : points d'appui calculés depuis W, H (paysage)
  coordonnees() {
  # Coordonnées calculées comme la mise en page du jeu (pixels logiques = pixels de l'écran paysage) :
  #  menu (src/ui/ecrans/menu.cpp + miseEnPage de chargement.cpp) : k = H/720 ; logo = clamp(0,14·W/k, 64, 140)·k ;
  #    x = clamp(0,06·W/k, 0, 96)·k + 16·k ; largeur = min(600·k, W − x − 16·k) ; lw = min(460·k, largeur) ;
  #    bloc = 12·k + 22·k + 2·0,82·logo ; liste = 3·70·k + 2·6·k ; total = bloc + 22·k + liste + 22·k + 44·k + 22·k + 44·k ;
  #    y0 = max(24·k, (H − total)/2) ; « Course solo » (g_zones[0]) : centre (x + 0,4·lw, y0 + bloc + 22·k + 35·k).
  #  écran de fin (drawOver de src/game/course_system.cpp) : u = H/100 ; bw = 40·u ; bx = W/2 − (1,7·bw + 2·u)/2 ;
  #    boutons à y = 70·u, hauteur 10·u ; « Rejouer » centre (bx + bw/2, 75·u) ; « Menu » centre (bx + bw + 2·u + 0,35·bw, 75·u).
  #  Exemple 2400x1080 (Pixel 6) : k = 1,5 ; Course solo ≈ (444, 541) ; Rejouer ≈ (1038, 810) ; Menu ≈ (1426, 810).
  read SX SY RX RY MX MY < <(awk -v W="$W" -v H="$H" 'function cl(v,a,b){return v<a?a:(v>b?b:v)} BEGIN{
    k=H/720; logo=cl(0.14*W/k,64,140)*k; x=cl(0.06*W/k,0,96)*k+16*k; larg=W-x-16*k; if (600*k<larg) larg=600*k;
    lw=(460*k<larg)?460*k:larg; bloc=12*k+22*k+2*0.82*logo; liste=3*70*k+2*6*k; tot=bloc+22*k+liste+22*k+44*k+22*k+44*k;
    y0=(H-tot)/2; if (y0<24*k) y0=24*k; u=H/100; bw=40*u; bx=W/2-(1.7*bw+2*u)/2;
    printf "%d %d %d %d %d %d\n", x+0.4*lw, y0+bloc+22*k+35*k, bx+bw/2, 75*u, bx+bw+2*u+0.35*bw, 75*u }')
  # accélérateur : zone de toucher x > 0,78 W hors nitro (Game::onTouch) ; appui en bas (0,84 W ; 0,92 H), sous la bulle
  # de compatibilité d'Android qui peut couvrir le centre du bouton
  gx=$((W * 84 / 100)); gy=$((H * 92 / 100))
  }
  appui() { alive || log "ATTENTION : l'appli ne tourne plus avant l'appui $1"; log "appui $1 $2,$3"; adb shell input tap "$2" "$3"; }
  parcours
  log "écran en fin de scénario : $(adb shell wm size | tail -1 | tr -d '\r')"   # WFS58-4 : taille tracée en fin de scénario
else
sleep 20
shot 01-demarrage
alive || log "ATTENTION : l'appli ne tourne plus après le démarrage"

# taille d'écran en paysage (largeur > hauteur)
read W H < <(adb shell wm size | tail -1 | sed -E 's/.* ([0-9]+)x([0-9]+).*/\1 \2/' | awk '{ if ($1 < $2) print $2, $1; else print $1, $2 }')
log "écran ${W}x${H}"
gx=$((W * 89 / 100)); gy=$((H * 74 / 100))     # accélérateur
bx=$((W * 70 / 100)); by=$((H * 80 / 100))     # frein
sx=$((W * 18 / 100)); sy=$((H * 72 / 100))     # zone volant
nx=$((W * 90 / 100)); ny=$((H * 32 / 100))     # nitro

log "accélération 8 s"
adb shell input motionevent DOWN $gx $gy; sleep 4; shot 02-acceleration-4s; sleep 4; shot 03-acceleration-8s
adb shell input motionevent UP $gx $gy
log "nitro 3 s"
adb shell input motionevent DOWN $nx $ny; sleep 3; shot 04-nitro; adb shell input motionevent UP $nx $ny
log "braquage à droite (glisser) 2 s"
(timeout 20 "$ADB" shell input swipe $sx $sy $((sx + W * 10 / 100)) $sy 2000 &) ; sleep 1; shot 05-braquage; sleep 2
log "freinage 3 s"
adb shell input motionevent DOWN $bx $by; sleep 3; shot 06-freinage; adb shell input motionevent UP $bx $by
sleep 6
shot 07-fin
fi
adb logcat -d -s WaForSpeed:* AndroidRuntime:E DEBUG:* libc:F vulkan:* > "$OUT/logcat.txt" 2>&1
grep -E "QA |ERREUR|Vulkan|GPU|Swapchain|Cibles" "$OUT/logcat.txt" | tee -a "$OUT/deroulement.txt" | head -40

# verdict technique
ok=1
grep -q "QA pret" "$OUT/logcat.txt" || { log "ÉCHEC : le moteur ne s'est pas initialisé"; ok=0; }
grep -qE "Fatal signal|FATAL EXCEPTION|SIGSEGV|SIGABRT" "$OUT/logcat.txt" && { log "ÉCHEC : plantage détecté"; ok=0; }
alive || { log "ÉCHEC : l'appli s'est arrêtée"; ok=0; }
grep -q "VK_ERROR" "$OUT/logcat.txt" && { log "ÉCHEC : erreur Vulkan (VK_ERROR) dans le journal"; ok=0; }
# WFS-58 RG-03 : une étape du parcours mvp manquée = échec franc (plus jamais « OK » si une étape manque)
[ "$SCENARIO" = mvp ] && [ -n "${ECHEC:-}" ] && { log "ÉCHEC : étape « $ECHEC » non atteinte"; ok=0; }
[ $ok = 1 ] && log "RÉSULTAT TECHNIQUE : OK" || log "RÉSULTAT TECHNIQUE : ÉCHEC"
[ $ok = 1 ]
