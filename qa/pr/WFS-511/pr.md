Agent : Claude, agent dev3d (01Us6uCtfa) (session_01Us6uCtfaZCtBn6FQY7UQw9)

**Ticket** : WFS-511 — règles : `regles-de-gestion/WFS-511.md` (et `regles-de-gestion/E21-market.md`)

## Description

- **Demande de l'utilisateur** (en une phrase) : vrais achats Google Play pour le market (packs d'étoiles, pack débutant, « sans pub »), avec les étoiles ajoutées **seulement après la confirmation de Google**, **jamais deux fois** pour un même achat, l'achat interrompu **rattrapé au lancement suivant** et le « sans pub » qui **revient seul après une réinstallation** (épic E21, point 5). Ce ticket = la version Android de l'interface `Boutique` de WFS-510, sans écran (écran Market : WFS-513 ; pub : WFS-512 ; essai réel avec la Play Console : WFS-515).
- **Ce qui change** :
  - **`android/app/build.gradle`** : `implementation 'com.android.billingclient:billing:8.0.0'` (RG-01 ; dépôt Maven de Google déjà utilisé par Gradle ; la version 8 est exigée par Google Play pour les mises à jour depuis le 31/08/2026 ; la bibliothèque ajoute elle-même la permission `com.android.vending.BILLING`) ;
  - **`android/gradle.properties`** : `android.useAndroidX=true` (Billing 8 tire des bibliothèques AndroidX par `play-services-base` ; sans ce réglage, l'AGP 8.5.2 arrête `assembleRelease` / `bundleRelease` avec `ANDROID_X_PROPERTY_NOT_ENABLED` : 1er retour du lead dev) ;
  - **`LogiqueAchats.java`** (nouveau, sans Android ni Google, testé sur la JVM) : les règles. Un achat « acheté » est **envoyé au jeu** et n'est **jamais consommé ni confirmé de lui-même** : c'est le jeu qui appelle `confirmer()` **après** avoir écrit crédit + jeton dans la sauvegarde (RG-04) ; « en attente » → message seulement ; « annulé » → rien ; rattrapage / restauration (RG-05, 06) : chaque achat « acheté » repasse par le jeu (le livre des achats de WFS-510 évite le double crédit) ; « sans pub » déjà confirmé chez Google → recrédité après réinstallation, sans nouvelle confirmation ; même jeton reçu deux fois → une seule consommation ; consommation refusée (réseau coupé) → reprise au rattrapage suivant ; « déjà possédé » → rattrapage relancé ; journal : jeton coupé à 8 caractères (RG-08) ;
  - **`Achats.java`** (nouveau, même paquet que `Paquets.java`) : lien avec `BillingClient` (RG-02, 03) : connexion au démarrage sans bloquer le chargement, reconnexion automatique (1 s, 2 s, 4 s… 60 s au plus), `queryProductDetailsAsync` (INAPP, prix = `formattedPrice`, produit introuvable → pas de prix → pack masqué), `launchBillingFlow`, `consumeAsync` (4 packs d'étoiles) / `acknowledgePurchase` (pack débutant, sans pub), `queryPurchasesAsync` (INAPP) à la connexion, à chaque retour au premier plan et sur « Restaurer » ; tout sur le fil de l'interface ; aucune exception sans Google Play (RG-07) ; achat demandé avant le démarrage du service → réponse « erreur » rendue au jeu (l'écran ne reste pas en attente) ;
  - **`src/platform/achats_android.cpp`** (nouveau, modèle : `paquets_android.cpp`, inclus par `android_main.cpp`) : `BoutiqueGoogle` (interface `market::Boutique` de WFS-510) ; branchée sur `sauvegarde()` par `market::brancher` au démarrage ; **les réponses de Google sont lues sur le fil du jeu** (4 fois par seconde au plus, une attache JNI), puis `traiterAchat` (crédit + jeton écrits et attendus) **puis** `confirmer` — c'était le point « à surveiller » du lead dev sur WFS-510 ;
  - **accroches** (annoncées au journal) : `android_main.cpp` +3 lignes (inclusion, démarrage après la lecture de la sauvegarde, lecture dans la boucle) ; `JeuActivity.onResume` +1 ligne (`Achats.premierPlan`, RG-05) ; `src/game/market.h/.cpp` +5 lignes : `market::boutiqueActive()` (boutique de la plateforme pour l'écran Market de WFS-513 ; `nullptr` sur PC) ;
  - **à l'écran : rien ne change** (aucun écran dans ce ticket ; sur PC, `boutiqueActive()` reste `nullptr`).
- **Comment c'est vérifié** :
  - **`LogiqueAchatsTest`** (RG-09, 12 cas, faux client de facturation qui note chaque appel à Google) : créditer puis consommer (aucune consommation avant le crédit du jeu) ; non consommable confirmé (jamais consommé) ; en attente puis payé ; annulé ; rattrapage au lancement (jeu tué après le paiement) ; même jeton deux fois → une consommation ; consommation refusée puis reprise ; « sans pub » restauré après réinstallation sans reconfirmation ; « déjà possédé » ; hors ligne ; prix de Google ; journal sans jeton complet. **12 / 12 OK** en local (javac + JUnit 4.13.2) ; en CI : `testReleaseUnitTest` (déjà dans `build.yml` et `matin.yml`) ;
  - `Achats.java` compilé en local contre de petites copies vides des classes Android et Google (signatures de Billing 8) : aucune erreur ; `achats_android.cpp` vérifié avec `g++ -fsyntax-only -D__ANDROID__` (jni.h du JDK) : seule différence, `AttachCurrentThread(JNIEnv**)` du NDK, écrit comme dans `paquets_android.cpp` ;
  - **`test_market`** OK et **les 142 tests CPU** (voir plus bas), sur la branche synchronisée avec `main` ;
  - **banc PC `market`** : identique octet pour octet entre `main` et la branche (`cmp`).
- **Limites et risques connus** :
  - **pas de vrai `logcat` de l'APK ici** : pas de NDK ni d'émulateur dans le conteneur, et le dépôt Maven de Google est bloqué par le proxy de la session (pas de contournement, `regles-reseau.md`) ; la CI de branche n'est pas permise pour ce ticket. **La compilation de l'APK avec la bibliothèque n'est donc prouvée que par la CI de `main` après la fusion** (risque : une signature de Billing 8 différente de mes copies → CI de `main` rouge, je corrige tout de suite tant que ma session existe). Capture 2 = lignes exactes du code + même séquence rejouée sur la JVM ; le vrai `logcat` : APK de `main` (émulateur des bancs du matin) et WFS-515 (comptes de test de la Play Console) ;
  - **version 8.0.0** : la dernière 8.x stable n'a pas pu être lue ici (Maven de Google bloqué) ; une montée de version mineure est sans risque pour ce code ;
  - tant que les produits ne sont pas créés dans la Play Console (WFS-515), Google ne rend aucun prix : tous les packs seront masqués (comportement voulu, RG-03) ;
  - **à signaler au PO (hors zone E7)** : `crediterAchat` (WFS-510) attend l'écriture de la sauvegarde mais n'en reçoit pas le résultat ; si le disque est plein, l'achat serait consommé sans étoiles gardées. Correction possible dans `sauvegarde.*` (zone E3) : petit ticket proposé au PO ;
  - pas de serveur de vérification (RC-E21-07, accepté).

## Tests CPU

Branche synchronisée avec `main` (code testé : commit dd466989, après le 1er retour du lead dev ; les commits suivants ne changent que `qa/pr/WFS-511`) ; journal complet : [`tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-511/qa/pr/WFS-511/tests-cpu.log).

```
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 142 test(s) réussi(s) en 487 s
```

## Captures pour le lead dev (3, textes : ce ticket n'a pas d'écran)

Dossier : [`qa/pr/WFS-511`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-511/qa/pr/WFS-511).

| Capture | Ce qu'elle montre |
|---|---|
| 1 — [tests JVM](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-511/qa/pr/WFS-511/texte-1-tests-jvm.md) | `LogiqueAchatsTest` : 12 cas OK (créditer puis consommer, rattrapage, en attente, annulé, même jeton deux fois, restauration, hors ligne), avec les lignes `ACHATS` du journal de chaque cas |
| 2 — [journal du démarrage](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-511/qa/pr/WFS-511/texte-2-journal-demarrage.md) | lignes exactes écrites par `Achats.java` (connexion, produits demandés, rattrapage) + même séquence rejouée sur la JVM ; aucun jeton complet. **Pas un vrai logcat** (limite ci-dessus) |
| 3 — [banc PC `market` avant / après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-511/qa/pr/WFS-511/texte-3-banc-market.md) | `main` et branche : `market.txt` identiques octet pour octet (`cmp`), code 0 |

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_01Us6uCtfaZCtBn6FQY7UQw9
