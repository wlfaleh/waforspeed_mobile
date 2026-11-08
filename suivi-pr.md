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
| #249 | WFS-281 | agent arwa | 🔁 Corrigée, à relire (commits poussés après le 3e refus ; attente réponse « corrigé dans » sous la revue) | 158cdee | 2026-11-08 22:30 |
| #279 | WFS-292 | agent arwa (Claude, session trafic) | ↩️ Retour lead dev (1re revue : RG-04 non tenue, décision écrite du PO requise ; fumée de jour en taches rondes, pas en nappe ; « à remplir » dans la description ; capture Ville de nuit ; 11 commits de retard) | 62751ac | 2026-11-08 21:28 |
| #282 | WFS-328 | agent arwa (Claude, session trafic) | ↩️ Retour lead dev (1re revue : RG-05 du PO sans preuve (pleine course, Cockpit) ; avant / après identiques à l'œil ; 34 commits de retard ; « Agent : » pas en tête) | 56a70670 | 2026-11-08 22:30 |
| #286 | WFS-306 | Claude, agent 3 (01UY1Vv3) | ↩️ Retour lead dev (1re revue : bandeau du menu qui télécharge sans taille ni alerte données mobiles (RG-02/06) ; phase Java bloquée sur COUPURE après « Plus tard » ; pas de test CPU de l'étape ; captures sans « avant » du menu ni format téléphone) | 6ec29c5c | 2026-11-08 22:37 |
| #287 | WFS-290 | Claude, agent 2 (01UY1Vv5) | 👀 En revue — lead-dev, 2026-11-08 23:04 | 7a24729 | 2026-11-08 23:03 |
