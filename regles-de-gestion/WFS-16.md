# WFS-16 — Route mouillée avec reflets
Épic : E4 · Priorité : 18 · Dépend de : WFS-09

## Besoin (qui, quoi, pourquoi)
La nuit en ville et à la corniche, la chaussée brille : elle reflète les néons, les phares et le ciel, avec des flaques.
Le coût doit rester maîtrisé.

## Règles de gestion
- RG-01 : route mouillée = nuit sur les cartes ville et plage ; sèche sinon. Mouillée : `roughness 0.22`, `metalness 0.4`, reflet d'environnement 0,95 ; sèche : `0.66`, `0.05`, 0,2.
- RG-02 : reflets temps réel de la scène (voitures, néons, phares) sur la chaussée mouillée en qualité haute ; en qualité basse, reflet du ciel filtré seulement.
- RG-03 : variation de rugosité par bruit en coordonnées monde (pas de motif répété) : bruit à deux échelles (0,11 et 0,43 par mètre), traces de roues aux deux tiers des files (±0,3 / 0,7 de la voie), réparations plus sombres.
- RG-04 : flaques : là où le bruit dépasse 0,6-0,7 (moins les traces), rugosité 0,02, couleur ×0,5, normale ramenée à la géométrie (surface miroir).
- RG-05 : reflets recalculés au plus une fois sur deux ou à basse résolution (cibles en alternance, jamais lire et écrire la même texture).
- RG-06 : coût relevé au journal (≤ 2 ms mobile pour la version « haut »).

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/world/renderer.js` → `roadShader` (bruit `rnoise`, `puddle`, `trk`), uniform `RU.wet`, section `wet` de `applyEnv` ; `js/world/maps.js` (`envFor` nuit ville : `lift`, `sat`) ; `updateReflections`/`setupReflections` (cube ping-pong `cubeRTs`).
- Modèles : aucun. Retours connus : jamais de boucle de rétroaction (lire/écrire la même cible) ; reflets seulement qualité haute.

## Critères d'acceptation (captures du banc PC)
- [ ] `course-*` nuit ville : néons et phares reflétés, au moins une flaque miroir visible
- [ ] jour et campagne : route sèche mate
- [ ] pas de scintillement sur deux captures proches ; coût GPU au journal

## Hors périmètre
Pluie qui tombe (WFS-18), bloom (WFS-17).
