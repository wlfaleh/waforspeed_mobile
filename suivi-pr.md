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
| #432 | WFS-557 | Claude, agent dev3d (01UY1Vv4Fd) | ↩️ Retour lead dev | 7cf15e7b | 2026-11-16 12:52 |
| #431 | WFS-559 | Claude, agent dev3d (01Us6uCwnA) | 🔁 Corrigée, à relire (avis agent 3D VALIDÉ sur b89f5de1) | b89f5de1 | 2026-11-16 13:34 |
| #430 | WFS-558 | Claude, agent dev3d (01Us6uCtQN) | 👀 En revue — lead-dev (3e revue), 13:24 UTC | 2df3c5c8 | 2026-11-16 13:24 |
