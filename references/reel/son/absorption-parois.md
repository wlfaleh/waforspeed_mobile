# Absorption des parois dures et diffraction par plusieurs obstacles (WFS-278, RG-01 et RG-04)

Relu le 2026-11-07 par arwa-4. Seuls les **nombres** et les **formules** sont repris dans `src/audio/parois_sons.*` ; aucune ligne de code
des dépôts cités n'entre dans ce dépôt. Fichiers lus en lecture seule dans un dossier temporaire, rien exécuté (`regles-reseau.md` § 2.4 et § 3).

## 1. Coefficients d'absorption (RG-01)
Source : base de matériaux de **pyroomacoustics**, dont le module `parameters.py` indique que les valeurs viennent de l'annexe de
**M. Vorländer, *Auralization: Fundamentals of Acoustics, Modelling, Simulation, Algorithms and Acoustic Virtual Reality*, Springer, 2008**.

| Dépôt | Fichier | Commit | Licence | SHA-256 du fichier lu |
|---|---|---|---|---|
| `LCAV/pyroomacoustics` (EPFL) | `pyroomacoustics/data/materials.json` | `08d16968f8db414affaf84771a650358a7961446` | MIT | `1249f0cfdcd4598cf98ec9be05230f910e53aa1da4861d7fe3f88de23a24e0e0` |

α par bande d'octave (125 Hz à 8 kHz), matériaux des façades du jeu :

| Clé | Description (source) | 125 | 250 | 500 | 1 k | 2 k | 4 k | 8 k |
|---|---|---|---|---|---|---|---|---|
| `hard_surface` | murs et surfaces dures, moyenne (brique, enduit, sols durs) | 0,02 | 0,02 | 0,03 | **0,03** | 0,04 | 0,05 | 0,05 |
| `brickwork` | murs de brique enduits | 0,01 | 0,02 | 0,02 | **0,03** | 0,03 | 0,04 | 0,04 |
| `rough_concrete` | béton brut | 0,02 | 0,03 | 0,03 | **0,03** | 0,04 | 0,07 | 0,07 |
| `unpainted_concrete` | béton lisse non peint | 0,01 | 0,01 | 0,02 | **0,02** | 0,02 | 0,05 | 0,05 |
| `limestone_wall` | murs de calcaire | 0,02 | 0,02 | 0,03 | **0,04** | 0,05 | 0,05 | 0,05 |
| `brick_wall_rough` | mur de brique crépi | 0,03 | 0,03 | 0,03 | **0,04** | 0,05 | 0,07 | 0,07 |
| `glass_window` | vitrage (0,68 kg/m²) | 0,10 | 0,05 | 0,04 | **0,03** | 0,03 | 0,03 | 0,03 |

### Valeurs retenues à 1 kHz (bande de calcul du jeu)
| Paroi du jeu | Matériau | α | Perte à la réflexion 10·lg(1 − α) |
|---|---|---|---|
| immeubles et entrepôts de la Ville, garages (WFS-176), maisons, commerces et fermes de la Campagne, villas, garages du Grand Prix | `hard_surface` | 0,03 | −0,13 dB |
| mur des stands, tribunes | `rough_concrete` | 0,03 | −0,13 dB |
| mur-rideau du centre commercial | `glass_window` | 0,03 | −0,13 dB |
| roche des lacets de la Corniche (calcaire), casino (pierre), église | `limestone_wall` | 0,04 | −0,18 dB |

**Avant / après** : WFS-275 avait α = 0,05 partout, valeur non sourcée, soit −0,22 dB. L'écart est de +0,09 dB pour α = 0,03 et de +0,04 dB
pour α = 0,04 à l'ordre 1, et de +0,27 dB au plus à l'ordre 3. La règle (perte ≤ 0,3 dB par réflexion) reste tenue.

## 2. Diffraction par plusieurs obstacles (RG-04)
Source : implémentation de référence de la propagation **CNOSSOS-EU** (CSTB, D. van Maercke, 2013-2014), fichier `ISO-9613-2.cpp`.
Elle applique l'équation 14 d'ISO 9613-2:1996 (§ 7.4) et ses plafonds.

| Dépôt | Fichier | Commit | Licence | SHA-256 du fichier lu |
|---|---|---|---|---|
| `genell/Cnossos-EU-SWE` (copie non officielle du logiciel CNOSSOS-EU) | `CNOSSOS-EU Task 2 - distrib 12-05-2014/PropagationPath/ISO-9613-2.cpp` (`getDz`, plafonds) | `8b6fe41ab6229cb7da0edb318b5f860600326e17` | « licence.EU.txt » (licence de l'UE, non relue) | `8128ecad9d18867ccbd14314d2fed676e5084234504823d8e2ec8e6ee2f01697` |

- D_z = 10·lg(3 + (C2 / λ) · C3 · z · K_met).
- C2 vaut 20 (diffraction par-dessus, images du sol comptées) ou 40 (diffraction latérale).
- C3 = 1 pour une seule arête. Pour plusieurs arêtes, C3 = (1 + (5λ / e)²) / (1/3 + (5λ / e)²), où e est la distance entre la première et la dernière arête.
- z est la différence de marche, c'est-à-dire le détour. K_met = 1 à courte distance.
- Plafond : **20 dB** pour une seule diffraction, **25 dB** pour plusieurs.

Dans le jeu, la diffraction simple garde la formule de WFS-275 : D = 10·lg(3 + 20·N), N = 2·δ / λ, ce qui revient à C2 = 40 et C3 = 1.
La diffraction multiple utilise la même forme avec le C3 ci-dessus : D = 10·lg(3 + 20·N·C3), plafond 25 dB. Le chemin tendu est calculé
par-dessus tous les obstacles traversés (profil en hauteur) ou autour d'eux (plan horizontal), en prenant le plus court. Une seule arête
correspond à e = 0 et C3 = 1. Le jeu ne retient jamais moins que le plus fort des masques simples.

Deux obstacles sont **accolés** quand moins de 1 m les sépare : morceaux d'une même roche (déclarée segment par segment), garages
côte à côte, immeubles mitoyens. Ils forment un **groupe**, et le chemin « autour » fait le tour du groupe entier. Sinon, le chemin le plus
court contournerait un morceau en passant à travers son voisin. Un groupe est traité comme plusieurs arêtes, avec C3 et le plafond de
25 dB, que la ligne traverse un morceau ou deux. Un obstacle seul, sans voisin accolé, garde la diffraction simple de WFS-275
(C3 = 1, plafond 20 dB).

## 3. Mesures du jeu (banc, 2026-11-07)
| Carte | Obstacles des masques | Profondeur (m) | Hauteur (m) |
|---|---|---|---|
| Ville (tracé 2, lieux de WFS-176 compris) | 625 | 9,0 à 26,0 (moyenne 14,4) | 6,0 à 101,1 |
| Corniche (roche côté montagne et villas) | 254 | 7,0 à 85,4 (moyenne 36,2) | 4,0 à 18,1 |
| Campagne (maisons, commerces, église, fermes) | 138 | 8,0 à 11,0 (moyenne 9,3) | 5,0 à 9,0 |
| Grand Prix (mur des stands, garages, tribunes) | 240 | 0,4 à 12,0 (moyenne 5,6) | 2,7 à 7,0 |

La profondeur de la roche de la Corniche va de sa face jusqu'au bout de la pente de garrigue (RW + 90 m), sans dépasser le bord du sol
de la branche (`bordSol` de `plage.cpp` : 0,9 fois le rayon intérieur du virage, ou la mi-distance de la branche suivante des lacets).
Elle vaut au moins 3,5 m, l'épaisseur de la face raide.
