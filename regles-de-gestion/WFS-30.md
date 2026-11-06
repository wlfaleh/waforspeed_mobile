# WFS-30 — suivi (fichier créé à la réorganisation de `TACHES.md`, 2026-11-06)


## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| 1 | WFS-30 | Points d'extension du moteur (systèmes enfichables, CMake par dossier) | E0 | ✅ Terminé | — | — | `feature/waforspeed-30` | #3 | ✅ VALIDÉ par qa-lead |

Critères et notes de suivi :

**WFS-30 — Points d'extension du moteur (systèmes enfichables, CMake par dossier)** · E0 · priorité 1 · ✅ Terminé
- [ ] Interface `System` (init / update / render / interface) enregistrée dans `Game` : un nouveau module = un nouveau fichier + 1 ligne d'enregistrement
- [ ] CMake ajoute automatiquement les fichiers de `src/world`, `src/ui`, `src/audio`, `src/game`
- [ ] Accès en lecture pour les systèmes : piste, joueur, caméra, renderer ; doc dans le skill `waforspeed-natif`
- [ ] Aucun changement visible ; CI verte, captures identiques
