# WFS-62 — Roues animées : elles tournent à la vitesse de la voiture et braquent
Épic : E3 · Priorité : 17.8 (hors MVP) · Dépend de : WFS-61 · Taille : ≈ 2 h

## Besoin (qui, quoi, pourquoi)
Quand la voiture roule, ses roues tournent ; quand elle s'arrête, elles s'arrêtent ; quand on tourne le volant, les roues avant se braquent. Sans cela la voiture paraît « glisser » sur la route.

## Règles de gestion
- RG-01 : angle de roulage de chaque roue += distance parcourue / rayon de la roue, à chaque image (donc vitesse nulle = roues immobiles, marche arrière = sens inverse).
- RG-02 : braquage des roues avant = angle de roue du modèle physique × 1,6 (jeu web), limité à ±0,6 rad ; roue intérieure au virage braque 1,12 fois, extérieure 0,9 fois (épure d'Ackermann).
- RG-03 : en accident (choc), le braquage est ramené à 0 ; les roues continuent de tourner selon la vitesse.
- RG-04 : s'applique à la voiture du joueur et au trafic proche ; le trafic lointain est ignoré (coût).
- RG-05 : l'angle de roulage est conservé entre les images (pas de saut) ; pas d'impact sur la physique ni sur les temps de course.

## À réutiliser du jeu web
`js/world/scene.js` lignes ≈ 208 et 238 (appels `animWheels(yaw * 1.6, vitesse × dt)`) ; `js/cars/models.js` → `animWheels` (Ackermann). Côté natif : angle de roue déjà calculé dans `src/game/player.cpp` (`wheelAngle`).

## Critères d'acceptation (captures du banc PC)
- [ ] Deux captures successives de la voiture à l'arrêt : roues identiques ; en roulant : repère de jante à un angle différent
- [ ] Capture en virage : roues avant braquées vers l'intérieur, arrière droites
- [ ] Banc `0 écart(s)` hors voitures animées ; aucun ralentissement mesurable

## Hors périmètre
Flou de roue à grande vitesse, suspension / roulis, fumée de pneus.
