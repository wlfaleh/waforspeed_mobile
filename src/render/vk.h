// WaForSpeed — contexte Vulkan : instance, appareil, file, swapchain (Android) ou cible hors écran (tests),
// et petits utilitaires (tampons, images, envois vers le GPU).
#pragma once
#if defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#endif
#include <vulkan/vulkan.h>
#include <cstddef>
#include <cstdint>
#include <vector>
#include "../core/log.h"

struct ANativeWindow;

namespace wfs {

#define VK_OK(x) do { VkResult r_ = (x); WFS_CHECK(r_ == VK_SUCCESS, "Vulkan : %s a échoué (%d) %s:%d", #x, (int)r_, __FILE__, __LINE__); } while (0)

// WFS-418 (RG-01) : réponses de Vulkan qui veulent dire « plus de mémoire » : la voie de repli (VkCtx::essayerBuffer) rend alors un
// tampon vide ; toute autre erreur arrête le jeu, comme VK_OK
constexpr bool manqueMemoire(VkResult r) { return r == VK_ERROR_OUT_OF_DEVICE_MEMORY || r == VK_ERROR_OUT_OF_HOST_MEMORY; }
// WFS-418 (RG-04) : banc PC seulement : la réservation n° numero (voie de repli, comptée à partir de 1 depuis le lancement) est
// refusée si echecA vaut 1 ou plus et numero >= echecA (la mémoire reste pleine après le premier refus) ; echecA = 0 : jamais
constexpr bool refusSimule(uint32_t numero, uint32_t echecA) { return echecA != 0 && numero >= echecA; }
// WFS-418 (RG-04) : valeur de WFS_MANQUE_MEMOIRE_GPU (banc PC) : entier de 1 à 10 000 000 ; absente, vide ou fausse : 0 (aucun refus)
inline uint32_t rangRefusSimule(const char* e) {
    if (!e || !*e) return 0;
    uint32_t n = 0;
    for (const char* c = e; *c; c++) {
        if (*c < '0' || *c > '9' || n > 1000000u) return 0;
        n = n * 10u + (uint32_t)(*c - '0');
    }
    return n <= 10000000u ? n : 0;
}

struct Buffer { VkBuffer buf = VK_NULL_HANDLE; VkDeviceMemory mem = VK_NULL_HANDLE; VkDeviceSize size = 0; void* mapped = nullptr; VkDeviceSize alloue = 0; uint8_t genre = 3; };   // WFS-297 : alloue (octets réels) ; WFS-420 : genre (memoire_gpu.h)
struct Image {
    VkImage img = VK_NULL_HANDLE; VkDeviceMemory mem = VK_NULL_HANDLE; VkImageView view = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED; uint32_t w = 0, h = 0, mips = 1;
    size_t octetsTexture = 0;   // WFS-149 : octets comptés dans VkCtx::octetsTextures (textures envoyées seulement)
    VkDeviceSize alloue = 0;    // WFS-297 (RG-06) : octets alloués (mémoire de la carte graphique), 0 si mémoire paresseuse
    uint8_t genre = 3;          // WFS-420 (RG-03) : textures, maillages, cibles ou autres (memoire_gpu.h)
};

class VkCtx {
public:
    // window == nullptr : rendu hors écran (tests sur le bureau), taille offW x offH
    bool init(ANativeWindow* window, uint32_t offW = 1280, uint32_t offH = 720);
    void shutdown();
    bool createSwapchain();          // (re)crée la swapchain ; renvoie false si la fenêtre est réduite
    void destroySwapchain();
    // vrai si l'écran a tourné ou changé de taille depuis la création de la swapchain
    bool surfaceChanged();
    void waitIdle() { if (device) vkDeviceWaitIdle(device); }

    // utilitaires
    uint32_t memoryType(uint32_t bits, VkMemoryPropertyFlags props, bool* found = nullptr) const;
    Buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, bool hostVisible);
    // WFS-418 (RG-01) : voie de repli de createBuffer / uploadBuffer : si la carte graphique répond « plus de mémoire »
    // (manqueMemoire), rien n'est gardé et le tampon rendu est vide (buf == VK_NULL_HANDLE), au lieu d'arrêter le jeu ; toute
    // autre erreur arrête le jeu comme createBuffer. Pour les réservations du chargement qui ont un secours (bâtiments cuits).
    Buffer essayerBuffer(VkDeviceSize size, VkBufferUsageFlags usage, bool hostVisible);
    Buffer essayerUploadBuffer(const void* data, VkDeviceSize size, VkBufferUsageFlags usage);
    // WFS-418 (décision du PO, 2026-11-11 12:42 UTC) : même voie de repli pour les images (atlas des bâtiments cuits seulement) :
    // image vide (img == VK_NULL_HANDLE) si « plus de mémoire » ; mêmes numéros de réservation et même refus simulé qu'essayerBuffer
    Image essayerImage(uint32_t w, uint32_t h, uint32_t mips, VkFormat fmt, VkImageUsageFlags usage);
    Image essayerUploadTexture(const uint8_t* rgba, uint32_t w, uint32_t h, bool srgb);
    uint32_t reservationsRepli = 0;   // WFS-418 : réservations faites par la voie de repli depuis le lancement (banc, journal)
    uint32_t echecReservation = 0;    // WFS-418 (RG-04) : banc PC seulement (WFS_MANQUE_MEMOIRE_GPU) : refusSimule ; 0 sur le téléphone
    void destroy(Buffer& b);
    Image createImage(uint32_t w, uint32_t h, uint32_t mips, VkFormat fmt, VkImageUsageFlags usage,
                      VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT, bool transient = false);
    void destroy(Image& i);
    // WFS-144 : appelé à la destruction d'une vue d'image (le Renderer oublie les matériaux qui la lisent)
    void (*vueDetruite)(void* ctx, VkImageView v) = nullptr; void* vueDetruiteCtx = nullptr;
    VkCommandBuffer beginOneShot();
    void endOneShot(VkCommandBuffer cb);
    // envoie des données dans un tampon local au GPU (via un tampon de transfert)
    Buffer uploadBuffer(const void* data, VkDeviceSize size, VkBufferUsageFlags usage);
    // texture RGBA8 avec mipmaps générés sur le GPU
    Image uploadTexture(const uint8_t* rgba, uint32_t w, uint32_t h, bool srgb);
    // WFS-134 : repli sans blit ; WFS-418 : repli = vrai : tampon de transfert par essayerBuffer, faux rendu s'il est refusé (image non remplie)
    bool uploadMipsCpu(Image& img, const uint8_t* rgba, uint32_t w, uint32_t h, uint32_t mips, bool repli = false);
    // WFS-149 (RG-01) : texture dont TOUS les niveaux sont fournis (KTX2 : ASTC, ETC2 ou RGBA8), copiés tels quels
    Image uploadNiveaux(VkFormat fmt, uint32_t w, uint32_t h, const uint8_t* const* niveaux, const size_t* tailles, uint32_t nNiveaux);
    // WFS-149 : le GPU sait échantillonner ce format avec filtre linéaire (fonctions ASTC / ETC2 activées à la création)
    bool formatEchantillonnable(VkFormat fmt) const;
    size_t octetsTextures = 0;   // WFS-149 : mémoire des textures envoyées (niveaux compris), journal et banc du réalisme
    bool statsPipeline = false;   // WFS-348 (RG-01) : banc seulement (WFS_STATS_PIPELINE=1) : pipelineStatisticsQuery activée
    bool astc = false, etc2 = false;   // WFS-149 : textureCompressionASTC_LDR / textureCompressionETC2 activées

    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice gpu = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t queueFamily = 0;
    VkPhysicalDeviceProperties props{};
    VkPhysicalDeviceMemoryProperties memProps{};
    VkCommandPool pool = VK_NULL_HANDLE;

    ANativeWindow* window = nullptr;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swapFormat = VK_FORMAT_R8G8B8A8_UNORM;
    bool swapSrgb = false;
    VkExtent2D swapExtent{};
    VkSurfaceTransformFlagBitsKHR transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    std::vector<VkImage> swapImages;
    std::vector<VkImageView> swapViews;

    bool offscreen = false;
    Image offTarget;                 // cible hors écran (RGBA8)
    uint32_t offW = 0, offH = 0;

    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    VkSampleCountFlagBits msaa = VK_SAMPLE_COUNT_1_BIT;
    bool linearBlitSrgb = true;      // mipmaps par blit linéaire disponibles
    float renderScale = 1.f;         // échelle de la scène 3D (1 = pleine définition)
};

} // namespace wfs
