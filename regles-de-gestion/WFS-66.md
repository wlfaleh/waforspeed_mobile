# WFS-66 — Caméras et vue cockpit
Épic : E3 · Priorité : P3 Moyenne · Dépend de : — · Taille : 2 à 3 jours

## Besoin
Offrir les 3 vues du jeu web (poursuite, capot, cockpit avec volant et aiguille) et le comportement de caméra qui donne la sensation de vitesse.

## Règles de gestion
- RG-01 : 3 vues, qu'on change d'un bouton en course (et dans les réglages) ; la vue choisie est mémorisée.
- RG-02 : suivi lissé, regard porté devant la voiture, jamais sous la route.
- RG-03 : champ de vision qui s'élargit avec la vitesse ; vue cockpit à 72°.
- RG-04 : cockpit : tableau de bord, montants de pare-brise, volant qui tourne avec le braquage (`-braquage × 2,3`, lissé), aiguille de vitesse (0-300 km/h sur 2,2 rad).
- RG-05 : le code de caméra vit dans son propre fichier ; la seule modification du noyau est une accroche ≤ 10 lignes annoncée au Journal.

## À réutiliser du jeu web
`js/world/scene.js` : `CAM_MODES` (3 vues), cockpit (≈ lignes 94-123), suivi caméra et champ de vision (≈ ligne 168) ; modèles et textures du cockpit sont dessinés par code.

## Critères d'acceptation
- [ ] `camera-poursuite`, `camera-capot`, `camera-cockpit` (1280x720 et 2400x1080) ; volant braqué visible en virage ; banc `0 écart(s)`

## Hors périmètre
Intérieurs détaillés propres à chaque voiture.
