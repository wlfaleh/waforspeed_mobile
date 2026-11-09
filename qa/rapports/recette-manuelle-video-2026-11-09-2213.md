# Recette manuelle — 11 vidéos de l'utilisateur du 2026-11-09 22:13 → 22:25 (heure du téléphone)

Message de l'utilisateur : « @po et @qa c'est quoi ce bordel une régression ou une catastrophe ? ».
APK **v0.1.969** (numéro lu sur l'écran de chargement). 11 vidéos 2340x1080, 4 à 25 s.
Recette faite par le `qa-lead` (agent appelé par le PO), sur les planches du PO (1 image/s) **et** sur des extractions propres
à 3, 4 et 10 images/s (zones HUD, boutons GAZ / FREIN / NITRO, libellé du moment de lumière). L'option Android
« Afficher les appuis » est active sur toutes les vidéos : chaque doigt posé laisse un **rond gris** à l'écran. C'est la preuve
utilisée plus bas.

Matériel : `qa/retours/2026-11-09-video2213-v02-campagne-doree-roule.jpg`, `-v03-gaz-sans-appui.jpg` (4 img/s),
`-v03-nitro-sans-appui.jpg` (10 img/s), `-v04-aube-libelle.jpg`, `-v06-doree-cockpit-sombre.jpg` (3 img/s).

## Verdict global
**Pas de catastrophe, pas de régression de conduite démontrée.** Le jeu démarre, les menus marchent, les 11 courses se lancent,
aucun plantage ni écran noir. La Corniche roule partout (jusqu'à 111 km/h) et la **Campagne roule aussi** quand on tient GAZ
(vidéo 2 : 12 puis 32 km/h, score 35).
Le vrai défaut vu sur les vidéos est la **lumière** : avec un moment du bouton **JOUR** (Réelle à 22 h, Aube, Dorée), la Campagne
s'affiche presque noire, si bien que la course paraît « de nuit » et cassée → **ANO-232, majeure, WFS-375**.

## Constat du PO n° 1 (« Campagne bloquée derrière la ligne ») : non confirmé
- Vidéos 3, 4, 5, 6 : **aucun rond d'appui sur GAZ après GO**, à aucune image (4 img/s de GO à la pause). GAZ reste gris, ce qui est
  normal sans doigt dessus. Dans les vidéos 4 et 5, l'utilisateur touche GAZ **une fraction de seconde pendant le « 3 »** puis lâche
  (v04 à 1,33 s, v05 à 6,0 s) ; ensuite il ouvre la pause et les réglages.
- Contre-épreuve sur les mêmes vidéos : quand le doigt est sur GAZ, le rond se voit et GAZ devient orange (v02 en Campagne, v07, v09,
  v10, v11 en Corniche ; v11 : GAZ orange **dès le décompte**, doigt tenu).
- v03zoom : la voiture « qui avance d'un mètre puis revient » vient du **recadrage de la caméra** après GO ; le compteur reste à
  0 km/h pendant ce mouvement. Pas de mur invisible visible.
- Conclusion : la voiture n'avance pas parce que le gaz n'est pas tenu. **Pas d'anomalie bloquante créée.** Reste un doute faible
  (un appui non dessiné par Android n'existe pas, mais l'utilisateur pense avoir appuyé) : à WFS-374 de lancer, à bas coût, le banc
  « départ Campagne, gaz tenu dès GO » pour Réelle / Aube / Matin / Dorée, en vue Arrière et Cockpit, et de le noter au journal.
- Une vraie bizarrerie est vue au passage (vidéo 3, 10,2 s) : un appui **en haut à droite (case SCORE)**, sans toucher NITRO ni GAZ,
  déclenche **une image de nitro** (anneau NITRO allumé, lignes de vitesse) et le compteur passe 0 → 3 km/h, puis retombe à 0 →
  **ANO-233, mineure**, à vérifier dans WFS-374.

## Vidéo par vidéo
| # | Heure | Carte / mode / moment / vue | Constat | Verdict |
|---|---|---|---|---|
| 1 | 22:13:26 | Campagne, Checkpoint, moment non visible (vidéo commencée au décompte), Arrière | Décompte 2, 1 puis pause ; image rouge-brun très sombre (ciel blanc-rose, sol brun) | KO lumière → ANO-232 |
| 2 | 22:13:52 | Campagne, Checkpoint, **Dorée**, Arrière | Choix Aube → Midi → Dorée ; doigt sur GAZ dès « 1 », GAZ orange à GO, 12 puis 32 km/h, score 35 ; image jaune-orangé lisible | OK, **la Campagne roule** |
| 3 | 22:14:37 | Campagne, Checkpoint, **Réelle** (22:14 au téléphone), Arrière | Image de nuit noire alors que JOUR est choisi ; aucun appui sur GAZ après GO, 0 km/h, chrono 60 → 57 ; appui sur la case SCORE → nitro d'une image, 3 km/h ; pause, réglages | KO lumière → ANO-232 ; ANO-233 ; arrêt = pas de gaz |
| 4 | 22:15:12 | Campagne, Checkpoint, Réelle → **Aube**, Arrière | Image presque noire ; tape GAZ pendant le « 3 » puis lâche ; 0 km/h ; pause, réglages | KO lumière → ANO-232 ; arrêt = pas de gaz |
| 5 | 22:15:40 | Campagne, Checkpoint, **Matin**, Arrière | Image claire de jour (herbe verte, ombres) ; tape GAZ pendant le « 3 » puis lâche ; 0 km/h ; pause, réglages | Lumière OK ; arrêt = pas de gaz |
| 6 | 22:22:48 | Campagne, Checkpoint, **Dorée**, Cockpit | Image rouge-brun presque noire (même moment que la vidéo 2, beaucoup plus sombre) ; aucun appui sur GAZ ; 0 km/h ; pause, réglages | KO lumière → ANO-232 ; arrêt = pas de gaz |
| 7 | 22:23:29 | Corniche, Checkpoint, **Dorée**, Cockpit | Écran « Construction de la ville 70 % » ≈ 3 s puis course ; GAZ tenu, 11 → 68 km/h, score 165 | OK |
| 8 | 22:24:01 | Corniche, Course à 5 (4 km), **Réelle** (22:24), Cockpit puis Arrière | Image de nuit alors que JOUR est choisi ; départ, 5 voitures ; vidéo finie avant l'appui sur GAZ | KO lumière → ANO-232 |
| 9 | 22:24:39 | Corniche, Course à 5, **NUIT**, Arrière | Seule vraie course de nuit ; phares, GAZ tenu, 16 → 111 km/h, score 360 | OK |
| 10 | 22:25:23 | Corniche, Course à 5, défilement Jour : Midi → Dorée → Aube → Midi → Réelle → Aube → Matin → Midi ; **Midi** | Jour clair, mer, plage ; GAZ tenu, 6 → 61 km/h | OK (défilement : WFS-375 déjà prêt) |
| 11 | 22:25:56 | Corniche, Course à 5, **Midi**, Arrière | GAZ tenu dès le décompte (orange), 23 → 91 km/h, dépassements, score 715 | OK |

## Anomalies
| ANO | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| ANO-232 | **Majeure** | v01, v03, v04, v06, v08 ; `-v06-doree-cockpit-sombre.jpg`, `-v04-aube-libelle.jpg` | Un moment du bouton **JOUR** donne une image de nuit ou presque noire : « Réelle » suit l'heure du téléphone (22 h → nuit noire, v03, v08), « Aube » presque noire (v04), « Dorée » rouge-brun presque noire en Cockpit (v06) alors que la même « Dorée » est jaune et lisible en Arrière (v02). Le joueur croit à une course de nuit cassée. Correction : **WFS-375** (JOUR = soleil haut, Midi, quelle que soit l'heure ; plus de Réelle / Aube / Dorée au choix du joueur). |
| ANO-233 | **Mineure** | v03 à 10,2 s ; `-v03-nitro-sans-appui.jpg` | Un appui en haut à droite (case SCORE), loin de NITRO et de GAZ, déclenche une image de nitro (anneau allumé, lignes de vitesse) et 0 → 3 km/h. Zone tactile du nitro ou geste parasite à vérifier dans **WFS-374**. |

## Hors périmètre (pas d'ANO, sur ordre)
- Décor en formes nues (maisons en boîtes, arbres en cônes, sols et mer en aplats, planche de bord du Cockpit en boîtes) : déjà couvert
  par l'épic E15 (WFS-369 à 373, 380) et WFS-378 ; anomalies de décor abandonnées sur ordre de l'utilisateur.
- Chargement de la Corniche : « Construction de la ville 70 % » tenu ≈ 3 s (v07, v09, v10) puis 100 % ; la Campagne part sans écran
  de chargement. Pas de blocage ni de plantage : à suivre avec la performance, pas d'ANO.

## Pour WFS-374 (dev Z)
Symptôme exact : **aucun défaut de conduite prouvé** sur v0.1.969 ; la Campagne roule gaz tenu (v02). Ce que l'utilisateur a vu, c'est
une Campagne presque noire avec un moment JOUR (ANO-232 → WFS-375) et une voiture à l'arrêt **sans doigt sur GAZ**. À faire : le banc
de départ Campagne gaz tenu (ci-dessus) pour lever le doute, et ANO-233.
