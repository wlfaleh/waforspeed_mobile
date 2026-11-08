#include "vk.h"
#include "../core/memoire.h"   // WFS-297 (RG-06)
#include <algorithm>
#include <cmath>
#include <vector>
#include <cstring>
#include "../core/gpu_logiciel.h"   // WFS-59
#if defined(__ANDROID__)
#include <android/native_window.h>
#endif

namespace wfs {

bool VkCtx::init(ANativeWindow* win, uint32_t w, uint32_t h) {
    window = win; offscreen = (win == nullptr); offW = w; offH = h;

    // ---------- instance ----------
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "WaForSpeed"; app.applicationVersion = 1;
    app.pEngineName = "WaForSpeed Engine"; app.engineVersion = 1;
    app.apiVersion = VK_API_VERSION_1_0;
    std::vector<const char*> exts;
    if (!offscreen) {
        exts.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#if defined(__ANDROID__)
        exts.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(__APPLE__)
        exts.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);   // WFS-78 : iOS (MoltenVK), window = CAMetalLayer
#endif
    }
    VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    ici.pApplicationInfo = &app;
    ici.enabledExtensionCount = (uint32_t)exts.size(); ici.ppEnabledExtensionNames = exts.data();
    if (vkCreateInstance(&ici, nullptr, &instance) != VK_SUCCESS) { WFS_ERR("Vulkan indisponible sur cet appareil"); return false; }

#if defined(__ANDROID__)
    if (!offscreen) {
        VkAndroidSurfaceCreateInfoKHR sci{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};
        sci.window = window;
        VK_OK(vkCreateAndroidSurfaceKHR(instance, &sci, nullptr, &surface));
    }
#elif defined(__APPLE__)
    if (!offscreen) {   // WFS-78 : surface Metal (calque de la vue iOS)
        VkMetalSurfaceCreateInfoEXT sci{VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT};
        sci.pLayer = (const CAMetalLayer*)(const void*)window;
        VK_OK(vkCreateMetalSurfaceEXT(instance, &sci, nullptr, &surface));
    }
#endif

    // ---------- carte graphique : la première qui sait dessiner (et présenter) ----------
    uint32_t n = 0; vkEnumeratePhysicalDevices(instance, &n, nullptr);
    if (!n) { WFS_ERR("Aucune carte graphique Vulkan"); return false; }
    std::vector<VkPhysicalDevice> gpus(n); vkEnumeratePhysicalDevices(instance, &n, gpus.data());
    for (VkPhysicalDevice g : gpus) {
        uint32_t qn = 0; vkGetPhysicalDeviceQueueFamilyProperties(g, &qn, nullptr);
        std::vector<VkQueueFamilyProperties> qs(qn); vkGetPhysicalDeviceQueueFamilyProperties(g, &qn, qs.data());
        for (uint32_t i = 0; i < qn; i++) {
            if (!(qs[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) continue;
            VkBool32 present = VK_TRUE;
            if (surface) vkGetPhysicalDeviceSurfaceSupportKHR(g, i, surface, &present);
            if (present) { gpu = g; queueFamily = i; break; }
        }
        if (gpu) break;
    }
    if (!gpu) { WFS_ERR("Pas de file graphique compatible"); return false; }
    vkGetPhysicalDeviceProperties(gpu, &props);
    vkGetPhysicalDeviceMemoryProperties(gpu, &memProps);
    WFS_LOG("GPU : %s (Vulkan %u.%u)", props.deviceName, VK_VERSION_MAJOR(props.apiVersion), VK_VERSION_MINOR(props.apiVersion));

    // ---------- appareil logique ----------
    float prio = 1.f;
    VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    qci.queueFamilyIndex = queueFamily; qci.queueCount = 1; qci.pQueuePriorities = &prio;
    std::vector<const char*> dexts;
    if (!offscreen) dexts.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    VkPhysicalDeviceFeatures supported{}; vkGetPhysicalDeviceFeatures(gpu, &supported);
    VkPhysicalDeviceFeatures feats{};
    feats.samplerAnisotropy = supported.samplerAnisotropy;
    // WFS-149 (RG-01) : textures compressées ASTC (Android) et ETC2 (repli), si le GPU les a
    feats.textureCompressionASTC_LDR = supported.textureCompressionASTC_LDR; astc = supported.textureCompressionASTC_LDR;
    feats.textureCompressionETC2 = supported.textureCompressionETC2; etc2 = supported.textureCompressionETC2;
    VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    dci.queueCreateInfoCount = 1; dci.pQueueCreateInfos = &qci;
    dci.enabledExtensionCount = (uint32_t)dexts.size(); dci.ppEnabledExtensionNames = dexts.data();
    dci.pEnabledFeatures = &feats;
    VK_OK(vkCreateDevice(gpu, &dci, nullptr, &device));
    vkGetDeviceQueue(device, queueFamily, 0, &queue);

    VkCommandPoolCreateInfo pci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; pci.queueFamilyIndex = queueFamily;
    VK_OK(vkCreateCommandPool(device, &pci, nullptr, &pool));

    // ---------- formats ----------
    for (VkFormat f : {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM}) {
        VkFormatProperties fp; vkGetPhysicalDeviceFormatProperties(gpu, f, &fp);
        if (fp.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) { depthFormat = f; break; }
    }
    VkSampleCountFlags sc = props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;
    msaa = (sc & VK_SAMPLE_COUNT_4_BIT) ? VK_SAMPLE_COUNT_4_BIT : VK_SAMPLE_COUNT_1_BIT;
    // GPU logiciel sur un appareil (émulateur de la QA) : qualité réduite pour rester jouable
    // WFS-59 : mode GPU logiciel (détecté ici, ou demandé par l'intent qa_simu=true) → rendu à 25 %
    if (!offscreen && (std::strstr(props.deviceName, "SwiftShader") || std::strstr(props.deviceName, "llvmpipe")))
        activerModeGpuLogiciel(props.deviceName);
    if (!offscreen && modeGpuLogiciel()) {
        msaa = VK_SAMPLE_COUNT_1_BIT; renderScale = gpu_logiciel::RENDU;
        WFS_LOG("GPU logiciel : qualité réduite (sans MSAA, rendu à 25 %%)");
    }
    VkFormatProperties tp; vkGetPhysicalDeviceFormatProperties(gpu, VK_FORMAT_R8G8B8A8_SRGB, &tp);
    linearBlitSrgb = (tp.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) &&
                     (tp.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) && (tp.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);
    WFS_LOG("Profondeur %d, MSAA x%d, mipmaps GPU %s", (int)depthFormat, (int)msaa, linearBlitSrgb ? "oui" : "non");

    if (offscreen) {
        offTarget = createImage(offW, offH, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        swapFormat = VK_FORMAT_R8G8B8A8_UNORM; swapSrgb = false;
        swapExtent = {offW, offH};
        swapImages = {offTarget.img}; swapViews = {offTarget.view};
        return true;
    }
    return createSwapchain();
}

bool VkCtx::createSwapchain() {
    if (offscreen) return true;
    VkSurfaceCapabilitiesKHR caps; VK_OK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &caps));
    if (caps.currentExtent.width == 0 || caps.currentExtent.height == 0) return false;
    uint32_t fn = 0; vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &fn, nullptr);
    std::vector<VkSurfaceFormatKHR> fmts(fn); vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &fn, fmts.data());
    VkSurfaceFormatKHR chosen = fmts[0];
    for (auto& f : fmts) if (f.format == VK_FORMAT_R8G8B8A8_UNORM || f.format == VK_FORMAT_B8G8R8A8_UNORM) { chosen = f; break; }
    swapFormat = chosen.format;
    swapSrgb = (swapFormat == VK_FORMAT_R8G8B8A8_SRGB || swapFormat == VK_FORMAT_B8G8R8A8_SRGB);

    // pré-rotation (recommandée sur Android) : on dessine dans l'orientation native de l'écran,
    // la passe finale tourne l'image ; le compositeur n'a plus rien à faire.
    // Android donne currentExtent dans l'orientation ACTUELLE (ex. 2400x1080 en paysage) avec currentTransform = ROTATE_90 :
    // pour la pré-rotation, la swapchain se crée dans l'orientation NATIVE, donc largeur et hauteur échangées
    // (guide Android « Vulkan pre-rotation »). Le rendu se fait ensuite en taille logique paysage (Renderer::createTargets).
    transform = caps.currentTransform;
    swapExtent = caps.currentExtent;
    if (swapExtent.width == 0xFFFFFFFFu) swapExtent = {1280, 720};
    if (transform == VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR || transform == VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR)
        std::swap(swapExtent.width, swapExtent.height);

    uint32_t count = std::max(caps.minImageCount, 3u);
    if (caps.maxImageCount && count > caps.maxImageCount) count = caps.maxImageCount;
    VkCompositeAlphaFlagBitsKHR alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (!(caps.supportedCompositeAlpha & alpha)) alpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;

    VkSwapchainCreateInfoKHR sci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    sci.surface = surface; sci.minImageCount = count;
    sci.imageFormat = swapFormat; sci.imageColorSpace = chosen.colorSpace;
    sci.imageExtent = swapExtent; sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sci.preTransform = transform; sci.compositeAlpha = alpha;
    sci.presentMode = VK_PRESENT_MODE_FIFO_KHR;   // synchro écran : fluide, économe en batterie
    sci.clipped = VK_TRUE; sci.oldSwapchain = swapchain;
    VkSwapchainKHR next; VK_OK(vkCreateSwapchainKHR(device, &sci, nullptr, &next));
    if (swapchain) { for (auto v : swapViews) vkDestroyImageView(device, v, nullptr); vkDestroySwapchainKHR(device, swapchain, nullptr); }
    swapchain = next;

    uint32_t in = 0; vkGetSwapchainImagesKHR(device, swapchain, &in, nullptr);
    swapImages.resize(in); vkGetSwapchainImagesKHR(device, swapchain, &in, swapImages.data());
    swapViews.resize(in);
    for (uint32_t i = 0; i < in; i++) {
        VkImageViewCreateInfo vci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        vci.image = swapImages[i]; vci.viewType = VK_IMAGE_VIEW_TYPE_2D; vci.format = swapFormat;
        vci.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        VK_OK(vkCreateImageView(device, &vci, nullptr, &swapViews[i]));
    }
    WFS_LOG("Swapchain %ux%u, %u images, rotation %d", swapExtent.width, swapExtent.height, in, (int)transform);
    return true;
}

bool VkCtx::surfaceChanged() {
    if (offscreen || !surface) return false;
    VkSurfaceCapabilitiesKHR caps;
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &caps) != VK_SUCCESS) return false;
    if (caps.currentExtent.width == 0 || caps.currentExtent.height == 0) return false;
    VkExtent2D e = caps.currentExtent;
    if (caps.currentTransform == VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR || caps.currentTransform == VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) std::swap(e.width, e.height);
    return caps.currentTransform != transform || e.width != swapExtent.width || e.height != swapExtent.height;
}

void VkCtx::destroySwapchain() {
    if (offscreen || !device) return;
    for (auto v : swapViews) vkDestroyImageView(device, v, nullptr);
    swapViews.clear(); swapImages.clear();
    if (swapchain) vkDestroySwapchainKHR(device, swapchain, nullptr);
    swapchain = VK_NULL_HANDLE;
}

void VkCtx::shutdown() {
    if (!device) return;
    vkDeviceWaitIdle(device);
    if (offscreen) { swapViews.clear(); swapImages.clear(); destroy(offTarget); }
    else destroySwapchain();
    if (pool) vkDestroyCommandPool(device, pool, nullptr);
    vkDestroyDevice(device, nullptr); device = VK_NULL_HANDLE;
    if (surface) vkDestroySurfaceKHR(instance, surface, nullptr);
    surface = VK_NULL_HANDLE;
    vkDestroyInstance(instance, nullptr); instance = VK_NULL_HANDLE;
}

uint32_t VkCtx::memoryType(uint32_t bits, VkMemoryPropertyFlags want, bool* found) const {
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
        if ((bits & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & want) == want) { if (found) *found = true; return i; }
    if (found) { *found = false; return 0; }
    WFS_CHECK(false, "Aucun type de mémoire compatible");
    return 0;
}

Buffer VkCtx::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, bool hostVisible) {
    Buffer b; b.size = size;
    VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bci.size = size; bci.usage = usage; bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VK_OK(vkCreateBuffer(device, &bci, nullptr, &b.buf));
    VkMemoryRequirements req; vkGetBufferMemoryRequirements(device, b.buf, &req);
    VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    mai.allocationSize = req.size;
    mai.memoryTypeIndex = memoryType(req.memoryTypeBits, hostVisible ? (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_OK(vkAllocateMemory(device, &mai, nullptr, &b.mem));
    b.alloue = req.size; memoire::compteurs().octetsGpu += (int64_t)req.size;   // WFS-297 (RG-06)
    VK_OK(vkBindBufferMemory(device, b.buf, b.mem, 0));
    if (hostVisible) VK_OK(vkMapMemory(device, b.mem, 0, VK_WHOLE_SIZE, 0, &b.mapped));
    return b;
}

void VkCtx::destroy(Buffer& b) {
    if (b.buf) vkDestroyBuffer(device, b.buf, nullptr);
    if (b.mem) vkFreeMemory(device, b.mem, nullptr);
    memoire::compteurs().octetsGpu -= (int64_t)b.alloue;   // WFS-297
    b = Buffer{};
}

Image VkCtx::createImage(uint32_t w, uint32_t h, uint32_t mips, VkFormat fmt, VkImageUsageFlags usage,
                         VkSampleCountFlagBits samples, VkImageAspectFlags aspect, bool transient) {
    Image i; i.w = w; i.h = h; i.mips = mips; i.format = fmt;
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ci.imageType = VK_IMAGE_TYPE_2D; ci.format = fmt; ci.extent = {w, h, 1};
    ci.mipLevels = mips; ci.arrayLayers = 1; ci.samples = samples; ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.usage = usage | (transient ? VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT : 0);
    ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE; ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VK_OK(vkCreateImage(device, &ci, nullptr, &i.img));
    VkMemoryRequirements req; vkGetImageMemoryRequirements(device, i.img, &req);
    VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    mai.allocationSize = req.size;
    bool lazy = false;
    // MSAA et profondeur temporaires : mémoire « paresseuse » des GPU mobiles (reste dans la mémoire de tuile)
    if (transient) mai.memoryTypeIndex = memoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, &lazy);
    if (!lazy) mai.memoryTypeIndex = memoryType(req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_OK(vkAllocateMemory(device, &mai, nullptr, &i.mem));
    if (!lazy) { i.alloue = req.size; memoire::compteurs().octetsGpu += (int64_t)req.size; }   // WFS-297 (RG-06) : la mémoire paresseuse reste dans la tuile
    VK_OK(vkBindImageMemory(device, i.img, i.mem, 0));
    VkImageViewCreateInfo vci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vci.image = i.img; vci.viewType = VK_IMAGE_VIEW_TYPE_2D; vci.format = fmt;
    vci.subresourceRange = {aspect, 0, mips, 0, 1};
    VK_OK(vkCreateImageView(device, &vci, nullptr, &i.view));
    return i;
}

void VkCtx::destroy(Image& i) {
    if (i.view && vueDetruite) vueDetruite(vueDetruiteCtx, i.view);   // WFS-144
    octetsTextures -= std::min(octetsTextures, i.octetsTexture);   // WFS-149
    if (i.view) vkDestroyImageView(device, i.view, nullptr);
    if (i.img) vkDestroyImage(device, i.img, nullptr);
    if (i.mem) vkFreeMemory(device, i.mem, nullptr);
    memoire::compteurs().octetsGpu -= (int64_t)i.alloue;   // WFS-297
    i = Image{};
}

VkCommandBuffer VkCtx::beginOneShot() {
    VkCommandBufferAllocateInfo ai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    ai.commandPool = pool; ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; ai.commandBufferCount = 1;
    VkCommandBuffer cb; VK_OK(vkAllocateCommandBuffers(device, &ai, &cb));
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_OK(vkBeginCommandBuffer(cb, &bi));
    return cb;
}

void VkCtx::endOneShot(VkCommandBuffer cb) {
    VK_OK(vkEndCommandBuffer(cb));
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1; si.pCommandBuffers = &cb;
    VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VkFence f; VK_OK(vkCreateFence(device, &fci, nullptr, &f));
    VK_OK(vkQueueSubmit(queue, 1, &si, f));
#if defined(__APPLE__)
    {   // WFS-88 : attente bornée et journalisée (MoltenVK) : une attente sans fin ne laisse plus le journal muet
        VkResult r; int s = 0;
        while ((r = vkWaitForFences(device, 1, &f, VK_TRUE, 5000000000ull)) == VK_TIMEOUT)
            WFS_LOG("iOS : envoi au GPU sans réponse depuis %d s (statut de la barrière %d)", s += 5, (int)vkGetFenceStatus(device, f));
        if (r != VK_SUCCESS) WFS_ERR("iOS : attente de l'envoi au GPU : %d", (int)r);
    }
#else
    VK_OK(vkWaitForFences(device, 1, &f, VK_TRUE, UINT64_MAX));
#endif
    vkDestroyFence(device, f, nullptr);
    vkFreeCommandBuffers(device, pool, 1, &cb);
}

Buffer VkCtx::uploadBuffer(const void* data, VkDeviceSize size, VkBufferUsageFlags usage) {
    Buffer staging = createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
    std::memcpy(staging.mapped, data, (size_t)size);
    Buffer dst = createBuffer(size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false);
    VkCommandBuffer cb = beginOneShot();
    VkBufferCopy cp{0, 0, size};
    vkCmdCopyBuffer(cb, staging.buf, dst.buf, 1, &cp);
    endOneShot(cb);
    destroy(staging);
    return dst;
}

static void barrier(VkCommandBuffer cb, VkImage img, uint32_t mip, uint32_t count, VkImageLayout from, VkImageLayout to,
                    VkAccessFlags srcA, VkAccessFlags dstA, VkPipelineStageFlags srcS, VkPipelineStageFlags dstS) {
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    b.oldLayout = from; b.newLayout = to; b.srcAccessMask = srcA; b.dstAccessMask = dstA;
    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = img; b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, mip, count, 0, 1};
    vkCmdPipelineBarrier(cb, srcS, dstS, 0, 0, nullptr, 0, nullptr, 1, &b);
}

// WFS-134 (RG-04) : chaîne de mipmaps par moyenne 2×2 (couleur sRGB moyennée en linéaire), tous les niveaux copiés d'un coup
void VkCtx::uploadMipsCpu(Image& img, const uint8_t* rgba, uint32_t w, uint32_t h, uint32_t mips) {
    std::vector<uint8_t> tout(rgba, rgba + (size_t)w * h * 4);
    std::vector<VkBufferImageCopy> copies;
    const bool srgb = img.format == VK_FORMAT_R8G8B8A8_SRGB;
    auto versLin = [&](uint8_t c) { const float v = c / 255.f; return srgb ? std::pow(v, 2.2f) : v; };
    auto versOct = [&](float v) { return (uint8_t)std::lround(std::clamp(srgb ? std::pow(v, 1 / 2.2f) : v, 0.f, 1.f) * 255.f); };
    size_t ofs = 0; uint32_t mw = w, mh = h;
    for (uint32_t i = 0; i < mips; i++) {
        VkBufferImageCopy r{};
        r.bufferOffset = ofs; r.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1}; r.imageExtent = {mw, mh, 1};
        copies.push_back(r);
        if (i + 1 == mips) break;
        const uint32_t nw = mw > 1 ? mw / 2 : 1, nh = mh > 1 ? mh / 2 : 1;
        const size_t src = ofs; ofs += (size_t)mw * mh * 4;
        tout.resize(ofs + (size_t)nw * nh * 4);
        for (uint32_t y = 0; y < nh; y++) for (uint32_t x = 0; x < nw; x++) for (int c = 0; c < 4; c++) {
            float s = 0;
            for (uint32_t dy = 0; dy < 2; dy++) for (uint32_t dx = 0; dx < 2; dx++) {
                const uint32_t sx = std::min(mw - 1, x * 2 + dx), sy = std::min(mh - 1, y * 2 + dy);
                const uint8_t v = tout[src + ((size_t)sy * mw + sx) * 4 + c];
                s += c == 3 ? v / 255.f : versLin(v);
            }
            tout[ofs + ((size_t)y * nw + x) * 4 + c] = c == 3 ? (uint8_t)std::lround(s / 4 * 255.f) : versOct(s / 4);
        }
        mw = nw; mh = nh;
    }
    Buffer staging = createBuffer(tout.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
    std::memcpy(staging.mapped, tout.data(), tout.size());
    VkCommandBuffer cb = beginOneShot();
    barrier(cb, img.img, 0, mips, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    vkCmdCopyBufferToImage(cb, staging.buf, img.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)copies.size(), copies.data());
    barrier(cb, img.img, 0, mips, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    endOneShot(cb);
    destroy(staging);
    static bool annonce = false;
    if (!annonce) { annonce = true; WFS_LOG("textures : mipmaps calculés sur le CPU (GPU sans blit filtré, WFS-134)"); }
}

Image VkCtx::uploadTexture(const uint8_t* rgba, uint32_t w, uint32_t h, bool srgb) {
    VkFormat fmt = srgb ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
    VkFormatProperties fp; vkGetPhysicalDeviceFormatProperties(gpu, fmt, &fp);
    bool canBlit = (fp.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) && (fp.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) &&
                   (fp.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);
    uint32_t mips = 1;
    { uint32_t m = std::max(w, h); while (m > 1) { m >>= 1; mips++; } }
    Image img = createImage(w, h, mips, fmt, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    img.octetsTexture = (size_t)w * h * 4 * 4 / 3; octetsTextures += img.octetsTexture;   // WFS-149 : niveaux compris (≈ 4/3)
    if (!canBlit) {   // WFS-134 (RG-04) : GPU sans blit filtré : mipmaps calculés sur le CPU (jamais de texture sans mipmaps)
        uploadMipsCpu(img, rgba, w, h, mips);
        return img;
    }
    VkDeviceSize size = (VkDeviceSize)w * h * 4;
    Buffer staging = createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
    std::memcpy(staging.mapped, rgba, (size_t)size);
    VkCommandBuffer cb = beginOneShot();
    barrier(cb, img.img, 0, mips, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkBufferImageCopy r{};
    r.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}; r.imageExtent = {w, h, 1};
    vkCmdCopyBufferToImage(cb, staging.buf, img.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &r);
    int32_t mw = (int32_t)w, mh = (int32_t)h;
    for (uint32_t i = 1; i < mips; i++) {
        barrier(cb, img.img, i - 1, 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        VkImageBlit bl{};
        bl.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i - 1, 0, 1}; bl.srcOffsets[1] = {mw, mh, 1};
        int32_t nw = mw > 1 ? mw / 2 : 1, nh = mh > 1 ? mh / 2 : 1;
        bl.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1}; bl.dstOffsets[1] = {nw, nh, 1};
        vkCmdBlitImage(cb, img.img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, img.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bl, VK_FILTER_LINEAR);
        barrier(cb, img.img, i - 1, 1, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        mw = nw; mh = nh;
    }
    barrier(cb, img.img, mips - 1, 1, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    endOneShot(cb);
    destroy(staging);
    return img;
}

bool VkCtx::formatEchantillonnable(VkFormat fmt) const {
    const bool astcF = fmt >= VK_FORMAT_ASTC_4x4_UNORM_BLOCK && fmt <= VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
    const bool etc2F = fmt >= VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK && fmt <= VK_FORMAT_EAC_R11G11_SNORM_BLOCK;
    if ((astcF && !astc) || (etc2F && !etc2)) return false;   // fonction non activée sur l'appareil : format interdit
    VkFormatProperties fp; vkGetPhysicalDeviceFormatProperties(gpu, fmt, &fp);
    const VkFormatFeatureFlags f = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
    return (fp.optimalTilingFeatures & f) == f;
}

// WFS-149 (RG-01) : niveaux fournis (KTX2) copiés d'un seul envoi : aucune mipmap calculée par le GPU (les formats
// compressés ne se « blittent » pas, et l'outil les a filtrées en linéaire)
Image VkCtx::uploadNiveaux(VkFormat fmt, uint32_t w, uint32_t h, const uint8_t* const* niveaux, const size_t* tailles, uint32_t nNiveaux) {
    if (!nNiveaux || !w || !h) return Image{};
    size_t total = 0;
    for (uint32_t i = 0; i < nNiveaux; i++) total += (tailles[i] + 15) & ~(size_t)15;   // décalages alignés sur 16 octets (bloc)
    Buffer staging = createBuffer(total, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
    std::vector<VkBufferImageCopy> copies;
    size_t ofs = 0; uint32_t lw = w, lh = h;
    for (uint32_t i = 0; i < nNiveaux; i++) {
        std::memcpy((uint8_t*)staging.mapped + ofs, niveaux[i], tailles[i]);
        VkBufferImageCopy r{};
        r.bufferOffset = ofs; r.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, i, 0, 1}; r.imageExtent = {lw, lh, 1};
        copies.push_back(r);
        ofs += (tailles[i] + 15) & ~(size_t)15;
        lw = lw > 1 ? lw / 2 : 1; lh = lh > 1 ? lh / 2 : 1;
    }
    Image img = createImage(w, h, nNiveaux, fmt, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    VkCommandBuffer cb = beginOneShot();
    barrier(cb, img.img, 0, nNiveaux, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    vkCmdCopyBufferToImage(cb, staging.buf, img.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)copies.size(), copies.data());
    barrier(cb, img.img, 0, nNiveaux, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    endOneShot(cb);
    destroy(staging);
    for (uint32_t i = 0; i < nNiveaux; i++) img.octetsTexture += tailles[i];
    octetsTextures += img.octetsTexture;
    return img;
}

} // namespace wfs
