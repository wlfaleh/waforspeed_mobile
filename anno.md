# WaForSpeed Mobile — anomalies mineures

Tableau des anomalies **mineures** restantes après les deux tours de correction du QA (verdict `PASSABLE AVEC ANOMALIES MINEURES`).
Le **QA** ajoute les lignes (statut 🆕) ; le **PO** les priorise et crée les US de correction (📋) ; le QA les passe en ✅ quand l'US est
validée. Règles : `TACHES.md` (règle 5). Toujours partir de la dernière version de `main` et ne modifier que ses propres lignes.

Statuts : 🆕 Nouvelle (QA) → 📋 Planifiée (PO, US de correction créée) → ✅ Corrigée (QA, US validée) · 🚫 Abandonnée (PO, raison au Journal).

| ANO | Ticket d'origine | Capture | Anomalie | Statut | US de correction |
|---|---|---|---|---|---|
| ANO-01 | WFS-34 | `trafic-cam0` | cœur du houppier un peu visible au bord d'un arbre | 📋 Planifiée | WFS-43 |
| ANO-02 | WFS-34 | `course-*` | ombres des arbres peu visibles (rues à l'ombre des immeubles) | 📋 Planifiée | WFS-43 |
| ANO-03 | WFS-34 | `course-*` | branches en « baleines de parapluie » | 🚫 Abandonnée | — (identique au jeu web) |
| ANO-04 | WFS-12 | `course-decompte-3`, `course-decompte-go`, `course-fin` | texte droit au lieu d'italique (chiffres du décompte, score, stats) : le texte natif (`src/ui/text.*`) ne sait pas incliner les glyphes | 📋 Planifiée | WFS-48 |
| ANO-10 | WFS-35 | `chargement-fondu-2400` | capture prise pendant la planche de test du texte (`WFS_TEXTE_ESSAI=1`) : le panneau « DÉMO DU TEXTE » chevauche l'écran de chargement en fondu ; prendre les captures `chargement-*-2400` dans un lancement du banc sans planche de test | 📋 Planifiée | WFS-49 |
| ANO-14 | WFS-38 | `fin-menu`, `course-fin` | paire « Rejouer » / « Menu » décentrée sur l'écran de fin : « Rejouer » centré seul, « Menu » à sa droite, milieu de la paire à x = 748 au lieu de 640 (+108 px en 1280x720) ; centrer la paire (dessin et zones de toucher, `src/game/course_system.cpp`, E3) | ✅ Corrigée | WFS-57 |
