# Humains réalistes (WFS-209, lot 1 de WFS-164)

Référentiel des personnages réalistes : morphotypes (taille, proportions, teint) tirés de `references/reel/anthropometrie.json`,
sources CC0 / CC BY et mesures M-01 (taille), M-02 (proportions), M-03 (albédo de la peau).

Plan :
1. Source CC0 / CC BY trouvée dans un dépôt GitHub public (exports MakeHuman), choix validé par `expert-realisme-3d`.
2. Conversion par `tools/convert_people.mjs` (étendu) en `.wfp` v3 avec tangentes et 3 niveaux de détail, squelette de WFS-63.
3. 12 morphotypes aux tailles réelles pour les conducteurs (WFS-73) ; repli : `homme.wfp` / `femme.wfp` (Quaternius).
4. Mesures dans `tests/fidelite_humains*` ; captures du banc `realisme` et vues Proche / Cockpit.
