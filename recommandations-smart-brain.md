# Recommandations Smart Brain

Fichier de l'agent `smart-brainstorming`. **L'agent propose, il ne décide rien.** Il ne parle qu'avec l'utilisateur.
L'utilisateur lit ces recommandations, choisit, et demande lui-même au PO ce qu'il veut faire.
Les priorités P1 à P3 ci-dessous sont des **recommandations**, pas des décisions.
Nouvelles recommandations ajoutées à la fin, chacune avec sa date.

---

## R01 — 2026-11-06 — Roadmap du week-end (7 et 8 novembre 2026) : 10 épics proposées

Mode standard : diagnostic, brainstorming, priorisation. Session demandée par l'utilisateur le 2026-11-06.
Ce sont des **propositions** : l'utilisateur choisit et en parle au PO s'il le souhaite.

### 1. Résumé et objectif
WaForSpeed Mobile est un jeu de course « street racing » natif sur Android (C++20 / Vulkan). L'utilisateur le teste sur son téléphone.
Il y a 4 cartes (Ville, Campagne, Corniche, Grand Prix) et 4 modes (Checkpoint, Course à 5, Poursuite police, Duel entre amis par
Internet). Il y a aussi 20 voitures réelles, un garage, des records par mode, un fantôme, une mini-carte, la nuit et la météo.
**Objectif** : choisir 10 épics à lancer ce week-end, avec plusieurs agents en parallèle, sans gêner l'axe Réalisme (E9 à E13) ni le
décor des cartes (E2), déjà en cours.

### 2. Diagnostic rapide
**Ce qui fonctionne déjà (faits, `TACHES.md`)** : le parcours complet, de l'accueil à la fin de course ; 4 modes jouables ; un duel
par Internet avec serveur de relais (WFS-112) ; des voitures aux fiches réelles (`src/game/cars.h` : ch, 0-100, vitesse max, masse) ;
du son riche (WFS-120, 121, 124, 131, 174) ; la tenue sur téléphone (WFS-123) ; l'annonce d'une nouvelle version de l'APK (WFS-132).

**Frictions**
- *Observée* : 122 lignes de `TACHES.md` portent encore « QA de nuit à faire », et 33 anomalies sont 📋 planifiées dans `anno.md`.
  Or iOS reste gelé tant que l'utilisateur n'a pas validé Android (`CLAUDE.md`).
- *Observée* : `tache utilisateur.md` montre que des écrans ont cassé sans que personne le voie avant la fusion (garage en 2400,
  ANO-19 rouverte).
- *Supposée* : il n'y a aucune raison de progresser. Aucune trace de carrière, de déblocage, de défi ou de succès dans `TACHES.md`
  ni dans `src/`. Tout est ouvert dès le départ. Le jeu est riche, mais il ne donne pas d'objectif après la 1re course.
- *Supposée* : il n'y a aucun tutoriel (aucune occurrence dans le dépôt). Les commandes tactiles ont déjà demandé 3 passages
  (WFS-98, 108, 168), signe qu'elles ne s'expliquent pas seules.

**Opportunités** : le fantôme (WFS-125) et le serveur de relais (WFS-112) existent déjà. Ils permettent du social et de la rejouabilité
à faible coût. Les fiches réelles des voitures permettent une préparation mécanique crédible, cohérente avec l'axe Réalisme.

**Ce qui manque pour une analyse fiable** : aucun retour de joueur autre que l'utilisateur, et aucune mesure d'usage. On ne sait pas
quel mode il joue le plus, ni combien de temps.

### 3. Tableau des 10 épics proposées

| ID | Domaine | Constat / opportunité | Proposition d'amélioration | Bénéfice joueur | Impact | Complexité | Priorité | Risques / points à vérifier |
|---|---|---|---|---|---|---|---|---|
| A01 | Technique / qualité | *Observé* : 122 tickets en « QA de nuit à faire », 33 ANO planifiées, écrans cassés vus par l'utilisateur. Condition pour dégeler iOS | **Épic « Android validé »** : recette par lot de tout l'existant sur les captures 2400 (densité du téléphone), correction des ANO 📋 par écran, puis une liste « prêt à valider » que l'utilisateur coche sur son téléphone | Un jeu sans défaut visible ; l'utilisateur peut valider Android | Élevé | Moyen | P1 | Charge du QA ; ne pas toucher la taille des boutons (gel WFS-168) ; risque de conflit avec les tickets en cours |
| A02 | UX / onboarding | *Supposé* : aucun tutoriel ; 3 passages sur les commandes tactiles | **Épic « Première course guidée »** : au 1er lancement, une course Checkpoint courte avec des bulles qui s'effacent (gaz, frein, nitro, volant / gyroscope), l'une après l'autre, à faire pour avancer ; on peut la passer et la revoir dans les Réglages | Le joueur sait jouer en moins d'une minute, sans lire | Élevé | Moyen | P1 | Ne déplace ni n'agrandit aucun bouton ; textes lisibles en 2400 ; sauvegarde du « déjà vu » (WFS-24) |
| A03 | Game design / progression | *Supposé* : rien à atteindre après la 1re course | **Épic « Saison street »** : 3 étoiles par course et par mode (exemples : temps, combo de quasi-contacts, sans accident), puis paliers d'étoiles qui ouvrent peintures, cartes de nuit et voitures. Tout reste gratuit et sans achat | Une raison de rejouer chaque carte, des buts clairs | Élevé | Moyen | P1 | Décision produit : faut-il bloquer des voitures déjà ouvertes ? Proposition : un compte existant garde tout ; seuils à équilibrer |
| A04 | Rétention / social | *Observé* : relais WFS-112 et fantôme WFS-125 existent ; *supposé* : pas de défi partagé | **Épic « Défi du jour »** : chaque jour, une carte, un moment, une voiture et un mode, tirés d'une graine sur la date (sans serveur). Le même défi pour tous, avec le classement entre amis par le relais | Une petite raison de lancer le jeu chaque jour, à comparer avec ses amis | Moyen | Faible | P2 | Pas de série punie par l'absence (pas de FOMO) ; si le relais tombe, le défi reste jouable hors ligne |
| A05 | Social / compétition | *Observé* : duel en direct seulement (WFS-112, 122) | **Épic « Records entre amis »** : envoyer son record et son fantôme à ses amis par le relais, puis courir contre le fantôme d'un ami en Checkpoint (en différé, sans être connecté en même temps) | Jouer « avec » ses amis même seul | Moyen | Moyen | P2 | Taille des fantômes ; vie privée (pseudo seulement) ; quota de Cloudflare Workers ; triche (records à vérifier côté relais) |
| A06 | Gameplay / modes | *Observé* : Course à 5 et IA combative (WFS-110, 133) | **Épic « Mode Élimination »** : Course à 5, et à chaque checkpoint le dernier est éliminé, jusqu'au duel final. Reprend tout le code de Course à 5 | Un mode tendu de plus, à faible coût | Moyen | Faible | P2 | Lisibilité au HUD (qui va sortir) ; cohérence avec la mini-carte WFS-173 ; zone E3 déjà occupée par WFS-183 |
| A07 | Gameplay / réalisme | *Observé* : fiches réelles dans `cars.h`, axe Réalisme ; *supposé* : voitures différentes en vitesse mais pas en réglages | **Épic « Préparation mécanique »** : au garage, 3 à 4 réglages réels par voiture (pneus route / sport, rapport de boîte court / long, répartition de freinage, aileron), avec leur effet mesuré sur la fiche (0-100, vitesse max) | Rendre sa voiture « à soi », de la profondeur pour les joueurs exigeants | Moyen | Élevé | P2 | Équilibrage de Course à 5 et du duel (réglages à égalité en duel ?) ; physique sourcée (DA-realisme) ; le garage est fragile (ANO-19) |
| A08 | Direction artistique / partage | *Observé* : le monde vise 99 % de réalisme, mais rien ne permet de le montrer ; *supposé* : pas de replay | **Épic « Ralenti et photo »** : en fin de course, revoir les 10 dernières secondes avec 3 caméras, faire pause, prendre une photo sans HUD, l'enregistrer et la partager | Montrer son jeu ; met en valeur le travail de l'axe Réalisme | Moyen | Moyen | P2 | Mémoire du tampon d'enregistrement ; dépend de la caméra (E3) et du rendu (E4) ; droits de partage Android |
| A09 | Accessibilité | *Supposé* : rien sur le daltonisme ni les aides ; la police en rouge / bleu, le cyan / orange de la mini-carte | **Épic « Confort et accessibilité »** : mode daltonien (couleurs du HUD et de la mini-carte), aide au freinage et à la trajectoire en option, annonces écrites des sons importants (sirène, compte à rebours), réglage de l'intensité des secousses | Plus de joueurs peuvent jouer à l'aise | Moyen | Faible | P2 | Ne touche ni à la taille ni à la place des boutons ; la DA « street racing » reste la même ; aides à interdire en duel |
| A10 | Narration / univers | *Supposé* : pas d'univers autour des courses | **Épic « Rivaux de la ville »** (expérimentale) : un rival par carte, avec un pseudo, une voiture, une couleur et une phrase de défi, à battre en Duel contre l'ordinateur, débloqué par les étoiles de A03 | Une identité « street racing », des buts nommés | Moyen | Moyen | P3 | Dépend de A03 ; textes à valider par l'utilisateur ; ne pas copier un jeu existant (personnages et histoire originaux) |

Impact et complexité : estimation qualitative, sans donnée d'usage.

#### Regroupement
- **Quick wins** : A04 (Défi du jour), A06 (Élimination), A09 (Confort et accessibilité).
- **Structurantes** : A01 (Android validé), A02 (Première course guidée), A03 (Saison street), A05 (Records entre amis),
  A07 (Préparation mécanique), A08 (Ralenti et photo).
- **Expérimentale** : A10 (Rivaux de la ville), à prototyper après A03.

#### Ordre proposé pour le week-end (simple suggestion)
- **Samedi matin** : A01, qui tourne tout le week-end ; A02 ; A06.
- **Samedi après-midi** : A03 (catalyseur de A10) et A09.
- **Dimanche** : A04, A05 (après A04, même relais) et A08 ; A07 si une place se libère au garage (E5) ; A10 seulement si A03 est fusionnée.

### 4. Top 3
1. **A01 — Android validé.** C'est la seule épic qui sert ta priorité écrite (« le jeu qui fonctionne sur Android »), et elle débloque
   iOS. Sans elle, chaque nouvelle épic ajoute des écrans non recettés.
2. **A02 — Première course guidée.** Les commandes ont demandé 3 passages. Un tutoriel règle la compréhension sans toucher la taille
   des boutons, qui est gelée.
3. **A03 — Saison street.** C'est le plus grand manque de game design : le jeu est riche mais sans objectif. Elle prépare aussi A04 et A10.

### 5. Risques et validations
- **Zones de code** : E2 et E9 à E13 sont très occupées. A06 touche E3 (WFS-183 en cours). A07 touche le garage (E5, ANO-19).
  Point à signaler au PO si l'épic est retenue : il faut une zone de code libre pour chacune.
- **Décision produit à trancher par l'utilisateur pour A03** : faut-il bloquer des voitures et des cartes déjà ouvertes aujourd'hui ?
- **Hypothèses testables**
  - A02 : si une première course guidée explique gaz, frein, nitro et volant, alors un nouveau joueur finit sa 1re course sans
    accident grave. On le vérifie par un essai de l'utilisateur sur un téléphone remis à zéro, et par une capture de banc à chaque étape.
  - A03 : si chaque course donne 3 étoiles à gagner, alors l'utilisateur rejoue la même carte pour les étoiles manquantes. On le vérifie
    par son retour après une journée d'essai, car il n'existe pas de mesure d'usage.
  - A01 : si toutes les captures 2400 sont recettées avant la fin du week-end, alors l'utilisateur peut cocher « Android validé »
    sur sa liste.

### 6. Prochaine étape
L'utilisateur choisit les épics qu'il veut garder et tranche la question de A03. S'il le souhaite, il les demande ensuite au PO.

**Recommandations de l'agent sur les deux questions** (l'utilisateur décide) :
- Épics à garder : les 10, ou au minimum A01 à A03.
- Pour A03 : ne pas bloquer ce qui est déjà ouvert ; les étoiles débloquent seulement des contenus nouveaux.
---

## R02 — 2026-11-06 — Court terme : aujourd'hui et demain matin

Mode Diagnostic et Priorisation. L'agent propose, l'utilisateur décide. État relevé le 2026-11-06 vers 15:25 UTC
(`TACHES.md`, date du dernier commit de chaque branche, `tache utilisateur.md`).

### 1. Diagnostic rapide
- **Observé** : 3 tickets urgents n'avancent plus.
  - WFS-109 (feux de freinage) : dernier commit le 2026-11-05 à 20:36, soit environ 19 h.
  - WFS-134 (voitures en cubes, « priorité absolue ») : dernier commit le 2026-11-05 à 22:44, soit environ 17 h.
  - WFS-145 (CI de `main` rouge depuis WFS-94) : dernier commit le 2026-11-06 à 01:18, soit environ 14 h.
- **Observé** : la CI de `main` est rouge (job `endurance`). Or la « fusion par confiance » demande à chaque développeur de suivre
  la CI de `main` après sa fusion. Tant qu'elle est rouge, une nouvelle casse ne se voit pas.
- **Observé** : WFS-109 (« feux de freinage », halos des clignotants) et WFS-182 (« halos de frein qui flottent ») touchent tous
  deux les feux arrière. WFS-171 vient d'y être fusionné et WFS-158 change les dimensions des voitures. *Supposé* : risque de conflit
  ou de travail en double sur le bloc des feux.
- **Observé** : 10 tickets sont en cours en même temps, dont 4 sur le décor des cartes (E2), par exceptions du PO.
- **Observé** : 3 questions te sont posées et attendent ta réponse dans `tache utilisateur.md`.
  - La densité de ton téléphone, demandée par le QA pour simuler ton écran exactement.
  - Veux-tu un réglage pour masquer la mini-carte ?
  - Veux-tu une mini-carte plus zoomée en Poursuite ?

### 2. Tableau

| ID | Domaine | Constat / opportunité | Proposition d'amélioration | Bénéfice joueur | Impact | Complexité | Priorité | Risques / points à vérifier |
|---|---|---|---|---|---|---|---|---|
| C01 | Technique / qualité | *Observé* : CI de `main` rouge depuis WFS-94 ; WFS-145 arrêté depuis environ 14 h | Remettre WFS-145 en route tout de suite et viser une CI verte avant toute fusion non urgente | Une casse nouvelle redevient visible tout de suite, avant d'arriver sur ton téléphone | Élevé | Moyen | P1 | Une pause des fusions est une règle de travail : à toi de décider si tu la veux, et pour combien de temps |
| C02 | Technique / suivi | *Observé* : WFS-109 et WFS-134 sans commit depuis 17 à 19 h | Demander au PO de reprendre ou de réattribuer ces deux tickets, WFS-134 en premier (« priorité absolue ») | Les voitures en cubes et les feux de freinage avancent enfin | Élevé | Faible | P1 | WFS-134 exige le verdict du QA avant fusion (RG-14 du ticket) ; il faut donc prévoir le QA |
| C03 | Organisation | *Observé* : WFS-109 et WFS-182 touchent les mêmes feux arrière, juste après WFS-171 et à côté de WFS-158 | Demander au PO si WFS-109 et WFS-182 doivent être réunis, ou faits l'un après l'autre | Moins de risque qu'une correction efface l'autre | Moyen | Faible | P1 | À vérifier par le PO dans les règles des deux tickets ; *supposé*, non confirmé |
| C04 | UX / qualité | *Observé* : le QA attend la densité de ton téléphone | Répondre avec `adb shell wm density` (ou Réglages, puis Affichage) | Le banc d'essai montre exactement ton écran : moins de surprises comme le garage cassé | Moyen | Faible | P1 | Rien à coder : la réponse se donne dans `tache utilisateur.md` |
| C05 | Retours joueur | *Observé* : tu es le seul joueur testeur, et beaucoup de tickets fusionnés n'ont pas été vérifiés | Un essai de 15 minutes sur l'APK de `main`, avec la liste du § 3, et tes remarques notées dans `tache utilisateur.md` | Les vrais défauts sont trouvés vite, classés par ton ressenti | Élevé | Faible | P1 | Faire l'essai après le vert de la CI (C01), sinon l'APK peut être instable |
| C06 | UX / HUD | *Observé* : le réglage « masquer la mini-carte » et le zoom en Poursuite attendent ta réponse (WFS-173) | Les décider après ton essai (C05) : par exemple « masquer : oui / non » et « zoom Poursuite : oui / non » | Le HUD convient à ta façon de jouer | Faible | Faible | P2 | Petits tickets si tu dis oui ; ne touchent aucun bouton |
| C07 | Organisation | *Observé* : 10 tickets en cours, 4 sur le décor (E2) | Ne pas ouvrir de nouvelle épic (R01) avant que C01 et C02 soient réglés ; démarrer R01 par A01 « Android validé » | Moins de conflits, plus de tickets finis | Moyen | Faible | P2 | Ralentit un peu les nouveautés du week-end |

#### Regroupement
- **À faire tout de suite, en quelques minutes pour toi** : C04, C06 (après l'essai), et les demandes au PO de C01, C02 et C03.
- **Aujourd'hui** : C05, l'essai de 15 minutes.
- **Avant le week-end** : C07, l'ordre de départ de R01.

### 3. Liste d'essai de 15 minutes (C05), sur l'APK de `main`
1. Lancement, chargement, menu : aucun texte coupé ni illisible.
2. Garage : les 20 voitures défilent, aucune voiture en cubes, la fiche est lisible.
3. Checkpoint de jour en Ville : commandes, HUD, mini-carte, fin, puis « Rejouer ».
4. Course à 5 de nuit à la Corniche : feux de frein bien à leur place (WFS-171), aucun halo qui flotte.
5. Poursuite en Campagne : police, sirène, mini-carte.
6. Duel avec un ami, ou au moins l'écran Salon.
7. Pause, puis le bouton Retour du téléphone, puis la sortie du jeu et le retour.
8. Réglages : qualité Basse, puis une course. Les voitures doivent rester belles (WFS-134).
9. Noter en une ligne chaque gêne, avec l'écran et le moment.

### 4. Top 3
1. **C01, CI verte** : sans elle, aucune fusion « par confiance » n'est vraiment contrôlée.
2. **C02, reprendre WFS-134 et WFS-109** : ce sont tes demandes urgentes, et elles sont à l'arrêt.
3. **C05, ton essai de 15 minutes** : c'est la seule vraie mesure du jeu dont dispose l'équipe.

### 5. Ce que l'utilisateur décide (recommandation de l'agent entre parenthèses)
- Faut-il mettre les fusions non urgentes en pause jusqu'au vert de la CI ? (Oui, seulement jusqu'au vert.)
- Faut-il réunir WFS-109 et WFS-182 ? (Laisser le PO vérifier les règles des deux tickets avant de trancher.)

## R03 — 2026-11-09 — Skills manquants des agents (mode Diagnostic / Priorisation)

**Demande** (utilisateur, transmise par le PO) : garder l'agent `dev`, ajouter les skills manquants chargés par défaut.
**Faits vérifiés** (`.claude/agents/`, `.claude/skills/`) : 9 agents, 9 skills. Sans skill : `po-scrum-master` (30 Ko dans le
fichier d'agent) et `expert-realisme-3d` (4 Ko). `qa-lead.md` (9 Ko) et `smart-brainstorming.md` (13 Ko) ont un skill **et**
un long fichier d'agent. `po-scrum-master.md` garde des consignes périmées (ligne 124 « QA par `qa-lead` avant fusion » ;
section « Fusion par confiance », barrée). `expert-ci-cd` ne charge pas `agent-developer` alors qu'il code des US techniques.

| ID | Domaine | Constat | Proposition | Bénéfice | Impact | Complexité | Priorité | Risques |
|---|---|---|---|---|---|---|---|---|
| A01 | Outillage agents | PO : 30 Ko de méthode dans l'agent, avec des règles périmées | Skill `po-scrum-master` : SKILL.md (attribution, règles de gestion, ménage, KO, US techniques) + `references/ordres-utilisateur.md` (ordres datés) ; agent réduit à un renvoi type `lead-dev.md` ; supprimer les règles barrées en déplaçant | Un PO qui applique les règles du jour | Élevé | Moyen | P1 | Perdre un ordre en déplaçant : relecture ligne à ligne par le PO |
| A02 | Outillage agents | Réalisme 3D sans skill : un dev de l'axe Réalisme ne peut pas le charger | Skill `expert-realisme-3d` (méthode, mesures de fidélité, budgets téléphone, renvoi à `DA-realisme.md`) ; agent = renvoi court ; reste conseiller, pas développeur | Choix graphiques mesurés, même méthode pour tous | Moyen | Faible | P1 | Doublon avec `DA-realisme.md` : le skill renvoie, ne recopie pas |
| A03 | Outillage agents | Skill « dev » distinct | **Ne pas créer** : `agent-developer` + `waforspeed-natif` couvrent déjà le dev ; `dev.md` les charge | Pas de 3e source contradictoire | — | — | — | — |
| A04 | Outillage agents | `qa-lead.md` et `smart-brainstorming.md` répètent leur skill | Réduire ces agents à un renvoi court, la méthode reste dans le skill | Une seule source par rôle | Moyen | Faible | P2 | Vérifier que rien d'unique ne se perd |
| A05 | Outillage agents | Skills par défaut incomplets | `expert-ci-cd` : + `agent-developer` + `waforspeed-natif` (US techniques) ; `dev` : + `expert-realisme-3d` si axe Réalisme ; `lead-dev` : + skill de l'expert du ticket relu | Même exigence pour tout code | Moyen | Faible | P2 | Contexte plus lourd : charger « si besoin » |
| A06 | Outillage agents | Pas de liste agent → skills | Colonne « skills chargés » dans `appel-agents.md` (déjà en partie) comme seule table de référence | Lecture rapide | Faible | Faible | P3 | Tenir à jour à chaque nouveau skill |

**Top 3** : A01 (le plus de règles périmées), A02 (seul expert inutilisable par un dev), A03 (éviter un doublon).
**Validation** : après A01, lancer `@po` sur une attribution test et vérifier qu'il cite les règles du 2026-11-09 (un seul sujet par session) et plus « QA avant fusion ».
**Décision** : à l'utilisateur, via le PO.

---

## R04 — 2026-11-12 — Les derniers jours avant le lancement : critique franche et chemin vers les premiers revenus

Mode Challenge + Priorisation, demandé par l'utilisateur (`@smartbrain`, 2026-11-12 20:42 UTC) : « comment on doit faire maintenant,
quelques jours avant le lancement ; critique l'application ; le but : gagner un peu d'argent ». **Propositions seulement** : l'utilisateur choisit et demande au PO.

### 1. Diagnostic (sources : `TACHES.md`, `tache utilisateur.md`, `juridique/rapport-2026-11-10.md`, `anno.md`, `src/ui/ecrans/menu.cpp`)
**Ce qui marche** : moteur maison stable, 4 modes (Checkpoint, Course à 5, Poursuite, Duel), Corniche téléchargeable en paquet (WFS-463, 12/11),
aide à la conduite validée par l'utilisateur (08/11), 18 voitures sans licence NC (WFS-404), processus outillé (CI, captures, lead dev).

**Critique franche (observé)** :
1. **Rien n'est fini du point de vue du joueur** : **81 tickets 🟡 attendent la validation de l'utilisateur** sur son téléphone, 11 ❌ refusés,
   57 ⏳ en attente. Le goulot n'est plus le code, c'est la validation. On empile des corrections sans savoir lesquelles tiennent.
2. **Le jeu n'a pas de boucle de jeu** : aucune progression, aucune monnaie, aucun déblocage (recherche dans `src/game`, `src/ui` : rien).
   Le joueur fait une course… et n'a aucune raison de revenir. **Sans retour du joueur, aucun modèle économique ne rapporte** (ni pub, ni achat).
   Idée déjà proposée en R01 (A03 « Saison street »), jamais lancée.
3. **L'effort va au réalisme, pas à ce qui fait vendre** : 99 % de réalisme, décors jusqu'à 700 Mo, vue Cockpit en 3 passages, formes géométriques
   chassées partout… pendant que la fluidité reste fragile (16 à 30 img/s en Basse sur le S21 FE selon les mesures du journal, 6 passages
   « performance »). Sur mobile, un joueur pardonne un banc imparfait, **jamais une saccade ni un téléchargement de 700 Mo**.
4. **Conduite pas stabilisée** : direction refusée 2 fois (WFS-295, 217/241), WFS-258 suspendu, freins/ESP en attente (WFS-259). C'est le cœur d'un jeu de course.
5. **Juridique** : J-01 à J-07 du juriste en 🔴 au 10/11 ; les tickets existent (WFS-405, 407, 408, 409, 320) mais sont « ⏸ en attente du jour ».
   Le **nom « WaForSpeed »** (J-09) rappelle une série très connue : le changer coûte 1 h avant la sortie, très cher après (fiche, avis, liens).
6. **Calendrier Google sous-estimé** (*à vérifier dans la Console*) : un **compte développeur personnel récent** doit faire un **test fermé
   avec au moins 12 testeurs pendant 14 jours** avant de pouvoir publier en production (J-15 du juriste). Si c'est votre cas, « quelques jours » = impossible ;
   le lancement est au plus tôt **J+14 après l'ouverture du test fermé**. Il faut l'ouvrir **dès maintenant**.

**Informations manquantes** : type de compte Play Console (personnel / organisation, date de création) ; budget pub/temps ; téléphone cible minimal.

### 2. Tableau des axes
| ID | Domaine | Constat / opportunité | Proposition d'amélioration | Bénéfice joueur | Impact | Complexité | Priorité | Risques / points à vérifier |
|---|---|---|---|---|---|---|---|---|
| A01 | Publication | *Observé* : J-15, test fermé 12 × 14 j possible | **Ouvrir aujourd'hui le test fermé** sur Play Console avec l'AAB de WFS-319 (amis, famille, forum) ; les 14 jours servent de bêta | Version publique déjà testée | Élevé | Faible | P1 | Vérifier le type de compte ; 12 testeurs **inscrits et actifs** les 14 jours |
| A02 | Process | *Observé* : 81 tickets 🟡 | **Gel des fonctionnalités jusqu'au lancement** + **une séance de recette par lot** : 30 min sur l'APK de `main` (Corniche, jour/nuit, Basse), l'utilisateur répond « OK / KO » par lot, le PO passe tout le reste en ✅ ou ouvre 1 ticket | Moins de bugs à la sortie | Élevé | Faible | P1 | Ne pas tout re-tester ticket par ticket |
| A03 | Juridique | *Observé* : J-01 à J-07 🔴 | Lancer **maintenant** (pas « le jour J ») WFS-405, 407, 408, 409, 320 ; décider le **nom du jeu** (J-09) avant la fiche | Pas de retrait du store | Élevé | Faible à Moyen | P1 | Avis d'avocat **avant** pub ou achat (J-17) |
| A04 | Performance | *Observé* : 16-30 img/s en Basse | **Critère de sortie unique** : Corniche ≥ 30 img/s stables en Basse sur le S21 FE, qualité choisie automatiquement au 1er lancement ; sinon on baisse la qualité, pas l'ambition du décor | Jeu fluide = bonnes notes | Élevé | Moyen | P1 | Lié à WFS-419 (diagnostic sur téléphone) |
| A05 | Game design | *Observé* : aucune progression | **Boucle minimale « Carrière »** : chaque course rapporte des crédits (temps, frôlements, checkpoints) ; voitures et peintures **à débloquer** avec ces crédits ; 3 étoiles par course | Raison de revenir chaque jour | Élevé | Moyen | P1 (v1.1, ou v1.0 si le test fermé laisse 14 j) | Équilibrage ; zone E3/E5 ; sauvegarde existante à réutiliser |
| A06 | Monétisation | *Observé* : « gratuit, sans pub ni achat » | **Modèle simple et propre** : jeu gratuit ; **pub récompensée volontaire** (« ×2 crédits ») ; **un seul achat** « Pack Pilote » (sans pub + 3 voitures) à 2-3 € ; **jamais** de pub forcée en pleine course ni de loot box | Gratuit, rien d'imposé | Élevé | Moyen | P2 (v1.1, après A05) | Nécessite A05 ; consentement RGPD (UMP) ; formulaire Sécurité des données à refaire ; avis d'avocat (J-17) |
| A07 | Taille | *Observé* : décors jusqu'à 700 Mo | **Version publique < 150 Mo à installer** (Corniche seule, WFS-405) ; autres cartes en téléchargement optionnel ; **plafond 300 Mo par carte** proposé (remet en cause la règle des 700 Mo) | Installation rapide, moins de désinstallations | Élevé | Faible | P1 | Décision utilisateur (contredit E15) |
| A08 | Fiche store / ASO | *Supposé* : pas de captures ni vidéo prêtes (J-02) | 6 captures **en jeu**, vidéo 20 s (départ de nuit, Poursuite, vue Cockpit), titre + mot-clé (« course de voitures, police »), description sans marque | Comprendre le jeu en 3 s | Élevé | Faible | P1 | Captures du banc PC = aspect CI ; préférer le téléphone |
| A09 | Mesure | *Observé* : aucune analytique | Au départ **Play Console seulement** (installations, désinstallations, plantages, ANR, notes) : aucun SDK, pas de nouveau formulaire RGPD | Bugs corrigés plus vite | Moyen | Faible | P1 | Regarder Android Vitals chaque jour la 1re semaine |
| A10 | Conduite | *Observé* : 2 refus direction, WFS-258 suspendu | **Ne plus toucher à la conduite avant la sortie** sauf bloquant ; tutoriel 10 s au 1er lancement (gauche/droite, frein, aide à la conduite activée par défaut) | Prise en main immédiate | Moyen | Faible | P2 | Chaque retouche risque une régression |
| A11 | Rétention | *Supposé* | **Défi du jour** (une course, un objectif, une récompense ; on ne perd rien si on le rate) + classement local de ses records | Revenir 2 min par jour | Moyen | Moyen | P2 | Pas de FOMO ni de punition de l'absence |
| A12 | Duel en ligne | *Observé* : relais Cloudflare, pseudo libre (J-12) | **Masquer le duel en v1.0** (moins de RGPD, moins de serveur à surveiller) ; le remettre en v1.2 | Moins de bugs réseau | Moyen | Faible | P3 | Perte d'une fonction « wahou » ; à décider par l'utilisateur |

**Quick wins** : A01, A02, A03, A07, A08, A09. **Structurantes** : A04, A05, A06. **Expérimentales** : A11, A12.

### 3. Plan proposé (si le test fermé de 14 jours s'applique)
- **J0 (aujourd'hui / demain)** : ouvrir le test fermé (A01) ; gel des fonctionnalités (A02) ; juridique en cours (A03) ; nom du jeu décidé.
- **J1 à J3** : recette par lots des 81 🟡 ; seuls les bloquants sont corrigés ; critère 30 img/s Corniche (A04).
- **J3 à J12** : boucle « Carrière » minimale (A05), fiche store et vidéo (A08), tutoriel (A10). Retours des testeurs = seule source de vérité.
- **J14** : sortie publique gratuite **sans pub** (premières notes, premiers avis).
- **v1.1 (≈ 2 semaines après)** : pub récompensée + « Pack Pilote » (A06), après l'avis d'avocat. Si le compte n'a pas l'obligation de test fermé : même ordre, en compressant J1-J12.

### 4. Top 3
1. **A01 test fermé ouvert aujourd'hui** : c'est le chemin critique ; chaque jour perdu repousse la sortie d'un jour.
2. **A02 gel + recette par lots** : 81 corrections non validées = le vrai risque de la sortie.
3. **A05 boucle « Carrière »** : sans elle, aucun revenu possible, quel que soit le modèle.

**Hypothèse testable** : si nous ajoutons la boucle « crédits → déblocages » (A05), alors les testeurs du test fermé feront plus d'une course par jour,
ce que nous vérifierons avec le nombre de sessions par testeur dans la Play Console et leurs retours écrits.

**À dire franchement** : avec un seul jeu, sans budget marketing, les premiers revenus seront **modestes** (aucun chiffre inventé ici). Le levier n'est pas
d'ajouter du réalisme, c'est **un jeu fluide, léger, avec une raison de revenir**, puis une monétisation qui ne gêne pas.

**Décision** : à l'utilisateur, qui en parle au PO (`@po`) s'il le souhaite.

---

## R05 — 2026-11-12 — Mode Carrière détaillé (suite de R04, A05 et A06)

Mode Prototype + Game design, demandé par l'utilisateur (« tu proposes quoi avec plus de détails »). **Propositions** : l'utilisateur choisit et demande au PO.
Sources : `configgame.json` (section `voitures`, 20 voitures dont 3 retirées par WFS-404), `src/game/sauvegarde.h` (records par mode, voiture, peinture),
`src/ui/garage.cpp`, `src/ui/ecrans/menu.cpp` (Course solo, Duel), modes Checkpoint / Course à 5 / Poursuite. Tous les chiffres ci-dessous sont des
**valeurs de départ à régler** pendant le test fermé, pas des mesures.

### 1. Principe
Menu : **Carrière** (nouveau, en premier) · **Course libre** (l'actuelle « Course solo », avec les voitures possédées) · Duel (masqué en v1.0, R04-A12) · Garage · Réglages.
Une boucle : **épreuve → crédits + étoiles → acheter une voiture / une peinture → épreuves plus dures**.

### 2. Structure : 4 ligues sur la Corniche (seule carte publique, WFS-405)
| Ligue | Voitures exigées | Épreuves | Pour l'ouvrir | Exemples d'épreuves |
|---|---|---|---|---|
| 1 Rookie | Classe D | 6 | — | Checkpoint de jour ; Course à 5 au coucher ; 1ʳᵉ Poursuite courte |
| 2 Street | Classe C | 8 | 12 étoiles | Checkpoint de nuit ; Course à 5 sous la pluie ; Poursuite 2 voitures de police |
| 3 Pro | Classe B | 8 | 30 étoiles | Course à 5 de nuit sous la pluie ; Poursuite avec barrages |
| 4 Légende | Classe A / S | 6 | 55 étoiles | Course à 5 contre des A ; Poursuite longue ; « finale » de nuit |
28 épreuves écrites **dans `configgame.json`** (mode, moment, météo, classe, objectifs) : aucune carte ni aucun mode nouveau à coder, seulement des combinaisons de l'existant.

### 3. Étoiles (1 à 3 par épreuve, 84 en tout)
- ★ finir l'épreuve (Checkpoint : arriver au bout ; Course à 5 : top 3 ; Poursuite : survivre / s'échapper).
- ★★ objectif chiffré (temps, 1ʳᵉ place, score).
- ★★★ objectif « style » : X frôlements, aucun choc, aucun checkpoint manqué.
Refaire une épreuve n'apporte que les **nouvelles** étoiles, mais toujours des crédits (moins : 50 %).

### 4. Gains (crédits, monnaie du jeu)
| Action | Crédits |
|---|---|
| Épreuve finie | 100 (ligue 1) → 400 (ligue 4) |
| Par étoile **nouvelle** | +100 |
| Frôlement d'une voiture du trafic | +10 (plafond 200 par course) |
| Checkpoint en avance | +20 |
| Course à 5 : 1ʳᵉ / 2ᵉ / 3ᵉ | +300 / +150 / +75 |
| Poursuite gagnée | +250 |
| Course libre | 50 % des gains (pour que la Carrière reste la voie principale) |
Moyenne visée : **≈ 250 crédits par course de 3 min** en ligue 1, ≈ 600 en ligue 4.

### 5. Voitures et prix (17 voitures actuelles ; les 2 remplaçantes de WFS-360 viendront plus tard)
| Classe | Voitures (id) | Prix | Temps de jeu visé |
|---|---|---|---|
| D (offertes, au choix 1 sur 3 au départ, les 2 autres à 1 500) | p208, corsa, abarth | 0 / 1 500 | dès le départ |
| C | golf, tcross, compass, model3, v60, s3 | 3 000 à 5 000 | 1ʳᵉ voiture ≈ 1 h |
| B | gt86, giulia, mustang, rrsport, m4 | 10 000 à 16 000 | ≈ 5 h |
| A | p911, gallardo | 30 000 à 35 000 | ≈ 15 h |
| S | sf90 | 60 000 | ≈ 25-30 h |
- **Essai gratuit** : chaque voiture bloquée peut être essayée **une fois** en Course libre (donne envie, sans la donner).
- **Peintures** : 200 à 800 crédits (petites récompenses fréquentes) ; la peinture d'origine reste offerte.
- **Pas d'améliorations moteur** en v1 (équilibrage trop risqué ; la conduite est réglée sur les fiches réelles).

### 6. Écrans à toucher (réutiliser l'existant)
1. **Fin de course** : détail des gains ligne par ligne qui défile, étoiles qui s'allument, total ; bouton « ×2 avec une vidéo » (v1.1, A06).
2. **Garage** (`src/ui/garage.cpp`) : cadenas + prix sur les voitures bloquées, bouton « Acheter » avec confirmation, « Essayer » une fois.
3. **Carrière** (nouvel écran, `src/ui/ecrans/`) : 4 ligues en cartes, épreuves avec leurs étoiles, cadenas « 12 ★ requises ».
4. **HUD du menu** : solde de crédits et total d'étoiles en haut.
5. **Sauvegarde** (`src/game/sauvegarde.*`) : crédits, voitures possédées, peintures possédées, étoiles par épreuve, essais utilisés. **Joueurs existants** (testeurs) : 5 000 crédits offerts + la voiture qu'ils utilisent déjà.

### 7. Monétisation (v1.1, après l'avis d'avocat, R04-A06)
- **Pub récompensée volontaire** en fin de course : « ×2 crédits », **5 fois par jour au plus**. Jamais de pub imposée, jamais en course.
- **Pack Pilote** (achat unique, 2,99 € proposé) : plus aucune pub + crédits ×2 (le bouton de pub donne directement le double) + 10 000 crédits au départ.
- **Refusé volontairement** : caisses au hasard, énergie qui limite le nombre de courses, voitures vendues seulement contre de l'argent réel, timers.
- Prérequis : SDK de pub + consentement RGPD (UMP), Google Play Billing, formulaire « Sécurité des données » et confidentialité refaits, IARC (pubs).

### 8. Découpage proposé (au PO de décider et d'écrire les règles de gestion)
| # | Ticket proposé | Épic / zone | Taille | Dépend de |
|---|---|---|---|---|
| C1 | Sauvegarde Carrière : crédits, voitures et peintures possédées, étoiles, migration des anciens joueurs + tests CPU | E3 | ≤ 1 j | — |
| C2 | Calcul des gains et des étoiles en fin de course (règles du § 3 et § 4, valeurs dans `configgame.json`) + tests | E3 | ≤ 1 j | C1 |
| C3 | Écran de fin : gains détaillés, étoiles, solde | E8 | ≤ 1 j | C2 |
| C4 | Garage : cadenas, prix, achat, essai gratuit | E5 | ≤ 1 j | C1 |
| C5 | Écran Carrière + 28 épreuves dans `configgame.json` | E8 + E3 | 1-2 j (à découper) | C2 |
| C6 | Banc d'équilibrage : simulation de 30 h de jeu (gains moyens → date d'achat de chaque classe) | E3 | ≤ 1 j | C2 |
| C7 | Tutoriel de 10 s à la 1ʳᵉ épreuve de la ligue 1 | E8 | ≤ 1 j | C5 |
| M1 | Pub récompensée + consentement (v1.1) | E7 | 1-2 j | C3, avocat |
| M2 | Pack Pilote via Google Play Billing (v1.1) | E7 | 1-2 j | M1 |
C1, C4 et C6 peuvent avancer **en parallèle** (zones différentes). Ordre conseillé : C1 → (C2, C4) → (C3, C5, C6) → C7.

### 9. Hypothèse testable
Si nous ajoutons la Carrière (ligues, étoiles, crédits, voitures à acheter), alors les testeurs du test fermé joueront plusieurs jours de suite et
plusieurs courses par session, ce que nous vérifierons avec les statistiques de la Play Console (utilisateurs actifs par jour sur les 14 jours) et
une question simple posée aux testeurs : « Quelle voiture voulez-vous acheter ensuite ? ».

### 10. Risques
- **Équilibrage** : trop lent → abandon ; trop rapide → plus rien à viser. D'où C6 et les valeurs dans `configgame.json` (réglables sans recompiler le code).
- **Temps** : ≈ 6-8 jours de développement en parallèle ; tient dans les 14 jours du test fermé seulement si le gel des autres fonctionnalités est respecté (R04-A02).
- **Conduite** : les épreuves de la ligue 4 exigent une conduite stable (WFS-258/259) ; sinon garder la ligue 4 pour la v1.1.

---

## R06 — 2026-11-12 — « Duel entre amis » rangé dans la Course solo (mode UX/UI)

Remarque de l'utilisateur (2026-11-12) : « c'est bizarre, dans le mode solo je trouve jouer avec des amis ».
**Constat observé** dans le code : le duel est proposé **deux fois**, sous deux noms différents :
- au menu principal : bouton « **Duel en ligne** » (`src/ui/ecrans/menu.cpp:55`) ;
- **dans** « Course solo », 4ᵉ carte de l'écran des modes : « **Duel entre amis** » (`src/game/modes_ecran.cpp:42`, WFS-112), qui mène au même salon.
Aucun ticket ni aucune anomalie ne le couvre (`TACHES.md`, `anno.md` vérifiés ; ANO-63 ne parlait que des textes).

| ID | Domaine | Constat / opportunité | Proposition d'amélioration | Bénéfice joueur | Impact | Complexité | Priorité | Risques / points à vérifier |
|---|---|---|---|---|---|---|---|---|
| A01 | UX/UI | *Observé* : duel en double, « solo » contredit « amis » | Retirer la carte « Duel entre amis » de l'écran des modes de la Course solo ; le duel reste **seulement** au menu, sous un seul nom (« Duel en ligne ») | Menu clair : solo = seul, duel = à deux | Moyen | Faible (1 ligne de tableau + banc `modes`) | P1 | Le banc `modes_ecran` teste « Duel → salon » (`modes_ecran.cpp:303`) : à mettre à jour ; disposition à 3 cartes au lieu de 4 |
| A02 | UX/UI | *Observé* : R04-A12 propose de cacher le duel en v1.0 | Si le duel est caché pour la sortie publique, le retirer **des deux endroits** et le remettre au menu en v1.2 | Pas de bouton qui mène à une fonction pas prête | Moyen | Faible | P2 | Décision de l'utilisateur |

**Recommandation** : A01 tout de suite (petit ticket E3, zone de `src/game/modes_ecran.cpp`) ; A02 selon votre choix sur le duel.

---

## R07 — 2026-11-12 — Nouveaux modes de jeu (mode Créatif + Gameplay)

Question de l'utilisateur : « tu n'as pas d'autre idée que Checkpoint, Course à 5 et Poursuite ? ». **Propositions** ; l'utilisateur choisit.
Ce qui existe déjà et peut servir (observé) : quasi-contacts avec combo et 250 × combo points, nitro (`src/game/course.h:39`), chocs, accidents,
trafic, police et barrages (Poursuite), IA de la Course à 5 (`course5.*`), fantôme (`fantome.*`), nuit, pluie, checkpoints.
Règle à respecter : tout objet nouveau visible (radar, colis…) doit être un **vrai modèle 3D** (pas de forme nue, CLAUDE.md).

| ID | Domaine | Constat / opportunité | Proposition d'amélioration | Bénéfice joueur | Impact | Complexité | Priorité | Risques / points à vérifier |
|---|---|---|---|---|---|---|---|---|
| A01 | Mode | *Observé* : combo de quasi-contacts déjà codé mais caché dans le Checkpoint | **« Heure de pointe »** : 2 min, trafic de plus en plus dense, score = frôlements × combo × vitesse ; un choc casse le combo, 3 accidents = fin | Le mode le plus « street » : prise de risque, record à battre | Élevé | Faible (réutilise `nearMiss`, `traffic`) | P1 | Densité du trafic et performance du téléphone |
| A02 | Mode | *Observé* : IA de la Course à 5 | **« Élimination »** : 5 voitures, le dernier est éliminé à chaque checkpoint, jusqu'au duel final | Tension croissante, chaque dépassement compte | Élevé | Faible | P1 | Lisibilité au HUD (« ÉLIMINÉ ») |
| A03 | Mode / Carrière | *Observé* : Course à 5 avec 1 seul adversaire possible | **« Rival »** : 1 contre 1 face au « boss » de chaque ligue de la Carrière (R05), qui a un nom, une voiture et une peinture ; le battre ouvre la ligue suivante et donne une remise sur sa voiture | Un objectif incarné, une fin de ligue | Élevé | Faible à Moyen | P1 (avec R05) | Pas de marque dans les noms ; IA du rival à régler (WFS-394) |
| A04 | Mode | *Supposé* | **« Convoi fragile »** : aller d'un point A à un point B dans le temps ; chaque choc retire 10 % de la prime | Conduite propre récompensée (l'inverse de l'Heure de pointe) | Moyen | Faible | P2 | Point d'arrivée = vrai lieu 3D de la Corniche (pas de marqueur nu) |
| A05 | Mode | *Supposé* | **« Radars »** : 5 radars fixes sur le parcours ; score = somme des vitesses au passage | Défi court, facile à comprendre, très rejouable | Moyen | Moyen (modèle 3D réel de radar à intégrer) | P2 | Modèle CC0 de radar aux vraies mesures ; message « vitesse excessive » à éviter dans la fiche (PEGI) |
| A06 | Mode | *Observé* : police et fuyard déjà codés | **« Interception »** (inversé de la Poursuite) : vous êtes la police, vous devez arrêter un fuyard avant la fin du temps | Rôle nouveau, utilise la voiture de police (WFS-460) | Élevé | Moyen à Élevé (IA du fuyard) | P3 | Après la stabilisation de la conduite |
| A07 | Social | *Observé* : fantôme enregistré ; Duel en ligne fragile | **« Défi fantôme »** : envoyer son fantôme à un ami par un code ; il court contre lui quand il veut (pas de temps réel) | Jouer « avec des amis » sans les contraintes du direct | Élevé | Moyen (relais existant, petit fichier) | P2 (remplace le Duel en v1.0, R04-A12) | Données échangées → formulaire Sécurité des données |
| A08 | Rétention | *Supposé* | **« Défi du jour »** : chaque jour, un mode + un objectif tiré dans A01 à A05 ; même défi pour tout le monde | Une raison de revenir 2 min par jour | Moyen | Faible (si A01-A05 existent) | P2 | Pas de punition si on le rate |

**Top 3** : A01 Heure de pointe (presque gratuit, le plus « street »), A02 Élimination (réutilise la Course à 5), A03 Rival (donne une histoire à la Carrière).
**Ordre conseillé** : dans les 14 jours du test fermé seulement A01 et A02 (petits) ; A03 avec la Carrière ; A07 en v1.1 pour remplacer le Duel ; A06 plus tard.
**Hypothèse testable** : si nous ajoutons l'Heure de pointe, alors les testeurs referont plusieurs fois la même course pour battre leur score,
ce que nous vérifierons en demandant aux testeurs leur mode préféré et en comptant les parties par mode dans le journal du jeu (sur leurs retours).

---

## R08 — 2026-11-12 — Défi aléatoire du jour sans coût de mélange (mode Prototype)

L'utilisateur aime le « défi aléatoire » (R07-A08) mais craint que « mélanger plus de défis » coûte cher. **Réponse proposée** : ne pas écrire de défis,
**les fabriquer à partir de pièces qui existent déjà**. Le coût est **fixe** (un générateur, un écran, une récompense) et ne grandit pas avec le nombre de défis.

### 1. Un défi = 4 tirages dans des listes (données dans `configgame.json`)
| Pièce | Valeurs possibles (existantes) |
|---|---|
| Mode | Checkpoint, Course à 5, Poursuite (+ Heure de pointe, Élimination quand ils existeront : ajoutés à la liste, rien d'autre à coder) |
| Moment / météo | jour, nuit, jour sous la pluie, nuit sous la pluie |
| Voiture | imposée parmi une classe (prêtée pour le défi, même si le joueur ne l'a pas : vitrine pour la Carrière R05) |
| Objectif | tiré dans la liste du mode : temps, place, X frôlements, aucun choc, distance |
- **Même défi pour tout le monde, sans serveur** : le tirage part de la date du jour (graine = AAAAMMJJ). Rien à envoyer, rien à stocker en ligne.
- 3 modes × 4 moments × 17 voitures × ≈ 4 objectifs ≈ **800 défis différents** sans écrire une seule épreuve.

### 2. Ce qui évite les défis ratés (le vrai risque du mélange)
- **Liste d'interdits** dans les données : ex. Poursuite + voiture lente, pluie + objectif « aucun choc » en Difficile.
- **Seuils calculés, pas inventés** : l'objectif est réglé par classe de voiture (temps de référence de la classe × 1,1, par exemple).
- **Banc CPU** : générer les 365 défis de l'année et vérifier qu'aucun n'est interdit ni impossible (seuil hors limites) ; captures de 3 défis au banc PC.

### 3. Coût estimé (qualitatif)
| Tâche | Zone | Taille |
|---|---|---|
| Générateur à graine + listes et interdits dans `configgame.json` + banc des 365 jours | E3 | ≤ 1 j |
| Écran « Défi du jour » (carte au menu : mode, moment, voiture, objectif) + fin de défi | E8 | ≤ 1 j |
| Récompense (crédits, R05) et « déjà fait aujourd'hui » dans la sauvegarde | E3 | ≤ 0,5 j |
Sans Carrière (R05), la récompense est seulement un record du jour : le défi marche quand même.

### 4. À éviter
Pas de série de jours qui se perd, pas de « dernière chance », pas de pub imposée pour relancer un défi raté (règles du mode Rétention).

**Hypothèse testable** : si un défi du jour change chaque jour, les testeurs ouvriront le jeu plusieurs jours de suite, ce que nous vérifierons avec les
utilisateurs actifs par jour de la Play Console pendant le test fermé.

---

## R09 — 2026-11-12 — Cadrage de l'utilisateur : 3 modes, étoiles / médailles / objectifs / cadeau du jour, peintures et dessins, cartes à télécharger, voitures à débloquer

**Choix de l'utilisateur (2026-11-12)** : pas de nouveau mode pour l'instant (Checkpoint, Course à 5, Poursuite seulement ; R07 et R08 mis de côté) ;
ajouter un système **score + étoiles + médailles + objectifs + cadeau du jour** ; **peintures plus attirantes et dessins sur les voitures** ;
**une seule carte par défaut**, les autres **à télécharger** ; **3 voitures gratuites**, les autres **achetées avec les étoiles / médailles**, **gagnées dans le cadeau du jour**
ou **achetées avec de l'argent réel**. Ci-dessous : la manière la plus simple de le faire, et les points d'attention. Remplace R05 § 2 à 5 (ligues et crédits).

### 1. Une seule monnaie de jeu : les étoiles (proposition de simplification)
Deux monnaies (étoiles + médailles + crédits) perdent le joueur. Proposé :
- **Médaille** = la qualité d'une course : 🥉 bronze, 🥈 argent, 🥇 or (seuils par mode et par classe de voiture).
- **Étoiles** = ce que rapporte une médaille, et **la seule monnaie** : bronze 1 ★, argent 2 ★, or 3 ★ ; + objectifs (§ 2) ; + cadeau du jour (§ 3).
- **Score** : celui du jeu (points, combo de frôlements, temps) décide de la médaille ; affiché en fin de course avec la médaille et les étoiles gagnées.
- Le **total d'étoiles gagnées depuis le début** reste affiché (niveau du joueur), même après dépense.
| Mode | 🥉 | 🥈 | 🥇 (exemples à régler par classe) |
|---|---|---|---|
| Checkpoint | 3 checkpoints | 6 | 10 |
| Course à 5 | 3ᵉ | 2ᵉ | 1ᵉʳ |
| Poursuite | tenir 60 s | 120 s | s'échapper |
Médaille améliorée = seulement les étoiles en plus (argent puis or : +1 ★), pour éviter de « farmer » la même course.

### 2. Objectifs
- **3 objectifs du jour**, simples, tirés dans une liste (« 20 frôlements », « une course de nuit », « finir sans choc en Poursuite ») : +2 ★ chacun.
- **Objectifs permanents** (succès) : « 1ʳᵉ médaille d'or », « 1 000 frôlements », « gagner avec chaque voiture » : +5 à +20 ★, une seule fois.

### 3. Cadeau du jour
- 1 cadeau par **jour de jeu** (jours non consécutifs : **rien ne se perd si on ne joue pas**).
- Calendrier visible de 7 cadeaux : jours 1-6 = étoiles, peinture ou dessin ; **7ᵉ cadeau = une voiture au choix parmi 3 proposées**, puis le calendrier recommence.
- **Le contenu est montré à l'avance, pas tiré au hasard** : pas d'effet « caisse mystère », et rien du cadeau ne s'achète avec de l'argent réel (risque juridique des loot boxes).

### 4. Voitures
| Groupe | Voitures (17 aujourd'hui) | Étoiles | Argent réel (proposé) |
|---|---|---|---|
| Gratuites | 208, Corsa, 500 (« p208 », « corsa », « abarth ») | 0 | — |
| Citadines / SUV | golf, tcross, compass, model3, v60, s3 | 30 à 50 ★ | 0,99 € |
| Sportives | gt86, giulia, mustang, rrsport, m4 | 100 à 150 ★ | 1,99 € |
| Haut de gamme | p911, gallardo | 250 à 300 ★ | 2,99 € |
| Exception | sf90 | 500 ★ | 3,99 € |
- **Toutes les voitures restent gagnables sans payer** : payer fait seulement gagner du temps (jeu solo, aucun avantage contre d'autres joueurs).
- **Essai gratuit une fois** de chaque voiture bloquée.
- Optionnel : **Pack « Toutes les voitures »** à prix fixe (ex. 9,99 €) au lieu de 14 achats séparés : plus clair, moins de « nickel and dime ».
- **Points juridiques** (rapport du juriste, J-17) : Google Play Billing obligatoire, prix TTC affichés, CGU et confidentialité à refaire, mention « achats intégrés » sur la fiche, **avis d'avocat avant la mise en vente** ; voitures sans marque (WFS-404) **obligatoire** avant toute vente.

### 5. Peintures et dessins (constat observé)
**Aujourd'hui** : 16 couleurs **unies et plutôt ternes** (`src/game/sauvegarde.h:34` : blanc, noir, gris, rouge foncé, bleu, bordeaux, sable…), sans finition.
- **Finitions** (shader véhicule, épic E10) : métallisé, nacré, **mat**, chromé / irisé (« caméléon »), candy ; mêmes couleurs + 12 couleurs vives (rouge vif, bleu électrique, vert acide, rose, orange fluo, jaune citron…).
- **Dessins (vinyles)** : bandes de course, numéro de course, flammes, motifs tribaux, damier, camouflage, dégradés, éclaboussures ; **dessins originaux** (faits pour le jeu ou CC0), **aucun logo ni sponsor réel** ; posés comme de vrais adhésifs (suivent les formes, pas de bande plate qui dépasse : cf. ANO-250 de la police).
- **Couleur secondaire** (toit, rétroviseurs, jantes) : fort effet pour peu de travail.
- Prix : peintures 5-15 ★, dessins 10-30 ★, certains seulement dans le cadeau du jour.
- Réalisme : un vinyle réel est **réaliste** ; rester dans des couleurs et motifs qui existent sur de vraies voitures de rue « tuning » (DA-realisme.md).

### 6. Cartes
- **Dans l'APK** : la Corniche seule (déjà décidé : WFS-405).
- **À télécharger, gratuitement**, depuis l'écran des paquets (épic E14, déjà fait pour la Ville et la Corniche) : Ville, Campagne, Grand Prix, Neige.
- Taille par carte à afficher **avant** le téléchargement ; plafond conseillé ≤ 300 Mo (R04-A07).
- Option plus tard : une carte « offerte » contre des étoiles pour donner un objectif (pas en v1.0).

### 7. Découpage proposé (au PO de décider)
| # | Ticket proposé | Épic | Taille |
|---|---|---|---|
| P1 | Sauvegarde : étoiles (solde + total), médailles par mode × voiture, voitures / peintures / dessins possédés, cadeaux reçus ; migration des testeurs | E3 | ≤ 1 j |
| P2 | Médailles et étoiles de fin de course (seuils dans `configgame.json`) + écran de fin | E3 + E8 | 1 j |
| P3 | Garage : cadenas, prix en ★, achat, essai gratuit ; 3 voitures gratuites | E5 | 1 j |
| P4 | Objectifs du jour + succès | E3 + E8 | 1 j |
| P5 | Cadeau du jour (calendrier de 7, contenu visible) | E8 | ≤ 1 j |
| P6 | Peintures : finitions métal / nacre / mat / caméléon + 12 couleurs vives + couleur secondaire | E10 | 1-2 j (agent `@dev3d`) |
| P7 | Dessins (vinyles) : 10 dessins originaux, pose réaliste sur les 17 voitures | E10 | 2 j (agent `@dev3d`, à découper) |
| P8 | Banc d'équilibrage des étoiles (temps de jeu pour chaque groupe de voitures) | E3 | ≤ 1 j |
| M1 | Achats intégrés (Google Play Billing) : voitures + pack ; après avis d'avocat (v1.1) | E7 | 2 j |
Ordre conseillé : P1 → (P2, P3, P6 en parallèle) → (P4, P5, P7, P8) → M1.

**Hypothèse testable** : si le joueur gagne des étoiles à chaque course et voit la prochaine voiture à débloquer, il fera plusieurs courses par session
et reviendra les jours suivants (cadeau), ce que nous vérifierons avec les utilisateurs actifs par jour de la Play Console pendant le test fermé.

---

## R10 — 2026-11-12 — Garder l'historique du joueur (compte, sauvegarde) sans demander nom et prénom

Remarque de l'utilisateur : « on a oublié d'authentifier un joueur, son nom, prénom…, pour qu'il garde son historique ».
**Constat observé** : la sauvegarde est un fichier local (`src/game/sauvegarde.cpp:241`) et la sauvegarde Android est **coupée**
(`android/app/src/main/AndroidManifest.xml:18` : `android:allowBackup="false"`). Aujourd'hui, **désinstaller ou changer de téléphone = tout perdre**
(records, et demain étoiles, voitures débloquées, achats). Avec R09 (étoiles, voitures), c'est un vrai problème.

| ID | Domaine | Constat / opportunité | Proposition d'amélioration | Bénéfice joueur | Impact | Complexité | Priorité | Risques / points à vérifier |
|---|---|---|---|---|---|---|---|---|
| A01 | Sauvegarde | *Observé* : `allowBackup="false"` | **Sauvegarde automatique Android** (Auto Backup vers le compte Google du téléphone) : activer `allowBackup`, règles qui ne gardent que le fichier de sauvegarde (pas les paquets de cartes ni les caches) | Rien à faire : la progression revient au changement de téléphone | Élevé | Faible (manifest + règles XML, E7) | P1 (v1.0) | Copie faite par Android ≈ 1 fois par jour (pas instantanée) ; à déclarer dans « Sécurité des données » (sauvegarde Google, pas de serveur à nous) |
| A02 | Compte | *Supposé* | **Google Play Jeux (Play Games Services v2)** : connexion **automatique** sans formulaire, pseudo Play Jeux, **sauvegarde dans le cloud** (Saved Games), **succès** (les objectifs permanents de R09) et **classements** par mode | Progression sur tous ses téléphones, succès visibles, classement mondial | Élevé | Moyen (SDK Java + pont JNI, E7) | P2 (v1.1) | Bibliothèque Google à ajouter (licence, taille) ; déclarer dans « Sécurité des données » ; jeu jouable sans connexion |
| A03 | Achats | *Observé* : achats prévus en v1.1 (R09) | Les achats faits avec Google Play **se restaurent seuls** (Play Billing) : pas besoin de compte à nous pour ne rien perdre | Achat jamais perdu | Élevé | Faible (inclus dans M1) | P2 | — |
| A04 | Identité | *Supposé* | **Ne PAS demander nom, prénom, e-mail, mot de passe** : un **pseudo** suffit (il existe déjà pour le duel, `src/ui/ecrans/salon.cpp:257`) | Entrer dans le jeu en 1 s | Élevé | Faible | P1 | Un compte maison obligerait : serveur, sécurité des mots de passe, RGPD (données personnelles), **suppression du compte depuis l'appli et un site** (règle Google Play), plus de formulaires au départ = plus d'abandons |

**Recommandation** : v1.0 = A01 + A04 (presque gratuit) ; v1.1 = A02 avec les achats (A03). Pas de compte maison.
**Validation** : sur le téléphone de l'utilisateur, gagner un record, faire une sauvegarde forcée (`adb shell bmgr backupnow <paquet>`), désinstaller, réinstaller : le record revient.

---

## R11 — 2026-11-12 — PROPOSITION FINALE pour le PO (synthèse de R04, R06, R09, R10, avec les ajustements discutés)

Synthèse demandée par l'utilisateur avant de la présenter au PO. **Remplace** les parties correspondantes de R04, R05, R06, R09 et R10 ; R07 et R08 (nouveaux modes, défi aléatoire) sont **mis de côté** par l'utilisateur.
Objectif : **sortir un jeu fluide, léger, avec une raison de revenir, puis gagner un peu d'argent** sans gêner le joueur.

### Étape 0 — Tout de suite (avant tout nouveau code)
| # | Action | Qui |
|---|---|---|
| 0.1 | Vérifier le type de compte Play Console ; si compte personnel récent : **ouvrir le test fermé aujourd'hui** (≥ 12 testeurs, 14 jours) avec l'AAB de WFS-319 | Utilisateur + `@cicd` |
| 0.2 | **Gel des fonctionnalités** hors liste ci-dessous jusqu'à la sortie | PO |
| 0.3 | **Recette par lots** des 81 tickets 🟡 : 30 min sur l'APK de `main` (Corniche, jour / nuit, Basse) ; OK → ✅, KO → 1 ticket | Utilisateur + PO |
| 0.4 | Lancer **maintenant** les tickets juridiques de la sortie (WFS-405, 407, 408, 409, 320) ; décider le **nom du jeu** (risque « …for Speed », J-09) | PO + `@juriste` |
| 0.5 | Critère de sortie : **Corniche ≥ 30 img/s stables en Basse sur le S21 FE** | PO |

### Étape 1 — v1.0 (sortie publique, gratuite, sans pub ni achat)
| # | Ticket proposé | Épic | Taille |
|---|---|---|---|
| V1 | Retirer « Duel entre amis » de l'écran des modes de la Course solo (`src/game/modes_ecran.cpp:42`) ; **Duel en ligne caché** du menu en v1.0 | E3 + E8 | ≤ 0,5 j |
| V2 | **Sauvegarde automatique Android** (`allowBackup` + règles : seulement le fichier de sauvegarde) ; **pseudo seul**, jamais nom / prénom / e-mail | E7 | ≤ 0,5 j |
| V3 | Sauvegarde du système : étoiles (solde + total gagné), médailles par mode × voiture, voitures / peintures possédées ; migration des testeurs | E3 | ≤ 1 j |
| V4 | **Médailles 🥉🥈🥇 et étoiles** en fin de course (bronze 1 ★, argent 2 ★, or 3 ★ ; seuils par mode et par classe dans `configgame.json`) + écran de fin avec la barre « prochaine voiture » | E3 + E8 | 1 j |
| V5 | **Garage** : cadenas, prix en ★, achat avec confirmation, **essai gratuit une fois** de chaque voiture bloquée | E5 | 1 j |
| V6 | **Peintures** : finitions métallisé, nacré, mat, caméléon + 12 couleurs vives + couleur secondaire (toit, rétros, jantes) | E10 (`@dev3d`) | 1-2 j |
| V7 | **Banc d'équilibrage** : temps de jeu pour débloquer chaque groupe de voitures | E3 | ≤ 1 j |
| V8 | Cartes : **Corniche seule dans l'APK** (WFS-405) ; Ville, Campagne, Grand Prix, Neige **à télécharger gratuitement**, taille affichée avant | E14 | déjà en cours |
Ordre : V1, V2 (petits) → V3 → (V4, V5, V6 en parallèle) → V7.

**Économie de départ (valeurs à régler pendant le test fermé)**
| Groupe | Voitures | Prix |
|---|---|---|
| Gratuites | **208, 500 et GT86** (2 citadines + 1 sportive pour une 1ʳᵉ impression « street ») | 0 |
| Citadines / SUV | Corsa, Golf, T-Cross, Compass, Model 3, V60, S3 | 30-50 ★ |
| Sportives | Giulia, Mustang, Range Rover Sport, M4 | 100-150 ★ |
| Haut de gamme | 911, Gallardo | 250-300 ★ |
| Exception | SF90 | 500 ★ |
- **1ʳᵉ course** : une supercar prêtée (911) pour donner envie.
- Médaille améliorée = seulement les étoiles en plus (pas de « farm »).
- Peintures 5-15 ★.

### Étape 2 — v1.1 (≈ 2 semaines après la sortie, selon les retours)
| # | Ticket proposé | Épic | Taille |
|---|---|---|---|
| W1 | **Objectifs** : 3 du jour (+2 ★ chacun) + succès permanents (+5 à +20 ★) | E3 + E8 | 1 j |
| W2 | **Cadeau du jour** : calendrier de 7 **jours joués** (rien ne se perd) ; contenu **affiché à l'avance** ; jours 1-6 étoiles / peinture / dessin ; **7ᵉ = une voiture du groupe Citadines au choix parmi 3, ou un gros paquet d'étoiles** | E8 | ≤ 1 j |
| W3 | **Dessins (vinyles)** originaux : bandes, numéro, flammes, damier, camouflage, dégradés ; aucun logo ni sponsor ; pose réaliste | E10 (`@dev3d`) | 2 j |
| W4 | **Google Play Jeux** : connexion automatique, sauvegarde cloud, succès, classements par mode | E7 | 2 j |
| W5 | **Pub volontaire** « ×2 étoiles » en fin de course, 5 par jour au plus, jamais imposée, jamais en course ; consentement RGPD | E7 | 1-2 j |
| W6 | **Achats** (Google Play Billing) : voitures 0,99 à 3,99 € **ou** pack « toutes les voitures » ≈ 9,99 € ; « sans pub » ; achats restaurés automatiquement | E7 | 2 j |
**Conditions de W5 et W6** : avis d'avocat (J-17), CGU, confidentialité et « Sécurité des données » refaites, prix TTC, fiche « contient des pubs / achats ».
**Tout reste gagnable sans payer** ; aucune caisse au hasard, aucune « énergie », aucun minuteur.

### Mis de côté (décision de l'utilisateur)
Nouveaux modes (Heure de pointe, Élimination, Rival, Radars, Interception, Défi fantôme), défi aléatoire du jour (R07, R08), compte maison avec nom / prénom.

### Comment on saura que ça marche
Pendant le test fermé puis après la sortie : utilisateurs actifs par jour, désinstallations, plantages et ANR, notes (Play Console) ; question aux testeurs :
« Quelle voiture voulez-vous débloquer ensuite ? ».

### Message à donner au PO
> `@po` Lance la proposition finale R11 de `recommandations-smart-brain.md` : étape 0 tout de suite, puis les tickets V1 à V7 pour la v1.0 dans l'ordre indiqué, et W1 à W6 en attente pour la v1.1.

---

## R12 — 2026-11-12 — Correctif de R11 : gagner de l'argent dès la v1.0 (décision de l'utilisateur)

**Décision de l'utilisateur** : « ne change rien, l'objectif est de gagner de l'argent à partir de la première version ». R11 reste valable **sauf** ce qui suit.

1. **W5 (pub volontaire « ×2 étoiles ») et W6 (achats) passent dans la v1.0**, après V5 (garage), sous les noms **V9** et **V10** :
   - **V9 — Pub volontaire** : en fin de course, « ×2 étoiles » contre une vidéo, 5 par jour au plus, jamais imposée, jamais en course ; écran de consentement RGPD au 1er lancement.
   - **V10 — Achats Google Play** : voitures de 0,99 à 3,99 €, pack « toutes les voitures » ≈ 9,99 €, option « sans pub » (supprime la proposition de vidéo et donne directement ×2) ; achats restaurés automatiquement.
2. **Prérequis ajoutés à l'étape 0** (sinon Google refuse la mise à jour, ou risque de retrait) :
   - 0.6 avis d'avocat sur la vente et la pub (J-17), CGU et politique de confidentialité mises à jour (pub, achats, éditeur, contact) ;
   - 0.7 compte marchand Google Play (paiements) ouvert par l'utilisateur, statut « commerçant » UE (J-15) ;
   - 0.8 formulaire « Sécurité des données » (identifiant publicitaire), mention « Contient des annonces » et « Achats intégrés », questionnaire IARC refait.
3. **Ordre v1.0** : V1, V2 → V3 → (V4, V5, V6) → **V9, V10** → V7 (équilibrage avec la pub et les achats).
4. W1 à W4 (objectifs, cadeau du jour, dessins, Google Play Jeux) restent en v1.1.

### Message à donner au PO
> `@po` Lance la proposition finale R11 de `recommandations-smart-brain.md` avec le correctif R12 : étape 0 (0.1 à 0.8) tout de suite, puis V1 à V10 pour la v1.0 (pub volontaire et achats compris), et W1 à W4 en attente pour la v1.1.

---

## R13 — 2026-11-12 — Avis sur la monnaie « étoiles + médailles » proposée par l'utilisateur (mode Challenge)

**Proposition de l'utilisateur** : les courses rapportent des **étoiles** (calcul selon les défis et les modes) ; **médaille de bronze = 100 ★** ;
**argent = 300 ★ ou 3 bronze** ; **or = 600 ★ ou 2 argent** ; au market, le joueur **n'achète pas les objets directement** : il achète des **médailles ou des étoiles**, puis les objets avec.

### Ce qui est bon
- Monnaie du jeu achetée avec de l'argent réel : modèle classique des jeux mobiles ; **peu de produits à déclarer** dans Google Play (3-5 packs au lieu d'un produit par voiture).
- Prix des objets **réglables sans toucher à Google Play** (données du jeu).
- Les taux sont cohérents : 3 bronze = 300 ★ = 1 argent ; 2 argent = 600 ★ = 1 or.

### Critiques
1. **Les médailles ne servent à rien de plus que les étoiles** : si 1 or = 600 ★ dans les deux sens, ce sont juste des « billets » de 100, 300, 600 ★. Deux noms pour une seule valeur = confusion.
2. **Conflit avec la médaille de fin de course** (R09-R12 : bronze / argent / or = résultat de la course). « Médaille d'or » ne peut pas vouloir dire à la fois « 1ʳᵉ place » et « 600 ★ ».
3. **Règles européennes sur les monnaies virtuelles** (*à faire vérifier par l'avocat*) : les autorités de protection des consommateurs de l'UE (réseau CPC, principes publiés en 2025) demandent notamment d'**afficher aussi le prix en euros** des objets payés en monnaie du jeu et de **ne pas forcer le joueur à acheter plus que nécessaire** (reliquats). Il faut des packs qui tombent juste par rapport aux prix des objets.
4. **Équilibrage** : si une course rapporte 10 à 40 ★, une médaille de bronze (100 ★) = 3 à 10 courses. Les prix des voitures de R11 (30-500 ★) sont à **multiplier** dans cette nouvelle échelle.

### Proposition (garde l'idée de l'utilisateur, enlève la confusion)
- **Une seule monnaie : les étoiles.** Les médailles deviennent des **packs d'étoiles** au market :
  | Pack | Étoiles | Prix proposé |
  |---|---|---|
  | Médaille de bronze | 100 ★ | 0,99 € |
  | Médaille d'argent | 300 ★ (+ 10 % offerts : 330 ★) | 2,49 € |
  | Médaille d'or | 600 ★ (+ 20 % offerts : 720 ★) | 4,49 € |
  | Coffre de champion | 5 médailles d'or (3 600 ★ + 25 %) | 19,99 € |
  Le bonus sur les gros packs fait acheter plus.
- **Fin de course** : le résultat devient un **rang** (« Rang C, B, A, S » ou une coupe), pour ne pas mélanger avec les médailles du market.
- **Gains en jouant** (exemples à régler par le banc d'équilibrage) : course finie 10 ★ ; + rang B 10 ★, A 20 ★, S 30 ★ ; + objectifs selon le mode (frôlements, checkpoints, place, poursuite) ; pub volontaire ×2.
- **Nouveaux prix des voitures** (même échelle) : citadines 300 ★ (1 argent), sportives 1 200 ★ (2 or), 911 / Gallardo 3 000 ★, SF90 6 000 ★ ; peintures 50-150 ★. Chaque prix affiché aussi en équivalent euros (point 3).

**Choix à faire par l'utilisateur** : (a) une seule monnaie, médailles = packs (recommandé) ; (b) deux monnaies séparées (étoiles gagnées, médailles seulement achetées, objets premium en médailles) : plus de revenus possibles, mais plus complexe et plus exposé aux règles européennes.

---

## R14 — 2026-11-12 — Ce qui manque au jeu actuel, hors étoiles / market (mode Diagnostic)

L'utilisateur valide la structure de R13 (option a : une seule monnaie, médailles = packs d'étoiles) et demande ce qui manque encore (« les peintures ? les dessins ? »).
Inventaire fait dans le code (`src/ui/ecrans/reglages.cpp:61-74`, `src/game/sauvegarde.h:34`, `src/ui/garage.cpp`, `src/audio/`, `assets/audio/`).

**Déjà là (observé)** : 3 modes, 17 voitures, garage et showroom, 16 couleurs, nitro, combo de frôlements, fantôme, nuit, pluie, mini-carte,
réglages complets (caméra 4 vues, qualité, son, musique, gaucher, glisser / flèches, gyroscope, aide à la direction, vibrations, grands boutons),
sons moteur par type de voiture, trafic, klaxon, sirène, chocs, pluie, musique (synthétisée par le jeu, `src/audio/musique.cpp`).

| ID | Domaine | Constat / opportunité | Proposition d'amélioration | Bénéfice joueur | Impact | Complexité | Priorité | Risques / points à vérifier |
|---|---|---|---|---|---|---|---|---|
| A01 | Marché | *Observé* : jeu **en français seulement** (aucune langue dans les réglages) | **Anglais** (puis espagnol, portugais, arabe…) : textes du jeu, fiche du store | Le jeu devient vendable dans le monde entier, pas seulement aux francophones | Élevé | Moyen (tous les textes passent par une table) | P1 | Polices (arabe = écriture de droite à gauche, plus tard) |
| A02 | Personnalisation | *Observé* : 16 couleurs unies, sans finition | **Peintures** : métallisé, nacré, mat, caméléon, 12 couleurs vives, couleur secondaire | Ce qu'on achète le plus dans un jeu de voitures | Élevé | Moyen | P1 | Déjà dans R11 / V6 |
| A03 | Personnalisation | *Observé* : aucun dessin | **Dessins (vinyles)** originaux : bandes, numéro, flammes, damier, camouflage | Voiture unique, envie de la montrer | Élevé | Moyen à Élevé (pose sur 17 carrosseries) | P2 | Aucun logo ni sponsor réel |
| A04 | Personnalisation | *Observé* : ni jantes, ni hauteur, ni vitres | **Jantes** (6 modèles × couleur), **vitres teintées**, **étriers de frein colorés** | Petits achats fréquents, voiture « à soi » | Moyen | Moyen (modèles 3D réels de jantes) | P2 | Règle « vrai modèle 3D » ; budget triangles |
| A05 | Progression | *Observé* : aucune amélioration de la voiture | **Préparation** en 3 niveaux : moteur, pneus, freins, nitro (payés en étoiles) | Sentir sa voiture progresser ; gros moteur de dépense d'étoiles | Élevé | Moyen | P2 | Conduite réglée sur les fiches réelles : rester dans ±10 % ; équilibrage des IA |
| A06 | Prise en main | *Observé* : aucun tutoriel | **Tutoriel de 20 s** à la 1ʳᵉ course (accélérer, tourner, freiner, nitro, frôler) | Le joueur comprend avant d'abandonner | Élevé | Faible | P1 | — |
| A07 | Audio | *Observé* : musique synthétisée par le jeu | **Vraies musiques** (électro / hip-hop / rock, licence CC0 ou CC BY, ou commandées), une par ambiance (menu, course, nuit) | Ambiance « street », souvenir du jeu | Moyen | Faible | P2 | Licence compatible avec la vente (pas de NC) |
| A08 | Social | *Observé* : aucun classement ni partage | **Classements** (Google Play Jeux, déjà prévu v1.1) + **photo de sa voiture à partager** depuis le garage | Publicité gratuite par les joueurs | Moyen | Faible à Moyen | P3 | Partage = rien envoyé par le jeu lui-même |
| A09 | Rétention | *Supposé* | **Notification** « ton cadeau du jour t'attend » (une par jour au plus, désactivable) | Rappel de revenir | Moyen | Faible | P3 | Permission Android 13+ ; pas de harcèlement |
| A10 | Fiche store | *Supposé* : pas de vidéo | **Vidéo de 20-30 s** + 6 captures en jeu (peintures, nuit, poursuite) | Plus de téléchargements | Élevé | Faible | P1 | Captures du téléphone, pas du banc PC |

**Top 3** : A01 anglais (multiplie le nombre de joueurs possibles), A02 peintures (ce qui se vend le mieux), A05 préparation (donne une raison de dépenser des étoiles).
**Ordre conseillé, à la suite de celui de l'utilisateur** : A06 tutoriel et A01 anglais avant la sortie ; A02 avec l'étape « couleurs gratuites » ; A03, A04, A05, A07 après le market ; A08, A09 en v1.1.

---

## R15 — 2026-11-12 — Évolution de la personnalisation : couleurs, skins, dessins, créativité (mode Créatif + Priorisation)

Demande de l'utilisateur : « maintenant évolutions couleur, skin, dessin, créativité ». Base observée : 16 couleurs unies (`src/game/sauvegarde.h:34`), une peinture par voiture, aucun dessin, aucune plaque.
Monnaie : étoiles (R13). Règles : vrai rendu de vraie peinture et de vrais adhésifs (DA-realisme.md), **aucun logo ni sponsor réel**, performance du téléphone.

### Les 4 niveaux, du plus simple au plus créatif
| Niveau | Contenu | Gratuit | Payant en étoiles | Complexité |
|---|---|---|---|---|
| 1. **Couleurs** | 16 actuelles + 12 vives (bleu électrique, vert acide, orange fluo, rose, jaune citron, violet néon…) + **couleur secondaire** (toit, rétros) | 8 couleurs | 50 ★ la couleur | Faible |
| 2. **Finitions** | brillant, **métallisé**, **nacré**, **mat**, **satiné**, **chromé**, **caméléon** (change selon l'angle), **candy** | brillant | 100-400 ★ (caméléon et chromé les plus chères) | Moyen (shader véhicule, E10) |
| 3. **Skins (livrées complètes)** | ensembles prêts à poser, dessinés pour chaque voiture : « Rallye », « Drift Tokyo », « Police rétro », « Néon nuit », « Camouflage urbain », « Damier », « Flammes », « Carbone » | 1 skin offerte par voiture débloquée | 300-800 ★ ; skins rares seulement dans le cadeau du jour ou les succès | Moyen à Élevé (une texture par voiture × skin) |
| 4. **Atelier créatif** | **éditeur d'autocollants** : choisir un autocollant (bande, numéro, flamme, étoile, éclair, tribal, lettres), le placer, tourner, agrandir, colorer, sur plusieurs **calques** (20 au plus) ; + texte libre (filtré) | 10 autocollants | packs d'autocollants 150-300 ★ | Élevé |

### Autour de la carrosserie
- **Jantes** : 6 modèles réels (branches, multi-rayons, pleines…) × couleur ; **étriers de frein** colorés ; **vitres teintées** (3 niveaux) ; **plaque** (style, couleur, néon, R-identité).
- **Néons sous la caisse** (la nuit) et **couleur des phares** (blanc, jaune) : forte envie, très « street ».
- **Fumée des pneus colorée** au dérapage.

### Créativité et partage
- **Mode photo** dans le garage : angles, fond (Corniche jour / nuit), filtre ; **partage** de l'image (le joueur partage lui-même, rien n'est envoyé par le jeu).
- **Mes créations** : 5 emplacements de sauvegarde de livrée par voiture.
- Plus tard (v1.2+) : **partager une livrée par un code** à un ami ; **concours de la semaine** (vote) : demande modération et serveur → pas en v1.

### Ordre conseillé
1. Couleurs + couleur secondaire (avec l'étape « couleurs gratuites »).
2. Finitions (métallisé, nacré, mat, caméléon).
3. Jantes, étriers, vitres, néons.
4. Skins : 4 skins d'abord, sur les 3 voitures gratuites + 3 voitures les plus vendues.
5. Atelier créatif (autocollants, calques).
6. Mode photo et partage.

### Risques
- **17 carrosseries différentes** : une skin = une texture par voiture → coût de production ; commencer par 6 voitures.
- **Pose réaliste** des autocollants (projection sur les formes, pas de bande plate qui dépasse : ANO-250) : le plus dur techniquement.
- **Mémoire** : textures de livrée compressées, une seule chargée à la fois en course.
- **Texte libre** : liste de mots interdits (comme le pseudo et la plaque).
- **Règle des marques** : aucun logo réel, aucun sponsor réel, aucune livrée copiée d'une vraie écurie.
