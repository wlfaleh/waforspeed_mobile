# QA — WFS-05 : retours QA de la livraison 1 (orientation, commandes)
Date : 2026-11-02 · Branche : feature/waforspeed-05 · Construction n° 40 · Release : apercu-waforspeed-05

## Verdict : VALIDÉ (avec limite connue)

## Captures regardées (émulateur Android 14, 2400x1080)
- `qa-01-demarrage` : image en **paysage**, proportions correctes (commandes rondes), Golf entière avec son ombre,
  route, trottoirs, ciel. ✅
- `qa-03-acceleration-8s` / `qa-05-braquage` : accélérateur allumé pendant l'appui, jauge de vitesse qui démarre. ✅
- `qa-07-fin` : rien de cassé, appli toujours en marche. ✅

## Journal
- « Swapchain 1080x2400, rotation 2 » puis « Cibles de rendu : 2400x1080 (logique) » : pré-rotation correcte.
- Vitesse 0 → 9 km/h pendant l'accélération, puis 4 → 3 km/h après le freinage : les commandes sont prises en compte.
- Aucun plantage.

## Limite
L'émulateur (GPU logiciel SwiftShader) tourne à ~0,2 image/s même en qualité réduite : la voiture n'a pas le temps de
prendre de la vitesse pendant le scénario. Limite de l'émulateur, pas du jeu ; performances réelles à mesurer sur téléphone.
