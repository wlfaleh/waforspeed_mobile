# WFS-21 — Duel en ligne
Épic : E3 · Priorité : 22 · Dépend de : WFS-12

## Besoin (qui, quoi, pourquoi)
Deux joueurs (ou plus) courent sur la même route et le même trafic pour une distance choisie ; chacun voit les autres
voitures, le classement et le résultat.

## Règles de gestion
- RG-01 : salon : un créateur choisit carte et distance (`DUEL_KMS = 2, 4, 6, 10` km), un code de salon (alphabet `abcdefghjkmnpqrstuvwxyz23456789`), pseudos ≤ 14 caractères, statuts prêt / course / arrivé ; départ quand tous sont prêts.
- RG-02 : même graine pour tous : `hashStr(codeSalon) + round×977` ; trafic identique chez tous (`trafficSeed`), horloge de course commune : `trafficClock = (maintenant − départ)/1000 − 3`.
- RG-03 : présence envoyée toutes les 0,1 s : distance `d` (arrondie, ≤ `FINISH`), écart latéral `x` (3 décimales), vitesse `v`, cap `h` (3 décimales), frein `b` (0, 1, 2) ; ligne d'arrivée `FINISH = km × (1000/0.006)` ; temps d'arrivée en ms.
- RG-04 : fantômes : position estimée `d + v × min(0,5 s, âge)` ; lissage `dt×8` (distance), `dt×10` (latéral) ; téléportation si écart > 4000 unités ; valeurs bornées (`x` ∈ [−2, 2], `v` ∈ [0, 1,6×MAX_SPEED]) ; nom, voiture et peinture validés.
- RG-05 : contact entre voitures : chaque joueur calcule sa part, effet symétrique ; côte à côte = frottement et séparation progressive (≤ `dt×2,5+0,004`), arrière = recul et perte de vitesse selon la masse relative (`kg/1450`).
- RG-06 : classement en direct (barres de distance, « Position n/N », « Arrivé ») ; écran de résultat avec temps, revanche.
- RG-07 : transport réseau natif à définir dans ce ticket (le web utilise une « room » d'artifact claude.ai ou PeerJS) ; protocole et messages ci-dessus conservés ; données reçues jamais crues (bornées, nettoyées).

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/net/duel.js` (`startDuelRace`, `sendRacePresence`, `updateGhosts`, `finishDuel`, `renderLobby`) ; `js/net/p2p.js` ; `js/game/update.js` → `hitGhosts` ; `js/ui/hud.js` → `renderBoard` ; `js/world/scene.js` (rivaux).
- Modèles : voitures `models/<voiture>/` (WFS-13 pour toutes) ; nom au-dessus de la voiture adverse (`nameSprite`).

## Critères d'acceptation (journal et captures du banc PC)
- [ ] test CPU : deux instances simulées avec même graine → trafic identique image par image
- [ ] test de messages hostiles (valeurs hors bornes, texte long) : pas de plantage
- [ ] capture : voiture adverse sur la route avec son nom, classement à l'écran

## Hors périmètre
Serveur de matchmaking public, chat, anti-triche avancé.
