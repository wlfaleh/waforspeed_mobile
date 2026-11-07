# Albédo de la peau — phototypes (WFS-210, RG-02)

Valeurs reprises de `references/reel/materiaux.json` (base PB-MAT, statut « vérifié »), sRGB linéaire, tolérance ± 10 %
(DA-realisme.md § 3, plage de la peau 0,05-0,65 depuis le 2026-11-07).

| Phototype | `id` | Albédo (R ; V ; B) | Luminance | Rugosité | F0 |
|---|---|---|---|---|---|
| I | `mat.peau_i.albedo` | 0,847 ; 0,638 ; 0,552 | 0,676 | — | — |
| III | `mat.peau_iii.albedo` | 0,623 ; 0,433 ; 0,343 | 0,467 | — | — |
| VI | `mat.peau_vi.albedo` | **0,09 ; 0,05 ; 0,02** | **0,055** | 0,5 (`mat.peau_vi.rugosite`) | 0,028 (indice 1,4) |

Sources d'origine (citées par PB-MAT pour ces valeurs) :
- Jensen, H. W., Marschner, S. R., Levoy, M. & Hanrahan, P. (2001). *A Practical Model for Subsurface Light Transport*. SIGGRAPH. https://doi.org/10.1145/383259.383319
- Donner, C. & Jensen, H. W. (2006). *A Spectral BSSRDF for Shading Human Skin*. http://graphics.ucsd.edu/~henrik/papers/skin_bssrdf/skin_bssrdf.pdf
- Lister, T., Wright, P. A. & Chappell, P. H. (2012). *Optical Properties of Human Skin*. Journal of Biomedical Optics 17(9), 090901.

Utilisation dans le jeu : `conducteurs::albedoPeau()` (`src/world/conducteurs.h`) : morphotype d'origine africaine = phototype VI
(au lieu du plancher 0,27 de WFS-211) ; origine asiatique = III ; européenne = I ramenée à 0,62 ; morphotypes mixtes : mélange
linéaire. Avis `expert-realisme-3d` (2026-11-07) : tirer par personne parmi IV / V / VI serait plus juste pour une population
(proposé au PO pour une suite : `albedoPeau` n'est tiré que par morphotype aujourd'hui).
Diffusion sous la surface : éclairage enveloppant par canal w = (0,45 ; 0,18 ; 0,10), effacé au-delà de 15 m
(`shaders/scene_skin.frag`, WFS-210).
