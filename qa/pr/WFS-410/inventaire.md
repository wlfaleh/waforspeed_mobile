# WFS-410 — inventaire des bâtiments hors kit (RG-01)

Relevé fait dans le code de `main` (2026-11-13) avant de coder : chaque bâtiment de la Ville, de la Corniche et de la Campagne que le kit
d'architecture (`archi_*`, WFS-161 / WFS-402) ne dessine pas, et l'état de ses portes et fenêtres.
« Peint à plat » = un rectangle sombre ou une fine boîte posée devant le mur (forme nue, règle du 2026-11-09 19:41 UTC).

## Avant / après

| Carte | Bâtiment (fichier) | Avant (`main`) | Après (cette branche) |
|---|---|---|---|
| Corniche | Casino (`plage.cpp`, `casino`) | 33 baies peintes à plat (`vitre`), appuis en boîtes | 33 baies percées : 8 fenêtres hautes du rez-de-chaussée, 3 portes vitrées à deux vantaux sous le portique, 22 fenêtres d'étage et d'attique ; menuiseries réelles WFS-402 (dormant, ouvrants, petits bois, imposte, appui de pierre à larmier) |
| Corniche | Cafés (`plage.cpp`, `cafe`) | vitrine et porte peintes, 3 fenêtres peintes, volets en boîtes plates | 2 vitrines à soubassement, porte du café, 3 fenêtres à persiennes du style méditerranéen (10 à 30 % fermées ou entrouvertes) |
| Corniche | Galerie marine (`plage.cpp`, `galerie`) | mur-rideau peint, baguettes en boîtes | 3 modules × 6 travées : vitrines et portes vitrées d'aluminium, verrière fixe à petits bois d'acier |
| Corniche | Location de scooters (`plage.cpp`, `scooters`) | vitrine peinte | 2 vitrines et porte vitrée d'aluminium à soubassement |
| Corniche | Bars de plage (paillotes) | bar ouvert, sans mur ni baie (comptoir à l'air libre, WFS-371) | inchangé : aucune porte ni fenêtre dans la réalité |
| Corniche | Villas (`plage.cpp`, `O_VILLA`) | kit (WFS-402) ; boîte seulement si le kit est coupé (`WFS_ARCHI=0`, secours) | inchangé (kit, hors périmètre) |
| Ville | Casino (`ville.cpp`, `CASINO`) | un bloc de verre de 40 × 24 × 14 m derrière les pilastres | façades avant et latérales percées : 30 baies à l'avant (dont 2 portes vitrées sous la marquise), 30 sur les côtés ; soubassement coupé devant les portes |
| Ville | Centre commercial (`ville.cpp`, `CENTRE_COMMERCIAL`) | un bloc de verre de 80 × 26 × 9 m, baguettes en boîtes | façade de 26 travées de 3 m (vitrines et portes vitrées, verrière), côtés à verrières |
| Ville | Garages (`ville.cpp`, `GARAGE`) | porte vitrée du bureau = plaque de verre cachée dans l'épaisseur du mur ; rouleau du rideau = cylindre nu | porte vitrée du bureau dans un trumeau percé ; rideau levé : coffre d'acier à pan coupé et coulisses en U |
| Ville | Commerces (`commerces.cpp`) | plaque sombre de 8 cm et plaque de verre devant l'immeuble | devanture en applique (boiserie de la couleur du commerce, 30 cm), vitrine à soubassement et porte vitrée, corniche et retours |
| Ville | Entrepôts et immeubles | kit (WFS-370, WFS-402) ; boîtes seulement si le kit est coupé (secours) | inchangé (kit) |
| Campagne | Église (`campagne.cpp`, `T_CHURCH`) | une boîte, un toit en losange, une tour en boîte et un cône | église romane modélisée (`decor_campagne_eglise.*`) : nef 9 × 17 m, abside, clocher-mur à 2 cloches, portail, 6 baies et 2 baies campanaires en plein cintre, vitraux |
| Campagne | Maisons du village (`T_VILLA`) | kit (WFS-402) | inchangé (kit) |
| Campagne | Ferme et hangar (`T_FARM`) | modèles de WFS-372 (fenêtres en retrait, porte, volets) | inchangé |
| Campagne | Commerces du village (`T_COMMERCE`) | vitrines peintes (boîtes) | **hors zone de WFS-410** (RG-07 : « campagne.cpp, église seulement ») : à reprendre par WFS-518 (village de la Campagne, en pause sur ordre de l'utilisateur) ; signalé au PO |

## Chiffre

- Bâtiments hors kit **dans la zone du ticket** sans vraies portes ni fenêtres : **8 sortes sur 8 avant** (casino, cafés, galerie, scooters
  de la Corniche ; casino, centre commercial, garages, commerces de la Ville) **+ l'église** → **0 après**.
- Reste hors zone : les commerces du village de la Campagne (WFS-518).
