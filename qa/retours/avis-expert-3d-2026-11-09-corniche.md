# Avis de l'expert réalisme 3D — vidéos du soir (Corniche), 2026-11-09 19:51 UTC

Agent 3d (session_01UY1Vv4DeCjYyC5BsYidg6X). Images regardées : `qa/retours/2026-11-09-video2045-*.jpg` (lacets, vues Proche et Capot)
et `qa/retours/2026-11-09-video2032-*.jpg` (Quai du Moulin, vue Cockpit). Consultant : je ne code pas, je donne un avis et des priorités au PO.

## Verdict en une phrase
La voiture (X5) est le seul objet crédible de l'image ; **tout le reste du décor de la Corniche est encore au niveau « maquette »** :
terrain en grandes facettes d'une seule couleur, mer en aplat, bateaux en triangles, portiques et lampadaires en formes nues,
et une lumière de crépuscule qui ne correspond pas au sol éclairé. Réalisme estimé : **voiture ≈ 70 %, décor ≈ 20 %** (à l'œil, pas de métrique ; à mesurer avec le banc `realisme` dès que WFS-377 le répare).

## Ce qui est déjà pris par le PO (je confirme, pas de doublon)
| Défaut vu | Ticket |
|---|---|
| Pans de terrain et plaques noires qui flottent dans le ciel (lacets) | ANO-212 → WFS-379 |
| Mer bleu ciel plate | ANO-214 → WFS-380 |
| Voiliers = triangles blancs alignés | ANO-206 → WFS-376 |
| Habitacle sans toit, rétroviseur boîte, volant à bandes de marque | ANO-207 / 209 → WFS-323 à 325 |
| Ciel nuit + sol plein soleil | ANO-208 |
| Capot flou en vue Capot | ANO-213 |
| Immeubles « boîtes » | WFS-370 |

## Ce que j'ajoute (pas encore dans les tickets)
| # | Constat sur les images | Vrai objet de référence | Proposition | Coût téléphone | Priorité proposée |
|---|---|---|---|---|---|
| R1 | **Pas de perspective atmosphérique** : les collines à 1 km ont le même contraste que celles à 50 m, d'où l'effet maquette et l'échelle perdue | Brume de mer de la Côte d'Azur : visibilité 15–30 km, les lointains bleuissent et pâlissent | Brouillard exponentiel en hauteur, couleur tirée du ciel (jour / nuit), dans le post existant | ≈ 0,2 ms, 0 triangle | **P1** (gain visuel énorme, coût nul) |
| R2 | **Terrain d'une seule couleur olive** en grandes facettes plates, sans roche ni végétation ; talus de gauche en bandes beiges répétées | Corniche : calcaire blanc-gris, maquis, pins d'Alep, murs de restanques en pierre sèche | Mélange de 3 matériaux selon la pente (herbe sèche < 25°, terre / maquis 25–40°, roche > 40°), normales lissées ; pins d'Alep en instances (1 modèle CC0, 2 LOD) | ≈ 0,5 ms, +15 k triangles avec LOD | P1 (à joindre à WFS-379 ou ticket suivant) |
| R3 | **Portiques des checkpoints** = 2 poteaux-boîtes + une dalle ; petits **panneaux orange sur boîtes** au bord | Portique de chronométrage gonflable ou en treillis aluminium | Ajouter à l'inventaire **WFS-378** (règle des formes interdites) | — | P1 (inventaire) |
| R4 | **Lampadaires du quai** = boule sur un cylindre | Lanterne de promenade niçoise (fût fonte cannelé, crosse, lanterne vitrée) ou candélabre routier | Inventaire WFS-378 / décor WFS-371 | — | P2 |
| R5 | **Glissière** : texture répétée en bandes très visibles, sans lisse ni poteaux | Glissière double onde (lisse 31 cm, poteaux tous les 4 m) ou parapet en pierre | Dans WFS-371 (déjà prévu « glissières »), insister sur le volume et le relief | — | P2 |
| R6 | **Rivage** : la mer coupe le terrain par une ligne droite, sans plage ni rochers | Côte rocheuse, enrochements, galets | À relier à WFS-380 (écume) : enrochement en instances le long du trait de côte | +5 k triangles | P2 |
| R7 | **Plateaux « en escalier »** posés sur la mer (au loin, à droite) | Îlot rocheux ou bateau au mouillage | À vérifier par le dev de WFS-379 / 376 : soit des terrasses mal placées, soit des bateaux lointains en LOD boîte → interdit | — | P1 (avec WFS-379) |
| R8 | **Lampadaire percuté à 135 km/h** : arrêt net, le lampadaire ne bouge pas | Mât fusible : il se plie ou tombe, la voiture perd ~30–50 % de sa vitesse | Déjà ANO-210 ; je propose : mât qui se couche (animation simple, pas de physique lourde) | négligeable | P2 |

## Ordre que je recommande au PO
1. **WFS-379** (terrain qui flotte) — c'est le défaut le plus visible ; y joindre R7.
2. **R1 brouillard / perspective atmosphérique** — nouveau petit ticket, le meilleur rapport réalisme / coût du jeu.
3. **WFS-380** mer + **R6** rivage, puis **WFS-376** bateaux (même zone, même agent si possible).
4. **R2** matériaux du terrain et pins — après WFS-379.
5. **WFS-378** inventaire en incluant **R3, R4, R5**.

## Performance (ligne des Réglages, Basse)
27 img/s, 189 k triangles, **voitures 73 %** du dessin. Le décor peut donc gagner en détail (R1, R2, R6) sans être la cause des lenteurs ;
la priorité performance reste le LOD des voitures (WFS-335). Tout ajout de décor doit passer par instances + LOD et être mesuré en Basse.
