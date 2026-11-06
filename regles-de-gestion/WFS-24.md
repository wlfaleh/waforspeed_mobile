# WFS-24 — Sauvegarde et progression
Épic : E3 · Priorité : 21 · Dépend de : WFS-12

## Besoin (qui, quoi, pourquoi)
Le joueur retrouve ses meilleurs scores, sa voiture, ses peintures et ses réglages à chaque lancement.

## Règles de gestion
- RG-01 : données sauvegardées (clés du web) : meilleur score (`waforspeed-best`, entier), voiture (`car`), peinture par voiture (`paint-<id>`), carte (`map`), moment (`tod` : jour/nuit, nuit par défaut), qualité (`quality`), caméra (`cam`, 0-2), son coupé (`muted`), pseudo (`nick`, 14 caractères max), distance de duel (`km`, parmi 2, 4, 6, 10), mode de braquage (`steer`).
- RG-02 : valeurs par défaut du web : voiture `golf`, carte `ville`, nuit, qualité haute, caméra proche, pseudo « Pilote », 4 km.
- RG-03 : meilleur score écrit seulement s'il est battu (`s > best`) à la fin de la course ; l'écran de fin affiche « Nouveau record ».
- RG-04 : valeur inconnue ou fichier absent/corrompu : retour au défaut sans plantage ; identifiants validés (voiture existante, peinture existante).
- RG-05 : fichier privé de l'application (Android `filesDir`), écriture atomique (fichier temporaire puis renommage), version de format pour les évolutions.
- RG-06 : écriture hors de la boucle de rendu (thread de travail) ; sauvegarde à la fin de course et au passage en arrière-plan.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/game/state.js` (`best`) ; `js/ui/screens.js` (`store(...)`, `endRace`, `setQuality`) ; `js/cars/catalog.js` (`myCar`, `myPaint`) ; `js/world/maps.js` (`setMap`) ; `js/net/duel.js` (`nick`, `duelKm`).
- Pas de modèle. Retour connu : le stockage du web peut échouer ou être vide : le jeu doit toujours démarrer.

## Critères d'acceptation
- [ ] test CPU : écrire puis relire toutes les clés ; fichier corrompu → défauts ; identifiant inconnu → défaut
- [ ] journal du banc : deux lancements successifs, le second retrouve voiture, peinture et record du premier
- [ ] capture : écran de fin « Nouveau record » avec le meilleur score (texte WFS-26)

## Hors périmètre
Compte en ligne, synchronisation entre appareils, succès.


## Précisions du PO (2026-11-04 00:20:10, à l'attribution — exception « deux tickets E3 »)
Ce ticket débloque la mémorisation attendue par WFS-11 (voiture, peinture), WFS-41 (réglages) et WFS-70 (mode de direction) : il fournit le **magasin de réglages**, les écrans le branchent ensuite.
- RG-07 : un seul module neuf, `src/game/sauvegarde.h/.cpp` : structure `Sauvegarde` (toutes les clés de RG-01, défauts de RG-02), `charger(chemin)`, `enregistrer()` (asynchrone, RG-05/06), accès global `sauvegarde()` lisible et modifiable par les autres épics. Format texte simple `cle=valeur` par ligne, première ligne `version=1` (lisible dans le journal du banc, aucune bibliothèque).
- RG-08 : chemin : Android = `app->activity->internalDataPath` + `/sauvegarde.txt` ; PC/banc = dossier de travail (`WFS_SAUVEGARDE` pour le forcer). Accroches autorisées, ≤ 10 lignes chacune, annoncées au Journal : `src/platform/android_main.cpp` (chemin + `enregistrer()` sur `APP_CMD_PAUSE`), `src/platform/desktop_main.cpp` (chemin), `src/game/game.cpp` (`charger` au démarrage, appliquer voiture / carte / moment existants).
- RG-09 : record : `CourseSystem` (`course_system.cpp`, zone E3) compare le score final au record, l'écrit s'il est battu et affiche « NOUVEAU RECORD » sur l'écran de fin existant (texte WFS-26, accent orange du design). Le menu (`src/ui/ecrans/menu.cpp`, WFS-41 actif) : accroche ≤ 3 lignes seulement pour initialiser `best_` depuis la sauvegarde ; synchroniser avec `main` avant (WFS-41 y travaille).
- RG-10 : ne **pas** modifier les fichiers de WFS-70 (`src/game/direction.*`, `player.*`) : le champ `steer` est seulement stocké ; le brancher au mode de direction est une ligne d'accroche faite par celui des deux tickets qui fusionne en second.
- RG-11 : banc : scène `WFS_BANC=sauvegarde` lancée deux fois dans le même job : 1er lancement écrit (voiture ≠ golf, peinture, record), le 2e journalise `SAUVEGARDE relue : car=… paint=… best=…` identiques ; capture `fin-record` (écran de fin avec « NOUVEAU RECORD »).
- Rien à réutiliser côté modèles ni sons ; seulement les **noms de clés et défauts** du web (`js/ui/screens.js` `store()`), pour garder la même signification.

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P3 Moyenne | WFS-24 | Sauvegarde et progression (records, voiture et réglages mémorisés) | E3 | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic), 2026-11-04 | WFS-12 | `feature/waforspeed-24` | [#54](https://github.com/wlfaleh/waforspeed_mobile/pull/54) |  |

Critères et notes de suivi :

**WFS-24 — Sauvegarde et progression** · E3 · priorité P3 Moyenne · ✅ Fusionné (QA de nuit à faire), PR #54 — Claude (session trafic), 2026-11-04, branche `feature/waforspeed-24` (règles : `regles-de-gestion/WFS-24.md`)
- [x] Meilleurs scores, voitures et réglages conservés
