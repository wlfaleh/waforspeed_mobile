# WFS-69 — CI : fin des builds inutiles et annulés (version stable)
Épic : E7 · Priorité : P1 Urgent · Dépend de : — · Taille : ≈ 2 h

## Besoin
Les agents attendent trop : chaque commit sur `main` (même `TACHES.md`, `journal.md`, règles) lance un build complet de ~30 min qui annule celui d'un autre ; chaque push de branche avec PR lance deux builds identiques. L'utilisateur veut une version stable demain.

## Règles de gestion
- RG-01 : `build.yml` ne se lance pas quand les fichiers modifiés sont seulement de la documentation : `TACHES.md`, `journal.md`, `anno.md`, `qa/**`, `regles-de-gestion/**`, `*.md`, `.claude/**`.
- RG-02 : un seul build par commit de branche : pas de doublon push + pull_request (garder le push sur `feature/**`, la PR ne relance pas le même commit).
- RG-03 : sur `main`, ne plus annuler un build en cours par le suivant (`cancel-in-progress` seulement hors `main`), pour que la Release `apercu` (APK stable) soit toujours publiée.
- RG-04 : aucune étape n'est supprimée : tests, captures et APK restent identiques.
- RG-05 : vérifié : un commit « documentation seule » ne lance rien ; un commit de code lance un seul build ; deux commits de code de suite sur `main` publient tous les deux.

## À réutiliser
Aucun équivalent dans le jeu web.

## Critères d'acceptation
- [ ] 3 essais (doc seule, code sur branche avec PR, 2 commits de code sur `main`) documentés au journal avec les runs

## Hors périmètre
Accélérer le build lui-même (cache Gradle / NDK) : à voir avec les retours des agents.
