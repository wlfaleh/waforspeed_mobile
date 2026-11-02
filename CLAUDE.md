# WaForSpeed Mobile — consignes pour les agents

- Lire et suivre le skill `.claude/skills/waforspeed-natif` pour tout changement de code ; `design-street-racing` pour l'interface ;
  `waforspeed-3d` est la référence de la version navigateur (dépôt `wlfaleh/waaps`).
- **Pas de moteur de jeu tiers** (Unity, Godot, Unreal…) : moteur maison C++20 + Vulkan.
- **Tickets WFS-XX** dans `TACHES.md` (organisation type Jira). Avant de coder : `git pull` de `main`, réserver le ticket
  (statut 🔧, responsable, branche, entrée « Début » au Journal) et l'envoyer sur `main` immédiatement.
- **Une branche par ticket** : `feature/waforspeed-XX`. Synchroniser avec `main` avant chaque validation et avant chaque PR.
- **Ne jamais supprimer de branche** (locale ou distante) : c'est l'utilisateur qui gère les branches.
- **QA** : après chaque APK d'un ticket, lancer l'agent `qa-lead` (`.claude/agents/qa-lead.md`, skill `qa-lead`) : il teste sur émulateur,
  écrit `qa/rapports/WFS-XX.md` et rend un verdict. **Ne fusionner qu'avec le verdict `VALIDÉ`.** L'utilisateur ne teste pas lui-même.
- Journal de `TACHES.md` à jour à chaque étape. Tout en français (code commenté, messages, interface, commits).
