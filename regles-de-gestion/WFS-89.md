# WFS-89 — Duel par Internet : salon par code à 4 caractères, compatible avec le jeu web (PeerJS)
Épic : E3 (+ accroches E8 `src/ui/ecrans/salon.*`, E0 `CMakeLists.txt`, E7 `android/**`) · Priorité : P2 Haute · Dépend de : WFS-83 ✅, WFS-21 ✅

## Besoin (qui, quoi, pourquoi)
Le joueur veut défier un ami **qui n'est pas sur le même Wi-Fi** (WFS-83 ne marche qu'en local), et si possible un ami
qui joue au **jeu web** dans son navigateur. Le jeu web le fait déjà sans serveur à nous : `js/net/p2p.js` relie les
joueurs en WebRTC via le service gratuit de mise en relation de PeerJS ; le créateur du salon sert de relais.
Ce ticket ajoute un **second transport** `duel::Transport` (à côté de `TransportUdp`) qui parle le même protocole que
`p2p.js`, et une **saisie du code** dans l'écran Salon. Les règles du duel (`src/game/duel*`) ne changent pas.

## Règles de gestion
- RG-01 (même protocole que `p2p.js`) : le salon de code `abcd` a pour relais le pair PeerJS d'identifiant
  **`waforspeed-wfs-abcd-v1`** (`'waforspeed-' + 'wfs-' + code + '-v1'`). Rejoindre / créer : on tente d'abord de
  **devenir le relais** (ouvrir ce pair) ; si le serveur répond `ID-TAKEN` (`unavailable-id`), le salon existe : on
  ouvre un pair à identifiant aléatoire et on se **connecte au relais** (canal fiable). Délai total : **12 s**, sinon
  échec (note RG-07).
- RG-02 (messages, identiques au web) : invité → relais `{ t: 'p', presence: {…} }` (envoi groupé toutes les 30 ms au
  plus) ; relais → tous `{ t: 'all', list: [[id, presence], …] }` à chaque changement (le relais s'inclut en tête).
  Les champs de présence sont ceux de `duel.js` déjà portés (`n, st, r, d, x, v, h, b, ft, car, paint, host, map, tod,
  km`) : réutiliser `duel::Fields` et les contrôles de `duel::decode` (tout message invalide est ignoré, aucun plantage).
- RG-03 (sérialisation) : PeerJS sérialise par défaut en **BinaryPack** (`serialization: 'binary'`). Le transport
  encode / décode BinaryPack pour les types utilisés (objet, tableau, chaîne, nombre entier / flottant, booléen, null).
  Le natif ouvre ses connexions avec `serialization: 'binary'`, `reliable: true`, comme `me.connect(hubId, { reliable: true })`.
- RG-04 (mise en relation) : serveur PeerJS public **`0.peerjs.com:443`**, chemin `/peerjs`, clé `peerjs`, en WebSocket
  sécurisé (`wss://0.peerjs.com/peerjs?key=peerjs&id=<id>&token=<aléatoire>`) ; messages `OPEN`, `ID-TAKEN`, `OFFER`,
  `ANSWER`, `CANDIDATE`, `LEAVE`, `EXPIRE`, `HEARTBEAT` (toutes les 5 s) du protocole PeerJS 1.5.4 (version chargée par
  le web). Serveurs ICE = ceux de PeerJS 1.5.4 par défaut (STUN Google + TURN `eu-0.turn.peerjs.com` / `us-0.turn.peerjs.com`).
- RG-05 (bibliothèque) : WebRTC (canal de données seulement) et WebSocket via **libdatachannel** (MPL 2.0, sans
  médias, `NO_MEDIA=ON`, `NO_EXAMPLES=ON`, `NO_TESTS=ON`) ; ce n'est pas un moteur de jeu, c'est autorisé. TLS au
  choix (mbedTLS conseillé pour Android). Version figée. Licence ajoutée à la liste des licences de l'application.
- RG-06 (horloge commune) : comme le web, `nowMs()` = **horloge murale** de l'appareil (ms depuis 1970, `Date.now()`),
  pas l'horloge de l'hôte (les téléphones sont à l'heure par le réseau). Le reste (décompte, arrivée, classement) vient
  de `duel.*` tel quel.
- RG-07 (écran Salon, `src/ui/ecrans/salon.*`) : sous « Créer un salon », un bloc **« Rejoindre avec un code »** :
  4 cases et un **clavier à l'écran** des 31 caractères de `duel::CODE_CHARS` (+ effacer), bouton « Rejoindre »
  actif à 4 caractères. « Créer un salon » ouvre **les deux** transports avec le même code : UDP local (WFS-83, inchangé)
  et Internet (relais PeerJS) ; le premier qui reçoit un second joueur est gardé, l'autre est fermé. Note sous le code :
  « Envoie ce code à ton adversaire : il le tape dans « Rejoindre avec un code », sur téléphone ou sur le jeu web. »
  Sans Internet ou après 12 s : note orange « Pas de connexion au service de duel. Réessaie, ou jouez sur le même
  Wi-Fi. », le Wi-Fi local reste utilisable, aucun plantage.
- RG-08 (fils) : réseau sur le fil de libdatachannel ; le jeu ne lit qu'une copie protégée par verrou (comme
  `TransportUdp`) ; jamais d'attente bloquante dans la boucle de jeu. `leave()` ferme proprement (message `LEAVE`).
- RG-09 (Android) : la permission `INTERNET` est déjà dans `android/app/src/main/AndroidManifest.xml` (rien à ajouter) ; ne pas toucher au manifeste sauf besoin réel (≤ 2 lignes annoncées).
- RG-10 (zone) : E3 : `src/game/duel*`, `src/game/reseau_peerjs.*` (nouveau), `src/game/binarypack.*` (nouveau),
  `tests/**` (nouveau `test_binarypack` + essai du transport sur un relais simulé en mémoire) ; accroches annoncées au
  Journal : `src/ui/ecrans/salon.*` (E8, comme WFS-83), `CMakeLists.txt` ≤ 10 lignes (FetchContent / `third_party`),
  manifeste Android ≤ 2 lignes.
- RG-11 (banc) : le banc PC n'a pas forcément Internet : scène `WFS_BANC=salon` avec un **relais simulé** (même
  messages `t:'p'` / `t:'all'` passés par BinaryPack en mémoire) pour les captures ; si la CI a Internet, une ligne de
  journal « peerjs : OPEN reçu » est un plus, pas un critère.

## À réutiliser du jeu web (wlfaleh/waaps, waforspeed/)
- `js/net/p2p.js` : `joinP2P` (relais = premier arrivé, `unavailable-id` → client, `t:'p'` / `t:'all'`, délai 12 s,
  envoi groupé 30 ms, liste des partis `left`) → porter tel quel dans `reseau_peerjs.cpp`.
- `js/net/duel.js` : `room.join('wfs-' + code)`, code `/^[a-z0-9]{4}$/`, texte d'aide du salon P2P (ligne 216).
- `index.html` : `#codeIn` + `#joinBtn` (« Rejoindre avec un code ») → modèle du bloc de saisie.
- Natif existant à garder : `duel::Transport`, `duel::Fields`, `duel::decode`, `duel::makeCode`, `TransportUdp`
  (`src/game/reseau_udp.*`) pour la structure (verrou, copie, `leave`), écran `salon.cpp` de WFS-83.
- Protocole PeerJS : sources de `peerjs@1.5.4` (serveur, BinaryPack `peerjs-js-binarypack`) pour les formats exacts.

## Critères d'acceptation (vérifiables sur les captures du banc PC)
- [ ] `salon-code` : bloc « Rejoindre avec un code », 4 cases, clavier à l'écran lisible dans la zone sûre, rien ne déborde.
- [ ] `salon-code-saisi` : code de 4 caractères tapé, « Rejoindre » actif.
- [ ] `salon-internet` : salon rejoint par le relais simulé (2 pilotes listés, voiture et terrain de l'hôte).
- [ ] `salon-hors-ligne` : note orange RG-07, pas de plantage.
- [ ] `test_binarypack` : aller-retour des présences et décodage d'un message produit par le BinaryPack de PeerJS
      (octets de référence dans le test) ; tests CPU de `build.yml` verts.
- [ ] APK (vérifié par l'utilisateur) : deux téléphones sur deux réseaux différents (4G + Wi-Fi) font un duel avec le
      code ; un téléphone et le jeu web dans un navigateur se voient dans le même salon.

## Hors périmètre
- Serveur de mise en relation à nous, liste publique des salons par Internet (le web n'en a pas non plus en P2P).
- Voix, chat, plus de 2 joueurs testés (le protocole le permet, non vérifié).
- iOS (compilera plus tard ; ne pas casser `ios.yml` : transport désactivé sous `__APPLE__` si besoin).

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P2 Haute | WFS-89 | Duel par Internet : salon par code à 4 caractères, compatible avec le jeu web (PeerJS) | E3 (+ accroches E8 `salon.*`, E0 `CMakeLists.txt`, E7 manifeste) | ✅ Fusionné (PR #77, revue WFS-103, décision B : duel Wi-Fi local, saisie du code, « Sans Internet » ; vrai réseau PeerJS → WFS-92 ⏸ ; QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04 | WFS-83 ✅, WFS-21 ✅ | `feature/waforspeed-89` |  |  |

Critères et notes de suivi :

**WFS-89 — Duel par Internet (code à 4 caractères, compatible jeu web)** · E3 · P2 Haute · ✅ Fusionné (PR #77, fb50d09, revue WFS-103 ; WebRTC réel → WFS-92) — Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04, branche `feature/waforspeed-89` (règles : `regles-de-gestion/WFS-89.md`). Zone : `src/game/duel*`, `src/game/reseau_peerjs.*`, `src/game/binarypack.*`, `tests/**` ; accroches annoncées : `src/ui/ecrans/salon.*`, `CMakeLists.txt` ≤ 10 lignes, manifeste Android ≤ 2 lignes
- [ ] `salon-code`, `salon-code-saisi` : bloc « Rejoindre avec un code », 4 cases, clavier à l'écran, « Rejoindre » actif à 4 caractères
- [ ] `salon-internet` (relais simulé : 2 pilotes) et `salon-hors-ligne` (note orange, pas de plantage)
- [ ] `test_binarypack` vert (octets de référence PeerJS) ; APK : duel entre deux réseaux différents et avec le jeu web (vérifié par l'utilisateur)
