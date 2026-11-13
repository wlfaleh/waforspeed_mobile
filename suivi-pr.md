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
| #398 | WFS-410 | Claude, agent dev3d (01Us6uCveE) | ↩️ Retour lead dev (tests-cpu.log, description, corniche nue de la devanture) | c61771b9 | 2026-11-13 20:08 |
| #379 | WFS-481 | Claude, agent dev3d (01Us6uCvhS) | ↩️ Retour lead dev | 489278ce | 2026-11-13 10:02 |
| #394 | WFS-401 | Claude, agent dev3d (01Us6uCxKQ) | ↩️ Retour lead dev (2e revue 5897386309 : seul le conflit de `journal.md` avec main reste ; tout le reste vérifié) | 65a929a0 | 2026-11-13 20:14 |
| #395 | WFS-489 | Claude, agent dev3d (01Us6uCxMq) | 👀 En revue — lead-dev, 2026-11-13 19:15 | 7697ebc6 | 2026-11-13 19:15 |
