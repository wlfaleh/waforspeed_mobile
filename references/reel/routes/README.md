# Revêtements de route réels (WFS-205, lot 1 de WFS-155)

Dossier du référentiel des revêtements de chaussée : plages d'albédo et de rugosité de chaque revêtement de la bibliothèque
(`assets/textures/route/`), sources des scans CC0 et corrections d'albédo appliquées (RG-02).

Plan (WFS-205) :
1. 7 revêtements : enrobé neuf, enrobé usé, enrobé grenu de campagne, béton bitumineux de circuit, béton de trottoir, pavés de
   granit, gravier de bas-côté. Scans CC0 trouvés dans des dépôts GitHub publics (`regles-reseau.md` § 2.4), sinon procédural recalibré.
2. Albédo recalibré sur `references/reel/materiaux.json` ; KTX2 avec l'outil de WFS-149 (`tools/textures/convertir.py`).
3. Mélange sans répétition dans `shaders/route.glsl` : détail 1 m, macro-variations 20-50 m, tirage stochastique des pavés de texture.
4. Repli procédural (WFS-149) si une texture manque.
