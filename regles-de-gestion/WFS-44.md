# WFS-44 — Point d'extension : scène dédiée par écran (caméra, fond, brouillard, lumière, ce que Game dessine)
Épic : E0 (Noyau partagé) · Priorité : 0.45 · Dépend de : WFS-36 (✅)

## Besoin (qui, quoi, pourquoi)
Le garage (WFS-11) et plus tard le choix du terrain (WFS-41) ont besoin d'une **scène à eux** : leur caméra, leur fond,
leur brouillard, sans la route ni la voiture de course. Aujourd'hui `Game::render` remplit seul `FrameParams` et dessine
toujours route + voiture du joueur : un système ne peut que dessiner en plus. Catalyseur : débloque WFS-11 (E5), puis WFS-40, WFS-13.

## Règles de gestion
- RG-01 : nouvelle méthode virtuelle facultative de `System` (`src/core/system.h`), par ex.
  `virtual bool scene(FrameParams& fp, SceneDediee& s, const GameView&) { return false; }`, appelée par `Game::render`
  **avant** `beginFrame`, dans l'ordre des systèmes ; le **premier** qui renvoie `true` prend la scène de l'image.
- RG-02 : le système qui prend la scène peut tout régler dans `fp` : `view`, `proj`, `camPos`, ciel (`skyTop`, `skyHorizon`,
  `ground` ; fond uni = mêmes couleurs), brouillard, soleil (direction, couleur, intensité, 0 = éteint), `ambient`, `exposure`,
  ombres (`shadows`, `focus`). `Game` part de ses valeurs habituelles puis laisse le système les modifier.
- RG-03 : `SceneDediee` dit ce que `Game` dessine encore : `route` (sol, trottoirs, route), `joueur` (voiture du joueur + ombre
  de contact), `commandes` (tactile de conduite) ; **tout à `false` par défaut** quand une scène dédiée est prise.
- RG-04 : les autres systèmes le savent : `GameView::sceneDediee` (bool) et `GameView::view/proj/camPos` = la caméra choisie ;
  la ville (E2) et le trafic (E3) **ne dessinent plus** quand `sceneDediee` est vrai (1 ligne d'accroche chacun, faite dans ce ticket).
- RG-05 : sans système qui prend la scène, **rien ne change** : même `FrameParams`, mêmes dessins (pixel près).
- RG-06 : pas de nouvelle lumière ici (spots, hémisphère : WFS-45, E4) ; ne pas toucher à `src/render/**` hors l'API de `renderer.h`.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- `js/world/garage.js` → `renderGarage` : le web fait `renderPass.scene = gScene; renderPass.camera = gCamera` (scène et caméra
  propres au garage, rendues par le même moteur) : même idée, sans second moteur.
- Natif : `Game::render` (`src/game/game.cpp`, remplissage de `fp`), `System`/`GameView` (WFS-30, WFS-36). Rien d'autre.

## Critères d'acceptation (captures du banc PC)
- [ ] Toutes les captures du banc **identiques** à `main` (aucune scène dédiée par défaut) ; tests trafic / course à 0 écart
- [ ] Nouvelle capture `scene-dediee-vide` : système de test du banc qui prend la scène : fond uni `#07070c`, aucune route,
      ville, trafic ni voiture du joueur, aucune commande tactile ; un cube de repère vu par la caméra choisie (FOV 36°)
- [ ] Doc du point d'extension dans le skill `waforspeed-natif` (comme WFS-30, WFS-36)

## Hors périmètre
Projecteurs et lumière hémisphère, brouillard linéaire (WFS-45) ; scène du garage elle-même (WFS-11).

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| 0.45 | WFS-44 | Point d'extension : scène dédiée par écran (caméra, fond, brouillard, ce que Game dessine) | E0 | ✅ Terminé |  | WFS-36 | `feature/waforspeed-44` | #15 (fusionnée) | ✅ VALIDÉ par qa-lead (b9771e0, [rapport](qa/rapports/WFS-44.md)) |

Critères et notes de suivi :

**WFS-44 — Point d'extension : scène dédiée par écran** · E0 · priorité 0.45 · ✅ Terminé (PR #15) (règles : `regles-de-gestion/WFS-44.md`)
- [ ] Captures du banc identiques à `main` (aucune scène dédiée par défaut), tests à 0 écart
- [ ] Capture `scene-dediee-vide` : fond uni `#07070c`, ni route, ni ville, ni trafic, ni voiture du joueur, ni commandes ; cube repère vu en FOV 36°
- [ ] Doc du point d'extension dans le skill `waforspeed-natif`
