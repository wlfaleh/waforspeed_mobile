# QA par lot — tickets fusionnés sans QA (fusion rapide sur ordre de l'utilisateur)

Consigne pour le `qa-lead` : pour chaque ligne, regarder les captures du banc PC de la CI sur `main` (au commit de fusion ou après),
vérifier les points listés, écrire `qa/rapports/WFS-XX.md` et rendre le verdict. Bloquant → ticket de correction prioritaire par le PO ;
mineur → `anno.md` (ANO-XX). Cocher la case une fois le rapport écrit.

| ☐ | Ticket | PR | Fusion | À regarder sur les captures |
|---|---|---|---|---|
| ☐ | WFS-37 | #23 | 2026-11-03 | `menu-0`, `menu-1` (720p et 2400x1080), `menu-anim` : ville assombrie, logo, 3 choix, record « — », « Golf 8 GTI » ; aucune commande de conduite ni HUD sur le menu (ajout PO : fusion rapide non listée) |
| ☐ | WFS-42 | #24 | 2026-11-03 | Lunette arrière de la Golf (trame de points), rayons des jantes, feux arrière non dégradés, fils chromés en transparence |
| ☐ | WFS-55 | #26 | 2026-11-03 | Comparaison avant / après : route, horizon, voiture à la même place ; aucune différence visible (banc de la PR non terminé à la fusion, captures non vues ; ≈ 27 captures, écart de quelques pixels) ; `test_cartes` à 1e-9 |

Ajouter ici chaque ticket fusionné sans QA (ticket, PR, date, points à regarder).
