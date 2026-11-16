# Épic E23 — Interface R19

Créée le 2026-11-15 par le PO, sur demande de l'utilisateur (« Ok crée les 5 tâches », transmise par la session smart-brainstorming).
Source : section « ## R19 » de `recommandations-smart-brain.md` (tableau des écrans 01 à 33) et images de `references/maquettes-R19/` (01 à 33, `00-vue-ensemble.jpg`, `19-33-ecrans-complementaires.jpg`, `icone-pistolet-utilisateur.png`).

## Découpage (décision de l'utilisateur : exactement 5 tickets, par thème ; 17 puis 6 refusés)
| Ticket | Thème | Dépend de | Agent |
|---|---|---|---|
| WFS-548 | Fonds et panneaux (verre dépoli, fond flouté, sur-titres gris, dégradé, couleurs d'état) | — | `@dev` |
| WFS-549 | Boutons (Start/Stop, autocollant, pilule chrome, prix, sélecteurs de verre, onglets) | — | `@dev` |
| WFS-550 | Indicateurs et jauges (cadrans fins, jauges, anneaux, 17 segments, compteur d'étoiles, bandeaux en course) | WFS-548, 549 | `@dev` |
| WFS-551 | Icônes et personnalisation (gouttes, pastilles, pistolet, cadenas-jauge, flèches, HUD, cartes grisées) | WFS-548, 549 | `@dev` |
| WFS-552 | Menu en 3D : SF-X caméléon dans le parking souterrain, caméra rapprochée | — | `@dev` (= `dev-artiste-3d`) + avis `@3d` |

## Ordre (décision de l'utilisateur)
- 548, 549 et 552 **prêts tout de suite, en parallèle** (une nouvelle session de développeur chacun) ; 550 et 551 après la fusion de 548 **et** 549.
- 548 et 549 touchent les mêmes écrans (exception voulue par l'utilisateur au garde-fou « un ticket par fichier de panneau du garage ») : **le 2e fusionné se synchronise avec `main` et refait ses captures**.
- 550 et 551 se partagent les fichiers : 550 garde `garage_fiche.*`, `garage_cadre.*`, `toasts.*`, les écrans du cadeau, du prêt et du compteur d'étoiles ; 551 garde `garage_couleur.*`, `garage_skin.*`, `garage_finition.*`, `garage_achat*`, `hud.*`, `terrain.*` / `paquets*`. Un fichier commun touché par les deux : le 2e fusionné se synchronise et refait ses captures.
- Cible : tout fusionné en une demi-journée environ.

## Règles communes
- 548 à 551 = **interface 2D** : skill `design-street-racing`, exemptée de la règle des formes géométriques (menus, HUD, boutons). 552 = **monde 3D** : règle des formes géométriques interdites, réalisme 99 % (`regles-de-gestion/DA-realisme.md`), skill `artiste-3d`, avis « VALIDÉ » de l'agent 3D dans la PR avant la fusion (comme E15).
- **Taille des boutons inchangée** (ordre du 2026-11-06), sauf **Menu et Caméra du HUD**, agrandis (validé par l'utilisateur dans la R19, suite de l'exception WFS-429).
- **Chaque écran respecte sa maquette R19 à 100 %** (disposition, éléments, proportions, couleurs, textes, icônes) ; règle de l'utilisateur, 2026-11-15 23:50:31 UTC. Les options du jeu restent. Les textes restent en français et en anglais (E20 : `ui::tr`). PR en cours (550, 551) : refusée si ce n'est pas 100 % ; travail déjà fusionné (548, 549, 552) : vérifié et corrigé par **WFS-557**.
- Un ticket par session, branche `feature/waforspeed-XX`, captures avant / après (`tools/qa/captures_pr.sh`, au moins 3), PR, relecture et fusion en squash par le lead dev, puis la session s'arrête. Gel iOS.
- Tickets nés d'une demande de l'utilisateur sur ses écrans : après la fusion → **🟡 À valider (utilisateur)** sur l'APK de `main`.

## Revue du lead dev : maquettes et code (règle de l’utilisateur, 2026-11-15 23:14 UTC)
Pour **WFS-550** et **WFS-551**, le lead dev compare chaque maquette citée par les RG **au code et aux captures** (une ligne par maquette dans la revue). Le développeur cite dans sa PR, pour chaque maquette, le fichier et la fonction qui la dessinent. Sans cette comparaison, pas de fusion.
