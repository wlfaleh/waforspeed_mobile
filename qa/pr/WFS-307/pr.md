Agent : Claude, agent 3 (01UY1Vv3)

## Demande
L'utilisateur : « on installe qu'une map, ensuite l'utilisateur peut télécharger les autres maps » : depuis le choix du terrain, télécharger ou supprimer les autres cartes, jouer pendant le téléchargement ; un paquet installé, mis à jour ou supprimé sert dès la course suivante, sans relancer l'application (RG-08). Règles : `regles-de-gestion/WFS-307.md`.

## Ce qui change
- **Java** (`android/app/.../game/`) :
  - `FilePaquets` (nouveau, règle pure sans Android) : état de chaque carte et file d'attente, une carte à la fois (phase `ATTENTE`). Le téléchargement réutilise `Installation.executer` de WFS-306 (`.part`, Range, SHA-256, reprise).
  - `Paquets` : méthodes par carte (`chercherTout`, `etatCarte`, `propositionCarte`, `installerCarte`, `arreterCarte`, `supprimee`, `ralentir`). Les anciennes méthodes de WFS-306 passent par la même file.
  - `Telechargeur.pauseMs` : débit réduit (RG-03).
- **C++, machine d'états sans rendu** : `CartesPaquets` (`paquets_etape.cpp`, déclarée dans `paquets.h`).
  - Fichier vérifié sur le téléphone (`verifier_carte`), puis phase de la source.
  - Fin d'un téléchargement → la carte est « prête » et marquée changée.
  - Téléchargement : contrôles de la connexion (RG-05) et de la place (RG-07 de WFS-306).
  - Suppression : tous les formats et le `.part` ; refusée pendant le téléchargement.
  - Débit réduit : une seule demande par changement.
- **Écran du choix du terrain** (`terrain.cpp`, dessin et toucher dans `paquets.cpp`) :
  - Carte à télécharger assombrie, avec « Télécharger — X Mo » ou « Mise à jour — X Mo ».
  - Pendant le téléchargement : file d'attente, barre, octets reçus, astuce, « Connexion perdue, reprise automatique », « Annuler ».
  - Sur données mobiles, confirmation où « Télécharger » n'est pas mis en avant.
  - Sans Internet, message « Connexion nécessaire ».
  - « Supprimer — libère X Mo » avec confirmation.
  - Liste lue en entrant dans l'écran, une fois par minute au plus.
- **Menu** (`BandeauPaquets`) :
  - Message « Corniche prête pour la prochaine course » (RG-03).
  - Bandeau de WFS-306 : « … installée : prête pour la prochaine course » (RG-08).
  - Débit réduit quand la course passe sous 30 img/s (moyenne glissante).
- **Pont JNI** `paquets_android.cpp` : méthodes par carte, avec un cache de 0,1 s par carte.
- **Accroches** (accord écrit du PO du 2026-11-09 00:16:07, annoncées au journal) :
  - `game.h` : champ `Demandes::recharger`.
  - `game.cpp` : 2 lignes de `changerCarte`.
  - Seul `terrain.cpp` met `recharger = true`, quand le paquet de cette carte a changé.
  - Une autre carte est toujours reconstruite ; sur Android, `debutChargement` appelle déjà `preparerCarte`.

## Comment c'est vérifié
- **Banc `paquets`** (2400×1080) : parcours complet RG-07 avec un faux paquet de la Corniche cuit en local. **0 écart.**
  1. Corniche « Télécharger ». Sans Internet, rien ne part. Sur données mobiles, « Plus tard » ne lance rien.
  2. Wi-Fi : la Corniche entre dans la file, la barre avance.
  3. Course sur la Ville déjà chargée, **sans reconstruction**, pendant le téléchargement, qui continue.
  4. La Corniche finit pendant la course. Retour au menu : « Corniche prête ».
  5. La Corniche est installée. Course sur la Corniche **lue dans son paquet** : 9 fichiers servis par le paquet.
  6. Supprimer, puis confirmer : de nouveau « Télécharger ».
  7. Course suivante sur la Corniche : relue, construite par le code.
  8. Même carte, paquet inchangé : **pas de reconstruction** (RG-02 de WFS-41 tenue).
  - Les étapes WFS-306 du même banc restent OK.
- **Tests CPU** : `test_paquets` étendu à `CartesPaquets` (RG-01, 02, 04, 05, mise à jour, débit). `tools/qa/tests_cpu.sh` : **80/80 OK**.
- **JUnit** : `FilePaquetsTest` (file, une carte à la fois, annuler) et les tests de WFS-306 : **22 OK**.
- **JNI** : vérification de syntaxe avec les en-têtes du JDK. Seul écart : la signature connue d'`AttachCurrentThread` entre JDK et NDK.

## Limites et risques
- RG-06 (ligne « Cartes : N téléchargées, X Mo » dans les Réglages) n'est **pas** faite : `reglages.*` appartient à agent 2. Elle passe dans un ticket suivant, comme le PO l'a décidé.
- Pas d'essai réel sur le téléphone tant que `cartes.json` de WFS-302 n'est pas publié. Seule la Corniche (faux paquet local) est essayée.
- La suppression du paquet de la carte chargée est sans danger : les archives du paquet actif sont en mémoire, et la carte est reconstruite à la course suivante.

**Ticket** : WFS-307 — règles : `regles-de-gestion/WFS-307.md`

## Captures pour le lead dev
Avant (main) : le choix du terrain n'a rien des paquets, d'où « — » pour les nouvelles captures ; course-pendant (Ville, jour) et course-corniche (paquet lu) prouvent RG-03 et RG-08.

 (14 après)

Dossier : [`qa/pr/WFS-307`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-307/qa/pr/WFS-307) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `paquets-coupure` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-307/qa/pr/WFS-307/avant-paquets-coupure.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-307/qa/pr/WFS-307/apres-paquets-coupure.jpg?raw=true) |
| `paquets-course-corniche` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-307/qa/pr/WFS-307/apres-paquets-course-corniche.jpg?raw=true) |
| `paquets-course-pendant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-307/qa/pr/WFS-307/apres-paquets-course-pendant.jpg?raw=true) |
| `paquets-menu-prete` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-307/qa/pr/WFS-307/apres-paquets-menu-prete.jpg?raw=true) |
| `paquets-menu-proposition-mobile` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-307/qa/pr/WFS-307/avant-paquets-menu-proposition-mobile.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-307/qa/pr/WFS-307/apres-paquets-menu-proposition-mobile.jpg?raw=true) |
| `paquets-menu-proposition-wifi` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-307/qa/pr/WFS-307/avant-paquets-menu-proposition-wifi.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-307/qa/pr/WFS-307/apres-paquets-menu-proposition-wifi.jpg?raw=true) |
| `paquets-telechargement` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-307/qa/pr/WFS-307/avant-paquets-telechargement.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-307/qa/pr/WFS-307/apres-paquets-telechargement.jpg?raw=true) |
| `paquets-terrain-apres-suppression` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-307/qa/pr/WFS-307/apres-paquets-terrain-apres-suppression.jpg?raw=true) |
| `paquets-terrain-installee` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-307/qa/pr/WFS-307/apres-paquets-terrain-installee.jpg?raw=true) |
| `paquets-terrain-mobile` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-307/qa/pr/WFS-307/apres-paquets-terrain-mobile.jpg?raw=true) |
| `paquets-terrain-progression` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-307/qa/pr/WFS-307/apres-paquets-terrain-progression.jpg?raw=true) |
| `paquets-terrain-sans-connexion` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-307/qa/pr/WFS-307/apres-paquets-terrain-sans-connexion.jpg?raw=true) |
| `paquets-terrain-supprimer` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-307/qa/pr/WFS-307/apres-paquets-terrain-supprimer.jpg?raw=true) |
| `paquets-terrain-telecharger` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-307/qa/pr/WFS-307/apres-paquets-terrain-telecharger.jpg?raw=true) |
