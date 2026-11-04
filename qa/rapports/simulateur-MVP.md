# QA simulateur MVP (WFS-50) : parcours complet sur Android

## Tour 2 — recette de nuit (2026-11-04 18:34:47 UTC, qa-lead)

**Verdict : VALIDÉ** (simulateur Android). Les 3 bloquantes et la mineure du 1er passage ne se voient plus.

- Release regardée : `simulateur-MVP` (publiée 2026-11-04T00:53:14Z, commit `2ac6ed5`, après la PR #47), `qa-01` à `qa-10`, `qa-deroulement.txt`, `qa-logcat.txt`.
- `qa-deroulement.txt` : `fenêtre du jeu 2400x1080`, `gaz tenu 2016,993`, **`RÉSULTAT TECHNIQUE : OK`**.
- `qa-05-go` : course lancée, 108 km/h, HUD, piétons ; `qa-06-course-hud` : 135 km/h ; `qa-07-fin` : score 4 170, 0,83 km, Rejouer / Menu.
- `qa-logcat.txt` : `mode GPU logiciel : actif (dt max 0,5 s, rendu 25 %, sans ombres)`, `QA ips=1.0` puis 0,6 (GPU logiciel).
- Le design des écrans (commandes tactiles, échelle de l'interface) est jugé sur le banc PC : ANO-19 et ANO-31.

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| — | — | — | Aucune anomalie. |


**Verdict : REFUSÉ** (simulateur Android), 1er passage, 3 anomalies bloquantes et 1 mineure. Le MVP n'est **pas** livrable
en l'état : le parcours RG-02 n'est pas visible sur les captures.

- Run regardé : `simulateur.yml` sur `main` (`b8b89fc`), `epic=MVP`, `scenario=mvp`, run 41702978455 (réussi, ≈ 7 min).
- Release regardée : **`simulateur-MVP`** (publiée 2026-11-03T19:30:57Z, cible `b8b89fc`), 14 fichiers : `qa-01-chargement`
  … `qa-10-retour-menu`, `qa-deroulement.txt`, `qa-ecran.txt`, `qa-logcat.txt`, `WaForSpeed.apk`.
  L'ancienne Release `simulateur-WFS-56-mvp` (16:09Z, avant WFS-38 et WFS-57) n'a **pas** servi de base au verdict.
- Règles : `regles-de-gestion/WFS-50.md` (RG-01 à RG-05). Émulateur : écran 1080x2400 (paysage 2400x1080),
  GPU **SwiftShader** (Vulkan 1.3, logiciel), qualité réduite (rendu à 50 %, sans MSAA).

## Ce que dit le journal du jeu (`qa-logcat.txt`)

| Heure | Événement |
|---|---|
| 19:28:03 | démarrage, Vulkan SwiftShader, swapchain 1080x2400 rotation 2, cibles 2400x1080 |
| 19:28:19 | chargement total 15,20 s, « QA pret : moteur initialise » |
| 19:28:21 | `Écran : course → menu` (le menu est bien atteint) |
| 19:28:30 | `menu : « Course solo »` puis `Écran : menu → course` : **l'appui tactile (444,541) a bien été reçu** |
| 19:28:30 → 19:30:43 | 21 lignes `QA ips=0.3 vitesse=0 km/h distance=0 m` : **le jeu tourne à 0,3 image/s**, la voiture ne bouge jamais |
| — | aucune ligne `Écran : course → fin`, `fin → course` ni `→ menu` après le départ : ni fin, ni « Rejouer », ni « Menu » |

Aucun plantage, aucun ANR, aucune ligne `E`/`F`, aucune erreur Vulkan (`VK_ERROR`, validation) : RG-03 « logcat » est OK.

## Captures

| Capture | Attendu (RG-02) | Constat |
|---|---|---|
| `qa-01-chargement` | écran de chargement avec barre | écran de démarrage **Android** (logo W sur noir), barre d'état (heure 7:27, batterie) et poignée de navigation visibles ; pas de barre de chargement du jeu |
| `qa-02-menu` | menu principal | écran de chargement du jeu, « Construction de la ville 100 % », barre pleine. **Pas de menu** (le menu arrive à 19:28:21 mais son image n'est pas encore présentée à 0,3 ips) |
| `qa-03-appui-course-solo` | réaction à l'appui | **écran entièrement noir** |
| `qa-04-decompte` | décompte 3-2-1 | **noir**, identique au pixel à `qa-03` (même md5) |
| `qa-05-go` | « GO » | **identique au pixel à `qa-02`** (écran de chargement 100 %) : image périmée |
| `qa-06-course-hud` | course, trafic, HUD lisible | course affichée mais **décompte « 3 »**, TEMPS 30,0, 0 km/h, score 0, **aucun véhicule de trafic** ; HUD lisible (temps, checkpoint, score, compteur, nitro, commandes) ; Golf bleue nette, image non déformée |
| `qa-07-fin` | écran de fin | même image : **décompte « 3 »**, pas d'écran de fin |
| `qa-08-rejouer-decompte` | nouveau décompte | toujours le **premier** décompte « 3 » (l'appui « Rejouer » tombe sur la course, pas d'écran de fin) |
| `qa-09-fin-2` | 2e fin | décompte « 2 », TEMPS 30,0 |
| `qa-10-retour-menu` | menu principal | décompte « 2 » : **pas de retour au menu** |

En ≈ 2 min 10 de course, le décompte n'est passé que de « 3 » à « 2 » : le temps du jeu avance d'environ 1 s par
minute. Les attentes fixes du scénario (`sleep` de `tools/qa/emulateur.sh`) ne correspondent donc à aucun état du jeu.

## Critères d'acceptation et règles

| Critère / règle | Constat | Résultat |
|---|---|---|
| RG-01 : `simulateur.yml` sur `main`, `epic=MVP`, APK de `main` après fusion du MVP | run 41702978455 sur `b8b89fc` | OK |
| RG-02 : une capture par étape (chargement, menu, Course solo, décompte, course + trafic + HUD, fin, Rejouer, Menu) | seules la course (décompte figé) et, en partie, le chargement (100 %) se voient ; menu, GO, conduite, trafic, fin, Rejouer, retour menu absents | **NON** |
| RG-03 : paysage plein écran sans barres système | paysage 2400x1080 OK ; plein écran OK sur les captures du jeu ; barres système visibles sur l'écran de démarrage Android (`qa-01`) | OK (voir mineure) |
| RG-03 : aucun plantage ni ANR | aucun | OK |
| RG-03 : logcat sans erreur Vulkan | aucune | OK |
| Critère : captures de chaque étape, rapport VALIDÉ | non | **NON** |

## Anomalies

| ANO-XX | Gravité | Capture | Ce qui ne va pas |
|---|---|---|---|
| SIM-MVP-1 | **Bloquante** | `qa-06` → `qa-10`, `qa-logcat.txt` | Sur l'émulateur (SwiftShader) la course tourne à **0,3 image/s** : décompte figé sur « 3 » puis « 2 », chrono 30,0, 0 km/h, aucun trafic. GO, conduite, fin, « Rejouer » et « Menu » ne sont jamais atteints : RG-02 n'est pas vérifiable. À corriger soit par un rendu allégé quand le GPU est logiciel (mode QA), soit en faisant avancer le scénario sur l'état du jeu plutôt que sur l'horloge. |
| SIM-MVP-2 | **Bloquante** | `qa-01` à `qa-05` | Les cinq premières captures ne montrent pas l'étape nommée : `qa-01` = écran de démarrage Android (pas de barre), `qa-02` = chargement 100 % au lieu du menu, `qa-03`/`qa-04` = noir (identiques), `qa-05` = copie au pixel de `qa-02`. Le menu (WFS-37) n'apparaît sur **aucune** capture. |
| SIM-MVP-3 | **Bloquante** | `qa-deroulement.txt` | Le scénario `mvp` (WFS-56) conclut « RÉSULTAT TECHNIQUE : OK » alors qu'aucune étape après « Course solo » n'a eu lieu (aucun `Écran : course → fin` / `→ menu` au journal) : faux positif. Il doit attendre et contrôler les changements d'écran du journal (`Écran : …`) avant chaque appui et chaque capture, et échouer s'ils manquent. |
| SIM-MVP-4 | Mineure | `qa-01` | Barre d'état et poignée de navigation visibles sur l'écran de démarrage Android, avant que le jeu passe en plein écran (le jeu lui-même est bien en plein écran). |

## Vérification à l'écran de WFS-37 et de WFS-56 (QA due après fusion)

Comme demandé par le PO, ce parcours sert aussi de vérification à l'écran après fusion de :

- **WFS-37 (menu principal)** : **non vérifiée**. Le journal prouve que le menu est atteint et qu'il reçoit l'appui
  « Course solo » (coordonnées 444,541 justes), mais **aucune capture ne montre le menu**, et le retour au menu après la fin
  n'a pas eu lieu. La QA à l'écran de WFS-37 reste due.
- **WFS-56 (scénario simulateur `mvp`)** : **en défaut** (SIM-MVP-2, SIM-MVP-3). Les appuis `adb shell input tap` sont bien
  envoyés et le premier est reçu, mais les attentes fixes ne suivent pas le jeu et le résultat « OK » est faux.

## Suite

- Verdict **REFUSÉ** : pas de « Livraison MVP » (RG-05). Selon RG-04, le PO crée un ticket de correction par défaut bloquant
  (SIM-MVP-1 à 3, zone E7 `tools/qa/**` pour 2 et 3 ; SIM-MVP-1 peut toucher le rendu si l'on choisit la voie « rendu allégé »).
- La mineure SIM-MVP-4 est jointe au même tour de correction ; elle n'ira au tableau `anno.md` que si elle reste après deux tours.
- Après correction : relancer `simulateur.yml` sur `main` (`epic=MVP`, `scenario=mvp`) et rappeler `qa-lead` sur la nouvelle
  Release `simulateur-MVP`.
