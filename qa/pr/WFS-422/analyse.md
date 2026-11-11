# WFS-422 — analyse du code avant de couper la pré-passe (Claude, agent dev (01UY1Vv5hT))

## Constat (lecture du code de `main`, 2026-11-11)
- La pré-passe (`PrePasse`, `src/render/passes.cpp`) ne tourne **que si une passe active la demande**
  (`avantScene` : `p->demandePrePasse() && tourne(*p, ip)`).
- Une seule passe la demande : la **passe d'essai** (`src/render/passe_essai.cpp`, `demandePrePasse() = true`),
  active seulement si `passes::essaiActif()`, allumé **uniquement par le banc** `bench_passes.cpp` (capture `realisme-prepasse-essai`).
- Les autres passes enregistrées (`CaptureEnvironnement`, `SondesIbl`, `PasseRetroviseurs`) ne la demandent pas,
  et **aucun shader du jeu** n'échantillonne `prePasseNormales` / `prePasseProfondeur`.
- Donc, **en jeu, la pré-passe ne tourne à aucun niveau de qualité** (Basse, Moyenne, Haute, Ultra) : l'interrupteur
  « sans pré-passe » de WFS-419 ne change rien à l'image ni au coût. L'écart mesuré sur le téléphone (44 ms en moyenne,
  57 ms en instantané contre 62 ms) ne peut pas venir de la pré-passe : c'est la variation d'une mesure à l'autre
  (chauffe, endroit, trafic).

## Vérification au banc
Voir `verification.md` (journal du banc : la pré-passe n'est créée que par la capture `realisme-prepasse-essai`).
