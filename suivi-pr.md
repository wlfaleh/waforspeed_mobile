# Suivi des PR ouvertes (lead dev)

> **Règle de l'utilisateur (2026-11-08)** : ce tableau dit où en est chaque PR ouverte et **quel lead dev la relit**, pour que plusieurs lead dev
> (appel du développeur, passage automatique de chaque heure, session de l'utilisateur) ne relisent ni ne fusionnent jamais la même PR en même temps.
> Toujours sur `main` : `git pull`, ne modifier que la ligne de la PR, envoyer tout de suite (commit `Suivi PR : #N <statut> (lead-dev)`).
> **PR fusionnée ou fermée → sa ligne est supprimée** dans le même commit que `TACHES.md`.

## Statuts
- 🆕 **À relire** : PR ouverte (ou passée inaperçue), aucun lead dev dessus.
- 👀 **En revue — <lead dev>, <heure UTC>** : un lead dev l'a prise. Les autres n'y touchent pas. Prise de plus d'**1 h** sans suite = libre (repasser en 🆕).
- ↩️ **Retour lead dev** : refusée, le développeur corrige. Personne ne la relit.
- 🔁 **Corrigée, à relire** : le développeur a répondu « corrigé dans <commit> » sous la revue (il met lui-même ce statut, ou le passage automatique le voit).

## Tableau

| PR | Ticket | Développeur | Statut | Commit relu | Mis à jour (UTC) |
|---|---|---|---|---|---|
| #398 | WFS-410 | Claude, agent dev3d (01Us6uCveE) | 🆕 À relire (E15 : attend l'avis de l'agent 3D) | 26e38ad4 | 2026-11-13 18:52 |
| #397 | WFS-483 | Claude, agent dev3d (01Us6uCvxs) | 👀 En revue — lead-dev, 2026-11-13 19:11:15 | 33945d53 | 2026-11-13 19:11:15 |
| #379 | WFS-481 | Claude, agent dev3d (01Us6uCvhS) | ↩️ Retour lead dev | 489278ce | 2026-11-13 10:02 |
| #388 | WFS-490 | Claude, agent dev3d (01Us6uCwDY) | ↩️ Retour lead dev (3e revue) | 170da9b2 | 2026-11-13 18:39 |
| #393 | WFS-397 | Claude, agent dev3d (01Us6uCt5A) | ↩️ Retour lead dev | c6b67bb2 | 2026-11-13 17:24 |
| #394 | WFS-401 | Claude, agent dev3d (01Us6uCxKQ) | 🆕 À relire (E15 : attend l'avis de l'agent 3D) | d3d070d9 | 2026-11-13 17:13 |
| #395 | WFS-489 | Claude, agent dev3d (01Us6uCxMq) | ↩️ Retour lead dev | 2b02fc63 | 2026-11-13 17:27 |
| #396 | WFS-507 | Claude, agent dev3d (01Us6uCw3C) | 👀 En revue — lead-dev, 18:55 UTC | 769fcb6a | 2026-11-13 18:55 |
