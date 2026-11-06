# WFS-59 — [MVP] Point d'extension : mode GPU logiciel (temps de jeu au temps réel, rendu allégé) (SIM-MVP-1)
Épic : E0 · Priorité : 0.438 (MVP, avant WFS-50) · Dépend de : — · Taille : ≈ ½ journée

## Besoin (qui, quoi, pourquoi)
Sur l'émulateur de la QA (GPU logiciel SwiftShader) le jeu tourne à 0,3 image/s malgré la qualité réduite actuelle
(rendu 50 %, sans MSAA, `src/render/vk.cpp`). Comme `Game::update` plafonne dt à 0,05 s, le temps du jeu avance de
≈ 0,015 s par seconde : le décompte dure 3 min 20 et la course de 30 s plus de 30 min (SIM-MVP-1). Le parcours MVP doit
se jouer sur l'émulateur en temps réel, **sans rien changer sur un vrai téléphone**.

## Règles de gestion
- RG-01 : **activation** : mode actif si le GPU est logiciel (détection existante `SwiftShader` / `llvmpipe` de
  `src/render/vk.cpp`, exposée par un booléen lisible via l'API de `renderer.h`, ex. `gpuLogiciel()`) **ou** si l'intent
  porte l'extra `qa_simu=true` (lecture dans `src/platform/android_main.cpp` : ligne d'accroche ≤ 10 lignes, annoncée au
  Journal). Ligne de logcat au démarrage : `mode GPU logiciel : actif (dt max 0,5 s, rendu 25 %, sans ombres)`.
- RG-02 : **temps de jeu** : en mode actif, plafond de dt de `Game::update` porté de 0,05 s à **0,5 s** ; la physique
  garde ses pas fixes de 1/120 s (boucle `acc_` existante), avec au plus 60 pas par image. Hors mode : 0,05 s inchangé.
- RG-03 : **rendu allégé** : en mode actif, `renderScale` **0,25** (au lieu de 0,5) et ombres du soleil coupées
  (`forceNoShadows`, déjà dans `renderer.h`). Modification de `vk.cpp` = accroche ≤ 10 lignes annoncée (E4 libre).
- RG-04 : objectif mesuré dans le logcat du simulateur : `QA ips` ≥ 1 en course (avec l'écran réduit de WFS-58) et décompte
  + course de 30 s finis en ≤ 3 min réelles. Règles de course inchangées (30 s, décompte 3 s : `src/game/course.h`).
- RG-05 : **aucun changement sur GPU matériel ni sur le banc PC** : captures du banc identiques à `main`, tests à 0 écart.

## À réutiliser
- Rien du jeu web (`wlfaleh/waaps`, `waforspeed/`) : le jeu navigateur n'a pas de mode GPU logiciel ; ne pas porter son
  réglage de qualité (`js/` : pixel ratio du navigateur, sans équivalent utile ici).
- Natif : détection de `src/render/vk.cpp` (l. 85-88), `forceNoShadows` de `renderer.h`, `renderScale` de `vk`, boucle à pas fixe
  de `Game::update` (`src/game/game.cpp`). **Ne pas créer de second système de qualité.**

## Critères d'acceptation
- [ ] banc PC : captures identiques à `main`, `0 écart(s)` (GPU matériel / rendu hors écran, mode inactif)
- [ ] simulateur (`simulateur.yml`, `scenario=mvp`, sur la branche) : logcat avec la ligne de RG-01 et `QA ips` ≥ 1 ;
      captures du décompte puis de la course qui avance (chrono < 30,0, vitesse > 0)

## Hors périmètre
- Durée de course raccourcie, distance de vue, scénario du simulateur (WFS-58), rapport `simulateur-MVP.md` (WFS-50).

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P1 Urgent | WFS-59 | [MVP] Point d'extension : mode GPU logiciel, temps de jeu au temps réel et rendu allégé (SIM-MVP-1) | E0 | ✅ Fusionné (QA de nuit à faire) — gel du simulateur non reproduit au run 41719456821 (WFS-64 suspendu) | agent 3, 2026-11-03 | — | `feature/waforspeed-59` | [#31](https://github.com/wlfaleh/waforspeed_mobile/pull/31) |  |

Critères et notes de suivi :

**WFS-59 — [MVP] Point d'extension : mode GPU logiciel (SIM-MVP-1)** · E0 · priorité P1 Urgent · ✅ Fusionné le 2026-11-03 (PR #31, QA de nuit : `qa/Check.md`) — agent 3, 2026-11-03 (règles : `regles-de-gestion/WFS-59.md`)
- [ ] GPU logiciel ou extra `qa_simu` : dt max 0,5 s, rendu 25 %, sans ombres ; logcat `QA ips` ≥ 1, course qui avance sur les captures du simulateur
- [ ] banc PC : captures identiques à `main`, `0 écart(s)`
