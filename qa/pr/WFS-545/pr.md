## Agent : Claude, agent dev3d (01Us6uCthf)

**Ticket** : WFS-545 — règles : `regles-de-gestion/WFS-545.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : sur son téléphone, un appui sur une lettre ou un chiffre l'écrit plusieurs fois (« Rrrrrr », « effacer » qui vide tout) ; il veut un délai après chaque appui.
- **Cause** : Android envoie chaque mouvement d'un doigt posé avec `down = true` (`AMOTION_EVENT_ACTION_MOVE`) ; `toucher()` de « Crée ton pilote » prenait chaque `down = true` pour un nouvel appui → un doigt qui bouge d'un pixel retapait la touche à chaque image. Le salon du duel gardait déjà un doigt à la fois, mais n'avait pas de délai.
- **Ce qui change** :
  - `creation_profil_saisie.{h,cpp}` : petite classe `FiltreAppuis` (sans rendu ni horloge, testée sur PC) : `poser(id, down)` vrai seulement au poser d'un doigt pas encore posé (RG-01) ; `accepter(t)` refuse tout caractère moins de **150 ms** après le précédent (RG-02).
  - `creation_profil.cpp` : `toucher()` passe par le filtre (mouvements du doigt ignorés, délai sur les 41 touches, effacer et majuscules compris) ; les champs et « C'est parti » réagissent toujours au poser du doigt ; le retour visuel de 120 ms est gardé. Banc : appuis espacés de 150 ms + 2 scénarios WFS-545.
  - `salon.cpp` : délai de 150 ms sur les 32 touches du code et « effacer » (RG-03) ; « Rejoindre », « Créer », etc. inchangés. Banc : scénario « effacer » maintenu + 2e appui à 50 ms.
  - Rien dans `android_main.cpp` ni `Game::onTouch` (RG-04) : la conduite ne change pas.
- **Comment c'est vérifié** :
  - `test_creation_profil` (RG-05) : appui + 10 mouvements + lever → 1 caractère ; 2 appuis à 50 ms → 1 ; à 200 ms → 2 ; pareil pour « effacer » ; bornes 149 / 150 ms. **138 tests CPU réussis** (`tools/qa/tests_cpu.sh`).
  - Banc `creation` (fr) : avant « Rrrrrrrrrrr » et « Ghjjj » (3 écarts), après « R » et « Ghj » ; **parcours complet : 0 écart** (scénario chronométré RG-05 de WFS-481 compris).
  - Banc `menu2` (salon) : avant « K7 » (effacement en série), après « K7Q » ; aucune nouvelle erreur au journal (les 3 erreurs restantes existent aussi sur `main`).
- **Limites et risques connus** : l'heure utilisée est celle de la dernière image (horloge réelle sur le téléphone) : si une image dure plus de 150 ms, deux vrais appuis tombés dans cette même image ne donnent qu'un caractère (cas rare, sans double frappe). Pas d'essai sur téléphone ici : ticket **🟡 À valider (utilisateur)** après la fusion. Captures « avant » faites à la main (code de `main` + scénarios du banc de la branche), car ces scénarios n'existent pas sur `main`. Salon : « Derniers adversaires : NADIA » n'apparaît que sur les « après » ; aucune ligne du diff ne touche cette liste : c'est l'état du banc `menu2` à ce moment (les appuis du banc sont maintenant espacés de 150 ms, l'horloge du banc avance donc autrement qu'avec le banc de `main` lancé dans un arbre neuf), pas un changement du jeu.

## Tests CPU

Commit testé : `23f4a4c3` (branche synchronisée avec `main`). Journal complet : [`qa/pr/WFS-545/tests-cpu.log`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-545/qa/pr/WFS-545/tests-cpu.log).

```
$ bash tools/qa/tests_cpu.sh build
[OK] test_creation_profil (0 s)
tests CPU : 138 test(s) réussi(s) en 493 s
$ ./build/test_creation_profil
WFS-545 : un appui = un caractère, 150 ms après chaque caractère (caractères et « effacer ») : vérifié
test_creation_profil : OK (0 échec(s))
```

## Captures pour le lead dev (6 après)

Dossier : [`qa/pr/WFS-545`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-545/qa/pr/WFS-545).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `creation-fr-repetition` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-545/qa/pr/WFS-545/avant-creation-fr-repetition.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-545/qa/pr/WFS-545/apres-creation-fr-repetition.jpg?raw=true) |
| `creation-fr-rapide` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-545/qa/pr/WFS-545/avant-creation-fr-rapide.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-545/qa/pr/WFS-545/apres-creation-fr-rapide.jpg?raw=true) |
| `salon-code-efface` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-545/qa/pr/WFS-545/avant-salon-code-efface.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-545/qa/pr/WFS-545/apres-salon-code-efface.jpg?raw=true) |
| `salon-code-saisi` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-545/qa/pr/WFS-545/avant-salon-code-saisi.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-545/qa/pr/WFS-545/apres-salon-code-saisi.jpg?raw=true) |
| `creation-fr-rempli` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-545/qa/pr/WFS-545/avant-creation-fr-rempli.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-545/qa/pr/WFS-545/apres-creation-fr-rempli.jpg?raw=true) |
| `creation-fr-menu` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-545/qa/pr/WFS-545/avant-creation-fr-menu.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-545/qa/pr/WFS-545/apres-creation-fr-menu.jpg?raw=true) |
