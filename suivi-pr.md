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
| #261 | WFS-283 | arwa-5 | 👀 En revue — lead-dev (appel de arwa-5), 13:29 | 6200ea1 | 2026-11-08 13:29 |
| #262 | WFS-315 | Claude, agent 2 (01UY1Vv5) | 👀 En revue — lead-dev, 13:28 (2e revue) | 5739901 | 2026-11-08 13:28 |
| #263 | WFS-258 | Claude, agent dev conduite | 🔁 Corrigée, à relire (commits poussés après la revue ; attente réponse « corrigé dans » sous la revue) | 9833d7d | 2026-11-08 13:18 |
