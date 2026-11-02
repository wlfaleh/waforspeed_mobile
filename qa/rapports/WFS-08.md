# QA — WFS-08 : Trafic multi-thread et collisions
Date : 2026-11-02 · Branche : `main` (PR #6 fusionnée) · Release : `apercu` (construction 83, commit `efa1f97`,
qui contient la fusion `6c70b75`) · Banc d'essai PC (règle WFS-31)

> **QA après coup** : la PR #6 a été fusionnée dans `main` à la demande explicite de l'utilisateur, **avant** ce verdict.

## Verdict : VALIDÉ (avec remarques non bloquantes)

## Captures regardées (PNG 1280x720 téléchargés depuis la Release `apercu`)
- `trafic-cam0` (88 km/h) : Golf du joueur entière, une voiture blanche du trafic devant sur la file de droite, à la
  bonne taille et sur la route. Image non déformée. ✅ (voir remarque 1)
- `trafic-cam1` (117 km/h) : trois voitures du trafic sur la route (berline grise, voiture blanche au loin, Gallardo
  bleu/blanc à droite), tailles cohérentes avec la distance, chacune dans sa file, aucune qui en traverse une autre. ✅
- `trafic-choc` : accident bien visible — Golf du joueur en travers de la route (tête-à-queue, 1,54 rad d'après le
  journal), voiture percutée (berline sombre) déportée vers le bord gauche ; le reste du trafic continue (Gallardo à
  droite, voiture au loin). ✅ (voir remarque 2)
- `course-cam0-0/1/2` : course en pilote automatique, voiture, route et ville correctes ; pas de voiture du trafic
  dans le champ sur ces trois images. ✅
- `course-cam1-0/1/2` : idem ; voiture rouge du trafic visible au loin sur `course-cam1-1`, sur la route. ✅
- `vitrine-golf-0`, `vitrine-p911-1` : voitures entières, décor intact, rien de cassé (pas de régression). ✅

Aucun écran noir, aucune couleur aberrante, aucun morceau qui flotte, aucune voiture du trafic dans le décor.

## Journal du banc d'essai (job `banc-essai` de la construction de `main`)
- `mode exact : 39 voitures comparées au jeu web, 0 écart(s)` → port de traffic.js avec la même graine. ✅
- `mode parallèle (4 threads) : déterministe, aucun chevauchement` → mise à jour sur les threads de travail. ✅
- `choc arrière à 156 km/h : 1 accident(s), voiture percutée en glissade` (test) et
  `capture trafic-choc : 1 accident(s) après 0.43 s, voiture 1 en glissade, joueur 108 km/h, tête-à-queue 1.54 rad`. ✅
- `trafic : 42 voitures, pendant la course 0 quasi-contacts, 0 accidents`.

## Remarques (non bloquantes, à suivre)
1. `trafic-cam0` : le joueur roule derrière **une seule** voiture visible, pas un « groupe ». La scène prouve le
   trafic mais pas un peloton ; placer le joueur derrière 2-3 voitures rendrait la capture plus probante.
2. `trafic-choc` : l'arrière de la voiture percutée dépasse sur le trottoir de gauche. Plausible pour une glissade,
   mais à comparer au jeu web (bornage latéral des voitures en glissade dans `crashTraffic`).
3. Quasi-contacts : comptés mais invisibles (pas de HUD avant WFS-12/WFS-27) et 0 pendant la course du banc ; vérifiés
   seulement par le test de fidélité. À revoir visuellement avec WFS-12.
4. Captures `course-*` : très peu de trafic dans le champ (1 voiture sur 6 images) ; normal avec 42 voitures sur la
   boucle, sans incidence sur le verdict.
5. Hors ticket (déjà présent avant WFS-08) : petits points blancs en trame sur la carrosserie (Golf près des feux
   arrière sur `course-cam0-1` et `vitrine-golf-0`, avant de la 911 sur `vitrine-p911-1`, Gallardo sur `trafic-choc`).
   Artefact de reflet/brillance du rendu, à traiter dans un ticket de rendu (WFS-17 TAA, par ex.).
