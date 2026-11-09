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
| #279 | WFS-292 | agent arwa (Claude, session trafic) | ↩️ Retour lead dev (1re revue : RG-04 non tenue, décision écrite du PO requise ; fumée de jour en taches rondes, pas en nappe ; « à remplir » dans la description ; capture Ville de nuit ; 11 commits de retard) | 62751ac | 2026-11-08 21:28 |
| #282 | WFS-328 | agent arwa (Claude, session trafic) | 🔁 Corrigée, à relire (commits poussés après la 1re revue ; attente réponse « corrigé dans » sous la revue) | 56a70670 | 2026-11-09 00:00 |
| #287 | WFS-290 | Claude, agent 2 (01UY1Vv5) | 🔁 Corrigée, à relire (commits poussés après la 1re revue ; attente réponse « corrigé dans » sous la revue) | 7a24729 | 2026-11-09 00:00 |
| #289 | WFS-322 | Claude, agent 1 (01UY1Vv2) | ↩️ Retour lead dev (1re revue : images et code bons ; manquent la mesure du banc `fluidite` (RG-04), la vraie cause et les mesures finales au journal (RG-01), l'annonce des accroches `renderer.*` / `passes.h`, tests et parcours au journal) | 567e68b2 | 2026-11-09 00:03 |
| #288 | WFS-215 | Claude, agent 5 (011cpqZaDiS2) | ↩️ Retour lead dev (1re revue : conflit journal.md avec main ; capture de nuit du RR Sport illisible (éblouie) ; reste bon) | aa12238 | 2026-11-09 00:10 |
| #291 | WFS-302 | agent expert ci cd (session 01UY1Vv4) | 👀 En revue — lead-dev, 2026-11-09 00:17 | f0aa1b0f | 2026-11-09 00:17 |
