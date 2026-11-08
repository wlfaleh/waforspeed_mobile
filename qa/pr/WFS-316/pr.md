## Agent : Claude, agent 4 (011cpqZan)

**Ticket** : WFS-316 — règles : `regles-de-gestion/WFS-316.md`

## Description

- **Demande de l'utilisateur** (en une phrase) : volant tenu à gauche dans le rond-point de la Ville (remplacé depuis par un virage de rue de 22 m, WFS-283), de 50 à 25 km/h, aide désactivée, en « Glisser », la voiture doit suivre l'anneau au lieu de filer à l'extérieur et de monter sur le trottoir (vidéo du 2026-11-08, ANO-172).
- **Ce qui change** : `src/game/player.cpp` : voiture du joueur, volant tenu (|volant| ≥ 0,05), aux vitesses des virages serrés (pleine jusqu'à 50 km/h, effacée de 50 à 70 km/h ; borne coupée sous 60 km/h, RG-05) : la force centrifuge forfaitaire du jeu web (la route y « porte » la voiture) et la borne du cap ne s'appliquent plus ; la route tourne déjà sous la voiture (repère de Frenet de WFS-295), la force comptait donc le virage deux fois (**3,2 m/s vers l'extérieur à 50 km/h dans l'anneau de 14 m**). Volant lâché et ordinateur : inchangés. `g_avantWfs316` (banc seulement) pour les mesures « avant ». Nouveau test `test_direction_anneau` ; banc `virages` : section 8, vues de dessus à 20 images/s (rouge avant, vert après), et section 9, suites d'images en caméra Arrière avant / après.
- **Comment c'est vérifié** : `test_direction_anneau` (RG-01 : la force forfaitaire explique 3,17 m de sortie en 1,2 s, la borne n'agit pas sans aide ; RG-03 : rayon dans l'anneau = rayon en ligne droite à ± 1 %, à 25 et 50 km/h, demi-volant et volant à fond, 20 img/s irrégulières et 60 ; RG-04 : anneau de la Ville 50 → 25 km/h, épingle de la Corniche à 30 km/h, virage à 90° de la Ville à 40 km/h : 0 sortie, Glisser et flèches, aide activée et désactivée, 20 et 60 img/s ; RG-05 : volant lâché inchangé, lacet à 150-230 km/h identique avant / après) ; 72 tests CPU OK ; bancs `virages` (0 écart), `conduite` (0 écart), `fluidite`, `parcours` (0 écart, vue normale et cockpit).
- **Limites et risques connus** : au-dessus de 70 km/h, la direction reste celle d'avant (relève de WFS-258). Signalé au PO : à 50 km/h volant à fond, l'adhérence « arcade » du jeu (≈ 2 g) donne un rayon de 9 m, contre ≈ 22 m pour une vraie voiture ; hors de ce ticket. Ticket de conduite : 🟡 après la fusion, à valider par l'utilisateur sur son téléphone.

## Captures pour le lead dev (6 après)

Dossier : [`qa/pr/WFS-316`](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-316/qa/pr/WFS-316) — produit par `tools/qa/captures_pr.sh`.

| Capture | Avant (main) | Après (branche) |
|---|---|---|
| `virages-anneau-dessus` (virage de rue de 22 m qui remplace le rond-point, doigt tenu) | rouge : voir `…-avant` | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-316/qa/pr/WFS-316/apres-virages-anneau-dessus-apres.jpg?raw=true) |
| `virages-anneau-dessus-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-316/qa/pr/WFS-316/apres-virages-anneau-dessus-avant.jpg?raw=true) |
| `virages-epingle-30-dessus-apres` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-316/qa/pr/WFS-316/apres-virages-epingle-30-dessus-apres.jpg?raw=true) |
| `virages-epingle-30-dessus-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-316/qa/pr/WFS-316/apres-virages-epingle-30-dessus-avant.jpg?raw=true) |
| `virages-virage90-dessus-apres` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-316/qa/pr/WFS-316/apres-virages-virage90-dessus-apres.jpg?raw=true) |
| `virages-virage90-dessus-avant` | — | ![après](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-316/qa/pr/WFS-316/apres-virages-virage90-dessus-avant.jpg?raw=true) |

- Note : `test_audio_chocs` a échoué une fois sur la machine chargée (0,0513 ms pour ≤ 0,05), puis passé 3 fois sur 3 : mesure de temps instable, sans lien avec la conduite.


## Retour du lead dev (refus du 2026-11-08) : ce qui a été fait

1. **Captures 2 et 4, en caméra Arrière** : suites de 8 images, avant et après, à 20 images/s irrégulières (pas de 50 à 75 ms).
   - Planche du virage de la Ville (ex-rond-point), doigt tenu à 6 %, de 50 à 25 km/h : ![ville](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-316/qa/pr/WFS-316/planche-ville-arriere.jpg?raw=true)
   - Planche du virage à 90° de nuit, 40 km/h : ![virage90](https://github.com/wlfaleh/waforspeed_mobile/blob/feature/waforspeed-316/qa/pr/WFS-316/planche-virage90-nuit-arriere.jpg?raw=true)
   - Images seules : `avant-virages-*-arriere-NN.jpg` et `apres-virages-*-arriere-NN.jpg`.
   - Aucune image de jeu hors de la chaussée dans les quatre suites.
2. **RG-01** (preuve acceptée par le PO, `regles-de-gestion/WFS-316.md`, commit a3bcc5e9) :
   - Sur l'anneau de 14 m, avant WFS-283 : « avant », 4,43 m vers l'extérieur en 1,2 s, dont 3,17 m dus à la force forfaitaire ; « après », 1,26 m.
   - Sur le virage de rue de 22 m qui le remplace : la force forfaitaire ne fait plus que 1,84 m.
   - Vue de dessus depuis le cœur du virage : avant (rouge), la voiture dérive vers le bord extérieur ; après (vert), elle suit la ligne.
3. **Journal et statut** : entrée au journal ; `TACHES.md` passe à 🔀 En PR.
4. **RG-03 et RG-05** :
   - RG-03 : le PO valide le critère « même rayon qu'en ligne droite, à ± 10 % ».
   - RG-05 : la correction est limitée aux virages serrés, décision validée par le PO. Elle est pleine jusqu'à 50 km/h et s'efface de 50 à 70 km/h ; la borne du cap n'est coupée que sous 60 km/h.
   - À 150, 190 et 230 km/h, le lacet dans le monde est identique avant et après, en ligne droite comme dans un virage rapide du Grand Prix, aide activée (0,296 / 0,250 / 0,205 rad/s en ligne droite) comme désactivée (0,492 / 0,391 / 0,324 rad/s). Tableau complet : `test_direction_anneau`.
   - Sans cette limite, l'aide activée était 1,6 à 3,5 fois plus vive à grande vitesse.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

https://claude.ai/code/session_011cpqZanQgkx5BB21ZddeBj
