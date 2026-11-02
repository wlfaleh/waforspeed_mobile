# WaForSpeed Mobile

Version mobile de **WaForSpeed**, la course de rue arcade en 3D, de nuit. Elle est tirée du projet [`wlfaleh/waaps`](https://github.com/wlfaleh/waaps), qui reste inchangé.

Elle existe sous deux formes, à partir du même code :

| Forme | iPhone / iPad | Android | Comment l'avoir |
|---|---|---|---|
| **Web app installable** | ✅ | ✅ | Ouvrir la page GitHub Pages du dépôt, puis l'installer sur l'écran d'accueil |
| **Appli Android (APK)** | — | ✅ | Télécharger `WaForSpeed.apk` dans les *Releases* du dépôt |

## Installer sur le téléphone

### iPhone / iPad (Safari)
1. Ouvre la page du jeu : `https://<compte>.github.io/<dépôt>/`.
2. Touche **Partager**, puis **Sur l'écran d'accueil**.
3. Lance WaForSpeed depuis son icône : il s'ouvre en plein écran, sans barre Safari.

### Android
- **Web app** : ouvre la page dans Chrome, puis menu ⋮ → **Installer l'application**.
- **Appli (APK)** : dans l'onglet **Releases** du dépôt, télécharge `WaForSpeed.apk`, ouvre-le, puis autorise l'installation depuis cette source si Android le demande.

## Ce qui change par rapport à waaps

- **Marche hors ligne.** three.js, PeerJS et les polices sont copiés dans `www/lib`, sans CDN. Une fois la web app chargée, un service worker (`www/sw.js`) la garde en cache, et les voitures sont gardées dès leur premier chargement.
- **Toujours à l'horizontale.** L'appli Android est bloquée en paysage. La web app passe en plein écran à l'horizontale au premier « Course solo » sur Android. Si on tourne le téléphone à la verticale en pleine course, un message **« Tourne ton téléphone »** s'affiche et la partie se met en pause.
- **Écrans revus pour les petites hauteurs** (téléphone à l'horizontale, de l'iPhone SE au Pixel 7) : menu, showroom, choix du terrain, pause. En course, le nitro et le compteur sont placés entre le volant et les pédales, et les boutons tactiles sont plus grands.
- **Écran toujours allumé** pendant la course (Wake Lock, et un réglage natif dans l'appli). Pas de zoom au pincement ni de menu au toucher long.
- **Bouton « retour » d'Android** dans l'appli : il met la course en pause au lieu de quitter.
- **Correctif** : le jeu ne plante plus si on tourne le téléphone pendant le chargement (`initRain` appelé trop tôt).
- **Duel en ligne** : connexion directe entre téléphones (PeerJS), comme sur GitHub Pages. Il faut Internet.

## Organisation

| Chemin | Rôle |
|---|---|
| `www/` | Le jeu (même organisation que `waforspeed/` dans waaps), plus `js/mobile.js`, `css/mobile.css`, `manifest.webmanifest`, `sw.js`, `icons/`, `lib/` |
| `www/lib/` | Bibliothèques et polices copiées par `npm run vendor` (versions exactes dans `package.json`) |
| `android/` | Projet Android natif (Capacitor 8) : plein écran, horizontal, écran allumé (`MainActivity.java`) |
| `assets/` | Images sources de l'icône et de l'écran de démarrage Android |
| `scripts/vendor.mjs` | Copie les bibliothèques dans `www/lib` et génère la liste du mode hors ligne (`www/sw-files.js`) |
| `.github/workflows/pages.yml` | Publie `www/` sur GitHub Pages à chaque mise à jour de `main` |
| `.github/workflows/android.yml` | Fabrique l'APK à chaque mise à jour de `main` et le publie dans *Releases* |

## Mise en route du dépôt (une seule fois)

1. **GitHub Pages** : Settings → Pages → Source : **GitHub Actions**.
2. **APK** : rien à faire, il est fabriqué automatiquement. Sans clé de signature, c'est un APK de test : pour le mettre à jour, il faut désinstaller l'ancienne version.
3. **(Conseillé) Clé de signature**, pour que les mises à jour s'installent par-dessus l'ancienne version. Sur ton ordinateur :
   ```bash
   keytool -genkeypair -v -keystore waforspeed.keystore -alias waforspeed -keyalg RSA -keysize 2048 -validity 10000
   base64 -w0 waforspeed.keystore   # (macOS : base64 -i waforspeed.keystore)
   ```
   Ensuite, dans Settings → Secrets and variables → Actions, ajoute `ANDROID_KEYSTORE_BASE64` (le texte base64), `ANDROID_KEYSTORE_PASSWORD`, `ANDROID_KEY_ALIAS` (`waforspeed`) et `ANDROID_KEY_PASSWORD`.
   **Garde le fichier `.keystore` en lieu sûr** : sans lui, plus aucune mise à jour possible sur Google Play.

## Développer

```bash
npm ci
npm run vendor        # après un changement de bibliothèque ou l'ajout d'un fichier dans www/
npm run serve         # puis ouvrir http://localhost:8000 (mode « téléphone » des outils de développement)
```

Fabriquer l'APK en local : il faut Android Studio (ou le SDK Android) et Java 21.
```bash
npm run android:debug   # → android/app/build/outputs/apk/debug/app-debug.apk
npx cap open android    # ouvrir le projet dans Android Studio
```

Après une modification du jeu, change `VERSION` dans `www/sw.js` : les web apps déjà installées se mettront à jour.

## Et une vraie appli iOS ?

La web app installée est déjà l'expérience iPhone : plein écran, icône, mode hors ligne. Pour une appli sur l'App Store, il faut un Mac avec Xcode et un compte Apple Developer (99 $ par an). Le projet Capacitor est prêt pour ça : `npm i @capacitor/ios && npx cap add ios`, puis la publication depuis Xcode.

## Licences

Les modèles 3D gardent leur licence d'origine (`www/models/*/license.txt`, détail dans `www/models/README.md`). Deux sont non commerciaux (CC BY-NC-SA) : **le jeu doit rester gratuit**, y compris sur les stores. Les polices sont sous licence SIL OFL 1.1, three.js et PeerJS sous licence MIT.
