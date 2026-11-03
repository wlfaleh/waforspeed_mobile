# WaForSpeed Mobile — consignes pour les agents

- Lire et suivre le skill `.claude/skills/waforspeed-natif` pour tout changement de code ; `design-street-racing` pour l'interface ;
  `waforspeed-3d` est la référence de la version navigateur (dépôt `wlfaleh/waaps`).
- **Pas de moteur de jeu tiers** (Unity, Godot, Unreal…) : moteur maison C++20 + Vulkan.
- **Épics et tickets WFS-XX** dans `TACHES.md` (Jira / Scrum). **Avant de créer une branche, demander un ticket à l'agent
  `po-scrum-master`** (`.claude/agents/po-scrum-master.md`) : il attribue le ticket prêt le plus prioritaire d'une épic libre,
  l'écrit dans `TACHES.md` sur `main`. Ne jamais se réserver un ticket seul.
- **Règles de gestion** : chaque US a son fichier `regles-de-gestion/WFS-XX.md`, écrit par le PO ; le lire avant de coder.
  **Réutiliser le code et les modèles 3D existants du jeu web** (`wlfaleh/waaps`, dossier `waforspeed/` : `js/`, `models/`, `audio/`) : ne rien recréer qui existe déjà.
- **Un ticket en cours par épic, un par agent. Ne modifier que la zone de code de son épic** (tableau des épics) ;
  le noyau partagé ne change que par un ticket E0 (ou une ligne d'accroche ≤ 10 lignes annoncée au Journal, `journal.md`).
- **Une branche par ticket** : `feature/waforspeed-XX`. Même si ta session Claude t'attribue une branche `claude/…`,
  travaille sur `feature/waforspeed-XX` (aucun travail hors ticket, aucune branche `claude/…`). Synchroniser avec `main` avant chaque validation et avant chaque PR.
- **Ne jamais supprimer de branche** (locale ou distante) : c'est l'utilisateur qui gère les branches.
- **Processus de fusion (règle en vigueur)** : le développeur **ne lance plus le `qa-lead`**. Après la construction, il fait un **contrôle rapide** :
  CI verte (compilation, tests, APK) et captures du banc d'essai PC regardées sans rien de bloquant → **il fusionne tout de suite**, puis ajoute
  **obligatoirement** le ticket (WFS-XX, PR, date, points à regarder sur les captures) dans la liste de `qa/Check.md`, au moment de la fusion (un ticket fusionné absent de la liste ne sera jamais recetté). **L'utilisateur relance le `qa-lead` chaque nuit** sur `qa/Check.md` : recette par lot,
  rapports `qa/rapports/WFS-XX.md`, verdicts, anomalies créées dans `anno.md` (ANO-XX) ; bloquant → ticket de correction prioritaire. Le reste du
  processus ne change pas : le **PO** priorise les anomalies et crée les US de correction. Les paragraphes QA ci-dessous décrivent les verdicts et la gravité,
  appliqués par le `qa-lead` lors de la recette de nuit (la fusion n'attend plus le verdict).
- **QA** : après chaque construction d'un ticket, **lancer l'agent `qa-lead`** : il regarde les captures du **banc d'essai PC** et rend le verdict
  (`qa/rapports/WFS-XX.md`). **Un développeur ne fait jamais la QA de son propre ticket** et n'écrit pas le rapport QA.
  Le **simulateur Android** n'est lancé qu'à la **fin de chaque épic** (ticket « Vérification simulateur EX » créé par le PO, workflow `simulateur.yml`).
  **Gravité des anomalies, décidée par le QA** : une anomalie **bloquante** donne `REFUSÉ` : le développeur corrige jusqu'à ce
  qu'il n'y ait plus de bloquante. S'il n'y a que des anomalies **mineures**, verdict `À CORRIGER` : le QA donne la main au
  développeur **deux fois au plus** pour les corriger. Si elles restent après ces deux tours, le QA crée les anomalies au
  **tableau des anomalies** `anno.md` (ANO-XX) et accepte avec le verdict `PASSABLE AVEC ANOMALIES MINEURES` : on fusionne,
  puis le **PO** priorise ces anomalies et crée des US de correction dédiées.
  **Fusion autorisée avec `VALIDÉ` ou `PASSABLE AVEC ANOMALIES MINEURES`, jamais avec `REFUSÉ` ni `À CORRIGER`.**
  **Exception, fusion rapide sur ordre de l'utilisateur** : CI verte (compilation, tests, APK) et rien de bloquant sur les
  captures → fusion tout de suite, QA après fusion sur `main` (bloquant → ticket de correction prioritaire, mineur → `anno.md`). L'utilisateur ne teste pas lui-même.
- **Statuts de `TACHES.md` mis à jour après chaque avancement et envoyés sur `main` tout de suite** (les autres agents s'y fient).
  Toujours partir de la dernière version de `main` (`git pull`), modifier seulement ses propres lignes, puis envoyer :
  **ne jamais copier un `TACHES.md` d'une autre branche par-dessus celui de `main`** (cela efface les réservations des autres).
- **Journal** (`journal.md`, nouvelles entrées à la fin, mêmes règles que `TACHES.md` : partir de `main`, n'ajouter que ses entrées) à jour à chaque étape. Tout en français (code commenté, messages, interface, commits).
