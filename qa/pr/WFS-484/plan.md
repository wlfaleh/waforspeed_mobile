# WFS-484 — plan (Claude, agent dev3d (01Us6uCvb4))

- Même plaque que WFS-483 (`src/render/vehicule_plaque*`) : mêmes maillages (porte-plaque à jupe par voiture, plaque, vis),
  mêmes matériaux, éclaireur de la plaque arrière la nuit.
- Nouveau module `src/render/vehicule_plaques_autres.*` (zone E10) :
  - tirage du texte (CPU, testé) : 7 caractères `AB123CD` ou `AB-1234`, filtre de mots de WFS-480, jamais la plaque du joueur,
    stable pour un numéro donné pendant la course ;
  - **un seul atlas** de 64 plaques (couleur + normales) préparé au chargement de la carte ; chaque case a son maillage de plaque
    (uv de la case) : aucune création pendant la course ;
  - Basse : version simple, plaques du trafic absentes au-delà de 40 m.
- Accroches (≤ 10 lignes, annoncées au journal) : trafic, Course à 5, police (Poursuite), duel ; fantôme = plaque du joueur.
- Tests : 1 000 tirages (aucun mot interdit, jamais la plaque du joueur, même texte avant / arrière) ; captures avant / après.
