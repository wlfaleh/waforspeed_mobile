# Trouver un vrai modèle de voiture sans intervention de l'utilisateur (méthode du jeu web)

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
