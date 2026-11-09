# Neige et adhérence — valeurs de référence (WFS-346)

Valeurs réelles utilisées par les réglages de saison des paquets de cartes (`saison` du bloc `decor`).

| Grandeur | Valeur retenue | Plage réelle | Source |
|---|---|---|---|
| μ pneu / asphalte sec | 1,0 | 0,8 à 1,0 | Wong, *Theory of Ground Vehicles*, 4e éd., tableau 1.4 ; valeur du moteur (`roues.h`) |
| μ pneu / neige tassée | 0,25 | 0,2 à 0,3 | Wong, tableau 1.4 (« snow, packed » : 0,2) ; Bosch, *Automotive Handbook* (neige tassée 0,2 à 0,3) |
| μ pneu / neige poudreuse ou bas-côté enneigé | 0,2 | 0,15 à 0,25 | Bosch, *Automotive Handbook* |
| Taille d'un flocon | 2 à 5 mm | 1 à 10 mm | RG du ticket ; Magono & Lee (1966), classification des cristaux |
| Vitesse de chute d'un flocon | 1 à 1,5 m/s | 0,5 à 1,5 m/s | Locatelli & Hobbs (1974), *J. Geophys. Res.* 79 |
| Albédo de la neige fraîche (couleur du bas-côté) | ≈ 0,8 à 0,9 | 0,8 à 0,9 | Warren (1982), *Rev. Geophys.* 20 |
| Albédo de la neige tassée et salie (chaussée) | ≈ 0,5 à 0,6 | 0,4 à 0,7 | Warren (1982) |

Distance de freinage attendue à 100 km/h (ABS) : d = v² / (2 μ g) ≈ 39 m au sec (μ = 1,0), ≈ 157 m sur neige tassée (μ = 0,25).
