# WFS-11 — Menu et garage 3D (voiture + couleur)
Épic : E5 · Priorité : 8 · Dépend de : WFS-26 (🔧)

## Besoin (qui, quoi, pourquoi)
Le joueur choisit sa voiture et sa peinture avant la course, dans un garage 3D (plateau tournant) et un menu
au style street racing. Le garage ne propose que les voitures déjà converties ; les 20 viendront avec WFS-13.

## Règles de gestion
- RG-01 : garage = voiture sur plateau (cylindre rayon 3,6 m, hauteur 0,08), sol cercle rayon 30, fond `#07070c`, brouillard 14-40 m, anneau néon de la couleur `glow` de la voiture.
- RG-02 : lumières : hémisphère (0x5060a0 / 0x050508, 0,6) + 3 projecteurs (orange 0xff8a3a, cyan 0x40e0ff, blanc) ; caméra FOV 36, distance `1,9×L` (paysage), regard à `y = −0,35`.
- RG-03 : la voiture tourne seule à 0,35 rad/s ; glisser = tourner ; glissement rapide (< 320 ms, > 70 px) = voiture suivante/précédente avec glissement latéral de 7 m.
- RG-04 : 16 peintures (`PAINTS`) ; la peinture choisie est mémorisée par voiture ; carrosserie seule repeinte (vitres, chrome, pneus intacts).
- RG-05 : fiche affichée : marque, modèle, type, `tag`, 4 barres de stats (`carStats` : vitesse, accélération, maniabilité, puissance sur 10) et crédit du modèle (licence obligatoire).
- RG-06 : voiture par défaut : Golf 8 GTI bleue ; pas de silhouette provisoire.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/world/garage.js` → `buildGarage`, `setGarageCar`, `renderGarage` ; `js/cars/catalog.js` → `CARS`, `PAINTS`, `carStats`, `CAR_MODELS` (crédits) ; `js/ui/screens.js` (menu, garage) ; `css/style.css` (style).
- Modèles : `models/golf-8`, `bmw-m4`, `porsche-911`, `lambo-gallardo` (déjà en `.wfm`). Skill `design-street-racing` pour l'interface.
- Retour connu : jamais de voiture générée par le code dans la vitrine ; reflets de vernis sans points blancs (WFS-32).

## Critères d'acceptation (captures du banc PC)
- [ ] capture `garage-*` : voiture sur plateau, anneau néon, 3 angles, couleur appliquée
- [ ] capture menu : titre, boutons, lisible sur 2400x1080 et petit écran, accents corrects (texte WFS-26)
- [ ] changement de voiture et de peinture visible entre deux captures

## Hors périmètre
Les 16 autres voitures (WFS-13), cartes (WFS-14), sauvegarde du choix (WFS-24).
