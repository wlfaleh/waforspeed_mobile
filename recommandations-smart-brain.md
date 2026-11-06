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