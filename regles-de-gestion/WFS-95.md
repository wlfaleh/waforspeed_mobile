# WFS-95 — Texte et bandeaux : caractères manquants, petits libellés lisibles, nom du quartier en course
Épic : E5 · Priorité : P2 Haute · Dépend de : — (WFS-26, WFS-47 ✅) · Taille : 1 jour

## Besoin
Sur la capture `manette-menu-focus` de `main`, le record vide du menu s'affiche « ? » au lieu de « — » : le tiret
cadratin (U+2014) n'est pas dans l'atlas de la police (`src/ui/text.cpp`, jeu de caractères ASCII + Latin-1 + 6
caractères) ; tout caractère absent devient « ? ». Les petits libellés espacés (11 px, « TA VOITURE », « RECORD ») perdent
leurs traits fins : le « I » de « VOITURE » disparaît presque. Enfin, le jeu web affiche le nom du quartier quand on en
change (`js/game/update.js` l. 240 : bandeau « Centre-ville », « Zone portuaire »…), le natif ne le fait pas.

## Règles de gestion
- RG-01 : l'atlas contient en plus : — (U+2014), – (U+2013), ‘ “ ” (U+2018, U+201C, U+201D), • (U+2022), → ← (U+2192,
  U+2190), ✓ (U+2713) s'ils existent dans la police ; « — » s'affiche sur le menu (record vide) et partout où le code l'écrit.
- RG-02 : un caractère absent de la police est journalisé une fois (« Police : caractère U+XXXX absent ») au lieu d'être
  remplacé en silence ; le repli reste « ? ».
- RG-03 : petits textes (≤ 14 px) lisibles : aucun trait d'une lettre ne disparaît à 1280x720 ni à 2400x1080 (atlas
  rendu à une taille adaptée ou mipmaps / filtrage de la texture de police, au choix du développeur, dans `src/ui/**` ou
  `shaders/ui*`).
- RG-04 : nom du quartier : à l'entrée dans une nouvelle zone (`zoneIndex` / `zoneName` de `src/world/zones.h`, lecture
  seule), un bandeau `toast(nom, '', 'cp')` comme le web : pas au départ (premier quartier), une fois par changement de
  zone, en course seulement, pile et durée des bandeaux de WFS-47 (3 bandeaux, 1,9 s).

## À réutiliser du jeu web
`js/game/update.js` l. 240 (`zoneSeen`, clé `zoneIdx:index/ZONE_LEN`) ; `js/ui/toasts.js` (déjà porté, `src/ui/toasts.cpp`) ;
`ZONES` de `js/world/decor.js` (déjà porté, `src/world/zones.cpp`). Police : celle déjà dans `assets/fonts/`.

## Critères d'acceptation
- [ ] `menu-0` / `manette-menu-focus` : « — » sous RECORD ; « TA VOITURE » lisible lettre par lettre
- [ ] nouvelle capture `toast-quartier` (banc des bandeaux) : bandeau « Quartier résidentiel » (ou autre nom) ;
  journal « bandeau : quartier … » une fois par changement de zone
- [ ] `texte-essai-1280` / `-2400` : petits corps nets ; autres captures sans régression ; tests CPU verts

## Hors périmètre
Nouvelle police, traduction, changement de mise en page des écrans (E8).

## Zone de code
`src/ui/text.*`, `src/ui/toasts.*`, `shaders/ui*`, `assets/fonts/**`. Interdit : `src/ui/garage.*` (accroche de WFS-96),
`src/ui/ecrans/**` (E8, WFS-96), `src/render/**` (WFS-97), `src/platform/**` (WFS-91), fichiers du duel (WFS-89).
