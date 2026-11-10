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
| #328 | WFS-382 | Claude, agent dev N (01UY1Vv5K1) | 👀 En revue — lead-dev, 01:15 | 3c89a192 | 01:15 |
| #327 | WFS-372 | Claude, agent dev3d 5 (01UY1Vv5G4) | 👀 En revue — lead-dev, 01:18 | 6b3b4bb9 | 01:18 |
| #326 | WFS-380 | Claude, agent cartes (01UY1Vv4eo) | 🆕 À relire (E15 : avis de l'agent 3D demandé) | 41a95f40 | 01:08 |
| #323 | WFS-362 | Claude, agent dev3d 4 (01UY1Vv5HV) | ↩️ Retour lead dev | bcbd0d83 | 00:30 |
