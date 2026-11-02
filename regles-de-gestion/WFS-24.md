# WFS-24 — Sauvegarde et progression
Épic : E3 · Priorité : 21 · Dépend de : WFS-12

## Besoin (qui, quoi, pourquoi)
Le joueur retrouve ses meilleurs scores, sa voiture, ses peintures et ses réglages à chaque lancement.

## Règles de gestion
- RG-01 : données sauvegardées (clés du web) : meilleur score (`waforspeed-best`, entier), voiture (`car`), peinture par voiture (`paint-<id>`), carte (`map`), moment (`tod` : jour/nuit, nuit par défaut), qualité (`quality`), caméra (`cam`, 0-2), son coupé (`muted`), pseudo (`nick`, 14 caractères max), distance de duel (`km`, parmi 2, 4, 6, 10), mode de braquage (`steer`).
- RG-02 : valeurs par défaut du web : voiture `golf`, carte `ville`, nuit, qualité haute, caméra proche, pseudo « Pilote », 4 km.
- RG-03 : meilleur score écrit seulement s'il est battu (`s > best`) à la fin de la course ; l'écran de fin affiche « Nouveau record ».
- RG-04 : valeur inconnue ou fichier absent/corrompu : retour au défaut sans plantage ; identifiants validés (voiture existante, peinture existante).
- RG-05 : fichier privé de l'application (Android `filesDir`), écriture atomique (fichier temporaire puis renommage), version de format pour les évolutions.
- RG-06 : écriture hors de la boucle de rendu (thread de travail) ; sauvegarde à la fin de course et au passage en arrière-plan.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/game/state.js` (`best`) ; `js/ui/screens.js` (`store(...)`, `endRace`, `setQuality`) ; `js/cars/catalog.js` (`myCar`, `myPaint`) ; `js/world/maps.js` (`setMap`) ; `js/net/duel.js` (`nick`, `duelKm`).
- Pas de modèle. Retour connu : le stockage du web peut échouer ou être vide : le jeu doit toujours démarrer.

## Critères d'acceptation
- [ ] test CPU : écrire puis relire toutes les clés ; fichier corrompu → défauts ; identifiant inconnu → défaut
- [ ] journal du banc : deux lancements successifs, le second retrouve voiture, peinture et record du premier
- [ ] capture : écran de fin « Nouveau record » avec le meilleur score (texte WFS-26)

## Hors périmètre
Compte en ligne, synchronisation entre appareils, succès.
