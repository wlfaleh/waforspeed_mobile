# Son des chocs selon la matière touchée (WFS-298, RG-02 et RG-03)

Relu le 2026-11-08 par arwa-4. Seuls les **nombres** sont repris dans `src/audio/chocs_sons.*` ; aucune ligne de code du dépôt cité n'entre
dans ce dépôt. Fichiers lus en lecture seule, par clone partiel, dans un dossier temporaire ; rien n'a été exécuté (`regles-reseau.md` § 2.4 et § 3).

## 1. Facteurs de perte (RG-02)
| Dépôt | Fichier | Commit | Licence | SHA-256 du fichier lu | Ouvrage cité par le dépôt |
|---|---|---|---|---|---|
| `jmrplens/phonometry` | `src/phonometry/solids/data/bies-2017-table-c1.json` | `d1a9027f050796d89e03d21c7c848910ef22bf5e` | MIT | `dd68038e84266d272957a4bb4dfd29fb34f704b34fb9d4979febf9669266a03f` | D. A. Bies, C. H. Hansen, C. Q. Howard, *Engineering Noise Control*, 5e éd., 2017, tableau C.1 (p. 718-721) |
| `jmrplens/phonometry` | `src/phonometry/solids/data/cremer-2005-table-4-3.json` | même commit | MIT | `4533d2af9740a96e7c0d7895d29b191d07015094264133fe833a1d1372332721` | L. Cremer, M. Heckl, B. A. T. Petersson, *Structure-Borne Sound*, 3e éd., Springer, 2005, tableau 4.3 |
| `jmrplens/phonometry` | `src/phonometry/solids/data/hopkins-2007-table-a2.json` | même commit | MIT | `84dca877ee2ef73ae1e8eb2399479dddaf33301c9de737e41f584e9b0d28f141` | C. Hopkins, *Sound Insulation*, 2007, tableau A2 |

Le tableau de Cremer ne donne que le facteur de perte **interne** des métaux (aluminium 0,0001, cuivre 0,002…). Celui de Bies donne en plus le
facteur de perte **en place** (« in situ » : matière, fixations et rayonnement). C'est lui que demande la règle (« matière montée ») :

| Matière (Bies, tableau C.1) | η interne | η en place | Retenu pour | η retenu |
|---|---|---|---|---|
| Steel | 0,0001 | 0,01 à 0,02 | mât, poteau (boulonné, raide) | **0,01** |
| Steel (tôle) | 0,0001 | 0,01 à 0,02 | tôle du mobilier, carrosserie | **0,02** |
| Oak, Pine, Douglas fir… | 0,04 | 0,05 à 0,06 | tronc d'arbre, lattes de banc | **0,05** |
| Polypropylene, PVC, Polycarbonate | 0,003 | 0,1 | pare-chocs, plastiques des feux | **0,1** |
| Glass | 0,0006 | 0,02 | verre des feux (éclats) | **0,02** |
| Concrete | 0,005 | 0,05 | (panneau : non retenu, voir § 2) | — |

Chaque mode décroît en **T60 = 2,2 / (f · η)**. C'est le temps de chute de 60 dB d'un mode de fréquence f et de facteur de perte η, d'où
une amplitude en r^n avec r = 10^(−3 / (T60 · fe)).

## 2. Modes retenus par famille
Les fréquences et les amplitudes des modes sont **[à vérifier]** (aucune mesure publiée lisible). Seules les décroissances viennent de la source.

| Famille | Modes (Hz) | T60 des modes (s) | T60 retenu | Plage du PO |
|---|---|---|---|---|
| Poteau (mât d'éclairage) | 188, 281, 393 (flexion) ; 647, 1 829, 3 507 (ovalisation) | 1,17 · 0,78 · 0,56 · 0,34 · 0,12 · 0,063 | **1,17 s** | 0,8 à 1,5 s |
| Arbre (tronc) + feuillage | 310, 520, 830 ; feuillage : bruit de 2 à 8 kHz | 0,142 · 0,085 · 0,053 ; feuillage 0,8 [à vérifier] | **0,142 s** | tronc ≤ 0,15 s ; feuillage 0,6 à 1 s |
| Mobilier (tôle mince et bois) | tôle 480, 810, 1 350, 2 200 ; bois 300, 760 | 0,23 · 0,14 · 0,081 · 0,050 ; 0,147 · 0,058 | **0,229 s** | 0,2 à 0,5 s |
| Béton (mur, bâtiment, îlot) | 70, 120, 190 | 0,08 (masse tenue par le sol) [à vérifier] | **0,08 s** | ≤ 0,1 s, énergie sous 250 Hz |
| Voiture (tôle et plastique) | tôle 290, 470, 760, 1 200 ; plastique 380, 950 | 0,38 · 0,23 · 0,14 · 0,092 ; 0,058 · 0,023 | **0,379 s** | 0,2 à 0,4 s |
| Éclats de l'accident (voiture) | 24 grains de 2 à 8 kHz sur 0,3 s, verre et plastique | 2,2 / (f · 0,02) ou 2,2 / (f · 0,1) | — | 0,2 à 0,4 s, 2 à 8 kHz |

Origine des fréquences du poteau, pour un mât en tube d'acier de 120 mm de diamètre, 3,5 mm d'épaisseur et 6 m de haut (EN 40 [à vérifier]) :
- **flexion du mât encastré à sa base** : f_n = (β_n·L)² / (2π·L²) · √(E·I / (ρ·A)), avec √(E·I / (ρ·A)) = c · a / √2 ≈ 5 190 × 0,041 = 213 m²/s. Les rangs 5 à 7 (β·L = 14,14, 17,28 et 20,42) donnent 188, 281 et 393 Hz ;
- **ovalisation de la paroi** (modes circonférentiels n = 2, 3 et 4) : f_n = (h·c / (2π·√12·a²)) · n·(n² − 1) / √(n² + 1), soit 647, 1 829 et 3 507 Hz.

Le béton n'est pas un panneau qui vibre : une masse tenue par le sol rayonne peu et s'amortit tout de suite. Il n'a donc qu'un coup grave et
bref (valeur du PO). Le η de 0,05 d'un panneau de béton (Bies) donnerait 0,6 s à 70 Hz, ce qui est faux pour un mur.

Chaque famille a **4 variantes** : fréquences des modes ±5 % (la décroissance suit, en 1 / f), et graine différente pour le bruit du contact, le
feuillage et les éclats. Une variante n'est **jamais jouée deux fois de suite**.

## 3. Vitesse d'impact (RG-03)
- **Niveau** : amplitude proportionnelle à la vitesse d'impact (+6 dB par doublement, hypothèse du PO [à vérifier]), de 2 m/s (7 km/h, seuil du
  choc de `chocs_cartes.cpp`) à 80 km/h, plafonnée au-delà. Mesuré : +5,8 à +6,3 dB par doublement selon la famille.
- **Clarté** : la force du contact suit une demi-sinusoïde de durée t (contact de Hertz). D'après K. L. Johnson, *Contact Mechanics*, § 11.4,
  t = 2,87 · (m² / (R · E*² · v))^(1/5) ∝ v^(−1/5). Seule cette loi est reprise. La durée à 30 km/h est un choix [à vérifier] :
  - poteau et mobilier : 0,25 ms ;
  - voiture : 0,33 ms ;
  - arbre : 0,5 ms ;
  - béton : 4 ms, soit une coupure de 250 Hz.

  La coupure de l'excitation vaut 1 / t, donc v^(1/5) : pour le poteau, 3 482, 4 000 et 4 867 Hz à 15, 30 et 80 km/h.
- **Aire de l'impulsion** constante, multipliée par le gain de vitesse : les modes graves suivent +6 dB par doublement, et les aigus montent un
  peu plus vite. Pour le poteau, de 15 à 60 km/h : aigus +15,1 dB, grave +12,5 dB.

## 4. Niveau (RG-05, décision du PO)
- À 30 km/h, la crête du son de l'obstacle est à **−3 dB** de celle du coup sourd du même choc. Ce coup sourd vient de `sfx.cpp`, SfxId::Choc : bruit de 0,3 s à 200 Hz, gain de 0,4 plus l'impact. Chaque variante est normalisée à son chargement.
- À l'accident, la crête du son de l'obstacle reste de 4 à 8 dB sous celle du son d'accident actuel (règle : +3 dB au plus).
- Le volume absolu dépend du téléphone : il n'est pas mesurable au sens de `DA-realisme.md`.
