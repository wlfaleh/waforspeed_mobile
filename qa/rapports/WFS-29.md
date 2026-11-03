# QA WFS-29 : Android, mode plein écran immersif, icône, nom

**Verdict : VALIDÉ** (banc d'essai PC + revue du code et de l'APK), 1er passage. **Fusion autorisée.**

- Construction regardée : commit `45f3d8a` (`feature/waforspeed-29`, PR #17), run Actions 41618704670 réussi
  (jobs `banc-essai`, `android`, `publication` au vert). Release `apercu-waforspeed-29` (texte : « commit 45f3d8a »),
  32 captures + `WaForSpeed.apk`.
- Règles : `regles-de-gestion/WFS-29.md` (RG-01 à RG-05).
- Code changé (`git diff origin/main...45f3d8a`) : `android/` (manifeste, `JeuActivity.java`, icône, couleurs) et
  `tools/qa/emulateur.sh` seulement. Aucun fichier `src/`, `shaders/`, `assets/`, `CMakeLists.txt` modifié. La branche
  se fusionne sans conflit sur `main` (`git merge-tree`).

## Preuves

| Preuve | Constat |
|---|---|
| CI `android` | `compileReleaseJavaWithJavac`, `dexBuilderRelease`, `mergeDexRelease` exécutés, `BUILD SUCCESSFUL` : le Java compile et entre dans l'APK. |
| CI `banc-essai` | compilation, test de la piste, captures : toutes les étapes réussies, 32 PNG produits jusqu'à `texte-essai-2400` (10:36). |
| Captures du banc | Comparaison pixel par pixel avec `apercu-waforspeed-44` (commit `240ca10`, dont `src/ shaders/ assets/ android/ tools/ .github/ CMakeLists.txt` sont identiques à la base `32eb391` de WFS-29) : **32/32 captures identiques** (0 pixel différent, mêmes tailles). Coup d'œil sur `vitrine-m4-0`, `course-cam0-1`, `trafic-cam0`, `course-fin` : rendu normal, voiture visible, pas d'écran noir. |
| APK (`python3 -I`, zipfile) | `classes.dex` contient `Lcom/waforspeed/game/JeuActivity;` ; `resources.arsc` contient `ic_launcher`, `ic_launcher_round`, `ic_avant_plan`, `fond`, `WaForSpeed` ; le manifeste binaire cite `JeuActivity` et `android.app.lib_name` = `waforspeed` ; `lib/arm64-v8a` et `lib/x86_64/libwaforspeed.so` présents. `aapt` / `aapt2` / `apkanalyzer` **non disponibles** ici : pas de décodage complet du manifeste binaire. |
| Icône redessinée (PIL, d'après `ic_avant_plan.xml`) | Fond `#07070C`, « W » orange `#FF6A00` incliné + soulignement, 3 lignes de vitesse cyan `#00E5FF`. Point le plus éloigné du centre : 32,9 dp (bout arrondi de la ligne du haut) pour 33 dp autorisés : tout tient dans la zone sûre de 66 dp. Aucun logo tiers. |

## Critères d'acceptation et règles

| Critère / règle | Constat | Résultat |
|---|---|---|
| Manifeste : `android:label="WaForSpeed"` | `@string/app_name` = « WaForSpeed » (`values/strings.xml`), présent dans `resources.arsc` | OK |
| Manifeste : icône adaptative | `android:icon="@mipmap/ic_launcher"`, `android:roundIcon="@mipmap/ic_launcher_round"`, `mipmap-anydpi-v26` avec `background` (`@color/fond`), `foreground` et `monochrome` (`@drawable/ic_avant_plan`) ; minSdk 26 donc toujours la version adaptative | OK |
| Manifeste : thème plein écran | `@style/Jeu` (`Theme.Black.NoTitleBar.Fullscreen`, `windowFullscreen`), `values-v28` : `windowLayoutInDisplayCutoutMode = shortEdges` | OK |
| Manifeste : activité, paysage | `.JeuActivity` (sous-classe de `NativeActivity`, `hasCode="true"`, `lib_name` = `waforspeed` conservé), `MAIN`/`LAUNCHER`, `sensorLandscape` | OK |
| RG-01 immersif collant au démarrage et à `onWindowFocusChanged` | `pleinEcran()` appelé dans `onCreate`, `onResume` et `onWindowFocusChanged(focus=true)` ; Android 11+ : `WindowInsetsController.hide(statusBars | navigationBars)` + `BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE` ; avant : `SYSTEM_UI_FLAG_IMMERSIVE_STICKY` + `FULLSCREEN` + `HIDE_NAVIGATION` + drapeaux `LAYOUT_*` | OK (revue du code) |
| RG-02 image sous l'encoche | `LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES` (code, Android 9+) en plus du thème | OK (revue du code) |
| RG-02 « le HUD respecte les marges » | Non traité : le HUD est hors de la zone de code E7 ; signalé par le développeur au PO (PR #17). Le mode `shortEdges` était déjà dans le thème de `main` avant ce ticket : pas de régression introduite. | Remarque (voir plus bas) |
| RG-03 paysage + écran allumé | `sensorLandscape` ; `FLAG_KEEP_SCREEN_ON` dans `onCreate` | OK |
| RG-04 nom, icône, couleurs, aucun logo tiers | voir Preuves | OK |
| RG-05 banc et simulateur démarrent, moteur inchangé | banc au vert, captures identiques ; `tools/qa/emulateur.sh` lance `com.waforspeed.game/.JeuActivity` (plus `android.app.NativeActivity`, qui n'est plus déclarée) ; aucune autre référence à l'ancienne activité dans `.github/` ni `tools/` | OK |
| Captures du banc PC inchangées | 32/32 identiques au pixel | OK |

## Vérifiable seulement au simulateur (WFS-50, fin d'épic E7)
- barres d'état et de navigation réellement masquées à l'écran, retour un instant au balayage puis nouveau masquage ;
- maintien après une mise en arrière-plan / reprise ;
- icône et nom « WaForSpeed » sur le lanceur (forme du masque, rendu de la version monochrome) ;
- démarrage effectif de `.JeuActivity` par `emulateur.sh` sans plantage (`qa-logcat.txt`) ;
- HUD et commandes tactiles hors de l'encoche (RG-02, voir remarque).

## Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| — | — | — | Aucune anomalie. |

Remarque pour le PO (pas une anomalie de ce ticket) : la partie « le HUD respecte les marges » de RG-02 relève de la zone
HUD (hors E7). À prévoir dans un ticket de la bonne épic ou à vérifier sur capture du simulateur avec encoche au
ticket WFS-50.
