# CNOSSOS-EU — tableau F-1, catégorie 1 (voitures) : coefficients du bruit de roulement et de propulsion

WFS-275 (RG-01), relu le 2026-11-07 par arwa-4. Seuls les **nombres** de la directive sont repris ici et dans `src/audio/voitures_sons.h` ;
aucune ligne de code des dépôts cités n'entre dans ce dépôt. Fichiers lus en lecture seule dans un dossier temporaire, rien exécuté
(`regles-reseau.md` § 2.4 et § 3).

## Texte en vigueur
Directive 2002/49/CE, annexe II, dans sa version consolidée :
- directive (UE) **2015/996** de la Commission : formules 2.2.1 à 2.2.20 et tableaux F-1 à F-4 ;
- directive déléguée (UE) **2021/1226** de la Commission : **remplace le tableau F-1** (annexe, point 19 a) et la pondération A de 2.5.5 (point 8 b).

Formules (catégorie m = 1, bande d'octave i, vitesse v en km/h, v_ref = 70 km/h) :
- roulement : L_WR = A_R + B_R · lg(v / v_ref) ;
- propulsion : L_WP = A_P + B_P · (v − v_ref) / v_ref.

## Valeurs (dB, bandes de 63 Hz à 8 kHz)

| | 63 | 125 | 250 | 500 | 1 k | 2 k | 4 k | 8 k |
|---|---|---|---|---|---|---|---|---|
| A_R | 83,1 | 89,2 | 87,7 | 93,1 | 100,1 | 96,7 | 86,8 | 76,2 |
| B_R | 30,0 | 41,5 | 38,9 | 25,7 | 32,5 | 37,2 | 39,0 | 40,0 |
| A_P | 97,9 | 92,5 | 90,7 | 87,2 | 84,7 | 88,0 | 84,4 | 77,1 |
| B_P | −1,3 | 7,2 | 7,7 | 8,0 | 8,0 | 8,0 | 8,0 | 8,0 |
| pondération A (2.5.5) | −26,2 | −16,1 | −8,6 | −3,2 | 0,0 | 1,2 | 1,0 | −1,1 |

Tableau F-3 (inchangé depuis 2015/996), catégorie 1 : C_R = −4,5 et **C_P = 5,5** dB par m/s² près d'un carrefour à feux
(−4,4 et 3,1 près d'un giratoire). Il sert à justifier le « moteur en charge » (+6 dB) des adversaires et de la police.

## Sources (deux dépôts publics, concordants)
| Dépôt | Fichier | Commit | Licence du dépôt | SHA-256 du fichier lu |
|---|---|---|---|---|
| `jmrplens/phonometry` | `src/phonometry/environment/sources/cnossos_road.py` (`_TABLE_F1`, « as replaced by Commission Delegated Directive (EU) 2021/1226 Annex point (19)(a) ») | `4e0b6b7c2d18c423c0a3ac193a34b891115fda47` | MIT | `5d7712cfa64287639ebfc49474bebe2900c6686acb8f53358363cbe2304dc3e3` |
| `Arpapiemonte/openoise-map` (agence régionale de l'environnement du Piémont) | `tools/on_Acoustics_CNOSSOS_Road_Params.xml` (catégorie 1 : `Ar`, `Br`, `Ap`, `Bp`) | `f40f9472329f1ac1be57decb2e4d78390606d2bc` | GPL-3.0 | `735ef2dc4ca2367bb3d07ce684317112cdf985c4cee639b157e29b754c2f821e` |

Les deux fichiers donnent les mêmes A_R, B_R, A_P et B_P à la décimale près.

## Comparaison avec WFS-272 (`voitures_sons.h`, avant WFS-275)
| | WFS-272 (écrit de mémoire) | texte en vigueur | écart |
|---|---|---|---|
| A_R | 79,7 · 85,7 · 84,5 · 90,2 · 97,3 · 93,9 · 84,1 · 74,3 | 83,1 · 89,2 · 87,7 · 93,1 · 100,1 · 96,7 · 86,8 · 76,2 | +1,9 à +3,5 dB |
| B_R | identique | identique | 0 |
| A_P | 94,5 · 89,2 · 88,0 · 85,9 · 84,2 · 86,9 · 83,3 · 76,1 | 97,9 · 92,5 · 90,7 · 87,2 · 84,7 · 88,0 · 84,4 · 77,1 | +0,5 à +3,4 dB |
| B_P | identique | identique | 0 |
| pondération A | identique | identique | 0 |

WFS-272 avait des A_R et A_P anciens : ce sont, d'après la documentation du premier dépôt, ceux de 2015/996, remplacés en 2021. Comme le jeu
utilise des niveaux **relatifs** (référence : voiture en croisière à 50 km/h), seul l'équilibre entre pneus et moteur change. À 60 km/h, les pneus
passent de +6,8 dB à +8,5 dB au-dessus du moteur ; à 90 km/h, de +9,4 dB à +11,1 dB. La pente du roulement reste de 34 dB par décade, et la voiture
du trafic à 50 km/h à 7,5 m sonne −9,0 dB sous le moteur du joueur à fond (−8,9 dB avant). `test_audio_voitures` reste vert.
