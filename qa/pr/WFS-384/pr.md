Agent : agent expert ci cd (session 0183bv2Y)

## Demande de l'utilisateur
« C'est le problème : pourquoi le QA n'arrive pas à trouver les captures pour tester » → « Prend 2 et 3 » (2026-11-10) : corriger durablement les bancs du matin (WFS-384, ANO-226).

## Cause (vérifiée sur GitHub le 2026-11-10 à 07:40 UTC)
- Aucun banc du matin complet depuis le run #5 (2026-11-09 01:34 heure de Paris, lancé à la main) : les réveils de GitHub sont arrivés entre 09:44 et 18:10 (runs #6 à #11), tous sautés (banc déjà fait ce jour-là, ou après 10:59) ; le 2026-11-10, **aucun** réveil avant 08:40.
- La Release `captures-matin` (où la QA lit depuis WFS-352) n'a donc jamais été créée ; l'issue « 🧪 Recette QA du matin » n'a rien dit à la QA.

## Ce qui change (zone E7 seulement : `.github/workflows/matin.yml`, `tools/qa/**`)
- `matin.yml` : réveils de 04:00 heure de Paris gardés ; les 4 réveils de secours à l'heure pleine sont remplacés par **un réveil chaque heure de 03:23 à 20:23 UTC** (minute 23, moins chargée chez GitHub). Un réveil sans banc dure ~10 s.
- `tools/qa/heure_matin.sh` (nouveau, sort le code du job « heure ») : **RG-01** bancs lancés au premier réveil à partir de 04:00 heure de Paris **quelle que soit l'heure** (avant : seulement jusqu'à 10:59) ; **RG-03** jamais deux bancs le même jour (même règle qu'avant : un run du jour dont la compilation a tourné) ; les réveils sautés de moins de 2 min sont écartés sans lire leurs jobs (quota de l'API).
- **RG-06** : nouveau job `retard` + `tools/qa/retard_matin.sh` : bancs partis après 07:00 → message « ⏳ Bancs du matin en retard » dans l'issue #242 (heure prévue de fin, état des captures, où lire en attendant). `tools/qa/etat_captures_matin.sh` (nouveau) lit `jour.txt` de `captures-matin`. `recette_prete.sh` : sans captures du jour, le titre devient « ⚠️ Recette QA : captures du jour manquantes » (jamais « Recette QA prête »).
- **RG-04** : `publier_apercu.sh` : les notes de la Release `apercu` ne disent plus « captures en cours de publication » sans fin, mais où sont les captures (`captures-matin`, `apercu-fusion`). 1 appel de plus (≤ 15, RG-11 de WFS-106).
- **RG-02** : inchangé, la garde de WFS-340 dans `statuts.yml` (run sauté = aucun statut) reste.

## Vérification
- `tools/qa/test_matin.py` (nouveau, lancé au début de chaque run de `matin.yml`) : 17 cas avec un faux `gh`, tous verts (`qa/pr/WFS-384/test_matin.txt`) : à la main, avant 04:00, premier réveil à 15:23, réveil à 05:23, banc déjà fait, banc en cours, banc rouge, liste illisible, état des captures (absent / veille / jour), messages recette et retard, pas de doublon.
- État réel lu sur le dépôt : `qa/pr/WFS-384/etat_reel_2026-11-10.txt` (Release absente → la QA est renvoyée vers `apercu-ancien-20261109-0925`).
- Pas de captures d'écran : aucun changement visible du jeu (US technique de CI). Pas de CI de branche lancée : sur une branche, `matin.yml` lance tout le banc (1 h 20) sans rien publier, cela ne prouverait pas le réveil programmé.

## Limites et risques
- Si GitHub ne réveille rien de toute la journée, aucun banc ne tourne : le lancement à la main reste possible.
- Un banc lancé tard (ex. 15:00) donne les ✅ des tickets le soir ; la QA est prévenue par le message de retard.
- Après la fusion, le prochain réveil (minute 23) lancera le banc du 2026-11-10 s'il n'a pas encore tourné : c'est le 1er critère d'acceptation (numéro du run au journal).
