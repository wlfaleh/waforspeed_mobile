// WaForSpeed — WFS-420 (RG-03) : mémoire de la carte graphique PAR TYPE (textures et leur format, maillages, cibles de rendu,
// autres tampons), pour le fichier du banc de performance du téléphone. Compté là où la mémoire est réservée et rendue
// (VkCtx::createBuffer / createImage / essayerBuffer / essayerImage, destroy) ; le total reste celui de WFS-297 (memoire.h).
// Coût : une addition atomique par création ou destruction (jamais par image) ; la table des formats est sous verrou.
#pragma once
#include <cstdint>
#include <string>
#include <vulkan/vulkan.h>

namespace wfs::memoire_gpu {

enum Genre : uint8_t { TEXTURES = 0, MAILLAGES = 1, CIBLES = 2, AUTRES = 3, N_GENRES = 4 };
inline constexpr const char* NOMS[N_GENRES] = {"textures", "maillages", "cibles", "autres tampons"};

// tampon de sommets ou d'indices → maillages ; le reste (uniformes, transferts, stockage) → autres
inline Genre genreTampon(VkBufferUsageFlags u) {
    return (u & (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT)) ? MAILLAGES : AUTRES;
}
// image attachée à une passe (couleur, profondeur) → cible ; image lue par les shaders seulement → texture
inline Genre genreImage(VkImageUsageFlags u) {
    if (u & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) return CIBLES;
    return (u & VK_IMAGE_USAGE_SAMPLED_BIT) ? TEXTURES : AUTRES;
}
// nom court d'un format (« BC7 sRGB », « RGBA8 », « ASTC 6x6 »…) ; inconnu : « format n »
std::string nomFormat(VkFormat f);

// octets > 0 à la réservation, < 0 au retour ; fmt sert seulement aux textures
void compter(Genre g, int64_t octets, VkFormat fmt);
double mo(Genre g);
// « textures 612 Mo (BC7 sRGB 380, RGBA8 sRGB 200, …) · maillages 140 Mo · cibles 96 Mo · autres tampons 12 Mo »
std::string texte();

} // namespace wfs::memoire_gpu
