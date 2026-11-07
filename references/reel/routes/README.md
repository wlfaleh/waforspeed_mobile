# Revêtements de route réels (WFS-205, lot 1 de WFS-155)

Référentiel des revêtements de chaussée : plages d'albédo et de rugosité de chaque revêtement de la bibliothèque
(`assets/textures/route/`), scans CC0 d'origine et corrections appliquées (RG-02).

| Fichier | Contenu |
|---|---|
| `revetements.json` | pour chaque revêtement : scan Poly Haven, plages du réel (M-01 albédo, M-02 rugosité), gain d'albédo k, rugosité a·r + b, octets |
| `textures-psnr.txt` | rapport de compression de `tools/textures/convertir.py` (PSNR de chaque carte, ASTC 4x4 / 6x6, RGBA8) |
| `outils/preparer.py` | scans téléchargés → KTX2 calibrés (relancer après un changement de plage ou de scan) |

Les 7 revêtements (`src/render/route_revetements*.{h,cpp}`) : enrobé neuf, enrobé usé (chaussée de toutes les cartes dans ce lot),
enrobé grenu de campagne, béton bitumineux de circuit, béton de trottoir (trottoirs), pavés de granit, gravier de bas-côté.
Les variantes par carte, l'usure, les raccords et les bordures sont le lot 2 (WFS-206).

- Sources : scans Poly Haven (CC0) lus dans des dépôts GitHub publics (`regles-reseau.md` § 2.4) ; provenance et SHA-256 :
  `assets/licences/routes-revetements.txt` et journal de WFS-205.
- Plages : `DA-realisme.md` § 3, `materiaux.json` et avis de l'expert-realisme-3d (statuts V / AC / E dans le journal).
- Tailles (décision du PO, 2026-11-07 01:40:18) : couleur et normale 1K (Ultra / Haute), 512 (Moyenne), 256 (Basse) ; ORM moitié ;
  ASTC 4x4 (le 6x6 reste sous le seuil PSNR de WFS-149 : bruit fin des scans) ; repli RGBA8 réduit à 512 (dans l'APK, + 8,6 Mo compressés : le motif d'exclusion du gradle était sans effet).
- Lecture sans répétition (`shaders/route.glsl`, RG-03) : pavage hexagonal (Haute / Ultra), deux échelles (Moyenne), une lecture
  (Basse) ; macro-variations procédurales à 23 et 47 m ; trottoir (dalles) sans rotation, teinte par carreau.
- Repli (RG-04) : GPU sans ASTC : variante RGBA8 (512) ; sans aucune carte lisible, la surface procédurale de WFS-149, recalibrée sur
  l'enrobé usé (albédo moyen ≈ 0,13 au lieu de ≈ 0,07).
- Vérification : `tests/fidelite_routes.cpp` (M-01, M-02, tailles, budget, indice de fidélité).
