# WFS-22 — Version iOS (Metal via MoltenVK)
Épic : E7 · Priorité : 24 · Dépend de : WFS-29

## Besoin (qui, quoi, pourquoi)
Le même jeu fonctionne sur iPhone/iPad : rendu Vulkan traduit par MoltenVK, mêmes règles et mêmes captures.

## Règles de gestion
- RG-01 : un seul code de jeu pour Android et iOS ; seule la couche `src/platform` et le projet de construction diffèrent ; pas de moteur de jeu tiers.
- RG-02 : rendu identique au banc PC : profondeur inversée, HDR, MSAA, passe finale ACES ; extensions Vulkan absentes de MoltenVK gérées par repli.
- RG-03 : paysage verrouillé, zone de sécurité (encoche, indicateur d'accueil), tactile multi-doigts comme Android.
- RG-04 : audio via Core Audio ou AVAudioEngine (même interface que `audio_out.h`) ; sauvegarde dans le dossier de l'application.
- RG-05 : construction par le CI sur macOS (xcodebuild / CMake iOS) ; signature manuelle documentée ; test TestFlight.
- RG-06 : assets partagés (`.wfm`, shaders SPIR-V, sons) ; licences des modèles incluses.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- Aucun code JS ni modèle propre à ce ticket : les règles de jeu et modèles (`models/`) arrivent via les autres tickets.
- Retour connu : les valeurs de `pixelRatio` et le budget mobile du skill `waforspeed-3d` (§8) s'appliquent aussi à l'iPhone.

## Critères d'acceptation
- [ ] workflow CI iOS vert, artefact `.ipa`/archive produit
- [ ] captures du banc (build iOS ou simulateur) comparables aux captures du banc PC pour les mêmes scènes
- [ ] aucun plantage au démarrage, en paysage, au tactile (rapport QA de fin d'épic E7)

## Hors périmètre
Publication App Store (WFS-25), manettes iOS, iPadOS multitâche.


## Périmètre élargi (décision de l'utilisateur 2026-11-03 : tickets plus gros, moins de builds)
Ce ticket absorbe : WFS-23, WFS-25. **Lire et appliquer aussi** `regles-de-gestion/WFS-23.md`, `regles-de-gestion/WFS-25.md` (toutes leurs règles et critères). Une seule branche, une seule CI à la fin ; livrer par morceaux sur la branche sans lancer de CI intermédiaire.

## Ordre de travail et précisions du PO (2026-11-04, attribution)
- RG-07 : **ordre** : (1) manettes Android (WFS-23, testable par l'utilisateur sur son téléphone) ; (2) stores (WFS-25 : `store/`, `assets/licences/`, AAB signé avec clé factice) ; (3) iOS (job macOS séparé). Livrer par morceaux sur la branche, une seule CI à la fin.
- RG-08 : le job iOS est **non bloquant** pour la CI Android (`continue-on-error` ou workflow à part) : son échec ne doit jamais empêcher l'APK `apercu`. Critère iOS minimal accepté : compilation du jeu pour iOS (simulateur) + archive produite ; TestFlight et signature réelle restent documentés, pas exigés (pas de compte Apple fourni).
- RG-09 : manette = même `input` que le tactile (WFS-23 RG-06) ; branchement par accroche ≤ 10 lignes annoncée au Journal ; aucune modification de `src/game/player.*` / `direction.*` (WFS-70 en cours).
- RG-10 : « du jeu qui fonctionne » : pas de finitions d'icônes ni de textes marketing au-delà du nécessaire ; crédits des 20 voitures complets (exigence de licence).

### Zone de code autorisée
`src/platform/**`, `android/**`, `ios/**` (nouveau), `.github/**`, `tools/qa/**`, nouveaux `store/**` et `assets/licences/**` ; accroches ≤ 10 lignes annoncées au Journal (entrée manette, focus des menus dans `src/ui/ecrans/`, `CMakeLists.txt` pour la cible iOS).

### Critères d'acceptation regroupés
- [ ] test CPU manette (zone morte 0,12, gâchettes, croix → `input`) ; capture de menu avec focus visible ; boutons tactiles masqués en mode manette
- [ ] `store/` : icônes 512 / 1024 px, textes FR, crédits des 20 voitures ; chaque modèle livré a sa licence dans `assets/licences/` ; AAB signé (clé factice) produit par la CI
- [ ] job iOS : compilation + archive produite, non bloquant ; CI Android verte, captures du banc inchangées (`0 écart(s)`)
