# WFS-18 — Météo : pluie, flaques
Épic : E2 · Priorité : P4 Basse · Dépend de : WFS-09 ✅ (absorbe WFS-16 : route mouillée, flaques et reflets déjà faits dans `src/render/nuit.cpp`)

## Besoin (qui, quoi, pourquoi)
Il pleut la nuit en ville : gouttes animées, ciel couvert, route mouillée avec flaques, comme le jeu web.

## Règles de gestion
- RG-01 : pluie active quand `ENV.rain` : la nuit sur la carte ville seulement (`E.rain = night`) ; ni jour ni autres cartes.
- RG-02 : nombre de gouttes = `min(160, largeur×hauteur / 9000)` (0 si « réduire les animations ») ; longueur 10-24 px, vitesse 900-1500 px/s (échelle d'écran) × (0,7 + 0,6 × vitesse relative).
- RG-03 : inclinaison `(−0,12 − braquage×0,25) × (0,4 + vitesse relative)` ; trait `rgba(170,190,255,.2)`, 1 px ; goutte qui sort de l'écran réapparaît en haut à une abscisse aléatoire.
- RG-04 : ciel nocturne couvert en ville (`nightClouds 0.35`, `cloudCover 0.52`) ; flaques et reflets de WFS-16 actifs en même temps.
- RG-05 : dessiné en 2D par-dessus la 3D, après le bloom ; coût négligeable (< 0,3 ms) ; désactivé en qualité basse si le journal montre une baisse sous 60 i/s.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/world/fx.js` → `initRain`, `drawFx` (pluie, lignes de vitesse, vignette) ; `js/world/maps.js` (`rain`, `nightClouds`, `cloudCover`) ; `js/world/renderer.js` (ciel `makeSkyMaterial`).
- Pas de son de pluie dans le web (nouveau, optionnel, dans E6). Retour connu : respecter `prefers-reduced-motion` (équivalent : option de réduction des animations).

## Critères d'acceptation (captures du banc PC)
- [ ] capture nuit ville : traits de pluie fins, inclinés, sur toute la largeur, sans masquer la voiture ; route mouillée
- [ ] deux captures successives : gouttes à des positions différentes
- [ ] jour / plage / campagne / circuit : aucune pluie

## Zone de code et mise en œuvre (précision du PO, 2026-11-04)
- Zone : **`src/world/meteo.*` (nouveau fichier, E2)** uniquement. Un `System` enregistré comme les autres systèmes du monde (accroche ≤ 2 lignes annoncée au Journal si l'enregistrement l'exige).
- Pluie : traits 2D dessinés dans `System::drawUi` (passe d'interface, donc après la 3D et le bloom, RG-05) ; seulement en écran Course, carte ville, nuit.
- Ciel couvert (RG-04) : dans `System::ambiance(FrameParams&)` en assombrissant `skyTop` / `skyHorizon` et en rapprochant le brouillard ; **aucun nouveau champ dans `renderer.h`** (sinon ticket E0, hors de ce ticket). La route mouillée de WFS-09 n'est pas retouchée (`src/render/**` interdit).
- Banc : une capture `course-pluie-0` / `-1` (deux instants) ajoutée par le système lui-même ou via le banc `nuit` existant si possible sans toucher à `src/platform/**` ; sinon, deux captures du job `nuit` existant suffisent (`course-nuit-*` en ville).

## Hors périmètre
Orage, vent, pluie sur le pare-brise, changement de météo en cours de course.

> **Décision PO (2026-11-04)** : RG-05 (« désactivé en qualité basse si baisse sous 60 i/s ») **abandonnée** : coût mesuré 0,008 ms par image, le jeu web n'a pas cette règle. Aucun ticket.
