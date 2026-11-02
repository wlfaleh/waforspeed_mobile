# WFS-27 — HUD de course (vitesse, chrono, score, nitro en texte)
Épic : E5 · Priorité : 10 · Dépend de : WFS-26, WFS-12

## Besoin (qui, quoi, pourquoi)
Pendant la course, le joueur lit sa vitesse, son temps, son score, son checkpoint et sa jauge de nitro, avec le style
« street racing » nocturne.

## Règles de gestion
- RG-01 : vitesse = `|kmh|` entier + unité `KM/H` (`R · KM/H` en marche arrière) ; compteur en arc : départ 135° (`0,75π`), balayage 270°, échelle 0-380, graduation tous les 20, grand trait tous les 60, zone rouge au-delà de 300 (couleur `#FF1F4B`), dégradé `#00E5FF` → `#FF6A00`.
- RG-02 : solo : « Temps » (`timeLeft` 1 décimale, classe `low` rouge sous 8 s), « Score » (format `fr-FR`), « Checkpoint » = `level + 1` avec barre de progression `1 − (nextCp − dist)/CHECK_DIST`, « Combo ×N » si combo > 1.
- RG-03 : nitro : 10 cases (`round(nitro×10)`), panneau « actif » quand `nitroOn` ; bouton tactile avec jauge circulaire par pas de 2,5 %, grisé sous 2 %.
- RG-04 : notifications (`toast`) : « Checkpoint N +x s », « Quasi-contact ×N +pts », « Accident ! », « Combo perdu », nom du quartier.
- RG-05 : décompte central 3-2-1-GO (GO en orange).
- RG-06 : mise à jour seulement des valeurs qui changent ; style du skill `design-street-racing` ; marges de sécurité pour encoche et coins arrondis.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/ui/hud.js` → `updateHud`, `drawSpeedo`, `fmtTime` ; `js/ui/toasts.js` ; `js/ui/screens.js` → `showCount` ; `css/style.css` (couleurs, tailles).
- Pas de modèle 3D. Retour connu : HUD agressif mais lisible, aucune info cachée sous les pouces des commandes tactiles.

## Critères d'acceptation (captures du banc PC)
- [ ] `course-*` : compteur, chrono, score, checkpoint et jauge de nitro lisibles, à 2400x1080 et en 16:9
- [ ] capture avec nitro actif et une avec vitesse > 300 (arc rouge)
- [ ] capture du décompte « 3 » et « GO » ; accents corrects

## Hors périmètre
Classement du duel (WFS-21), menu et écran de fin détaillé (WFS-11), commandes tactiles (déjà en WFS-05).
