# WFS-54 — Carte Grand Prix (circuit)
Épic : E2 · Priorité : 12.2 · Dépend de : WFS-14

## Besoin (qui, quoi, pourquoi)
Troisième nouvelle carte, sur le socle de WFS-14 : circuit de course.

## Règles de gestion
- RG-01 : valeurs `MAPS` circuit (déjà saisies par WFS-14) : `seed 777, curves [3,5,6], hills 12, sections 34, edge 1.66, trafic 16 (0,5-0,78)`.
- RG-02 : zones : Tribunes, Paddock, Virages rapides, Section boisée (`ZONE_LEN = 448`).
- RG-03 : décor : vibreurs rouge / blanc, tribunes, murs de pneus, bacs à gravier ; texture de route `roadCircuit`.
- RG-04 : ambiance `envFor` : jour brouillard 820 m ; nuit `#0e1020`, 640 m.
- RG-05 : ≤ 300 appels de dessin ; autres cartes inchangées.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- `js/world/decor.js` (`DECOR`, `ZONES` circuit, `GROUND`), `js/world/maps.js` (`envFor`), textures de route du jeu web ; socle natif de WFS-14.

## Critères d'acceptation (captures du banc PC)
- [ ] `carte-circuit-jour` / `carte-circuit-nuit` : vibreurs, tribunes, murs de pneus, gravier reconnaissables
- [ ] ≤ 300 appels de dessin ; autres captures identiques à `main`

## Hors périmètre
Sélecteur (WFS-41).

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| — | WFS-54 | Carte Grand Prix (circuit) | E2 | ↪ Fusionné dans WFS-53 |  | — | `feature/waforspeed-54` |  |  |

Critères et notes de suivi :

**WFS-54 — Carte Grand Prix (circuit)** · E2 · priorité P3 Moyenne · ✅ Fusionné avec WFS-53 (PR #61) (règles : `regles-de-gestion/WFS-54.md`) — ↪ fusionné dans WFS-53
- [ ] `carte-circuit-jour` / `-nuit` : vibreurs, tribunes, murs de pneus, gravier ; ≤ 300 appels de dessin
