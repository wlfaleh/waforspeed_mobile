Agent : Claude, agent dev3d (01Us6uCvhS)

**Ticket** : WFS-481 — règles : `regles-de-gestion/WFS-481.md`, `regles-de-gestion/E17-identite-joueur.md`, `regles-de-gestion/E20-langue-anglaise.md`

## Description

- **Demande de l'utilisateur** : au tout premier lancement, le joueur choisit son pseudo et sa plaque en moins de 20 secondes, en français ou en anglais selon son téléphone, puis arrive au menu.
- **Ce qui change** :
  - Nouvel écran **« Crée ton pilote » / « Create your driver »** (`src/ui/ecrans/creation_profil.*`). Il s'affiche au menu principal après le chargement et les CGU tant que `profil=0` (WFS-480 RG-06), jamais ensuite (RG-01). Un joueur déjà installé le voit une fois, avec son pseudo actuel proposé.
  - Un seul écran (RG-02) :
    - champ **Pseudo** (indice « 3 à 16 caractères ») ;
    - champ **Plaque**, proposée d'après le pseudo (`profil::plaqueProposee`) et remise à jour tant que le joueur ne l'a pas touchée ; si le pseudo n'a ni lettre ni chiffre, la plaque de secours `WFS-123` reste stable (même graine) ;
    - **aperçu 2D de la plaque** : gabarit simple, car la plaque 3D de WFS-483 n'est pas encore fusionnée ;
    - bouton **« C'est parti » / « Let's go »**, grisé tant que le pseudo ou la plaque ne sont pas valides.
  - **Clavier du jeu** (RG-03), dans le style du salon du duel : **AZERTY** en français, **QWERTY** en anglais. Il a les chiffres, `-`, `_`, espace, effacer et majuscules. La majuscule vaut pour une seule lettre et revient quand le champ est vidé, comme sur un téléphone. Dans le champ Plaque, seules les lettres (en capitales), les chiffres, `-` et effacer marchent ; les autres touches sont grisées. Les touches mesurent **57 dp, soit 9,05 mm** : mesuré au banc, la zone touchable fait 62 dp (9,8 mm) en 2400x1080. Saisie séparée et testée : `creation_profil_saisie.*`.
  - **Erreurs à la frappe** (RG-04), en rouge sous le champ, sans jamais répéter le mot : « 3 caractères au moins », « 16 caractères au plus », « Pas deux espaces de suite », « Mot non autorisé » ; pour la plaque : « 1 caractère au moins », « 8 caractères au plus », « Tiret mal placé ».
  - **Langue** (RG-06, RC-E20-01/02) : tous les textes passent par `ui::tr`. Les clés `creation.*` sont dans `assets/textes/fr.txt` **et** `en.txt`. La langue du téléphone est déjà lue par WFS-504 : **aucune accroche dans `android_main.cpp`** n'a été nécessaire.
  - **Validation** (RG-07) : `Profil::creer` écrit le pseudo dans `nick` (aussi pour le duel), la plaque et `profil=1`, puis enregistre. Le menu revient avec son fondu d'entrée.
  - **Retour d'Android** (RG-08) : rien ne change. Au menu, le jeu passe en arrière-plan comme d'habitude, et l'écran revient à la reprise. Jamais de menu sans profil.
  - **Accroches dans la zone E8**, 1 ou 2 lignes chacune :
    - `menu.cpp` : rien du menu sous l'écran ;
    - `reglages.cpp` : pas de bouton « Réglages » sur l'écran ;
    - `salon.cpp` : le lien d'invitation du duel attend la création du profil ;
    - `cgu.cpp` (banc seulement) : profil créé avant la partie Réglages du banc des CGU.
  - **Accroches hors E8**, annoncées au journal :
    - `desktop_main.cpp` : 2 lignes, `WFS_BANC=creation` ;
    - `tools/qa/emulateur.sh` : étape `profil()` d'environ 18 lignes, autorisée par le PO (`regles-de-gestion/WFS-481.md`), avis de l'expert CI / CD « OK ». Retour du lead dev, voir plus bas.
  - **Émulateur du matin** (retour du lead dev) : à la 1re image, l'écran écrit au journal ses points d'appui (`création du profil (WFS-481) : points d'appui : p=… e=… t=… i=… valider=…`). L'étape `profil()` de `emulateur.sh` tape « Petit » (p, e, t, i, t sont à la même place en AZERTY et en QWERTY), puis « C'est parti », et attend la ligne du profil écrit. Captures `02a-profil` et `02b-profil-cree`. Sans écran (profil déjà créé), elle continue après 20 s. Elle ne recopie pas la mise en page dans le script.
  - **Manette** : tant que l'écran est ouvert, seules ses zones ont le focus (champs, touches, « C'est parti ») ; le menu caché dessous n'en a plus.
  - **Touches grisées** (champ Plaque) : fond opaque plus sombre, la scène ne passe plus à travers.
  - Deux systèmes :
    - `CreationProfilToucher` (ordre 401) prend tous les touchers avant le menu et ses bandeaux ;
    - `CreationProfil` (ordre 459) dessine par-dessus le menu, les bandeaux et le panneau des modes, sous les Réglages.
- **Comment c'est vérifié** :
  - `test_creation_profil` (nouveau) : « Petit » de l'émulateur à la même place sur les deux claviers, claviers AZERTY/QWERTY (41 touches, aucune ne chevauche une autre, 38 caractères), touches de la plaque, majuscule, plaque qui suit le pseudo puis plus, limites 16/8, chaque erreur RG-04, scénario RG-05, joueur déjà installé, clés FR/EN.
  - `bash tools/qa/tests_cpu.sh build` : **130 tests réussis** après synchronisation avec main (voir « Tests CPU »).
  - Banc `WFS_BANC=creation` en **FR et EN**, 2400x1080 : **0 écart**. Parcours complet : installation neuve → CGU acceptées → chargement → création → menu → course → fin → menu → 2e lancement (plus d'écran) → joueur déjà installé (pseudo proposé).
  - **RG-05** : 6 lettres + « C'est parti » = **7 appuis, 8,4 s** d'actions (1,2 s par appui), sous la limite de 20 s.
  - En anglais, le garde-fou des textes de WFS-504 (RC-E20-06) trouve **aucune phrase française et aucun texte qui dépasse** sur l'écran.
  - Banc `cgu` relancé sur la branche : voir le résultat dans les commentaires de la PR.
- **Limites et risques** :
  - Aperçu de la plaque en gabarit 2D : il prendra le dessin de la plaque 3D quand WFS-483 sera fusionné.
  - Branche synchronisée avec main après la fusion de WFS-505 : le menu qui suit l'écran est maintenant en anglais sur un téléphone en anglais (capture `creation-en-menu`).
  - Les textes sont ajoutés dans `assets/textes/*` (zone de WFS-504), comme le demande RC-E20-02 (« les deux langues dans la même PR »).
  - Pas de captures « avant » : c'est un nouvel écran et le banc `creation` n'existe pas sur `main` (`SANS_AVANT=1`). Sur `main`, le 1er lancement va directement des CGU au menu.
  - Pas testé sur un vrai téléphone : à valider par l'utilisateur sur l'APK de `main`.

## Tests CPU

Commit testé : `e24205e3` (branche synchronisée avec main, dernier commit de code). Journal complet : [`qa/pr/WFS-481/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-481/qa/pr/WFS-481/tests-cpu.log).

```
[OK] test_version_chargement (0 s)
[OK] test_ville_lieux (0 s)
[OK] test_ville_sol (3 s)
[OK] test_virages_fleches (1 s)
[OK] test_voitures_retirees (0 s)
tests CPU : 130 test(s) réussi(s) en 463 s
```

## Captures pour le lead dev (12 après, banc `creation` en 2400x1080, `WFS_LANGUE=fr` puis `en`, refaites sur `e24205e3`)

Dossier : [`qa/pr/WFS-481`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-481/qa/pr/WFS-481) — produit par `tools/qa/captures_pr.sh` (`SANS_AVANT=1` : nouvel écran).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `creation-en-vide` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-481/qa/pr/WFS-481/apres-creation-en-vide.jpg?raw=true) |
| `creation-fr-vide` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-481/qa/pr/WFS-481/apres-creation-fr-vide.jpg?raw=true) |
| `creation-en-court` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-481/qa/pr/WFS-481/apres-creation-en-court.jpg?raw=true) |
| `creation-fr-court` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-481/qa/pr/WFS-481/apres-creation-fr-court.jpg?raw=true) |
| `creation-fr-interdit` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-481/qa/pr/WFS-481/apres-creation-fr-interdit.jpg?raw=true) |
| `creation-en-rempli` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-481/qa/pr/WFS-481/apres-creation-en-rempli.jpg?raw=true) |
| `creation-fr-rempli` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-481/qa/pr/WFS-481/apres-creation-fr-rempli.jpg?raw=true) |
| `creation-en-plaque` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-481/qa/pr/WFS-481/apres-creation-en-plaque.jpg?raw=true) |
| `creation-en-menu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-481/qa/pr/WFS-481/apres-creation-en-menu.jpg?raw=true) |
| `creation-fr-menu` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-481/qa/pr/WFS-481/apres-creation-fr-menu.jpg?raw=true) |
| `creation-en-installe` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-481/qa/pr/WFS-481/apres-creation-en-installe.jpg?raw=true) |
| `creation-fr-installe` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-481/qa/pr/WFS-481/apres-creation-fr-installe.jpg?raw=true) |
