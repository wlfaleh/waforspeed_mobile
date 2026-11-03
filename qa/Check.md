# QA par lot — tickets fusionnés sans QA (fusion rapide sur ordre de l'utilisateur)

Consigne pour le `qa-lead` : pour chaque ligne, regarder les captures du banc PC de la CI sur `main` (au commit de fusion ou après),
vérifier les points listés, écrire `qa/rapports/WFS-XX.md` et rendre le verdict. Bloquant → ticket de correction prioritaire par le PO ;
mineur → `anno.md` (ANO-XX). Cocher la case une fois le rapport écrit.

| ☐ | Ticket | PR | Fusion | À regarder sur les captures |
|---|---|---|---|---|
| ☐ | WFS-37 | #23 | 2026-11-03 | `menu-0`, `menu-1` (720p et 2400x1080), `menu-anim` : ville assombrie, logo, 3 choix, record « — », « Golf 8 GTI » ; aucune commande de conduite ni HUD sur le menu (ajout PO : fusion rapide non listée) |
| ☐ | WFS-42 | #24 | 2026-11-03 | Lunette arrière de la Golf (trame de points), rayons des jantes, feux arrière non dégradés, fils chromés en transparence |
| ☐ | WFS-55 | #26 | 2026-11-03 | Comparaison avant / après : route, horizon, voiture à la même place ; aucune différence visible (banc de la PR non terminé à la fusion, captures non vues ; ≈ 27 captures, écart de quelques pixels) ; `test_cartes` à 1e-9 |
| ☐ | WFS-59 | #31 | 2026-11-03 | Banc PC : toutes les captures identiques à `main` avant fusion (mode GPU logiciel inactif hors écran : ombres présentes, MSAA x4, scène pleine définition — comparaison au pixel non faite à la fusion, Release `apercu` en reconstruction) ; `test_systems` (plafond de dt). Simulateur : ligne logcat `mode GPU logiciel : actif (dt max 0,5 s, rendu 25 %, sans ombres)`, `QA ips` ≥ 1, décompte puis course qui avance (à voir avec WFS-50) |
| ☐ | WFS-58 | #32 | 2026-11-03 | Fusion sur ordre de l'utilisateur alors que le dernier QA était REFUSÉ au tour 2 (WFS58-5 : `01-chargement` presque noire) : relancer `simulateur.yml` (scénario `mvp`) sur `main` ; chaque étape attend son événement du logcat, aucun faux `OK`, `scenario=conduite` inchangé |
| ☐ | WFS-48 | #33 | 2026-11-03 | Texte en italique : chiffres du décompte (`course-decompte-*`), score et stats de `course-fin`, HUD (`hud-*`), notifications ; ANO-04 corrigée ; autres textes droits inchangés |
| ☐ | WFS-39 | #34 | 2026-11-03 | Captures `pause-*` : bouton Menu en course, écran de pause, reprise, recommencer, quitter vers le menu ; simulation figée pendant la pause |
| ☐ | WFS-45 | #35 | 2026-11-03 | `spots-sol` : sol sombre, 3 taches orange, cyan et blanche aux bords adoucis, hémisphère bleutée, brouillard noir au loin, Golf éclairée ; toutes les autres captures identiques à avant (0 projecteur par défaut) ; journal du banc : « passe de scène » avec 3 projecteurs au plus 0,3 ms de plus qu'avec 0. Écart à RG-02 : atténuation r128 non physique (comme le jeu web), à confirmer par le PO |
| ☐ | WFS-28 | #36 | 2026-11-03 | `test_audio` (bruitages : crissement, nitro, chocs, bips) ; fichiers `.wav` du banc s'il y en a ; aucune régression du son du moteur (`moteur-courbe`) |
| ☐ | WFS-69 | #37 | 2026-11-03 | CI : banc en 4 jobs (`partie1`, `partie2`, `texte`, `ecrans-2400`), run 435 en 7 min (contre 29-52) ; Release `apercu-waforspeed-69` : 70 fichiers, aucune capture perdue ; 59 captures identiques à `main` à ±1 niveau près (lavapipe local) ; à regarder : `texte-essai-1280/2400`, `chargement-*-2400`, `hud-2400`, `menu-1`, `encoche-hud-2400` (nouveaux lancements `WFS_BANC=texte|chargement`) ; un commit de doc seule ne lance plus de build |

Ajouter ici chaque ticket fusionné sans QA (ticket, PR, date, points à regarder).
