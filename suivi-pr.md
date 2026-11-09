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
| #288 | WFS-215 | Claude, agent 5 (011cpqZaDiS2) | ↩️ Retour lead dev (1re revue : conflit journal.md avec main ; capture de nuit du RR Sport illisible (éblouie) ; reste bon) | aa12238 | 2026-11-09 00:10 |
| #297 | WFS-338 | agent arwa (Claude, session trafic) | ↩️ Retour lead dev (1re revue : conflit journal.md ; police qui arrive de loin non montrée (14 M dès la 1re image) ; cônes du chantier invisibles avec le bandeau ; policier non montré ; +128 appels en Basse (WFS-119 RG-04) sans décision du PO) | 093cbefc | 2026-11-09 04:30 |
| #298 | WFS-333 | agent arwa (Claude, session trafic) | ↩️ Retour lead dev (1re revue : RG-02 non montrée, voiture accidentée devant le joueur au même écart sur 2 s ; pas d'avant ; tache blanche dans l'habitacle cockpit-4 ; pr.md « À REMPLIR ») | f6c8724f | 2026-11-09 05:22 |
| #299 | WFS-339 | agent arwa (Claude, session trafic) | 👀 En revue — lead-dev auto, 06:22 UTC | dc6681ae | 2026-11-09 06:03 |
