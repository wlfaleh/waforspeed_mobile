# Direction artistique « Réalisme » — analyse, charte et feuille de route (PO, 2026-11-05)

> **Directive de l'utilisateur (2026-11-05)** : le jeu natif est la seule référence ; **il n'y a plus de version Web** (décision du 2026-11-06). Nouvel axe stratégique : un rendu proche du réel (routes, voitures, personnages, environnements, éclairage,
> immersion). Toutes les tâches de cet axe sont en **priorité HAUTE (P2 Haute)**.
> **Complément de l'utilisateur (2026-11-06)** : avancer **étape par étape**, une US par partie, chaque US **implémentée, testée et
> validée avant la suivante** ; fidélité visée **99 % du réel** quand les données le permettent ; **jamais l'apparence au détriment
> de la précision** (données fiables, calculs exacts, validation systématique par comparaison avec des données réelles).
> Ce fichier est la **charte commune** des tickets WFS-148 à WFS-167 : chaque développeur la lit avec les règles de son ticket.

## 1. État graphique actuel (analyse du code de `main`, f80dea2)
| Domaine | Ce qui existe | Écart avec le réel |
|---|---|---|
| Moteur de rendu | Rendu direct (forward) PBR GGX, HDR, MSAA, tonemapping ACES, bloom, vignette (`shaders/scene.frag`, `post.frag`) | Bonne base, mais **un seul shader monolithique** (`scene.frag`, 293 lignes) pour route, voitures, décor : chaque épic y toucherait en même temps |
| Éclairage d'environnement | Ciel **analytique à deux couleurs** (`skyColor`, `common.glsl`), reflet du ciel calculé | **Pas de cubemap, pas d'IBL préfiltré** : carrosseries « plastique », aucun reflet des bâtiments, intérieurs et tunnels éclairés comme dehors |
| Ombres | **Une seule carte d'ombre** autour de la caméra, PCF 9 lectures ; ombre de contact = rectangle flou (`F_SHADOW`) | Pas de cascades (ombres absentes ou floues au loin), **aucune ombre des lumières la nuit** (« pas d'ombre portée des projecteurs »), **aucune occlusion ambiante** : les objets flottent |
| Routes | Asphalte **100 % procédural** dans le shader (bruit fbm, grain, lignes calculées) ; WFS-142 a supprimé le miroir | Aucune texture scannée, aucune normale de relief, chaussée uniforme ; pas de fissures, plaques, rapiéçages, traces de pneus ; marquages sans usure ni rétroréflexion |
| Textures | Une seule texture de décor (`assets/world/feuillage.png`) + façades générées en code ; envoi **RGBA8 non compressé** (`vk.h` `uploadTexture`) | Impossible d'ajouter des textures 2K sans exploser la mémoire du téléphone et la taille de l'APK |
| Géométrie | Sommet `Vertex` de 32 octets (position, normale, uv), **sans tangente** (`renderer.h` l. 23) | Normal maps approximées, reliefs faux sous certains angles |
| Véhicules | 20 vraies voitures (114 Mo, glTF convertis en `.wfm`) ; LOD en cours (WFS-134) | Peinture sans flocons ni double couche, verre sans épaisseur, pneus et jantes lisses, intérieurs sommaires, pas de dynamique visible (roulis, plongée), pas de salissure |
| Personnages | Quaternius « Universal Base Characters » CC0, **2 modèles stylisés low-poly** (homme / femme, 4,9 Mo), animations génériques | Style « jeu mobile », pas humain réaliste ; peu de variété ; comportements simples (marche) |
| Environnements | Bâtiments = boîtes extrudées + façade procédurale ; arbres simples ; mer et montagnes simples | Pas de profondeur de fenêtres, toits plats, mobilier urbain générique, panneaux non réglementaires, végétation peu variée, mer sans vagues |
| Moments de la journée | Jour / nuit **binaire** | Pas d'aube, pas de lumière dorée, pas d'heure bleue, pas de nuages physiques, pas d'exposition qui s'adapte |
| Mesure | Banc PC avec captures par ticket | **Aucune vue de référence fixe** pour comparer le réalisme avant / après, **aucune mesure du coût GPU par passe** |

## 2. Limitations qui empêchent d'atteindre le réalisme (à lever dans cet ordre)
0. **Aucune donnée réelle de référence ni outil de mesure** : sans elles, « réaliste » reste un avis. Il faut un référentiel
   sourcé (dimensions, normes, photométrie, couleurs, soleil) et des mesures automatiques comparées à ce référentiel. → WFS-148.
1. **Textures compressées et mipmaps** (ASTC sur Android, ETC2 en repli) : sans elles, pas de textures réelles. → WFS-149.
2. **Tangentes dans les sommets** : indispensables aux normal maps (asphalte, carrosserie, peau, façades). → WFS-149.
3. **Shader monolithique** : découper `scene.frag` en fichiers par domaine (`route.glsl`, `vehicule.glsl`, `decor.glsl`,
   `eclairage.glsl`) pour que 4 épics travaillent en parallèle sans conflit. → WFS-149.
4. **Passes enfichables** (pré-passe de profondeur, cibles demi-résolution, capture d'environnement) : prérequis de l'IBL, de
   l'occlusion ambiante et des reflets d’écran. → WFS-150.
5. **Budget du téléphone** : GPU mobile à tuiles (bande passante limitée), chauffe (WFS-123). Chaque effet coûteux a son
   interrupteur par niveau de qualité ; un niveau **Ultra** est ajouté pour les téléphones puissants. → WFS-150.
6. **Taille de l'APK** : 114 Mo de voitures aujourd'hui. Budget total **≤ 200 Mo** (limite Google Play de l'APK de base) :
   textures compressées obligatoires, résolution par niveau, ressources partagées.
7. **Sources d'actifs et licences** : textures et modèles **CC0 de préférence** (ambientCG, Poly Haven, MakeHuman, Quaternius),
   CC-BY accepté avec crédit dans `assets/licences/`. Si le réseau de l'environnement bloque un site de téléchargement, c'est un
   **blocage d'accès** à signaler à l'utilisateur (une question, avec une recommandation), pas une raison de dessiner à la main.
8. **Référence** : pour l'aspect, la référence est **la photo du réel** et les valeurs physiques ci-dessous, rien d'autre.

## 3. Charte du réel (valeurs de référence, à ±10 %)
**Albédo (couleur de base, en linéaire)** : asphalte neuf 0,04-0,07 · asphalte usé 0,10-0,18 · béton de trottoir 0,25-0,35 ·
marquage blanc 0,65-0,80 · herbe 0,15-0,25 · sable sec 0,35-0,45 · pierre claire 0,40-0,55 · brique 0,20-0,30 · neige 0,80-0,90 ·
peau 0,05-0,65 selon le teint (jamais de blanc ni de noir purs ; plancher abaissé de 0,25 à 0,05 le 2026-11-07 par le PO : la peau du phototype VI publiée mesure 0,05-0,10, signalé par WFS-211 avec avis `expert-realisme-3d`, source à citer dans `references/reel/humains/` par WFS-210).
**Rugosité** : asphalte sec 0,75-0,90 · mouillé 0,35-0,50 (jamais miroir, WFS-142) · eau de flaque 0,05-0,10, flaques ≤ 10 % ·
vernis de voiture 0,02-0,08 · verre 0,0-0,05 · chrome 0,05-0,15 (métal 1) · pneu 0,85-0,95 · tissu 0,8-1,0.
**F0 (réflexion de face)** : diélectriques 0,02-0,05 · métaux = couleur du métal. **Aucun asphalte, béton ou peau métallique.**
**Lumière** : soleil de midi ≈ 100 000 lux (EV100 ≈ 15), ciel couvert ≈ 10 000 lux (EV 12), heure bleue EV 8-10, rue éclairée
la nuit EV 3-5 · lampadaire sodium 1900-2200 K, LED 3000-4000 K, 10 à 30 lux sur la chaussée · phares halogènes 3200 K,
LED / xénon 5500-6500 K · néons d'enseigne saturés mais **bornés** (pas de halo qui mange l'écran).
**Routes (proche des normes européennes)** : voie 3,0 m en ville, 3,5 m sur route et autoroute · ligne 12-15 cm de large ·
discontinue 3 m de trait / 10 m de vide · ligne d'alerte 3 m / 1,33 m · passage piéton : bandes de 0,5 m espacées de 0,5 à 0,8 m ·
bordure de trottoir 12-15 cm de haut.
**Voitures** : dimensions constructeur à ±2 % (longueur, largeur, hauteur, empattement, diamètre de roue).
**Humains** : 1,55 à 1,95 m, proportions réelles (tête ≈ 1/7,5 de la taille), aucun personnage identique côte à côte.

## 4. Ce que veut dire « 99 % de fidélité » (mesurable, jamais à l'œil seul)
Chaque US définit une **liste de mesures** comparées à une **donnée réelle sourcée** du référentiel `references/reel/` (WFS-148).
**Indice de fidélité de l'US = part des mesures dans leur tolérance ; l'US n'est validée qu'à ≥ 99 %** (sur 100 mesures, au plus
une hors tolérance, expliquée au Journal). Tolérances par grandeur :
| Grandeur | Tolérance | Source réelle |
|---|---|---|
| Dimensions (voitures, routes, marquages, bâtiments, humains) | ±1 % | fiches constructeur, normes de signalisation, données anthropométriques |
| Position du soleil (hauteur, azimut) | ±0,5° | algorithme solaire de la NOAA (domaine public), date / heure / latitude de la carte |
| Albédo et couleur des matériaux (charte de couleurs rendue sous lumière blanche connue) | ΔE2000 ≤ 2 ; albédo dans la plage sourcée | valeurs publiées de la charte ColorChecker, base de matériaux physiques (CC0) |
| Rugosité, F0, métal | dans la plage sourcée de la charte (§ 3) | base de matériaux physiques, mesures publiées |
| Éclairement et luminance relatifs (soleil / ombre, ciel / sol, lampadaire au sol) | ±5 % | photométrie publiée (lux, cd/m², EV100) |
| Température de couleur des lumières | ±100 K | fiches des sources (sodium, LED, halogène) |
| Exposition (charte grise 18 % rendue à la bonne valeur) | ±1 % | définition de l'EV100 et de la charte grise |
| Mouvements (marche, course, roulis, plongée) | ±5 % sur vitesse, cadence, angle | données de biomécanique et d'essais publiées |
Quand **aucune donnée fiable n'existe**, la mesure est marquée « non mesurable » avec la raison : elle n'entre pas dans l'indice,
et on n'affirme pas de fidélité qu'on ne peut pas prouver.

## 5. Règles communes à toutes les US « Réalisme » (RC)
- **RC-01 Ordre** : les US suivent l'ordre du § 7 (colonne « Dépend de » de `TACHES.md`). Attribution, QA et fusion : **règles de travail en vigueur de `CLAUDE.md`**, que cette charte ne modifie pas.
- **RC-02 Avant / après** : captures du **banc de référence** (`realisme-*`, WFS-148) sur `main` et sur la branche, mêmes vues,
  mêmes réglages, jour et nuit, Basse et Haute, plus le **rapport de fidélité** chiffré `realisme-fidelite.txt` (avant / après).
- **RC-03 Rapport de fidélité** : chaque US joint son indice de fidélité (≥ 99 %, § 4) et ses captures avant / après au Journal et dans sa ligne de `qa/Check.md`.
- **RC-04 Précision avant apparence** : interdit d'ajouter un facteur « pour faire joli » qui sort une valeur de sa plage
  physique (albédo, rugosité, intensité, couleur). Toute correction artistique est écrite au Journal avec sa raison et reste dans
  la plage sourcée. Chaque constante physique du code cite sa source (commentaire d'une ligne).
- **RC-05 Tests automatiques** : chaque mesure qui peut l'être devient un **test CPU** (dimensions, position du soleil,
  conversions photométriques, normes de marquage) ou une mesure du banc ; un test rouge bloque la fusion.
- **RC-06 Performances** : perte ≤ 10 % d'img/s en Moyenne par US au banc ; Basse tient 30 img/s au banc de chauffe ; chaque
  effet coûteux se coupe par niveau (Basse / Moyenne / Haute / Ultra) ; coût GPU par passe au Journal. Un niveau plus bas
  **simplifie le calcul, pas les valeurs physiques** (même couleur, même exposition, moins de détails).
- **RC-07 Jamais de régression** : aucune voiture en cube (WFS-134), aucune route miroir (WFS-142), HUD et trafic lisibles.
- **RC-08 Licences et taille** : chaque actif a son fichier dans `assets/licences/` ; Mo ajoutés à l'APK au Journal ; total ≤ 200 Mo.
- **RC-09 Parcours** : le test de parcours complet (accueil → mode → garage → terrain → course → fin → retour) reste vert.
- **RC-10 Interface** : le style « street racing » (`design-street-racing`) reste la règle **pour l'interface** (menus, HUD) ;
  pour le monde 3D, c'est cette charte (néons crédibles, pas arcade).


## 6. Nouvelles épics (zones de code séparées)
| Épic | Nom | Zone de code |
|---|---|---|
| E9 | Réalisme — Routes | `shaders/route*.glsl`, `src/render/route_*`, `assets/textures/route/**` (+ accroches ≤ 10 lignes `src/game/track.cpp`) |
| E10 | Réalisme — Véhicules | `shaders/vehicule*.glsl`, `src/render/vehicule_*`, `assets/cars/**`, `tools/convert_car.mjs` (repris de E5) |
| E11 | Réalisme — Personnages | `assets/people/**`, `tools/convert_people*`, `src/world/pietons.*`, `src/world/foule_*`, `src/render/squelette.*`, `shaders/scene_skin*` (repris de E2 / E4) |
| E12 | Réalisme — Environnements | `src/world/archi_*`, `src/world/mobilier_*`, `src/world/vegetation.*` (repris de E2), `shaders/decor*.glsl`, `assets/textures/decor/**`, `assets/world/modeles/**` |
| E13 | Réalisme — Lumière et image | `shaders/eclairage*.glsl`, `sky*`, `post*`, `shadow*` (repris de E4), `src/render/eclairage_*`, `ibl_*`, `atmosphere_*`, `post_*` |
Le référentiel `references/reel/**` et les tests `tests/fidelite_*` sont créés par WFS-148 (E0), puis chaque épic ajoute ses
propres fichiers `references/reel/<domaine>/**` et `tests/fidelite_<domaine>*`. Le ticket en cours WFS-144 (E4) garde ses droits
jusqu'à sa fusion. Brancher une épic Réalisme dans une carte (`ville.cpp`, `plage.cpp`…) : accroche ≤ 10 lignes, annoncée au Journal.

## 7. Paliers, ordre de réalisation et affectation
Chaque flèche = « terminée avant de commencer la suivante » (ordre des dépendances).
| Palier | US (épic), dans l'ordre | Pourquoi cet ordre | Profil d'agent |
|---|---|---|---|
| 0 — Référence et socle | **WFS-148** référentiel du réel + banc de fidélité → **WFS-149** textures, tangentes, shaders découpés → **WFS-150** passes, Ultra, budget GPU (E0) | on ne peut ni mesurer ni comparer sans référence ; aucun effet réaliste sans ce socle | agent moteur Vulkan |
| 1 — Lumière | **WFS-151** exposition physique + IBL → **WFS-152** ciel physique et soleil calculé, cycle jour / nuit → **WFS-153** ombres en cascades + occlusion → **WFS-154** reflets et post-traitement (E13) | tout matériau se juge sous une lumière juste : la lumière passe d'abord | agent rendu / éclairage |
| 2 — Véhicules | **WFS-158** dimensions et modèles (dès WFS-148) → **WFS-159** matériaux (après WFS-151) → **WFS-160** dynamique visible (E10) | la géométrie se mesure sans lumière ; les matériaux, sous la lumière validée | technical artist véhicules |
| 2 — Routes | **WFS-155** revêtements (après WFS-151) → **WFS-156** marquages et imperfections → **WFS-157** états de surface (après WFS-154) (E9) | la surface avant les détails, l'eau après les reflets | rendu matériaux / technical artist route |
| 3 — Environnements | **WFS-161** architecture (après WFS-151) → **WFS-162** mobilier et signalisation → **WFS-163** végétation, terrain, mer (après WFS-153) (E12) | le bâti structure la ville ; la végétation a besoin des ombres | technical artist décor |
| 4 — Personnages | **WFS-164** humains réalistes (après WFS-151) → **WFS-165** animations → **WFS-166** comportements de foule (E11) | modèle, puis mouvement, puis comportement | technical artist personnages, puis animation / gameplay |
| 5 — Harmonisation | **WFS-167** validation finale par carte (E13, après toutes) | réglage d'ensemble mesuré, recette complète | agent rendu |
Seul **WFS-148** est `⏳ Prêt` aujourd'hui : c'est la première marche. L'attribution nominative se fait quand un agent demande
un ticket au PO (règle du projet) ; le profil dit quel agent doit le prendre. Après le palier 0, jusqu'à 4 épics avancent en
même temps (E10, E13, puis E9, E12, E11), chacune une US à la fois, dans l'ordre des dépendances.

## 8. Résultat attendu (fin de l'axe)
Indice de fidélité ≥ 99 % sur l'ensemble des mesures du référentiel, et, sur les vues de référence de jour, une capture qu'on
confond avec une photo : asphalte réel et marquages réglementaires usés, voitures aux dimensions et reflets exacts, passants
humains et variés, bâtiments avec profondeur, soleil à sa vraie place et lumière naturelle continue ; la nuit, une rue éclairée
aux vraies valeurs (ni miroir, ni néons d'arcade).
