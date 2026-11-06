# WFS-48 — Texte en italique (correction ANO-04)
Épic : E5 · Priorité : 0.48 (hors MVP) · Dépend de : WFS-26 ✅ · Taille : ≈ 1 à 2 h · Origine : ANO-04 (QA WFS-12)

## Besoin
Le jeu web écrit les chiffres du HUD, du décompte, du score et des stats en italique (`font-style: italic`) ; le texte natif
ne sait pas incliner les glyphes, donc tout est droit.

## Règles de gestion
- RG-01 : `ui::text` gagne une option d'inclinaison (ex. paramètre `italique` ou `inclinaison` en degrés) : chaque quad de glyphe
  est cisaillé en x selon `y` (haut décalé vers la droite), **12°** par défaut (`skewX(-12deg)` du web pour l'italique synthétique).
- RG-02 : l'appel par défaut reste droit : aucune capture existante ne change tant qu'un écran ne demande pas l'italique.
- RG-03 : la largeur mesurée (`ui::width`) ne change pas ; l'alignement à droite et au centre reste juste.
- RG-04 : activer l'italique là où le web l'a : décompte et écran de fin (`src/game/course_system.cpp`, accroche E3 ≤ 10 lignes
  annoncée), HUD (`src/ui/hud.*`).

## À réutiliser du jeu web
`css/style.css` : `.time-value`, `.score-value`, `.speedo-read`, `.stats dd`, `.combo`, `.toast b` (`font-style: italic`).
Natif : `src/ui/text.*` (WFS-26).

## Critères d'acceptation
- [ ] `course-decompte-3`, `course-decompte-go`, `course-fin`, `course-*` (HUD) : chiffres inclinés, nets, sans débordement
- [ ] `texte-essai-*` : une ligne droite et une ligne italique côte à côte ; autres captures inchangées

## Hors périmètre
Vraie police italique (fichier de police supplémentaire).

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P2 Haute | WFS-48 | Texte en italique (ANO-04) | E5 | ✅ Fusionné (ordre de l'utilisateur, QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2u), 2026-11-03 | WFS-26 | `feature/waforspeed-48` | [#33](https://github.com/wlfaleh/waforspeed_mobile/pull/33) |  |

Critères et notes de suivi :

**WFS-48 — Texte en italique (ANO-04)** · E5 · priorité P2 Haute · ✅ Fusionné le 2026-11-03 sur ordre de l'utilisateur (QA de nuit : `qa/Check.md`) — Claude (agent 5), 2026-11-03 (règles : `regles-de-gestion/WFS-48.md`)
- [ ] décompte, écran de fin et HUD en italique (12°) ; `texte-essai-*` : ligne droite + ligne italique ; autres captures inchangées
