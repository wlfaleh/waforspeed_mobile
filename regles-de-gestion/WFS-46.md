# WFS-46 — Compteur de vitesse en arc (zone rouge)
Épic : E5 · Priorité : 0.49 (hors MVP, juste après) · Dépend de : WFS-27 · Taille : ≈ 2 h

## Besoin
Remplacer le chiffre seul de la vitesse (WFS-27) par le compteur en arc du jeu web, signature visuelle du HUD.

## Règles de gestion
- RG-01 : arc de départ 135° (`0,75π`), balayage 270°, échelle 0-380 ; graduation tous les 20, grand trait tous les 60.
- RG-02 : arc rempli en dégradé `#00E5FF` → `#FF6A00` jusqu'à la vitesse ; zone rouge `#FF1F4B` au-delà de 300 ; chiffres
  rouges au-delà de 300 (déjà en WFS-27).
- RG-03 : dessiné par segments `uiRect` / `uiCircle` existants (≤ 120 rectangles) ; pas de nouveau shader.
- RG-04 : même place que la vitesse de WFS-27 (bas centre), diamètre ≈ 26 % de la hauteur, rien sous les pouces.

## À réutiliser du jeu web
`js/ui/hud.js` → `drawSpeedo(k)` (angles, graduations, couleurs) ; `css/style.css` `.speedo`, `.speedo-read`, `.speedo.red`.

## Critères d'acceptation
- [ ] `hud-arc-120` et `hud-arc-320` (zone rouge) ; 1280x720 et 2400x1080 ; banc `0 écart(s)`

## Hors périmètre
Jauge circulaire du bouton Nitro tactile (commandes de `Game`, E0).

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| — | WFS-46 | Compteur de vitesse en arc (zone rouge) | E5 | ↪ Fusionné dans WFS-11 |  | — | `feature/waforspeed-46` |  |  |

Critères et notes de suivi :

**WFS-46 — Compteur de vitesse en arc** · E5 · priorité P2 Haute · ⏳ Prêt (règles : `regles-de-gestion/WFS-46.md`) — ↪ fusionné dans WFS-11
- [ ] `hud-arc-120`, `hud-arc-320` (zone rouge) en 1280x720 et 2400x1080 ; banc `0 écart(s)`
