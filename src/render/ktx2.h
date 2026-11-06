// WaForSpeed — WFS-149 (RG-01) : textures KTX2 (conteneur Khronos « KTX File Format Specification 2.0 ») compressées
// ASTC 4x4 / 6x6, ETC2 RGBA ou non compressées RGBA8, avec TOUTES leurs mipmaps (calculées par l'outil tools/textures/,
// filtrées en linéaire, jamais par le GPU).
//
// Une texture = jusqu'à trois fichiers côte à côte, produits par tools/textures/convertir.py :
//   <nom>.astc.ktx2   ASTC (téléphones Android : Mali, Adreno, PowerVR, Xclipse)
//   <nom>.etc2.ktx2   ETC2 RGBA (repli : GPU sans ASTC)
//   <nom>.rgba8.ktx2  RGBA8 = l'ASTC décompressé par l'encodeur (repli du PC : lavapipe n'a ni ASTC ni ETC2 ; même image
//                     que sur le téléphone, aux pertes de compression près)
// chargerTexture() prend le premier que le GPU sait lire. sRGB (couleur) ou linéaire (normales, ORM) : donné par le
// format du fichier (vkFormat), choisi par l'outil selon le type de la texture.
//
// Limites de lecture (fichier corrompu : erreur et texture absente, jamais de plantage) : 2D seulement (pas de cube,
// de tableau ni de volume), sans supercompression, chaîne de mipmaps complète jusqu'à 1×1, 16384 px au plus.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "vk.h"

namespace wfs::ktx2 {

struct Bloc { uint32_t w = 0, h = 0, octets = 0; bool srgb = false; bool compresse = false; };
// formats lus par le moteur (les autres : refusés) ; faux si inconnu
bool decrireFormat(VkFormat f, Bloc& b);
const char* nomFormat(VkFormat f);

struct Niveau { size_t offset = 0, taille = 0; uint32_t w = 0, h = 0; };
struct Fichier {
    VkFormat format = VK_FORMAT_UNDEFINED; Bloc bloc;
    uint32_t w = 0, h = 0;
    std::vector<Niveau> niveaux;   // niveau 0 = pleine taille ; offset dans le fichier
    const uint8_t* donnees = nullptr; size_t taille = 0;   // le fichier entier (non possédé)
    size_t octetsTexture() const { size_t s = 0; for (const Niveau& n : niveaux) s += n.taille; return s; }
};
// lit l'en-tête et l'index des niveaux ; vérifie tailles et bornes. Faux (et erreur en clair) si le fichier est invalide.
bool lire(const uint8_t* p, size_t n, Fichier& f, std::string& erreur);

// texture chargée : image GPU + variante prise (astc, etc2, rgba8) et format
struct Texture { Image image; std::string variante; VkFormat format = VK_FORMAT_UNDEFINED; size_t octets = 0; bool ok = false; };
// cherche <base>.astc.ktx2, <base>.etc2.ktx2 puis <base>.rgba8.ktx2 (chemin des assets, readAsset) et envoie au GPU le
// premier que le GPU sait échantillonner (filtre linéaire). Faux : rien d'utilisable (erreur au journal).
bool chargerTexture(VkCtx& vk, const std::string& base, Texture& out);

} // namespace wfs::ktx2
