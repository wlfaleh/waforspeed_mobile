# Recette manuelle — APK 0.1.928 (vidéos de l'utilisateur, 2026-11-08 ≈ 22:01-22:07 UTC)

- **QA** : agent qa lead. Demande du PO (pour l'utilisateur), `qa/recette-manuelle-apk928.md`.
- **APK** : v0.1.928, construction 928 (`b5d8ddb`). WFS-328 (départ délavé) est encore **en PR (#282)** : il n'est pas dans cet APK. WFS-290 (boutons) est en cours, WFS-327 (Course à 5) est dans l'APK.
- **Matériel** : les 6 images, toutes ouvertes (5 planches de vidéos et l'écran de chargement).
- **Limite** : les planches sont petites ; les chiffres du bas des Réglages (img/s, triangles) et le classement de la Course à 5 n'y sont **pas lisibles**. Une capture d'écran en pleine taille des Réglages suffirait.

## Verdict par point

| # | Point | Verdict | Preuve | Constat |
|---|---|---|---|---|
| 1 | Image blanche / rose | **KO** | `apk928-video6-planche.jpg`, `apk928-video1-planche.jpg` | **Départ** (vidéo 6, Course à 5, vue Cockpit, Ville de nuit) : délavée de 0:01,3 à 0:05,3, normale à 0:07,0. Même défaut que ANO-177, attendu puisque WFS-328 n'est pas dans l'APK ; il dure jusqu'à 5 s et touche aussi la vue Cockpit. **Nouveau cas** (vidéo 1, Checkpoint, vue Arrière) : délavée **en pleine course**, de 239,8 à 256,9 s restants, checkpoints 12 et 13 → **ANO-179, bloquante**. Même aspect (sol et murs blancs-roses, ciel noir), donc probablement la même cause, mais pas limitée au départ : WFS-328 doit couvrir les deux cas. |
| 2 | Boutons Réglages / Fermer / Reprendre | **À REVOIR** | `apk928-video3-planche.jpg` | Les allers-retours Réglages ↔ Pause s'enchaînent et « Reprendre » relance bien la course (cockpit, 53 s). Mais après un appui sur « FERMER » (1re image), les Réglages restent affichés sur les 4 images suivantes (≈ 4 s) : soit l'appui n'a pas été pris, soit l'utilisateur attendait. La planche ne permet pas de trancher ; cohérent avec ANO-159 / WFS-290, en cours. |
| 3 | Lien « Conditions d'utilisation » | **OK** | `apk928-video1-planche.jpg`, `-video3`, `-video4` | Lien « CONDITIONS D'UTILISATION » en cyan, en bas à droite des Réglages, sur la ligne « Rendu » ; aucun bouton déplacé. |
| 4 | Vue Cockpit de nuit (noter seulement) | **À REVOIR** | `apk928-video4-planche.jpg`, `-video5`, `-video6` | La rue se voit bien (Ville et voie rapide). Planche de bord noire, avec des aérateurs dessinés en simples traits blancs et une ligne rouge sur toute la largeur ; volant gris plat ; montants épais. Pas de compteur sur la planche : la vitesse s'affiche en haut à gauche. Noté pour WFS-323 à 325. |
| 5 | Course à 5 | **À REVOIR** | `apk928-video6-planche.jpg` | Départ en cockpit, adversaires visibles devant ; classement illisible sur la planche (place du joueur non vérifiable). WFS-327 est déjà validé par l'utilisateur. |
| 6 | Aide à la direction | **OK** | `apk928-video5-planche.jpg`, `-video4-planche.jpg` | Voie rapide, tunnel et pont de 162 à 233 km/h : la voiture reste dans sa voie. Ville de 8 à 74 km/h, virages serrés « à droite » : pas de sortie de route. |
| 7 | Performance | **À REVOIR** | Réglages des vidéos 1, 3, 4, 5 et 6 | Chiffres présents en bas des Réglages mais illisibles sur les planches : pas de comparaison possible avec l'APK 918. |
| 8 | Foule en Ville et menu fluide | **Non visible** | — | Aucune image du menu principal ; foule peu visible en vue Cockpit de nuit. |
| 9 | Écran de chargement | **OK** | `apk928-chargement.jpg` | v0.1.928, « Construction de la ville 70 % », 55 Mo lus, mémoire jeu 212 Mo, carte graphique 146 Mo, partie 1 : tout est lisible. |
| 10 | Autres défauts | voir les points 11 et 12 | — | Rien d'autre sur les planches réduites. Les points 11 et 12 ont été vus en grand, à la demande du PO. |
| 11 | Voiture rouge du trafic de travers à côté du joueur (point ajouté par le PO) | **KO** (confirmé) | `2026-11-08-apk928-video1-voiture-de-travers.jpg` | De 232,9 à 229,5 s, 80 à 129 km/h : une voiture rouge du trafic avance **en travers**, à moitié sur le trottoir de droite, collée à la voiture du joueur et à sa vitesse, pendant 3,4 s. Elle quitte l'image à 229,0 s. → ANO-180 / WFS-333 (créés par le PO). |
| 12 | Voiture noire dans l'habitacle en vue Cockpit (point ajouté par le PO) | **Probable, à confirmer** | `2026-11-08-apk928-video4-cockpit-voiture.jpg` | À 40,4 s, la roue avant et l'aile d'une voiture noire qui double par la gauche sont dessinées sous la ligne de la planche de bord, à la place de la portière ; à 40,0 s, elle est devant à gauche. Soit la portière n'est pas dessinée, soit la voiture traverse celle du joueur → **ANO-181** (mineure, à confirmer). |

## Anomalies

| ANO | Gravité | Statut |
|---|---|---|
| ANO-177 (départ délavé) | Bloquante | toujours présente dans 928 (WFS-328 en PR #282) ; dure jusqu'à 5 s, vue Cockpit comprise |
| **ANO-179** (image délavée en pleine course, Checkpoint) | **Bloquante** | 🆕 nouvelle, à rattacher à WFS-328 |
| ANO-159 (boutons) | — | à revoir (WFS-290 en cours) |
| ANO-180 (voiture de travers) | Bloquante | confirmée (WFS-333) |
| **ANO-181** (voiture visible dans la portière, cockpit) | **Mineure**, à confirmer | 🆕 nouvelle |

## Leçon pour les prochaines recettes

Regarder chaque vidéo aussi en grand (planches de 640 px au moins par image), pas seulement les planches réduites : le point 11 était invisible à petite taille.

## Verdict global

**KO** : l'image délavée est toujours là au départ, et apparaît aussi en pleine course (ANO-179). Pour la prochaine recette, il faudrait une capture d'écran pleine taille des Réglages (chiffres de performance) et du classement de la Course à 5.
