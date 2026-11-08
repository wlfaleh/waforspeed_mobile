# Recette manuelle — APK 0.1.916 et 0.1.918 (retours de l'utilisateur du 2026-11-08)

Demandée par l'utilisateur le 2026-11-08 17:44:52 UTC (« regarde ce que tu as regardé et envoie-le au QA pour une recette manuelle »). Préparée par le PO. Recette par l'agent `qa-lead` :
regarder **toutes** les images ci-dessous, vérifier chaque point, rendre un rapport `qa/rapports/recette-manuelle-apk918.md` (verdict par point, nouvelles anomalies dans `anno.md`).

## Matériel (tout dans `qa/retours/`)
| Fichier | Contenu |
|---|---|
| `2026-11-08-apk918-video1-planche.jpg` | Ville de nuit, Course à 5, vue Arrière, 0:26 → 0:42, 100 à 212 km/h |
| `2026-11-08-apk918-video2-planche.jpg` | menus → choix Course à 5 → garage (Giuliva Trefoil) → Ville Néon, **Nuit** → départ (image blanche / rose jusqu'à ≈ 0:03) → pause → Réglages |
| `2026-11-08-apk918-video3-planche.jpg` | Réglages → pause → Ville de nuit, 0:04 → 0:11, 92 à 161 km/h, foule sur les trottoirs |
| `2026-11-08-apk918-video4-planche.jpg` | Réglages (chiffres « Rendu / Images ») → voie rapide et pont de nuit, 215 → 284 km/h |
| `2026-11-08-apk918-1.jpg` … `-5.jpg` | captures : vue Arrière nuit (fenêtres lumineuses hors des façades à droite), départ délavé blanc / rose, Réglages en menu (10 img/s, 699 k triangles), écran de chargement (v0.1.918, chiffres de mémoire) |
| `2026-11-08-apk916-1.jpg` … `-7.jpg` | APK 916 : Réglages en course (10 et 20 img/s, triangles par famille), Course à 5 (5/5), cockpit de nuit, fenêtres flottantes, chargement |

## Points à vérifier (verdict OK / KO / À REVOIR, avec l'image qui le prouve)
1. **Départ de nuit** : image blanche / rose délavée de 0:00 à ≈ 0:03 (ANO-177 → WFS-328). Toutes cartes ? Après « Recommencer » ?
2. **Fenêtres lumineuses hors des façades** la nuit, Ville (ANO-175 → WFS-326, en cours) : où exactement, à quelle distance, de quel côté.
3. **Course à 5** : le joueur finit 5/5, adversaires très en avance (ANO-176 → WFS-327, en cours).
4. **Performance** : images par seconde, appels, triangles par famille, mémoire lus dans les Réglages (APK 916 et 918) ; comparer aux objectifs (≥ 25 img/s Ville de nuit Basse ; WFS-317, 265).
5. **Aide à la direction** (validée par l'utilisateur) : la voiture reste-t-elle sur la route jusqu'à 284 km/h (vidéo 4) ? virages serrés (vidéos 1 et 3) ?
6. **Foule en Ville** : densité visible (avant le −70 % de WFS-317, pas encore dans l'APK 918).
7. **Vue Cockpit de nuit** (APK 916, capture 3) : planche sombre, bloc sombre en haut (WFS-282 / 315 / 323 ; l'utilisateur validera après WFS-323 à 325 : seulement noter).
8. **Écran de chargement** : version v0.1.918, chiffres de données et de mémoire lisibles (WFS-297, 279).
9. **Menus et Réglages** : rien de cassé (choix du mode, du terrain, garage, pause, Réglages, boutons Fermer / Reprendre).
10. **Tout autre défaut visible** sur ces images : nouvelle anomalie (gravité décidée par le QA).
