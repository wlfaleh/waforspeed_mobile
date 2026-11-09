# Recette manuelle — APK 0.1.954 (vidéos de l'utilisateur, 2026-11-09 ≈ 11:36-11:44 UTC)

Demandée par l'utilisateur le 2026-11-09 11:46:43 UTC (« regarde le test de la dernière version, passe-le au QA »). Préparée par le PO. Recette par `qa-lead` : ouvrir chaque planche
**en grand** (et les vidéos si besoin, `/root/…` non accessibles : planches seulement), verdict OK / KO / À REVOIR par point avec l'image, rapport `qa/rapports/recette-manuelle-apk954.md`,
nouvelles anomalies dans `anno.md`.

## Matériel (`qa/retours/2026-11-09-apk954-video1..5-planche.jpg`)
| Planche | Contenu vu par le PO |
|---|---|
| video1 | Réglages **en onglets** (Affichage / Conduite / Son), ligne **Fantôme du record** ; puis Checkpoint de nuit, vue Proche, 54 → 123 km/h |
| video2 | Réglages (Son) → pause → **cockpit de nuit**, accident (image rouge, « ACCIDENT ! »), bandeau du checkpoint qui traverse l'habitacle, R au compteur |
| video3 | **Écran des CGU : case cochée, appuis répétés sur ACCEPTER, l'écran ne part pas** (8 images) |
| video4 | « Mise à jour et installation » (Ville 1 / 22 Mo, Plus tard) → chargement v0.1.954 → menu → garage « **FOLG 8** » (nom sans marque) → choix du terrain « **Supprimer — libère 22 Mo** » → départ de nuit **très éblouissant** (halos énormes au compte à rebours) |
| video5 | Pause → Réglages en **Haute** (onglets, chiffres « Rendu / Images ») |

## Points à vérifier
1. **CGU** : le bouton **ACCEPTER ne fait rien** (video3) ? Bloquant : le joueur ne peut plus entrer dans le jeu. Lien avec ANO-199 (émulateur bloqué aux CGU) : même cause ?
2. **Paquets de cartes** (WFS-306, 307) : écran « Mise à jour et installation », « Plus tard », « Supprimer — libère 22 Mo » : clair, juste ?
3. **Voitures sans marque** (WFS-192, 193) : « FOLG 8 » au garage ; logos et formes.
4. **Réglages en onglets** (WFS-290, 291, 318) : onglets, Fermer en un appui, Fantôme du record.
5. **Départ de nuit éblouissant / délavé** (video4, ANO-177 / 179, WFS-328).
6. **Cockpit de nuit** (video2) : planche, bandeau de checkpoint dans l'habitacle, pluie dedans (ANO-186), voitures qui entrent (ANO-183).
7. **Performance** : chiffres lisibles des Réglages en Basse (video1) et en Haute (video5).
8. **Tout autre défaut visible.**
