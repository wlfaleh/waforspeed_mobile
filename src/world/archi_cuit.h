#pragma once
// WaForSpeed — WFS-416 (épics E14 + E15) : bâtiments du kit d'architecture CUITS dans le paquet de la carte.
//
// Demande de l'utilisateur (2026-11-10) : la CI construit tous les bâtiments, fenêtres comprises, dans leurs 3 niveaux de
// détail, et les range dans le « .wfcarte » ; le jeu ne fait plus que lire et afficher (aucune construction en course).
// Ce module, sans Vulkan (testé par tests/test_batiments_cuits.cpp), fait la cuisson et la lecture :
//   - cuire() appelle le code existant du kit (archi_geo, archi_ouvertures, archi_details : rien n'est réécrit) pour chaque
//     bâtiment posé, aux 3 niveaux, et regroupe la géométrie par morceau de route (48 segments ≈ 58 m) et par matériau ;
//       niveau 0 (près, ≤ 20 m)  : menuiseries 3D de WFS-402 (Baies::Reelles) ;
//       niveau 1 (jusqu'à 300 m) : trous, tableaux et vitres en volume, menuiserie en IMAGE AVEC RELIEF (Baies::Image) ;
//       niveau 2 (au-delà)       : volume seul, baies peintes à plat ;
//   - RG-05, niveau 0 : le contenu d'une baie (tableaux, menuiserie, volets, appui, balcon) est cuit UNE fois par modèle
//     (« Modele », repère de la baie) ; chaque baie identique n'est plus qu'une « Instance » (modèle, bâtiment, place sur la
//     façade). Sans cela la Ville pèserait 430 Mo (4,8 millions de triangles au niveau 0) ; le jeu RECOPIE ces modèles tout
//     prêts dans des tampons réservés au chargement (developper), sans rien construire ;
//   - RG-04, RG-05, niveau 1 : l'image avec relief de chaque modèle (4 fenêtres et 4 portes par style, vitrine, porte de
//     garage) est dessinée UNE fois, à partir de la vraie menuiserie 3D vue de face (couleur + découpe du vitrage, relief =
//     carte de normales tirée des profondeurs), et rangée dans un atlas ; toutes les baies du modèle la réutilisent ;
//   - ecrire() / lire() : bloc « batiments » du paquet (petit-boutiste, versionné, toutes les tailles et tous les indices
//     vérifiés : un bloc abîmé est refusé et le kit construit par le code reprend la main, RG-06). lire() ne copie rien :
//     le bloc reste en mémoire une seule fois (Lu::octets) et les lots sont des vues dedans.
// Le bloc est le même pour les 3 formats de texture du paquet (atlas en RGBA8).
#include <array>
#include <bit>
#include <cstdint>
#include <memory>
#include <vector>
#include "archi_geo.h"

namespace wfs::archi::cuit {

static_assert(std::endian::native == std::endian::little, "bloc des bâtiments : sommets et indices lus tels quels (petit-boutiste)");

constexpr uint32_t VERSION_BLOC = 1;
constexpr const char* NOM_BLOC = "batiments";
constexpr int64_t MORCEAU = 48;   // segments de route par morceau (≈ 58 m)
constexpr int NIVEAUX = 3;

// repère d'une façade dans le monde : o = coin bas gauche au sol, x le long de la façade, z vers la rue (unitaires, horizontaux)
struct Repere { vec3 o, x, z; };
// un bâtiment posé par la carte (archi::carte::poser)
struct Bati { int64_t seg; vec3 o, x, z; float profondeur; Plan plan; };

// emplacements de maillage d'un morceau : un par matériau (murs par style, toits par type, puis parties communes)
constexpr int NS = (int)Style::Nombre, NT = 3;
enum Slot : int { S_MUR = 0, S_TOIT = NS, S_ENCADREMENT = NS + NT, S_VITRE, S_MENUISERIE, S_METAL, S_VOLET, S_FIN, S_CHASSIS, S_IMAGE, NB_SLOTS };
int slot(int part, const Plan& p);

// couleurs des menuiseries (sRGB linéaire), partagées par les matériaux du jeu (archi_carte.cpp) et par l'image cuite
inline constexpr Albedo COUL_MENUISERIE{"bois", 0.18f, 0.12f, 0.07f};                // menuiserie bois (conception, provisoire)
inline constexpr Albedo COUL_VOLET{"volet", 0.09f, 0.2f, 0.13f};                     // volets peints (conception, provisoire)
inline constexpr Albedo COUL_CHASSIS{"peinture blanc cassé", 0.70f, 0.68f, 0.62f};   // WFS-402 : bois peint (RAL 9001, albédo ≈ 0,7)
inline constexpr float K_METAL = 0.3f;                                               // aluminium laqué sombre : ALB_ALUMINIUM × 0,3

// sommet prêt pour la carte graphique : mêmes 36 octets que wfs::Vertex (vérifié dans archi_carte.cpp)
struct Sommet { float px, py, pz, nx, ny, nz, u, v; int8_t tx = 0, ty = 0, tz = 0, tw = 0; };
static_assert(sizeof(Sommet) == 36, "Sommet cuit : 36 octets, comme Vertex");
struct Lot { std::vector<Sommet> v; std::vector<uint32_t> i; };

// atlas des images de menuiserie : tuiles de TUILE_L × TUILE_H pixels, COLONNES par rangée, première rangée = v 0
constexpr uint32_t TUILE_L = 128, TUILE_H = 256, COLONNES = 8;

// ---------------------------------------------------------------- côté cuisson (données possédées)
struct Modele { Lot lot[NB_SLOTS]; };   // contenu d'une baie au niveau 0, repère de la baie (x depuis son axe, y depuis son seuil, z sortant)
struct Instance { uint32_t modele = 0, bati = 0; float dx = 0, dy = 0; };   // place de la baie sur la façade du bâtiment (m)
struct Morceau {
    int64_t c = 0;
    vec3 centre{}; float rayon = 0;   // sphère englobante (tri de vue de la carte graphique)
    Lot lot[NIVEAUX][NB_SLOTS];       // niveau 0 : gros œuvre et vitres seulement ; le reste des baies est dans « instances »
    std::vector<Instance> instances;
};
struct Atlas {
    uint32_t l = 0, h = 0;
    std::vector<int> tuiles;          // numéros de tuile (archi::tuileImage) rangés dans l'atlas, croissants
    std::vector<uint8_t> couleur;     // RGBA8 sRGB ; alpha = 0 dans le vitrage (la vitre et sa pièce se voient derrière)
    std::vector<uint8_t> relief;      // RGBA8 linéaire : carte de normales (repère de la façade : x, y, z sortant), alpha = hauteur
};
struct Decor {
    uint64_t signature = 0;
    std::vector<Repere> batis;
    std::vector<Modele> modeles;
    std::vector<Morceau> morceaux;    // triés par c croissant
    Atlas atlas;
};

// empreinte des bâtiments posés (nombres entiers seulement : segment, style, graine, étages, travées, baies) : la même sur le
// PC qui cuit et sur le téléphone qui lit ; si le kit ou le plan de la carte change, le bloc est refusé (kit de secours)
uint64_t signature(const std::vector<Bati>& bats);

// bâtiment b au niveau lod, passé dans le repère du monde, ajouté aux lots d'un morceau (NB_SLOTS lots) ; commun au kit
// de secours (mode Interrupteur) et à la cuisson. premierMur (facultatif) : indice du premier sommet de ses murs dans son lot
void ajouter(const Bati& b, int lod, Baies mode, Lot* lots, size_t* premierMur = nullptr);

// RG-02 : tous les bâtiments, 3 niveaux, par morceau ; déterministe (deux appels → mêmes octets après ecrire)
Decor cuire(const std::vector<Bati>& bats);
std::vector<uint8_t> ecrire(const Decor& d);

// ---------------------------------------------------------------- côté jeu (bloc lu, sans copie)
struct Vue { size_t pos = 0; uint32_t nv = 0, ni = 0; };   // un lot dans le bloc : nv sommets à pos, puis ni indices
struct MorceauLu {
    int64_t c = 0;
    vec3 centre{}; float rayon = 0;
    Vue lot[NIVEAUX][NB_SLOTS];
    size_t premiereInstance = 0, nbInstances = 0;   // dans Lu::instances
    uint32_t nv0[NB_SLOTS]{}, ni0[NB_SLOTS]{};      // niveau 0 développé (gros œuvre + instances) : tailles par emplacement
};
struct Lu {
    std::shared_ptr<const std::vector<uint8_t>> octets;
    uint64_t signature = 0;
    uint32_t atlasL = 0, atlasH = 0;
    size_t posCouleur = 0, posRelief = 0;   // RGBA8, atlasL × atlasH × 4 octets chacun
    std::vector<int> tuiles;
    std::vector<Repere> batis;
    std::vector<std::array<Vue, NB_SLOTS>> modeles;
    std::vector<Instance> instances;
    std::vector<MorceauLu> morceaux;
    uint32_t nvMax[NB_SLOTS]{}, niMax[NB_SLOTS]{};   // plus grand niveau 0 développé, par emplacement (taille des tampons réservés)

    // recopie un lot : nv sommets (36 octets chacun) vers sommets, ni indices (+ base) vers i ; aucune allocation
    void copier(const Vue& l, void* sommets, uint32_t* i, uint32_t base) const;   // sommets ou i nul : partie sautée
    // RG-05 : recopie le modèle d'une instance, emplacement s, placé dans le monde ; rend le lot recopié (tailles)
    Vue developper(const Instance& in, int s, void* sommets, uint32_t* i, uint32_t base) const;
    size_t triangles(int niveau) const;   // niveau 0 : développé
};
// lit un bloc (tailles, bornes et indices vérifiés) ; faux et raison (texte fixe, en français) s'il est refusé
bool lire(std::shared_ptr<const std::vector<uint8_t>> o, uint64_t signatureAttendue, Lu& d, const char** raison = nullptr);

}  // namespace wfs::archi::cuit
