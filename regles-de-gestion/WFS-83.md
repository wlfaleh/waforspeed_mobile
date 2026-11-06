# WFS-83 — Duel jouable entre deux téléphones sur le même Wi-Fi : écran Salon, transport réseau local, menu « Duel en ligne » actif
Épic : E3 (+ nouveaux fichiers `src/ui/ecrans/salon.*`, accroches E0 / E7 / E8 listées en RG-10) · Priorité : P2 Haute ·
Dépend de : WFS-21 ✅ (logique du duel), WFS-24 ✅ (pseudo et voiture sauvegardés), WFS-41 ✅ (terrain), WFS-74 ✅ (changement de carte)

## Besoin (qui, quoi, pourquoi)
Le joueur veut défier un ami. Toute la logique du duel existe depuis WFS-21 (`src/game/duel.*`, `duel_system.cpp` :
salon, graine commune, présence, fantômes, contacts, classement, résultat, revanche) mais elle ne tourne que sur un
**transport en mémoire** (banc) : dans l'APK, **« Duel en ligne » est grisé dans le menu** (`menu.cpp` l. 117) et
inutilisable. Ce ticket rend le duel **jouable pour de vrai entre deux appareils sur le même Wi-Fi**, sans serveur :
un écran Salon (créer / rejoindre / prêt) et un transport UDP sur le réseau local qui implémente l'interface
`duel::Transport` existante. Rien de la logique de course du duel n'est à refaire.

## Règles de gestion
- RG-01 (menu) : « Duel en ligne » n'est plus grisé ; une touche ouvre l'écran **Salon** (`Ecran::Salon`). Retour → menu.
- RG-02 (écran Salon, accueil = `#lobbyHome` du jeu web) : sur-titre « Duel en ligne · 2 pilotes », titre « Duel » ;
  ligne « Ton pseudo » = `sauvegarde.nick` (« Pilote » par défaut, pas de saisie clavier dans ce ticket) ; bouton
  **« Créer un salon »** ; bloc **« Salons qui t'attendent »** = salons annoncés sur le Wi-Fi depuis moins de 3 s
  (pseudo de l'hôte, code en majuscules, terrain), 4 au plus, une touche = rejoindre ; note d'état : « Recherche de
  salons sur le Wi-Fi… », ou « Aucun réseau : le duel se joue entre deux appareils sur le même Wi-Fi » (texte orange
  `.warn`) si la prise réseau ne s'ouvre pas ; bouton « Retour ». Style : skill `design-street-racing` et écrans
  existants (`terrain.cpp` sert de modèle de mise en page, 1280x720 et 2400x1080).
- RG-03 (écran Salon, dans un salon = `#lobbyRoom`) : « Code du salon » en gros (`makeCode`, 4 caractères de
  `CODE_CHARS`) ; « Ta voiture » (voiture et peinture du garage) ; « Terrain » (carte et moment de l'hôte : l'invité suit
  l'hôte, `followHost`) ; « Distance » 2 / 4 / 6 / 10 km (`DUEL_KMS`, modifiable par l'hôte seulement, `setKm`,
  mémorisée dans `sauvegarde.duelKm` si le champ existe) ; liste des joueurs avec statut (« En salon » / « Prêt ») ;
  boutons **« Prêt »** (bascule `setReady`) et **« Quitter le salon »** (`leave`, retour à l'accueil). Note « En attente
  d'un adversaire… » tant que l'hôte est seul. Un salon a **2 pilotes au plus** : il n'est plus annoncé dès qu'un invité
  l'a rejoint (`advertise` du web : `open` seulement si `others().length === 0`), un troisième ne peut pas entrer.
- RG-04 (départ) : quand tous sont prêts (`checkStart`), on passe à l'écran Course ; si la carte ou le moment de l'hôte
  diffèrent de ceux chargés, on passe par l'écran de chargement de WFS-74 (même chemin que « Rouler » de Terrain), puis
  `DuelSystem` démarre la manche comme aujourd'hui (graine `hashStr(code) + round × 977`, décompte de 3 s). Fin de course,
  classement, résultat, « Revanche » et « Menu » : inchangés (WFS-21). « Menu » quitte le salon.
- RG-05 (transport UDP, nouveau `src/game/reseau_udp.*`, classe `TransportUdp : duel::Transport`) : port UDP **47321**
  (modifiable pour les tests) ; envoi en **diffusion** (255.255.255.255 et adresse de diffusion de chaque interface
  IPv4 active) ; message texte `WFS1|<code>|<id>|<horlogeMs>|<encode(champs)>` (`duel::encode`), **≤ 512 octets**
  (`MSG_MAX`) ; identifiant de joueur aléatoire de 8 caractères par lancement ; cadence : toutes les **0,1 s** en course
  (`PRESENCE_DT`), toutes les **0,5 s** hors course ; prise non bloquante lue à chaque image (aucun blocage de la boucle,
  aucun thread obligatoire). Mes propres messages, ceux d'un autre code, d'une autre version que `WFS1`, trop longs ou
  que `duel::decode` refuse sont **ignorés sans planter** (données reçues jamais crues, comme RG-07 de WFS-21).
- RG-06 (présence et départ d'un joueur) : un joueur dont on ne reçoit plus rien depuis **3 s** est retiré de `peers()` ;
  pendant la course, l'adversaire disparaît alors du classement et le résultat affiche « Ton adversaire a quitté le duel »
  (`renderDuelResult` du web, `rows.length === 1`). `leave()` envoie un dernier message `bye=1` pour un retrait immédiat.
- RG-07 (horloge commune, `nowMs`) : l'horloge de référence est celle de l'**hôte** ; chaque message porte l'horloge de
  son émetteur ; l'invité retient décalage = horloge hôte − horloge locale à la réception (moyenne glissante des 10
  derniers messages de l'hôte ; latence du Wi-Fi négligée) ; `nowMs()` de l'invité = horloge locale + décalage. Ainsi
  `trafficClock()` et le trafic sont les mêmes sur les deux téléphones (RG-02 de WFS-21).
- RG-08 (annonce des salons) : hors course, chaque joueur diffuse sa présence ; l'hôte seul dans son salon ajoute
  `host=<code>`, son pseudo `n`, sa carte et son moment : c'est ce que l'accueil liste (RG-02). Un appareil sur
  l'accueil écoute sans être dans un salon (il n'envoie rien tant qu'il n'a ni créé ni rejoint).
- RG-09 (Android) : permissions `INTERNET`, `ACCESS_NETWORK_STATE`, `ACCESS_WIFI_STATE` et
  `CHANGE_WIFI_MULTICAST_STATE` dans `AndroidManifest.xml` ; **verrou de diffusion** (`WifiManager.MulticastLock`, par
  JNI, nouveau fichier `src/platform/reseau_android.cpp` sur le modèle de `vibration_android.cpp`) pris à l'ouverture de
  l'écran Salon et rendu quand on quitte le duel ; sans Wi-Fi ou si la prise échoue : note RG-02, aucun plantage, le
  reste du jeu marche. iOS : hors périmètre (le transport compile mais n'est pas testé).
- RG-10 (zone) : E3 : `src/game/duel*`, `src/game/reseau_udp.*` (nouveau), `tests/**` (nouveau test ou `test_duel`
  étendu) ; **nouveaux fichiers** `src/ui/ecrans/salon.*` (autorisés par le PO : ne recouvrent pas `reglages.*` de
  WFS-75, en cours dans E8) ; accroches annoncées au Journal : `src/core/ecran.h` ≤ 5 lignes (`Ecran::Salon` et son
  nom), `src/ui/ecrans/menu.cpp` ≤ 10 lignes (dégriser, aller à Salon, banc `menu-bientot` à jour), `src/game/game.cpp`
  ≤ 10 lignes (chargement de la carte de l'hôte si besoin), `android/app/src/main/AndroidManifest.xml` (4 permissions),
  `src/platform/android_main.cpp` ≤ 3 lignes (inclusion de `reseau_android.cpp`), `CMakeLists.txt` ≤ 3 lignes (test
  CPU), `src/platform/desktop_main.cpp` ≤ 3 lignes (groupe de banc). **Interdit** : `src/ui/ecrans/reglages.*`
  (WFS-75), `ios/**` et `ios_main.mm` (WFS-78), `src/world/**` (WFS-18).

## À réutiliser
- Jeu web (`wlfaleh/waaps`, `waforspeed/`) : `js/net/duel.js` → `initRoom`, `renderLobby`, `renderOpenRooms`,
  `advertise`, `createRoom`, `joinRoom`, `followHostMap`, `onRoomPeers`, `leaveRoom` (déroulé du salon, textes,
  annonce des salons ouverts) ; `index.html` sections `#lobby` (lignes 234-271 : libellés exacts « Ton pseudo »,
  « Salons qui t'attendent », « Créer un salon », « Code du salon », « Ta voiture », « Terrain », « Distance », « Prêt »,
  « Quitter le salon ») et `#duelOver` (déjà porté).
- `js/net/p2p.js` : **seulement le principe** (le premier arrivé est l'hôte, les présences circulent en texte, la liste
  complète est rediffusée). **Rien d'autre à réutiliser** : PeerJS / WebRTC passent par un service de mise en relation
  sur Internet et une bibliothèque de navigateur, impossibles à porter en natif sans libwebrtc (des semaines) ; la
  diffusion UDP sur le Wi-Fi fait le même travail en quelques centaines de lignes, sans serveur.
- Natif : `duel::Transport`, `TransportMemoire` (modèle de `presence` / `peers` / `leave`), `duel::Session`,
  `encode` / `decode` / `cleanName` / `makeCode`, `DuelSystem` (course, classement, résultat, revanche : à ne pas
  toucher sauf branchement), `sauvegarde` (`nick`, voiture, peinture), écran `terrain.cpp` (mise en page, boutons),
  `vibration_android.cpp` (modèle d'appel JNI). **Ne rien recréer.**

## Critères d'acceptation
- [ ] Test CPU (CI) : deux `TransportUdp` en boucle locale (127.0.0.1, port de test) dans le même processus : l'invité
      voit le salon annoncé, rejoint, les deux passent prêts, même graine, `trafficClock` égal à ±50 ms près ; message
      hostile (trop long, mal formé, autre code) ignoré ; joueur retiré 3 s après son dernier message ; `bye` immédiat.
- [ ] Banc PC (1280x720 et `-2400`) : `menu-duel` (« Duel en ligne » plus grisé) ; `salon-accueil` (« Créer un salon »,
      un salon d'un second joueur local listé avec son pseudo et son code) ; `salon-hote` (code, voiture, terrain,
      distance, 2 joueurs listés) ; `salon-pret` (les deux « Prêt ») ; `duel-reseau-course` (course démarrée, voiture de
      l'adversaire visible, classement à 2) — le second joueur du banc passe par `TransportUdp` en boucle locale.
- [ ] Journal du banc : `duel : manche 1, salon <code>, graine <n>, <km> km` identique pour les deux joueurs.
- [ ] APK : « Duel en ligne » ouvre le Salon ; sans Wi-Fi, la note orange s'affiche, aucun plantage. (L'utilisateur
      vérifie le duel à deux téléphones lui-même.)

## Hors périmètre
Duel par Internet (serveur relais ou WebRTC), saisie du pseudo ou du code au clavier, plus de 2 pilotes, choix de la
voiture ou du terrain depuis le Salon (on prend ceux du garage et de Terrain), iOS (autorisation « réseau local »),
partie croisée avec le jeu web, chat.

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P2 Haute | WFS-83 | Duel jouable entre deux téléphones sur le même Wi-Fi : écran Salon, transport réseau local, menu « Duel en ligne » actif | E3 (+ `src/ui/ecrans/salon.*`, accroches E0 / E7 / E8) | ✅ Fusionné (par confiance, QA de nuit à faire) | Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04 | WFS-21 ✅, WFS-24 ✅, WFS-41 ✅, WFS-74 ✅ | `feature/waforspeed-83` | [#68](https://github.com/wlfaleh/waforspeed_mobile/pull/68) |  |

Critères et notes de suivi :

**WFS-83 — Duel jouable sur le même Wi-Fi** · E3 · P2 Haute · ✅ Fusionné le 2026-11-04 (PR #68, par confiance ; QA de nuit : `qa/Check.md`) — Claude (agent 5, session 011cpqZaDiS2unNJuwyaruGd), 2026-11-04, branche `feature/waforspeed-83` (règles : `regles-de-gestion/WFS-83.md`)
- [ ] test CPU : deux `TransportUdp` en boucle locale (annonce, rejoindre, prêts, même graine, horloge commune, message hostile ignoré, retrait après 3 s)
- [ ] banc : `menu-duel`, `salon-accueil`, `salon-hote`, `salon-pret`, `duel-reseau-course` (1280x720 et `-2400`)
- [ ] APK : « Duel en ligne » ouvre le Salon ; sans Wi-Fi, note orange, aucun plantage
