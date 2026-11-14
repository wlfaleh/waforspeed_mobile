## Agent : Claude, agent dev3d (01Us6uCxU3)

**Ticket** : WFS-499 — règles : `regles-de-gestion/WFS-499.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : au garage, le joueur voit quelles voitures sont bloquées, leur prix en étoiles (avec l'équivalent en médailles et en euros), les achète avec une confirmation, et peut essayer une fois chaque voiture bloquée, en français et en anglais.
- **Ce qui change** :
  - **Garage, voiture bloquée** (sauvegarde active, vendue, ni gratuite ni achetée) : cadenas sous son point (vignette) ; carte « VOITURE BLOQUÉE · cadenas · 2 200 ★ · ≈ 3 or + 1 argent + 1 bronze · ≈ 21,78 € » au-dessus du panneau, à droite ; boutons **« Acheter 2 200 ★ »**, **« Essai gratuit »** (liseré cyan) ou **« Essai déjà fait »** (inactif), **« Retour »** : mêmes tailles qu'avant (200 × 52 et 200 × 46 en 720 p, RG-06), écart réduit de 12 à 8 px quand il y a 3 boutons. Voiture gratuite ou possédée : aucun changement (Choisir / Retour).
  - **Solde d'étoiles** toujours écrit **en haut à droite** du garage (RG-01) ; il était au-dessus des boutons (WFS-489).
  - **Fenêtre de confirmation** (RG-02) : « Acheter la Draven Vantor GT pour 2 200 ★ ? », équivalents, solde ; Annuler / Confirmer. Solde insuffisant : « Il te manque N ★ », **pas de Confirmer**, rien de débité. Confirmer : débit une seule fois (`boutique::acheterVoiture`), cadenas retiré, voiture choisie (`car=` dans la sauvegarde + voiture de la partie).
  - **Essai gratuit** (RG-03) : « Choisir » sans garder la voiture dans la sauvegarde → terrain → course ; l'essai est noté au départ (WFS-498). **Écran de fin de l'essai** : « Essai gratuit terminé » + **« Acheter N ★ »** en haut à droite (au-dessus des panneaux de fin de tous les modes, 4:3 compris), même fenêtre de confirmation. Pas d'essai en duel.
  - **Garde-fou** (RG-03 « une voiture bloquée ne peut pas partir en course autrement ») : au compte à rebours d'une course avec une voiture bloquée dont l'essai est fait (« Rejouer », « Recommencer », duel), retour au garage sur cette voiture.
  - **Sans sauvegarde active** (bancs des autres tickets, parcours de la CI) : aucune voiture bloquée, garage et courses comme avant.
  - RG-04 : WFS-494 n'est pas fusionné : aucune fenêtre de skin offerte.
  - **FR / EN** (RG-05) : `garage.bloquee`, `garage.essai`, `garage.essai_fait`, `garage.achat_titre`, `garage.achat_question`, `garage.confirmer`, `garage.annuler`, `fin.essai_termine` ; séparateur des milliers « 2 200 » / « 2,200 », « 21,78 € » / « €21.78 », « 3 gold + 1 silver + 1 bronze ».
  - **Fichiers** : `src/ui/garage_achat.*` (panneau), nouveaux `src/ui/garage_achat_voiture.cpp` (fenêtre, écran de fin, garde-fou : 2 petits systèmes, 205 pour les touchers avant les modes et 445 pour le dessin après eux : **aucun fichier des modes ni de `src/ui/ecrans/**` touché**, pas de conflit avec WFS-537), `src/ui/garage_achat_texte.cpp` (textes des prix), `src/ui/garage_voitures_banc.cpp` (banc), `tests/test_garage_voitures.cpp` ; `assets/textes/fr.txt`, `en.txt`. Accroches : `garage.cpp` (+11 / −1), `garage_commun.h` (zone `essai`), `garage_couleur.cpp` (cadenas sorti de l'espace anonyme pour être partagé, code inchangé), `garage_achat_prix.cpp` (`voitureBloquee`), `desktop_main.cpp` (1 ligne), `CMakeLists.txt` (test), `garage.h` (déclaration du banc).
- **Comment c'est vérifié** :
  - **Tests CPU** : `bash tools/qa/tests_cpu.sh build` sur la branche synchronisée avec `main` : voir `qa/pr/WFS-499/tests-cpu.log` (**133 tests réussis**, dont `test_garage_voitures`, 24 cas : sans sauvegarde aucune voiture bloquée ; 3 gratuites ; retirées jamais bloquées ; achat refusé / fait / déjà possédé ; essai une seule fois ; relance ; textes FR / EN).
  - **Parcours RG-07** (banc `WFS_BANC=garage-voitures`, vrais touchers, vraie sauvegarde) : 208 → M4 bloquée (1 000 ★) → Acheter : « Il te manque 1 200 ★ », rien de débité → **Essai gratuit** → course → écran de fin « Acheter 2 200 ★ » → Menu → garage : « Essai déjà fait » → course demandée avec la M4 : **retour au garage** → 3 000 ★ : Acheter → **Confirmer** : 800 ★, M4 choisie → Rouge (gratuite) → Choisir → Corsa : essai → **achat depuis l'écran de fin** (550 ★) → course M4 de jour et de nuit → **relance** : M4 et Corsa gardées, Corsa choisie → anglais : Golf, « You need 560 more ★ », « Confirm », écran de fin « Buy 600 ★ ». **0 écart** en 1024 × 768 (journal du dernier lancement) ; en 1280 × 720 le parcours complet a tourné avec les mêmes contrôles (captures ci-dessous).
  - Banc `garage-couleurs` (WFS-489) avant / après : seule différence voulue, le solde passe en haut à droite et les cadenas des voitures bloquées apparaissent sous les points (sauvegarde active dans ce banc).
- **Limites et risques connus** :
  - Le bouton « Acheter » de l'écran de fin est posé **par-dessus** l'écran de fin du mode (en haut à droite) : si WFS-537 ou un autre ticket ajoute un élément en haut à droite de l'écran de fin, il faudra vérifier qu'ils ne se chevauchent pas.
  - Le garde-fou ramène au garage au compte à rebours : un « Rejouer » après l'essai ouvre donc le garage au lieu de relancer la course (voulu par RG-03).
  - Pas de 2400 × 1080 dans ces captures (bancs très lents sous lavapipe) : la mise en page suit l'échelle `k = H / 720` comme le reste du garage ; 16:9 (1280 × 720) et 4:3 (1024 × 768) capturés.
  - Non vérifié sur téléphone (APK de `main` après fusion) ; manette : focus posé sur les nouveaux boutons (Essai, fenêtre, écran de fin), non essayé avec une vraie manette.

## Captures pour le lead dev (42 après)

Dossier : [`qa/pr/WFS-499`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `garage-couleurs-en-golf-magenta-apercu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/avant-garage-couleurs-en-golf-magenta-apercu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-couleurs-en-golf-magenta-apercu.jpg?raw=true) |
| `garage-couleurs-en-golf` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/avant-garage-couleurs-en-golf.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-couleurs-en-golf.jpg?raw=true) |
| `garage-couleurs-en-il-manque` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/avant-garage-couleurs-en-il-manque.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-couleurs-en-il-manque.jpg?raw=true) |
| `garage-couleurs-en-toit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/avant-garage-couleurs-en-toit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-couleurs-en-toit.jpg?raw=true) |
| `garage-couleurs-p208-debut` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/avant-garage-couleurs-p208-debut.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-couleurs-p208-debut.jpg?raw=true) |
| `garage-couleurs-p208-defile` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/avant-garage-couleurs-p208-defile.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-couleurs-p208-defile.jpg?raw=true) |
| `4x3-garage-voitures-course-m4-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-4x3-garage-voitures-course-m4-jour.jpg?raw=true) |
| `4x3-garage-voitures-course-m4-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-4x3-garage-voitures-course-m4-nuit.jpg?raw=true) |
| `4x3-garage-voitures-en-confirmation` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-4x3-garage-voitures-en-confirmation.jpg?raw=true) |
| `4x3-garage-voitures-en-fin-essai-confirmation` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-4x3-garage-voitures-en-fin-essai-confirmation.jpg?raw=true) |
| `4x3-garage-voitures-en-fin-essai` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-4x3-garage-voitures-en-fin-essai.jpg?raw=true) |
| `4x3-garage-voitures-en-golf-bloquee` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-4x3-garage-voitures-en-golf-bloquee.jpg?raw=true) |
| `4x3-garage-voitures-en-golf-essai-fait` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-4x3-garage-voitures-en-golf-essai-fait.jpg?raw=true) |
| `4x3-garage-voitures-en-il-manque` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-4x3-garage-voitures-en-il-manque.jpg?raw=true) |
| `4x3-garage-voitures-fin-essai-achetee` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-4x3-garage-voitures-fin-essai-achetee.jpg?raw=true) |
| `4x3-garage-voitures-fin-essai-confirmation` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-4x3-garage-voitures-fin-essai-confirmation.jpg?raw=true) |
| `4x3-garage-voitures-fin-essai-m4` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-4x3-garage-voitures-fin-essai-m4.jpg?raw=true) |
| `4x3-garage-voitures-m4-bloquee` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-4x3-garage-voitures-m4-bloquee.jpg?raw=true) |
| `4x3-garage-voitures-m4-confirmation` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-4x3-garage-voitures-m4-confirmation.jpg?raw=true) |
| `4x3-garage-voitures-m4-debloquee` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-4x3-garage-voitures-m4-debloquee.jpg?raw=true) |
| `4x3-garage-voitures-m4-essai-fait` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-4x3-garage-voitures-m4-essai-fait.jpg?raw=true) |
| `4x3-garage-voitures-m4-il-manque` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-4x3-garage-voitures-m4-il-manque.jpg?raw=true) |
| `4x3-garage-voitures-p208` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-4x3-garage-voitures-p208.jpg?raw=true) |
| `4x3-garage-voitures-relance-corsa` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-4x3-garage-voitures-relance-corsa.jpg?raw=true) |
| `garage-voitures-course-m4-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-voitures-course-m4-jour.jpg?raw=true) |
| `garage-voitures-course-m4-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-voitures-course-m4-nuit.jpg?raw=true) |
| `garage-voitures-en-confirmation` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-voitures-en-confirmation.jpg?raw=true) |
| `garage-voitures-en-fin-essai-confirmation` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-voitures-en-fin-essai-confirmation.jpg?raw=true) |
| `garage-voitures-en-fin-essai` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-voitures-en-fin-essai.jpg?raw=true) |
| `garage-voitures-en-golf-bloquee` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-voitures-en-golf-bloquee.jpg?raw=true) |
| `garage-voitures-en-golf-essai-fait` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-voitures-en-golf-essai-fait.jpg?raw=true) |
| `garage-voitures-en-il-manque` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-voitures-en-il-manque.jpg?raw=true) |
| `garage-voitures-fin-essai-achetee` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-voitures-fin-essai-achetee.jpg?raw=true) |
| `garage-voitures-fin-essai-confirmation` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-voitures-fin-essai-confirmation.jpg?raw=true) |
| `garage-voitures-fin-essai-m4` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-voitures-fin-essai-m4.jpg?raw=true) |
| `garage-voitures-m4-bloquee` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-voitures-m4-bloquee.jpg?raw=true) |
| `garage-voitures-m4-confirmation` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-voitures-m4-confirmation.jpg?raw=true) |
| `garage-voitures-m4-debloquee` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-voitures-m4-debloquee.jpg?raw=true) |
| `garage-voitures-m4-essai-fait` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-voitures-m4-essai-fait.jpg?raw=true) |
| `garage-voitures-m4-il-manque` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-voitures-m4-il-manque.jpg?raw=true) |
| `garage-voitures-p208` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-voitures-p208.jpg?raw=true) |
| `garage-voitures-relance-corsa` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-499/qa/pr/WFS-499/apres-garage-voitures-relance-corsa.jpg?raw=true) |
