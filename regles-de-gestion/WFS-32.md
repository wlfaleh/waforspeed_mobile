# WFS-32 — suivi (fichier créé à la réorganisation de `TACHES.md`, 2026-11-06)


## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| 5.5 | WFS-32 | Carrosseries : supprimer les points blancs en trame (reflets du vernis) | E4 | ✅ Terminé | | — | `feature/waforspeed-32` | #8 | ✅ VALIDÉ par qa-lead (84b489b, banc PC) — [rapport](qa/rapports/WFS-32.md) |

Critères et notes de suivi :

**WFS-32 — Carrosseries : supprimer les points blancs en trame (reflets du vernis)** · E4 · priorité 5.5 · ✅ Terminé
Origine : remarque 5 du [rapport QA WFS-08](qa/rapports/WFS-08.md) (défaut antérieur à WFS-08). Zone : `src/render/**`
(sauf `ui_*`), `shaders/scene*`. Pistes : reflet spéculaire du vernis (`scene.frag`) trop fin / non filtré (aliasing
spéculaire, normales non normalisées, NaN, valeurs HDR extrêmes avant le tonemapping), résolution MSAA.
- [ ] Cause identifiée et notée au Journal (avec la capture qui la montre)
- [ ] Captures du banc PC `vitrine-golf-0`, `vitrine-p911-1`, `course-cam0-1`, `trafic-choc` : plus aucun point blanc
  isolé en trame sur les carrosseries (Golf près des feux arrière, avant de la 911, Gallardo)
- [ ] Les reflets du vernis restent visibles (peinture toujours brillante, pas d'aspect mat) ; aucune autre régression visible
- [ ] Coût GPU inchangé à ±0,5 ms (journal du banc)
