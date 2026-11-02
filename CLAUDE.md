# WaForSpeed Mobile — consignes pour les agents

- Lire et suivre le skill `.claude/skills/waforspeed-natif` pour tout changement de code ; `design-street-racing` pour l'interface ;
  `waforspeed-3d` est la référence de la version navigateur (dépôt `wlfaleh/waaps`).
- **Pas de moteur de jeu tiers** (Unity, Godot, Unreal…) : moteur maison C++20 + Vulkan.
- **Avant de commencer une tâche** : `git pull` de `main`, vérifier qu'elle n'est pas réservée, la réserver dans `TACHES.md`
  (🔧 + « Qui » + entrée « Début » dans le Journal) et l'envoyer sur `main` immédiatement — pour ne pas se marcher sur les pieds.
- **Après chaque étape** : mettre à jour `TACHES.md` (état de la fonctionnalité + entrée dans le Journal : fait / fichiers / vérification / limites).
- **Toutes les 5 fonctionnalités** : une livraison (APK dans la Release `apercu`, captures vérifiées, lien à l'utilisateur).
- Tout en français (code commenté, messages, interface). Commits en français.
