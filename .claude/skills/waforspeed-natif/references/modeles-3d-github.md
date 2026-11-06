# Trouver un vrai modèle 3D (voitures, humains, animaux, bâtiments) sans intervention de l'utilisateur (méthode du jeu web)

Méthode reprise du jeu web (`wlfaleh/waaps`, `.claude/skills/waforspeed-3d/references/modeles-voitures.md`), qui a trouvé ainsi
ses 20 voitures (`waforspeed/models/README.md` : « tous trouvés dans des dépôts GitHub publics, avec leur licence d'origine (Sketchfab) »).
**Aucun site à ouvrir, aucun compte** : la recherche de code GitHub et la lecture des dépôts publics par `git` passent déjà par le réseau
des agents. Règles de `regles-reseau.md` : en vigueur (licence, dossier à part, SHA-256, journal, rien d'exécuté).

## 1. Chercher
Les modèles Sketchfab téléchargés sont souvent rangés dans des dépôts GitHub publics avec le `license.txt` écrit par Sketchfab.
```bash
gh api -X GET search/code -f q='"Tesla" "Model 3" filename:license.txt' --jq '.items[] | "\(.repository.full_name)  \(.path)"'
```
- par titre dans la licence : `"BMW X5" filename:license.txt` ; par marque : `"Volkswagen" sketchfab filename:license.txt` ;
- par dossier : `tesla_model_3`, `bmw_x5` (minuscules et `_`) ; par fichiers de crédits : `CREDITS.md`, `ATTRIBUTION.md` ;
- le même modèle apparaît souvent dans plusieurs dépôts : comparer le `source:` du `license.txt` (lien Sketchfab) pour savoir lequel.
- Exemple (2026-11-06) : 27 dépôts pour « Tesla Model 3 », dont plusieurs `tesla_2018_model_3`.

## 2. Récupérer seulement le dossier utile
```bash
GIT_LFS_SKIP_SMUDGE=1 git clone -q --depth 1 --filter=blob:none --sparse https://github.com/<owner>/<repo> <bac-à-sable>/<owner>_<repo>
cd <bac-à-sable>/<owner>_<repo>
git ls-tree -r --name-only HEAD | grep -i <voiture>     # lister sans télécharger
git sparse-checkout set --no-cone <dossier/du/modele>    # télécharger seulement ce dossier
```
Dans le bac à sable de la session, **jamais** dans le dépôt. Ne jamais faire `git ls-tree -l` sur un clone `blob:none` (tout serait
téléchargé). Fichiers LFS : arrivent en « pointeurs » (non servis) → prendre un autre dépôt.

## 3. Vérifier avant de garder
| Critère | Accepté pour le jeu natif |
|---|---|
| Licence (lue dans `license.txt`, recopiée) | **CC0, CC BY** ; CC BY-SA avec accord du PO ; **refus** : NC, ND, « Standard / Editorial », pas de licence (`regles-reseau.md` § 4, plus strict que le jeu web) |
| Contenu | carrosserie complète, 4 roues, version de série (pas widebody, course ou accidentée) |
| Taille | conforme aux budgets de `regles-reseau.md` § 5 (après conversion `.wfm` ≤ 8 Mo) |
| Qualité | carrosserie sans facettes (rendu `vitrine-<id>-0` comparé à l'avant), dimensions ±1 % (`test_fidelite_voitures`) |
Si le modèle exact n'existe pas : le plus proche (même voiture, autre année) et **le dire** au journal.

## 4. Convertir et tracer
- Conversion par `tools/convert_car.mjs` (ou `tools/convertir_voitures.sh`), comme les 20 voitures actuelles.
- `assets/licences/<id>.txt` : texte d'origine + dépôt GitHub où il a été trouvé + ligne « Modifications for WaForSpeed ».
- Journal : dépôt, chemin, lien Sketchfab, licence, auteur, SHA-256 du fichier source, taille avant / après (`regles-reseau.md` § 6).

## 5. Même méthode pour les humains, les animaux, les bâtiments, la végétation (2026-11-06, demande de l'utilisateur)
Recherche de code GitHub, puis étapes 2 à 4 (dossier à part, licence lue, conversion par les outils du dépôt, journal). Essais du 2026-11-06 :

| Recherche (`gh api -X GET search/code -f q='…'`) | Résultats |
|---|---|
| `"cow" sketchfab filename:license.txt` / `"horse" …` / `"sheep" …` / `"pigeon" …` | 118 / 68 / 41 / 31 |
| `"character" "CC-BY-4.0" sketchfab filename:license.txt` ; `"man" "rigged" sketchfab filename:license.txt` | 151 ; 34 |
| `"building" sketchfab filename:license.txt` ; `"apartment" sketchfab filename:license.txt` | 364 ; 68 |
| `quaternius CC0 filename:License.txt` (paquets CC0 de Quaternius : personnages, animations, animaux) | 2 424 |
| `kenney.nl "CC0" filename:License.txt` ; `kenney "City Kit"` (paquets CC0 de Kenney : villes, bâtiments, mobilier) | 20 448 ; 3 576 |

- Les piétons actuels du jeu viennent déjà de là : « Universal Base Characters » et « Universal Animation Library » de Quaternius, CC0
  (`wlfaleh/waaps`, `waforspeed/models/people/license.txt`).
- **Réalisme** : les paquets Quaternius et Kenney sont **stylisés (low-poly)** ; l'axe Réalisme (`regles-de-gestion/DA-realisme.md`, fidélité
  99 %) demande des modèles réalistes : préférer les modèles Sketchfab réalistes (CC0 / CC BY) trouvés par `license.txt`, et faire valider le
  choix par `expert-realisme-3d` (consultant) avant la conversion. Un modèle stylisé n'est permis que comme remplaçant provisoire, noté au journal.
- **Animaux et humains animés** : vérifier qu'il y a un squelette et des animations (marche, repos) ou prévoir celles de Quaternius (CC0) ;
  sinon le noter.
- **Bâtiments** : vérifier l'échelle (1 unité = 1 m), le nombre de triangles et la taille des textures (budget du téléphone, `regles-reseau.md` § 5).
