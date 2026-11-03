# WFS-19 — Piétons animés (ville et plage)
Épic : E2 · Priorité : P2 Haute · Dépend de : WFS-06 (✅), WFS-63 (✅), WFS-72 (✅)

> Découpage du PO (2026-11-03) : les conducteurs (ancienne RG-05) passent dans **WFS-73** ; les poses assis, allongés et baigneurs passent dans **WFS-71** (avec terrasses et serviettes).

## Besoin (qui, quoi, pourquoi)
La ville et la plage sont peuplées de vrais personnages animés (marche, debout, discussion, joggeurs). Les silhouettes géométriques sont proscrites.

## Règles de gestion
- RG-01 : personnages réels `homme` et `femme` avec animations d'origine (`Walk_Loop`, `Walk_Formal_Loop`, `Idle_Loop`, `Idle_Talking_Loop`, `Sitting_Idle_Loop`, `Sitting_Talking_Loop`) ; vitesse de marche 1,1 m/s, course légère 2,9 m/s (joggeurs 2,6).
- RG-02 : au plus 220 personnes placées par la graine de la carte (`populatePeople`, `mulberry32(seed×13+5)`) : ville centre 55 % par tronçon de 3 de chaque côté, résidentiel 40 %, parc 35 % + joggeurs 30 %, port 6 % ; plage : promeneurs de la promenade. **Seules les cartes ville et plage** (les autres n'ont pas encore de décor). Le côté du parc est tiré à la graine tant que `parkSide` n'existe pas en natif. Les places assis / allongé / baigneur sont ignorées (WFS-71).
- RG-03 : seuls les personnes entre −10 et +115 tronçons autour du joueur sont habillées ; les plus proches prioritaires (devant avant derrière) ; 8 maillages complets, LOD à 16 m et 50 m, 10 ombres.
- RG-04 : apparence tirée au hasard mais stable (graine de la place) : 6 teintes de peau, 8 coiffures, hauts, bas, chaussures, maillots de bain ; 18 % tenue formelle ; manches et jambes (short sur la plage) ; échelle 0,95 à 1,05.
- RG-05 : (déplacée dans WFS-73 — conducteurs.)
- RG-06 : animation sur threads de travail si possible, sans allocation par image.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/world/people.js` (`PED_*`, `pedLook`, `updatePeopleReal`, `pedClone`) ; `js/world/life.js` (`populatePeople`, `addPerson`, `updatePeople`) .
- Modèles : `models/people/homme.json`, `femme.json`, `animations.json` + `license.txt` — **ne pas créer d'autres personnages**.
- Natif (WFS-63, sur `main`) : utiliser le point d'extension existant `drawSkinned`, `src/render/squelette.h` et les modèles déjà convertis `assets/people/*.wfp` (doc dans le skill `waforspeed-natif`) ; ne pas toucher au renderer (zone E0/E4).
- Retours connus : jamais de silhouettes géométriques ; conducteur face au volant (pas de dos).

## Critères d'acceptation (captures du banc PC)
- [ ] capture de ville : piétons sur les trottoirs dans des poses variées, bien proportionnés
- [ ] capture plage : promeneurs sur la promenade
- [ ] deux captures à des instants différents : poses de marche différentes (animation en marche)

## Hors périmètre
Conducteurs (WFS-73) ; assis aux terrasses, allongés sur serviettes, baigneurs (WFS-71) ; campagne et paddock (après WFS-53) ; piétons qui traversent ou sont percutés, voix, foule dense.

