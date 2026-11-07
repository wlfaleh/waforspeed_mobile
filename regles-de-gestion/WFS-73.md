# WFS-73 — Conducteurs dans les voitures
Épic : E0 (point d'extension, avec accroches) · Priorité : P3 Moyenne · Dépend de : WFS-19 (tenues et dessin des personnages) · Taille : 1 à 2 jours

## Besoin
Chaque voiture (joueur, trafic, adversaire du duel) a un conducteur visible face au volant, comme dans le jeu web. Issu du découpage de WFS-19 (ancienne RG-05).

## Règles de gestion
- RG-01 : mesure du siège au chargement du `.wfm` (`src/render/model_loader.cpp`) : même méthode que `measureSeat` (rayons verticaux sur la carrosserie pour trouver le toit) ; profil gardé dans `Model` (quelques flottants, pas la géométrie). Rien de mesuré par image.
- RG-02 : buste figé = pose `Sitting_Idle_Loop` figée, haut du corps seul (0,85 m), comme `bakeBust` ; dessiné par `drawSkinned` avec une palette d'os calculée une fois (pas d'animation).
- RG-03 : posé à la place mesurée, tourné vers le volant (`rotation.y = π ± 0,06`), 3 coiffures par sexe, apparence tirée de `pedLook` (WFS-19) stable par voiture ; réduit jusqu'à 0,86 sur les voitures très basses.
- RG-04 : accroches : joueur (`src/game/game.cpp`), trafic (`traffic_system`) et adversaire du duel ; chaque accroche ≤ 10 lignes, annoncée au Journal. Le code du conducteur vit dans un fichier dédié (`src/world/conducteurs.{h,cpp}` ou `src/game/conducteurs.{h,cpp}`).
- RG-05 : coût : un buste par voiture visible à moins de 50 m (au-delà, rien) ; pas d'allocation par image.

## À réutiliser
- Jeu web (`wlfaleh/waaps`, `waforspeed/`) : `js/world/drivers.js` (`bakeBust`, `measureSeat`, `addDriver`) — mêmes valeurs ; `js/world/people.js` (`pedLook`).
- Natif : `drawSkinned`, `src/render/squelette.h`, `assets/people/*.wfp`, `ApparencePersonnage` (WFS-72) et l'apparence de WFS-19. Ne créer aucun personnage.

## Critères d'acceptation (captures du banc PC)
- [ ] capture poursuite : conducteur visible à travers la vitre du joueur, face au volant (pas de dos), ni dans le toit ni flottant
- [ ] capture trafic : conducteurs dans les voitures du trafic ; deux modèles de hauteurs différentes bien placés
- [ ] autres captures inchangées hors voitures, banc `0 écart(s)`

## Hors périmètre
Mains sur le volant, animation du conducteur, conducteur en vue cockpit (WFS-66 dessine l'habitacle).

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P3 Moyenne | WFS-73 | Conducteurs dans les voitures (siège mesuré au chargement, buste figé, joueur / trafic / duel) | E0 | ✅ Fusionné (QA de nuit à faire) |  | WFS-19 | `feature/waforspeed-73` | #42 (fusionnée, 710c9fc) |  |

Critères et notes de suivi :

**WFS-73 — Conducteurs dans les voitures** · E0 · priorité P3 Moyenne · ⏳ En attente de WFS-19 (règles : `regles-de-gestion/WFS-73.md`)
- [ ] conducteur face au volant chez le joueur et dans le trafic, bien placé sur deux modèles de hauteurs différentes ; banc `0 écart(s)`

### 2026-11-07 05:47:27 — Ticket rouvert : jamais codé (po-scrum-master)
Constat de Claude, agent 2 (01UY1Vv5), vérifié par le PO : le statut « ✅ Fusionné, #42 (710c9fc) » était faux (la PR #42 est WFS-19). Aucun conducteur dans le code. Ticket remis dans `TACHES.md` (retiré de `archive/taches-terminees.md`), **P2 Haute** (il débloque WFS-211), attribué à Claude, agent 2, branche `feature/waforspeed-73`. Taille ramenée à **une journée** : mains, animation et vue cockpit restent hors périmètre.
Il n'y a plus de version Web (décision de l'utilisateur, 2026-11-06) : la section « À réutiliser » côté Web est sans objet ; les valeurs se prennent dans le natif et dans le réel (charte `DA-realisme.md`).

Règles ajoutées :
- RG-06 **Limite des instances à squelette** : `SKIN_MAX_INSTANCES` (`src/render/renderer.h`) relevée pour afficher ensemble les conducteurs des voitures à moins de 50 m (RG-05) et les 8 piétons de WFS-19 (16 dessins) ; valeur choisie et mémoire ajoutée (tampon `osBuf_`, `OS_SLOT × SKIN_MAX_INSTANCES` par image) notées au journal ; au-delà de la limite, les conducteurs les plus loin sont omis, jamais un piéton proche.
- RG-07 **Graphisme réaliste** (charte § 3) : conducteur à l'échelle 1 (hauteur assise réelle, tête sous le pavillon, mesurée sur deux voitures de hauteurs différentes) ; preuve par captures avant / après (poursuite, trafic) de jour et de nuit. Les humains réalistes arrivent avec WFS-211.

Zone de code et accroches autorisées :
- Fichiers nouveaux : `src/world/conducteurs.h`, `src/world/conducteurs.cpp` (tout le code du conducteur) ; test CPU dans `tests/` (nouveau fichier `tests/conducteurs*`).
- Noyau E0 : `src/render/renderer.h` (limite RG-06), `src/render/renderer.cpp` (tampon et contrôle de la limite seulement), `CMakeLists.txt` (ajout des fichiers).
- Accroches, chacune ≤ 10 lignes annoncées au journal : `src/render/model_loader.{h,cpp}` (mesure du siège au chargement, profil de quelques flottants dans `Model`), `src/game/game.cpp` (joueur), `src/game/traffic_system.cpp` (trafic), `src/game/duel_system.cpp` (adversaire), `src/platform/desktop_main.cpp` (banc, si une capture le demande).
- À réutiliser sans le refaire : `drawSkinned`, `src/render/squelette.*`, `assets/people/*.wfp`, `pedLook` et `ApparencePersonnage` (`src/world/pietons.*`, lecture seule), `src/render/vehicule_mesure.h` si la mesure de la carrosserie y est déjà.
- Interdit : toucher `src/world/pietons.*` autrement qu'en lecture, créer un personnage, modifier un autre fichier sans le demander au PO.
Critères inchangés (captures poursuite et trafic, banc `0 écart(s)` hors voitures) + tests CPU verts + **parcours complet** (accueil → choix du terrain → course → retour) vert au banc `parcours`.
