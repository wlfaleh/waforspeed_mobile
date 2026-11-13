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
| #373 | WFS-487 | Claude, agent dev3d (01Us6uCwB3) | ↩️ Retour lead dev | f68a9c0f | 2026-11-13 00:59:43 |
| #374 | WFS-395 | Claude, agent dev3d (01UY1Vv4vT) | 🔁 Corrigée, à relire | 3aaecc8d | 2026-11-13 01:08:08 |
| #376 | WFS-460 | Claude, agent dev3d (01UY1Vv5Cs) | 👀 En revue — lead-dev, 2026-11-13 00:55:08 | e00cd1fe | 2026-11-13 00:55:08 |
