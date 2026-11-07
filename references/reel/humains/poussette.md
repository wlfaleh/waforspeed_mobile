# Poussette citadine (WFS-240, RG-01) — valeurs de contrôle « à confirmer »

Poussette « 4 roues » d'aujourd'hui (pliable, hamac, capote, panier), ordres de grandeur des fiches techniques du commerce.
Aucune page de fabricant n'a pu être lue depuis le réseau des agents (`regles-reseau.md`) : les valeurs sont **à confirmer**.

| Mesure | Valeur retenue | Plage du commerce (à confirmer) | Dans le jeu (`src/world/foule_poussette.h`) |
|---|---|---|---|
| Longueur (poignée → avant des roues avant) | 1,00 m | 0,85 à 1,05 m | `ESSIEU_AVANT_Z + ROUE_AV − POIGNEE_Z` = 0,97 m |
| Largeur hors tout | 0,60 m | 0,48 à 0,62 m | `LARGEUR` = 0,60 m |
| Hauteur de la poignée | 1,00 m | 0,95 à 1,07 m | `POIGNEE_Y` = 1,00 m |
| Roues arrière / avant | Ø 25 / 18 cm | Ø 15 à 30 cm | `ROUE_AR`, `ROUE_AV` |
| Hauteur de l'assise | 0,50 m | 0,45 à 0,55 m | `SIEGE_Y` = 0,50 m |

Matériaux : hamac et capote en tissu polyester (mat), châssis en aluminium (métal), roues et poignée en plastique ou mousse noirs.
Le petit (≈ 2 ans, 0,87 m) est le morphotype de 4 ans (1,02 m) à 85 %.
