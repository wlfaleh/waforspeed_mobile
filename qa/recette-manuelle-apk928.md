# Recette manuelle — APK 0.1.928 (vidéos de l'utilisateur, 2026-11-08 ≈ 22:01-22:07 UTC)

Demandée par l'utilisateur le 2026-11-08 22:09:49 UTC (« regarde et envoie au QA, test manuel »). Préparée par le PO. Recette par l'agent `qa-lead` : ouvrir **chaque** planche,
verdict OK / KO / À REVOIR par point avec l'image qui le prouve, rapport `qa/rapports/recette-manuelle-apk928.md`, nouvelles anomalies dans `anno.md`.
(L'utilisateur a envoyé 6 vidéos ; deux sont identiques : 5 planches.)

## Matériel (`qa/retours/`)
| Fichier | Contenu vu par le PO |
|---|---|
| `2026-11-08-apk928-video1-planche.jpg` | Réglages (lien « Conditions d'utilisation »), puis Checkpoint, vue Arrière, **à 230-260 s de course** : toute l'image est **blanche / rose délavée** (ciel noir, sol et immeubles blancs) — pas seulement au départ |
| `2026-11-08-apk928-video3-planche.jpg` | allers-retours Réglages ↔ Pause (boutons Réglages / Fermer / Reprendre) puis cockpit de nuit |
| `2026-11-08-apk928-video4-planche.jpg` | Réglages → Pause → cockpit de nuit en Ville, 8 à 74 km/h, checkpoint 1 |
| `2026-11-08-apk928-video5-planche.jpg` | Réglages → Pause → cockpit de nuit, voie rapide, 162 à 233 km/h, tunnel / pont |
| `2026-11-08-apk928-video6-planche.jpg` | Réglages → départ Course à 5 en cockpit : **0:01 à 0:05 blanc délavé**, puis nuit normale |
| `2026-11-08-apk928-chargement.jpg` | écran de chargement v0.1.928 (55 Mo lus, mémoire jeu 212 Mo, carte graphique 146 Mo) |

## Points à vérifier
1. **Image blanche / rose** : au départ (ANO-177 / WFS-328, video6) **et en pleine course** (video1, 230-260 s, vue Arrière, Checkpoint) : nouveau cas ou même défaut ? Mode, vue, heure.
2. **Boutons Réglages / Fermer / Reprendre** (video3, nombreux appuis) : chaque appui répond-il (WFS-290 en cours) ?
3. **Lien « Conditions d'utilisation »** dans les Réglages (WFS-308) : visible, lisible, ne déplace aucun bouton.
4. **Vue Cockpit de nuit** (videos 3 à 6) : planche, volant, montants, rue visible (WFS-282 / 315 ; l'utilisateur validera après WFS-323 à 325 : noter seulement).
5. **Course à 5** (video6) : place du joueur, adversaires (WFS-327 fusionné).
6. **Aide à la direction** à haute vitesse (video5, 162-233 km/h) et en Ville (video4).
7. **Performance** : chiffres « Rendu / Images » lisibles sur les Réglages (img/s, triangles, mémoire) ; comparer à l'APK 918.
8. **Foule en Ville** (−70 %, WFS-317) et **menu fluide** (WFS-329) si visibles.
9. **Écran de chargement** v0.1.928.
10. **Tout autre défaut visible.**
11. **Ajout du PO (22:12 UTC)** : voiture rouge collée au joueur, de travers, à sa vitesse (video1, 232,9 → 229,5 s) → ANO-180 / WFS-333 ; voiture noire qui semble entrer dans l'habitacle (video4, 40,4 s) : à confirmer.
