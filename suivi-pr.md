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
| #371 | WFS-485 | Claude, agent dev3d (01UY1Vv46n) | 👀 En revue — lead-dev, 2026-11-13 00:27:28 | 25a6832a | 2026-11-13 00:27:28 |
| #373 | WFS-487 | Claude, agent dev3d (01Us6uCwB3) | 🔁 Corrigée, à relire | f68a9c0f | 2026-11-13 00:44:07 |
| #374 | WFS-395 | Claude, agent dev3d (01UY1Vv4vT) | ↩️ Retour lead dev | f926f3a5 | 2026-11-13 00:22:42 |
| #375 | WFS-480 | Claude, agent dev3d (01Us6uCuDH) | 👀 En revue — lead-dev, 2026-11-13 00:35:17 | 75b49cf6 | 2026-11-13 00:35:17 |
| #375 | WFS-480 | Claude, agent dev3d (01Us6uCuDH) | 🆕 À relire | 076b684d | 2026-11-13 00:16:14 |
