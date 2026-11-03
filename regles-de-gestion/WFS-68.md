# WFS-68 — Réglages (qualité, caméra, son, commandes)
Épic : E8 · Priorité : P3 Moyenne · Dépend de : WFS-39 · Taille : 1 à 2 jours

## Besoin
Un écran de réglages accessible depuis le menu et la pause : le joueur choisit la qualité, la vue de caméra, le son, la disposition des commandes tactiles.

## Règles de gestion
- RG-01 : réglages : vue caméra (3), qualité graphique (basse / moyenne / haute), son (oui / non), disposition des commandes (droitier / gaucher), vibrations (oui / non).
- RG-02 : les choix sont appliqués tout de suite et mémorisés (sauvegarde simple, lue au démarrage).
- RG-03 : style du jeu (skill `design-street-racing`), boutons assez grands pour le doigt, dans les marges de sécurité.
- RG-04 : ouvrable depuis le menu principal et depuis la pause.

## À réutiliser du jeu web
`js/ui/screens.js` → `renderSettings`, boutons `#setCam`, `#setGfx`, `#setSnd` ; textes en français repris tels quels.

## Critères d'acceptation
- [ ] `reglages-menu`, `reglages-pause` ; un choix de qualité change bien le rendu ; banc `0 écart(s)`

## Hors périmètre
Détection automatique de la qualité (WFS-15).
