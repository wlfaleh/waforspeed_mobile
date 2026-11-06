# WFS-72 — Point d'extension : apparence des personnages (peau, cheveux, vêtements)
Épic : E0 · Priorité : P2 Haute · Dépend de : WFS-63 ✅ · Débloque : WFS-19 (RG-04, apparence des piétons et conducteurs)

## Besoin (qui, quoi, pourquoi)
WFS-19 doit habiller les personnages comme le jeu web (RG-04). Le jeu web le fait dans un shader dédié qui découpe, pixel
par pixel, peau / cheveux / haut / bas / chaussures selon un attribut par sommet et des valeurs par personne. Le point
d'extension `drawSkinned` (WFS-63) ne transporte ni cet attribut ni ces valeurs : `scene.frag` ne donne qu'une teinte
uniforme. Sans ce ticket, E2 devrait toucher au renderer.

## Règles de gestion
- RG-01 : l'attribut par sommet `_cloth` (vec4 : bras, hauteur, jambe, partie corps / cheveux) de `homme.json` / `femme.json`
  est converti par `tools/convert_people.mjs` et ajouté à `SkinVertex` et au format `.wfp` (version du format incrémentée ;
  `assets/people/*.wfp` régénérés). Aucun nouveau modèle.
- RG-02 : `drawSkinned` reçoit, par instance, une apparence : 6 couleurs (`skin`, `hair`, `top`, `bottom`, `shoe`, `swim`)
  et 2 vec4 (`cloth` : manches, longueur du bas, habillé… ; `body` : cou, taille), mêmes sens et mêmes unités que les
  uniformes `uSkin`, `uHair`, `uTop`, `uBottom`, `uShoe`, `uSwim`, `uCloth`, `uBody` du jeu web. Transport libre (palette
  d'os ou petit bloc à côté), sans allocation par image.
- RG-03 : variante `shaders/scene_skin.frag` qui porte **telle quelle** la logique de `pedMaterial` (`js/world/people.js`
  l. 54-110, partie `onBeforeCompile`) : mêmes seuils, mêmes tests ; éclairage, ombres et brouillard identiques à `scene.frag`.
- RG-04 : sans apparence fournie (valeur par défaut), le rendu reste celui de WFS-63 : les captures `squelette-*`
  existantes ne changent pas, hors capture ajoutée par ce ticket.
- RG-05 : la structure d'apparence est publique dans `src/render/squelette.h` (ou `renderer.h`) et documentée en 5 lignes
  dans le skill `waforspeed-natif` : WFS-19 la remplit avec `pedLook` sans toucher au renderer.
- RG-06 : pas de finitions : le tirage aléatoire des tenues (`pedLook`) reste dans WFS-19.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/world/people.js` → `pedMaterial` (l. 54-110 : uniformes et code GLSL injecté, à recopier dans `scene_skin.frag`),
  `pedLook` (lecture seule ici, pour connaître les valeurs attendues des uniformes).
- Modèles : `models/people/homme.json`, `femme.json` (attribut `_cloth` déjà présent).
- Natif : `tools/convert_people.mjs`, `src/render/squelette.*`, `shaders/scene_skin.vert`, `src/render/bench_squelette.cpp` (WFS-63).

## Critères d'acceptation (captures du banc PC)
- [ ] nouvelle capture `squelette-tenue` : un homme et une femme avec deux tenues différentes fixées en dur dans le banc
  (peau, cheveux, haut à manches courtes, pantalon / short, chaussures visibles et de couleurs distinctes)
- [ ] captures existantes (dont `squelette-essai`, `squelette-marche-0/1`) identiques à `main`, banc `0 écart(s)`
- [ ] doc du point d'extension dans le skill `waforspeed-natif`

## Hors périmètre
Tirage des tenues, placement, conducteurs (WFS-19) ; maillots de bain et tenue formelle peuvent n'apparaître que dans WFS-19
pourvu que le shader les gère (couleur `swim`, seuil « habillé »).

## Zone de code autorisée (exception E0 annoncée au Journal)
`src/render/renderer.h`, `src/render/renderer.cpp` (partie skinning uniquement), `src/render/squelette.*`,
`src/render/bench_squelette.*`, `shaders/scene_skin.*`, `shaders/shadow_skin.*` si besoin, `tools/convert_people.mjs`,
`assets/people/**`, `CMakeLists.txt` (liste des shaders), skill `waforspeed-natif`. E4 n'a aucun ticket actif : pas de conflit.

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P2 Haute | WFS-72 | Point d'extension : apparence des personnages (peau, cheveux, vêtements) | E0 | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic), 2026-11-03 | WFS-63 ✅ | `feature/waforspeed-72` | #40 (fusionnée, 4314bf7) |  |

Critères et notes de suivi :

**WFS-72 — Point d'extension : apparence des personnages** · E0 · priorité P2 Haute · ✅ Fusionné (PR #40, QA de nuit à faire) — Claude (session trafic), 2026-11-03, branche `feature/waforspeed-72` (règles : `regles-de-gestion/WFS-72.md`)
- [ ] `squelette-tenue` : homme et femme habillés, peau / cheveux / haut / bas / chaussures distincts ; captures existantes identiques, banc `0 écart(s)`
