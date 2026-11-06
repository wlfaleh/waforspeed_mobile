# Règles de gestion des US (user stories)

> **Fusion directe par confiance (règle de l’utilisateur, 2026-11-04, prioritaire)** : développeur confiant à plus de 50% → il fusionne tout de suite (conditions et suite : `CLAUDE.md`, règle de fusion n°1).

Un fichier par ticket : `WFS-XX.md`, **écrit par l'agent `po-scrum-master`** quand il crée ou affine le ticket,
et **au plus tard avant de l'attribuer**. Le développeur le lit avant de coder ; le QA lead s'en sert pour juger.

## Ce qu'on réutilise (obligatoire à vérifier pour chaque US)
Le code, les modèles 3D (`assets/cars/`, `assets/people/`), les sons (`assets/audio/`) et les systèmes **déjà présents dans ce dépôt** :
chaque US cite les fichiers et fonctions à réutiliser ; ne jamais recréer ce qui existe. Il n'y a plus de version Web (décision de
l'utilisateur, 2026-11-06). Pour l'aspect du monde 3D, la référence est le réel mesuré, fidélité 99 % : `DA-realisme.md`.

## Modèle de fichier
```markdown
# WFS-XX — <titre>
Épic : EX · Priorité : N · Dépend de : …

## Besoin (qui, quoi, pourquoi)
…

## Règles de gestion
- RG-01 : <règle précise, chiffrée si possible (valeurs sourcées)>
- RG-02 : …

## À réutiliser (dans ce dépôt)
- Code : `src/…` → fonctions `…` (ce qu'on réutilise, ce qu'on adapte)
- Modèles 3D / sons / textures : `models/…`, `audio/…`
- Retours de l'utilisateur déjà connus : …

## Critères d'acceptation (vérifiables sur les captures du banc PC)
- [ ] …

## Hors périmètre
- …
```
