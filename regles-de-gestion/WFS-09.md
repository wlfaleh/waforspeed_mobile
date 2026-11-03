# WFS-09 — Nuit : vrais phares et éclairage public
Épic : E4 · Priorité : 9 · Dépend de : WFS-07

## Besoin (qui, quoi, pourquoi)
La nuit, la route doit être éclairée par les phares de la voiture et les lampadaires, avec les feux arrière/stop/recul
visibles, comme dans la version web (ambiance « ville néon »).

## Règles de gestion
- RG-01 : ambiance nuit (`envFor` nuit) : `headlight 2.4` (3 sur la campagne), `lamps: true` (sauf campagne), hémisphère `#46327a`/`#08080e` intensité 0,75, lune `#8a96ff` 0,35, brouillard `#120d1d` 40-540 m (420 max en qualité basse).
- RG-02 : phares du joueur : projecteur `0xfff1dd`, intensité = `ENV.headlight`, portée 170 m, angle 0,7, pénombre 0,65, placé à `nose − 0,12` m, cible 60 m devant à −0,5 m.
- RG-03 : tache de lumière sur la route (`BEAM`, longueur 110 m, largeur 26 m) : grille 17 rangées × 11 colonnes épousant la route (côtes, creux, virages) ; deuxième passage (≈ 4× au cœur) seulement quand il n'y a pas d'éclairage public.
- RG-04 : feux arrière allumés la nuit, plus forts au freinage (halo 0,38 contre 0,16), feux de recul blancs en marche arrière (lumière 2,2 la nuit) ; feux repérés par matériau, jamais un plastique partagé.
- RG-05 : lampadaires : au plus 4 lumières ponctuelles actives sur les plus proches (`0xffb46b`, intensité 1,1, portée 26 m) ; projecteurs de stade `0xeaf2ff` 1,8 / 60 m ; sinon matériaux émissifs + halo au sol (`MAT.pool`).
- RG-06 : nuit par carte : ville, plage, circuit éclairés ; campagne sans lampadaires (phares seuls).

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/cars/player.js` → `buildPlayer`, `buildBeam`, `updateBeam` ; `js/cars/models.js` (`BEAM`, repérage `tail`/`head` par matériau) ; `js/world/scene.js` → boucle « éclairage des lampadaires proches » ; `js/world/maps.js` → `envFor`.
- Modèles : `models/<voiture>/` avec matériaux feux (`tail` dans `CAR_MODELS`, ex. Volvo `lamp_*`, Gallardo `sans-feu-stop`).
- Retours connus : pas plus de 3 à 4 lumières actives (coût) ; lampadaires = émissif + halo ; un matériau n'est un feu que si toutes ses pièces le sont (sinon plastiques allumés).

## Critères d'acceptation (captures du banc PC)
- [ ] `course-*` de nuit : cône de lumière sur la chaussée, feux arrière rouges, flaques de lumière sous les lampadaires
- [ ] jour : aucun faisceau, feux éteints ; campagne de nuit : faisceau seul, sans lampadaire
- [ ] pas de point blanc parasite sur les carrosseries (WFS-32) ; coût GPU relevé dans le journal

## Hors périmètre
Route mouillée (WFS-16), bloom (WFS-17), réglages de qualité (WFS-15).


## Périmètre élargi (décision de l'utilisateur 2026-11-03 : tickets plus gros, moins de builds)
Ce ticket absorbe : WFS-16. **Lire et appliquer aussi** `regles-de-gestion/WFS-16.md` (toutes leurs règles et critères). Une seule branche, une seule CI à la fin ; livrer par morceaux sur la branche sans lancer de CI intermédiaire.
