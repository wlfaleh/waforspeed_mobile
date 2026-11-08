Agent : agent arwa (Claude, session trafic)

**Ticket** : WFS-266 — règles : `regles-de-gestion/WFS-266.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : les 12 conducteurs réalistes doivent avoir des cheveux et des yeux, sans taches grises autour des yeux et de la bouche (ANO-134, bloquante en recette de WFS-210).
- **Cause** : les `.wfp` des 12 adultes ne contenaient que le groupe « body » de MakeHuman : aucun globe oculaire (les orbites ouvertes laissaient voir l'intérieur de la tête : taches gris clair), aucun sommet en partie 1 (yeux) ou 2 (cheveux), aucune plage `Hair_*`.
- **Ce qui change** :
  - `tools/convert_people_makehuman.py` : globes oculaires MakeHuman (`helper-l-eye` / `helper-r-eye`, CC0) en partie 1, centre du globe dans `_cloth.xyz` ; cuir chevelu peint en partie 2 avec `cuir_chevelu()` et `VOLUME_CHEVEUX` (4 mm) réutilisés ; f01, f02, f03, f05 (4 femmes sur 6) : coiffure `Hair_Long` de `femme.wfp` (Quaternius CC0, comme WFS-240) posée sur la tête (boîte de la tête, mèches à 3 mm au moins de la peau), ajoutée aux plages `corps-0/1/2` (Hair_Long-1 de près, Hair_Long-2 au loin) : **aucun appel de dessin en plus** ; dents et langue non reprises (RG-03).
  - `assets/people/humains/h01..h06, f01..f06.wfp` régénérés (mêmes sources CC0, SHA-256 au journal) ; **8 enfants inchangés** (aucune différence de fichier).
  - `shaders/scene_skin.frag` (réglage, bloc WFS-72) : œil dessiné au pixel (iris 11,7 mm, pupille 3,5 mm, limbe, sclère en codes sRGB 0,76 / 0,72 / 0,68, jamais blanc pur ; cornée GGX F0 0,025 déjà là) ; cheveux des humains adultes à la couleur de la palette ; ligne des cheveux jamais classée « œil » (plus de liseré clair au front). Piétons Quaternius et enfants : branches inchangées (`_cloth.x = −1`).
  - `src/world/conducteurs.cpp` : **accroche de 2 lignes** (annoncée au journal) : la stature d'un humain v3 est lue dans l'en-tête du `.wfp` (le crâne est maintenant en partie 2 : le sommet « partie 0 » ne donne plus la stature).
  - `tests/fidelite_humains.cpp` : mèches et yeux hors des mesures du corps, cuir chevelu −4 mm (comme les enfants) ; nouveaux cas : 2 yeux par adulte (écart des pupilles 57 à 63 mm, réel 55 à 70), cheveux peints au-dessus du cou, au moins 3 femmes sur 6 aux cheveux longs (4).
  - `references/reel/humains/oeil.md` (sources de l'œil), `assets/licences/humains-makehuman.txt`.
- **Comment c'est vérifié** (résultats) :
  - tests CPU `bash tools/qa/tests_cpu.sh build` : **64 réussis** (branche synchronisée avec `main`) ; `test_fidelite_humains` : **indice 100 % (72 / 72)**, tous les cas OK (yeux, cheveux, niveaux de détail : écart de forme corps-1 ≤ 3,9 mm, corps-2 ≤ 11,3 mm) ;
  - **parcours complet 0 écart** (et 0 écart en vue Cockpit) ;
  - **banc perf Ville** (avant / après, même binaire, assets seuls changés) : appels **930 → 930** (Haute), **303 → 303** (Basse) ; triangles de l'image 1 734 448 → 1 738 126 (**+0,21 %**, Haute), 549 493 → 550 703 (**+0,22 %**, Basse) ; « Pietons » inchangé (338 687 / 249 274) : tout l'écart vient des conducteurs, **< 5 %** (≈ +1,1 % des triangles des personnes) ;
  - triangles par conducteur (corps-0) : 26 756 → 27 036 (+280 pour les yeux) ; 28 140 pour une femme aux cheveux longs (+1 104 de plus).
- **Limites et risques connus** : la ligne des cheveux courts suit les arêtes du maillage (dentelée au gros plan, comme les enfants de WFS-233) ; albédo de la sclère et teinte de l'iris « à confirmer » (aucune mesure spectrale accessible : réseau limité à GitHub) ; coiffure longue rigide sur la tête et le cou (comme WFS-240) ; derrière la vitre, à 6,5 m, les yeux ne se distinguent pas (gros plan de face : planche des 12 humains) ; temps d'image du banc non comparables (lavapipe, autres bancs en parallèle) : seuls appels et triangles comptent.

## Gros plans (WFS_TAILLE=3840x2160, recadrés)

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `gros-plan-tetes-jour` (f01 et h01, de face) | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/avant-gros-plan-tetes-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/apres-gros-plan-tetes-jour.jpg?raw=true) |
| `gros-plan-conducteur-golf-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/avant-gros-plan-conducteur-golf-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/apres-gros-plan-conducteur-golf-jour.jpg?raw=true) |
| `gros-plan-conducteur-golf-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/avant-gros-plan-conducteur-golf-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/apres-gros-plan-conducteur-golf-nuit.jpg?raw=true) |

## Captures pour le lead dev (10 après)

Dossier : [`qa/pr/WFS-266`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `conducteurs-golf-arriere-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/avant-conducteurs-golf-arriere-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/apres-conducteurs-golf-arriere-jour.jpg?raw=true) |
| `conducteurs-golf-arriere-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/avant-conducteurs-golf-arriere-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/apres-conducteurs-golf-arriere-nuit.jpg?raw=true) |
| `conducteurs-golf-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/avant-conducteurs-golf-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/apres-conducteurs-golf-jour.jpg?raw=true) |
| `conducteurs-golf-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/avant-conducteurs-golf-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/apres-conducteurs-golf-nuit.jpg?raw=true) |
| `conducteurs-rrsport-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/avant-conducteurs-rrsport-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/apres-conducteurs-rrsport-jour.jpg?raw=true) |
| `conducteurs-rrsport-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/avant-conducteurs-rrsport-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/apres-conducteurs-rrsport-nuit.jpg?raw=true) |
| `conducteurs-sf90-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/avant-conducteurs-sf90-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/apres-conducteurs-sf90-jour.jpg?raw=true) |
| `conducteurs-sf90-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/avant-conducteurs-sf90-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/apres-conducteurs-sf90-nuit.jpg?raw=true) |
| `realisme-pietons-humains-assis` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/avant-realisme-pietons-humains-assis.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/apres-realisme-pietons-humains-assis.jpg?raw=true) |
| `realisme-pietons-humains-debout` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/avant-realisme-pietons-humains-debout.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-266/qa/pr/WFS-266/apres-realisme-pietons-humains-debout.jpg?raw=true) |
