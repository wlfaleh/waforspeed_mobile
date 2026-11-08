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
| #248 | WFS-268 | Claude, Agent 0 developer (reprise de Claude, agent 5, décision du PO 11:22) | 🔁 Corrigée, à relire (commits poussés après le 2e refus ; attente réponse « corrigé dans » sous la revue) | 213df11 | 2026-11-08 12:17 |
| #249 | WFS-281 | agent arwa | ↩️ Retour lead dev (2e refus : banc après avec la voiture sur le trottoir, RG-02 non tenu, décision PO attendue) | 98d22f7 | 2026-11-08 02:16 |
| #269 | WFS-308 | Claude, agent 3 (01UY1Vv3) | ↩️ Retour lead dev (3e refus : conflit journal.md avec main après #275, captures « après » faites avant la synchronisation ; code bon) | 38ad6a8a | 2026-11-08 18:38 |
| #272 | WFS-300 | Claude, agent 5 (011cpqZaDiS2) | ↩️ Retour lead dev (1re revue : 24 commits de retard sur main avec 2 conflits, captures à régénérer ; code bon) | 22390b9 | 2026-11-08 17:25 |
| #276 | WFS-317 | Claude, agent 1 (01UY1Vv2) | ↩️ Retour lead dev (1re revue : captures après faites sans WFS-265, branche en retard de #267 et #275, 267 ≠ 263 personnes, rue vide sur vie-terrasse ; code bon) | b1ac2ca | 2026-11-08 18:29 |
