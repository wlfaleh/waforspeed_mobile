# WFS-37 — Menu principal : affichage (logo, choix, record, ta voiture)
Épic : E8 (Écrans et menus) · Priorité : 0.3 · Dépend de : WFS-35, WFS-36

## Besoin
Après le chargement, le joueur arrive sur le menu du jeu web, par-dessus la ville. Ce ticket **dessine** le menu ;
les appuis viennent avec WFS-38 (livraison visible plus tôt, ticket plus petit).

## Règles de gestion
- RG-01 : écran `Menu` (WFS-36) ; fond = scène 3D + voile dégradé gauche → droite du web (`.screen` :
  `linear-gradient(100deg, rgba(10,11,16,.94) 0%, .78 42%, .12 100%)`), approximé par 8 à 16 bandes verticales `uiRect`.
- RG-02 : même bloc, logo et sur-titre que WFS-35 (**réutiliser les fonctions de `chargement.cpp`**, ne pas les recopier).
- RG-03 : 3 choix du web (`index.html`, `.menu-list`) : « Course solo » / « Bats le chrono, frôle le trafic, enchaîne les
  checkpoints » ; « Duel en ligne » / « Défie un ami sur la même route, même trafic » ; « Showroom » / « Choisis ta voiture et
  sa couleur ». Titre Rajdhani capitales, description gris `--muted` ; zone tactile ≥ 48 px logiques de haut.
- RG-04 : pied (`.menu-foot`) : « RECORD » + meilleur score (`—` s'il n'y en a pas, web `toMenu()`) et « TA VOITURE » + nom
  de la voiture (`myCar.name`, ex. « Golf 8 GTI »).
- RG-05 : apparition comme le web : fondu 300 ms puis chaque ligne glisse de −36 px à 0 avec 40 ms de décalage (`slide-in`).
- RG-06 : texte d'aide tactile du web (`.intro.only-touch`) : « Flèches à gauche : volant. À droite : Gaz, Frein (à l'arrêt :
  marche arrière), Nitro. »
- RG-07 (précision PO, 2026-11-03) : le logo et le sur-titre sont aujourd'hui internes à `chargement.cpp`. Les **sortir** en une
  fonction déclarée dans `src/ui/ecrans/chargement.h` (ex. `dessinerBlocLogo(r, W, H, opacite)`), appelée par le chargement
  et par le menu : captures `chargement-*` identiques au pixel près.
- RG-08 : système `Menu` dans `src/ui/ecrans/menu.{h,cpp}`, dessiné seulement quand `GameView::ecran == Ecran::Menu`
  (pas de scène dédiée : la ville et le trafic restent derrière le voile). L'écran par défaut reste `Course` (le passage
  chargement → menu au démarrage vient avec WFS-38) : toutes les autres captures restent identiques.
- RG-09 : le HUD de course (WFS-27) et les commandes tactiles ne s'affichent pas sur le menu (déjà garanti hors `Course`
  par WFS-36 ; à vérifier sur la capture). Le record vient de `activeCourse()` / session seulement, sinon « — ».
- RG-10 : captures du banc : `menu-0` (1280x720), `menu-anim` (100 ms après l'entrée), et `menu-1` en 2400x1080 dans un
  lancement **sans** `WFS_TEXTE_ESSAI` (mode `WFS_BANC=menu`, comme `WFS_BANC=hud` de WFS-27 ; voir ANO-10).

## Zone de code et accroches
Zone E8 : `src/ui/ecrans/**`. Accroches annoncées au Journal, ≤ 10 lignes chacune : enregistrement du système (une ligne,
`src/game/game.cpp` ou mécanisme d'enregistrement de WFS-30), captures `menu-*` dans `src/platform/desktop_main.cpp`,
1 ligne dans `.github/workflows/build.yml` pour `menu-1`. Lecture seule : `src/ui/text.h`, `src/core/ecran.h`, `src/core/system.h`.

## À réutiliser
`index.html` (`#menu`), `css/style.css` (`.screen`, `.mitem`, `.mi-t`, `.mi-d`, `.menu-foot`, `.record`, `slide-in`),
`js/ui/screens.js` → `toMenu()`. Natif : `src/ui/ecrans/chargement.*` (WFS-35), `src/ui/text.h`.

## Critères d'acceptation
- [ ] `menu-0` (1280x720) et `menu-1` (2400x1080) : ville en fond assombri à gauche, logo, 3 choix lisibles, record « — », « Golf 8 GTI »
- [ ] `menu-anim` : capture à 100 ms montrant les lignes en cours de glissement
- [ ] Aucune commande de conduite visible ; autres captures inchangées

## Hors périmètre
Appuis et navigation (WFS-38), réglages, sauvegarde du record (WFS-24 : record de la session seulement).
