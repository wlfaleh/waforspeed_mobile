# WaForSpeed Mobile — consignes pour les agents

- Lire et suivre le skill `.claude/skills/waforspeed-natif` pour tout changement de code ; `design-street-racing` pour l'interface ;
  `waforspeed-3d` est la référence de la version navigateur (dépôt `wlfaleh/waaps`).
- **Pas de moteur de jeu tiers** (Unity, Godot, Unreal…) : moteur maison C++20 + Vulkan.
- **Épics et tickets WFS-XX** dans `TACHES.md` (Jira / Scrum). **Avant de créer une branche, demander un ticket à l'agent
  `po-scrum-master`** (`.claude/agents/po-scrum-master.md`) : il attribue le ticket prêt le plus prioritaire d'une épic libre,
  l'écrit dans `TACHES.md` sur `main`. Ne jamais se réserver un ticket seul.
- **Un ticket en cours par épic, un par agent. Ne modifier que la zone de code de son épic** (tableau des épics) ;
  le noyau partagé ne change que par un ticket E0 (ou une ligne d'accroche ≤ 10 lignes annoncée au Journal).
- **Une branche par ticket** : `feature/waforspeed-XX`. Même si ta session Claude t'attribue une branche `claude/…`,
  travaille sur `feature/waforspeed-XX` (aucun travail hors ticket, aucune branche `claude/…`). Synchroniser avec `main` avant chaque validation et avant chaque PR.
- **Ne jamais supprimer de branche** (locale ou distante) : c'est l'utilisateur qui gère les branches.
- **QA** : après chaque APK d'un ticket, l'agent `qa-lead` regarde les captures et rend un verdict (`qa/rapports/WFS-XX.md`).
  **Ne fusionner qu'avec le verdict `VALIDÉ`.** L'utilisateur ne teste pas lui-même.
- **Statuts de `TACHES.md` mis à jour après chaque avancement et envoyés sur `main` tout de suite** (les autres agents s'y fient).
  Toujours partir de la dernière version de `main` (`git pull`), modifier seulement ses propres lignes, puis envoyer :
  **ne jamais copier un `TACHES.md` d'une autre branche par-dessus celui de `main`** (cela efface les réservations des autres).
- Journal de `TACHES.md` à jour à chaque étape. Tout en français (code commenté, messages, interface, commits).
