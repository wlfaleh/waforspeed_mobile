# Tunnel routier générique des cartes de données — valeurs réelles (WFS-392)

> Écrit par Claude, agent cartes (01UY1Vv2ghpU), le 2026-11-11, avec l'avis de l'agent `expert-realisme-3d` du même jour.
> **Réserve** : le réseau des agents est limité à GitHub (`regles-reseau.md`) : ni la norme CIE 88:2004, ni le dossier pilote du CETU,
> ni une fiche de fabricant n'ont pu être relus. Les valeurs marquées **[à confirmer]** viennent de la mémoire de ces documents ;
> elles sont dans les plages demandées par la règle RG-02 du ticket (10 à 25 m) mais ne sont pas des mesures vérifiées à ± 1 %.

## Documents de référence (non téléchargés)
- CIE 88:2004, « Guide for the lighting of road tunnels and underpasses » : zones d'un tunnel (accès, seuil, transition, intérieur,
  sortie), luminance de seuil L_seuil = k × L20, papillotement.
- CETU (Centre d'études des tunnels, France), « Dossier pilote des tunnels — Équipements — Éclairage ».
- CEN CR 14380 « Lighting applications — Tunnel lighting ».

## Implantation retenue
| Grandeur | Valeur | Source |
|---|---|---|
| Files de luminaires | 2, sous la dalle, au milieu de chaque demi-chaussée (± 3,75 m de l'axe pour une chaussée de 15 m), vitre horizontale | éclairage symétrique d'un tunnel à dalle plate [à confirmer, CETU] |
| Hauteur de la vitre | 7,25 m (0,35 m sous la dalle à 7,60 m) : gabarit routier de 4,75 m libre | gabarit [à confirmer, CETU] ; dalle à 7,60 m : même hauteur que les tunnels de la Ville et de la Corniche du jeu, choisie pour que la caméra de la vue Arrière (6,3 m) reste sous la dalle (un tunnel réel à dalle plate est plus bas, vers 5,5 à 6 m) |
| Pas en section courante | 12 m (défaut du paquet ; bornes 6 à 25 m) | plage 10 à 25 m (RG-02) [à confirmer, CETU] |
| Pas dans la zone de seuil | 6 m (renforcement de jour à chaque bouche) | seuil plus dense que l'intérieur (CIE 88:2004) [à confirmer] |
| Longueur du seuil | 60 m (défaut), jamais plus du tiers du tunnel | distance d'arrêt : v × 1 s + v² / (2 g × 0,30) = 47 m à 50 km/h, 84 m à 70 km/h (calcul) |
| Zone de transition | non modélisée : on passe du pas de 6 m au pas de 12 m | simplification (la loi réelle L = L_seuil × (1,9 + t)^−1,4 demande 170 à 300 m) |

Papillotement (fréquence = vitesse / pas ; gêne entre 4 et 11 Hz si elle dure plus de 20 s [CIE 88:2004, à confirmer]) :
à 12 m : 1,2 Hz à 50 km/h, 2,1 Hz à 90 km/h, 5,8 Hz à 250 km/h ; à 6 m : 2,3 Hz à 50 km/h, 4,2 Hz à 90 km/h (seuil traversé en moins de 5 s).

## Luminaire
| | Sodium haute pression (défaut) | LED 4000 K |
|---|---|---|
| Corps (long × large × haut) [à confirmer] | 0,90 × 0,34 × 0,16 m, profilé en trapèze | 0,80 × 0,30 × 0,10 m, 6 ailettes de 25 mm |
| Vitre plane | 0,78 × 0,24 m, en retrait de 5 mm dans un cadre de 3 cm | 0,72 × 0,24 m, 3 traverses (4 modules) |
| Couleur de la lumière | ≈ 2000 K, sRGB `#FFB46B` (celle des lampadaires du moteur) | 4000 K, sRGB ≈ `#FFD1A3` |
| Fixation | 2 étriers (plat d'acier de 40 × 5 mm), chemin de câbles en U de 0,20 × 0,06 m à 0,12 m sous la dalle, tiges filetées de 12 mm tous les 2 m | idem |
| Matériaux | aluminium thermolaqué gris (albédo ≈ 0,40) ; acier galvanisé (albédo ≈ 0,45) ; mats | idem |

## Niveaux de lumière réels (pour mémoire) [CIE 88:2004, à confirmer]
- Seuil de jour : 150 à 350 cd/m² (k = 0,05 à 0,07 × L20 de 3 000 à 5 000 cd/m²) ; intérieur de jour : 2 à 6 cd/m² ; nuit : 1 à 2 cd/m².
- Dans le moteur, la nuit : une lampe du registre des lampadaires à 7,25 m donne 1,1 × (1 − 7,25 / 26)² = 0,57 unité, soit ≈ 20 lux au sol
  (dans la charte : 10 à 30 lux), soit ≈ 0,6 cd/m² sur l'enrobé.
- De jour : l'exposition automatique du moteur ne descend pas sous EV100 10 (`src/render/eclairage_unites.h`, épic E13) : l'intérieur
  d'un tunnel réel (EV 5 à 7) y paraît plus sombre que dans la réalité. Ce réglage est hors de la zone de WFS-392 (signalé au PO).
