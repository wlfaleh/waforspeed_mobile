# Pluie de référence et son de la pluie (WFS-288, RG-01 à RG-05)

Relu le 2026-11-08 par arwa-4. Seuls les **nombres** et les **formules** sont repris dans `src/audio/pluie_sons.*`. Aucune ligne de code des
dépôts cités n'entre dans ce dépôt. Les fichiers ont été lus en lecture seule, par clone partiel, dans un dossier temporaire, sans rien
exécuter (`regles-reseau.md` § 2.4 et § 3).

## 1. Sources relues

| Loi | Dépôt | Fichier | Commit | Licence | SHA-256 du fichier lu | Ouvrage cité par le dépôt |
|---|---|---|---|---|---|---|
| Répartition des tailles : N(D) = 8 000 · e^(−Λ·D) m⁻³·mm⁻¹, Λ = 4,1 · R^(−0,21) mm⁻¹ | `velatkilic/LISA` | `pylisa/atmos_models.py` (`n_mp_rain`, `n_mp_tot_rain`) | `ce06a7fbf2d0c9c4ad35d9dad55a79dfa8d5d7d4` | GPL-3.0 (nombres seulement) | `4873ecbe6c33bee3aa7bfbfb9759de2dbef0291d3a8c98865ac9688d9afb4028` | Marshall et Palmer (1948) |
| Vitesse de chute : v = 9,65 − 10,3 · e^(−0,6·D) m/s (D en mm) | `ltelab/disdrodb` | `disdrodb/fall_velocity/rain.py` (`get_fall_velocity_atlas_1973`) | `ccca01c72afc0ec074180ebe1e57c83bd29e1682` | GPL-3.0 (nombres seulement) | `f124b46a123b2e14e3b45c58f0820dc5a62b485790f48b8307594c43e10e3897` | Atlas, Srivastava et Sekhon (1973), *Rev. Geophys.* 11 ; Gunn et Kinzer (1949), *J. Meteor.* 6 |
| Résonance d'une bulle : R0 = √(3γP0 / ρ) / (2π·f) | `ryancinsight/kwavers` | `crates/kwavers-python/src/analytical_bindings/cavitation/bubble.rs` | `4b5ced3115fb856ed498ae08da750c8ca25d5d1c` | MIT | `f9946a15be1498a6a53cda0b1cfb6d2b3dc08bce44694a79a7b8c77c948dda47` | Minnaert (1933) |

Non trouvé sur GitHub, donc marqué **[à vérifier]** :
- le pic de 13 à 16 kHz des petites gouttes et la plage de 0,8 à 1,1 mm des gouttes qui laissent une bulle (Pumphrey et Crum, 1989) ;
- l'isolation de l'habitacle (passe-bas à 1 kHz, −20 dB) ;
- les résonances du toit (450 Hz) et du pare-brise (1,5 kHz) ;
- les sons des essuie-glaces (frottement, chocs, moteur).

## 2. Pluie de référence : R = 5 mm/h (pluie modérée, décision du PO)
Le calcul intègre de D = 0,5 à 8 mm (règle de Simpson, `pluie::reference`). Le test le refait d'une autre façon (trapèzes jusqu'à 12 mm) : écart ≤ 1 %.

| Grandeur | Valeur |
|---|---|
| Λ = 4,1 · 5^(−0,21) | **2,924 mm⁻¹** |
| Gouttes de D ≥ 0,5 mm par m³ : ∫ N dD | **634** |
| Impacts par m² et par seconde sur une surface horizontale : ∫ N · v dD | **2 104** |
| dont grosses gouttes (D ≥ 2 mm) | **55,8 par m² et par s, soit 2,65 %** |
| dont gouttes de 0,8 à 1,1 mm (bulles des flaques) | 576 par m² et par s |
| Contrôle : eau tombée, 3,6·10⁶ · ∫ N · v · π/6 · D³ dD | 5,90 mm/h (pour 5 : la loi de Marshall et Palmer prise avec celle d'Atlas donne 18 % de plus) |
| Vitesse de chute à 1 / 2 / 5 mm (Atlas) | 4,00 / 6,55 / 9,14 m/s (valeurs de Gunn et Kinzer données par le PO : 4,0 / 6,5 / 9,1 ; écart ≤ 1 %) |

## 3. Pare-brise et toit (RG-04)
Le pare-brise de la Golf a une pente dy / dz de 0,655, soit **α = 33,2°** sur l'horizontale. La valeur vient de la géométrie du cockpit de WFS-235 : du pied (−0,22 ; −1,10) au haut (+0,16 ; −0,52) par rapport à l'œil. WFS-282 a retiré la constante de `vehicule_cockpit.h`, elle est donc gardée dans `pluie::PENTE_PARE_BRISE`. Vues de la voiture,
les gouttes arrivent avec la vitesse (0, −v(D), +V). Le flux à travers le pare-brise vaut donc ∫ N · (v · cos α + V · sin α) dD, soit
**flux · cos α + V · sin α · concentration**. Le toit est horizontal : son flux ne dépend que de la chute.

| Vitesse | Impacts par m² et par s sur le pare-brise | Rapport à l'arrêt |
|---|---|---|
| 0 km/h | 1 760 | 1 |
| 50 km/h | 6 585 | **3,74** |
| 130 km/h | 14 305 | **8,13** |

Surfaces de la voiture du joueur, estimées depuis ses dimensions (WFS-158) **[proportions de la Golf 8, à vérifier pour les autres]** :
- toit : 0,60 × l'empattement de long et 0,70 × la largeur de large. Pour la Golf : **1,97 m²**, soit 4 145 impacts par seconde.
- pare-brise : 0,26 × la hauteur de haut (Golf : du pied à −0,22 m jusqu'au haut à +0,16 m de l'œil), sur 0,80 × la largeur. Pour la Golf : **0,99 m²**, soit 1 743 impacts par seconde à l'arrêt et 14 160 à 130 km/h.

Chaque impact tire la taille d'une goutte selon son poids dans le flux : N·v pour la chute, N pour la part balayée par la voiture. Son amplitude
suit la quantité de mouvement de la goutte, (D / 2 mm)³ × vitesse normale / v(2 mm), bornée à 4.

## 4. Bulles des flaques (RG-03)
- La formule de Minnaert donne f · r = √(3 · 1,4 · 101 325 / 998) / 2π = **3,29 m/s** (air dans l'eau, 1 atm). Les 13 à 16 kHz correspondent à des bulles de 0,21 à 0,25 mm de rayon.
- La cadence vient des gouttes de 0,8 à 1,1 mm qui tombent sur 0,1 m² de flaque proche [à vérifier], soit **58 bulles par seconde**.
- La cadence des gouttes isolées vient des grosses gouttes qui tombent sur 1 m² autour du micro [à vérifier], soit **56 par seconde**.
