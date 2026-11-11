// WaForSpeed — WFS-205 (lot 1 de WFS-155, épic E9) : bibliothèque des revêtements de route réels (scans CC0 calibrés, KTX2).
//
// Chaque revêtement = 3 textures KTX2 (references/reel/routes/outils/preparer.py → tools/textures/convertir.py) dans
// assets/textures/route/<id>/ : route_<id>-couleur (albédo sRGB recalibré sur le référentiel : RG-02), route_<id>-normale,
// route_<id>-orm (occlusion, rugosité, métal).
// Le matériau de la chaussée (MAT_ROAD) ou du trottoir (MAT_WALK) reçoit ces cartes ; shaders/route.glsl les lit sans
// répétition visible (RG-03). Repli (RG-04) : une carte absente ou illisible → matériau sans carte → surface procédurale de
// WFS-149 (recalibrée), jamais de plantage.
// Qualité (RG-01, décision du PO 2026-11-07 01:40:18, budget de l'APK) : couleur et normale 1K en Ultra / Haute, 512 en Moyenne,
// 256 en Basse (ORM : moitié) : niveaux de mipmaps sautés au chargement. ASTC 4x4 (6x6 sous le seuil PSNR : bruit fin des scans).
// Repli RGBA8 (GPU sans ASTC, banc PC ; dans l'APK : + 8,6 Mo) : fichier déjà réduit à 512 (ORM 256) ; pas d'ETC2 (refusé sous le seuil).
// Textures partagées par tous les matériaux du même revêtement pendant une partie : un seul envoi au GPU.
#pragma once
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>
#include "renderer.h"

namespace wfs::route {

enum class Revetement : int { EnrobeNeuf, EnrobeUse, EnrobeCampagne, BetonCircuit, BetonTrottoir, PavesGranit, Gravier, N };
constexpr int NB_REVETEMENTS = (int)Revetement::N;

// plages du réel (références/reel/routes/revetements.json) : albédo moyen sRGB linéaire (M-01) et rugosité sèche (M-02)
struct InfoRevetement {
    const char* id;            // dossier et préfixe des fichiers (assets/textures/route/<id>/<id>-couleur…)
    const char* nom;           // nom en clair (journal, rapport)
    float albedoMin, albedoMax;
    float rugositeMin, rugositeMax;
    float metres;              // côté du scan en mètres (UV de la route en mètres : une texture = « metres » m)
};
const InfoRevetement& info(Revetement r);

// côté maximal chargé (texels) selon la qualité (qualite::Niveau : 0 Basse … 3 Ultra) ; ORM : moitié
uint32_t coteMax(int qualite, bool orm);
// niveaux à sauter pour qu'une texture de « cote » texels ne dépasse pas « maxi » (jamais sous 4 texels)
int niveauxSautes(uint32_t cote, uint32_t maxi);

// ---------------------------------------------------------------- WFS-206 (lot 2) : revêtement propre à chaque carte (RG-02)
// Ville : enrobé usé, raccords, bordures de granit ; Campagne : enrobé grenu, bords effrités ; Corniche (plage) : enrobé usé
// clair, sable en rive ; Grand Prix (circuit) : béton bitumineux de course sombre. La carte va au shader avec le mode de lecture :
// pc.params.z = mode + 4 × carte (route.glsl : modeLecture, carteRoute).
enum class Carte : int { Ville, Campagne, Corniche, Circuit };
Carte carteDe(const char* mapId);          // « ville », « campagne », « plage », « circuit » ; autre : Ville
Revetement chaussee(Carte c);              // revêtement de la chaussée de la carte
Revetement chaussee();                     // … de la carte de la partie (preparer)
// valeur de Material::cutoff lue par route.glsl : mode + 4 × carte + 16 × neige (WFS-346 : neige au sol de la carte, 0 à 1, par huitièmes)
// WFS-392 : + 256 × longueur du tour en segments quand la carte a des abris (0 : aucun abri, valeur inchangée)
float codeMateriau(int qualite, Carte c, float neige = 0, int tourSeg = 0);
constexpr int NEIGE_PAS = 8;               // WFS-346 : couverture de neige transmise par huitièmes (route.glsl : neigeRoute)
// WFS-392 (RG-01, RG-03) : abris de la carte (sections « tunnel » du tracé du paquet) sous lesquels la chaussée et le trottoir restent
// secs. Le matériau de la route n'a que 3 nombres libres (sa couleur, que route.glsl ne lit pas) : un abri par nombre, entier exact en
// flottant = début (segments) × 1024 + longueur (segments) ; 0 : case vide. Un abri de plus de ABRI_LONG_MAX segments (1,2 km) prend
// plusieurs cases qui se recouvrent de ABRI_RECOUVRE segments (24 m : aucune bande de neige au raccord, le fondu d'entrée fait 8 m).
// Abris qui se touchent : réunis. Au-delà de ABRIS_MAX cases ou de ABRI_DEBUT_MAX segments : abri non transmis (compté dans perdus).
constexpr double SEG_ROUTE_M = 1.2;          // = SEG_M (src/game/track.h) et SEG_ROUTE_M de route.glsl (le test les compare)
constexpr int ABRIS_MAX = 3, ABRI_LONG_MAX = 1023, ABRI_DEBUT_MAX = 16383, ABRI_RECOUVRE = 20, TOUR_SEG_MAX = 65535;
struct Abris { float code[ABRIS_MAX] = {0, 0, 0}; int tourSeg = 0, tunnels = 0, perdus = 0; };
// tunnels : paires [début, fin) en segments, triées, dans le tour ; tourSeg : longueur du tour (segments)
Abris coderAbris(const std::vector<std::pair<int64_t, int64_t>>& tunnels, int64_t tourSeg);
// part de neige gardée (1 dehors, 0 sous l'abri, fondu de 9 m à chaque bouche) à y mètres le long de la route : même calcul que
// abriRoute de route.glsl (tests/test_cartes_v2_abris.cpp)
float abriRoute(const Abris& a, double yM);

// M-03 (RG-01) : bandes de roulement : largeur 0,8-1,0 m (± 5 % : voie des voitures) ; écartement des deux bandes d'une voie =
// voie moyenne des voitures du jeu (references/reel/voitures.json : 23 voies relevées, 1 572 mm) ; mêmes valeurs dans route.glsl
// (BANDE_ROULEMENT_M, ECARTEMENT_ROUES_M : test_fidelite_routes vérifie qu'elles sont identiques)
constexpr float BANDE_ROULEMENT_M = 0.9f, ECARTEMENT_ROUES_M = 1.572f;
// M-04 : bordure de trottoir (Ville : granit) : 15 cm de haut (CURB_H de game.cpp et ville.cpp), largeur du bloc de granit 0,30 m
constexpr float BORDURE_H_M = 0.15f, BORDURE_LARGEUR_M = 0.30f;

// qualité du chargement (qualite::Niveau : 0 Basse … 3 Ultra), lue au début de chaque partie (Game::init) : taille chargée et
// mode de lecture du shader (Ultra / Haute : pavage hexagonal, Moyenne : deux échelles, Basse : une lecture) ; WFS-206 : carte
// de la partie (mapId)
void preparer(int qualite, const char* mapId = nullptr);
// matériau avec les cartes du revêtement (base : rugosité, drapeaux MAT_ROAD / MAT_WALK). Cartes absentes : matériau sans
// carte (repli procédural). Le matériau se rend par r.destroy comme les autres ; base.cutoff reçoit le mode de lecture
// (la chaussée n'a pas de MAT_MASK : shaders/route.glsl lit pc.params.z).
Material creerMateriau(Renderer& r, Revetement rev, const Material& base);
// textures de route rendues (fin de partie : Game::shutdown, après la destruction des matériaux qui les lisent)
void liberer(Renderer& r);

// mémoire GPU des textures de route chargées (octets) et variante (« astc », « etc2 », « rgba8 », « absente »)
size_t octetsGpu();
const char* variante(Revetement r);

} // namespace wfs::route
