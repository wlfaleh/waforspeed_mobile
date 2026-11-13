## Agent : Claude, agent dev3d (01Us6uCxMq) (session_01Us6uCxMq4ZcucYCU7MHDaF)

**Ticket** : WFS-489 — règles : `regles-de-gestion/WFS-489.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : au garage, choisir sa couleur parmi 28 et sa couleur secondaire (toit et rétroviseurs), voir ce qui est gratuit et le prix du reste, et acheter en étoiles (épic E18, `regles-de-gestion/WFS-489.md`, RC-E18-01 à 11).
- **Ce qui change** :
  - `src/ui/garage_couleur.*` (rangée des couleurs) : onglets **« Principale » / « Toit et rétros »** (RG-02) à la place du libellé « Couleur » ; **28 pastilles** de taille inchangée (30 × 30, RG-01) qui **défilent au doigt** (glisser) ou par les **chevrons** cyan aux bords ; en « Toit et rétros », pastille **« Comme la carrosserie »** (signe =) en premier. Pastille non possédée : **petit cadenas + prix en ★** dessous (50 ★ × coefficient de la classe : 50 sur la 208, 75 sur la Golf) ; un appui la montre tout de suite sur la voiture (aperçu, toit et rétros compris : `peinture2ProchaineVoiture`) ; à droite, nom de la couleur et, si elle est payante, **« 75 ★ = 1 bronze = 0,74 € »** (RC-E18-11). Choix possédé : gardé tout de suite (`paint-`, `paint2-`, RG-04). À chaque ouverture du garage, chaque voiture reprend sa **dernière couleur possédée** (aperçu non acheté oublié, RG-03).
  - `src/ui/garage_achat.*` + `garage_achat_prix.cpp` : **« Acheter N ★ »** à la place de « Choisir » (même bouton, même taille) quand la couleur montrée, le droit « toit et rétros » ou la couleur secondaire ne sont pas possédés (N = total de ce qui manque) ; **solde** au-dessus des boutons ; solde insuffisant → bandeau **« Il te manque N ★ »** (bord rouge), **rien n'est débité** ; achat → `boutique::acheterObjet` (débit une seule fois par objet), choix gardé, sauvegarde écrite. L'étoile ★ est **dessinée** (la police Rajdhani n'a pas ce caractère).
  - **Don provisoire de WFS-487 retiré** (`perso::donGarageProvisoire` n'est plus appelé par « Choisir ») ; les couleurs déjà données restent possédées. La fonction (zone E3) reste dans `personnalisation.*` avec son test : à retirer par un ticket E3 (signalé au PO).
  - Textes FR / EN (`assets/textes/fr.txt`, `en.txt`) : `garage.choisir|retour|chargement`, `garage.couleur.principale|toit|comme`, `garage.acheter|solde|manque`, `medaille.*`, `couleur.<id>` (28 noms). Accord avec WFS-507 (session 01Us6uCw3C) : ces clés sont les miennes, WFS-507 ne touche pas `garage_couleur.*` ni `garage_achat.*`.
  - **Accroches** (annoncées au journal) : `garage.cpp` 8 lignes changées + 2 retirées (ouverture, couleur 2 dessinée, toucher de la rangée, « Acheter », état, hauteur de la rangée, planche limitée aux 16 peintures historiques, don retiré) ; `garage_commun.h` (28 couleurs, `EtatGarage::peinture2`, zones des onglets / chevrons / pastilles du toit) ; `desktop_main.cpp` 1 ligne (banc) ; `garage.h` 1 ligne ; `CMakeLists.txt` 1 ligne (test).
- **Comment c'est vérifié** :
  - nouveau test CPU **`test_garage_achat`** (13 cas : gratuites sans prix, 50 / 75 ★ selon la classe, droit du toit, même couleur comptée une fois, refus sans débit, achat débité une fois, couleur achetée pour une voiture seulement) ;
  - nouveau banc **`WFS_BANC=garage-couleurs`** (`src/ui/garage_couleur_banc.cpp`), vraie sauvegarde du banc, vrais touchers : défilement au doigt, aperçu, **refus (30 ★ pour 50 ★ : rien de débité)**, **achat** Bleu électrique (208), **toit et rétros Noir** (208), Orange fluo (GT86), Rose (Golf), aperçu Violet néon puis « Retour » (Golf reste Rose), **relance du jeu** (sauvegarde relue : tout gardé), courses de jour et de nuit avec les 3 voitures, puis anglais : **0 écart** ;
  - parcours RC-E18-09 : banc ci-dessus + **`WFS_BANC=parcours` : 0 écart** (menu → garage → choix → course → retour) ;
  - bancs `garage` avant / après (même captures, la rangée a maintenant ses onglets et ses 28 pastilles).
- **Limites et risques connus** :
  - la fiche de la voiture (« Vitesse », « Puissance », type, accroche) reste en français dans les captures anglaises : c'est **WFS-507** (en cours, autre session) ;
  - RC-E18-03 (images/s en Basse) : aucun code de la course n'est touché (garage seulement ; la course lit les couleurs déjà possédées comme avant), pas de mesure d'endurance faite ;
  - aucune confirmation avant l'achat d'une couleur (le ticket n'en demande pas ; WFS-499 en a une pour les voitures) ;
  - captures produites sur le commit 0b69e622 ; depuis : test CPU, calcul des prix sorti dans `garage_achat_prix.cpp` (même code) et prix de la couleur montrée même quand sa pastille est hors de la vue (cas absent des captures).

## Tests CPU

Commit testé : 8276380f (branche synchronisée avec `main`), journal complet : `qa/pr/WFS-489/tests-cpu.log`.

```
[OK] test_ville_lieux (0 s)
[OK] test_ville_sol (3 s)
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 125 test(s) réussi(s) en 327 s
code de sortie : 0
```

## Captures pour le lead dev (37 après)

Dossier : [`qa/pr/WFS-489`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `garage-couleurs-course-golf-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-course-golf-jour.jpg?raw=true) |
| `garage-couleurs-course-golf-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-course-golf-nuit.jpg?raw=true) |
| `garage-couleurs-course-gt86-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-course-gt86-jour.jpg?raw=true) |
| `garage-couleurs-course-gt86-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-course-gt86-nuit.jpg?raw=true) |
| `garage-couleurs-course-p208-jour` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-course-p208-jour.jpg?raw=true) |
| `garage-couleurs-course-p208-nuit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-course-p208-nuit.jpg?raw=true) |
| `garage-couleurs-en-golf-magenta-apercu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-en-golf-magenta-apercu.jpg?raw=true) |
| `garage-couleurs-en-golf` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-en-golf.jpg?raw=true) |
| `garage-couleurs-en-il-manque` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-en-il-manque.jpg?raw=true) |
| `garage-couleurs-en-toit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-en-toit.jpg?raw=true) |
| `garage-couleurs-golf-rose` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-golf-rose.jpg?raw=true) |
| `garage-couleurs-golf-violetneon-apercu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-golf-violetneon-apercu.jpg?raw=true) |
| `garage-couleurs-gt86-orangefluo` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-gt86-orangefluo.jpg?raw=true) |
| `garage-couleurs-p208-bleuelectrique-apercu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-p208-bleuelectrique-apercu.jpg?raw=true) |
| `garage-couleurs-p208-bleuelectrique` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-p208-bleuelectrique.jpg?raw=true) |
| `garage-couleurs-p208-debut` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-p208-debut.jpg?raw=true) |
| `garage-couleurs-p208-defile` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-p208-defile.jpg?raw=true) |
| `garage-couleurs-p208-il-manque` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-p208-il-manque.jpg?raw=true) |
| `garage-couleurs-p208-toit-noir-apercu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-p208-toit-noir-apercu.jpg?raw=true) |
| `garage-couleurs-p208-toit-noir` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-p208-toit-noir.jpg?raw=true) |
| `garage-couleurs-p208-toit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-p208-toit.jpg?raw=true) |
| `garage-couleurs-relance-golf` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-relance-golf.jpg?raw=true) |
| `garage-couleurs-relance-p208` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-couleurs-relance-p208.jpg?raw=true) |
| `course-p911-gris` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/avant-course-p911-gris.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-course-p911-gris.jpg?raw=true) |
| `garage-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/avant-garage-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-0.jpg?raw=true) |
| `garage-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/avant-garage-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-1.jpg?raw=true) |
| `garage-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/avant-garage-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-2.jpg?raw=true) |
| `garage-course` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/avant-garage-course.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-course.jpg?raw=true) |
| `garage-m4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/avant-garage-m4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-m4.jpg?raw=true) |
| `garage-peinture-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/avant-garage-peinture-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-peinture-0.jpg?raw=true) |
| `garage-peinture-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/avant-garage-peinture-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-peinture-1.jpg?raw=true) |
| `garage-peintures-p911` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/avant-garage-peintures-p911.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-peintures-p911.jpg?raw=true) |
| `garage-peintures` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/avant-garage-peintures.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-peintures.jpg?raw=true) |
| `garage-suivant` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/avant-garage-suivant.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-garage-suivant.jpg?raw=true) |
| `showroom-suivante-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/avant-showroom-suivante-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-showroom-suivante-1.jpg?raw=true) |
| `showroom-suivante-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/avant-showroom-suivante-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-showroom-suivante-2.jpg?raw=true) |
| `showroom-suivante-3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/avant-showroom-suivante-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-489/qa/pr/WFS-489/apres-showroom-suivante-3.jpg?raw=true) |
