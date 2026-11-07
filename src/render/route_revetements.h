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

// qualité du chargement (qualite::Niveau : 0 Basse … 3 Ultra), lue au début de chaque partie (Game::init) : taille chargée et
// mode de lecture du shader (Ultra / Haute : pavage hexagonal, Moyenne : deux échelles, Basse : une lecture)
void preparer(int qualite);
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
