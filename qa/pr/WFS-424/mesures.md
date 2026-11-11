# WFS-424 — mesures avant de coder (Claude, agent dev3d (01UY1Vv3cC), 2026-11-11 18:59:28 UTC)

Banc PC (lavapipe, même rendu que la CI), `WFS_BANC=perf WFS_PERF_SCENE=1 WFS_PERF_SCENE_QUALITE=basse WFS_PERF_SCENE_LIEU=mimosas WFS_PERF_IMAGES=20` :
Ville de nuit sous la pluie, Rue des Mimosas, vue Arrière, Basse (scène à 60 %, ombres / reflets / captures coupés). La route couvre ≈ 45 % de l'image.

| Variante du shader de la route (Basse seulement, essai local non gardé) | scène (ms), 2 à 3 essais |
|---|---|
| `main` tel quel | 142,4 · 145,4 · 149,2 · 154,7 |
| tout le bruit procédural retiré (macro-variations, usure, raccords, bruit des lignes, bruit de la pluie) = ce qu'une texture précuite remplacerait | 142,2 · 145,0 · 150,7 |
| route plate (couleur unie : ni texture, ni relief, ni bruit) = borne haute de tout gain sur la route | 133,7 · 135,7 |

## Constats
1. Le `fbm` des lignes 129, 191, 193 et 222 de `shaders/route.glsl` **ne tourne pas** sur la chaussée de la Ville ni de la Corniche :
   - 222 = repli procédural, seulement si les textures de l'enrobé manquent ; elles sont dans l'APK (`assets/textures/route/*/…astc.ktx2`, lisibles par l'Adreno 660) ;
   - 191 et 193 = rive de la **Campagne** seulement (`carte == 1`) ;
   - 129 = chaussée **enneigée** seulement (saison avec neige).
2. En Basse, la chaussée lit déjà **une seule** lecture de texture précuite (scan CC0, 256 px) : RG-01 est en fait déjà le cas.
3. Retirer **tout** le bruit restant ne donne **aucun gain mesurable** (dans le bruit de mesure, ± 6 ms sur 145).
4. Même une route **plate** ne gagne que ≈ 8 % de la scène sur le banc (≈ 4 à 5 ms des 62 ms du téléphone, au mieux) : la route n'est pas ce qui fait passer la Ville de 62 à 30 ms.

## Conclusion
Comme WFS-422, le gain attendu de WFS-424 venait d'une hypothèse (« fbm 4 octaves par pixel ») qui ne correspond pas au code exécuté. Aucun code changé.
