#!/usr/bin/env bash
# WaForSpeed — QA automatique sur émulateur Android (lancé par la CI).
# Installe l'APK, lance le jeu en paysage, joue un scénario (accélérer, freiner, braquer, nitro),
# prend des captures et récupère le journal. Échoue si l'appli plante ou ne démarre pas le moteur.
set -u
APK="$1"; OUT="${2:-qa-sortie}"
mkdir -p "$OUT"
PKG=com.waforspeed.game
log() { echo "[QA] $*" | tee -a "$OUT/deroulement.txt"; }
shot() { adb exec-out screencap -p > "$OUT/$1.png" && log "capture $1"; }
alive() { adb shell pidof $PKG > /dev/null 2>&1; }

adb wait-for-device
adb shell settings put system accelerometer_rotation 0
adb shell settings put system user_rotation 1           # paysage
adb shell wm size > "$OUT/ecran.txt"
log "installation"; adb install -r -g "$APK" >> "$OUT/deroulement.txt" 2>&1 || { log "ÉCHEC installation"; exit 1; }
adb logcat -c
log "lancement"; adb shell am start -W -n $PKG/android.app.NativeActivity >> "$OUT/deroulement.txt" 2>&1
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
adb shell input swipe $sx $sy $((sx + W * 10 / 100)) $sy 2000 & sleep 1; shot 05-braquage; wait
log "freinage 3 s"
adb shell input motionevent DOWN $bx $by; sleep 3; shot 06-freinage; adb shell input motionevent UP $bx $by
sleep 6
shot 07-fin
adb logcat -d -s WaForSpeed:* AndroidRuntime:E DEBUG:* libc:F vulkan:* > "$OUT/logcat.txt" 2>&1
grep -E "QA |ERREUR|Vulkan|GPU|Swapchain|Cibles" "$OUT/logcat.txt" | tee -a "$OUT/deroulement.txt" | head -40

# verdict technique
ok=1
grep -q "QA pret" "$OUT/logcat.txt" || { log "ÉCHEC : le moteur ne s'est pas initialisé"; ok=0; }
grep -qE "Fatal signal|FATAL EXCEPTION|SIGSEGV|SIGABRT" "$OUT/logcat.txt" && { log "ÉCHEC : plantage détecté"; ok=0; }
alive || { log "ÉCHEC : l'appli s'est arrêtée"; ok=0; }
[ $ok = 1 ] && log "RÉSULTAT TECHNIQUE : OK" || log "RÉSULTAT TECHNIQUE : ÉCHEC"
[ $ok = 1 ]
