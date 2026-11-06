# Relevés manuels WFS-148 (voitures, marquage, lieux) → JSON du référentiel. Valeurs relevées dans les résumés du moteur de
# recherche (pages non ouvrables depuis la session) : statut « a_confirmer », lien de la page citée.
import json, re, sys
OUT = sys.argv[1]
NO = "page non ouverte (réseau de la session) : valeur lue dans le résumé du moteur de recherche, 2026-11-06"
def v(x):   # nombre, plage (a, b) ou None
    if x is None: return {"valeur": None}
    if isinstance(x, tuple): return {"min": x[0], "max": x[1]}
    return {"valeur": x}
def diametre(pneu):   # ETRTO : largeur / série R jante → diamètre extérieur (mm) = jante × 25,4 + 2 × largeur × série / 100
    m = re.match(r"(\d+)/(\d+)\s*Z?R\s*(\d+)", pneu)
    l, s, j = int(m.group(1)), int(m.group(2)), int(m.group(3))
    return round(j * 25.4 + 2 * l * s / 100, 1)
# id : (titre de la source, lien, L, W sans rétroviseurs, H, empattement, voie av, voie ar, pneu av, pneu ar, note)
CARS = {
 "golf": ("Volkswagen Golf 8 GTI : données techniques (Volkswagen Croatie) ; voies et pneus : auto-data.net", "https://www.volkswagen.hr/golf/golf-gti/tehnicki-podaci/gti",
          4289, 1789, (1454, 1472), 2627, (1539, 1541), (1513, 1518), "225/40 R18", "225/40 R18", "hauteur 1454 (sans barres de toit, Volkswagen) à 1472 selon la source ; voies : https://www.auto-data.net/en/compare&carId1=52152&carId2=&carId3="),
 "m4": ("BMW M4 Competition (G82) : auto-data.net ; pneus : annonce carsales.com.au", "https://www.auto-data.net/en/bmw-m4-g82-competition-3.0-510hp-m-xdrive-m-steptronic-43389",
        4794, 1887, (1393, 1394), 2857, 1617, 1605, "275/35 R19", "285/30 R20", "pneus : https://www.carsales.com.au/cars/details/2021-bmw-m4-competition-g82-auto/OAG-AD-26079341/"),
 "p911": ("Porsche 911 Carrera 4S (992) : autotijd.be ; pneus : autostw.com.au", "https://autotijd.be/en/dimensions/porsche/911/carrera-4s-450-hp-awd",
          (4519, 4535), 1852, 1300, 2450, None, None, "245/35 R20", "305/30 R21", "voies propres à la Carrera 4S non trouvées ; pneus : https://autostw.com.au/blogs/news/porsche-911-carrera-4s-tyre-size-guide-awd-wheel-specs-pcd-5x130-torque-autosport-tyreworld"),
 "gallardo": ("Lamborghini Gallardo LP 560-4 Spyder : auto-data.net, lambocars.com", "https://www.auto-data.net/en/lamborghini-gallardo-lp-560-4-spyder-5.2-v10-560hp-awd-17485",
              4345, 1900, 1184, 2560, 1632, 1597, "235/35 R19", "295/30 R19", "une source donne 1165 mm de haut (autre carrosserie probable)"),
 "p208": ("Peugeot 208 (P21, 2019-2023) : autotijd.be", "https://autotijd.be/en/dimensions/peugeot/208-2019-2023",
          4055, 1765, 1430, 2540, 1500, 1500, "195/55 R16", "195/55 R16", "pneus de la 1.2 PureTech (205/45 R17 en option)"),
 "abarth": ("Abarth 595 : fiche technique Stellantis (2016) ; voies : cararac.com ; pneus : carsguide.com.au", "https://www.media.stellantis.com/uploads/em/2016/ABARTH/Schde_tecniche/160607_Abarth_595_Technical-Specs_ENG.pdf",
            3660, 1627, 1485, 2300, 1415, 1408, "195/45 R16", "195/45 R16", "le modèle du jeu est une « Abarth 500 » : données de l'Abarth 595 (même carrosserie, à confirmer) ; pneus selon version (205/40 R17 en Competizione)"),
 "tcross": ("Volkswagen T-Cross (2019-2023) : Volkswagen UK, autotijd.be ; pneus : carsales.com.au", "https://volkswagen.co.uk/en/new/t-cross/t-cross-dimensions.html",
            4108, 1760, (1583, 1584), (2551, 2563), (1526, 1534), (1504, 1512), "205/60 R16", "205/60 R16", "empattement 2551 (Volkswagen UK) ou 2563 (CarExpert)"),
 "compass": ("Jeep Compass II (MP, restylée 2021) : autotijd.be, vehiclesizes.com, auto-data.net", "https://autotijd.be/en/dimensions/jeep/compass-2021-2025",
             (4394, 4404), 1819, (1624, 1657), 2636, (1542, 1550), 1532, None, None, "hauteur selon version (1657 : Trailhawk) ; pneus non trouvés"),
 "s3": ("Audi S3 Limousine (8V, 2013-2016) : angurten.de, encycarpedia.com, carexpert.co.nz", "https://www.angurten.de/is/abmessungen/1472-S3+Limousine",
        (4456, 4469), 1796, (1392, 1416), (2631, 2637), 1555, 1527, None, None, "deux jeux de valeurs selon la source ; voie arrière 60,1 in convertie ; pneus de la berline non confirmés"),
 "x5": ("BMW X5 (G05 LCI, 2023) xDrive40i : vehiclesizes.com", "https://www.vehiclesizes.com/cars/bmw/x5/x5-g05-lci-facelift-2023-suv-2024",
        4935, 2004, (1762, 1765), 2975, None, None, None, None, "voies et pneus de la 40i restylée non trouvés"),
 "giulia": ("Alfa Romeo Giulia Quadrifoglio : guide d'achat Alfa Romeo Australie", "https://tools.alfaromeo.com.au/vehicles/Giulia-Quadrifoglio-Buyers-Guide.pdf",
            4639, 1873, 1426, 2820, (1555, 1559), 1607, "245/35 R19", "285/30 R19", ""),
 "a45": ("Mercedes-AMG A 45 4MATIC (W176 restylée, 2015-2018) : paultan.org, auto-data.net", "https://paultan.org/research/mercedes-benz/a-amg/w176-facelift/a-45-4matic-2016/",
         4367, 1780, 1417, 2699, 1557, 1561, None, None, "le modèle du jeu est daté de 2018 (W176) ; pneus non confirmés"),
 "model3": ("Tesla Model 3 (2017-2023) : basenor.com, autotijd.be", "https://www.basenor.com/pages/model-3-legacy-dimensions",
            (4694, 4720), (1849, 1850), (1441, 1443), 2875, (1580, 1584), (1580, 1584), "235/45 R18", "235/45 R18", "longueur 4720 sur certaines versions"),
 "corsa": ("Opel Corsa F (2019-2023) : autotijd.be, cararac.com", "https://autotijd.be/en/dimensions/opel/corsa-2020-2023",
           (4060, 4061), 1765, (1433, 1435), 2538, (1489, 1501), (1488, 1500), None, None, "pneus selon version (non confirmés)"),
 "gt86": ("Toyota GT86 (2012-2020) : autotijd.be", "https://autotijd.be/en/dimensions/toyota/gt86-2017-2020",
          4240, 1775, (1285, 1320), 2570, 1520, 1540, "215/45 R17", "215/45 R17", "hauteur 1320 sur certaines versions"),
 "elantra": ("Hyundai Elantra N (CN7 restylée, 2024) : auto-data.net, conceptcarz.com, edmunds.com", "https://www.auto-data.net/en/hyundai-elantra-vii-cn7-facelift-2023-n-2.0-t-gdi-286hp-52432",
             (4675, 4710), (1825, 1826), 1415, 2720, 1584, 1579, "245/35 R19", "245/35 R19", ""),
 "v60": ("Volvo V60 Polestar (2013-2015) : Volvo (assistance, tableau des dimensions), auto-data.net", "https://www.auto-data.net/en/volvo-v60-i-facelift-2013-polestar-2.0-t6-367hp-awd-geartronic-23138",
         4635, (1865, 1899), 1484, 2776, (1578, 1588), (1575, 1585), "245/35 R20", "245/35 R20", "largeur 1865 ou 1899 (rétroviseurs rabattus) selon la source"),
 "rrsport": ("Land Rover Range Rover Sport (L461, 2023) : vehiclesizes.com, carexpert.co.nz", "https://www.vehiclesizes.com/cars/land-rover/range-rover-sport/range-rover-sport-iii-suv-2023",
             (4946, 4970), (2043, 2047), (1814, 1820), 2997, 1702, None, None, None, "largeur rétroviseurs rabattus ; voie arrière et pneus (selon version) non trouvés"),
 "mustang": ("Ford Mustang Shelby GT500 (2020) : encycarpedia.com, carsales.com.au ; pneus : tirewheelguide.com", "https://www.encycarpedia.com/ford/20-mustang-shelby-gt500-coupe",
             (4813, 4831), 1946, 1379, (2718, 2720), 1618, 1633, "305/30 R20", "315/30 R20", ""),
 "sf90": ("Ferrari SF90 XX Stradale : ferrari.com (article du record de Fiorano)", "https://ferrari.com/en-EN/articles/the-ferrari-sf90-xx-stradale-sets-a-1-17309-lap-record-at-fiorano-for-a-road-going-car",
          4850, (2000, 2014), 1225, 2650, 1683, 1674, None, None, "largeur 2014 (Ferrari) ou 2000 (concessionnaires) ; pneus de la XX non trouvés"),
}
sources, mesures = {}, []
TOL = {"type": "relative", "valeur": 0.01, "regle": "DA-realisme.md § 4 (dimensions ± 1 %)"}
for cid, (titre, lien, L, W, H, wb, va, vr, pa, pr, note) in CARS.items():
    sid = "FICHE-" + cid.upper()
    sources[sid] = {"titre": titre, "lien": lien, "acces": "non_ouvert", "annee": "2026 (consultation)", "note": (note + " ; " if note else "") + NO}
    for g, nom, x in (("longueur", "longueur hors tout", L), ("largeur", "largeur sans rétroviseurs", W), ("hauteur", "hauteur", H),
                      ("empattement", "empattement", wb), ("voie_av", "voie avant", va), ("voie_ar", "voie arrière", vr)):
        e = dict(v(x), id=f"voiture.{cid}.{g}", domaine="voitures", grandeur=nom, unite="mm", source=sid,
                 statut="a_confirmer" if x is not None else "a_sourcer", tolerance=TOL)
        if x is None: e["note"] = "valeur non trouvée dans les résultats de recherche"
        mesures.append(e)
    for g, p in (("roue_av", pa), ("roue_ar", pr)):
        if p: mesures.append({"id": f"voiture.{cid}.{g}", "domaine": "voitures", "grandeur": f"diamètre extérieur du pneu ({'avant' if g == 'roue_av' else 'arrière'}), {p}",
                              "unite": "mm", "valeur": diametre(p), "pneu": p, "source": sid, "statut": "a_confirmer",
                              "formule": "ETRTO : jante × 25,4 + 2 × largeur × série / 100", "tolerance": TOL})
        else: mesures.append({"id": f"voiture.{cid}.{g}", "domaine": "voitures", "grandeur": "diamètre extérieur du pneu", "unite": "mm", "valeur": None,
                              "source": sid, "statut": "a_sourcer", "note": "dimension des pneus non trouvée", "tolerance": TOL})
json.dump({"fichier": "voitures.json", "description": "Dimensions constructeur des 20 voitures du jeu — WFS-148 RG-01 (a)", "sources": sources, "mesures": mesures},
          open(OUT + "/voitures.json", "w", encoding="utf-8"), ensure_ascii=False, indent=1)

# ---------------------------------------------------------------- marquage routier
src = {
 "IISR-7": {"titre": "Instruction interministérielle sur la signalisation routière, 7e partie : marques sur chaussées (version consolidée du 2025-04-04)",
            "editeur": "DSR / DGITM, copie Cerema", "annee": "2025", "lien": "https://equipementsdelaroute.cerema.fr/IMG/pdf/iisr_7epartie_vc_20250404.pdf",
            "acces": "non_ouvert", "note": "texte de référence ; PDF non ouvrable depuis la session (réseau) : les valeurs ci-dessous viennent de sources secondaires et restent « à confirmer » sur ce texte"},
 "SEC-T1": {"titre": "Modulations des lignes discontinues (T1 : 3 m de trait, 13 m de période hors agglomération ; T3 : 3 m / 1,33 m) — sources secondaires",
            "editeur": "codedelaroute.io (ligne d'annonce), franco.wiki (marquage d'une route de rase campagne en France)", "annee": "2026 (consultation)",
            "lien": "https://codedelaroute.io/blog/ligne-annonce/", "acces": "non_ouvert", "note": NO},
 "GRANDLYON-PP": {"titre": "Référentiel des espaces publics : passage piéton en carrefour (bandes de 50 cm, espacement préconisé 50 cm, 80 cm au plus)",
                  "editeur": "Métropole de Lyon, Lab des espaces publics", "annee": "2017",
                  "lien": "https://labdesespacespublics.grandlyon.com/app/uploads/2024/07/2019-04-RFX-Passage-pietons-carrefour-MAJjuin2017.pdf",
                  "acces": "non_ouvert", "note": "cite l'arrêté du 15 janvier 2007 ; " + NO},
 "CODE-PP": {"titre": "Passage pour piétons : bandes de 2,5 m de long en agglomération, 4 à 6 m hors agglomération (sites de code de la route)",
             "editeur": "envoituresimone.com, ornikar.com", "annee": "2026 (consultation)",
             "lien": "https://www.envoituresimone.com/code-de-la-route/cours/autres-usagers/usagers-vulnerables/pietons/passage-pieton", "acces": "non_ouvert", "note": "source secondaire ; " + NO},
 "MTE-VOIES": {"titre": "Fiche emprise des routes : largeur des voies (routes nationales 3,50 m ; RD à 1 chaussée 3,00 m, à 2 chaussées 3,50 m)",
               "editeur": "Ministère de la Transition écologique (DDT 74)", "annee": "2026 (consultation)",
               "lien": "https://piece-jointe-carto.developpement-durable.gouv.fr/REG074B/FONCIER_SOL/N_OCCUPATION_SOL/L_EMPRISE_ROUTE_R74/Fiche1-7-1.pdf",
               "acces": "non_ouvert", "note": NO},
}
TOLM = {"type": "relative", "valeur": 0.01, "regle": "DA-realisme.md § 4 (dimensions ± 1 %)"}
mk = [
 {"id": "route.voie.route_principale", "grandeur": "largeur d'une voie, route principale hors agglomération", "unite": "m", "valeur": 3.50, "source": "MTE-VOIES", "statut": "a_confirmer", "cartes": ["campagne", "plage"]},
 {"id": "route.voie.agglomeration", "grandeur": "largeur d'une voie en agglomération", "unite": "m", "valeur": None, "source": "IISR-7", "statut": "a_sourcer", "cartes": ["ville"],
  "note": "aucune norme française d'agglomération trouvée (la charte du PO dit 3,0 m, sans source) ; le circuit (Grand Prix) relève des règles de la FIA, hors norme routière"},
 {"id": "route.t1.trait", "grandeur": "ligne discontinue T1 (axiale), longueur du trait, hors agglomération", "unite": "m", "valeur": 3.0, "source": "SEC-T1", "statut": "a_confirmer", "cartes": ["campagne", "plage"]},
 {"id": "route.t1.periode", "grandeur": "ligne discontinue T1, période trait + vide, hors agglomération", "unite": "m", "valeur": 13.0, "source": "SEC-T1", "statut": "a_confirmer", "cartes": ["campagne", "plage"]},
 {"id": "route.t1.trait_agglo", "grandeur": "ligne discontinue T1, longueur du trait, en agglomération (valeurs « divisées par deux »)", "unite": "m", "valeur": 1.5, "source": "SEC-T1", "statut": "a_confirmer", "cartes": ["ville"]},
 {"id": "route.t1.periode_agglo", "grandeur": "ligne discontinue T1, période, en agglomération", "unite": "m", "valeur": 6.5, "source": "SEC-T1", "statut": "a_confirmer", "cartes": ["ville"]},
 {"id": "route.t3.trait", "grandeur": "ligne d'annonce T3, longueur du trait", "unite": "m", "valeur": 3.0, "source": "SEC-T1", "statut": "a_confirmer"},
 {"id": "route.t3.vide", "grandeur": "ligne d'annonce T3, longueur du vide", "unite": "m", "valeur": 1.33, "source": "SEC-T1", "statut": "a_confirmer"},
 {"id": "route.ligne.largeur_axe", "grandeur": "largeur de la ligne axiale T1 (2u, module u selon la route)", "unite": "m", "valeur": None, "source": "IISR-7", "statut": "a_sourcer",
  "note": "la largeur 2u est citée, mais la valeur du module u (en cm) n'a pas pu être lue dans l'IISR"},
 {"id": "route.ligne.largeur_rive", "grandeur": "largeur de la ligne de rive", "unite": "m", "valeur": None, "source": "IISR-7", "statut": "a_sourcer", "note": "module u non lu (voir ci-dessus)"},
 {"id": "route.pieton.bande_largeur", "grandeur": "passage piéton : largeur d'une bande", "unite": "m", "valeur": 0.50, "source": "GRANDLYON-PP", "statut": "a_confirmer"},
 {"id": "route.pieton.intervalle", "grandeur": "passage piéton : intervalle entre bandes", "unite": "m", "min": 0.50, "max": 0.80, "source": "GRANDLYON-PP", "statut": "a_confirmer"},
 {"id": "route.pieton.bande_longueur_agglo", "grandeur": "passage piéton : longueur des bandes en agglomération", "unite": "m", "valeur": 2.5, "source": "CODE-PP", "statut": "a_confirmer", "cartes": ["ville"]},
 {"id": "route.pieton.bande_longueur_hors", "grandeur": "passage piéton : longueur des bandes hors agglomération", "unite": "m", "min": 4.0, "max": 6.0, "source": "CODE-PP", "statut": "a_confirmer", "cartes": ["campagne"]},
]
for e in mk: e["domaine"] = "routes"; e["tolerance"] = TOLM
json.dump({"fichier": "marquage.json", "description": "Normes de marquage et de voirie (France) — WFS-148 RG-01 (b)", "sources": src, "mesures": mk},
          open(OUT + "/marquage.json", "w", encoding="utf-8"), ensure_ascii=False, indent=1)

# ---------------------------------------------------------------- lieux fictifs des cartes
srcl = {"CONCEPTION-148": {"titre": "Choix de conception de WFS-148 : lieux fictifs cohérents avec le décor de chaque carte (aucune ville nommée)",
                           "editeur": "WaForSpeed (expert-realisme-3d), d'après RG-01 (g) de regles-de-gestion/WFS-148.md", "annee": "2026",
                           "lien": "regles-de-gestion/WFS-148.md", "acces": "ouvert",
                           "note": "valeurs proposées, à valider par le PO : ce ne sont pas des données mesurées ; seule la latitude de la Corniche (43,5° N) est donnée par RG-01"}}
lx = []
for carte, lat, lon, decor in (("plage", 43.5, 7.0, "Corniche : côte méditerranéenne (exemple de RG-01)"), ("campagne", 43.9, 5.0, "Campagne : vignobles, cyprès, mas (Provence intérieure)"),
                               ("ville", 45.0, 5.0, "Ville néon : grande ville du sud-est"), ("circuit", 44.0, 4.5, "Grand Prix : circuit du sud de la France")):
    for g, val, u in (("latitude", lat, "° N"), ("longitude", lon, "° E")):
        lx.append({"id": f"lieu.{carte}.{g}", "domaine": "lieux", "grandeur": f"{g} de la carte {carte} ({decor})", "unite": u, "valeur": val,
                   "source": "CONCEPTION-148", "statut": "conception", "tolerance": {"type": "absolue", "valeur": 0}})
    for mo, date, utc in (("jour", "2026-07-15", "12:00"), ("nuit", "2026-07-15", "21:30")):
        lx.append({"id": f"lieu.{carte}.{mo}", "domaine": "lieux", "grandeur": f"date et heure du moment « {mo} » (heure légale d'été = UTC + 2)", "unite": "UTC",
                   "valeur": f"{date}T{utc}:00", "source": "CONCEPTION-148", "statut": "conception", "tolerance": {"type": "absolue", "valeur": 0},
                   "note": "jour : 14 h heure légale ; nuit : 23 h 30 heure légale (soleil sous l'horizon)"})
json.dump({"fichier": "lieux.json", "description": "Lieux fictifs des 4 cartes : position, date et heure de chaque moment — WFS-148 RG-01 (g)", "sources": srcl, "mesures": lx},
          open(OUT + "/lieux.json", "w", encoding="utf-8"), ensure_ascii=False, indent=1)
print("relevés écrits")
