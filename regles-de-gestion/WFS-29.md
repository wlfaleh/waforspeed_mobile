# WFS-29 — Android : mode plein écran immersif, icône, nom
Épic : E7 · Priorité : 0.35 (MVP) · Dépend de : —

## Besoin (qui, quoi, pourquoi)
L'application doit occuper tout l'écran pendant la course (barres système masquées) et se présenter correctement sur le
téléphone : nom « WaForSpeed » et icône au style du jeu. Ce n'est pas dans le jeu web : aucun code à porter.

## Règles de gestion
- RG-01 : mode immersif collant (`SYSTEM_UI_FLAG_IMMERSIVE_STICKY` / `WindowInsetsController`) : barres masquées en course, qui reviennent un instant au balayage du bord ; reprises à la reprise de l'application.
- RG-02 : zone de sécurité : l'image va sous l'encoche (`layoutInDisplayCutoutMode = shortEdges`), mais le HUD respecte les marges.
- RG-03 : orientation paysage verrouillée ; écran toujours allumé en course.
- RG-04 : nom affiché « WaForSpeed » ; icône adaptative (avant-plan + fond) aux couleurs du jeu : orange `#FF6A00`, cyan `#00E5FF`, fond `#07070c` ; aucun logo tiers.
- RG-05 : le banc PC et le simulateur doivent continuer à démarrer ; aucun changement du moteur de jeu.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : aucun ; identité visuelle : `css/style.css` (variables de couleur), skill `design-street-racing`.
- Modèles / sons : aucun. Retour connu : le simulateur Android n'est lancé qu'en fin d'épic ; les captures du banc ne montrent pas les barres système.

## Critères d'acceptation
- [ ] manifeste : `android:label="WaForSpeed"`, icône adaptative présente, thème plein écran
- [ ] revue du code Android : appel immersif au démarrage et à `onWindowFocusChanged`
- [ ] captures du banc PC inchangées ; vérification sur capture du simulateur au ticket de fin d'épic E7

## Hors périmètre
Manettes (WFS-23), iOS (WFS-22), fiches des stores (WFS-25).

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| 0.35 | WFS-29 | [MVP] Android : mode plein écran immersif, icône, nom | E7 | ✅ Terminé | | — | `feature/waforspeed-29` | #17 | ✅ VALIDÉ par qa-lead ([rapport](qa/rapports/WFS-29.md)) |

Critères et notes de suivi :

**WFS-29 — [MVP] Android : mode plein écran immersif, icône, nom** · E7 · priorité 0.35 · ✅ Terminé (règles : `regles-de-gestion/WFS-29.md`)
- [ ] manifeste : `android:label="WaForSpeed"`, icône adaptative, thème plein écran ; appel immersif au démarrage et à `onWindowFocusChanged`
- [ ] captures du banc PC inchangées (vérification à l'écran par WFS-50)
