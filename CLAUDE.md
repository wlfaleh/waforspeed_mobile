# WaForSpeed Mobile — consignes pour les agents

- Lire et suivre le skill `.claude/skills/waforspeed-natif` pour tout changement de code ; `design-street-racing` pour l'interface ;
  `waforspeed-3d` est la référence de la version navigateur (dépôt `wlfaleh/waaps`).
- **Pas de moteur de jeu tiers** (Unity, Godot, Unreal…) : moteur maison C++20 + Vulkan.
- **Tickets WFS-XX** dans `TACHES.md` (organisation type Jira). Avant de coder : `git pull` de `main`, réserver le ticket
  (statut 🔧, responsable, branche, entrée « Début » au Journal) et l'envoyer sur `main` immédiatement.
- **Une branche par ticket** : `feature/waforspeed-XX`. Même si ta session Claude t'attribue une branche `claude/…`,
  travaille sur `feature/waforspeed-XX` (aucun travail hors ticket, aucune branche `claude/…`). Synchroniser avec `main` avant chaque validation et avant chaque PR.
- **Ne jamais supprimer de branche** (locale ou distante) : c'est l'utilisateur qui gère les branches.
- **QA** : après chaque APK d'un ticket, l'agent `qa-lead` regarde les captures et rend un verdict (`qa/rapports/WFS-XX.md`).
  **Ne fusionner qu'avec le verdict `VALIDÉ`.** L'utilisateur ne teste pas lui-même.
- **Statuts de `TACHES.md` mis à jour après chaque avancement et envoyés sur `main` tout de suite** (les autres agents s'y fient).
  Toujours partir de la dernière version de `main` (`git pull`), modifier seulement ses propres lignes, puis envoyer :
  **ne jamais copier un `TACHES.md` d'une autre branche par-dessus celui de `main`** (cela efface les réservations des autres).
- Journal de `TACHES.md` à jour à chaque étape. Tout en français (code commenté, messages, interface, commits).
