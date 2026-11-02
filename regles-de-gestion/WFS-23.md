# WFS-23 — Manettes de jeu
Épic : E7 · Priorité : 23 · Dépend de : WFS-29

## Besoin (qui, quoi, pourquoi)
Le jeu se joue à la manette Bluetooth sur Android : conduite, nitro, caméra, navigation dans les menus.

## Règles de gestion
- RG-01 : actions du web (`KEYMAP`) : haut = accélérer, bas = freiner / marche arrière, gauche/droite = braquer, nitro = espace ; caméra = changer de vue (3 vues) ; pause.
- RG-02 : correspondance manette : gâchette droite = accélérer, gâchette gauche = freiner, stick gauche (axe X) ou croix = braquer, bouton A/Croix = nitro, Y/Triangle = caméra, Start = pause ; zone morte 0,12 ; le braquage reste progressif (taux `steerIn 4`, `steerOut 6`).
- RG-03 : la manette remplace les commandes tactiles dès qu'une entrée est reçue (masquer les boutons tactiles) et inversement au prochain toucher.
- RG-04 : menus : croix/stick pour se déplacer, A valider, B retour, focus visible.
- RG-05 : connexion/déconnexion à chaud : pause automatique si la manette part en course.
- RG-06 : aucun changement du noyau de conduite : les entrées alimentent le même `input` (up, down, left, right, nitro) que le clavier.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/ui/screens.js` → `KEYMAP` et gestion tactile ; `js/game/update.js` (braquage progressif) ; `js/core/config.js` (`PHYS.steerIn/steerOut`).
- Pas de modèle. Le web n'a pas de manette : ticket natif, comportement de conduite inchangé.

## Critères d'acceptation
- [ ] test CPU : événements de manette simulés → états `input` attendus (zone morte, gâchettes)
- [ ] journal du banc : entrées injectées reproduisent la course automatique
- [ ] capture : menu avec focus visible ; aucun bouton tactile affiché en mode manette (vérifié au simulateur de fin d'épic E7)

## Hors périmètre
Vibration de la manette, remappage utilisateur, claviers.
