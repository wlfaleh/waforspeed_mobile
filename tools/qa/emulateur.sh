#!/usr/bin/env bash
# WaForSpeed — QA automatique sur émulateur Android (lancé par la CI).
# Installe l'APK, lance le jeu en paysage, joue un scénario, prend des captures et récupère le journal.
# Échoue si l'appli plante ou ne démarre pas le moteur.
# usage : emulateur.sh <apk> [dossier] [scénario]
#   conduite (par défaut) : accélérer, nitro, braquer, freiner (captures 01-demarrage … 07-fin)
#   mvp (WFS-56)          : parcours du MVP par appuis : chargement → menu → Course solo → décompte → course (HUD)
#                           → fin → Rejouer → fin → Menu (captures 01-chargement … 10-retour-menu)
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
log "lancement (scénario $SCENARIO)"; adb shell am start -n $PKG/.JeuActivity >> "$OUT/deroulement.txt" 2>&1
if [ "$SCENARIO" = mvp ]; then
  # ---------------------------------------------------------------- scénario « parcours MVP » (WFS-56)
  sleep 2; shot 01-chargement                     # écran de chargement (WFS-35) : barre de progression
  sleep 18; shot 02-menu                          # menu principal (WFS-37) après le fondu
  alive || log "ATTENTION : l'appli ne tourne plus après le démarrage"
  read W H < <(adb shell wm size | tail -1 | sed -E 's/.* ([0-9]+)x([0-9]+).*/\1 \2/' | awk '{ if ($1 < $2) print $2, $1; else print $1, $2 }')
  log "écran ${W}x${H}"
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
  appui() { alive || log "ATTENTION : l'appli ne tourne plus avant l'appui $1"; log "appui $1 $2,$3"; adb shell input tap "$2" "$3"; }
  appui course-solo $SX $SY; sleep 0.5; shot 03-appui-course-solo
  sleep 0.5; shot 04-decompte                     # décompte 3-2-1 (WFS-12)
  sleep 2.3; shot 05-go
  log "accélération 6 s"
  adb shell input motionevent DOWN $gx $gy; sleep 6; shot 06-course-hud; adb shell input motionevent UP $gx $gy
  log "attente de la fin du chrono (30 s depuis GO, sans gagner de temps)"
  sleep 31; shot 07-fin                           # écran de fin (affiché 0,9 s après la fin)
  sleep 1; appui rejouer $RX $RY; sleep 1; shot 08-rejouer-decompte
  log "attente de la fin de la 2e course (décompte 3 s + 30 s)"
  sleep 35; shot 09-fin-2
  sleep 1; appui menu $MX $MY; sleep 1.5; shot 10-retour-menu
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
[ $ok = 1 ] && log "RÉSULTAT TECHNIQUE : OK" || log "RÉSULTAT TECHNIQUE : ÉCHEC"
[ $ok = 1 ]
