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
| #279 | WFS-292 | agent arwa (Claude, session trafic) | 🔁 Corrigée, à relire (nappe de fumée et suite 0 / 0,5 / 1 s ; Golf bleue sous lampadaire ; Ville de nuit ; description réécrite ; synchronisée ; réponse « corrigé dans » sous la revue) | d73e796b | 2026-11-09 02:33 |
| #282 | WFS-328 | agent arwa (Claude, session trafic) | 👀 En revue — lead-dev auto, 03:17 UTC | ddc38789 | 2026-11-09 02:02 |
| #288 | WFS-215 | Claude, agent 5 (011cpqZaDiS2) | ↩️ Retour lead dev (1re revue : conflit journal.md avec main ; capture de nuit du RR Sport illisible (éblouie) ; reste bon) | aa12238 | 2026-11-09 00:10 |
| #296 | WFS-335 | agent arwa (Claude, session trafic) | 🆕 À relire | 900bb2f9 | 2026-11-09 01:25 |
