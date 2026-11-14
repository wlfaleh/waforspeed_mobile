Agent : Claude, agent dev3d (01Us6uCvpo)

**Ticket** : WFS-513 — règles : `regles-de-gestion/WFS-513.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : un écran Market clair, en français et en anglais, ouvert depuis le menu, le compteur d'étoiles et le garage, où le joueur voit son solde, chaque pack (étoiles, bonus offert, prix), le « Sans pub », achète et restaure ses achats.
- **Ce qui change** :
  - nouvel écran `src/ui/ecrans/market_ecran.*` (système « MarketEcran », ordre 469, par-dessus le menu ou le garage) : solde en haut (animé quand des étoiles arrivent), 4 cartes (nom, médaille, total en gros, badge « +N % offerts », « base ★ + N offertes », prix de la boutique sur le bouton orange), pack débutant en tête quand il est proposé (aujourd'hui jamais : `SKINS_EN_VENTE = false`, WFS-510), « Sans pub » à part avec « Acheté ✓ », mentions RG-06, « Restaurer mes achats », « Retour » (= touche Retour d'Android) ;
  - achat : `Boutique::acheter`, boutons « Patiente… » / grisés pendant l'attente (rendus après 60 s sans réponse), achat réussi → « +720 ★ » et solde qui monte en 1,2 s ; en attente → « Achat en attente de paiement » ; annulé → rien ; restauration → « Achats restaurés » / « Aucun achat à restaurer » (2 s) ;
  - hors ligne (aucune boutique ou aucun prix de Google) : packs visibles, boutons grisés, « Connexion à Google Play nécessaire » ;
  - **sécurité des achats inchangée** : l'écran ne crédite rien ; il rebranche la boutique par `market::brancher(b, sauvegarde(), apresAchat)` (traitement de WFS-510 / 511 identique, plus un rappel pour l'écran) et appelle `chargerProduits()` à l'ouverture ;
  - menu : bouton « ★ MARKET » au-dessus de « Profil » ; le compteur d'étoiles (WFS-537 RG-09) ouvre le Market au lieu du garage ; le bandeau « Voiture prêtée » remonte d'un cran (au-dessus de « Market », voir `pret-menu-actif`) ;
  - garage : bouton « ★ MARKET » sous le solde (accroche de 4 lignes dans `garage.cpp`) ;
  - textes `market.*` dans `assets/textes/fr.txt` et `en.txt` ; noms des packs repris de `market::Pack` (WFS-510) ;
  - calculs purs `market_ecran_calculs.h` + `tests/test_market_ecran.cpp` ; banc `WFS_BANC=market` : banc logique de WFS-510 puis parcours de l'écran (accroche de 3 lignes dans `desktop_main.cpp`).
- **Comment c'est vérifié** : banc `market` en 2400x1080 et 1280x720 : FR + EN, **0 écart** (menu → Market → achat → solde monté → retour menu ; touche Retour d'Android ; compteur → Market ; garage → Market → retour garage ; en attente, annulé, restauration vide puis avec « Sans pub », aucun double crédit ; hors ligne sans plantage), RC-E20-06 : 0 texte français ou qui dépasse en anglais ; parcours `WFS_BANC=parcours` : 0 écart ; 143 tests CPU verts (dont `test_market_ecran`).
- **Limites et risques connus** : sur PC les prix sont les prix de secours de `configgame.json` au format français (« 0,99 € », aussi en anglais) ; sur Android, ce sont ceux de Google (pas encore vus sur un vrai téléphone : WFS-515). Message dessiné dans l'écran (bandeau orange) et pas par `toasts.cpp`, qui ne dessine qu'en course. Grande étoile des médailles dessinée sans couture (pentagone + 5 cerfs-volants) : `garage::etoile` montre ses joints en grand. Les captures `pret-*` de course et de garage changent seulement par le bouton Market du garage.

## Tests CPU

Commit testé : `14084fe8` (code identique au commit de la PR : la dernière synchronisation avec `main` n'apporte que `TACHES.md`, `journal.md`, `suivi-pr.md`, `track-agent.md`). Journal complet : `qa/pr/WFS-513/tests-cpu.log`.

```
[OK] test_vehicule_lod (1 s)
[OK] test_vehicule_skin (0 s)
[OK] test_version_chargement (0 s)
[OK] test_ville_lieux (0 s)
[OK] test_ville_sol (4 s)
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 143 test(s) réussi(s) en 478 s
code de sortie : 0
```

## Captures pour le lead dev (26 après)

Dossier : [`qa/pr/WFS-513`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `pret-course-jour-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/avant-pret-course-jour-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-pret-course-jour-en.jpg?raw=true) |
| `pret-course-jour-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/avant-pret-course-jour-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-pret-course-jour-fr.jpg?raw=true) |
| `pret-course-nuit-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/avant-pret-course-nuit-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-pret-course-nuit-en.jpg?raw=true) |
| `pret-course-nuit-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/avant-pret-course-nuit-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-pret-course-nuit-fr.jpg?raw=true) |
| `pret-garage-actif-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/avant-pret-garage-actif-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-pret-garage-actif-en.jpg?raw=true) |
| `pret-garage-actif-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/avant-pret-garage-actif-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-pret-garage-actif-fr.jpg?raw=true) |
| `pret-menu-actif-en` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/avant-pret-menu-actif-en.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-pret-menu-actif-en.jpg?raw=true) |
| `pret-menu-actif-fr` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/avant-pret-menu-actif-fr.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-pret-menu-actif-fr.jpg?raw=true) |
| `market-en-achat` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-market-en-achat.jpg?raw=true) |
| `market-en-attente` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-market-en-attente.jpg?raw=true) |
| `market-en-garage-market` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-market-en-garage-market.jpg?raw=true) |
| `market-en-garage` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-market-en-garage.jpg?raw=true) |
| `market-en-hors-ligne` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-market-en-hors-ligne.jpg?raw=true) |
| `market-en-market` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-market-en-market.jpg?raw=true) |
| `market-en-menu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-market-en-menu.jpg?raw=true) |
| `market-en-restauration` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-market-en-restauration.jpg?raw=true) |
| `market-en-sans-pub` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-market-en-sans-pub.jpg?raw=true) |
| `market-fr-achat` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-market-fr-achat.jpg?raw=true) |
| `market-fr-attente` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-market-fr-attente.jpg?raw=true) |
| `market-fr-garage-market` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-market-fr-garage-market.jpg?raw=true) |
| `market-fr-garage` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-market-fr-garage.jpg?raw=true) |
| `market-fr-hors-ligne` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-market-fr-hors-ligne.jpg?raw=true) |
| `market-fr-market` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-market-fr-market.jpg?raw=true) |
| `market-fr-menu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-market-fr-menu.jpg?raw=true) |
| `market-fr-restauration` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-market-fr-restauration.jpg?raw=true) |
| `market-fr-sans-pub` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-513/qa/pr/WFS-513/apres-market-fr-sans-pub.jpg?raw=true) |
