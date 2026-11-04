# WFS-75 — Réglages : commandes gaucher et vibrations
Épic : E8 (écrans) + accroches E0 / E3 · Priorité : P3 Moyenne · Dépend de : WFS-41 ✅, WFS-70 (direction et vibration)

## Besoin (qui, quoi, pourquoi)
L'écran Réglages (WFS-41 / WFS-68) prévoit « Commandes : droitier / gaucher » et « Vibrations : oui / non », mais les deux
sont absents : il n'y avait ni vibreur Android ni moyen d'inverser les commandes. WFS-70 apporte la vibration Android
(`src/platform/vibration_android.cpp`) et déplace le volant dans `src/game/direction.cpp`. Ce ticket ajoute les deux réglages
et les fait marcher en course.

## Règles de gestion
- RG-01 : réglage « Commandes » : **Droitier** (défaut, disposition actuelle) / **Gaucher**.
- RG-02 : gaucher = miroir horizontal des commandes tactiles : volant (et ses modes de WFS-70) à droite, gaz, frein et nitro à
  gauche, à la même hauteur et à la même taille ; les zones de toucher suivent exactement le dessin ; marges de sécurité
  (WFS-51) échangées gauche / droite. Le HUD (WFS-27, WFS-52) ne bouge pas.
- RG-03 : réglage « Vibrations » : **Activées** (défaut) / **Coupées** ; coupées → aucune vibration (journal
  `vibration ignorée (réglage)`), y compris les motifs de WFS-70 `[90, 40, 120]` ms et 40 ms.
- RG-04 : changement appliqué tout de suite (sans redémarrer, même depuis la pause) et enregistré avec les autres réglages
  (fichier de WFS-41 ; sauvegarde WFS-24 si elle est fusionnée), relu au démarrage.
- RG-05 (zone) : `src/ui/ecrans/reglages.*` (E8) ; le miroir se fait en un seul point (fonction `miroirX` appliquée au dessin et
  au toucher) dans `src/game/game.cpp` et `src/game/direction.cpp`, accroches ≤ 10 lignes chacune annoncées au Journal ;
  l'interrupteur de vibration dans `src/game/ressenti.cpp` (≤ 5 lignes).

## À réutiliser
- Natif : panneau de réglages existant (`reglages.cpp`, mêmes boutons à choix, même enregistrement) ; `vibrer()` de WFS-70.
- Jeu web (`wlfaleh/waaps`, `waforspeed/`) : vibration `navigator.vibrate` dans `js/game/update.js` (motifs déjà portés par
  WFS-70) ; réglages `js/ui/screens.js` (`renderSettings`) pour le style des choix. **Pas de disposition gaucher dans le jeu
  web** : rien à réutiliser, simple miroir des positions natives.

## Critères d'acceptation (captures du banc PC)
- [ ] `reglages-commandes` : les deux nouvelles lignes « Commandes » et « Vibrations » lisibles, dans la zone sûre
- [ ] `course-gaucher` : volant à droite, pédales et nitro à gauche, rien sous le HUD ; appui simulé à gauche = gaz (journal)
- [ ] vibrations coupées : choc du banc → `vibration ignorée (réglage)` au journal ; captures droitier inchangées

## Hors périmètre
- Personnalisation libre de la position des commandes ; intensité de vibration.
