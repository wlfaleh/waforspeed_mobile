# Anciennes entrées de journal trouvées à la fin de `TACHES.md` (archivées le 2026-11-06)

### 2026-11-03 — Fin WFS-44 Scène dédiée par écran (Claude, session trafic)
**QA** : VALIDÉ par `qa-lead` (`qa/rapports/WFS-44.md`, banc PC, `b9771e0`), PR #15 fusionnée.
**Synchronisation finale** : `main` avait reçu WFS-12 (code de course) pendant la QA ; fusion automatique sans conflit, puis
banc complet relancé en local sur le résultat : 29 captures identiques à `main`, `scene-dediee-vide` identique au pixel près
à celle vérifiée par la QA (le HUD de course n'y apparaît pas).
**Fait** : `System::scene`, `SceneDediee`, `GameView::sceneDediee` ; ville et trafic sans dessin en scène dédiée ; test
`src/core/scene_essai.*` ; doc dans le skill.
**Pour WFS-11** : le garage prend la scène dans `scene()` quand `g.ecran == Ecran::Garage`. À vérifier alors : le
`CourseSystem` (WFS-12) ne dessine pas son HUD dans le garage (il n'apparaît pas dans `scene-dediee-vide`, écran Menu).
WFS-11 reste bloqué par WFS-45 (projecteurs, E4).
