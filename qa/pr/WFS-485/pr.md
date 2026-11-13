## Agent : Claude, agent dev3d (01UY1Vv46n)

**Ticket** : WFS-485 — règles : `regles-de-gestion/WFS-485.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : désinstaller, réinstaller ou changer de téléphone ne doit plus faire perdre profil, records et réglages (sauvegarde automatique Android, seulement le fichier de sauvegarde).
- **Ce qui change** (fichiers, comportement, à l'écran) :
  - `android/app/src/main/AndroidManifest.xml:18-20` : `android:allowBackup="false"` → `"true"`, plus `android:dataExtractionRules="@xml/regles_sauvegarde"` et `android:fullBackupContent="@xml/sauvegarde_ancienne"` (RG-01).
  - `android/app/src/main/res/xml/regles_sauvegarde.xml` (nouveau, Android 12 et plus) : `<cloud-backup>` **et** `<device-transfer>` avec deux règles `<include domain="file" path="sauvegarde.txt" />` et `<include domain="file" path="reglages.txt" />`.
  - `android/app/src/main/res/xml/sauvegarde_ancienne.xml` (nouveau, Android 11 et moins, minSdk 26) : mêmes deux `<include>` (`sauvegarde.txt`, `reglages.txt`).
  - Chemin vérifié dans le code : `src/platform/android_main.cpp:351` charge `internalDataPath + "/sauvegarde.txt"` (= `getFilesDir()`, domaine `file`) ; profil, records, voiture, peintures et la plupart des réglages y sont ; **retour lead dev** : cinq réglages ne vivent que dans `files/reglages.txt` (`src/ui/ecrans/reglages.cpp:160`, lignes 181-185 : réduire les mouvements, Ultra, regard Cockpit, fantôme affiché, onglet), d'où la 2e règle. RG-05 tenue sans WFS-480.
  - Une règle `include` exclut tout le reste (RG-02) : `files/paquets/` (`.wfcarte`, `.part`), `pipelines.cache` (shaders compilés), `sauvegarde.txt.tmp`, `fantome-*.bin`, `plantage*.txt`, caches, fichiers externes (`configgame.json`, `rendu-mesure.txt`).
  - Rien ne change à l'écran ; aucun code C++, aucune CI touchés.
- **Comment c'est vérifié** (tests CPU, parcours, bancs, mesures) :
  - Les trois XML se lisent sans erreur (analyseur XML) ; attributs conformes à la doc Android (Auto Backup, `dataExtractionRules` lu à partir d'Android 12 / cible 31+, `fullBackupContent` en dessous).
  - Banc `sauvegarde` lancé deux fois (record puis relecture) et `menu1` en 1280x720, branche et `main` : images **identiques octet pour octet** (aucun rendu touché).
  - RG-03 : fichier `sauvegarde.txt` écrit par le banc = **323 octets**, 27 lignes ; `reglages.txt` = 14 lignes `clé=valeur` (≈ 150 octets) ; total très loin de 64 Ko (Android accepte 25 Mo).
  - **Essai pour l'utilisateur (preuve principale, ticket 🟡)** : 1) battre un record ; 2) téléphone branché au PC : `adb shell bmgr backupnow com.waforspeed.game` (ou laisser le téléphone en charge et en Wi-Fi une nuit) ; 3) désinstaller ; 4) réinstaller l'APK de `main` ; 5) au lancement, le record, la voiture, la peinture et tous les réglages (Ultra, regard Cockpit, réduire les mouvements, fantôme compris) reviennent (le pseudo aussi quand WFS-480 sera fusionné). La sauvegarde Google doit être activée dans les réglages du téléphone.
- **Limites et risques connus** :
  - **Pour le PO** : RG-01 ne citait que `sauvegarde.txt` ; `reglages.txt` est ajouté pour tenir le besoin (« ses réglages reviennent »), sur demande du lead dev.
  - **APK non construit en local** (pas de SDK Android dans le conteneur) : la construction et la liaison des ressources se vérifient à la CI de `main` après la fusion.
  - **Pas d'émulateur ici** : l'essai désinstaller / réinstaller n'a pas été fait par le développeur ; il est décrit ci-dessus pour l'utilisateur.
  - Les fantômes (`fantome-<carte>.bin`) ne sont **pas** sauvegardés (RG-01 : seulement les fichiers de sauvegarde) : le record revient, son fantôme non. À proposer au PO si besoin.
  - Android copie la sauvegarde environ une fois par jour : une partie jouée juste avant la désinstallation peut manquer.
  - À déclarer dans le formulaire « Sécurité des données » de Google Play (sauvegarde Android vers le compte Google, aucun serveur à nous) : signalé au PO / juriste.

## Tests CPU

Commit testé : `b5df9a18` (branche synchronisée avec `main`), `bash tools/qa/tests_cpu.sh build` — journal complet : [`qa/pr/WFS-485/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-485/qa/pr/WFS-485/tests-cpu.log)

```
[OK] test_ville_lieux (0 s)
[OK] test_ville_sol (2 s)
[OK] test_virages_fleches (0 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 115 test(s) réussi(s) en 301 s
```

## Captures pour le lead dev (6 après)

Dossier : [`qa/pr/WFS-485`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-485/qa/pr/WFS-485) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `fin-menu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-485/qa/pr/WFS-485/avant-fin-menu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-485/qa/pr/WFS-485/apres-fin-menu.jpg?raw=true) |
| `garage-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-485/qa/pr/WFS-485/avant-garage-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-485/qa/pr/WFS-485/apres-garage-0.jpg?raw=true) |
| `menu-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-485/qa/pr/WFS-485/avant-menu-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-485/qa/pr/WFS-485/apres-menu-0.jpg?raw=true) |
| `terrain-0` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-485/qa/pr/WFS-485/avant-terrain-0.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-485/qa/pr/WFS-485/apres-terrain-0.jpg?raw=true) |
| `fin-record` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-485/qa/pr/WFS-485/avant-fin-record.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-485/qa/pr/WFS-485/apres-fin-record.jpg?raw=true) |
| `sauvegarde-relue` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-485/qa/pr/WFS-485/avant-sauvegarde-relue.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-485/qa/pr/WFS-485/apres-sauvegarde-relue.jpg?raw=true) |

Les 6 paires avant / après sont identiques octet pour octet (aucun rendu touché : seuls le manifeste et deux XML Android changent).
