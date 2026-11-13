Agent : Claude, agent dev3d (01Us6uCv7i) (session_01Us6uCv7iTDc5Ni5pnSqHnh)

**Ticket** : WFS-510 — règles : `regles-de-gestion/WFS-510.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : market de l'épic E21 (dernière étape identité → score → voitures et couleurs → cadeau → market) : packs d'étoiles (bronze 100, argent 330, or 720, coffre 4 500 ★), pack débutant (500 ★ + 1 skin, une fois, après la 3e course), « sans pub », ×2 de fin de course 5 fois par jour, et **jamais deux fois les étoiles pour un même achat**. **Ce ticket = la logique, sans écran ni Google** (Google Play : WFS-511 ; vidéo : WFS-512 ; écrans : WFS-513, 514).
- **Ce qui change** :
  - **nouveau module `src/game/market.h/.cpp`** (zone E3) : catalogue (`catalogue`, `pack`, `totalEtoiles` = base + arrondi(base × offert / 100), noms FR / EN), pack débutant (`packDebutantPropose` : à partir de la fin de la 3e course du profil de WFS-480, jamais après l'achat, et **seulement si des skins sont en vente** : `SKINS_EN_VENTE = false`, une ligne à passer à `true` avec WFS-494) ; **livre des achats** (`crediterAchat` → « crédité » / « déjà crédité » / « inconnu » ; jeton déjà au livre → 0 ★ ; crédit + jeton dans la même écriture de la sauvegarde, **écriture attendue** avant la confirmation auprès de Google ; 200 derniers jetons gardés ; jeton mal formé refusé) ; `traiterAchat` (acheté → crédit puis confirmer, même si déjà crédité ; en attente → rien, message « Achat en attente de paiement » ; annulé / erreur → rien, sans message) ; `sansPub` ; ×2 (`x2Restants`, `bonusX2Disponible`, `appliquerX2`, `x2SansPub` direct avec « sans pub », `recompenseVideo` pour WFS-512 : 5 par jour, date locale, l'heure qui recule ne remet pas à zéro, une seule fois par course) ; **interface `Boutique`** (`chargerProduits`, `prix`, `acheter`, `restaurer`, `confirmer`, rappel `surAchat`), `brancher` (boutique ↔ sauvegarde) et **`BoutiqueSimulee`** (achat, en attente, annulé, interrompu, même jeton deux fois ; `WFS_BOUTIQUE=simulee` → `creerBoutique`) ;
  - `src/game/market_etat.h` : données gardées ; **sauvegarde** (`sauvegarde.*`) : nouvelles clés seulement (`achats-credites`, `pack-debutant`, `bons-skin`, `sans-pub`, `x2=jour;nombre;course`), écrites seulement quand elles ne valent pas leur défaut (les fichiers d'aujourd'hui ne changent pas) ; valeur mal écrite ignorée sans erreur ;
  - **`configgame.*`, section `market` seulement** (exception du PO) : 18 clés `market.packs.<produit>.etoiles / offert / prix_secours_eur`, `pack_debutant.bons_skin / apres_courses`, `market.x2_par_jour`. `git diff origin/main -- configgame.json configgame.README.md` : seulement des ajouts (section `market`, son paragraphe et ses 18 lignes), comme demandé au refus de WFS-498 ;
  - `src/game/market_banc.cpp` : banc `WFS_BANC=market` (sans rendu, 11 s, écrit `market.txt`, code 3 si un résultat est faux) ;
  - **accroches** annoncées au journal : `src/platform/desktop_main.cpp` +2 lignes (banc `market`) ; `CMakeLists.txt` +1 ligne (`test_market`) ;
  - **à l'écran : rien ne change** (16 paires avant / après identiques octet pour octet, plus bas).
- **Comment c'est vérifié** :
  - **`test_market`** (RG-10, 37 cas) : chaque pack crédite son total une seule fois ; même jeton deux fois → une fois ; en attente / annulé / erreur → 0 ; interrompu puis relance (sauvegarde relue) → une fois, puis confirmé ; pack débutant absent avant la 3e course, absent sans skin en vente, une seule fois même avec un autre jeton ; sans pub, restauration après réinstallation ; ×2 5 par jour avec et sans « sans pub », minuit, heure qui recule, une fois par course, compteur relu ; livre de 200 jetons relu ; clés mal écrites ; `configgame.json` mal écrit (−5 ★, texte, 500 ×2) → valeurs du code + messages ; JSON cassé → valeurs du code, pas d'arrêt ;
  - **124 tests CPU** passent (`tests-cpu.log`), sur la branche synchronisée avec `main` (commit 7a7b57e8) ;
  - **banc `market`** — capture 1, [catalogue](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/texte-market-1-catalogue.md) ; capture 2, [même jeton deux fois puis achat interrompu et relance](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/texte-market-2-jeton-double-interrompu.md) (solde inchangé à la relance, 0 achat non confirmé) ; capture 3, [7 demandes de ×2 le même jour](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/texte-market-3-x2.md) (5 accordées, minuit, heure reculée refusée, sans pub 5 sur 7) ; capture 4, [argent à 350 ★ dans `configgame.json`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/texte-market-4-argent-350.md) (385 ★, même exécutable, sans recompiler) ; [texte complet](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/texte-market.md) ;
  - **garage, course et sauvegarde avant / après** (`captures_pr.sh`, bancs `garage`, `sauvegarde` ×2) : 16 paires **identiques octet pour octet** (`cmp`).
- **Limites et risques connus** :
  - aucun écran ni Google : rien de visible pour le joueur avant WFS-511 / 513 / 514 ; `creerBoutique()` rend `nullptr` sur Android tant que WFS-511 n'est pas là ;
  - les étoiles de base des 3 médailles sont dans `market.packs` (RG-01) ; les `boutique.medailles` de WFS-498 restent la base de l'équivalent en euros du garage (mêmes valeurs aujourd'hui) : changer un pack ne change pas cet équivalent (à décider par le PO si l'on veut une seule valeur) ;
  - sans serveur (RC-E21-07) : le livre protège contre un double crédit sur le téléphone, pas contre une triche ; la vérification de signature par Google sera dans WFS-511 ;
  - la sauvegarde garde `version=1` (nouvelles clés seulement, comme WFS-480, 487, 498).

## Tests CPU

```
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 124 test(s) réussi(s) en 420 s
code de sortie : 0
```

## Captures pour le lead dev (16 après)

Dossier : [`qa/pr/WFS-510`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `course-p911-gris` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/avant-course-p911-gris.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/apres-course-p911-gris.jpg?raw=true) |
| `garage-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/avant-garage-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/apres-garage-0.jpg?raw=true) |
| `garage-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/avant-garage-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/apres-garage-1.jpg?raw=true) |
| `garage-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/avant-garage-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/apres-garage-2.jpg?raw=true) |
| `garage-course` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/avant-garage-course.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/apres-garage-course.jpg?raw=true) |
| `garage-m4` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/avant-garage-m4.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/apres-garage-m4.jpg?raw=true) |
| `garage-peinture-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/avant-garage-peinture-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/apres-garage-peinture-0.jpg?raw=true) |
| `garage-peinture-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/avant-garage-peinture-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/apres-garage-peinture-1.jpg?raw=true) |
| `garage-peintures-p911` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/avant-garage-peintures-p911.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/apres-garage-peintures-p911.jpg?raw=true) |
| `garage-peintures` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/avant-garage-peintures.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/apres-garage-peintures.jpg?raw=true) |
| `garage-suivant` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/avant-garage-suivant.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/apres-garage-suivant.jpg?raw=true) |
| `showroom-suivante-1` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/avant-showroom-suivante-1.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/apres-showroom-suivante-1.jpg?raw=true) |
| `showroom-suivante-2` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/avant-showroom-suivante-2.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/apres-showroom-suivante-2.jpg?raw=true) |
| `showroom-suivante-3` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/avant-showroom-suivante-3.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/apres-showroom-suivante-3.jpg?raw=true) |
| `fin-record` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/avant-fin-record.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/apres-fin-record.jpg?raw=true) |
| `sauvegarde-relue` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/avant-sauvegarde-relue.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-510/qa/pr/WFS-510/apres-sauvegarde-relue.jpg?raw=true) |
