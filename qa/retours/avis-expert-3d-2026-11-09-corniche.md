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

---

# Complément — toutes les captures du jour (2026-11-09 ≈ 20:00 UTC)

Images regardées en plus : `qa/retours/2026-11-09-video2032-*` (les 6), `qa/pr/WFS-192/` (vitrine des 20 voitures, garage, course),
`qa/pr/WFS-193/` (X5, Compass, 208 : nez et zooms), `qa/pr/WFS-338/` (poursuite police, jour et nuit), `qa/pr/WFS-343/` (cartes de données : Noël, Campagne, Circuit, automne).

## Note par zone (à l'œil, sans métrique)
| Zone | Note | Ce qui va | Ce qui ne va pas |
|---|---|---|---|
| **Voitures (20 modèles)** | 🟢 ≈ 70 % | Proportions, jantes, pneus, plaques FR, peinture PBR, ombres de contact : c'est le meilleur du jeu | Voir V1 à V3 |
| **Ville de nuit (poursuite)** | 🟡 ≈ 45 % | Ambiance sodium, pluie, sol mouillé, police avec policier à bord | Voir N1 à N3 |
| **Ville de jour** | 🟠 ≈ 35 % | Arbres d'alignement, terrasses, fissures du bitume | Immeubles boîtes (WFS-370), une grande **boîte grise sans texture** à gauche de la vitrine (début de rue) |
| **Corniche / Quai du Moulin** | 🔴 ≈ 20 % | — | Partie haute de cet avis + habitacle de boîtes (WFS-323 à 325) |
| **Carte de Noël (WFS-343)** | 🔴 ≈ 25 % | Guirlandes lumineuses, idée juste | Voir C1 à C3 |
| **Campagne / automne** | 🟠 ≈ 30 % | Clôtures, arbres variés | Maisons boîtes à toit prisme, sol vert ou orange uni et saturé |

## Points nouveaux
| # | Constat | Référence réelle | Proposition | Priorité |
|---|---|---|---|---|
| V1 | **X5 (zooms WFS-193)** : bord de grille déchiré, triangles qui débordent sur la carrosserie, trou autour de l'ancien logo du capot | Jonction propre, interstice de 3–5 mm, capot lisse | C'est **WFS-358** (attribué ce soir à l'agent dev3d) : je confirme les 4 zones à reprendre | P1 (en cours) |
| V2 | **R4 Competizione** : la calandre garde la forme en double haricot de la marque d'origine ; **Folg 8** : un écusson rond reste au centre de la grille | Règle « voitures sans marque » : calandre et feux sans signature | Ajouter ces 2 voitures à la suite WFS-359 à 361 | P2 |
| V3 | **Garage** : la voiture tourne sur un **anneau néon plat** (disque) | Le garage 3D tombe sous la règle des formes interdites ; plateau tournant réel : disque métallique avec chanfrein, joint et éclairage encastré | À ajouter à l'inventaire **WFS-378** (ou décision de l'utilisateur : le garage compte-t-il comme interface ?) | P2 |
| N1 | **Phares de nuit** : cônes blancs énormes et brûlés sur la route (police, voitures en face), qui mangent l'image | Phare croisement : tache nette au sol à 30–60 m, coupure horizontale, éblouissement limité | Limiter l'intensité et l'étalement du « bloom », coupure du faisceau | P1 |
| N2 | **Lampadaires** : boules lumineuses (disques de halo) ; **fenêtres** : rectangles blancs uniformes, toutes allumées au même niveau | Lanterne avec verre et source cachée ; à 22 h, 30–50 % des fenêtres allumées, teintes 2700–4000 K variées | Inventaire WFS-378 (halos) + variation des fenêtres dans WFS-370 | P2 |
| N3 | **Rampe de la police** : tache bleue sans forme | Rampe réelle 1,2 m × 0,25 m, deux couleurs bleues alternées, boîtier noir | Modèle de rampe dans la suite de WFS-338 | P3 |
| C1 | **Chalets de Noël** = boîtes rouges avec toits en prisme ; **sapins** = cônes empilés | Chalet savoyard / scandinave : bardage bois, débord de toit 60–80 cm, neige sur le toit ; épicéa réel | Vrais modèles CC0 dans **WFS-344** (déjà prévu par la règle, je confirme que c'est bloquant) | P1 (avec WFS-344) |
| C2 | **Neige** : blanc uni, sans relief, sans ombre bleutée ni traces de pneus | Neige : albédo 0,8–0,9, ombres bleues, scintillement, bas-côtés tassés gris | Matériau neige (normale de bruit + teinte des ombres selon le ciel) dans WFS-343 / 344 | P1 |
| C3 | **Sol des cartes de données** (Campagne, automne) : vert et orange saturés, sans variation | Pré réel : vert 0x6B7F3A environ, taches de terre et d'herbe sèche | Même mélange de matériaux selon la pente que R2 | P2 |

## Ordre recommandé (mis à jour)
1. WFS-379 terrain qui flotte (+ R7) · 2. **N1 phares brûlés** et **R1 brume** (petits, gros gain) · 3. WFS-380 mer + R6 · 4. WFS-376 bateaux ·
5. WFS-358 X5 (en cours) · 6. WFS-344 chalets et sapins + C2 neige · 7. R2 / C3 sols · 8. WFS-378 inventaire avec R3, R4, R5, V3, N2.

---

# Complément 2 — captures de la CI (décor seulement), 2026-11-09 ≈ 20:10 UTC

Source (fournie par l'agent expert-ci-cd) : Release `apercu-fusion`, run `build.yml` #965 (vert), commit `529b89e` — 29 images de course, poursuite, vie de la ville, menu.
`apercu` est vide (run #966 en cours) ; captures de nuit / réalisme du banc du matin #5 non téléchargeables (proxy) ; pas de carte de Noël.
**Je ne regarde que le décor** (pas les voitures, ni l'interface, ni la conduite).

| # | Carte | Constat (images) | Référence réelle | Proposition | Priorité |
|---|---|---|---|---|---|
| D1 | Toutes | **Vues de dessus** (`course5-depart-*-dessus`, `course5-epingle-*`) : autour de la route, **vide gris** (Ville, Corniche, Circuit) ou aplat vert (Campagne) ; l'épingle flotte dans le gris | Vu d'en haut, le terrain et le décor existent partout autour de la route | Vérifier si le terrain n'est pas chargé ou coupé par la distance de vue ; même famille que les terrains qui flottent (WFS-379) | P1 (à joindre à WFS-379) |
| D2 | Ville | **Parasols des terrasses = disques plats** rouges / blancs sur un poteau (`parcours-course-2400`, `vie-terrasse`) | Parasol de terrasse : toile à 8 baleines, mât bois ou alu, lestage, Ø 2,5–3 m | Vrais modèles dans **WFS-370** ou inventaire **WFS-378** | P1 (forme nue très visible, premier plan) |
| D3 | Ville | Immeubles du fond = **boîtes grises** sans texture ; façades proches avec fenêtres **peintes à plat** (sans profondeur) | Fenêtre réelle en retrait de 15–25 cm, appuis, volets | WFS-370 (déjà prévu) ; ajouter le retrait des fenêtres | P1 (WFS-370) |
| D4 | Corniche | Plage = **sable uni jaune-vert** ; mer cyan plate ; **scène de concert = boîte** (`vie-plage`) | Sable 0xD8C8A0 avec traces, laisse de mer ; scène avec structure en treillis | Mer : WFS-380 ; sable et scène : **WFS-371** | P1 / P2 |
| D5 | Campagne | **Grandes boîtes grises** au bout de la route et portique sur poteaux (`endurance-avant-retour-1`) ; champs vert vif uni | Bâtiments agricoles en bardage, hangar, champs cultivés en rangs | **WFS-372** + mélange de sols (R2 / C3) | P2 |
| D6 | Grand Prix | **Bâtiment des stands = boîte blanche** à bandes ; **piles de pneus = disques** rouges et blancs ; herbe d'un vert uni ; touffes posées en grille régulière | Stands vitrés avec auvent ; murs de pneus empilés et sanglés ; herbe tondue en bandes | **WFS-373** ; touffes en semis aléatoire | P2 |
| D7 | Ville nuit | Fenêtres = rectangles lumineux uniformes ; bon point : rue mouillée et pont en arc crédibles | 30–50 % de fenêtres allumées, teintes variées | Déjà N2 | P2 |
| D8 | Toutes | **Bons points** à garder : arbres d'alignement et ombres portées de la Ville, palmiers et lampadaires de la plage de nuit, tribunes avec public du circuit, bordures rouges et blanches | — | Ne pas casser en refaisant le reste | — |
