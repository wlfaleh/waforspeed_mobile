# WFS-76 — Ambiances sonores par carte et musique (menu et course)
Épic : E6 (Audio) · Priorité : P3 Moyenne · Dépend de : WFS-10 ✅, WFS-28 ✅, WFS-41 ✅

## Besoin (qui, quoi, pourquoi)
Le joueur, sur son téléphone, n'entend aujourd'hui que le moteur et les bruitages (WFS-28) : le menu, le garage et le
choix du terrain sont muets, et la ville, la plage, la campagne et le circuit sonnent tous pareil. WFS-28 a laissé
« musique, ambiance de ville/mer » hors périmètre pour un ticket dédié : c'est celui-ci. Objectif : un jeu qui « sonne »
vivant dès l'ouverture, sans fichier lourd ni licence à gérer.

## Règles de gestion
- RG-01 (musique, tout synthétisé) : une boucle **synthwave / street racing** calculée au démarrage par le code (aucun fichier
  musical, aucune licence) : tempo **110 BPM**, 4 mesures de 4 temps (≈ 8,7 s) qui bouclent sans clic ; basse en arpège
  (dents de scie filtrée passe-bas), nappe (accords lents), batterie simple (grosse caisse sur chaque temps, caisse claire
  sur 2 et 4, charleston en croches, bruit filtré). Deux variantes de la même grille : **« menu »** (sans batterie, plus douce)
  et **« course »** (avec batterie).
- RG-02 (quand) : musique « menu » sur les écrans menu, garage, terrain, réglages et pause ; musique « course » de « GO » à
  l'écran de fin ; pendant le décompte et sur l'écran de fin : musique « menu ». Passage d'une variante à l'autre par un
  **fondu de 0,8 s** (jamais de coupure sèche). Volume de la musique : **0,25** du `master` (le moteur et les bruitages
  restent devant).
- RG-03 (ambiance par carte, en course seulement) : un fond sonore en boucle, synthétisé à partir de bruit filtré
  (même principe que `noiseHit` du jeu web) : **ville** = rumeur grave (passe-bas ≈ 400 Hz) + klaxon lointain rare (une fois
  toutes les 12 à 25 s, au hasard, gain faible) ; **plage** = vagues (bruit passe-bande ≈ 800 Hz, volume modulé par une
  houle de période 6 à 9 s) + mouette rare ; **campagne** = vent doux (passe-bande lent ≈ 300 Hz) + oiseaux rares
  (bips aigus courts) ; **circuit** = foule lointaine (bruit passe-bande ≈ 1 kHz, faible). Volume : **0,15** du `master`.
  La carte se lit à l'entrée en course (identifiant de carte de `GameView` / `MapDef` ; zones de `src/world/zones.h` si utile).
- RG-04 (nuit) : la nuit, l'ambiance est **atténuée de moitié** et sans oiseaux ni mouettes ; la ville garde la rumeur.
- RG-05 (pause, fin, changement de carte) : en pause l'ambiance s'arrête en fondu (0,3 s) et la musique passe en « menu » ;
  au changement de carte (WFS-74) l'ambiance repart avec celle de la nouvelle carte, sans craquement.
- RG-06 (réglages) : le réglage existant « Son : Activé / Coupé » coupe **tout** (musique et ambiance comprises). Nouvelle
  ligne **« Musique : Activée / Coupée »** dans l'écran Réglages (défaut : Activée), enregistrée avec les autres réglages
  (`musique=` dans le fichier de WFS-41) et appliquée tout de suite. Coupée → seulement la musique ; l'ambiance reste.
- RG-07 (thread audio) : tout est calculé **au démarrage, hors du thread audio** (comme `SfxMixer::build`) ; le thread
  audio ne fait que lire les tampons et mélanger (aucune allocation, aucun verrou), sinon le son craque sur téléphone.
  Mémoire totale des tampons ≤ 8 Mo (mono, à la fréquence de la sortie).
- RG-08 (banc d'essai PC, pas de carte son) : chaque changement écrit au journal une ligne `MUSIQUE menu|course|coupée` et
  `AMBIANCE ville|plage|campagne|circuit|nuit|coupée` ; le banc audio enregistre hors ligne `musique-menu.wav`,
  `musique-course.wav` et `ambiance-<carte>.wav` (10 s chacun) et trace leur forme d'onde en PNG (comme `moteur-courbe.png`
  de WFS-10) pour vérifier l'absence de coupure ou de saturation (crête < 1,0).
- RG-09 (zone) : nouveaux fichiers `src/audio/musique.*` et `src/audio/ambiance.*` ; branchement dans
  `src/audio/audio_system.cpp` / `audio_out_android.cpp` (zone E6). Hors zone, accroches annoncées au Journal :
  ligne « Musique » dans `src/ui/ecrans/reglages.cpp` (E8, ≤ 10 lignes, même tableau que « Son ») ; si l'écran courant ou
  la carte ne sont pas lisibles depuis l'audio, une accroche ≤ 10 lignes dans `src/game/game.cpp`. Rien d'autre.

## À réutiliser
- **Natif (obligatoire, ne pas refaire)** : sortie AAudio et thread de mélange de WFS-10 (`audio_out*.cpp`,
  `audio_system.cpp`) ; file sans verrou, voix et calcul hors thread de `SfxMixer` (`src/audio/sfx.*`, `sfx_synth.h` :
  générateurs de bruit, filtres et enveloppes déjà écrits — les reprendre pour la batterie, les vagues et le vent) ;
  `setMuted` / réglage « Son » (`src/ui/ecrans/reglages.cpp`, fichier `son=`) ; banc audio `benchAudioScene` (WAV + PNG).
- **Jeu web** (`wlfaleh/waaps`, `waforspeed/js/audio/sfx.js`) : `noiseHit` (bruit filtré passe-bande + enveloppe, base des
  vagues, du vent, de la foule et de la caisse claire), `beep` (oscillateur triangle + enveloppe, base des oiseaux et de la
  nappe), `setMuted` et le gain `sfxGain = 0,5` (même `master`). **Le jeu web n'a ni musique ni ambiance** : rien d'autre à
  réutiliser, la grille musicale est à écrire. Pas de fichier musical téléchargé (licence, poids de l'APK).

## Critères d'acceptation (banc PC + téléphone)
- [ ] Journal du banc : `MUSIQUE menu` au menu, `MUSIQUE course` à « GO », `MUSIQUE menu` en pause et à l'écran de fin ;
      `AMBIANCE ville` puis `AMBIANCE plage` (ou autre carte) après un changement de carte ; `AMBIANCE nuit` la nuit.
- [ ] Fichiers du banc : `musique-menu.wav`, `musique-course.wav`, `ambiance-ville.wav`, `ambiance-plage.wav`,
      `ambiance-campagne.wav`, `ambiance-circuit.wav` + PNG de forme d'onde : aucune coupure à la boucle, crête < 1,0.
- [ ] Capture `reglages-musique` : ligne « Musique » lisible dans la zone sûre, à côté de « Son » ; « Coupée » → journal
      `MUSIQUE coupée`, l'ambiance continue ; « Son : Coupé » → tout se tait.
- [ ] Téléphone (APK de `main`) : musique au menu, ambiance différente en ville et à la plage, pas de craquement.
- [ ] Autres captures et tests inchangés.

## Hors périmètre
- Vraie musique enregistrée, radio, plusieurs morceaux, curseurs de volume séparés.
- Pluie (WFS-18, météo, pas encore faite) : son de pluie au ticket météo.
- Son 3D positionnel des voitures du trafic.
