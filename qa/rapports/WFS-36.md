# QA WFS-36 : Point d'extension « écrans de l'application »

**Verdict : VALIDÉ** (banc d'essai PC, règle WFS-31), 1er passage

- Construction regardée : commit `e3d7ea4` (branche `feature/waforspeed-36`, PR #13). Release `apercu-waforspeed-36`
  (« construction 158 », texte « Branche feature/waforspeed-36, commit e3d7ea4 », publiée le 2026-11-03 08:18 UTC).
  Run Actions 41600152434 (push) : jobs `banc-essai`, `android`, `publication` verts.
- Référence : Release `apercu` de `main` (« construction 160 », commit `484650e`). La base de la branche est `5490383`
  (`git merge-base`) ; entre `5490383` et `484650e` seul `TACHES.md` change : la référence est donc équivalente à la base exacte.

## Critère 1 : captures identiques (écran `Course` par défaut)

Comparaison pixel par pixel (PIL, `ImageChops.difference(...).getbbox()`) des 27 PNG communs aux deux Releases :
**27 / 27 identiques**, aucun pixel différent, tailles identiques (`course-cam0/1-*`, `trafic-*`, `ombres-*`,
`vitrine-*` des 4 voitures, `texte-essai-1280/2400`, `moteur-courbe`).

Journal du job `banc-essai` :
- `test_track`, `test_systems` (« systèmes : 2, ordre : 12 »), `test_traffic` (« mode exact : 39 voitures comparées au jeu web,
  0 écart(s) », mode parallèle déterministe, choc arrière OK) et `test_audio` (« → OK ») passent.
- Pas encore de test de course (WFS-12 en revue) : constat, pas un défaut de ce ticket.

## Critère 2 : nouvelle capture `ecran-menu-vide`

- Image 1280×720, non déformée : route, trottoir, ville, arbres, ciel et trafic (Porsche grise, voiture au loin) dessinés,
  la Golf bleue bien visible.
- **Aucune commande tactile** : ni cercle de volant (gris), ni frein (rouge), ni accélérateur (vert), ni nitro (bleu),
  ni barres de vitesse / nitro en haut à gauche (présents sur `course-*` et `trafic-*`).
- Voiture immobile : journal « Écran : course → menu » puis « capture ecran-menu-vide : écran menu, 0.0 km/h ».
- Remarque (sans conséquence) : la scène suit `trafic-choc`, la Golf est donc garée contre la bordure après le tête-à-queue
  et son cap s'est redressé pendant les 1,5 s à 0 km/h. Une capture sur une scène « propre » sera plus parlante quand le
  menu (WFS-37) la remplacera.

## Critère 3 : documentation

`.claude/skills/waforspeed-natif/SKILL.md` (section « Écrans de l'application ») et `references/architecture.md`
(section « Écrans (WFS-36) ») : lire `GameView::ecran`, changer par `allerA`, réagir par `onEcran`, règles du noyau,
écran de départ, ordre des touchers. Complet et conforme au code.

## Relecture du diff au regard des règles de gestion

- RG-01 : `src/core/ecran.h`, `enum class Ecran` avec les 7 valeurs, `GameView::ecran`. OK.
- RG-02 : `allerA()` ne fait que mémoriser la demande ; `appliquerEcran()` l'applique au début de `update` et de `render`
  (jamais au milieu d'une image), lâche les commandes et appelle `onEcran` pour **tous** les systèmes. Méthode virtuelle
  facultative (corps vide). OK.
- RG-03 : hors `Course`, `DriveInput{}` (aucune commande), pilote automatique coupé, `onTouch` ne pilote plus après les
  systèmes, `drawControls()` non appelé, vitesse remise à 0 à chaque pas ; la scène et les systèmes continuent.
  `Pause` : retour immédiat de `update` (ni conduite ni `systemsUpdate`), le rendu continue. OK.
- RG-04 : écran de départ `Course`, réinitialisé à chaque `Game::init` ; le banc force `Menu` par `g.allerA`. OK.
- RG-05 : hors périmètre (WFS-38).
- Zone : noyau (E0) + accroche de 5 lignes dans `desktop_main.cpp` pour le banc. OK.

## Problèmes

Aucun problème bloquant.
