# Checklist des stores — WaForSpeed (WFS-22 / WFS-25)

Coché = prêt dans le dépôt ou produit par la CI. Non coché = à faire par l'éditeur (compte, soumission : hors périmètre).

## Commun
- [x] Nom « WaForSpeed », identifiant `com.waforspeed.game`
- [x] Icônes au style du jeu, sans logo tiers : `store/icones/icone-play-512.png` (512 × 512), `store/icones/icone-ios-1024.png`
      (1024 × 1024, opaque), dessinées par `tools/qa/icones_store.py` depuis l'icône adaptative Android
- [x] Textes français : `store/fiche-fr.md` (description courte et longue, mots-clés, catégorie)
- [x] Captures paysage du banc PC : `store/captures/` (à compléter par le simulateur de fin d'épic E7)
- [x] Crédits des 20 voitures, personnages, sons, police : `store/credits.md` ; dans l'application : `assets/licences/`
      (`CREDITS.txt` + une licence par modèle) et crédit de chaque voiture affiché au garage
- [x] Vérification croisée automatique : `python3 tools/qa/verifier_licences.py` (CI, job « texte ») : chaque modèle,
      son, personnage et police livré a sa licence ; crédits complets
- [x] Jeu gratuit, sans achat intégré ni publicité (licences NC) ; versions modifiées SA partagées sous la même licence
- [x] Politique de confidentialité et classification d'âge : `store/confidentialite.md`
- [ ] Publier la politique de confidentialité à une adresse publique (ex. GitHub Pages) et renseigner un contact
- [x] Version : `versionCode` = numéro de construction de la CI (croissant), `versionName` 0.1.N ; iOS : même numéro
      de build (`CFBundleVersion`). Passer `versionName` / `CFBundleShortVersionString` à 1.0 à la première soumission

## Google Play
- [x] AAB signé produit par la CI (`WaForSpeed.aab` dans la Release d'aperçu ; clé factice tant que les secrets
      `WFS_CLE_*` ne sont pas fournis : `store/signature.md`)
- [x] `targetSdk` 35 et `compileSdk` 35 (Android 15, exigé par Google Play pour les nouvelles applications et les mises à
      jour ; WFS-129, 2026-11-05), `minSdk` 26, 64 bits (arm64-v8a ; x86_64 pour l'émulateur), Vulkan 1.0.3 exigé (`uses-feature`)
- [x] Pages mémoire de 16 Ko (WFS-129, 2026-11-05) : `libwaforspeed.so` construit avec `ANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES`
      (NDK r27), bibliothèques non compressées ; la CI (job `android`) vérifie chaque segment `LOAD` (≥ `2**14`) de l'APK et
      de l'AAB, et `zipalign -c -P 16 -v 4` (`tools/qa/pages_16k.sh`)
- [x] Bord à bord d'Android 15 : plein écran immersif gardé, image sous l'encoche (`ALWAYS`), marges sûres envoyées au jeu ;
      touche Retour comme WFS-116, sans retour prédictif (`enableOnBackInvokedCallback` à `false`)
- [x] Paysage, plein écran, encoche, manettes (`android.hardware.gamepad` non exigé : le tactile suffit)
- [ ] Fournir la clé de publication en secrets du dépôt (`WFS_CLE_ANDROID`, `WFS_CLE_MDP`, `WFS_CLE_ALIAS`, `WFS_CLE_ALIAS_MDP`)
- [ ] Compte Google Play Console, fiche, questionnaire IARC, sécurité des données, test interne puis production

## App Store
- [x] Application iOS construite par la CI (`ios.yml`, macOS, non bloquant) : `WaForSpeed-ios-simulateur.zip`,
      `WaForSpeed-ios-non-signe.ipa` ; paysage verrouillé, plein écran, zone de sécurité, tactile multi-doigts
- [x] `ITSAppUsesNonExemptEncryption` = non (aucun chiffrement propre), `UIRequiredDeviceCapabilities` = metal
- [ ] Rendu à l'écran sur iOS : surface Metal dans `VkCtx` (`VK_EXT_metal_surface`, `src/render/vk.cpp`, ticket à créer
      hors E7) ; en attendant, rendu hors écran recopié dans la vue (2 images par seconde)
- [ ] Son iOS (Core Audio / AVAudioEngine derrière `audio_out.h`) : ticket E6 à créer ; aujourd'hui muet sur iOS
- [ ] Compte Apple Developer, certificat et profil en secrets (`store/signature.md`), TestFlight
- [ ] Captures 6,7 pouces (2796 × 1290) et 13 pouces iPad prises sur le simulateur iOS une fois le rendu à l'écran branché
