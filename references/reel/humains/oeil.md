# Œil humain : valeurs utilisées par WFS-266 (ANO-134)

Valeurs reprises par `shaders/scene_skin.frag` (branche « œil d'un humain réaliste ») et contrôlées par `tests/fidelite_humains.cpp`.
Le PO les ajoutera à la charte (`regles-de-gestion/DA-realisme.md` § 3) après la recette.

| Grandeur | Valeur retenue | Valeur réelle | Source |
|---|---|---|---|
| Diamètre visible de l'iris (« blanc à blanc ») | 11,7 mm (rayon 5,85 mm) | 11,71 ± 0,42 mm (adultes sains) | Rüfer F., Schröder A., Erb C., « White-to-white corneal diameter: normal values in healthy humans obtained with the Orbscan II topography system », *Cornea* 24 (3), 2005, p. 259-261 |
| Diamètre de la pupille (jour) | 3,5 mm | ≈ 2 à 4 mm en pleine lumière, 4 à 8 mm dans l'obscurité, selon l'âge | Watson A. B., Yellott J. I., « A unified formula for light-adapted pupil size », *Journal of Vision* 12 (10), 2012 |
| Écart des pupilles (adulte) | celui du maillage MakeHuman (test : 55 à 70 mm) | 61,7 mm en moyenne (femmes ≈ 61,7, hommes ≈ 64,0), étendue ≈ 51-77 mm | Dodgson N. A., « Variation and extrema of human interpupillary distance », *Proc. SPIE* 5291, 2004 |
| Cornée (film de larmes) | GGX lisse, F0 = 0,025 (déjà dans `scene_skin.frag`, WFS-210) | indice ≈ 1,376 → F0 ≈ 0,025 | Atchison D. A., Smith G., *Optics of the Human Eye*, 2000 |
| Albédo de la sclère (blanc de l'œil) | codes sRGB (0,76 ; 0,72 ; 0,68), soit ≈ 0,54 / 0,48 / 0,42 en linéaire : **jamais blanc pur** | légèrement jaunâtre, nettement sous le blanc de référence ; **valeur à confirmer** (aucune mesure spectrale sourcée accessible depuis l'environnement : réseau limité à GitHub) | estimation de l'expert réalisme, à remplacer par une mesure sourcée |
| Iris (brun, le plus fréquent) | codes sRGB (0,30 ; 0,19 ; 0,11), limbe plus sombre (0,16 ; 0,10 ; 0,06), pupille 0,03 | iris brun foncé, limbe sombre visible | à confirmer (même limite) |

Géométrie : globes oculaires de MakeHuman (`helper-l-eye` / `helper-r-eye` de `base.obj`, CC0, mêmes cibles et mêmes poids que
le corps). L'iris et la pupille sont dessinés au pixel autour de l'axe du regard (+z en pose de repos), à partir du centre du
globe écrit dans `_cloth.xyz` par `tools/convert_people_makehuman.py`.
