# WFS-64 — Diagnostic : gel intermittent juste après le chargement sur le simulateur
Épic : E0 · Priorité : P4 Basse · Statut : ⏸ Suspendu (réactivé seulement si le gel se reproduit) · Dépend de : WFS-59 ✅ · Taille : ≈ ½ journée

> **Révision PO 2026-11-03 21:40:54** : l'ancien contenu (« repli du rendu à 50 % ») est abandonné. Le run 41719456821
> (cc78b4d, code de la PR #32 avec WFS-59, Release `simulateur-WFS-58`) passe tout le parcours MVP avec le rendu à 25 % :
> le gel vu à 20:55 (run 41716390682) et dans un essai `conduite` sans WFS-59 n'est **pas systématique** et pas causé par
> WFS-59. Changer la valeur du rendu sans preuve serait coûteux et inutile.

## Besoin
Le QA doit pouvoir faire confiance au simulateur : si l'APK se fige parfois juste après le chargement (plus aucune ligne
au logcat, sans plantage), savoir **où** (mise à jour du jeu, rendu, ou émulateur / focus) avant de corriger quoi que ce soit.

## Condition de réactivation (décidée par le PO)
- Le ticket repasse en ⏳ Prêt si un essai `simulateur.yml` lancé sur `main` (WFS-50 ou une autre vérification) se fige
  après le chargement. Le QA le note dans son rapport avec le run et `qa-journal-jeu.txt` ; le PO change le statut.

## Règles de gestion
- RG-01 : **diagnostic d'abord, aucune correction à l'aveugle** : ne changer ni `gpu_logiciel::RENDU`, ni `PAS_MAX`, ni le dt max.
- RG-02 : en mode GPU logiciel seulement, deux lignes de logcat au plus, jamais répétées à chaque image : fin de la première
  mise à jour (`Game::update`, nombre de pas faits) et première image présentée après chaque recréation de swapchain.
  Ajouter aussi au journal du scénario (`tools/qa/`) l'état du focus de l'activité (`dumpsys window | grep mCurrentFocus`) au moment du gel.
- RG-03 : lecture : figé avant la fin de la mise à jour → jeu (ramener `PAS_MAX` à 20) ; figé après → rendu (ticket E4 à part,
  au PO) ; jeu qui tourne mais activité sans focus → émulateur (relance de l'étape côté `tools/qa/`, ticket E7 à part).
- RG-04 : aucun changement sur GPU matériel ni sur le banc PC : captures du banc identiques à `main`, `0 écart(s)`.
- RG-05 : zone : `src/core/gpu_logiciel.h` (E0) ; accroches ≤ 10 lignes annoncées au journal dans `src/game/game.cpp`, `src/render/vk.cpp`.

## À réutiliser
- Rien du jeu web (`wlfaleh/waaps`, `waforspeed/`) : pas de Vulkan ni d'émulateur ; rien à porter.
- Natif : mécanisme de WFS-59 (`src/core/gpu_logiciel.h`, `activerModeGpuLogiciel`, `plafondDt`, `PAS_MAX`) et scénario `mvp` de WFS-58, sans les modifier.

## Critères d'acceptation
- [ ] le gel reproduit est expliqué au journal par les lignes de RG-02 (mise à jour, rendu ou focus), avec le run cité
- [ ] après la correction ciblée qui en découle : 3 essais `scenario=mvp` de suite sans gel
- [ ] banc PC : `0 écart(s)` ; tests OK

## Hors périmètre
- Réparer le rendu à 25 % ou le changer de valeur sans preuve ; le scénario `mvp` (WFS-58) ; le rapport `simulateur-MVP.md` (WFS-50).
