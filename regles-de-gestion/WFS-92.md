# WFS-92 — Duel par Internet : pair PeerJS réel (signalisation WebSocket + WebRTC) branché sur `TransportPeerJs`
Épic : E3 (+ accroches E0 `CMakeLists.txt`, E7 `android/**`) · Priorité : P2 Haute · Statut : ⏸ En attente (décision de l'utilisateur sur la dépendance) · Dépend de : WFS-89 ✅

## Besoin (qui, quoi, pourquoi)
WFS-89 a livré le duel Wi-Fi local, la saisie du code, BinaryPack et `TransportPeerJs` sur un réseau PeerJS **simulé**.
Il manque le **vrai** pair PeerJS (WebSocket sécurisé vers `0.peerjs.com` + canal de données WebRTC) pour jouer entre
deux réseaux différents et contre le jeu web. Cela demande des bibliothèques tierces que la session n'a pas pu intégrer.

## Décision en attente (utilisateur)
Copier dans `third_party/` (avec licences) : **libdatachannel 0.22.6** (MPL 2.0) + libjuice, usrsctp, plog, nlohmann/json,
et **Mbed TLS 3.6.2** (Apache 2.0) : ≈ 14 Mo de sources, compilées seulement pour l'APK (+1 à 2 Mo d'APK, quelques minutes
de construction Android). Le système d'autorisations des sessions a refusé le téléchargement (FetchContent) ; l'utilisateur
doit autoriser l'ajout (ou le faire lui-même). Recommandation du PO : **autoriser** (seule voie compatible avec le jeu web,
pas de serveur à nous).

## Règles de gestion
- RG-01 : toutes les règles RG-01 à RG-07 de `regles-de-gestion/WFS-89.md` restent valables (protocole `p2p.js`, BinaryPack,
  `0.peerjs.com:443`, ICE de PeerJS 1.5.4, 12 s, note orange) ; ce ticket ne fait **que** remplacer le réseau simulé par
  le réseau réel derrière l'interface déjà écrite.
- RG-02 : bibliothèques à version figée dans `third_party/<nom>/` avec leur fichier de licence, ajoutées à la liste des
  licences de l'application ; options `NO_MEDIA=ON`, `NO_EXAMPLES=ON`, `NO_TESTS=ON` ; aucun téléchargement à la construction.
- RG-03 : compilées seulement pour la cible Android (le banc PC garde le réseau simulé) ; échec d'Internet = note
  « Sans Internet », jamais de plantage.

## À réutiliser
- Jeu web (`wlfaleh/waaps`, `waforspeed/js/net/p2p.js`) : `joinP2P`, identifiants et messages, déjà portés dans
  `src/game/reseau_peerjs.*` (WFS-89) : ne rien réécrire, brancher seulement.

## Critères d'acceptation
- [ ] APK : duel entre deux téléphones sur deux réseaux différents (4G / Wi-Fi), vérifié par l'utilisateur.
- [ ] APK : duel téléphone contre jeu web avec le même code, vérifié par l'utilisateur.
- [ ] Sans Internet : note orange, Wi-Fi local utilisable, aucun plantage ; CI verte, APK publié.

## Hors périmètre
iOS (gel), serveur PeerJS à nous, voix / vidéo.
