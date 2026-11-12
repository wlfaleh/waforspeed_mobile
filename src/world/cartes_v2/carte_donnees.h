// WaForSpeed — cartes 100 % dynamiques (WFS-343, épic E14) : une carte entièrement décrite par les données de son paquet.
// Principe de l'utilisateur (2026-11-09) : rien de propre à une carte dans le code ; le moteur générique lit et joue les données.
// Trois blocs texte du paquet (.wfcarte, WFS-301), une instruction par ligne, « # » pour un commentaire :
//   trace : nom, description, quartiers, graine, bord, trafic, distance des checkpoints, vignette du choix du terrain, routes
//           (addRoad du jeu web : entrée, tenue, sortie, courbure, montée), sections typées (autoroute, tunnel, pont, virage…)
//   chocs : obstacles solides (boîtes, cylindres) le long de la route, dessinés et pris par les chocs du joueur (chocs_decor.h)
//   decor : ciel et brouillard, bandes du sol, herbe des bas-côtés, maisons, arbres, lampadaires, lumières (guirlandes), répétitions ;
//           WFS-346 (format 2) : saison : adhérence des surfaces, neige sur la chaussée, flocons qui tombent (struct Saison)
//           WFS-392 (format 3) : éclairage des tunnels (struct EclairageTunnel)
// Les arbres sont posés sur le sol des bandes ; les couleurs (sol, murs, toits, obstacles) vont dans les cases libres de la palette
// commune des cartes (au plus PAL_W − C_NB couleurs différentes, au-delà : couleur la plus proche).
// Un bloc faux → la carte n'est pas enregistrée (absente du choix du terrain), une ligne du journal dit pourquoi ; jamais de plantage.
// Le départ est le début du tracé (1re route) ; les checkpoints suivent la distance « checkpoint_m » de la carte (défaut : configgame).
// Sources des cartes de test : paquets/sources/<carte>/{trace,chocs,decor}.txt, cuites par cartes_v2::cuire (banc).
#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "../../core/carte_cuite.h"
#include "../../game/track.h"
#include "../../game/trace_cartes.h"

namespace wfs {
namespace cartes_v2 {

constexpr int FORMAT_DONNEES = 3;   // version des blocs texte ; un bloc plus récent est refusé (carte absente). 2 : saison (WFS-346) ; 3 : éclairage des tunnels (WFS-392)

// WFS-346 (RG-02, RG-03) : saison de la carte, lue du bloc decor ; valeurs par défaut = carte sèche sans neige (cartes du code, inchangées).
// Valeurs réelles et sources : references/reel/cartes_v2/neige.md.
//   adherence chaussee bas_cote      μ des surfaces (0,05 à 1,2) : sec 1,0 ; neige tassée ≈ 0,25 ; herbe sèche 0,6 (MU_HERBE du moteur)
//   neige_chaussee couverture        part de la chaussée couverte de neige tassée (0 à 1) ; traces de roues plus sombres
//   flocons intensite chute taille   chute de neige : intensité (0 à 1 : faible à forte), vitesse de chute (0,3 à 3 m/s), flocon (1 à 10 mm)
struct Saison {
    float muChaussee = 1.0f, muBasCote = 0.6f;
    float neigeChaussee = 0;
    float flocons = 0, chuteMs = 1.2f, floconMm = 3.5f;
};
// saison de la partie en cours (thread principal) : posée au début de Game::init par preparerSaison(carte), lue par la conduite (μ),
// la route (neige au sol) et la météo (flocons). Carte du code ou inconnue : Saison{} (sec, sans neige).
inline Saison& saisonPartie() { static Saison s; return s; }
void preparerSaison(const char* carte);

// WFS-392 (RG-02, RG-03) : éclairage des tunnels de la carte (sections « tunnel » du tracé), lu du bloc decor ; sans la ligne : valeurs
// par défaut ci-dessous. Valeurs réelles et sources : references/reel/cartes_v2/tunnel.md.
//   tunnel_eclairage type pas_m pas_seuil_m seuil_m
//     type        : sodium (défaut : lampes au sodium haute pression, lumière orangée ≈ 2000 K), led (blanc neutre 4000 K), aucun
//     pas_m       : écart entre deux luminaires d'une file en section courante (6 à 25 m ; défaut 12 m)
//     pas_seuil_m : écart dans la zone de seuil, à chaque bouche (renforcement de jour ; 3 à 25 m, ≤ pas_m ; défaut 6 m)
//     seuil_m     : longueur de la zone de seuil à chaque bouche (0 à 200 m ; défaut 60 m ; jamais plus du tiers du tunnel)
struct EclairageTunnel {
    enum Type { Aucun, Sodium, Led } type = Sodium;
    float pasM = 12, pasSeuilM = 6, seuilM = 60;
};
// segments [debut, fin) du tunnel qui portent une paire de luminaires (une par file), du début vers la fin : pas du seuil près de
// chaque bouche, pas courant au milieu, 1er luminaire à un demi-pas de la bouche ; vide si type Aucun (pure, testée)
std::vector<int64_t> luminairesTunnel(const EclairageTunnel& e, int64_t debut, int64_t fin);

struct Route { int entree = 0, tenue = 0, sortie = 0; double courbe = 0, montee = 0; };
// objet du décor ou obstacle ; seg : segment du tour, lat : m (+ à droite), couleurs en 0xRRGGBB
struct Objet {
    enum Type { Maison, Sapin, Feuillu, Cypres, Lampe, Lumiere, Boite, Cylindre } type = Maison;
    int64_t seg = 0; float along = 0, lat = 0, angle = 0;
    float largeur = 0, profondeur = 0, hauteur = 0, rayon = 0;   // maison, boîte (largeur en travers), cylindre, lumière (rayon)
    uint32_t couleur = 0xffffff, couleur2 = 0x803020;          // murs / toit (maison) ; lumière : couleur émise
    int genre = 0;                                              // chocs::Genre (boîte, cylindre)
};
struct Bande { float a = 0, b = 0, ya = 0, yb = 0; uint32_t couleur = 0; };

struct DonneesCarte {
    std::string id, nom, description;
    std::vector<std::string> quartiers;
    bool cache = false;                     // carte de test : absente du choix du terrain (sauf banc)
    uint32_t graine = 1;
    double bord = 1.6, checkpointM = 0;     // 0 : distance de configgame.json
    int traficN = 20; double traficMin = 0.3, traficMax = 0.55;
    uint32_t vignetteNuit = 0x14151c, vignetteJour = 0x55565c, accent = 0x00e5ff;
    std::vector<Route> routes;
    std::vector<trace::Section> sections;
    // décor
    uint32_t brouillardJour = 0xd3e0d0, horizonNuit = 0x1b2236, brouillardNuit = 0x0d1218;
    float lointainJour = 820, lointainNuit = 540;
    std::vector<Bande> sol;                 // bandes de chaque côté (m depuis l'axe), au plus 6
    bool herbe = true;                      // touffes d'herbe des bas-côtés (« herbe 0 » : sol enneigé, parking…)
    Saison saison;                          // WFS-346 : adhérence, neige au sol, flocons
    int climatSol = 0;                      // WFS-383 : « sol mediterraneen|tempere|automne » (sols::Climat : 1, 2, 3) ; 0 : sans (bandes unies)
    EclairageTunnel eclairageTunnel;        // WFS-392 : luminaires des tunnels
    std::vector<Objet> objets, obstacles;
    int64_t segments() const;               // longueur du tour (segments) d'après les routes
};

// lecture des blocs (pures, testées par tests/test_cartes_v2_donnees.cpp) ; faux et raison en français si le bloc est faux
bool lireTrace(const std::string& texte, DonneesCarte& d, std::string& erreur);
bool lireChocs(const std::string& texte, DonneesCarte& d, std::string& erreur);
bool lireDecor(const std::string& texte, DonneesCarte& d, std::string& erreur);

// ---- registre des cartes lues dans les paquets
// enregistre une carte (blocs lus) ; remplace une carte de même identifiant ; les 4 cartes du code ne sont jamais remplacées
bool enregistrer(DonneesCarte d, std::string& erreur);
// données d'une carte (lecture seule) ; nullptr : carte du code ou inconnue. Un nouvel enregistrement remplace les données sans toucher
// à une copie déjà prise (le décor de la carte en cours garde la sienne)
std::shared_ptr<const DonneesCarte> donnees(const std::string& id);
std::vector<std::string> cartesDonnees(bool avecCachees);   // ordre d'enregistrement
void oublier();                                         // banc, tests
// paquets du dossier des paquets (<carte>_<format>.wfcarte) d'une carte qui n'est pas dans le code : vérifiés (CRC, empreinte des
// données), blocs lus, cartes enregistrées ; un paquet inchangé (fichier, taille, date) n'est pas relu ; renvoie le nombre de cartes
// de paquets disponibles ; une ligne au journal par paquet refusé
int decouvrir();
// empreinte attendue d'une carte de données : identifiant, format des blocs, VERSION_GENERATION_CARTE (pas de graine du code)
uint64_t empreinteDonnees(const std::string& id);
// paquet d'une carte de données (cuisson, banc) depuis ses sources texte ; vide si les sources manquent ou sont fausses
bool sourcesCarte(const std::string& racine, const std::string& id, carte::FormatTexture f, carte::Paquet& p, std::string& erreur);

} // namespace cartes_v2
} // namespace wfs
