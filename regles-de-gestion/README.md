# Règles de gestion des US (user stories)

> **Fusion directe par confiance (règle de l’utilisateur, 2026-11-04, prioritaire)** : développeur confiant à plus de 50% → il fusionne tout de suite (conditions et suite : `CLAUDE.md`, règle de fusion n°1).

Un fichier par ticket : `WFS-XX.md`, **écrit par l'agent `po-scrum-master`** quand il crée ou affine le ticket,
et **au plus tard avant de l'attribuer**. Le développeur le lit avant de coder ; le QA lead s'en sert pour juger.

## Ce qu'on réutilise (obligatoire à vérifier pour chaque US)
Le jeu existe déjà en version navigateur dans le dépôt **`wlfaleh/waaps`**, dossier **`waforspeed/`** :
- **Code de référence** : `js/` (règles du jeu, physique, trafic, décor, caméra, sons, interface). Le natif reprend
  les **mêmes règles et les mêmes valeurs** : chaque US cite les fichiers et fonctions JS à porter.
- **Modèles 3D déjà prêts** : `models/` (20 voitures réelles avec licences, versions allégées `-lod`, personnages
  `models/people/`), à convertir avec `tools/convert_car.mjs` (voir skill `waforspeed-natif`) — **ne jamais en chercher
  ou en créer de nouveaux si un modèle existe déjà**.
- **Sons** : `audio/engines/` (déjà copiés dans `assets/audio/`).
- **Leçons apprises** (ce que l'utilisateur a refusé ou validé) : skill `waforspeed-3d` (référence de la version web).

## Modèle de fichier
```markdown
# WFS-XX — <titre>
Épic : EX · Priorité : N · Dépend de : …

## Besoin (qui, quoi, pourquoi)
…

## Règles de gestion
- RG-01 : <règle précise, chiffrée si possible (valeurs du jeu web)>
- RG-02 : …

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/…` → fonctions `…` (ce qu'on porte, ce qu'on adapte)
- Modèles 3D / sons / textures : `models/…`, `audio/…`
- Retours de l'utilisateur déjà connus : …

## Critères d'acceptation (vérifiables sur les captures du banc PC)
- [ ] …

## Hors périmètre
- …
```
