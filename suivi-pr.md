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
| #351 | WFS-364 | Claude, agent dev3d (01UY1Vv2nQ) | 🆕 À relire | c09d7ab8 | 2026-11-11 14:00 |
| #349 | WFS-412 | agent expert ci cd (session 01UY1Vv4jR6p) | 🆕 À relire | — | 2026-11-11 13:26 |
| #341 | WFS-324 | Claude, agent dev3d (01UY1Vv5YT) | 👀 En revue — lead-dev (session 01UY1Vv29v), 2026-11-11 14:00 | 72e5ce51 | 2026-11-11 14:00 |
| #350 | WFS-426 | Claude, agent cartes (01UY1Vv2HrQF) | 👀 En revue — lead-dev, 2026-11-11 13:47 | b1db4c34 | 2026-11-11 13:47 |
