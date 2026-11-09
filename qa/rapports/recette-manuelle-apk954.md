# Recette manuelle — APK 0.1.954 (vidéos de l'utilisateur, 2026-11-09 ≈ 11:36-11:44 UTC)

- **QA** : agent qa lead. Demande du PO (pour l'utilisateur), `qa/recette-manuelle-apk954.md`.
- **APK** : v0.1.954, construction 954, commit `788c286`. Il contient WFS-308 (CGU), WFS-328 (départ de nuit), WFS-291 (onglets), WFS-318 (fantôme), WFS-306 (paquets).
- **Matériel** : les 5 planches ouvertes en grand, avec des zooms : `2026-11-09-apk954-cgu-accepter-appui.jpg`, `-depart-nuit-eblouissant.jpg`, `-cockpit-accident.jpg`, `-reglages-rendu-haute-basse.png`.

| # | Point | Verdict | Preuve | Constat |
|---|---|---|---|---|
| 1 | CGU : ACCEPTER | **KO, bloquant** | `cgu-accepter-appui.jpg`, `video3-planche.jpg` | Case cochée, bouton orange, au moins 4 appuis en plein sur ACCEPTER, l'écran reste → **ANO-201**. Probablement la même cause que l'émulateur bloqué (ANO-199) : défaut du jeu possible, pas seulement de l'outil. |
| 2 | Paquets de cartes | **OK** | `video4-planche.jpg` | « Mise à jour et installation » : « Ville — 1 Mo / 22 Mo », barre de progression, astuce, « Plus tard ». Choix du terrain : « Supprimer — libère 22 Mo » lisible sur la carte. Clair. |
| 3 | Voitures sans marque | **OK** | `video4-planche.jpg` | Menu « TA VOITURE : FOLG 8 », garage « FOLG 8 », pas de logo lisible à cette taille. |
| 4 | Réglages en onglets, Fermer, fantôme | **OK** | `video1-planche.jpg`, `video5-planche.jpg` | 3 onglets (Affichage, Conduite, Son) qui changent à chaque appui ; ligne « Fantôme du record : Affiché / Caché » ; « Fermer » en un appui ; lien « Conditions d'utilisation ». |
| 5 | Départ de nuit éblouissant | **KO, bloquant** | `depart-nuit-eblouissant.jpg` | WFS-328 est dans l'APK, mais au « 3 » les façades sont toujours des blocs blancs éblouissants avec halos énormes → **ANO-202**. |
| 6 | Cockpit de nuit | **KO, mineur** | `video2-planche.jpg`, `cockpit-accident.jpg` | Ligne et portique du checkpoint à travers l'habitacle (**ANO-203**) ; au choc, disques flous dans l'habitacle et « R » de marche arrière (**ANO-204**). Rien qui entre dans la voiture sur ces images (ANO-183, WFS-333 : pas de nouveau cas). |
| 7 | Performance | **KO** | `reglages-rendu-haute-basse.png` | Basse 60 % : 18 img/s (médiane 48,2 ms) ; **Haute 80 % : 7 img/s** (médiane 165 ms). Ajouté à ANO-198. |
| 8 | Autres | voir ANO-205 | `cgu-accepter-appui.jpg` | Texte provisoire montré au joueur dans les CGU : « Contact : à compléter par l'éditeur… » → **ANO-205**. |

## Anomalies

| ANO | Gravité | Ce qui ne va pas |
|---|---|---|
| ANO-201 | **Bloquante** | ACCEPTER des CGU ne répond pas, case cochée |
| ANO-202 | **Bloquante** | départ de nuit toujours éblouissant malgré WFS-328 |
| ANO-203 | Mineure | ligne du checkpoint à travers l'habitacle (cockpit) |
| ANO-204 | Mineure | disques flous dans l'habitacle au choc, « R » affiché |
| ANO-205 | Mineure | texte provisoire « Contact : à compléter » dans les CGU |
| ANO-198 (revue) | Bloquante | 18 img/s en Basse, 7 img/s en Haute |

## Verdict global

**KO** : un nouveau joueur peut rester bloqué sur les CGU (ANO-201), et le départ de nuit reste éblouissant (ANO-202).
