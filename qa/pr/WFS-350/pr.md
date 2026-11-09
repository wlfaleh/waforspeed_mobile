Agent : agent expert ci cd

**Demande de l'utilisateur** : « Go » (2026-11-09) sur le plan de l'expert : débloquer tout de suite la publication de l'APK de main.

## Diagnostic (RG-01)
- Run 946 de main (et 942, 943, 945) : seul le job `publication-apk` est rouge ; compilation, tests, captures, endurance, APK : verts.
- Journal : `HTTP 422 … file_count limited to 1000 assets per release` (`tools/qa/publier_apercu.sh`, envoi de l'APK sous un nom provisoire).
- Cause : la Release `apercu` a atteint la limite de GitHub (1 000 fichiers), remplie par les captures des bancs du matin (WFS-277, conception de l'agent expert ci cd). **Aucun ticket du jeu n'est fautif.**

## Ce qui change
- `.github/workflows/apercu-place.yml` (neuf, lancé à la main seulement) : si `apercu` a au moins 900 fichiers, la Release change d'étiquette en `apercu-ancien-<date>` (tous ses fichiers gardés, lisibles par la QA) en **1 appel** à l'API (pas 1 000 suppressions, qui videraient le quota horaire commun à tous les jobs).
- Ensuite, je relance à la main le job `publication-apk` du dernier run de main : `publier_apercu.sh` recrée `apercu` (il la crée si elle manque) avec l'APK, à la même adresse fixe `…/releases/download/apercu/WaForSpeed.apk`.

## Limites et risques
- Quelques minutes sans APK à l'adresse fixe entre le renommage et la publication.
- Correction de fond (captures du matin hors de `apercu`, message « Recette prête » juste) : **WFS-352**, juste après ; sans elle, les bancs du matin rempliraient de nouveau `apercu`.
- Garde-fou : moins de 900 fichiers → rouge, rien n'est changé.

**Captures** : ticket de CI sans changement à l'écran.
