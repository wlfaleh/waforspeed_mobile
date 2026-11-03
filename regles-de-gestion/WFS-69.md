# WFS-69 — CI : fin des builds inutiles et annulés (version stable)
Épic : E7 · Priorité : P1 Urgent · Dépend de : — · Taille : 1 jour (gros ticket, retours des agents 3, 4 et 5)

## Besoin
Les agents attendent trop : chaque commit sur `main` (même `TACHES.md`, `journal.md`, règles) lance un build complet de ~30 min qui annule celui d'un autre ; chaque push de branche avec PR lance deux builds identiques. L'utilisateur veut une version stable demain.

## Règles de gestion
- RG-01 : `build.yml` ne se lance pas quand les fichiers modifiés sont seulement de la documentation : `TACHES.md`, `journal.md`, `anno.md`, `qa/**`, `regles-de-gestion/**`, `*.md`, `.claude/**`.
- RG-02 : un seul build par commit de branche : pas de doublon push + pull_request (garder le push sur `feature/**`, la PR ne relance pas le même commit).
- RG-03 : sur `main`, ne plus annuler un build en cours par le suivant (`cancel-in-progress` seulement hors `main`), pour que la Release `apercu` (APK stable) soit toujours publiée.
- RG-04 : aucune étape n'est supprimée : tests, captures et APK restent identiques.
- RG-05 : vérifié : un commit « documentation seule » ne lance rien ; un commit de code lance un seul build ; deux commits de code de suite sur `main` publient tous les deux.

- RG-06 (retours des 3 agents) : **le temps vient du banc d'essai** (étape « Captures du rendu » : 37 à 52 min, sur lavapipe, en série ; Android 1,7 min, publication < 1 min, compilation + tests < 1 min). Découper le banc en **jobs parallèles (matrice) par groupe de scènes** : vitrines voitures / course + trafic + HUD + fin / menus + chargement / texte + encoche (+ son). Cible : retour CI **≈ 10-15 min** au lieu de 45-52 min. `WFS_BANC=<groupe>` existe déjà ; ajouter les groupes manquants.
- RG-07 : option `WFS_CAPTURES=<groupe>` pour ne relancer que les captures du ticket en local / en CI manuelle.
- RG-08 : réduire les images rendues pour rien sous lavapipe (boucles de 240 à 480 images, mesures de coût : une mesure courte suffit) ; ne pas recharger les 5 voitures à chaque scène si évitable.
- RG-09 : une branche `feature/**` = un seul banc par commit (PR seulement, ou push seulement) et `main` = un build par commit de code sans annulation ; la Release d'aperçu de la branche (`apercu-waforspeed-XX`) doit rester publiée.
- RG-10 : les 4 groupes de captures publient toutes leurs captures dans la même Release (aucune capture perdue, `0 écart(s)` identique à avant).

## À réutiliser
Aucun équivalent dans le jeu web.

## Critères d'acceptation
- [ ] 3 essais (doc seule, code sur branche avec PR, 2 commits de code sur `main`) documentés au journal avec les runs

## Hors périmètre
Accélérer le build lui-même (cache Gradle / NDK) : à voir avec les retours des agents.
