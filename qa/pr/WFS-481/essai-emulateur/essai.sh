#!/usr/bin/env bash
# WFS-481 : essai de l'étape profil() de tools/qa/emulateur.sh avec un faux journal (sans téléphone), revue #379.
# Usage, depuis la racine du dépôt : D=$(mktemp -d); sed -n '52,158p' tools/qa/emulateur.sh > $D/fonctions.sh; bash qa/pr/WFS-481/essai-emulateur/essai.sh $D
D=$1; OUT=$D/sortie; mkdir -p $OUT; LIVE=$D/journal.txt; : > $LIVE
log() { echo "[QA] $*"; }
alive() { return 0; }
shot() { echo "image $1 $RANDOM" > "$OUT/$1.png"; }
APPUIS=""
appui() { log "appui $1 $2,$3"; APPUIS+="$1 "; [ "$1" = profil-valider ] && echo "[info] création du profil (WFS-481) : « C'est parti » : pseudo « Petit », plaque « PETIT » enregistrés → menu" >> $LIVE; }
sleep() { command sleep 0.01; }
source $D/fonctions.sh
MARQUE=0
# faux jeu : menu, puis l'écran de création (ligne écrite une seule fois), puis des images
{ echo "[info] Écran : course → menu"; echo "[info] QA ips=60 vitesse=0"; } >> $LIVE
attendre menu "Écran : course → menu" 5 || exit 1
{ echo "[info] création du profil (WFS-481) : écran ouvert (fr, pseudo proposé «  », plaque proposée « WFS-340 »)"
  echo "[info] création du profil (WFS-481) : points d'appui : p=2195,302 e=548,302 t=820,302 i=1636,302 valider=318,893"
  echo "[info] QA ips=60 vitesse=0"; echo "[info] QA ips=60 vitesse=0"; } >> $LIVE
( command sleep 0.5; for i in 1 2 3 4; do echo "[info] QA ips=60 vitesse=0" >> $LIVE; command sleep 0.2; done ) &
profil; code=$?
wait
echo "code de profil() = $code ; appuis : $APPUIS ; captures : $(ls $OUT | tr '\n' ' ')"
# 2e cas : profil déjà créé (aucune ligne « points d'appui ») → l'étape continue
: > $LIVE; MARQUE=0; echo "[info] Écran : course → menu" >> $LIVE; attendre menu "Écran : course → menu" 5
premier() { echo 0; }   # délai de 20 s raccourci : aucun motif trouvé
profil; echo "code de profil() sans écran = $?"
