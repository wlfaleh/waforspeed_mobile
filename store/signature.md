# Signature des versions de publication (WFS-22 / WFS-25, RG-02)

Aucune clé ni profil n'est dans le dépôt. La CI signe avec les **secrets du dépôt** (Settings › Secrets and variables ›
Actions) ; sans ces secrets, elle crée une **clé factice** jetable pour vérifier que l'AAB signé se construit.

## Android (AAB pour Google Play)
| Secret | Contenu |
|---|---|
| `WFS_CLE_ANDROID` | fichier `.jks` encodé en base64 (`base64 -w0 publication.jks`) |
| `WFS_CLE_MDP` | mot de passe du magasin de clés |
| `WFS_CLE_ALIAS` | alias de la clé |
| `WFS_CLE_ALIAS_MDP` | mot de passe de la clé (souvent le même) |

Créer la clé une fois, sur son ordinateur (à garder hors du dépôt, avec une sauvegarde) :
```
keytool -genkeypair -v -keystore publication.jks -alias waforspeed -keyalg RSA -keysize 4096 -validity 10000
```
La CI (`build.yml`, job `android`) construit `WaForSpeed.aab` (`gradle bundleRelease`), retire la signature de
débogage et signe avec `jarsigner` (clé du secret, sinon clé factice « CN=WaForSpeed test »). L'AAB est publié avec
l'APK dans la Release d'aperçu. Conseillé : activer la **signature d'application par Google Play** (Play App Signing),
la clé ci-dessus devient alors la clé d'importation.

L'**APK d'aperçu** garde la signature de débogage (installation et mises à jour sur le téléphone de test inchangées).

## iOS (App Store / TestFlight)
Pas de compte Apple fourni : la CI (`ios.yml`) construit pour le **simulateur** sans signature et produit l'archive
`WaForSpeed-ios-simulateur.zip`. Pour TestFlight, il faudra :
1. un compte Apple Developer (99 $ par an) et l'identifiant `com.waforspeed.game` dans App Store Connect ;
2. un certificat de distribution (`.p12`) et un profil de provisionnement App Store ;
3. les secrets `WFS_IOS_P12` (base64), `WFS_IOS_P12_MDP`, `WFS_IOS_PROFIL` (base64), `WFS_IOS_EQUIPE` (Team ID),
   et une clé d'API App Store Connect (`WFS_ASC_CLE`, `WFS_ASC_ID`, `WFS_ASC_EMETTEUR`) pour l'envoi ;
4. dans `ios.yml` : importer le certificat dans un trousseau temporaire, copier le profil, puis
   `xcodebuild -archivePath … archive` pour `generic/platform=iOS` avec `CODE_SIGN_STYLE=Manual`, `xcodebuild -exportArchive`
   (méthode `app-store`) et `xcrun altool --upload-app` (ou `xcrun notarytool`/Transporter).
