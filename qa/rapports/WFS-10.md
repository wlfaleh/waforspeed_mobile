# QA WFS-10 : Sons de moteur (AAudio, régime et rapports)

**Verdict : VALIDÉ** (banc d'essai PC, règle WFS-31)

- Construction regardée : commit `d65cd1e` (branche `feature/waforspeed-10`, PR #7). Texte de la Release `apercu-waforspeed-10` : « Branche feature/waforspeed-10, commit d65cd1e » ; runs Actions 41532718132 (push, publication) et 41532756285 (pull_request), tous deux verts.
- Référence : `waaps/waforspeed/js/audio/sfx.js`, `engineUpdate()` (régime cible `0,22 + 0,7 × position dans le rapport ± 0,1 selon la pédale`, 5 rapports, chute de régime au passage, voix en charge plus forte, voix lever de pied plus douce).

## Preuves regardées

- `moteur-courbe.png` (forme d'onde, régime, rapport, vitesse, pédale), scénario ralenti 2 s / plein gaz → 16 s / lever de pied → 20 s / freinage → 23 s / gaz → 28 s.
- Journal du job `banc-essai` (job 125577073954) : lignes « son t=0s … t=28s », ligne finale « son : 28.0 s enregistrées (1344000 échantillons), crête 0.23, plus grand saut 0.059 ».
- Test `test_audio` dans le même job : « rapports vus 5 (4 passages), régime max 1.01, crête 0.23, RMS 0.064, plus grand saut 0.060, après pause 0.00278 → OK ».
- Captures `course-cam0-1`, `vitrine-m4-0`, `trafic-cam0` : ville, voitures, HUD et commandes tactiles rendus correctement, rien de cassé.

## Constats

| Critère | Preuve | Résultat |
|---|---|---|
| Régime suit la pédale | courbe + journal | Ralenti 0,10 à l'arrêt ; monte dès le gaz (0,68 à t=3 s) ; retombe au lever de pied (0,42 → 0,23 → 0,15 à t=16-18 s) ; remonte au gaz à t=24 s. |
| 5 rapports, chute de régime à chaque passage | courbe (marches cyan) + journal | 1 → 2 → 3 → 4 → 5 pendant le plein gaz, chaque passage suivi d'une chute nette de la courbe orange (ex. 0,87 → 0,42 au passage en 4e, 0,91 → 0,36 en 5e) ; rétrogradages 5 → 1 au lever de pied et au freinage, puis 2 → 3 à la reprise. |
| Son plus fort en charge, plus faible au lever de pied | forme d'onde | Enveloppe large en charge, creux bref à chaque passage (coupure comme le jeu web), enveloppe très faible de 16 à 23 s, reprise à 23 s. |
| Pas de saturation | journal + test | Crête 0,23 (< 1). |
| Pas de craquement | journal + test | Plus grand saut entre deux échantillons 0,059-0,060 (seuil du test 0,25). Pas de discontinuité visible sur la forme d'onde. |
| Coupure en pause | test_audio | 0,00278 après 0,1 s sans nouvelles du jeu (< 0,005, environ −40 dB sous la crête). |
| Rien de cassé ailleurs | `course-cam0-1`, `vitrine-m4-0`, `trafic-cam0` | OK. |

Aucun problème bloquant.

## Remarques (non bloquantes)

- `moteur.wav` annoncé au Journal n'est pas dans la Release (seule la courbe y est) ; la courbe suffit comme preuve (PO : « courbe ou enregistrement WAV »).
- Pas de fichier de licence/provenance dans `assets/audio/` ; les sons viennent du jeu web (`wlfaleh/waaps`) d'après le Journal. À noter au Journal si besoin.
- Limite connue (Journal du développeur) : un coup de gaz à l'arrêt sans avancer ne fait pas monter le régime (pédale déduite de la conduite), alors que le jeu web le fait (`rpmT = 0,85` à l'arrêt). Écart mineur, à reprendre si `GameView` expose la pédale.

## Non vérifiable sur le banc PC (pas un motif de refus, WFS-31)

- Son réel sur téléphone (sortie AAudio, volume, timbre entendu, casque / haut-parleur, réouverture au changement de sortie).
- Latence AAudio et absence de décrochages (underruns) sous charge réelle du jeu sur mobile.
- Coupure du son lors du passage en arrière-plan Android (seule la coupure « plus de nouvelles du jeu » est testée).
À contrôler lors de la vérification simulateur / appareil de fin d'épic E6.
