# WFS-56 — [MVP] Simulateur : scénario « parcours MVP » par appuis (étape 1 de WFS-50)
Épic : E7 · Priorité : 0.441 (MVP du 2026-11-03, juste avant WFS-50) · Dépend de : WFS-31 ✅ (WFS-38 pour l'essai complet, voir RG-07) · Taille : ≈ 2 h

## Besoin (qui, quoi, pourquoi)
WFS-50 doit jouer tout le MVP sur l'émulateur comme un joueur. Aujourd'hui `tools/qa/emulateur.sh` ne sait que conduire
(accélérer, nitro, braquer, freiner) dès le lancement : il n'appuie sur aucun bouton du menu ni de l'écran de fin.
Préparer ce scénario **maintenant**, pendant que WFS-38 (PR #27) est en revue, pour que WFS-50 se réduise à « lancer le
workflow et lire les captures » dès la fusion de WFS-38.

## Règles de gestion
- RG-01 : `simulateur.yml` reçoit un 2e paramètre `scenario` (`conduite` par défaut = comportement actuel inchangé, `mvp`)
  transmis à `emulateur.sh` (3e argument). Le paramètre `epic` et la Release `simulateur-<epic>` restent tels quels.
- RG-02 : scénario `mvp`, une capture nommée par étape, dans cet ordre :
  `01-chargement` (≈ 1-3 s après `am start`, barre visible), `02-menu` (après la fin du chargement), `03-appui-course-solo`,
  `04-decompte` (≈ 0,5-1 s après l'appui), `05-go`, `06-course-hud` (accélérateur maintenu 6 s, trafic et HUD visibles),
  `07-fin` (temps écoulé), `08-rejouer-decompte` (après appui « Rejouer »), `09-fin-2` (2e fin de temps),
  `10-retour-menu` (après appui « Menu »).
- RG-03 : appuis par `adb shell input tap x y`, coordonnées en **pourcentage de l'écran paysage** (`W`, `H` déjà calculés par
  le script) et **calculées depuis la mise en page du code**, pas au jugé :
  - « Course solo » : 1re ligne du menu, `src/ui/ecrans/menu.cpp` (`itemH = 70·k`, zone `g_zones[0]`, bloc centré verticalement) ;
  - « Rejouer » / « Menu » : `drawOver` de `src/game/course_system.cpp` (branche `feature/waforspeed-38`, PR #27) :
    `btn_[0..4]`, bouton de `40·u × 10·u` à `py + 60·u`, « Menu » à `btn_[4]`, largeur `0,7 × 40·u`.
  Viser le **centre** de chaque bouton ; écrire en commentaire la formule et la valeur obtenue pour 2400x1080 (Pixel 6).
- RG-04 : attentes fixes et commentées : chrono initial **30 s** (`src/game/course.h`, `timeLeft = 30`, comme `js/` du jeu web),
  décompte ≈ 3 s → attendre **36 s** après « GO » sans checkpoint (ne pas accélérer après `06-course-hud`, ou freiner, pour ne
  pas gagner de temps) ; une seconde de marge avant chaque appui ; durée totale du scénario ≤ 3 min.
- RG-05 : avant chaque appui, vérifier que l'appli tourne (`alive`) et le noter dans `deroulement.txt` (`appui <nom> x,y`).
- RG-06 : verdict technique inchangé (`QA pret`, aucun `Fatal signal|FATAL EXCEPTION|SIGSEGV|SIGABRT`, appli vivante) et en plus
  `logcat.txt` sans ligne `VK_ERROR` ; le scénario `conduite` donne exactement les mêmes captures qu'avant.
- RG-07 : **aucun changement du jeu** (ni `src/`, ni `android/`) : zone `.github/**` et `tools/qa/**` seulement. Tant que
  WFS-38 n'est pas fusionné, l'APK de `main` ne réagit pas aux appuis : l'essai de ce ticket se fait en lançant le workflow
  sur `feature/waforspeed-56` ; les étapes 01-02 doivent déjà être bonnes, les autres montrent le menu (attendu). Si WFS-38
  est fusionné avant la fin du ticket : se synchroniser avec `main` et relancer, tout le parcours doit passer.

## À réutiliser
- Rien du jeu web (`wlfaleh/waaps`, `waforspeed/`) : c'est de l'outillage CI Android, sans équivalent dans le jeu navigateur.
- Natif : `tools/qa/emulateur.sh` (fonctions `log`, `shot`, `alive`, calcul `W`/`H`, `adb` borné par `timeout`) et
  `.github/workflows/simulateur.yml` (WFS-31) : **les compléter, ne pas les réécrire**.
- Mises en page à lire pour les coordonnées : `src/ui/ecrans/menu.cpp`, `src/game/course_system.cpp` (`drawOver`).

## Critères d'acceptation (vérifiables sur les captures de la Release du simulateur)
- [ ] workflow lancé sur `feature/waforspeed-56` avec `scenario=mvp` : Release avec les 10 captures nommées de RG-02,
      `01-chargement` montre la barre de chargement, `02-menu` le menu principal, paysage plein écran ;
      `deroulement.txt` liste chaque appui avec ses coordonnées et `RÉSULTAT TECHNIQUE : OK`
- [ ] lancé avec `scenario=conduite` (défaut) : mêmes 7 captures qu'avant (`01-demarrage` … `07-fin`)
- [ ] si WFS-38 est fusionné : `03` à `10` montrent le parcours (décompte, course + HUD, fin, Rejouer, retour au menu)

## Hors périmètre
- Le rapport `qa/rapports/simulateur-MVP.md` et la « Livraison MVP » (WFS-50).
- Tout changement du jeu, du manifeste ou de la construction de l'APK.

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| 0.441 | WFS-56 | [MVP] Simulateur : scénario « parcours MVP » par appuis (étape 1 de WFS-50) | E7 | ✅ Terminé (fusionné par l'utilisateur, QA après fusion) | Claude (session 01UY1Vv1), 2026-11-03 | WFS-31 ✅ (essai complet après WFS-38) | `feature/waforspeed-56` | PR #28 (fusionnée, 94087c7) | ⏳ QA après fusion |

Critères et notes de suivi :

**WFS-56 — [MVP] Simulateur : scénario « parcours MVP » par appuis** · E7 · priorité 0.441 · ✅ Terminé (PR #28 fusionnée par l'utilisateur, QA après fusion) — Claude (session 01UY1Vv1), 2026-11-03 (règles : `regles-de-gestion/WFS-56.md`)
- [ ] `simulateur.yml` avec `scenario=mvp` sur `feature/waforspeed-56` : 10 captures nommées (RG-02), `01-chargement` et `02-menu` corrects, appuis et coordonnées dans `deroulement.txt`, `RÉSULTAT TECHNIQUE : OK`
- [ ] `scenario=conduite` (défaut) : mêmes 7 captures qu'avant ; aucun changement de `src/` ni `android/`
