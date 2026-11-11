Agent : agent expert ci cd (session 01UY1Vv2Hq)

**Ticket** : WFS-427 — [TECHNIQUE] Bancs du matin : le groupe `ecrans-2400` dépasse 75 minutes, le scinder en deux (ANO-244). Règles : `regles-de-gestion/WFS-427.md`.

## Description

- **Demande de l'utilisateur** (2026-11-11) : « Répare les bancs du matin », rouges deux jours de suite (runs #12 et #16).
- **Cause** : le job `banc (ecrans-2400)` était coupé à sa limite de 75 min pendant le banc `fluidite` (partie 417 sur 568 au run #16), et le banc `camera`, placé après, ne tournait plus. Aucun banc du groupe ne trouve d'écart : le groupe est seulement devenu trop long (≈ 97 min sur la CI).
- **Ce qui change** :
  - `tools/qa/banc_groupe.sh` : la ligne du banc `fluidite` sort du cas `ecrans-2400` et devient le cas `fluidite` (même commande, mêmes fichiers copiés, au caractère près). `ecrans-2400` garde `chargement` + `hud` + `camera` (RG-01, RG-02).
  - `.github/workflows/matin.yml` : `fluidite` ajouté à la matrice des bancs et à la liste `GROUPES` de la publication (archive `captures-fluidite.zip`) ; texte du message de recette « 17 groupes » → « 18 groupes » (RG-03, RG-05, RG-06 : le job `banc` reste un seul résultat, un groupe rouge ou absent = bancs rouges, comme avant).
  - Doc du skill `expert-ci-cd` : nombre de groupes, nouveau cas dans `references/historique.md` (RG-09).
  - **Rien n'est affaibli** (RG-04) : limite de 75 min par groupe inchangée, aucun seuil, aucun test, aucune partie, aucune résolution changés. Aucun fichier du jeu. Les lignes de l'étape des tests (WFS-412) ne sont pas touchées (RG-07).
- **Comment c'est vérifié** :
  - **Sur la vraie CI** : `matin.yml` lancé à la main sur la branche, [run #19](https://github.com/wlfaleh/waforspeed_mobile/actions/runs/43025086545) (commit adf935fa) : `banc (ecrans-2400)` **vert en 61,3 min**, `banc (fluidite)` **vert en 35,6 min**. Hors de `main`, ce run ne publie rien et ne donne aucun ✅ (RG-08).
  - **En local** (lavapipe, `bash tools/qa/banc_groupe.sh <groupe>`) : voir les journaux ci-dessous.
  - `bash -n tools/qa/banc_groupe.sh` et `python3 tools/qa/test_matin.py` : verts.
- **Limites et risques connus** :
  - `ecrans-2400` reste à 61 min sur la CI pour une limite de 75 : la marge est mince (14 min). C'est le banc `hud` qui est lent (≈ 34 min en local pour 22 parties). Signalé au PO (hors périmètre de ce ticket) ; s'il s'allonge encore, il faudra sortir `hud` ou `camera` à son tour.
  - Le run #19 est rouge sur un autre groupe, `realisme-2` : 3 erreurs du code du jeu, hors de ce ticket (WFS-377 et WFS-428). Les bancs du matin de `main` resteront donc rouges tant que ces deux tickets ne sont pas fusionnés.
  - Le critère « verts aux bancs du matin de `main` » se constate au prochain run du matin après la fusion.

## Journaux (à la place des images avant / après : découpage de la CI)

### 1. Groupe `ecrans-2400` après la correction (local)
```
ecrans-2400 code=0 duree=2348s      (39 min ; chargement, hud puis camera joués, 23 parties)
25 fichiers dans captures/ : chargement-{0,1,2,erreur,fondu}-2400.png, hud-2400.png, encoche-hud-2400.png,
  minicarte-{checkpoint,ligne}-2400.png, minicarte-{ville,plage,campagne,circuit}-{jour,nuit}-2400.png,
  camera-{poursuite,poursuite-arret,poursuite-trafic,proche,proche-arret,cockpit,cockpit-droit,capot}-2400.png
```

### 2. Groupe `fluidite` (local)
```
fluidite code=0 duree=2053s         (34 min ; 568 parties)
49 fichiers dans captures/ : fluidite.txt + 48 images fluidite-{avant,apres}-{poursuite,proche}[-20ips]-0[0-5].png
fluidite.txt, dernière ligne :
après, pire cas : à-coup WFS-247 1.76 mm (… ; < 5), écart à la référence 120 Hz 4.73 mm (… ; < 5 à ≥ 30 img/s, ≤ 10 à 10-20),
saut d'angle 0.080° (… ; ≤ 0,2), aller-retour 1.89 cm (… ; ≤ 2) ; objectifs manqués : 0
```

### 3. Fichiers produits, avant / après
| | Avant (`main`, un seul groupe) | Après (branche) |
|---|---|---|
| `ecrans-2400` | 74 fichiers prévus ; aux runs #12 et #16 : job coupé, les 8 `camera-*-2400.png` absents | 25 fichiers (chargement, hud, minicarte, camera) |
| `fluidite` | (dans `ecrans-2400`) | 49 fichiers (`fluidite.txt` + 48 images) |
| Total | 74, mêmes noms | 74, mêmes noms |

Durées sur la CI : run #5 41 min (tout le groupe) ; runs #12 et #16 coupés à 75 min ; run #19 : 61,3 min + 35,6 min, en parallèle.

## Captures : le banc `camera` rejoué (il ne tournait plus), 2400 x 1080

| `camera-poursuite-2400` | `camera-cockpit-2400` | `camera-capot-2400` |
|---|---|---|
| ![poursuite](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-427/qa/pr/WFS-427/apres-camera-poursuite-2400.jpg?raw=true) | ![cockpit](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-427/qa/pr/WFS-427/apres-camera-cockpit-2400.jpg?raw=true) | ![capot](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-427/qa/pr/WFS-427/apres-camera-capot-2400.jpg?raw=true) |
