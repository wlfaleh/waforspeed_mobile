# WFS-61 — Point d'extension : transformation par roue
Épic : E0 · Priorité : 17.7 (hors MVP) · Dépend de : WFS-60 · Taille : ≈ 2 h

## Besoin (qui, quoi, pourquoi)
Pour animer les roues sans toucher au noyau à chaque fois, le rendu doit accepter **une transformation (rotation) par roue** d'une voiture. Ticket noyau minimal : une accroche, pas de logique de jeu.

## Règles de gestion
- RG-01 : l'API du rendu permet de dessiner une voiture avec, pour chacune des 4 roues, un **angle de rotation** (roulage) et, pour les 2 roues avant, un **angle de braquage**.
- RG-02 : valeurs par défaut = 0 : sans appel, la voiture est dessinée comme aujourd'hui (captures inchangées).
- RG-03 : la rotation se fait autour du centre de la roue (données de WFS-60) ; le braquage tourne autour de la verticale passant par ce centre.
- RG-04 : aucun appel de dessin supplémentaire par voiture au-delà de 4 (une par roue) ; le trafic lointain peut ne pas utiliser l'accroche.
- RG-05 : modification limitée à l'API du rendu et à l'appel de dessin d'une voiture (≤ 10 lignes d'accroche ailleurs, annoncées au Journal).

## À réutiliser du jeu web
`js/cars/models.js` → `animWheels(steerAngle, meters)` (structure : un groupe « braquage » et un groupe « rotation » par roue).

## Critères d'acceptation
- [ ] Test de rendu : voiture avec roues à 45° de rotation / 20° de braquage visibles sur une capture de test ; sans appel, capture identique à avant ; banc `0 écart(s)`

## Hors périmètre
Calcul des angles (WFS-62), flou de roue, suspension / roulis.


## Périmètre élargi (décision de l'utilisateur 2026-11-03 : tickets plus gros, moins de builds)
Ce ticket absorbe : WFS-62. **Lire et appliquer aussi** `regles-de-gestion/WFS-62.md` (toutes leurs règles et critères). Une seule branche, une seule CI à la fin ; livrer par morceaux sur la branche sans lancer de CI intermédiaire.
