# WFS-35 — Écran de chargement de l'application
Épic : E8 (Écrans et menus) · Priorité : 0.1 · Dépend de : WFS-26 (✅)

## Besoin (qui, quoi, pourquoi)
Au lancement, le joueur voit aujourd'hui un **écran noir** pendant que `Game::init` construit la route, charge la voiture
(textures décodées) et crée les systèmes (ville, trafic, sons, texte). Il doit voir tout de suite le **logo WaForSpeed** et
une **barre de progression** qui avance, puis la scène apparaît en fondu. C'est la première image du jeu : elle fixe le style
de tous les menus qui suivent (WFS-37 et suivants réutiliseront le logo et la mise en page de ce ticket).

## Règles de gestion
- RG-01 : **Première image en moins de 0,5 s** après la création de la fenêtre (Android) / du contexte Vulkan (banc) :
  l'écran de chargement est dessiné **avant** tout chargement lourd (piste, voiture, systèmes). Aucun écran noir.
- RG-02 : **Fond** uni `--bg` `#0A0B10` (plein écran, `uiRect`), aucune scène 3D derrière pendant le chargement.
- RG-03 : **Logo** repris du menu web (`index.html`, `#logo`) sur deux lignes, en capitales, police Rajdhani Bold (celle de WFS-26) :
  ligne 1 « WA » blanc `#F2F4F8` + « FOR » en contour (web : `-webkit-text-stroke: 2px`) — **adaptation native permise** :
  « FOR » en gris `--muted` `#8A90A6` si le contour seul n'est pas disponible dans le texte SDF ;
  ligne 2 « SPEED » orange `--accent` `#FF6A00` avec un halo orange léger (web : `text-shadow 0 0 22px rgba(255,106,0,.45)`,
  natif : 1 à 2 copies du texte en orange transparent, alpha ≤ 0,25, légèrement décalées — facultatif).
  Hauteur du logo : `clamp(64 px, 14 % de la largeur, 140 px)` à 1280 px (web : `clamp(64px, 14vw, 140px)`), interligne 0,82.
- RG-04 : **Sur-titre** au-dessus du logo, comme `.eyebrow` du web : « SPRINT URBAIN · VRAIES VOITURES », cyan `--accent-2`
  `#00E5FF`, 12 px logiques mis à l'échelle (≈ 1,7 % de la hauteur), espacement des lettres 0,22 em.
- RG-05 : **Mise en page** des écrans du web (`.screen`, `.screen-inner`) : bloc aligné **à gauche**, centré verticalement,
  marge gauche `clamp(0, 6 % de la largeur, 96 px)` + gouttière 16 px (zone sûre : bords d'écran arrondis / encoche), largeur max 600 px.
  Valable en paysage 1280x720 et 2400x1080 : rien ne sort de l'écran, rien ne se chevauche.
- RG-06 : **Barre de progression** sous le logo : fine (≈ 6 px logiques à 720 p), largeur = celle du bloc (≤ 600 px),
  fond `--surface-2` `#1E2130`, remplissage en **dégradé cyan → orange** (web : `.cp-bar i { linear-gradient(90deg, accent-2, accent) }`) ;
  si le dégradé n'est pas disponible avec `uiRect`, 8 à 16 tranches de couleur interpolée.
- RG-07 : **Étapes et poids** (progression en %, jamais en recul, arrondie à l'entier) :
  | Étape (libellé affiché) | Début → fin |
  |---|---|
  | « Préparation du moteur » (police, matériaux) | 0 → 10 % |
  | « Construction de la route » (`track_.build`) | 10 → 20 % |
  | « Chargement du modèle 3D… » (`loadCarModel`, libellé du web `#carLoading`) | 20 → 70 % |
  | « Construction de la ville » (`systemsInit` : ville, trafic, sons…) | 70 → 100 % |
  Texte sous la barre : `<libellé> <n> %` (web `renderLoadProgress` : `' ' + Math.round(p * 100) + ' %'`, espace avant %),
  gris `--muted`, ≈ 2,2 % de la hauteur. Le pourcentage s'écrit avec `ui::nombre`.
- RG-08 : **Fin** : à 100 %, la scène apparaît par un **fondu de 300 ms** (web : `@keyframes screen-in`, 300 ms) : l'écran de
  chargement passe de l'opacité 1 à 0 par-dessus les premières images de la scène. **Aucune attente artificielle** (pas de durée
  minimale d'affichage : on gagne du temps, comme le web qui n'attend les polices que 1,5 s au plus).
- RG-09 : **Échec** : si le chargement échoue (modèle de voiture introuvable, `Game::init` faux), l'écran reste affiché avec le
  message du web (`main.js boot()`) en rouge `--danger` `#FF1F4B` : « Le moteur 3D n'a pas pu démarrer sur cet appareil. »
  et le journal `WFS_ERR` — jamais de plantage ni d'écran noir.
- RG-10 : **Journal** : une ligne par étape avec sa durée (`chargement : Chargement du modèle 3D… 1,42 s`) et le total, pour que
  le QA et les futurs tickets de performances (WFS-20) voient où part le temps.
- RG-11 : **Captures du banc PC** (`desktop_main.cpp`) : `chargement-0.png` (0 %, étape 1), `chargement-1.png` (étape « Chargement
  du modèle 3D… », entre 20 et 70 %), `chargement-2.png` (100 %), `chargement-fondu.png` (milieu du fondu, scène visible à travers),
  `chargement-erreur.png` (voiture inexistante demandée → message RG-09). Les captures existantes (vitrine, course, trafic…)
  ne changent pas : le banc les prend après le fondu.

## À réutiliser
- Jeu web (`/home/user/waaps/waforspeed`, dépôt `wlfaleh/waaps`) :
  - `index.html` → balisage du logo `#logo` (« Wa<em>for</em> » / « Speed ») et du sur-titre `.eyebrow` ;
  - `css/style.css` → jetons `:root` (`--bg`, `--surface-2`, `--accent`, `--accent-2`, `--muted`, `--text`, `--danger`),
    `.logo`, `.logo .speed` (halo), `.eyebrow`, `.screen` / `.screen-inner` (alignement à gauche, gouttières), `.cp-bar i` (dégradé),
    `@keyframes screen-in` (fondu 300 ms) ;
  - `js/ui/screens.js` → `renderLoadProgress()` (format « Chargement du modèle 3D… 42 % ») ;
  - `js/main.js` → `boot()` (ordre des étapes, message d'échec du moteur 3D, attente des polices bornée à 1,5 s).
  - Il n'y a **pas** d'écran de chargement d'application dans le web (le navigateur affiche la page tout de suite) : seul le style
    et les textes ci-dessus se réutilisent, le reste est à écrire (petit).
- Natif : texte `src/ui/text.h` (WFS-26 : `ui::text`, `ui::width`, `ui::nombre`, `ui::majuscules`, couleurs `ui::CYAN`, `ORANGE`,
  `GRIS`, `ROUGE`), `Renderer::uiRect`, `readAsset` ; police `assets/fonts/Rajdhani-Bold.ttf` (la même que le logo web).
  **Ne pas** créer de nouvelle police, de nouvelle image de logo ni de nouveau pipeline graphique.
- Retours de l'utilisateur déjà connus : style `design-street-racing` (sombre, néon cyan / orange, aligné à gauche) ; tout en français,
  accents corrects.

## Zone de code
- Ticket : `src/ui/ecrans/chargement.{h,cpp}` (nouveau dossier de l'épic E8, compilé automatiquement par le `GLOB_RECURSE` de CMake).
- **Accroches annoncées au Journal (≤ 10 lignes au total)** :
  - `src/game/game.{h,cpp}` : paramètre facultatif de progression à `Game::init` (ex. `std::function<void(float, const char*)>`),
    appelé entre les étapes de RG-07 (attention : WFS-07 et WFS-12 touchent aussi `game.cpp`, lignes différentes) ;
  - `src/ui/text.h/.cpp` : rendre le chargement de la police public et sans double chargement (ex. `ui::chargerPolice(r)`) ;
  - `src/platform/android_main.cpp` et `desktop_main.cpp` : afficher l'écran avant `game.init`, passer la progression, captures RG-11.
- Interdit : toute autre modification du noyau (`src/core`, API du renderer, CMake) — passer par un ticket E0.

## Critères d'acceptation (vérifiables sur les captures du banc PC)
- [ ] `chargement-0/1/2` : fond `#0A0B10`, sur-titre cyan, logo « WAFOR / SPEED » (SPEED orange), barre dégradée cyan → orange
      remplie à 0 %, entre 20 et 70 %, puis 100 %, libellé de l'étape + « n % » sous la barre, accents corrects
- [ ] mêmes captures en `WFS_TAILLE=2400x1080` : bloc à gauche, lisible, rien de coupé ni de chevauché
- [ ] `chargement-fondu` : la scène de la ville se voit à travers l'écran de chargement à moitié transparent
- [ ] `chargement-erreur` : message rouge « Le moteur 3D n'a pas pu démarrer sur cet appareil. », pas de plantage
- [ ] journal du banc : une ligne par étape avec sa durée + total ; première image de chargement < 0,5 s après l'initialisation Vulkan
- [ ] les autres captures du banc (vitrine, course, trafic, ombres, arbres) inchangées ; CI verte (banc, APK, émulateur)

## Hors périmètre
- Menu principal (WFS-37), écrans et navigation (WFS-36), garage (WFS-11).
- Chargement en arrière-plan sur un autre thread (la progression par étapes suffit ; à revoir avec WFS-20 si le chargement dépasse 3 s).
- Icône et nom de l'application, écran de démarrage système Android (WFS-29, E7).

## Suivi repris de `TACHES.md` (2026-11-06, réorganisation demandée par l'utilisateur)
Ligne complète du tableau à cette date :

| 0.1 | WFS-35 | [MVP] Écran de chargement de l'application (logo, barre de progression, fondu) | E8 | ✅ Terminé |  | WFS-26 | `feature/waforspeed-35` | PR #12 (fusionnée) | 🟡 PASSABLE AVEC ANOMALIES MINEURES (ANO-10) |

Critères et notes de suivi :

**WFS-35 — Écran de chargement de l'application** · E8 · priorité 0.1 · ✅ Terminé (QA : PASSABLE AVEC ANOMALIES MINEURES, [rapport](qa/rapports/WFS-35.md)), PR #12 (règles : `regles-de-gestion/WFS-35.md`)
- [ ] `chargement-0/1/2` : fond `#0A0B10`, sur-titre cyan, logo « WAFOR / SPEED » (SPEED orange), barre cyan → orange à 0 %, 20-70 %, 100 %, étape + « n % »
- [ ] mêmes captures en 2400x1080 : bloc à gauche, rien de coupé ; `chargement-fondu` : ville visible à travers (fondu 300 ms)
- [ ] `chargement-erreur` : « Le moteur 3D n'a pas pu démarrer sur cet appareil. » en rouge, pas de plantage
- [ ] journal : durée de chaque étape + total, première image < 0,5 s ; autres captures inchangées
