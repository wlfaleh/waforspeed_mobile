# WFS-64 — [MVP] Correction : jeu figé sur le simulateur en mode GPU logiciel (repli du rendu à 50 %)
Épic : E0 · Priorité : P1 Urgent (MVP, avant WFS-50) · Dépend de : WFS-59 ✅ · Taille : ≈ ½ journée au plus

## Besoin (qui, quoi, pourquoi)
Depuis WFS-59, l'émulateur de la QA (SwiftShader, ou intent `qa_simu=true`) se fige à la première image qui suit la
recréation de la swapchain (« Cibles de rendu : 2400x1080, scène 600x270 ») : plus aucune ligne au logcat (ni
« Écran : course → menu », ni `QA ips`), sans erreur ni plantage ; le scénario `mvp` de WFS-58 échoue à l'étape « menu »
(délai 180 s). Preuves : Releases `simulateur-WFS-58` (run 41714023261, sans WFS-59 : la scène à 1200x540 tourne et atteint
le GO) et `simulateur-WFS-58-avec-59` (run 41716390682, figé), fichiers `qa-journal-jeu.txt`. WFS-50 (vérification
simulateur du MVP) est bloqué par ce défaut. Un vrai GPU et le banc PC ne sont pas concernés (mode inactif).

## Règles de gestion
- RG-01 : **repli décidé par le PO** : en mode GPU logiciel, la scène 3D revient à **50 %** (`gpu_logiciel::RENDU = 0.5f`
  dans `src/core/gpu_logiciel.h`), valeur qui tournait sur le simulateur avant WFS-59 (scène 1200x540). Le plafond de dt
  à 0,5 s, les 60 pas au plus par image et la coupure des ombres de WFS-59 **sont gardés**.
- RG-02 : la ligne de logcat de démarrage et le message de `vk.cpp` disent la vraie valeur :
  `mode GPU logiciel : actif (dt max 0,5 s, rendu 50 %, sans ombres)` et `GPU logiciel : qualité réduite (sans MSAA, rendu à 50 %)`
  (texte construit à partir de la constante, pas recopié).
- RG-03 : **diagnostic minimal** pour lever le doute « rendu ou 60 pas » : en mode GPU logiciel seulement, une ligne de
  logcat à la **fin de la première mise à jour** (`Game::update`, nombre de pas faits) et une à la **première image présentée**
  après chaque recréation de swapchain. Deux lignes au plus, jamais répétées à chaque image.
- RG-04 : si, malgré le repli à 50 %, le jeu reste figé, le développeur s'appuie sur les lignes de RG-03 : figé avant la fin
  de la mise à jour → ramener `PAS_MAX` à **20** (dt max 0,5 s gardé, le temps de jeu ralentit seulement si l'image dure plus
  de 1/6 s) ; figé après → c'est le rendu : le signaler au PO avec le journal (nouveau ticket E4), sans élargir ce ticket.
- RG-05 : **aucun changement sur GPU matériel ni sur le banc PC** : captures du banc identiques à `main`, `0 écart(s)`.
- RG-06 : zone : `src/core/gpu_logiciel.h` (E0) ; accroches ≤ 10 lignes annoncées au Journal dans `src/render/vk.cpp`,
  `src/render/renderer.cpp` (ligne de RG-03) et `src/game/game.cpp`. Le rendu à 25 % n'est **pas** réparé ici.

## À réutiliser
- Rien du jeu web (`wlfaleh/waaps`, `waforspeed/`) : il n'a ni Vulkan ni mode GPU logiciel ; ne rien porter.
- Natif : tout le mécanisme de WFS-59 (`src/core/gpu_logiciel.h`, `activerModeGpuLogiciel`, `plafondDt`, `PAS_MAX`,
  `renderScale` de `vk.cpp` l. 87-93, `forceNoShadows`). **Changer une constante, pas créer un second mécanisme.**
- Scénario `mvp` de WFS-58 (`simulateur.yml`, `scenario=mvp`) comme preuve, sans le modifier.

## Critères d'acceptation
- [ ] simulateur (`simulateur.yml`, `scenario=mvp`, sur la branche) : logcat avec la ligne de RG-02 (« rendu 50 % »),
      les lignes de RG-03, « Écran : course → menu », puis des lignes `QA ips` > 0 ; le scénario dépasse l'étape « menu »
      et atteint au moins le GO (captures `02-menu` et décompte visibles)
- [ ] banc PC : captures identiques à `main`, `0 écart(s)` ; tests OK

## Hors périmètre
- Faire marcher le rendu à 25 % sous SwiftShader (piste : cibles ou passes à 1/8-1/16 de 600x270, blits minuscules) :
  ticket E4 séparé seulement si le besoin de vitesse revient. Durée de course, scénario du simulateur (WFS-58),
  rapport `simulateur-MVP.md` (WFS-50).
