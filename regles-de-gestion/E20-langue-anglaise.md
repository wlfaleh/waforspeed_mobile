# Épic E20 — Le jeu en anglais (langue du téléphone, réglage « Langue »)

Créée le 2026-11-12 par le PO, sur demande de l'utilisateur (`@po`, 2026-11-12 ≈ 22:29 UTC).
**Priorité** : avant la **sortie publique** (WFS-320 en dépend), **en parallèle** de l'ordre identité → score → voitures et couleurs → cadeau → market.

## Demande de l'utilisateur (texte fidèle)
- **Règle** : téléphone en français (`fr`, quel que soit le pays : France, Belgique, Canada, Maroc, Tunisie…) → le jeu reste en français.
  Téléphone dans **toute autre langue** → le jeu est en **anglais**.
- Un réglage **« Langue »** dans Réglages : **Automatique** (par défaut) / **Français** / **English**. Le choix est gardé dans la sauvegarde.
- **À traduire** : tous les textes du jeu : menus, HUD, garage, réglages, fin de course, messages, écran des paquets de cartes, CGU et confidentialité.
  Les **futurs écrans** (étoiles, médailles, cadeau du jour, market) sont écrits **dès le départ dans les deux langues**.
- **Hors jeu** : fiche Google Play en anglais (titre, description, captures) en plus du français.
- **Critères** : aucune phrase française visible avec un téléphone en anglais ; aucun texte qui dépasse de son bouton en anglais ;
  captures du banc en français et en anglais (menu, garage, course, fin de course, réglages).

## Découpage (≤ 1 jour chacun, une session chacun)
| Ticket | Contenu | Zone | Dépend de | Agent |
|---|---|---|---|---|
| WFS-504 | Système de langue : tables FR / EN, détection de la langue du téléphone, réglage « Langue », sauvegarde, garde-fous (phrase française, texte qui dépasse), Réglages traduits | E20 (exception PO : `src/ui/text.*`, `src/ui/langue.*` nouveau, `src/ui/ecrans/reglages*`, `src/game/sauvegarde.*`, `src/platform/android_main.cpp` + Java, `assets/textes/**`, `tests/**`) | — | `@dev` |
| WFS-505 | Menus en anglais : accueil, choix du terrain, modes, chargement, pause, salon du duel, crédits | E8 (`src/ui/ecrans/menu,terrain,chargement,pause,salon*`, `src/game/modes_ecran.cpp`, `src/ui/credits*`) | WFS-504 | `@dev` |
| WFS-506 | Course en anglais : HUD, messages (toasts), consignes de la carte de navigation, fin de course de tous les modes | E3 + E5 (`src/ui/hud*`, `toasts*`, `minicarte*`, `src/game/course*_system.cpp`, `poursuite_system.cpp`, `duel_system.cpp`, `traffic_system.cpp`, `camera_vue.cpp`) | WFS-504 | `@dev` |
| WFS-507 | Garage et écran des paquets de cartes en anglais | E5 + E14 (`src/ui/garage*`, `planche_voitures*`, `src/ui/ecrans/paquets*`) | WFS-504 | `@dev` |
| WFS-508 | CGU et politique de confidentialité en anglais (dans le jeu et dans `store/`), avis du `juriste` | E8 (`src/ui/ecrans/cgu*`) + `store/cgu-en.md`, `store/confidentialite-en.md` | WFS-504 | `@dev` + avis `juriste` |
| WFS-509 | Fiche Google Play en anglais : titre, descriptions, captures anglaises | Publication app (`store/fiche-en.md`, Play Console) | WFS-505, 506, 507, 508 ; jour de publication | agent expert ci |

WFS-505 à 508 touchent des fichiers différents : ils peuvent avancer **en même temps** (4 sessions) dès que WFS-504 est fusionné.

## Règles communes à toute l'épic (à rappeler dans chaque PR)
- RC-E20-01 **Un seul chemin pour le texte** : tout texte visible passe par le système de langue de WFS-504 (`ui::tr("cle")`, tables
  `assets/textes/fr.txt` et `assets/textes/en.txt`). **Plus aucune phrase française écrite en dur** dans un fichier traduit. Le lead dev refuse
  une PR qui en ajoute une (épic E20, E17, E18, E19 et tout nouvel écran).
- RC-E20-02 **Les deux langues ensemble** : chaque clé existe dans les deux tables (test CPU de WFS-504). Un nouvel écran (étoiles, médailles,
  cadeau, market, profil…) livre ses textes **français et anglais dans la même PR**.
- RC-E20-03 **Anglais simple et naturel**, comme un jeu de course mobile (US English : « Settings », « Race », « Garage », « Continue »), tutoiement
  français → « you » anglais ; pas de traduction mot à mot ; les noms propres inventés (noms des voitures, des rues, des cartes « Corniche »…)
  restent tels quels ; « WaForSpeed » ne se traduit pas.
- RC-E20-04 **Boutons : taille inchangée** (ordre permanent du 2026-11-06). Si le texte anglais est plus long : traduction plus courte d'abord,
  sinon police réduite **de 20 % au plus** (fonction de WFS-504) ; jamais un bouton agrandi, jamais un texte coupé ou qui dépasse.
- RC-E20-05 **Captures** : chaque ticket montre ses écrans **en français ET en anglais** (banc PC, `WFS_LANGUE=fr` / `WFS_LANGUE=en`), avant / après.
- RC-E20-06 **Garde-fous** : le banc « textes » de WFS-504 (journal de tous les textes affichés, en anglais) ne trouve **aucune phrase française**
  et **aucun texte qui dépasse** sur les écrans du ticket.
- RC-E20-07 **Interface 2D** : le style reste celui de `design-street-racing` (polices, couleurs) ; la police Rajdhani couvre déjà l'anglais.

## Hors périmètre de l'épic (non demandé)
Autres langues ; unités (km/h restent km/h) et formats des nombres ; voix ; noms des voitures et des cartes ; iOS (gel).

## Règle pour les écrans des épics E17, E18, E19 (et le futur market)
Leurs écrans (WFS-481, 482, 489, 491, 494, 496, 497, 499, 501 et suivants) passent par le système de WFS-504 : ils **dépendent de WFS-504**
et livrent leurs textes en français et en anglais (RC-E20-01, 02). Les tickets de données sans texte (WFS-480, 485, 487, 498, 500) ne changent pas.
