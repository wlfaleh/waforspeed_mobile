# WFS-58 — [MVP] Correction simulateur : scénario `mvp` piloté par l'état du jeu (SIM-MVP-2, 3, 4)
Épic : E7 · Priorité : 0.437 (MVP, avant WFS-50) · Dépend de : — (WFS-59 en parallèle pour la vitesse) · Taille : ≈ ½ journée

## Besoin (qui, quoi, pourquoi)
Le QA lead doit pouvoir croire les captures et le verdict technique du simulateur. Aujourd'hui (rapport
`qa/rapports/simulateur-MVP.md`) le scénario `mvp` de WFS-56 avance à l'horloge (`sleep`) : les captures 01 à 05 ne
montrent pas l'étape nommée (SIM-MVP-2), le script conclut « OK » alors que rien n'a eu lieu après « Course solo »
(SIM-MVP-3), et la 1re capture montre les barres système d'Android (SIM-MVP-4).

## Règles de gestion
- RG-01 : chaque étape du scénario `mvp` **attend un événement du logcat**, pas une durée fixe : fonction
  `attendre "<motif>" <délai max s>` (lecture de `adb logcat` filtré, bornée par `timeout`). Motifs : `QA pret`,
  `Écran : … → menu`, `menu : « Course solo »`, `Écran : menu → course`, lignes `course : …` (décompte, GO, fin) déjà écrites
  par le jeu, `Écran : … → fin` / `fin → course` / `→ menu`. Lire les motifs exacts dans le code (`grep WFS_LOG src/`), ne
  pas les deviner. **Aucun changement de `src/`** : si un événement n'est pas journalisé, le noter au Journal pour le PO.
- RG-02 : capture **après** l'événement + présentation d'images : attendre 2 nouvelles lignes `QA ips=` (ou au moins
  2 s) après l'événement avant `screencap`. Deux captures consécutives identiques (même md5) = étape notée `FIGÉE` dans
  `deroulement.txt`.
- RG-03 : délai max par étape **commenté** (ex. menu 60 s, décompte 60 s, fin 240 s). Étape non atteinte : ligne
  `ÉCHEC étape <nom> (délai <n> s)`, capture prise quand même (preuve), scénario arrêté, **`RÉSULTAT TECHNIQUE : ÉCHEC`** et
  code de sortie ≠ 0. Plus jamais `OK` si une étape manque.
- RG-04 (SIM-MVP-4) : `01-chargement` prise **après** la 1re image du jeu (1re ligne `QA ips=` ou
  la ligne de début du chargement du jeu), jamais sur l'écran de démarrage Android ; barres système absentes.
- RG-05 : levier E7 pour la vitesse : l'émulateur tourne avec un écran réduit (`adb shell wm size 1200x540` en paysage,
  ou `wm size 540x1200` en portrait selon l'orientation de l'appareil, `wm density` adaptée), remis à zéro en fin de script.
  Les coordonnées d'appui restent calculées depuis `W`/`H` (lus **après** `wm size`). Noter la taille dans `deroulement.txt`.
- RG-06 : lancer l'appli avec l'extra `--ez qa_simu true` (`am start … --ez qa_simu true`) : sans effet tant que WFS-59
  n'est pas fusionné, utilisé par WFS-59 ensuite. Le scénario `conduite` reste inchangé (mêmes 7 captures).
- RG-07 : durée du job ≤ 45 min (limite de `simulateur.yml`) : somme des délais max ≤ 30 min.

## À réutiliser
- Rien du jeu web (`wlfaleh/waaps`, `waforspeed/`) : outillage CI Android sans équivalent navigateur.
- Natif : `tools/qa/emulateur.sh` (fonctions `log`, `shot`, `alive`, calcul `W`/`H`, coordonnées de WFS-56) et
  `.github/workflows/simulateur.yml` : **compléter, ne pas réécrire**. Lignes du jeu à attendre : `WFS_LOG` de
  `src/game/game.cpp` (`Écran : …`), `src/game/course_system.cpp` (`course : …`), `src/ui/ecrans/menu.cpp` (`menu : …`).

## Critères d'acceptation (Release `simulateur-WFS-58` du workflow lancé sur `feature/waforspeed-58`, `scenario=mvp`)
- [ ] `deroulement.txt` : chaque étape avec l'événement attendu, l'heure, la taille d'écran ; aucune attente fixe > 2 s hors marge de RG-02
- [ ] `01-chargement` montre la barre du jeu, sans barres système ; `02-menu` montre le menu (WFS-37)
- [ ] tant que WFS-59 n'est pas fusionné : le scénario **échoue proprement** (`ÉCHEC étape …`, `RÉSULTAT TECHNIQUE : ÉCHEC`) au lieu d'un faux OK — ou passe s'il est assez rapide ; jamais `OK` avec une étape manquante
- [ ] `scenario=conduite` : mêmes 7 captures qu'avant

## Hors périmètre
- La vitesse du jeu sur GPU logiciel (WFS-59), tout changement de `src/` ou `android/`, le rapport `simulateur-MVP.md` (WFS-50).

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| P4 Basse | WFS-58 | [MVP] Correction simulateur : scénario `mvp` piloté par l'état du jeu (SIM-MVP-2, 3, 4) | E7 | ✅ Fusionné (ordre de l'utilisateur, QA de nuit à faire) | Claude (session trafic), 2026-11-03 | — | `feature/waforspeed-58` | #32 |  |

Critères et notes de suivi :

**WFS-58 — [MVP] Correction simulateur : scénario piloté par l'état du jeu (SIM-MVP-2, 3, 4)** · E7 · priorité P4 Basse · ✅ Fusionné le 2026-11-03 sur ordre de l'utilisateur (QA de nuit : `qa/Check.md`) — Claude (session trafic), 2026-11-03 (règles : `regles-de-gestion/WFS-58.md`)
- [ ] scénario `mvp` : chaque étape attend sa ligne du logcat (délai max), capture après présentation d'images, `ÉCHEC` au lieu d'un faux `OK` si une étape manque
- [ ] `01-chargement` = barre du jeu sans barres système, `02-menu` = menu ; écran d'émulateur réduit (`wm size`) ; `scenario=conduite` inchangé
