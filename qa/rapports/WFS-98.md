# QA WFS-98 : commandes tactiles du jeu web, HUD de la vue cockpit, caméra du demi-tour

## QA après fusion, recette de nuit (2026-11-04 22:36:48 UTC, qa-lead, accent qualité et design)

**Verdict : PASSABLE AVEC ANOMALIES MINEURES**

- Captures regardées : Release `apercu` (main, commit de fusion `f542093`, run 41920610966 vert, publiée à 22:16 UTC).
- Référence : `regles-de-gestion/WFS-98.md`, skill `design-street-racing`, jeu web `waaps/waforspeed/css/style.css` l. 130-251.
- Fusion par confiance : la recette est faite après la fusion ; la mineure nouvelle va au tableau `anno.md` (ANO-45).
- Les trois anomalies bloquantes du ticket sont corrigées : **ANO-31, ANO-24 et ANO-30 passent en ✅ Vérifiée**.

### Critères d'acceptation

| Critère | Capture | Constat |
|---|---|---|
| Pédales « GAZ » (orange) et « FREIN » (rouge) lisibles | `hud-course`, `hud-2400`, `encoche-hud-2400` | OK. Gaz 64 px de large (x 1200 → 1263 en 720p), coins coupés, double chevron #FF6A00. Frein ≈ 84 × 88 px avec disque et étriers roses. Libellés espacés en bas, bordure du gaz orangée |
| Nitro rond, anneau vert acide, « NITRO » | idem, `hud-nitro`, `nitro-lignes` | OK. Anneau #B6FF00 proportionnel à la réserve, flamme verte, halo vert quand la nitro est active. Libellé « NITRO » à peine lisible en 720p : il ne fait que 7 px de haut (ANO-17 / ANO-19, déjà planifiées) |
| Volant dans un pavé vitré | idem | OK. Pavé biaisé, piste, chevrons ‹ ›, volant à 3 branches avec repère cyan, halo cyan à l'appui (`camera-cockpit`, `conduite-*`) |
| Rien sous l'encoche | `encoche-hud-2400` | OK. Pavé du volant à droite de la bande rouge ; pédales à ≈ 23 px du bas et du bord droit en 2400x1080 |
| Gaucher en miroir | `course-gaucher` | OK. Gaz, frein et nitro à gauche, volant à droite, gaz appuyé (rempli orange) |
| Gaz appuyé = remplissage orange | `hud-nitro`, `encoche-hud-2400` | OK. Dégradé orange de bas en haut, icône et libellé en #0A0B10 |
| Boutons du HUD sans rayures | `hud-course` | POURSUITE / COCKPIT : OK, d'un seul tenant. **MENU encore rayé** : `pause.cpp`, hors zone, déjà noté ANO-20 → WFS-102 |
| Un seul compteur en cockpit, plus de disque cyan isolé | `camera-cockpit`, `camera-cockpit-2400` | OK. Seul le cadran du tableau de bord ; « 153 KM/H » en chiffres et cases de nitro sous le panneau du temps, à droite de MENU |
| Demi-tour : Golf entière au tiers bas, en travers de la route | `conduite-demitour` | OK. Golf entière entre 0,62 et 0,83 H, perpendiculaire aux lignes de la route, trottoir et passants derrière |
| Pas de régression | `camera-poursuite`, `course-cam*`, `course-decompte-3`, `course-fin`, `trafic-cam0` | Image nette, voiture visible, rien de cassé |

### Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-45 | Mineure | `chargement-fondu-2400`, `course-fin` | Les commandes tactiles (volant, nitro, frein, gaz) restent dessinées pendant le fondu du chargement (sous le logo) et sous le panneau de fin de course. Au jeu web, `#touch` n'est affiché qu'en course (`screens.js` l. 17, `touchOn`). **Correction** : dans `Game::drawControls`, ne dessiner les commandes et n'accepter leurs appuis que si l'écran est `Ecran::Course`, que la course n'est pas finie et qu'aucun fondu n'est en cours ; décompte compris, comme au web |
| (ANO-20) | Bloquante, déjà planifiée | `hud-course` | Bouton MENU rayé (`pause.cpp`), hors zone de WFS-98 → WFS-102 |
| (ANO-17 / ANO-19) | Bloquante, déjà planifiée | `hud-course`, `nitro-lignes` | Libellé « NITRO » de 7 px en 720p, à peine lisible quand la nitro est active. Cibles en 2400x1080 : gaz 96 px ≈ 37 dp et nitro 102 px ≈ 39 dp, sous les 44 dp → WFS-95 / WFS-102 |

### À tester sur téléphone (utilisateur)
Gaz, frein et nitro au pouce (la zone de toucher peut déborder du dessin, RG-05 bis) ; caméra pendant un vrai demi-tour.
