// WaForSpeed — WFS-495 (épic E18, domaine E10 Véhicules) : compression ETC2 RGBA8 sur le téléphone, pour la texture d'habillage
// cuite (skin + autocollants, vehicule_autocollants.h). La cuisson se fait au garage ou au chargement (jamais en course) et doit
// tenir en moins d'1 s sur le S21 FE (RG-03) : encodeur rapide, sans recherche exhaustive.
//
// Format (Khronos Data Format Specification 1.3) : VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, blocs de 4 × 4 texels sur 16 octets :
//   - 8 octets d'alpha EAC (§ 21.3, variante 8 bits) : base, multiplicateur (1 à 15), table, 16 indices de 3 bits ;
//   - 8 octets de couleur ETC2, seulement dans les deux modes hérités d'ETC1 (§ 21.1) : « individuel » (deux couleurs 4 bits)
//     et « différentiel » (couleur 5 bits + écart 3 bits, sans débordement : jamais lu comme un mode T, H ou plan).
// Ordre des texels dans un bloc : i = x × 4 + y (colonnes d'abord), comme vehicule_skin::decoderEacR11.
// Le décodeur sert au test (tests/test_vehicule_autocollants.cpp) et au repli des GPU sans ETC2 (PC : lavapipe) : même image
// que le téléphone. Sans Vulkan, sans allocation dans le codage d'un bloc ; appelable depuis plusieurs threads (blocs séparés).
#pragma once
#include <cstddef>
#include <cstdint>

namespace wfs::vehicule_etc2 {

// encode un bloc de 16 texels RGBA8 (ordre i = x × 4 + y) en 16 octets (alpha EAC puis couleur ETC2)
void encoderBloc(const uint8_t texels[16][4], uint8_t sortie[16]);
// décode un bloc de 16 octets (modes individuel et différentiel seulement) en 16 texels RGBA8 (ordre i = x × 4 + y)
void decoderBloc(const uint8_t bloc[16], uint8_t texels[16][4]);

// image RGBA8 w × h (lignes contiguës) → blocs ETC2 RGBA8 (lignes de blocs) ; bords complétés par répétition du dernier texel.
// taille de la sortie : octets(w, h). encoderLignes ne code que les lignes de blocs [l0, l1) : l'appelant les répartit sur les
// cœurs (jobs().parallelFor) ; lignesDeBlocs(h) lignes en tout.
size_t octets(uint32_t w, uint32_t h);
uint32_t lignesDeBlocs(uint32_t h);
void encoderLignes(const uint8_t* rgba, uint32_t w, uint32_t h, uint8_t* sortie, uint32_t l0, uint32_t l1);
void decoderImage(const uint8_t* blocs, uint32_t w, uint32_t h, uint8_t* rgba);

} // namespace wfs::vehicule_etc2
