# WFS-52 — HUD et notifications dans les marges de sécurité (encoche)
Épic : E5 · Priorité : 0.435 · Dépend de : WFS-51, WFS-27 · Origine : RG-02 de WFS-29

## Besoin (qui, quoi, pourquoi)
Le HUD de course (WFS-27) ne doit jamais passer sous l'encoche ni dans les coins arrondis du téléphone.

## Règles de gestion
- RG-01 : chaque panneau du HUD (temps, checkpoint, score, vitesse, nitro) est placé dans la zone sûre : position = marge de `view.marges` + écart prévu par WFS-27.
- RG-02 : sans encoche (marges 0), rendu identique au pixel à WFS-27.
- RG-03 : les notifications (WFS-47) suivent la même règle si elles sont déjà fusionnées ; sinon WFS-47 l'applique.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Rien à porter : principe CSS `env(safe-area-inset-*)` de `css/style.css`, déjà repris par WFS-51.

## Critères d'acceptation (captures du banc PC)
- [ ] `encoche-hud` (2400x1080, `WFS_ENCOCHE=90`) : aucun texte ni panneau dans la bande de 90 px
- [ ] `course-*`, `hud-*` sans encoche identiques à `main`, banc `0 écart(s)`

## Hors périmètre
Commandes tactiles (WFS-51), menus (E8).

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| 0.435 | WFS-52 | HUD et notifications dans les marges de sécurité (encoche) | E5 | ✅ Terminé | Claude (session trafic), 2026-11-03 | WFS-51 ✅, WFS-27 ✅ | `feature/waforspeed-52` | #30 (fusionnée, 318ac8b) | ✅ VALIDÉ par qa-lead (d2b19dc) — [rapport](qa/rapports/WFS-52.md) |

Critères et notes de suivi :

**WFS-52 — HUD dans les marges de sécurité (encoche)** · E5 · priorité 0.435 · ✅ Terminé (PR #30 fusionnée, VALIDÉ) — Claude (session trafic), 2026-11-03 (règles : `regles-de-gestion/WFS-52.md`)
- [ ] `encoche-hud` : aucun panneau du HUD dans la bande ; sans encoche, captures identiques
