# WFS-11 — Garage 3D : plateau tournant et choix de la voiture
Épic : E5 · Priorité : 0.5 · Dépend de : WFS-26 (✅), WFS-36

> Redécoupé le 2026-11-03 par le PO : le menu passe dans l'épic E8 (WFS-37, WFS-38), la fiche et les peintures dans WFS-40.
> Ce ticket ne garde que la scène 3D du garage et le changement de voiture.

## Besoin (qui, quoi, pourquoi)
Le joueur choisit sa voiture avant la course, dans un garage 3D (plateau tournant), écran `Garage` de WFS-36. Le garage ne propose que les voitures déjà converties ; les 20 viendront avec WFS-13.

## Règles de gestion
- RG-01 : garage = voiture sur plateau (cylindre rayon 3,6 m, hauteur 0,08), sol cercle rayon 30, fond `#07070c`, brouillard 14-40 m, anneau néon de la couleur `glow` de la voiture.
- RG-02 : lumières : hémisphère (0x5060a0 / 0x050508, 0,6) + 3 projecteurs (orange 0xff8a3a, cyan 0x40e0ff, blanc) ; caméra FOV 36, distance `1,9×L` (paysage), regard à `y = −0,35`.
- RG-03 : la voiture tourne seule à 0,35 rad/s ; glisser = tourner ; glissement rapide (< 320 ms, > 70 px) = voiture suivante/précédente avec glissement latéral de 7 m.
- RG-04 : nom de la voiture et index (« 01 / 04 ») affichés en haut ; flèches précédente / suivante (web `#carPrev`, `#carNext`) ; « Retour » → menu.
- RG-05 : fiche complète et peintures : WFS-40.
- RG-06 : voiture par défaut : Golf 8 GTI bleue ; pas de silhouette provisoire.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Code : `js/world/garage.js` → `buildGarage`, `setGarageCar`, `renderGarage` ; `js/cars/catalog.js` → `CARS`, `PAINTS`, `carStats`, `CAR_MODELS` (crédits) ; `js/ui/screens.js` (menu, garage) ; `css/style.css` (style).
- Modèles : `models/golf-8`, `bmw-m4`, `porsche-911`, `lambo-gallardo` (déjà en `.wfm`). Skill `design-street-racing` pour l'interface.
- Retour connu : jamais de voiture générée par le code dans la vitrine ; reflets de vernis sans points blancs (WFS-32).

## Critères d'acceptation (captures du banc PC)
- [ ] capture `garage-*` : voiture sur plateau, anneau néon, projecteurs, 3 angles
- [ ] changement de voiture visible entre deux captures (`garage-golf`, `garage-m4`), nom et index à jour
- [ ] lisible en 1280x720 et 2400x1080, accents corrects (texte WFS-26)

## Hors périmètre
Menu (WFS-37/38), fiche et peintures (WFS-40), les 16 autres voitures (WFS-13), sauvegarde du choix (WFS-24).
