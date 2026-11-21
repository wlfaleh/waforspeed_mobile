# WFS-590 — références réelles et mesures sourcées (RG-08, RG-09)

## Photos réelles (RG-08) : non téléchargeables depuis la session

Le réseau de l'environnement n'autorise que `sketchfab.com` (voitures) et GitHub (`regles-reseau.md`, § 1). Essai du 2026-11-21 :
`commons.wikimedia.org` et `upload.wikimedia.org` répondent **403 « no rule or network setting allows this host »** (proxy de l'agent).
Aucun contournement (règle § 3). Conformément à RG-08 (« Sans site autorisé : le dire au PO … et utiliser les mesures ci-dessus avec la
source citée »), aucune photo n'est jointe ; le PO est prévenu au journal et dans la PR. Photos proposées, à télécharger par l'utilisateur
ou après ouverture de Wikimedia Commons (CC BY / CC BY-SA, licence à lire sur la page) :
- rue commerçante française, enseignes bandeau et drapeau : catégorie Commons « Shop signs in France » / « Enseignes de Paris » ;
- croix de pharmacie en drapeau : catégorie Commons « Pharmacy crosses in France » ;
- café de bord de mer avec bandeau en façade : catégorie Commons « Cafés in Provence-Alpes-Côte d'Azur ».

## Mesures du modèle et sources (RG-09)

| Objet | Valeur du modèle | Règle ou source | Statut |
|---|---|---|---|
| Bandeau à plat : saillie | 0,15 m (caisson dos au mur) | Code de l'environnement, art. R581-60 : enseigne parallèle au mur, saillie ≤ 0,25 m, sans dépasser les limites du mur | `a_confirmer` (Légifrance non ouvert depuis la session) |
| Bandeau : position | bas à 3,15 m (Ville), 3,50 m au-dessus du pied de façade (Corniche), au-dessus de la vitrine et du store, haut à 3,85-4,25 m (sous l'appui du 1er étage) | R581-60 (dans les limites du mur, pas au-dessus de l'égout du toit) ; RG-02 du ticket (bas ≈ 2,8-3,2 m) | `a_confirmer` |
| Bandeau : caisson et lettres | 0,70 × 6,5 m au plus × 0,15 m ; majuscules ≈ 0,30 m | RG-02 (caisson de 10 à 20 cm, lettres de 25 à 50 cm) | conforme à la RG |
| Drapeau : saillie | 0,79 m (bras), caisson de x = 0,16 à 0,76 m | R581-61 : enseigne perpendiculaire, saillie ≤ 1/10 de la distance entre les deux alignements de la voie, dans la limite de 2 m ; RG-03 (≤ 0,8 m) | `a_confirmer` |
| Drapeau : hauteur | bas à 3,20 m (≥ 2,50 m), au-dessus du store | règlements de voirie communaux (saillies sur le domaine public : 2,50 m au moins au-dessus du trottoir) ; RG-03 | `a_confirmer` |
| Drapeau : caisson | 0,60 × 0,60 × 0,12 m, profilé 35 mm | RG-03 (≈ 0,6 × 0,6 m, 8 à 15 cm d'épaisseur) | conforme à la RG |
| Potence | tube Ø 33,7 mm (bras), Ø 24 mm (tirant), platine 160 × 400 × 12 mm, 4 vis H M8 (tête de 13 mm) | diamètres de tubes d'acier normalisés EN 10255 (33,7 mm) ; vis ISO 4017 M8 | `a_confirmer` |
| Croix de pharmacie | croix grecque de 0,62 m, bras de 0,20 m, 0,10 m d'épaisseur, verte, allumée la nuit | tailles courantes des croix d'officine (0,6 à 1 m) ; RG-06 (rien d'allumé de jour) | `a_confirmer` |

Rien de ces objets n'est cuit dans un paquet `.wfcarte` (seules les villas de la Corniche le sont : `plage::poserVillasKit`) : aucune
publication de paquet n'est nécessaire.
