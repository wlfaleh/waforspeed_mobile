# QA — WFS-01 → WFS-05 (lot de la livraison 1)
Date : 2026-11-02 · Branche : feature/waforspeed-01 · Construction n° 22 (PR #1) · PR : #1

## Verdict : REFUSÉ (passage 1)

## Constat (émulateur Android 14, Pixel 6, 2400x1080, GPU SwiftShader)
- Installation et démarrage : OK (« QA pret », aucun plantage, appli encore active à la fin).
- **Bloquant — orientation** : la surface est signalée en 2400x1080 avec rotation 90° ; le moteur l'a traitée comme
  une surface en orientation native et a rendu la scène en **portrait 1080x2400** (journal : « Cibles de rendu :
  1080x2400 (logique) »). C'est la cause de l'image étirée vue sur le téléphone de l'utilisateur.
- **Bloquant — commandes** : vitesse 0 km/h pendant tout le scénario (accélération, nitro) ; le jeu tournait à
  0,1 image/s (GPU logiciel + MSAA x4 en pleine définition), l'appui et le relâchement arrivaient dans la même image
  et n'étaient jamais pris en compte.

## Corrections demandées
1. Pré-rotation : échanger largeur/hauteur de `currentExtent` quand la rotation est de 90°/270° (guide Android).
2. Qualité réduite sur GPU logiciel (sans MSAA, scène à 50 %) pour que l'émulateur soit testable.
3. Un appui bref sur une pédale doit compter au moins une mise à jour.

## Limites du test
Émulateur à GPU logiciel : performances non représentatives d'un vrai téléphone.
