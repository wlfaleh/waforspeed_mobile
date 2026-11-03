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
  # WFS-58 RG-05 : écran réduit (moins de pixels pour le GPU logiciel) ; taille naturelle de l'appareil (portrait), remise
  # à zéro en fin de script ; densité divisée d'autant
  adb shell wm size 540x1200; adb shell wm density 210
  log "écran réduit : $(adb shell wm size | tail -1 | tr -d '\r') (densité 210)"
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
  attendre() {
    local nom="$1" motif="$2" max="$3" t=0 n mort=0
    while [ $t -lt "$max" ]; do
      n=$(tail -n +$((MARQUE + 1)) "$LIVE" | grep -nE "$motif" | head -1 | cut -d: -f1)
      if [ -n "$n" ]; then MARQUE=$((MARQUE + n)); log "étape $nom atteinte après ${t} s"; return 0; fi
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
  etape() { if attendre "$1" "$2" "$3"; then images; capture "$4"; else capture "$4"; return 1; fi; }
  parcours() {
    # délais max : 1 440 s d'étapes + 9 × 30 s d'images = 1 710 s ≈ 28,5 min ≤ 30 min (RG-07) ; jeu ≥ 1 image/s attendu avec WFS-59
    # 1re image du jeu (RG-04) : « Cibles de rendu » est écrit quand l'écran de chargement commence à se dessiner (pas de
    # ligne « QA ips » pendant le chargement) ; 3 s plus tard la barre avance, jamais l'écran de démarrage Android
    if attendre chargement "Cibles de rendu" 120; then sleep 3; capture 01-chargement; else capture 01-chargement; return 1; fi
    etape menu "Écran : course → menu" 180 02-menu || return 1               # chargement fini, menu (WFS-37)
    read W H < <(adb shell wm size | tail -1 | sed -E 's/.* ([0-9]+)x([0-9]+).*/\1 \2/' | awk '{ if ($1 < $2) print $2, $1; else print $1, $2 }')
    log "écran ${W}x${H} (lu après wm size)"
    coordonnees
    appui course-solo $SX $SY
    etape course-solo "menu : « Course solo »" 60 03-appui-course-solo || return 1
    etape decompte "course : nouvelle course" 60 04-decompte || return 1     # remise à zéro : décompte 3-2-1 (WFS-12)
    # « GO » n'est pas journalisé : gaz tenu, la voiture ne roule qu'après GO → première ligne « QA ips » à vitesse > 0
    adb shell input motionevent DOWN $gx $gy
    etape go "QA ips=.* vitesse=[1-9]" 300 05-go || { adb shell input motionevent UP $gx $gy; return 1; }
    images; capture 06-course-hud; adb shell input motionevent UP $gx $gy
    etape fin "course : temps écoulé" 300 07-fin || return 1                # l'écran de fin s'affiche 0,9 s (temps de jeu) après
    appui rejouer $RX $RY
    etape rejouer "course : nouvelle course" 60 08-rejouer-decompte || return 1
    etape fin-2 "course : temps écoulé" 300 09-fin-2 || return 1
    appui menu $MX $MY
    etape retour-menu "Écran : course → menu" 60 10-retour-menu || return 1
  }
  # coordonnees : points d'appui calculés depuis W, H (lus après wm size)
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
  gx=$((W * 89 / 100)); gy=$((H * 74 / 100))     # accélérateur
  }
  appui() { alive || log "ATTENTION : l'appli ne tourne plus avant l'appui $1"; log "appui $1 $2,$3"; adb shell input tap "$2" "$3"; }
  parcours
  adb shell wm size reset; adb shell wm density reset   # RG-05 : écran de l'émulateur rétabli
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
