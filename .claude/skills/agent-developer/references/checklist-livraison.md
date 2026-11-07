# Checklist de livraison — revue de son propre diff (agent développeur)

À parcourir avant chaque fusion (et avant chaque demande de relecture). Une case non cochée = on ne fusionne pas,
ou on écrit au Journal pourquoi elle ne s'applique pas.

## Périmètre et décisions
- [ ] Chaque critère d'acceptation et chaque RG de `regles-de-gestion/WFS-XX.md` est couvert, preuve à l'appui.
- [ ] Rien de plus que le ticket : aucun texte, écran, option ou valeur de réglage non demandés.
- [ ] Aucune décision de l'utilisateur, du PO ou de la QA contredite ; les anomalies liées (`anno.md`, rapports QA) traitées.
- [ ] Seuls les fichiers de la zone de l'épic sont modifiés ; accroches ≤ 10 lignes annoncées au Journal.
- [ ] Aucun fichier iOS touché (gel) ; aucun moteur ni bibliothèque tiers non autorisé.

## Code
- [ ] `git diff origin/main` relu ligne par ligne : pas de code mort, de trace de débogage, de `TODO` sans ticket.
- [ ] Aucune allocation, E/S ou journal verbeux ajouté dans la boucle de course.
- [ ] Threads : aucune donnée partagée en écriture entre le thread principal et `jobs()`.
- [ ] Ressources GPU : créées une fois, détruites de façon différée (`Game::retire`), libérées dans `shutdown`.
- [ ] Entrées externes (fichiers, JSON, sauvegarde, réseau) bornées et validées ; valeur par défaut si invalide, jamais de plantage.
- [ ] Aucun secret, jeton ou donnée personnelle dans le code, la CI ou les journaux.
- [ ] Sauvegarde et protocole réseau : version incrémentée et migration si le format change.
- [ ] Commentaires en français ; source citée pour chaque constante physique (réalisme).

## Preuves
- [ ] Branche synchronisée : `git fetch origin && git merge origin/main`, conflits résolus en gardant les deux côtés.
- [ ] Compilation locale propre, sans nouvel avertissement.
- [ ] `bash tools/qa/tests_cpu.sh build` vert ; nouveau test pour chaque règle calculable ; test de non-régression pour chaque bug corrigé.
- [ ] Serveur touché : `serveur/test` vert.
- [ ] Captures du banc d'essai du ticket produites et **regardées** ; aucune autre capture dégradée.
- [ ] **`bash tools/qa/captures_pr.sh WFS-XX <banc>[:LxH] ...`** lancé : au moins 3 captures avant / après (jour et nuit si le sujet se voit) dans `qa/pr/WFS-XX/`, `pr.md` collé dans la PR.
- [ ] PR ouverte, puis **agent `lead-dev` appelé** (numéro de PR, ticket) : c'est lui qui fusionne ; je ne fusionne jamais moi-même (règle de l'utilisateur, 2026-11-07).
- [ ] Écran ou parcours touché : parcours complet (accueil → … → course → retour) prouvé au Journal (`WFS_BANC=parcours`, 0 écart).
- [ ] Performance : coût mesuré et noté au Journal si le rendu ou la simulation change.

## Livraison
- [ ] Règle de fusion en vigueur appliquée (`CLAUDE.md` : confiance > 50 % → fusion directe, sinon CI verte et captures vues).
- [ ] Ligne `qa/Check.md` (ticket, PR, date, mode de fusion, points à regarder sur les captures).
- [ ] `TACHES.md` (ses lignes seulement, cases courtes ; critères et notes dans `regles-de-gestion/WFS-XX.md`) et `journal.md` (entrée datée à la seconde, `date -u`) envoyés sur `main`.
- [ ] PR fusionnée **en squash** (`merge_method: "squash"`), titre `WFS-XX : <titre> (#PR) (<agent>)` ; chaque commit, synchronisations comprises, finit par `(<agent>)` (règle de l'utilisateur du 2026-11-06).
- [ ] `track-agent.md` : sa ligne à jour (ticket, branche, dernier envoi, état), dans le même commit que `TACHES.md` (règle de l'utilisateur du 2026-11-06).
- [ ] Réponse à l'utilisateur dans `tache utilisateur.md` si sa demande est concernée.
- [ ] CI de `main` suivie après la fusion ; toute casse due au ticket corrigée en priorité.
