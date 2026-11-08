// WaForSpeed — cache de pipelines Vulkan (WFS-262, RG-01, épic E4).
// Un VkPipelineCache passé à toutes les créations de pipelines (Renderer::makePipeline), enregistré dans les données de
// l'application et relu au lancement suivant : le pilote ne recompile plus les shaders, le chargement est plus court.
// En-tête vérifié avant usage (taille, version, fournisseur, appareil, UUID du cache) : cache d'un autre téléphone, d'un
// autre pilote ou abîmé → refusé, on repart à vide, sans erreur.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace wfs::cache_pipelines {

// identité de l'appareil attendue dans l'en-tête (VkPhysicalDeviceProperties)
struct Identite { uint32_t fournisseur = 0, appareil = 0; uint8_t uuid[16] = {}; };
// vrai si les octets lus sont un cache valide pour cet appareil (VkPipelineCacheHeaderVersionOne, little endian)
bool enteteValide(const uint8_t* d, size_t n, const Identite& id);
// fichier entier (vide s'il manque) ; écriture atomique (fichier temporaire puis renommage)
std::vector<uint8_t> lire(const std::string& chemin);
bool ecrire(const std::string& chemin, const std::vector<uint8_t>& d);

// dossier du fichier pipelines.cache : données internes de l'application (android_main.cpp) ; banc PC : WFS_CACHE_PIPELINES
// (dossier) ; vide : cache en mémoire seulement (rien relu ni écrit)
inline std::string dossier;
std::string chemin();

} // namespace wfs::cache_pipelines
