## Agent : agent arwa (Claude, session trafic)

**Ticket** : WFS-292 — règles : `regles-de-gestion/WFS-292.md`

## Description (à remplir par le développeur avant d'ouvrir la PR)

- **Demande de l'utilisateur** : la fumée des pneus doit se voir en vrai dérapage (sans les « boules grises »), le banc d'effets doit refaire un vrai choc, et la Golf doit se lire bleue la nuit en Campagne (ANO-165, ANO-167, remarque VABF).
- **Ce qui change** :
  - `src/render/effets_systeme.cpp` : la fumée n'émet plus de lumière fixe. Ce gris était calé sur l'exposition 0,85 et ne suivait plus l'exposition physique : de jour, la fumée sortait plus sombre que l'asphalte. Les volutes sont maintenant une matière diffuse claire (albédo 0,75, `MAT_VTINT` : Lambert sans reflet, normale vers le haut), éclairée par la scène : soleil et ciel le jour, phares et lampadaires la nuit, rien dans le noir (RG-02). Opacité au plus dense 0,53, soit environ 0,44 à la naissance (0,25 avant). Une volute toutes les 35 ms par roue au lieu de 50 ms.
  - `src/render/effets_particules.h` : effacement en fondu^1,2 au lieu de fondu^1,5 : la nappe dure 1 à 2 s au lieu de disparaître dès 0,5 s. Profil gaussien sans bord net (WFS-224) inchangé.
  - `src/render/bench_effets.cpp` (RG-03) : le pilote automatique est coupé dès le placement du choc. Cause nommée : à c9eda61 (journal de la CI 42414202799), le pilote automatique évitait la voiture de devant (changement de file, x 0,44 → 0,77, 171 km/h, 0 accident en 3 s). Les règles des chocs de WFS-256 ne changent pas.
  - Aucune accroche hors zone. `shaders/vehicule.glsl` n'est pas modifié (voir RG-04 ci-dessous).
- **Comment c'est vérifié** : parcours complet 0 écart (vue normale et Cockpit) ; jour et Campagne de nuit identiques au pixel (`carte-campagne-cascade*`, aucune carrosserie touchée) ;
  - 74 tests CPU OK (`tools/qa/tests_cpu.sh`), dont `test_effets`.
  - Banc `effets` : choc avec 1 accident et 20 étincelles émises, de jour comme de nuit ; 57 bouffées dans `accident-fumee`.
  - Mesure sur `fumee-derapage-jour` : fumée / chaussée 1,17 en médiane, 1,34 au 90e centile, 2,0 au cœur des volutes ; la nuit, visible seulement sous les lampadaires.
  - Parcours complet : voir le journal.
- **RG-04 (Golf bleue la nuit en Campagne) : pas tenue, la cause est hors de ma zone, à arbitrer par le PO**. Mesures sur `carte-campagne-cascade-nuit` :
  - Sur `main` : carrosserie (76,76,99), saturation 0,23, rapport carrosserie / route voisine 1,23.
  - Trois variantes du shader de peinture donnent la même image, à ±1 près : K 2,0 → 1,0 avec la part diffuse teintée × 2 ; reflet du ciel du vernis mis à 0 ; peinture forcée bleu pur à luminance égale (78,78,102). Ce n'est donc ni la part diffuse K ni le vernis : la teinte se perd dans la vision de nuit (Purkinje) de `post.frag` (WFS-230), que je n'ai pas le droit de toucher.
  - Ma compensation dans `vehicule.glsl` (saturation selon le poids de Purkinje) n'a gagné que +0,03 : je l'ai retirée.
  - Le rapport ≤ 1,5 est tenu avec ma mesure (1,23).
- **Limites et risques connus** :
  - Les volutes de jour restent des taches douces et espacées à 108 km/h (pas de bord net). Je n'ai pas réduit la montée de 1,2 m/s ni la durée de 2,2 s, car `test_effets` vérifie ces valeurs.
  - La durée des étincelles (0,33 à 0,66 s, valeurs du jeu web vérifiées par `test_effets`) dépasse la cible réaliste de 0,3 s ; non changée.
- **Banc `campagne`** : il sort en « échec » des deux côtés, sur main comme sur la branche (plus long arrêt de 4,0 s sur la route du col, au-dessus de la limite de 3 s). Ce défaut existe déjà sur main et ne vient pas de ce ticket.
- **Accroches** : aucune. **Fichiers touchés** : `src/render/effets_systeme.cpp`, `src/render/effets_particules.h`, `src/render/bench_effets.cpp`.

## Captures pour le lead dev (8 après)

Dossier : [`qa/pr/WFS-292`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `carte-campagne-cascade-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/avant-carte-campagne-cascade-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/apres-carte-campagne-cascade-nuit.jpg?raw=true) |
| `carte-campagne-cascade` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/avant-carte-campagne-cascade.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/apres-carte-campagne-cascade.jpg?raw=true) |
| `carte-campagne-haut-du-col` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/avant-carte-campagne-haut-du-col.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/apres-carte-campagne-haut-du-col.jpg?raw=true) |
| `accident-fumee-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/avant-accident-fumee-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/apres-accident-fumee-nuit.jpg?raw=true) |
| `accident-fumee` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/avant-accident-fumee.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/apres-accident-fumee.jpg?raw=true) |
| `choc-etincelles` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/avant-choc-etincelles.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/apres-choc-etincelles.jpg?raw=true) |
| `fumee-derapage-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/avant-fumee-derapage-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/apres-fumee-derapage-jour.jpg?raw=true) |
| `fumee-derapage-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/avant-fumee-derapage-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/apres-fumee-derapage-nuit.jpg?raw=true) |
