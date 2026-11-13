Agent : Claude, agent dev3d (01Us6uCw3C)

## Demande de l'utilisateur
Épic E20 « Le jeu en anglais » : avec un téléphone dans une autre langue que le français, aucune phrase française au garage ni dans l'écran « Mise à jour et installation des paquets » (WFS-507, `regles-de-gestion/WFS-507.md`, RC-E20-01 à 07).

## Ce qui change
- **Textes** (`assets/textes/fr.txt`, `en.txt`, mêmes clés dans les deux) :
  - garage : `garage.puissance`, `garage.zero_cent`, `garage.vmax`, `garage.note.*` (Speed, Acceleration, Handling, Power), `garage.credit` (« 3D model by … · model modified for WaForSpeed »), `garage.type.<id>` et `garage.accroche.<id>` des 20 voitures ;
  - paquets : `paquets.*` (titre, recherche, proposition, Wi-Fi / données mobiles, téléchargement, temps restant, astuce, coupure, place insuffisante, paquet abîmé, échec, « Connexion nécessaire », Télécharger / Mise à jour / Réessayer / Supprimer / Annuler / Plus tard / Installer / Continuer, bandeau du menu, annonce « prête ») et `unite.mo` (« 22 Mo » / « 22 MB ») ;
  - `assets/textes/astuces-paquets-en.txt` : les 17 astuces en anglais (lues selon la langue active).
- **Garage** (`garage_fiche.cpp`, `garage_cadre.cpp`, `garage_commun.*`) : type, accroche, caractéristiques, notes, crédit et « Chargement du modèle 3D… » par `ui::tr` ; la fiche `Fiche` ne garde que `{id, glow, paint}` (textes passés dans les tables). Marque et modèle inventés inchangés (notés comme noms propres). Unités inchangées (ch, s, km/h). `cars.h` : accroche annoncée au journal, `creditAffiche()` (phrase française en dur) remplacée par `auteurDuCredit()` (auteur et licence seuls), la phrase étant traduite par `garage.credit`.
- **Couleurs et boutons Choisir / Retour / Chargement…** (`garage_couleur.*`, `garage_achat.*`) : **non touchés ici**, traduits par **WFS-489** (accord avec sa session, clés `couleur.<id>`, `garage.choisir`…) : tant que WFS-489 n'est pas fusionné, ces 3 boutons et le nom de la couleur restent en français.
- **Paquets** (`paquets.cpp`, `paquets_etape.cpp`) : tous les textes par `ui::tr` ; noms des cartes inchangés. RG-04 : la largeur des boutons est celle du plus long des deux textes (FR, EN) → même taille dans les deux langues ; texte posé par `ui::textBoite` (police réduite de 20 % au plus, dépassement noté au banc).
- RG-03 : aucun texte de `cartes.json` n'est affiché à part le nom de la carte (nom propre, inchangé) : **format publié inchangé**.
- **Banc** `textes` étendu (`src/ui/garage_textes_banc.cpp`) + banc local `textes-garage` : paquets (banc complet de WFS-306 / 307, textes des écrans des paquets notés) puis garage (fiche Golf, fiche SF90, parcours accueil → garage → Choisir → terrain → Rouler → course → Menu → pause → Quitter → accueil), garde-fou de WFS-504.
- **Défaut corrigé dans le banc `paquets`** : à sa fin, l'étape gardait un pointeur vers sa source factice (variable locale) ; un banc lancé ensuite plantait au menu (bandeau). Le banc remet `secours_` à faux avant de rendre la main.
- `test_paquets` : lié au système de langue ; vérifie aussi « 22 MB », « Not enough space: 30 MB needed, 12 MB free », « Connection required ».

## Comment c'est vérifié
- Construction complète (aucun nouvel avertissement), **124 tests CPU verts (après synchronisation avec main)** (`tools/qa/tests_cpu.sh`).
- Banc `textes-garage` en `WFS_LANGUE=en` et `fr` (résultats ci-dessous), banc `paquets` et `garage` de `main` pour les captures « avant ».

## Limites et risques
- Sans WFS-489 : boutons Choisir / Retour / Chargement… et nom de la couleur encore en français au garage (voir ci-dessus).
- Cartes d'essai cachées (`WFS_CARTES_TEST`, jamais montrées aux joueurs) : leur description et leurs quartiers restent en français au choix du terrain (zone de WFS-505) ; non vérifiés par ce banc.
- Formats des nombres (« 6,2 s ») inchangés (hors périmètre de l'épic).
- Les messages déjà affichés (place insuffisante, échec) sont écrits dans la langue active au moment où ils apparaissent.

## Résultats des bancs (après)
- `WFS_LANGUE=en WFS_BANC=textes-garage` : paquets **0 écart du banc, 0 phrase française, 0 dépassement → VERT** (526 textes vus) ; garage **0 échec → VERT** (88 textes vus), parcours complet OK.
- `WFS_LANGUE=fr WFS_BANC=textes-garage` : paquets **VERT**, garage **VERT**.

## Captures
Tableau complet (avant `main` / après FR / après EN, 14 écrans + 26 autres) : https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-507/qa/pr/WFS-507/README.md

| Écran | Avant (main) | Après FR | Après EN |
|---|---|---|---|
| Fiche du garage | ![](https://github.com/wlfaleh/waforspeed_mobile/raw/feature/waforspeed-507/qa/pr/WFS-507/avant-garage-0.jpg) | ![](https://github.com/wlfaleh/waforspeed_mobile/raw/feature/waforspeed-507/qa/pr/WFS-507/apres-textes-fr-garage-fiche.jpg) | ![](https://github.com/wlfaleh/waforspeed_mobile/raw/feature/waforspeed-507/qa/pr/WFS-507/apres-textes-en-garage-fiche.jpg) |
| Liste des paquets (choix du terrain) | ![](https://github.com/wlfaleh/waforspeed_mobile/raw/feature/waforspeed-507/qa/pr/WFS-507/avant-paquets-terrain-telecharger.jpg) | ![](https://github.com/wlfaleh/waforspeed_mobile/raw/feature/waforspeed-507/qa/pr/WFS-507/apres-textes-fr-paquets-terrain-telecharger.jpg) | ![](https://github.com/wlfaleh/waforspeed_mobile/raw/feature/waforspeed-507/qa/pr/WFS-507/apres-textes-en-paquets-terrain-telecharger.jpg) |
| Téléchargement en cours | ![](https://github.com/wlfaleh/waforspeed_mobile/raw/feature/waforspeed-507/qa/pr/WFS-507/avant-paquets-telechargement.jpg) | ![](https://github.com/wlfaleh/waforspeed_mobile/raw/feature/waforspeed-507/qa/pr/WFS-507/apres-textes-fr-paquets-telechargement.jpg) | ![](https://github.com/wlfaleh/waforspeed_mobile/raw/feature/waforspeed-507/qa/pr/WFS-507/apres-textes-en-paquets-telechargement.jpg) |
| Erreur « pas d'Internet » | ![](https://github.com/wlfaleh/waforspeed_mobile/raw/feature/waforspeed-507/qa/pr/WFS-507/avant-paquets-terrain-sans-connexion.jpg) | ![](https://github.com/wlfaleh/waforspeed_mobile/raw/feature/waforspeed-507/qa/pr/WFS-507/apres-textes-fr-paquets-terrain-sans-connexion.jpg) | ![](https://github.com/wlfaleh/waforspeed_mobile/raw/feature/waforspeed-507/qa/pr/WFS-507/apres-textes-en-paquets-terrain-sans-connexion.jpg) |

**À fusionner après WFS-489** (ou fusionnée avant, puis WFS-489 synchronise) : je synchronise ma branche dès la fusion de WFS-489 et je refais la capture de la fiche du garage.
