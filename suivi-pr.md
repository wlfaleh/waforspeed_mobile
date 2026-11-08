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
| #245 | WFS-232 | arwa-3 | 👀 En revue — lead-dev (session 01Us6uCt), 2026-11-08 04:51 | 6baa3128 | 2026-11-08 04:51 |
| #248 | WFS-268 | Claude, agent 5 | ↩️ Retour lead dev (2e revue : captures après périmées, RG-11 / ANO-164, resynchroniser) | 213df11 | 2026-11-08 02:16 |
| #249 | WFS-281 | agent arwa | ↩️ Retour lead dev (2e refus : banc après avec la voiture sur le trottoir, RG-02 non tenu, décision PO attendue) | 98d22f7 | 2026-11-08 02:16 |
| #251 | WFS-282 | Claude, agent 2 (01UY1Vv5) | 🔁 Corrigée, à relire (commits poussés après le 2e refus ; attente réponse « corrigé dans » sous la revue) | f97172f | 2026-11-08 03:17 |
| #256 | WFS-269 | Claude, Agent 0 developer | ↩️ Retour lead dev (2e refus) | ae2ae77 | 2026-11-08 04:41 |
| #257 | WFS-297 | Claude, agent 1 (01UY1Vv2) | 🔁 Corrigée, à relire (commit poussé après la 1re revue ; attente réponse « corrigé dans » sous la revue) | d8bc9d7 | 2026-11-08 04:18 |
| #259 | WFS-263 | Claude, agent dev X | 👀 En revue — lead-dev (session 0183bv2X), 2026-11-08 04:48 | faa6aaef | 2026-11-08 04:48 |
