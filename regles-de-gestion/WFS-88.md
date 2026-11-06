# WFS-88 — iOS : Renderer::init ne se bloque plus, démarrage jusqu'au menu
Épic : E4 · Priorité : P4 Basse · Dépend de : WFS-78 ✅ (surface Metal)

## Besoin (qui, quoi, pourquoi)
Sur iOS (simulateur de la CI `ios.yml`), l'application crée sa surface Metal et sa swapchain (2622x1206), puis reste
bloquée dans `Renderer::init` (aucun journal pendant 600 s, jamais « QA pret »). Même arrêt que WFS-22. Tant que ce
blocage dure, le menu iOS ne s'affiche pas et le son iOS (WFS-79) ne peut pas être vérifié.

## Règles de gestion
- RG-01 : partir de la pile `ios-pile.txt` (artefact du run `ios.yml` de main, 41877869097, ou d'une relance) pour
  trouver l'appel exact qui bloque (création de pipeline, compilation de shader par MoltenVK, attente de fence,
  allocation…). Si la pile n'est pas exploitable, ajouter des journaux par étape dans `Renderer::init` (une ligne par
  sous-étape, temps en ms) et relancer `ios.yml`.
- RG-02 : corriger la cause dans `src/render/**`, sous `#if defined(__APPLE__)` si la correction ne vaut que pour
  MoltenVK. Aucun changement de comportement sous Android ni sur le banc PC.
- RG-03 : `Renderer::init` se termine sur iOS en moins de 60 s ; l'écran de chargement puis le menu s'affichent.
- RG-04 : si les 2 relances d'`ios.yml` ne suffisent pas à trouver la cause, s'arrêter, consigner la pile et
  l'hypothèse au Journal et passer le ticket en ⏸ En attente (pas d'acharnement : iOS est P4).

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Rien à réutiliser : blocage propre au moteur natif Vulkan / MoltenVK, sans équivalent dans three.js.
- Réutiliser la capture iOS déjà en place dans `ios.yml` (WFS-78 : `ios-menu.png`, `ios-course.png`, `ios-pile.txt`).

## Critères d'acceptation (vérifiables sur la CI)
- [ ] `ios.yml` de la branche : « QA pret » atteint, `ios-menu.png` montre le menu (pas un écran noir)
- [ ] journal : pas de trou de plus de 60 s après l'étape vulkan
- [ ] CI Android (`build.yml`) verte, captures du banc PC inchangées

## Hors périmètre
- Son iOS (WFS-79) : se vérifie de lui-même une fois le menu atteint, pas de travail en plus ici.
- Performances iOS, TestFlight, signature.
- Zone : `src/render/**` (E4) ; accroche ≤ 10 lignes dans `ios/**` ou `ios.yml` permise si annoncée au Journal.

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P4 Basse | WFS-88 | iOS : `Renderer::init` ne se bloque plus, démarrage jusqu'au menu | E4 | ⏸ Suspendu (GEL iOS) ; **PR #76 fusionnée le 2026-11-05** (1c28ae5, fusion par confiance, étape 0 de WFS-94 ; ordre de l'utilisateur, `tache utilisateur.md`, 2026-11-04 : seule exception au GEL iOS, aucune autre suite iOS) | Claude (session trafic), 2026-11-04 | WFS-78 ✅ | `feature/waforspeed-88` | #76 |  |

Critères et notes de suivi :

**WFS-88 — iOS : `Renderer::init` ne se bloque plus** · E4 · P4 Basse · ⏸ Suspendu (GEL iOS) ; PR #76 à regarder et fusionner par WFS-103 (ordre de l'utilisateur, 2026-11-04) — Claude (session trafic), 2026-11-04, branche `feature/waforspeed-88` (règles : `regles-de-gestion/WFS-88.md`). Zone : `src/render/**` (+ accroche ≤ 10 lignes `ios/**` / `ios.yml` annoncée)
- [ ] `ios.yml` : « QA pret » atteint, `ios-menu.png` montre le menu
- [ ] plus de trou de 60 s après l'étape vulkan ; CI Android verte, banc PC inchangé
