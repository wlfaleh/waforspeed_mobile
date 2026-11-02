# QA — WFS-30 : points d'extension du moteur (systèmes enfichables)
Date : 2026-11-02 · Relecteur : qa-lead · Captures : Release `apercu-waforspeed-30`, comparées à `apercu-waforspeed-05`

## Verdict : VALIDÉ

Critères d'acceptation du ticket (TACHES.md) :
- **« Aucun changement visible »** : capture du banc d'essai `course-cam0-2` comparée à celle de WFS-05 : 7 pixels différents sur 921 600, écart maximal de 1 niveau sur 255. C'est identique à l'affichage près. Aucun changement visible.
- **Rendu émulateur** : pas de régression visible (paysage, voiture, route, décor, commandes).
- **CI verte, `test_systems`** : non vérifiés par cette revue, qui ne porte que sur les captures.

## Constats par capture
- `qa-01-demarrage` : paysage non déformé, voiture bleue centrée et posée sur la route, horizon et bordures normaux, HUD et commandes ronds. OK.
- `qa-03-acceleration-8s` : même cadrage que le démarrage, la voiture a peu avancé (fréquence d'image très basse, voir limites). OK.
- `qa-05-braquage` : marquages de voie décalés, voiture stable, aucun artefact. OK.
- `qa-07-fin` : même rendu, rien de cassé. OK.
- `course-cam0-2` (banc) : identique à WFS-05 (voir verdict). OK. Défaut présent dans les deux captures, donc non imputable à WFS-30 : une tache blanche écrêtée sur l'horizon à gauche.
- `vitrine-golf-0` : voiture complète en vue trois-quarts, couleurs et route correctes. OK.
- `qa-logcat.txt` : « QA pret : moteur initialise » présent, aucune erreur, aucun avertissement, aucune exception.

## Limites
- Émulateur à GPU logiciel (SwiftShader, qualité réduite : rendu à 50 %, sans MSAA). Les logs montrent 0,5 image par seconde en permanence, avec une vitesse maximale de 18 km/h et environ 3 m parcourus en 70 s. Les captures ne disent rien de la fluidité ni du comportement à vitesse réelle, et il n'existe pas de log WFS-05 pour comparer les performances.
- Les captures de l'émulateur n'étant pas prises au même instant que celles de WFS-05, seule la comparaison du banc d'essai fait preuve au pixel près.
- CI et `test_systems` non vérifiés ici.
