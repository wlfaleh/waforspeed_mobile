 instruction dans ce fichier je veux laisse mes remarques et mes ordes 

il faut me laisse un commentaire après chaque tiret entre parenthèses 

exemple :

- j'ai merger la pr 87 il faut mettre a jour le tache et le journal 
( agent 3 session xyxyxt : c'est fait )

demande utilisateur :

- on a des tâches avec des statuts en cours alors que l'agent na pas encore créé la branche 
pour ça il faut remettre a jour les status dans les fichiers taches.md et journal.md  pour ne pas bloquer le dev
( Copilot session 0e5c192d : c'est fait — vérifié avec git ls-remote (22:06 UTC) : WFS-91, 93, 95, 97, 99, 100 et 101 étaient « 🔧 En cours » sans aucune branche ; le PO les a remis en « ⏳ Prêt », responsable libéré, dans TACHES.md (tableau + détail) avec une entrée au journal (22:04:18, commit 1d53d00). WFS-89 et WFS-96 ont leur branche et leur PR : ils restent en cours. WFS-98 est fini et fusionné (PR #80). Le PO m'a donné WFS-99 et j'ai créé et poussé sa branche tout de suite, pour ne plus avoir de ticket en cours sans branche. )


- on a des PRs qui traîne depuis des heurs demander au po pour préparer une ticket pour les regarder et les merger
( Claude session trafic : c'est fait — le PO a créé WFS-103 « Revue et fusion des PR en attente » (P1, règles dans regles-de-gestion/WFS-103.md, commit e7c1c9f), attribué à moi. Ordre : #77 (WFS-89) puis #79 (WFS-96) ; #76 (WFS-88) laissée telle quelle à cause du GEL iOS. Nouvelle règle RG-10 : aucune PR ouverte plus de 2 h sans décision. Je m'y mets tout de suite. — Mise à jour 22:20 UTC : #77 (WFS-89) fusionnée (fb50d09) et #79 (WFS-96) fusionnée (6863763), toutes deux resynchronisées avec main, compilées, tests CPU OK, CI de branche verte, captures regardées ; TACHES.md, qa/Check.md, anno.md et journal à jour. #76 (WFS-88) laissée ouverte (GEL iOS). Plus aucune PR en attente hors iOS. )
( Copilot session 0e5c192d : vu et complété — j'avais aussi demandé l'analyse au PO ; il n'a pas créé de second ticket (doublon de WFS-103), il a vérifié #77 et #79 de son côté (CI verte, conflit build.yml de #77 bien résolu, tests CPU OK), complété regles-de-gestion/WFS-103.md (RG-03, RG-04 : les tests CPU sont la ligne « Tests CPU » de build.yml, ctest ne lance rien) et consigné au journal les accroches de WFS-96 dans garage.*, sfx.* et manette_jeu.cpp (22:20:27 et 22:22:58). Je les garde dans WFS-99, que je termine. #76 (iOS) reste ouverte : gel iOS. )

- Tas deja pr non merge
( Claude session trafic : la seule PR ouverte est #76 (WFS-88, iOS, prête : build iOS et CI Android verts). Décision de l'utilisateur (22:29 UTC) : la laisser ouverte à cause du GEL iOS ; consigné dans TACHES.md et le journal. Aucune autre PR ouverte. )



- même pour ios il faut la regarder et la merger
( PO : c'est fait côté planification — ordre ajouté à WFS-103 (RG-12) : l'agent de la revue regarde et fusionne la PR #76 (WFS-88, iOS) ; le gel iOS reste pour tout le reste. ) 


- j'ai tester le jeux ya un problème lorsque je clique sur les fleche ou j'essaye de tourner a gauche ou a droit ya une annimation bizzare et grave commme si ya du feux blanc dit au po et qa de voir le sujet
( PO : c'est créé — WFS-107 (E4, P1) : reproduire, nommer la cause (flash, vignette, étincelles, halo de roues…) et supprimer l'effet blanc au braquage ; captures « braquage-gauche » et « braquage-droite » pour le QA, qui les regarde à la prochaine recette de nuit. )
( Copilot session dc0cdd3f : cause trouvée — reproduit au banc PC en braquant vers 120 km/h : ce n'est ni un flash ni la vignette, c'est la **fumée des pneus** (crissement au braquage) qui sort ≈ 4,6 fois trop claire (mauvaise conversion des couleurs vers l'éclairage HDR) : les bouffées grises deviennent des taches blanches lumineuses, avec un halo la nuit. Anomalie ANO-46 (bloquante) ; la correction est **WFS-107, que je fais maintenant** (fumée grise discrète comme le jeu web, bouffées rondes, aucune fumée à petite vitesse ni avec « réduire les mouvements », captures braquage-gauche / braquage-droite de jour et de nuit). Avant, WFS-101 (fusionné, PR #84) a rendu visibles les étincelles des chocs et les traits de la nitro sans toucher à cet effet. )
( Copilot session dc0cdd3f, 2026-11-05 : **corrigé et fusionné** — WFS-107, PR #87. La fumée des pneus a maintenant le gris du jeu web (≈ 4 fois moins claire), elle n'éclaire plus et ne brille plus la nuit. Les bouffées sont rondes et ont la taille du jeu web. Il n'y a **aucune fumée sous 72 km/h**, ni avec « réduire les mouvements ». Au banc, dans les 12 captures `braquage-gauche` / `braquage-droite` (30, 120, 250 km/h, jour et nuit, vue Proche), il n'y a plus de tache ni de voile blanc. À essayer sur ton téléphone avec l'APK de `main` : tourne aux flèches vers 120 km/h, tu dois voir une fumée grise discrète derrière les roues, plus de « feu blanc ». Si quelque chose de blanc reste au braquage, dis-moi de jour ou de nuit et à quelle vitesse. La QA regarde les captures à la recette de nuit. )


- les button accélération frein turbo et flech sont trop petit  voir la possibilité de faire mieux
( PO : c'est créé — WFS-108 (E3, P1) : boutons ≥ 48 dp (GAZ / FREIN plus grands), taille réglable. Regroupé avec le frein / marche arrière et le gyroscope dans un seul gros ticket. )  


- le marche arrière et le frein a revoir
( PO : c'est dans WFS-108 (RG-02) : frein maintenu à l'arrêt = marche arrière sans attendre ; comportement vérifié par test CPU et corrigé. ) 

- feux de freinage est trop artificiel et loin de la réalité
( PO : c'est créé — WFS-109 (E4, P1) : 2e passage des feux de frein (zone du feu du modèle qui s'allume, pas de gros halo, transition douce). Il démarre dès la fusion de WFS-101 (même fichier du trafic), en parallèle de WFS-107. Constat du PO : aujourd'hui, de jour, le feu de la voiture ne s'allume jamais au freinage (seul un grand halo rouge plaqué sur le trafic) ; WFS-109 le corrige. ) 


- la camera pour le mode proche est catastrophique je vois pas la rue devant la voiture il faut zoomer un peux se sort sue je vois lavant de voiture avec clenaison de la camera
( PO : c'est ajouté à WFS-105 (RG-06) : vue Proche reculée / zoomée, caméra inclinée vers le bas (≈ 20-25°), la rue devant la voiture visible ; une seule CI avec la poursuite à 40°. )
( Copilot session 1ef8d4d6 : c'est fait — WFS-105 fusionné (PR #85). **Proche** : caméra reculée à 6 m et montée à 2,67 m, inclinée vers le bas (24°) : on voit l'arrière de la voiture et la rue devant, par-dessus le toit. **Poursuite** : 9 m derrière et 6,3 m de haut, soit 35° à l'arrêt et ≈ 28° en course. J'ai écarté 40° parce que l'horizon sortait de l'image et qu'on ne voyait plus loin devant. Au banc, avec 6 voitures devant, les 6 sont visibles. À essayer sur ton téléphone avec l'APK de `main` : si tu veux plus haut ou plus bas, c'est une ligne à changer (`POURSUITE_LOIN` / `POURSUITE_PROCHE` dans `src/game/camera_vue.h`). )

- voir la possibilité dajouter aussi le mode gyroscope acec les fleches ou le glissage
( PO : c'est dans WFS-108 (RG-03) : modes flèches, glissement et gyroscope (zone morte et sensibilité réglables), au choix dans les réglages. )

- Créer une tâche mode course entre 5 voitures (1 joueur + 4 ordinateurs, voitures aléatoires, le joueur choisit sa voiture et les km, même chose pour le duel : seulement les voitures qui participent)
( PO : c'est créé — WFS-110 (E3, P2) : Course à 5 voitures, 4 ordinateurs tirés au hasard, choix de la voiture et de 2 / 4 / 6 / 10 km, aucun trafic ; le duel n'a plus de trafic non plus. )

- Mode poursuite police : score 100, 10 km, garder au moins 1, beaucoup de trafic, embouteillages, surprises
( PO : c'est créé — WFS-111 (E3, P2) : police qui poursuit (renfort tous les 2 km), score qui baisse aux chocs et à la proximité, trafic 2 à 3 fois plus dense, embouteillages, 6 types de surprises. Les valeurs de points sont proposées par le PO et ajustables. Il démarre après WFS-110 (menu des modes). )

- Garder le mode checkpoint actuel avec 60 secondes
( PO : c'est dans WFS-110 (RG-08) : temps de départ 60 s, checkpoints inchangés. )

- Revoir le duel en ligne : pas de Wi-Fi, jouer avec des amis, faire mieux que la version web
( PO : c'est créé — WFS-112 (E3, P2). Solution : un petit serveur de relais (marche en 4G et partout), code à 4 caractères + lien d'invitation partagé par WhatsApp / SMS qui ouvre le jeu dans le salon, reconnexion automatique, ping, revanche. WFS-92 (bibliothèques tierces) est abandonné. Une décision pour toi : autoriser un compte gratuit Cloudflare Workers pour héberger le relais (recommandé, ~5 min). )

- Bug showroom : les flèches sautent des voitures
( PO : c'est créé — WFS-113 (E5, P1 urgent) : une flèche = exactement une voiture, cause documentée, 40 appuis sans saut. )

- Cloudflare ok ; checkpoint de départ 60 sec et 30 sec de plus à chaque checkpoint ; ok pour les propositions de la poursuite police
( PO : c'est pris en compte — WFS-110 RG-08 : départ 60 s, +30 s à chaque checkpoint ; WFS-111 : valeurs validées ; WFS-112 : relais sur Cloudflare Workers (compte gratuit) : le développeur livre le code, le script de déploiement et un mode d'emploi en 5 lignes ; il te demandera de créer le compte et de déployer ou de lui donner un jeton d'API. )

- Quand je clique sur un mode et que je clique sur continuer, il me montre la page menu
( PO : c'est créé — WFS-115 (E8, P1 urgent) : reproduire sur tous les chemins du menu, nommer la cause, faire avancer « Continuer » vers l'écran suivant. Je précise une limite : dans le code de main, l'écran du terrain a « Rouler » et pas « Continuer » ; si tu vois « Continuer », tu testes peut-être une ancienne version (WFS-106 : publication de l'APK de main en cours). )

