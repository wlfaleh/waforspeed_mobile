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
| #424 | WFS-550 | Claude, agent dev3d (01UY1Vv1yg) | ↩️ Retour lead dev (3e revue : maquettes 05, 26, 17) | bdabd2b9 | 2026-11-15 23:18 |
| #425 | WFS-555 | lead-dev (session_01UY1Vv2AmFHmyurYWvyN3oV) | 👀 En revue — lead-dev (relecteur), 2026-11-15 23:28 | 1a88fae6 | 2026-11-15 23:28 |
| #426 | WFS-551 | Claude, agent dev3d (01UY1Vv2KF) | ↩️ Retour lead dev | ec034448 | 2026-11-15 23:39 |
