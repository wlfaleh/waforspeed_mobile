# WFS-460 — plan de travail (Claude, agent dev3d (01UY1Vv5Cs))

1. ANO-250 : remplacer le quadrilatère plat des bandes « POLICE » (cube unité mis à l'échelle de la boîte) par un maillage
   qui épouse la vraie tôle des portières de la 208 : grille projetée sur les triangles de la carrosserie (lecture du `.wfm`),
   décalée de quelques millimètres vers l'extérieur, des deux côtés, texte à l'endroit des deux côtés.
2. ANO-260 : trouver la cause exacte du gris argent (matériaux de la 208, niveaux de détail, lots de la qualité Basse) et
   appliquer une seule peinture blanche partout.
3. Test CPU de la projection, captures avant / après (`tools/qa/captures_pr.sh`), banc perf Poursuite en Basse.
