## Agent : agent arwa (Claude, session trafic)

**Ticket** : WFS-292 — règles : `regles-de-gestion/WFS-292.md` (décision du PO sur RG-04 : a2d554d, 2026-11-08 22:51 UTC)

## Description

- **Demande de l'utilisateur** : en vrai dérapage, une fumée claire qui se voit derrière les roues, sans les « boules grises » ; un vrai choc au banc d'effets ; la Golf qui se lit bleue la nuit sous les lumières (ANO-165, ANO-167, remarque VABF).
- **Ce qui change** :
  - **Fumée (RG-01, RG-02)**, `src/render/effets_systeme.cpp` et `effets_particules.h` :
    - La lumière émise fixe (calée sur l'exposition 0,85) est remplacée par une matière diffuse claire (albédo 0,75, `MAT_VTINT`, normale vers le haut), éclairée par la scène : le soleil et le ciel le jour, les phares et les lampadaires la nuit, rien dans le noir. La même matière sert le jour et la nuit.
    - Opacité environ 0,44 à la naissance ; apparition en 0,05 de la vie de la volute ; effacement en fondu^1,2.
    - La volute part avec 65 % de la vitesse de la voiture, prise sur la piste quand le modèle du monde n'est pas actif. Avant, elle restait immobile, ce qui donnait les ronds espacés de 1,2 m mesurés au banc. La montée est retenue à 0,4 m/s.
    - Les volutes sont 1,5 fois plus larges que hautes : elles se recouvrent en une nappe au ras de la chaussée.
    - Le seuil de glisse de WFS-224 et les valeurs vérifiées par `test_effets` ne changent pas.
  - **Choc du banc (RG-03)**, `src/render/bench_effets.cpp` : le pilote automatique est coupé dès le placement. Cause : il évitait la voiture de devant (CI 42414202799, c9eda61 : x 0,44 → 0,77, 171 km/h, 0 accident). Les règles des chocs de WFS-256 ne changent pas. La suite `fumee-derapage-jour` / `-05s` / `-1s` est ajoutée.
  - **RG-04** : décision du PO du 2026-11-08 à 22:51 (a2d554d). En rase campagne la nuit, le gris bleuté est accepté (vision de nuit de WFS-230, `post.frag` reste interdit). Sous les lampadaires, la Golf se lit bleue. `shaders/vehicule.glsl` n'est pas modifié.
  - **Accroches** : aucune. **Fichiers touchés** : `src/render/effets_systeme.cpp`, `src/render/effets_particules.h`, `src/render/bench_effets.cpp`.
- **Comment c'est vérifié** :
  - Rapport fumée / chaussée juste derrière les roues, moyenne de la zone : 1,32 sur `fumee-derapage-jour`, 1,42 à +0,5 s, 1,39 à +1 s. La nuit : 1,30 sous les lampadaires.
  - Banc d'effets : 1 accident et 20 étincelles émises au choc, de jour comme de nuit ; 57 bouffées dans `accident-fumee`.
  - Golf sous les lampadaires en Ville de nuit : RGB (26,21,32), saturation 0,35, B ≥ 1,22 × max(R, G).
  - **RG-05**, Ville de nuit sur les captures de WFS-270 : écart moyen 0,70 % sur `phares-autres-ville-haut` (pluie et adaptation de l'œil) et 0,03 % sur `phares-autres-ville-face` (≤ 1 %). Aucun shader n'est modifié ; `carte-campagne-cascade` de jour et de nuit était identique au pixel au 1er passage.
  - **RG-04, condition du PO** (Golf bleue sous un lampadaire) : sur `fumee-derapage-nuit` (Ville), carrosserie (26,21,32), saturation 0,35, B = 1,22 × max(R, G).
  - Tests CPU : 79 réussis. Deux tests mesurés au temps, `test_duel` et `test_audio_chocs`, ont échoué une fois sous la charge de la machine ; ils passent seuls et au passage suivant.
  - Parcours complet : 0 écart, vue normale et Cockpit.
- **Limites et risques connus** :
  - Sur `phares-autres-course5-go-haut`, la Golf du joueur, éclairée de très près par les phares des 4 adversaires et les lampadaires, se lit gris argent bleuté. Cette image n'a pas changé avec ce ticket : la peinture n'est pas modifiée. La preuve « bleue sous un lampadaire » est donc `fumee-derapage-nuit`. Si le PO veut aussi la Golf bleue dans ce cas, il faut un ticket à part sur le reflet du vernis la nuit (WFS-270).
  - La durée des étincelles (0,33 à 0,66 s, vérifiée par `test_effets`) reste au-dessus de la cible réaliste de 0,3 s.
  - En rase campagne la nuit, la Golf reste gris bleuté, comme le PO l'a accepté.
  - Le banc `campagne` est en « échec » sur `main` aussi (arrêt de 4,0 s au col) : ce défaut ne vient pas de ce ticket.

## Captures pour le lead dev (10 après)

Dossier : [`qa/pr/WFS-292`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292) — après : `tools/qa/captures_pr.sh` ; avant : même passage (`effets`) et banc `nuit` de main réduit aux scènes de la Ville (voir README).

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `fumee-derapage-jour` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/avant-fumee-derapage-jour.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/apres-fumee-derapage-jour.jpg?raw=true) |
| `fumee-derapage-jour-05s` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/apres-fumee-derapage-jour-05s.jpg?raw=true) |
| `fumee-derapage-jour-1s` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/apres-fumee-derapage-jour-1s.jpg?raw=true) |
| `fumee-derapage-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/avant-fumee-derapage-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/apres-fumee-derapage-nuit.jpg?raw=true) |
| `choc-etincelles` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/avant-choc-etincelles.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/apres-choc-etincelles.jpg?raw=true) |
| `accident-fumee` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/avant-accident-fumee.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/apres-accident-fumee.jpg?raw=true) |
| `accident-fumee-nuit` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/avant-accident-fumee-nuit.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/apres-accident-fumee-nuit.jpg?raw=true) |
| `phares-autres-ville-haut` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/avant-phares-autres-ville-haut.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/apres-phares-autres-ville-haut.jpg?raw=true) |
| `phares-autres-ville-face` | ![avant](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/avant-phares-autres-ville-face.jpg?raw=true) | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/apres-phares-autres-ville-face.jpg?raw=true) |
| `phares-autres-course5-go-haut` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-292/qa/pr/WFS-292/apres-phares-autres-course5-go-haut.jpg?raw=true) |
