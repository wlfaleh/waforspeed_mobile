// WaForSpeed — WFS-488 (épic E18, domaine E10 Véhicules) : couleur secondaire d'une voiture (toit et coques de rétroviseurs).
//
// Données : assets/cars/<id>.pt2, écrit par tools/peinture2_voitures.mjs (chaîne de conversion) à côté du .wfm, qui ne change pas :
// pour chaque maillage peint, le bit de chaque triangle du pavillon ou d'une coque de rétroviseur (îlots entiers : vrais joints).
// Au chargement (marquer, appelée par loadCarModel juste avant l'envoi au GPU) :
//   - les sommets de ces triangles reçoivent une normale de longueur MARQUE (au lieu de 1) : même maillage, même matériau, même
//     tampon, donc AUCUN appel de dessin en plus (RG-05) ; le shader normalise toujours la normale avant l'éclairage ;
//   - un sommet partagé avec un triangle d'une seule couleur (bord d'une zone, niveau de détail .lod, version Basse) est dédoublé
//     (même position, même normale, même tangente) : la limite tombe exactement sur l'arête, sans dégradé ni trou (RG-02) ;
//   - triangles des versions allégées (.lod, Basse) : en couleur 2 seulement si leurs 3 sommets sont du toit ou d'un rétro.
// Au dessin (Renderer::peinture2ProchaineVoiture) : la couleur 2 voyage dans le 4e nombre de la couleur des pièces peintes
// (pc.color.w), sans constante de poussée en plus : 1 (ou toute valeur ≥ 0 : alpha d'un matériau) = une seule couleur (aspect
// d'avant, RG-04 et RG-06), sinon −(0xRRGGBB + 1), entier exact en flottant (≤ 2²⁴). Lu par couleurPeinture() de shaders/vehicule.glsl.
// Empreinte du maillage différente (modèle reconverti ou retouché sans relancer l'outil) : .pt2 ignoré, une seule couleur.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "renderer.h"

namespace wfs::vehicule_peinture2 {

constexpr float MARQUE = 4.f;     // longueur de la normale des sommets en couleur 2 (seuil du shader : 2, voir vehicule.glsl)
constexpr float UNE_COULEUR = 1.f;   // pc.color.w : pas de couleur 2

// code de la couleur 2 pour pc.color.w : −(0xRRGGBB + 1) (sRGB 8 bits, comme PEINTURES[].hex ; noir pur = −1) ; exact en float
inline float code(unsigned hexSrgb) { return -(float)((hexSrgb & 0xFFFFFFu) + 1u); }
// relecture du code (même règle que le shader) : vrai si une couleur 2 est posée (w < −0,5), hex rendu
inline bool decoder(float w, unsigned& hex) { if (!(w < -0.5f)) return false; hex = (unsigned)(-w) - 1u; return true; }   // entier exact : pas de « − 0,5 » (perdu au-delà de 2²³)

// finition de la carrosserie pour une pièce non peinte repeinte (pc.params.z) : 4 + métal × 100 (entier) + rugosité (bornée à 0,99)
inline float codeFinition(float metal, float rugosite) {
    const float m = metal < 0 ? 0 : (metal > 1 ? 1 : metal), r = rugosite < 0 ? 0 : (rugosite > 0.99f ? 0.99f : rugosite);
    return 4.f + (float)(int)(m * 100.f + 0.5f) + r;
}

// FNV-1a 32 bits des indices puis des positions (12 premiers octets de chaque sommet de 32 octets) : même calcul que l'outil
uint32_t empreinte(const uint8_t* sommets, uint32_t nSommets, const uint8_t* indices, uint32_t nIndices);

// contenu d'un .pt2 : un bloc par maillage peint repéré
struct Bloc { uint32_t maillage = 0, triangles = 0, empreinte = 0, toit = 0, retro = 0; std::vector<uint8_t> bits; };
bool lire(const uint8_t* p, size_t n, std::vector<Bloc>& blocs);   // faux si format inconnu ou tronqué

// maillage du .wfm tel que lu (ordre du fichier) : premier sommet dans le tampon de la voiture, octets bruts pour l'empreinte
struct MaillageWfm { uint32_t premierSommet = 0, nSommets = 0, nIndices = 0, materiau = 0; bool peint = false; const uint8_t* sommets = nullptr; const uint8_t* indices = nullptr; };

// pièces d'origine non peintes (toit noir ou carbone pris dans les garnitures : Corsa, M4, Range Rover Sport, Brezza) : leurs sommets
// marqués deviennent de la peinture en couleur 2 quand une couleur 2 est posée (Material::peinture2Piece) ; le reste du matériau et
// la voiture sans couleur 2 gardent l'aspect d'origine ; matériaux opaques seulement (noterMateriaux)
struct Bilan { uint32_t triangles = 0, toit = 0, retro = 0, dedoubles = 0, lod = 0; bool lu = false; std::vector<uint32_t> piecesNonPeintes; };
// marque les sommets du toit et des rétros (normale × MARQUE), dédouble ceux du bord ; listes = toutes les listes de pièces du
// modèle (complète, allégées, Basse), indices relatifs à vertexOffset. Sans .pt2 valable : rien ne change.
Bilan marquer(const std::vector<Bloc>& blocs, const std::vector<MaillageWfm>& maillages, std::vector<Vertex>& verts,
              std::vector<uint32_t>& idx, const std::vector<std::vector<ModelPart>*>& listes);

// accroche de loadCarModel : lit <chemin sans .wfm>.pt2, relit la table des maillages du .wfm (fichier), applique marquer, journal ;
// rend les matériaux d'origine non peints qui ont des sommets marqués (à passer à noterMateriaux une fois les matériaux créés)
std::vector<uint32_t> appliquer(const std::string& cheminWfm, const std::vector<uint8_t>& fichierWfm, std::vector<Vertex>& verts, std::vector<uint32_t>& idx, Model& m);
// accroche de loadCarModel, après la création des matériaux : ces matériaux reçoivent la finition de la peinture (RG-04)
void noterMateriaux(const std::vector<uint32_t>& entiers, Model& m);

} // namespace wfs::vehicule_peinture2
