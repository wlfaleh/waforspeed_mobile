# WFS-79 — Son iOS (sortie Core Audio)
Épic : E6 (+ accroche E7 dans `ios/ios.cmake`) · Priorité : P4 Basse · Dépend de : WFS-22 ✅, WFS-10 ✅, WFS-76 ✅

## Besoin (qui, quoi, pourquoi)
Sur iOS, `audioOutStart` est le bouchon du banc d'essai (`#else` de `src/audio/audio_out_android.cpp`, renvoie 0) : aucun
son. Tout le son (moteur WFS-10, bruitages WFS-28, musique et ambiances WFS-76) est déjà calculé par `AudioSystem` ; il
manque seulement la sortie de la plateforme.

## Règles de gestion
- RG-01 : nouveau fichier `src/audio/audio_out_ios.cpp` (C++ pur, API C d'AudioToolbox, pris par le `GLOB` de CMake),
  sous `#if defined(__APPLE__)` : unité `RemoteIO` (`kAudioUnitSubType_RemoteIO`), flux **mono flottant 32 bits** à
  48 000 Hz demandés, même contrat que `audio_out.h` : le rappel appelle `fill(out, frames)` ; `audioOutStart` renvoie la
  fréquence obtenue, 0 en cas d'échec (le jeu continue sans son, message au journal).
- RG-02 : le bouchon de `audio_out_android.cpp` devient `#elif !defined(__APPLE__)` (1 ligne) pour ne pas définir deux fois
  les fonctions.
- RG-03 : session audio en catégorie « ambiante » (respecte le bouton silencieux de l'iPhone, se mélange à la musique de
  l'utilisateur) ; passage en arrière-plan = `AudioOutputUnitStop`, retour = `AudioOutputUnitStart` si c'est simple à
  brancher par l'arrêt / redémarrage existant de `AudioSystem` ; sinon laisser tourner (jeu en pause, silence).
- RG-04 : aucune modification d'`audio_system.*`, des synthés, ni des volumes : même mixage que sur Android.
- RG-05 (zone) : `src/audio/**` ; accroche ≤ 3 lignes dans `ios/ios.cmake` (`-framework AudioToolbox`, et
  `AVFAudio` seulement si nécessaire) annoncée au Journal. Ne pas toucher `ios_main.mm` ni le reste d'`ios.cmake` (WFS-78).

## À réutiliser
- Natif : `audio_out_android.cpp` (structure, format mono flottant, gestion d'échec), `AudioSystem::init` (appel unique).
- Jeu web (`wlfaleh/waaps`, `waforspeed/`) : **rien à réutiliser** (le web passe par WebAudio ; les sons sont déjà portés).

## Critères d'acceptation
- [ ] run `ios.yml` vert ; journal du simulateur : `audio iOS : 48000 Hz` (ou la fréquence obtenue) puis, après 10 s, un
      compteur de rappels > 0 (`audio iOS : n rappels`)
- [ ] CI Android verte, son Android inchangé ; captures et WAV du banc PC inchangés

## Hors périmètre
Son spatial, AirPods / changement de sortie à chaud, réglage du volume système.
