# WFS-65 — Effets de conduite et d'accident
Épic : E4 · Priorité : P3 Moyenne · Dépend de : — · Taille : 2 à 3 jours (gros ticket, une seule CI à la fin)

## Besoin
La conduite doit « sentir » la vitesse et les chocs : étincelles, fumée, lignes de vitesse, vignette, flash. Aujourd'hui la version mobile n'a aucun de ces effets.

## Règles de gestion
- RG-01 : étincelles 3D au choc (voiture contre voiture, contre glissière) ; durée et nombre repris du jeu web.
- RG-02 : fumée sur les voitures accidentées, qui dure tant qu'elles sont arrêtées.
- RG-03 : lignes de vitesse sur les bords de l'écran, qui apparaissent avec la vitesse et surtout sous nitro.
- RG-04 : vignette sombre qui s'accentue avec la vitesse ; flash blanc court au choc et au départ du nitro.
- RG-05 : tout est coupé ou réduit en qualité basse ; aucun effet ne fait passer le jeu sous 30 images/s sur la machine de test.
- RG-06 : les effets ne touchent ni la physique ni le chrono.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- `js/world/particles.js` (`makeParticleSystem`, étincelles, fumée) ; `js/world/fx.js` (`drawFx` : lignes de vitesse, vignette, flash) ; la pluie est dans WFS-19.

## Critères d'acceptation (captures du banc PC)
- [ ] `choc-etincelles`, `accident-fumee`, `nitro-lignes` : effets visibles et fidèles au jeu web ; autres captures inchangées ; banc `0 écart(s)`

## Hors périmètre
Pluie et flaques (WFS-19), flou de mouvement (WFS-15).
