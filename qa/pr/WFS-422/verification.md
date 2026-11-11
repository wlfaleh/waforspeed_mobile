# WFS-422 — vérification au banc PC (lavapipe, même rendu que la CI), 2026-11-11 16:30:00 UTC

Commande : `WFS_BANC=halos-sol ./build/wfs_bench <sortie> assets/` sur `main` (`4461455b`).
Ce banc (WFS-421) rend la Ville et la Corniche **de nuit** en **Basse, Moyenne et Haute** (6 images, vraie partie, tous les systèmes).

- Passes enregistrées (ligne 9 du journal) : pré-passe, capture d'environnement, sondes IBL, rétroviseurs, passe d'essai.
- La pré-passe écrit « pré-passe (WFS-150) : cibles WxH » la première fois qu'elle tourne (`PrePasse::preparer`).
  Nombre de ces lignes dans le journal des 6 rendus : **0**.
- Conclusion : la pré-passe **ne tourne pas en jeu**, à aucun niveau ; seule la passe d'essai du banc `realisme`
  (`bench_passes.cpp`, `passes::activerEssai(true)`) la déclenche. Il n'y a **aucun effet lecteur** de la pré-passe dans le jeu,
  donc rien à couper ni de secours à écrire, et aucun gain possible de ce côté.
