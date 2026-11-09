// WaForSpeed — cartes 100 % dynamiques (WFS-343, épic E14) : une carte entièrement décrite par les données de son paquet.
// Principe de l'utilisateur (2026-11-09) : rien de propre à une carte dans le code ; le moteur générique lit et joue les données.
// Trois blocs texte du paquet (.wfcarte, WFS-301), une instruction par ligne, « # » pour un commentaire :
//   trace : nom, description, quartiers, graine, bord, trafic, distance des checkpoints, vignette du choix du terrain, routes
//           (addRoad du jeu web : entrée, tenue, sortie, courbure, montée), sections typées (autoroute, tunnel, pont, virage…)
//   chocs : obstacles solides (boîtes, cylindres) le long de la route, dessinés et pris par les chocs du joueur (chocs_decor.h)
//   decor : ciel et brouillard, bandes du sol, herbe des bas-côtés, maisons, arbres, lampadaires, lumières (guirlandes), répétitions
// Les arbres sont posés sur le sol des bandes ; les couleurs (sol, murs, toits, obstacles) vont dans les cases libres de la palette
// commune des cartes (au plus PAL_W − C_NB couleurs différentes, au-delà : couleur la plus proche).
// Un bloc faux → la carte n'est pas enregistrée (absente du choix du terrain), une ligne du journal dit pourquoi ; jamais de plantage.
// Le départ est le début du tracé (1re route) ; les checkpoints suivent la distance « checkpoint_m » de la carte (défaut : configgame).
// Sources des cartes de test : paquets/sources/<carte>/{trace,chocs,decor}.txt, cuites par cartes_v2::cuire (banc).
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "../../core/carte_cuite.h"
#include "../../game/track.h"
#include "../../game/trace_cartes.h"

namespace wfs {
namespace cartes_v2 {

constexpr int FORMAT_DONNEES = 1;   // version des blocs texte ; un bloc plus récent est refusé (carte absente)

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
const DonneesCarte* donnees(const std::string& id);   // nullptr : carte du code ou inconnue
std::vector<std::string> cartesDonnees(bool avecCachees);   // ordre d'enregistrement
void oublier();                                         // banc, tests
// paquets du dossier des paquets (<carte>_<format>.wfcarte) d'une carte qui n'est pas dans le code : vérifiés (CRC, empreinte des
// données), blocs lus, cartes enregistrées ; renvoie le nombre de cartes enregistrées ; une ligne au journal par paquet refusé
int decouvrir();
// empreinte attendue d'une carte de données : identifiant, format des blocs, VERSION_GENERATION_CARTE (pas de graine du code)
uint64_t empreinteDonnees(const std::string& id);
// paquet d'une carte de données (cuisson, banc) depuis ses sources texte ; vide si les sources manquent ou sont fausses
bool sourcesCarte(const std::string& racine, const std::string& id, carte::FormatTexture f, carte::Paquet& p, std::string& erreur);

} // namespace cartes_v2
} // namespace wfs
