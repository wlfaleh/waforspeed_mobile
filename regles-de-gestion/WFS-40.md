# WFS-40 — Garage : fiche de la voiture et peintures
Épic : E5 (Interface et contenu) · Priorité : 0.7 · Dépend de : WFS-11

## Besoin
Dans le garage, le joueur voit la fiche de la voiture et choisit sa peinture, puis « Choisir » ou « Retour ».

## Règles de gestion
- RG-01 : 16 peintures (`PAINTS` de `catalog.js`), rangée de pastilles + nom de la peinture ; carrosserie seule repeinte
  (vitres, chrome, pneus intacts) ; peinture mémorisée par voiture pendant la session.
- RG-02 : fiche : marque · index (« 01 / 04 »), modèle, type (`body`), `tag`, puissance (ch), 0 à 100 (s), vitesse max (km/h)
  (`spec`), 4 barres `carStats` sur 10, crédit du modèle (`CAR_MODELS`, licence obligatoire).
- RG-03 : « Choisir » → voiture et peinture retenues pour la course (et nom affiché au menu, WFS-37) ; « Retour » → menu sans changement.
  Ordre du web : menu « Course solo » → garage → terrain (WFS-41) → course.

## À réutiliser
`js/cars/catalog.js` → `PAINTS`, `paintById`, `carStats`, `CAR_MODELS` ; `js/ui/screens.js` → `renderGarageUI`, `selectPaint`,
`leaveGarage` ; `index.html` `#garage` ; `css/style.css` `.show-*`, `.specs`, `.statlist`, `.paint-row`.

## Critères d'acceptation
- [ ] `garage-fiche` : fiche complète de la Golf, crédit lisible ; `garage-peinture-0/1` : deux peintures différentes appliquées
- [ ] après « Choisir », capture de course avec la voiture et la peinture choisies

## Hors périmètre
Les 16 autres voitures (WFS-13), sauvegarde (WFS-24).
