# WFS-86 — [URGENT] Voitures en course aussi belles qu'au garage
Épic : E4 · Priorité : P1 Urgent · Dépend de : — · Taille : 1 jour · Retour de l'utilisateur (test réel sur téléphone, 2026-11-04)

## Besoin
Au garage les voitures sont « nickel ». Pendant la course, elles ne montrent pas la même qualité. On veut que la voiture du joueur et celles du trafic proche soient aussi nettes et belles qu'au garage.

## Règles de gestion (on veut que ça marche, pas la perfection)
- RG-01 : **d'abord trouver la cause** et la noter au Journal. Pistes à vérifier dans cet ordre : (1) qualité graphique réellement appliquée en course (Basse / Moyenne / Haute, WFS-15, détection automatique du téléphone) ; (2) échelle de rendu inférieure à 100 % en course ; (3) anticrénelage (MSAA) coupé ou réduit en course ; (4) ombres, reflets de l'environnement et bloom différents du garage ; (5) modèle simplifié (LOD) ou textures plus petites utilisés pour la voiture du joueur ; (6) éclairage (hémisphère, brouillard WFS-45) qui écrase les couleurs.
- RG-02 : la voiture du joueur utilise **toujours le modèle complet** et le même matériau qu'au garage ; seules les voitures du trafic éloignées (> 50 m) peuvent utiliser un modèle allégé.
- RG-03 : en qualité Moyenne et Haute, le rendu de la voiture du joueur en course est visuellement équivalent à celui du garage (même netteté des contours, mêmes reflets de vernis).
- RG-04 : en qualité Basse, on accepte une baisse (téléphone faible) mais la voiture du joueur reste nette (pas de pixels visibles) ; ne pas dégrader les images par seconde.
- RG-05 : le réglage de qualité du menu (WFS-41) change réellement ce rendu, et le téléphone de l'utilisateur doit passer en Moyenne ou Haute par défaut s'il tient 30 images/s.

## À réutiliser
Le rendu du garage (WFS-11) sert de référence ; jeu web : `js/world/renderer.js`, `js/cars/models.js` (voiture complète à moins de 50 m, `LOD_CAR_M`).

## Critères d'acceptation (captures du banc PC)
- [ ] capture « course » vs « garage » de la même voiture : même netteté et mêmes reflets (qualité Haute) ; cause documentée au Journal

## Hors périmètre
Nouveaux effets (WFS-65), lumières de nuit (WFS-84).
