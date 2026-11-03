# WFS-47 — [MVP] Notifications de course (checkpoint, quasi-contact, accident)
Épic : E5 · Priorité : 0.42 (MVP du 2026-11-03) · Dépend de : WFS-27 · Taille : ≈ 2 h

## Besoin (qui, quoi, pourquoi)
Le joueur doit **sentir** qu'il passe un checkpoint (+x s) ou frôle une voiture (+points) : c'est la boucle de jeu du mode
solo. Les bandeaux (« toasts ») du jeu web le disent à droite de l'écran.

## Règles de gestion
- RG-01 : dans le système du HUD (`src/ui/hud.*`, WFS-27) ou un fichier voisin `src/ui/toasts.*`. **Aucune modification de
  `src/game/`** : les évènements sont **déduits des compteurs** de `Course` d'une image à l'autre (`level`, `nearCount`, `combo`,
  `accidents`), lus par `activeCourse()`.
- RG-02 : `level` augmente → « CHECKPOINT N » + « +x,x s » avec `x = Course::checkpointBonus(N)` (1 décimale, virgule) ;
  genre `cp` (bordure et valeur orange `#FF6A00`).
- RG-03 : `nearCount` augmente → « QUASI-CONTACT » (ou « QUASI-CONTACT ×N » si `combo > 1`) + « +250×combo » ; genre normal
  (bordure et valeur cyan `#00E5FF`).
- RG-04 : `accidents` augmente → « ACCIDENT ! » ; `combo` passe de > 1 à 0 sans accident → « COMBO PERDU » ; genre `bad`
  (bordure et valeur rouge `#FF1F4B`).
- RG-05 : pile à droite, sous le score (web `.toasts` : `top = gouttière + 110 px`), au-dessus du bouton Nitro sans le
  recouvrir ; **3 bandeaux au plus**, le plus récent en haut ; chacun vit **1,9 s** : entrée 240 ms (glisse de +60 px à 0,
  opacité 0 → 1), sortie à 1,5 s en 260 ms (vers +40 px, opacité → 0).
- RG-06 : bandeau = fond vitré `rgba(20,22,31,.72)`, bordure gauche 4 px de la couleur du genre, titre 18 px capitales,
  valeur 14 px ; incliné de −10° si WFS-48 est livré, sinon droit (pas de bricolage local d'italique).
- RG-07 : remise à zéro quand une nouvelle course démarre (Rejouer) : pas de bandeau fantôme.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/ui/toasts.js` → `toast(title, value, kind)` (pile de 3, 1,9 s) ; textes exacts dans `js/game/update.js`
  l. 38 (« Accident ! »), l. 46 (« Combo perdu »), l. 231 (quasi-contact), l. 257 (checkpoint) ; `css/style.css` l. 90-107.
- Natif : `src/ui/text.h`, `src/ui/hud.*` (WFS-27), `Course::checkpointBonus`.
- Hors MVP : le nom du quartier (`zoneName`, l. 240) n'existe pas encore en natif : ne pas l'inventer.

## Critères d'acceptation (captures du banc PC)
- [ ] `toast-checkpoint` : « CHECKPOINT 1 » « +11,0 s » à droite, sous le score
- [ ] `toast-pile` : 3 bandeaux empilés (quasi-contact cyan, checkpoint orange, accident rouge), aucun sur les commandes
- [ ] banc `0 écart(s)`, autres captures inchangées

## Hors périmètre
Nom du quartier, sons associés (WFS-28), duel (« X a quitté »).
