# WFS-19 — Piétons animés et conducteurs
Épic : E2 · Priorité : 17 · Dépend de : WFS-06 (✅)

## Besoin (qui, quoi, pourquoi)
La ville et les plages sont peuplées de vrais personnages animés (marche, assis, allongés, joggeurs) et chaque voiture a un
conducteur face au volant. Les silhouettes géométriques sont proscrites.

## Règles de gestion
- RG-01 : personnages réels `homme` et `femme` avec animations d'origine (`Walk_Loop`, `Walk_Formal_Loop`, `Idle_Loop`, `Idle_Talking_Loop`, `Sitting_Idle_Loop`, `Sitting_Talking_Loop`) ; vitesse de marche 1,1 m/s, course légère 2,9 m/s (joggeurs 2,6).
- RG-02 : au plus 220 personnes placées par la graine de la carte (`populatePeople`, `mulberry32(seed×13+5)`) : ville centre 55 % par tronçon de 3 de chaque côté, résidentiel 40 %, parc 35 % + joggeurs 30 %, port 6 % ; plage 50 % côté mer, 10 % rochers ; campagne seulement au village (25 %) ; circuit : paddock 35 %, assis ou debout.
- RG-03 : seuls les personnes entre −10 et +115 tronçons autour du joueur sont habillées ; les plus proches prioritaires (devant avant derrière) ; 8 maillages complets, LOD à 16 m et 50 m, 10 ombres.
- RG-04 : apparence tirée au hasard mais stable (graine de la place) : 6 teintes de peau, 8 coiffures, hauts, bas, chaussures, maillots de bain ; 18 % tenue formelle ; manches et jambes (short sur la plage) ; échelle 0,95 à 1,05.
- RG-05 : conducteur : buste (0,85 m) posé à la place mesurée sur le toit de chaque voiture (`measureSeat`), tourné vers le volant (`rotation.y = π ± 0,06`), 3 coiffures par sexe, réduit jusqu'à 0,86 sur les voitures très basses.
- RG-06 : animation sur threads de travail si possible, sans allocation par image.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/world/people.js` (`PED_*`, `pedLook`, `updatePeopleReal`, `pedClone`) ; `js/world/life.js` (`populatePeople`, `addPerson`, `updatePeople`) ; `js/world/drivers.js` (`bakeBust`, `measureSeat`, `addDriver`).
- Modèles : `models/people/homme.json`, `femme.json`, `animations.json` + `license.txt` — **ne pas créer d'autres personnages**.
- Retours connus : jamais de silhouettes géométriques ; conducteur face au volant (pas de dos).

## Critères d'acceptation (captures du banc PC)
- [ ] capture de ville : piétons sur les trottoirs dans des poses variées, bien proportionnés
- [ ] capture plage : baigneurs et promeneurs ; capture intérieur voiture : conducteur visible face au volant
- [ ] deux captures à des instants différents : poses de marche différentes (animation en marche)

## Hors périmètre
Piétons qui traversent ou sont percutés, voix, foule dense hors liste ci-dessus.
