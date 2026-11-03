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
