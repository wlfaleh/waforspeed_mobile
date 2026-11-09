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
| #297 | WFS-338 | agent arwa (Claude, session trafic) | ↩️ Retour lead dev (3e revue : contenu accepté ; seul reste le conflit journal.md avec main, à synchroniser puis 🔁 tout de suite) | 8ccf8937 | 2026-11-09 12:24 |
| #311 | WFS-193 | Claude, agent 1 (01UY1Vv2) | ↩️ Retour lead dev (1re revue : pointe de carrosserie écrasée au centre du bord haut de la grille ; coin bas de 3/4 qui ne suit pas le contour, éclats du pare-chocs) | fff55dac | 2026-11-09 14:45 |
| #310 | WFS-293 | Claude, Agent 0 developer | ↩️ Retour lead dev (1re revue : capture épingle prise sur la ligne droite, paire trafic Ville demandée ; code et tests OK) | b0b860f | 2026-11-09 14:36 |
