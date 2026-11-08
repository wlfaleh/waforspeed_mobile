# Recette manuelle — APK 0.1.916 et 0.1.918 (retours de l'utilisateur du 2026-11-08)

- **QA** : agent qa lead. Demande du PO (pour l'utilisateur), `qa/recette-manuelle-apk918.md`.
- **Matériel regardé** : les 16 images, toutes ouvertes : 4 planches de vidéos (`2026-11-08-apk918-video1` à `-video4-planche.jpg`), captures `2026-11-08-apk918-1` à `-5.jpg` et `2026-11-08-apk916-1` à `-7.jpg`.
- **Note** : les planches de vidéos sont trop petites pour lire les chiffres du bas des Réglages (vidéos 3 et 4) ; les mesures viennent des captures en pleine taille.

## Verdict par point

| # | Point | Verdict | Preuve | Constat |
|---|---|---|---|---|
| 1 | Départ de nuit blanc / rose (ANO-177 → WFS-328) | **KO** | `apk918-2.jpg`, `apk918-video2-planche.jpg` | APK 918, Ville Néon de nuit : toute l'image est blanche / rose délavée **dès le compte à rebours (3, 2, 1)** et jusqu'à 0:02,6 ; normale à 0:03,2. Sur l'APK 916, le départ de nuit est correct (`apk916-6.jpg`, gris-bleu normal au « 1 ») : **régression entre 916 et 918**. Seule la Ville de nuit est filmée : autres cartes et « Recommencer » non montrés (à vérifier par WFS-328). |
| 2 | Fenêtres lumineuses hors des façades (ANO-175 → WFS-326) | **KO** | `apk916-5.jpg`, `apk916-2.jpg`, `apk916-3.jpg`, `apk918-1.jpg`, `apk918-3.jpg`, `apk918-video1-planche.jpg` | Toujours dans l'APK 918. Ce sont les **façades les plus proches de la caméra** (≈ 5 à 20 m), **des deux côtés** de la rue : de grands rectangles clairs à bord rouge, inclinés, décollés du mur, qui flottent au-dessus du trottoir (à droite sur `apk916-5`, à gauche sur `apk916-2` et `apk918-3`, en pointes de triangle à droite en vue Cockpit sur `apk916-3`). Plus loin (> 50 m), les fenêtres restent bien sur les façades. Image de pause de la vidéo 1 : les mêmes quadrilatères dispersés dans le noir. |
| 3 | Course à 5 : joueur toujours dernier (ANO-176 → WFS-327) | **KO** | `apk916-2.jpg`, `apk916-3.jpg`, `apk916-5.jpg`, `apk918-1.jpg`, `apk918-3.jpg` | 5/5 sur toutes les captures en course. À 1:54,6, les 4 adversaires sont « Arrivé » alors que le joueur est à 5,86 km sur 6 à 236 km/h (`apk916-2`). À 0:09,6 il a déjà 0,11 à 0,19 km de retard (`apk918-1`). La place au départ varie (3/5 sur `apk916-6`, 5/5 sur `apk918-2`) : c'est la vitesse des adversaires qui pèche. |
| 4 | Performance lue dans les Réglages | **KO** | `apk916-1.jpg`, `apk916-4.jpg`, `apk918-4.jpg` | APK 916, Ville de nuit, Basse 60 % : **10 img/s** (372 appels, 439 k triangles : décor 32 %, voitures 41 %, personnes 27 % ; médiane 99,5 ms, scène GPU 88,7 ms) puis **20 img/s** (225 appels, 212 k triangles ; médiane 75,3 ms). APK 918, **menu principal sans partie**, Basse 75 % : **10 img/s**, 462 appels, 699 k triangles (voitures 52 %, personnes 30 %) → **ANO-178 (nouvelle)**. Objectif ≥ 25 img/s (WFS-299) et ≥ 30 img/s (WFS-317) : non atteint. Mémoire : jeu 53 à 79 Mo, carte graphique 821 à 946 Mo. |
| 5 | Aide à la direction (activée) | **OK** | `apk918-video4-planche.jpg`, `apk918-video1-planche.jpg`, `apk918-video3-planche.jpg` | Aide activée (Réglages). Vidéo 4 : voie rapide et pont, 215 → 284 km/h, la voiture reste dans sa voie sans écart. Vidéos 1 et 3 : virages de la Ville à 100-160 km/h passés sans sortie de route. À noter : aucune épingle ni rond-point serré filmé ici (ANO-172 / WFS-316 concerne l'aide désactivée). |
| 6 | Foule en Ville | **OK (dense)** | `apk918-video3-planche.jpg`, `apk918-1.jpg`, `apk918-3.jpg` | Piétons nombreux sur les deux trottoirs, groupes devant les terrasses, rien sur la chaussée. Elle coûte cher : 27 à 30 % des triangles (`apk916-1`, `apk918-4`) ; le −70 % de WFS-317 n'est pas dans l'APK 918. |
| 7 | Vue Cockpit de nuit (seulement noter) | **À REVOIR** | `apk916-3.jpg` | Planche de bord en aplat noir sans détail, volant noir plat, montant gauche épais en diagonale qui coupe le haut de l'écran, compteur peu lisible (« 269 » flou). Noté pour WFS-323 à 325 ; l'utilisateur validera après. |
| 8 | Écran de chargement | **OK** | `apk918-5.jpg`, `apk916-7.jpg` | Version lisible (v0.1.918, v0.1.916), étape « Construction de la ville 70 % », données lues (56 Mo, 14 objets 3D) et mémoire (jeu 207 / 168 Mo, carte graphique 146 / 122 Mo, partie 1) lisibles. |
| 9 | Menus et Réglages | **OK** | `apk918-video2-planche.jpg`, `apk918-4.jpg`, `apk916-1.jpg` | Choix du mode (Course à 5, 6 km, Difficile), garage (Giuliva Trefoil), choix du terrain (Ville Néon, Nuit), départ, pause (Reprendre, Recommencer, Quitter la course), Réglages (en course et depuis le menu) : rien de cassé, « FERMER » en haut à droite. |
| 10 | Autres défauts | voir ANO-178 | `apk918-4.jpg` | Menu principal à 10 img/s (ANO-178, majeure). Rien d'autre de nouveau : pluie, GPS (« Virage serré à droite », « Carrefour à feux, tout droit ») et HUD corrects. |

## Anomalies

| ANO | Gravité | Statut |
|---|---|---|
| ANO-175 (fenêtres hors des façades) | Bloquante | toujours présente dans 918 (WFS-326) |
| ANO-176 (toujours dernier) | Bloquante | toujours présente (WFS-327) |
| ANO-177 (départ de nuit délavé) | Bloquante | présente dans 918, pas dans 916 : régression (WFS-328) |
| **ANO-178** (menu principal à 10 img/s) | **Majeure** | 🆕 nouvelle, à rattacher à WFS-317 |

## Verdict global

**KO** : 3 bloquantes toujours présentes (ANO-175, 176, 177) et performance sous l'objectif (en course et au menu).
