# WFS-57 — Correction : centrer la paire « Rejouer » / « Menu » de l'écran de fin (ANO-14)
Épic : E3 · Priorité : 0.405 (MVP, avant WFS-50) · Dépend de : WFS-38 ✅ · Taille : ≈ 1 h · Origine : ANO-14 (`anno.md`, QA WFS-38 tour 2/2)

## Besoin (qui, quoi, pourquoi)
Le joueur voit, à la fin de chaque course, « Rejouer » centré seul et « Menu » collé à sa droite : la paire est décalée de
+108 px (1280x720), avec 288 px de marge à gauche contre 72 à droite, alors que tout le reste du panneau est centré.
C'est l'écran que le MVP montre le plus souvent (WFS-50). Le scénario du simulateur (`tools/qa/emulateur.sh`, WFS-56)
**suppose déjà la paire centrée** : son appui « Menu » (≈ x = 791 en 1280) tombe aujourd'hui à côté du bouton (799 à 1000).

## Règles de gestion
- RG-01 : dans `drawOver` de `src/game/course_system.cpp`, la **paire** est centrée sur `cx` : largeur totale
  `bw + 2·u + 0,7·bw` (= 1,75·bw, même écart qu'aujourd'hui : `0,05·bw` = `2·u`) ; départ `bx = cx − (1,7·bw + 2·u)/2`.
  « Rejouer » de `bx` à `bx + bw`, « Menu » de `bx + bw + 2·u` à `bx + 1,75·bw`. En 1280x720 (u = 7,2, bw = 288) :
  Rejouer 388 → 676, Menu ≈ 690 → 892, milieu de la paire = 640.
- RG-02 : **aucune autre valeur ne change** : largeurs (bw, 0,7·bw), hauteur 10·u, y = py + 60·u, biseau 14 px (réf. 720 p),
  couleurs (orange / #1E2130), textes centrés dans leur bouton, délai d'activation 0,9 s.
- RG-03 : les **zones de toucher** de `onTouch` suivent exactement les rectangles dessinés (une seule source : `btn_` pour
  « Rejouer » et un second rectangle pour « Menu », plus de facteurs 1,05 / 1,75 recopiés à la main).
- RG-04 : accroche autorisée hors zone, **≤ 2 lignes**, annoncée au Journal : `benchMenuTactile` (`src/ui/ecrans/menu.cpp`,
  ligne `bx = …` du toucher « Menu ») prend le centre du nouveau « Menu » (`W/2 − (1,7·bw + 2·u)/2 + bw + 2·u + 0,35·bw`),
  sinon `fin-retour-menu` n'arriverait plus au menu. Ne pas toucher `tools/qa/emulateur.sh` : sa formule est déjà la bonne.
- RG-05 : captures renouvelées : `fin-menu`, `course-fin` (et toute autre capture d'écran de fin) ; aucune autre capture ne
  change, banc `0 écart(s)`.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- `index.html` (section `#over`) : `<div class="actions">` avec `#againBtn` puis `#menuBtn` ; `css/style.css` ligne 292 :
  `.actions { display: flex; gap: 12px; }` — deux boutons côte à côte, même rangée. On garde l'écart natif actuel (2·u ≈ 14 px
  en 720 p, déjà validé par WFS-38 et utilisé par le scénario simulateur) : rien d'autre à porter.

## Critères d'acceptation (vérifiables sur les captures du banc PC)
- [ ] `fin-menu` et `course-fin` : milieu de la paire à x = 640 ± 2 en 1280x720 (marges gauche et droite égales dans le panneau)
- [ ] `fin-retour-menu` : toujours le menu, record à jour (l'appui « Menu » du banc touche le nouveau bouton)
- [ ] autres captures identiques à `main`, banc `0 écart(s)` ; ANO-14 n'est plus visible

## Hors périmètre
- Restyler les boutons, changer l'écart pour 12 px du web, ajouter d'autres boutons ; tout changement de `tools/qa/**`.

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| 0.405 | WFS-57 | [MVP] Correction : centrer la paire « Rejouer » / « Menu » de l'écran de fin (ANO-14) | E3 | ✅ Terminé | Claude (session trafic), 2026-11-03 | WFS-38 ✅ | `feature/waforspeed-57` | #29 (fusionnée) | ✅ VALIDÉ par qa-lead (cc76b01, banc PC, ANO-14 corrigée) — [rapport](qa/rapports/WFS-57.md) |

Critères et notes de suivi :

**WFS-57 — [MVP] Correction : centrer « Rejouer » / « Menu » (ANO-14)** · E3 · priorité 0.405 · ✅ Terminé (PR #29, VALIDÉ) — Claude (session trafic), 2026-11-03 (règles : `regles-de-gestion/WFS-57.md`)
- [ ] `fin-menu`, `course-fin` : milieu de la paire à x = 640 ± 2 (1280x720) ; `fin-retour-menu` toujours au menu ; autres captures identiques, banc `0 écart(s)`
