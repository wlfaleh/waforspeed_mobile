# WFS-85 — [URGENT] Caméra de poursuite plus haute (voir les voitures devant)
Épic : E3 · Priorité : P1 Urgent · Dépend de : WFS-66 ✅ · Taille : 2 h · Retour de l'utilisateur (test réel sur téléphone, 2026-11-04)

## Besoin
Avec la vue Poursuite actuelle, on voit surtout sa propre voiture et on ne voit pas les voitures devant elle. On veut la caméra un peu plus haute (et un peu plus loin) pour voir la route et le trafic devant.

## Règles de gestion
- RG-01 : la vue **Poursuite par défaut** reprend la vue « loin » du jeu web : distance 8 m, hauteur 2,5 m, regard 14 m devant, hauteur du point visé 0,9 m, champ de vision 62° (`CAM_MODES` : `{ id: 'loin', dist: 8, h: 2.5, ahead: 14, lookH: 0.9, fov: 62 }`).
- RG-02 : l'ancienne vue basse (« proche » : 4,6 m, 1,28 m, 58°) reste disponible comme vue supplémentaire dans le cycle des caméras et dans les réglages (WFS-41) ; le choix est mémorisé.
- RG-03 : le suivi lissé, le champ de vision qui s'élargit avec la vitesse et le « jamais sous la route » restent inchangés.
- RG-04 : la voiture du joueur ne doit pas cacher l'horizon : le joueur voit au moins les 3 voitures devant lui sur une capture de trafic.
- RG-05 : valeurs dans des constantes faciles à ajuster (le réglage final se fait au test sur téléphone).

## À réutiliser du jeu web
`js/world/scene.js` `CAM_MODES` (lignes ≈ 90-96) et `updateCamera` ; natif : `src/game/camera_vue.*` (WFS-66).

## Critères d'acceptation (captures du banc PC)
- [ ] `trafic-cam0` / `course-*` : la voiture du joueur plus petite à l'écran, la route et au moins 3 voitures devant visibles
- [ ] bascule entre les vues : Poursuite (haute), Proche (basse), Capot, Cockpit

## Hors périmètre
Cockpit et capot (inchangés).
