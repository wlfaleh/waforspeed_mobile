Agent : Claude, agent dev3d-cori (01UY1Vv4T3)

**Ticket** : WFS-533 — [TECHNIQUE] Correction CI main : crédit manquant de `profil/mots_interdits.txt` — règles : `regles-de-gestion/WFS-533.md`

## Description

- **Demande** (en une phrase) : remettre au vert la CI de `main` (build.yml #43392460299, job « banc-essai (texte) » ; bancs du matin #43396191571, job « tests CPU et licences »), rouge depuis #375 (WFS-480) parce que le contrôle des licences ne connaît pas `assets/profil/mots_interdits.txt`.
- **Ce qui change** : `tools/qa/verifier_licences.py`, **une ligne** ajoutée à `COUVERTURE` : `(r"profil/mots_interdits\.txt", None, None)`. Elle déclare ce seul fichier comme « créé par le projet », comme `textes/.*`, `world/feuillage\.png` ou `textures/essai/.*`. La liste a été écrite pour le jeu (`qa/pr/WFS-480/pr.md`). Il n'y a pas de joker sur `profil/*` et aucun contrôle n'est affaibli (RG-02). Rien d'autre ne change : jeu, tests, CI et liste des mots sont inchangés.
- **Écart à RG-01, signalé au PO** : je n'ai pas ajouté de ligne dans `AUTRES`. Cette table ne sert qu'aux œuvres **tierces** : chaque entrée demande une licence connue de `LIEN` et devient une ligne de `CREDITS.txt`. Aucun actif écrit pour le jeu n'y figure. Le message du script (« AUTRES et COUVERTURE ») vise les actifs tiers. Les actifs écrits pour le jeu sont couverts par la phrase « Créés par le jeu » de `CREDITS.txt`. `CREDITS.txt` et `CREDITS.md` sont donc inchangés : `--generer` ne produit aucune différence.
- **RG-03** : j'ai lancé le script sur `main` à jour (4e01a6da). Sur 143 fichiers d'assets vérifiés, `profil/mots_interdits.txt` est **le seul** manque. Aucun autre actif ajouté depuis #375 n'est dans ce cas.

## Comment c'est vérifié (sujet sans image : sorties à la place des captures)

Contrôle des licences **avant** (script de `main`, 4e01a6da) :
```
licences : 17 voitures livrées (3 retirées : a45, elantra, x5), 143 fichiers d'assets vérifiés, 1 manque(s)
  MANQUE profil/mots_interdits.txt : actif livré sans crédit connu (ajouter sa ligne dans AUTRES et COUVERTURE)
code=1
```

Contrôle des licences **après** (branche) :
```
licences : 17 voitures livrées (3 retirées : a45, elantra, x5), 143 fichiers d'assets vérifiés, 0 manque(s)
code=0
```

- **Tests CPU** : je ne les ai pas compilés dans le conteneur, faute de `glslc`, dont CMake a besoin pour se configurer. La modification ne touche aucun code compilé. Les tests CPU et le contrôle des licences sont vérifiés par le run de branche (US technique) : **build.yml #43416967480** — https://github.com/wlfaleh/waforspeed_mobile/actions/runs/43416967480 (commit cf566eb8, branche synchronisée avec `main`).
- Résultat du run : RESULTAT_RUN

## Limites et risques

- Aucun risque pour le jeu : seul le script de contrôle change.
- Un futur fichier ajouté sous `assets/profil/` sera de nouveau signalé « MANQUE ». C'est voulu (RG-02 : pas de joker).
