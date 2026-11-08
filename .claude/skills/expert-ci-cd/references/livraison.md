# Livraison continue (CD) de WaForSpeed Mobile — versions, signature, Google Play, mises à jour

À relire avant tout travail de publication (WFS-319, WFS-320, WFS-279) et avant de répondre à une question de l'utilisateur sur une mise à jour.
Toujours vérifier l'état réel (Play Console décrite par l'utilisateur, Releases GitHub, secrets présents ou non) : ne jamais supposer.

## 1. Les deux numéros d'une version Android
| Numéro | Où | Règle |
|---|---|---|
| `versionCode` | `android/app/build.gradle` : `GITHUB_RUN_NUMBER` | **Toujours croissant.** Google Play refuse un code égal ou plus petit qu'une version déjà envoyée ; le téléphone refuse d'installer par-dessus une version plus récente. L'APK des bancs du matin reprend le numéro du dernier run de `build.yml` (`matin.yml`, job `heure`). |
| `versionName` | même fichier | Le numéro montré au joueur : 0.1.<run> pour les aperçus, **1.0.<run> pour le store** (WFS-319) ; affiché à l'écran de chargement (WFS-279, ticket jeu). |

Même commit = même version (pas de nouveau numéro pour un APK identique). Aucun robot n'écrit de fichier de version sur `main` (décision de
l'utilisateur : pas d'`info.json`).

## 2. Signature
- **APK d'aperçu** (Release `apercu`) : clé de test fixe du dépôt (WFS-118), empreinte vérifiée à chaque construction. Ne jamais la changer :
  sinon les téléphones refusent la mise à jour par-dessus.
- **AAB du store** : clé d'envoi (« upload key ») dans les secrets du dépôt `WFS_CLE_ANDROID` (keystore en base64), `WFS_CLE_MDP`,
  `WFS_CLE_ALIAS`, `WFS_CLE_ALIAS_MDP` (`store/signature.md`). **Sans ces secrets, la publication échoue avec un message clair** : jamais de
  clé factice pour un AAB destiné au store.
- **Signature par Google Play** (« Play App Signing ») : conseillée et obligatoire pour un AAB ; Google garde la clé de l'application,
  l'utilisateur ne garde que la clé d'envoi. Une clé d'envoi perdue se remplace par une demande à Google ; une clé d'application perdue sans
  Play App Signing = plus aucune mise à jour possible.
- **Jamais** de clé, de mot de passe ou de fichier `.jks` / JSON de compte de service dans le dépôt, un message, le journal ou une Release.
  Seulement les secrets du dépôt, mis par l'utilisateur lui-même.

## 3. Publier sur Google Play (premier envoi, WFS-319)
1. La CI (`publication.yml`, lancé à la main sur `main`) construit l'AAB 1.0.<run>, le signe avec la clé d'envoi, vérifie : signature,
   targetSdk 35, pages de 16 Ko (`tools/qa/pages_16k.sh`), et le dépose dans la Release `play-<version>` avec ses empreintes SHA-256.
2. L'utilisateur crée l'application dans la Play Console, active la signature par Google Play, **téléverse l'AAB à la main** (le premier envoi
   ne peut pas être fait par l'API), dans **Test interne**, et ajoute les testeurs (adresses e-mail ou groupe Google).
3. Avant la production : fiche du store (`store/fiche-fr.md`), captures, icône, politique de confidentialité publiée à une adresse publique,
   questionnaire de classification (IARC), « Sécurité des données » (d'après `store/confidentialite.md`), public cible, annonces : non (WFS-320).
4. Production : depuis la version testée, en **déploiement progressif** (ex. 10 % → 50 % → 100 %), avec possibilité d'arrêter.
   Revue de Google : de quelques heures à quelques jours (plus long pour un tout premier compte).

## 4. Mettre à jour le jeu (chaque nouvelle version)
1. `versionCode` plus grand (automatique), même clé d'envoi.
2. Construire l'AAB (`publication.yml`), notes « Nouveautés » en français.
3. Test interne d'abord, vérification sur le téléphone de l'utilisateur, puis production en déploiement progressif.
4. Si les données collectées ou les autorisations changent : mettre à jour « Sécurité des données » et la politique de confidentialité **avant**.
5. La sauvegarde des joueurs doit passer d'une version à l'autre (sauvegarde versionnée et migrée : règle du moteur).
6. Conseil : une mise à jour par semaine au plus au début, le temps de lire les retours.

## 5. Automatiser l'envoi (après le premier envoi manuel)
| Niveau | Ce que fait la CI | Ce que fait l'utilisateur |
|---|---|---|
| 1 (WFS-319) | AAB signé dans une Release | téléverse dans la Play Console |
| 2 (recommandé ensuite) | envoie l'AAB en **Test interne** par l'API Google Play Developer (compte de service, clé JSON dans un secret du dépôt) | crée le compte de service et lui donne le droit « Gérer les versions de test » |
| 3 | passage en **production** en déploiement progressif | **lance lui-même** ce bouton : un robot n'envoie jamais seul en production |
Outils possibles : action GitHub d'envoi vers Google Play ou `fastlane supply` ; versions épinglées, comme toutes les actions du dépôt.
Le niveau 2 peut être préparé sans être actif : il ne s'active que si le secret du compte de service existe.

## 6. iOS (gelé tant que l'utilisateur n'a pas validé Android)
Le jour venu : compte Apple Developer (99 $ par an), certificat et profil de distribution (secrets), numéro de version + numéro de construction
croissants, envoi vers App Store Connect (clé d'API dans un secret), **TestFlight** pour le test, revue Apple (1 à 3 jours, règles plus strictes),
publication progressive sur 7 jours possible. Machines macOS dans GitHub Actions : une minute compte comme ≈ 10 minutes Linux sur un compte gratuit.
Rien de tout cela ne se prépare pendant le gel iOS (`CLAUDE.md`).

## 7. Ce qui reste à l'utilisateur (jamais fait par un agent)
Compte Play Console (et Apple plus tard), acceptation des conditions de Google, paiement, mise des secrets, adresse publique de la politique de
confidentialité, réponses légales (classification, sécurité des données) relues, bouton de production.
