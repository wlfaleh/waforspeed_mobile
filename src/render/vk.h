// WaForSpeed — contexte Vulkan : instance, appareil, file, swapchain (Android) ou cible hors écran (tests),
// et petits utilitaires (tampons, images, envois vers le GPU).
#pragma once
#if defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#endif
#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>
#include "../core/log.h"

struct ANativeWindow;

namespace wfs {

#define VK_OK(x) do { VkResult r_ = (x); WFS_CHECK(r_ == VK_SUCCESS, "Vulkan : %s a échoué (%d) %s:%d", #x, (int)r_, __FILE__, __LINE__); } while (0)

struct Buffer { VkBuffer buf = VK_NULL_HANDLE; VkDeviceMemory mem = VK_NULL_HANDLE; VkDeviceSize size = 0; void* mapped = nullptr; };
struct Image {
    VkImage img = VK_NULL_HANDLE; VkDeviceMemory mem = VK_NULL_HANDLE; VkImageView view = VK_NULL_HANDLE;
    VkFormat format = VK_FORMAT_UNDEFINED; uint32_t w = 0, h = 0, mips = 1;
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
    void destroy(Buffer& b);
    Image createImage(uint32_t w, uint32_t h, uint32_t mips, VkFormat fmt, VkImageUsageFlags usage,
                      VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT, bool transient = false);
    void destroy(Image& i);
    VkCommandBuffer beginOneShot();
    void endOneShot(VkCommandBuffer cb);
    // envoie des données dans un tampon local au GPU (via un tampon de transfert)
    Buffer uploadBuffer(const void* data, VkDeviceSize size, VkBufferUsageFlags usage);
    // texture RGBA8 avec mipmaps générés sur le GPU
    Image uploadTexture(const uint8_t* rgba, uint32_t w, uint32_t h, bool srgb);

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
