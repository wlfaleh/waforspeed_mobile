# Peintures automobiles — valeurs du réel pour les 8 finitions (WFS-490)

Domaine E10 Véhicules. Valeurs utilisées par `shaders/vehicule.glsl` (constantes `FIN_*`, fonctions `finitionSurface` et
`vernisFinition`) et contrôlées par `tests/fidelite_voitures_finitions.cpp`. Charte commune : `regles-de-gestion/DA-realisme.md` § 3
(vernis 0,02-0,08, chrome 0,05-0,15 en métal 1, F0 des diélectriques 0,02-0,05).

Rugosité = paramètre « roughness » perceptuel (alpha GGX = rugosité²), comme le reste du moteur.

## Structure d'une peinture de série (toutes les finitions sauf chromé)
Apprêt, **base** colorée (15-25 µm) puis **vernis** incolore 2K (35-50 µm), indice ≈ 1,5 → F0 ≈ 0,04 (Fresnel, n = 1,5).
Sources : norme ISO 2813 / ASTM D523 (brillant spéculaire, unités de brillant GU) ; documentation technique des fabricants de
peinture automobile (fiches « basecoat / clearcoat », PPG, Axalta, BASF Glasurit) ; Kirchner E. et al., « Visibility of sparkle
in metallic paints », JOSA A 32 (2015), et Kirchner E., Cramer W., « Making sense of measurement geometries for multi-angle
spectrophotometers », Color Res. Appl. 37 (2012) (flop, géométries 15° / 45° / 110°).

| Finition | Brillant mesuré (réel) | Modèle du jeu | Constante |
|---|---|---|---|
| Brillant | ≥ 90 GU à 20° (vernis 2K tendu) | vernis rugosité 0,03 (alpha 0,0009), aspect d'avant inchangé (RG-01) | `FIN_VERNIS_BRILLANT` |
| Satiné | 30 à 60 GU à 60° | base rugosité 0,45, vernis rugosité 0,18, force du vernis 0,85 : reflet du ciel flou, soleil large | `FIN_SATINE_RUGOSITE`, `FIN_VERNIS_SATINE` |
| Mat | < 10 GU à 60° (vernis mat, silice) | pas de vernis net, rugosité 0,62 : reflet du soleil large et faible ; couleur gardée (aucun blanchiment) | `FIN_MAT_RUGOSITE` |

## Métallisé (paillettes d'aluminium)
- Paillettes d'aluminium de 10 à 40 µm, 15 à 20 % du film de base, orientées presque à plat (écart type de l'orientation
  ≈ 5 à 10°) ; réflectance de l'aluminium ≈ 0,91 dans le visible (Rakić A. D., « Algorithm for the determination of intrinsic
  optical constants of metal films: application to aluminum », Appl. Opt. 34 (1995) ; données sur refractiveindex.info).
- **Flop** : clair près du spéculaire (15°), sombre loin du spéculaire (110°) ; rapport de luminance typique 3 à 6 entre 15° et 110°
  (indice de flop DuPont / X-Rite, 10 à 17 pour un argent, 3 à 8 pour une couleur).
- Jeu : flop × 0,15 (rasant) à × 1,6 (de face), part métal 0,65, rugosité de base 0,35 ; paillettes visibles seulement de près
  (cellule de 0,4 mm au plus près, une sur 6 porte un éclat, orientation ± 8°, lobe alpha 0,05) : points plus fins qu'un pixel
  autour du reflet du soleil, fondus dans la couleur moyenne au loin et en qualité Basse (RG-02, RG-10).

## Nacré (mica enrobé d'oxyde de titane)
- Pigments à effet perlé : plaquettes de mica (5 à 25 µm) enrobées de TiO₂ ; couleur d'interférence selon l'épaisseur de TiO₂
  (Merck, gamme Iriodin, documentation technique ; Pfaff G., « Special Effect Pigments », Vincentz 2008).
- Effet : lustre laiteux et reflet teinté (doré / rosé pour un blanc nacré) qui change doucement avec l'angle, moins de contraste
  que le métallisé.
- Jeu : part métal 0,25, rugosité 0,30 ; reflet légèrement froid de face, doré-rosé en rasant (+ reflet doré d'interférence) ; lustre laiteux 0,03 sur une couleur saturée, jusqu'à 0,075 sur les blancs et gris ; paillettes 0,3 × celles du métallisé.

## Chromé (film chromé, chrome teinté)
- Chrome métal : réflectance 0,55 à 0,65 dans le visible (constantes n et k du chrome, Rakić A. D. et al., Appl. Opt. 37 (1998),
  données sur refractiveindex.info) ; jamais 1 (miroir parfait) ni blanc uniforme ; métal pur : **aucune part diffuse**.
- Jeu : métal 1, F0 = 0,60 × (teinte de la couleur à 15 % au plus), rugosité 0,06 (charte : 0,05-0,15), pas de vernis ; noir →
  chrome fumé (F0 ≈ 0,30). Reflet : au-dessus de l'horizon, la sonde d'environnement existante à pleine force (RG-06) ; en dessous,
  la chaussée réelle (asphalte ρ = 0,14, DA-realisme § 3, éclairé par le soleil et le ciel comme le sol de `scene.frag`) ; horizon
  net (R.y de 0,02 à −0,05) ; aucune lumière diffuse la nuit. Arbitrage du PO (2026-11-13, `regles-de-gestion/WFS-490.md`) :
  le décor dans le reflet et un ciel plus clair que l'asphalte au soleil viendront de WFS-539 (la sonde actuelle ne contient que
  le ciel : `qa/pr/WFS-490/preuve-sonde-brute-gt86.jpg`).

## Caméléon (pigments à interférence multicouche)
- Pigments à interférence multicouche (ex. ChromaFlair de Viavi, Colorstream de Merck) : la couleur renforcée se décale vers les
  courtes longueurs d'onde quand l'angle augmente (couche mince : λ ∝ cos θ_réfracté) ; 2 à 4 teintes vues sur la même caisse.
- Jeu : teinte tournée en continu de 0 (de face) à +1,5 rad (rasant, proportionnel à 1 − cos θ) en TSV sur la couleur affichée,
  saturation au moins 0,85 et clarté gardées (couleurs vives, jamais olive) : ex. rouge → orange → or ; couleur sans teinte (gris,
  blanc, noir) : base violette. Angle pris sur la normale lissée du maillage. Part métal 0,75, rugosité 0,20, vernis brillant (RG-07).

## Candy (vernis teinté sur base argent)
- Base argent métallisée, puis vernis teinté transparent, puis vernis incolore : le chemin de la lumière dans le vernis teinté
  s'allonge en biais (loi de Beer-Lambert) → couleur plus saturée et plus sombre de biais, lumineuse de face.
- Jeu : teinte normée (canal le plus fort = 1) élevée à la puissance 1,0 (de face) à 2,8 (rasant), × 1 à 0,10 ; base argent
  (métal 0,85, rugosité 0,20) ; vernis brillant par-dessus (RG-08).

## Nuit (RG-09)
Toutes les finitions reçoivent les lumières de nuit existantes (phares, lampadaires, rebond de la chaussée de WFS-270, dans
`vernisCouche`) ; les reflets restent bornés par `SPEC_MAX` (paillettes : `SPEC_MAX` / 2) : aucune tache blanche.

Limite : valeurs choisies dans les plages publiées ci-dessus, sans mesure goniophotométrique d'une teinte précise ; les sources
sont citées d'après leurs références publiques (pas de téléchargement pour ce ticket).
