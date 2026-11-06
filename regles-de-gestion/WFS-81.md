# WFS-81 — Nuit sur la corniche et le circuit : vrais lampadaires, lumières alignées, feux du trafic sans double halo
Épic : E2 (+ accroche E3 dans `src/game/traffic_system.cpp`) · Priorité : P3 Moyenne · Dépend de : WFS-09 ✅, WFS-53 ✅, WFS-67 ✅ ·
Attribuable quand WFS-18 (E2) est terminé (un ticket actif par épic)

## Besoin (qui, quoi, pourquoi)
La nuit (WFS-09), `src/render/nuit.cpp` place lui-même des têtes de lampe sur la corniche (tous les 30 segments) et le
circuit (tous les 34) parce que ces cartes n'inscrivent pas leurs lampadaires : sur la **corniche**, il n'y a **aucun poteau**
(lumière dans le vide) ; sur le **circuit**, les mâts existent depuis WFS-53 (`circuit.cpp`, pas 34 / 40 / 44 selon la
zone) mais les lumières de `nuit.cpp` suivent un pas uniforme et ne tombent pas sur les mâts. Par ailleurs, la nuit, une
voiture du trafic qui freine reçoit **deux halos rouges** : le feu arrière du moteur (`Renderer::feuxVoiture`, WFS-09,
`renderer.cpp` l. 902) et le halo stop de WFS-67 (`TrafficSystem::dessineHalos`).

## Règles de gestion
- RG-01 : corniche (`src/world/plage.cpp`) : lampadaires « beach » du jeu web, côté et pas par zone exactement comme
  `js/world/decor.js` l. 115 (`z0 + 15`, pas 30, côté 1), l. 127 (`z0 + 15`, pas 28, côté 1), l. 134 (`z0 + 20`, pas 40,
  côté 1), l. 142 (`z0 + 12`, pas 26, côté −1) — retrouver la zone de chaque ligne dans `decor.js`.
- RG-02 : forme « beach » (`decor.js`, `lamp`) : poteau blanc `#e8e8e8` 0,18 × 5,8 × 0,18 à `RW + 0,8`, boule 0,35 couleur
  `lampWarm #ffd9a8` à `RW + 0,8`, hauteur 6,1 (partie néon, allumée la nuit comme les autres lampes). Ajouter ce style à
  `CarteDecor::lampe` (`decor_carte.cpp`), qui fait déjà « city » et « flood ».
- RG-03 : corniche et circuit **inscrivent leurs lampadaires** dans `lampadaires()` (`src/render/eclairage.h`) comme la ville
  (`ville.cpp` l. 181) : position de la lumière du jeu web (`scene.js` l. 278) : beach `RW + 0,6` à 5,6 m, flood `RW + 4` à
  16 m, côté du poteau ; styles `StyleLampadaire::Plage` / `Stade` existants. Les lignes de repli de `nuit.cpp` ne servent
  alors plus (on ne les modifie pas : zone E4).
- RG-04 : campagne inchangée (pas d'éclairage public, WFS-09 RG-06) ; de jour, les poteaux se voient, lampes éteintes
  (neonK 0,5 comme les autres).
- RG-05 : double halo : regarder d'abord `nuit-*-stop` / captures de nuit avec trafic sur `main`. Si deux halos rouges se
  voient sur une voiture qui freine : la nuit, le trafic **ne dessine plus son halo stop** et donne l'état « stop » au
  feu arrière du moteur (`r.feuxProchaineVoiture`, comme le joueur dans `nuit.cpp`) ; les clignotants et feux de détresse de
  WFS-67 restent. Accroche ≤ 10 lignes dans `traffic_system.cpp`, annoncée au Journal. Si un seul halo se voit : ne rien
  changer et le noter au Journal.
- RG-06 (zone) : `src/world/plage.cpp`, `src/world/circuit.cpp`, `src/world/decor_carte.*`, banc `cartes_banc.cpp` ;
  **interdit : `src/world/meteo.*`** (WFS-18) et `src/render/**`.

## À réutiliser
- Jeu web (`wlfaleh/waaps`, `waforspeed/`) : `js/world/decor.js` (`generateDecor`, lignes `lamp(n, …, 'beach')` et
  `'flood'`, primitive `lamp`, couleurs `C.pole`, `lampWarm`, `lampCool`), `js/world/scene.js` l. 270-282 (position,
  couleur, portée et intensité des lumières).
- Natif : `CarteDecor::lampe` et les items `T_LAMP` de `circuit.cpp`, `lampadaires()` + `ville.cpp` l. 181. Ne rien recréer.

## Critères d'acceptation (banc PC)
- [ ] `nuit-plage-*` : poteaux blancs à boule sous chaque lumière, aucune lumière dans le vide ; `carte-plage-*` de jour :
      poteaux visibles
- [ ] `nuit-circuit-*` : chaque tache de lumière sous un mât de projecteur ; journal `Nuit : carte circuit, n lampadaires`
      égal au nombre de mâts proches inscrits
- [ ] voiture du trafic qui freine la nuit : un seul halo rouge par feu ; clignotants toujours visibles ; banc ville
      et campagne inchangés hors de ce point

## Hors périmètre
Lampadaires de la campagne, nouvelles formes de lampe, ombres portées des lampadaires, pluie (WFS-18).

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P3 Moyenne | WFS-81 | Nuit corniche et circuit : vrais lampadaires, lumières alignées, feux du trafic sans double halo | E2 (+ accroche E3 `traffic_system.cpp`) | ✅ Fusionné (QA de nuit à faire) | Claude (session trafic, agent WFS-81), 2026-11-04 | WFS-09 ✅, WFS-53 ✅, WFS-67 ✅ | `feature/waforspeed-81` | #75 |  |

Critères et notes de suivi :

**WFS-81 — Nuit corniche et circuit** · E2 · P3 Moyenne · ✅ Fusionné (QA de nuit à faire), PR #75 — Claude (session trafic, agent WFS-81), 2026-11-04, branche `feature/waforspeed-81` (règles : `regles-de-gestion/WFS-81.md`)
- [ ] `nuit-plage-*` : poteaux sous chaque lumière ; `nuit-circuit-*` : lumière sous chaque mât ; trafic qui freine la nuit : un seul halo rouge par feu
