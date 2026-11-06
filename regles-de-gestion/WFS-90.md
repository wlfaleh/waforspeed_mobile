# WFS-90 — Conduite libre : demi-tour, marche arrière, rouler dans l'autre sens
Épic : E3 · Priorité : P2 Haute · Dépend de : — · Taille : 1 à 2 jours · Demande de l'utilisateur (test réel, 2026-11-04) : « donner la possibilité à l'utilisateur de faire ce qu'il veut, il veut tourner de l'autre sens »

## Besoin
Aujourd'hui la voiture ne peut jamais se retourner : son angle par rapport à la route est limité (`maxHeading`, `player.cpp`) et elle se **réaligne toute seule** sur la route. Le joueur veut pouvoir faire demi-tour et rouler dans l'autre sens s'il le décide.

## Règles de gestion (on veut que ça marche, pas la perfection)
- RG-01 : l'angle de la voiture par rapport à la route n'est plus limité à un petit angle : il peut aller jusqu'à un demi-tour complet (±180°), à basse vitesse comme en marche arrière.
- RG-02 : le **réalignement automatique** sur la route est supprimé ou très faible quand l'angle est grand ; il ne reste que pour les petits angles à haute vitesse (confort de conduite actuel inchangé en ligne droite).
- RG-03 : demi-tour possible en un ou plusieurs manœuvres : braquer et avancer, puis marche arrière en braquant dans l'autre sens ; la marche arrière reste à ~30 km/h (`REV_MAX`), sans devoir attendre l'arrêt complet pour passer de l'avant à l'arrière.
- RG-04 : la voiture reste dans la route (bords et glissières comme aujourd'hui, pas de sortie dans le décor) ; le trafic roule toujours dans le sens normal : un choc de face est un choc normal.
- RG-05 : la caméra de poursuite (WFS-66 / WFS-85) suit le cap de la voiture, y compris retournée ; le portique, les checkpoints et le chrono ne comptent que dans le sens de la course (rouler à l'envers ne donne ni points ni checkpoints).
- RG-06 : les roues, les feux et le sens de la voiture (avant / arrière) restent cohérents à l'écran ; aucun plantage ni valeur absurde (la position sur la route reste correcte après plusieurs tours).
- RG-07 : un bouton « Demi-tour » (facultatif, si simple) remet la voiture dans le bon sens à l'arrêt.

## À réutiliser du jeu web
`js/game/update.js` (≈ lignes 160-172 : `heading`, `maxHeading`, réalignement) ; natif : `src/game/player.cpp` (lignes ≈ 28-50, mêmes formules). Le jeu web a la même limite : c'est un changement de comportement voulu, propre à la version mobile.

## Critères d'acceptation
- [ ] test CPU : depuis l'arrêt, braquer + avancer puis reculer permet d'atteindre un cap de 180° ; `position` et `dist` restent cohérentes
- [ ] capture : voiture retournée sur la route, caméra derrière elle, trafic qui vient en face ; banc sans régression

## Hors périmètre
Faire rouler le trafic dans les deux sens, routes à double sens, points pour rouler à l'envers.

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P2 Haute | WFS-90 | Conduite libre : demi-tour, marche arrière facile, rouler dans l'autre sens | E3 (exception : 2e ticket E3, fichiers séparés de WFS-89) | ✅ Fusionné (fusion par confiance, QA de nuit à faire) | Claude (agent 4, session 011cpqZan), 2026-11-04 | — | `feature/waforspeed-90` | #78 |  |

Critères et notes de suivi :

**WFS-90 — Conduite libre : demi-tour, marche arrière, autre sens** · E3 · priorité P2 Haute · ✅ Fusionné le 2026-11-04 par confiance (PR #78, QA de nuit : `qa/Check.md`) — Claude (agent 4, session 011cpqZan), 2026-11-04, branche `feature/waforspeed-90` (règles : `regles-de-gestion/WFS-90.md`). Zone (exception E3, fichiers séparés de WFS-89) : `src/game/player.*`, `src/game/course*`, `src/game/camera_vue.*`, `src/game/direction.*`, `src/game/roues.*`, nouveau `tests/test_conduite_libre.cpp` ; accroches ≤ 10 lignes annoncées : `CMakeLists.txt`, `src/game/traffic_system.cpp`, `src/ui/hud.cpp`, bouton « Demi-tour » ; interdit : `src/game/duel*`, `reseau_*`, `binarypack*`, `tests/test_binarypack*`, `src/ui/ecrans/salon.*` (WFS-89), `src/render/**` (WFS-88)
- [ ] demi-tour possible, caméra qui suit, chrono et checkpoints seulement dans le bon sens
