# WFS-53 — Carte Campagne
Épic : E2 · Priorité : 12.1 · Dépend de : WFS-14

## Besoin (qui, quoi, pourquoi)
Deuxième nouvelle carte, sur le socle de WFS-14 : routes de campagne vallonnées.

## Règles de gestion
- RG-01 : valeurs `MAPS` campagne (déjà saisies par WFS-14) : `seed 512, curves [2,3,4], hills 120, sections 36, edge 1.38, trafic 26 (0,28-0,5)`.
- RG-02 : zones : Champs, Forêt, Village, Vignobles, Plaine, Bois de pins (`ZONE_LEN = 448`).
- RG-03 : décor : haies, champs, fermes, éoliennes (rotors animés), forêts (`vegBroadleaf`, `vegPine`).
- RG-04 : ambiance `envFor` : jour brouillard 820 m ; nuit `#0d1218`.
- RG-05 : ≤ 300 appels de dessin ; autres cartes inchangées.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- `js/world/decor.js` (`DECOR`, `ZONES` campagne), `js/world/life.js` (`vegBroadleaf`, `vegPine`), `js/world/maps.js` (`envFor`) ; socle natif de WFS-14 et `src/world/vegetation.*`.

## Critères d'acceptation (captures du banc PC)
- [ ] `carte-campagne-jour` / `carte-campagne-nuit` : collines, champs, haies, fermes, éoliennes reconnaissables
- [ ] ≤ 300 appels de dessin ; autres captures identiques à `main`

## Hors périmètre
Circuit (WFS-54), sélecteur (WFS-41).


## Périmètre élargi (décision de l'utilisateur 2026-11-03 : tickets plus gros, moins de builds)
Ce ticket absorbe : WFS-54, WFS-43. **Lire et appliquer aussi** `regles-de-gestion/WFS-54.md`, `regles-de-gestion/WFS-43.md` (toutes leurs règles et critères). Une seule branche, une seule CI à la fin ; livrer par morceaux sur la branche sans lancer de CI intermédiaire.
