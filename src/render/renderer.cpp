#include "renderer.h"
#include "mesh.h"   // WFS-149 : tangentes
#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include "feux_modele.h"           // WFS-171 : valeurs photométriques des feux, éclairement au sol
#include "phares.h"                // WFS-226 : phares des autres voitures notés pour le système de nuit
#include "cache_pipelines.h"        // WFS-262 (RG-01)
#include "shader_leger.h"           // WFS-264 (RG-06)
#include "lod_distance.h"           // WFS-265 (RG-01)
#include "qualite_auto.h"          // WFS-144 : niveau de netteté (MSAA, anisotrope)
#include "passes.h"                // WFS-150 : passes enfichables (ordonnanceur, pré-passe, capture, coûts)
#include "../core/memoire.h"   // WFS-297 (RG-06)
#include "eclairage_physique.h"   // WFS-151 : éclairage physique et IBL (accroche E13)
#include "../core/configgame.h"   // WFS-142 : force du reflet de la route mouillée
#include "../core/platform.h"   // WFS-222 : nowSeconds (banc de profilage)
#include "vehicule_lod.h"         // WFS-158 (RG-04) : niveaux de détail des voitures
#if defined(__APPLE__)
#include <chrono>
#endif

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../third_party/stb/stb_image_write.h"

namespace wfs {

// SPIR-V compilé à la construction (glslc -mfmt=num) puis inclus ici
static const uint32_t SPV_SCENE_VERT[] = {
#include "scene.vert.inc"
};
static const uint32_t SPV_SCENE_FRAG[] = {
#include "scene.frag.inc"
};
static const uint32_t SPV_SKY_VERT[] = {
#include "sky.vert.inc"
};
static const uint32_t SPV_SKY_FRAG[] = {
#include "sky.frag.inc"
};
static const uint32_t SPV_POST_VERT[] = {
#include "post.vert.inc"
};
static const uint32_t SPV_POST_FRAG[] = {
#include "post.frag.inc"
};
static const uint32_t SPV_UI_VERT[] = {
#include "ui.vert.inc"
};
static const uint32_t SPV_UI_FRAG[] = {
#include "ui.frag.inc"
};
static const uint32_t SPV_SHADOW_VERT[] = {
#include "shadow.vert.inc"
};
static const uint32_t SPV_SHADOW_FRAG[] = {
#include "shadow.frag.inc"
};
static const uint32_t SPV_SCENE_SKIN_VERT[] = {   // WFS-63 : maillages animés par squelette
#include "scene_skin.vert.inc"
};
static const uint32_t SPV_SHADOW_SKIN_VERT[] = {
#include "shadow_skin.vert.inc"
};
static const uint32_t SPV_SCENE_SKIN_FRAG[] = {   // WFS-72
#include "scene_skin.frag.inc"
};
// une palette (64 matrices, 4 096 octets) puis l'apparence (WFS-72 : 8 vec4, 128 octets) ; tranche alignée sur 256 octets
static constexpr VkDeviceSize OS_PALETTE = sizeof(float) * 16 * SKIN_MAX_OS, OS_APPARENCE = sizeof(float) * 4 * 8;
static constexpr VkDeviceSize OS_SLOT = (OS_PALETTE + OS_APPARENCE + 255) / 256 * 256;

#include "eclairage_ombres.h"   // WFS-227
#include "frame_ubo.h"   // WFS-150 : FrameUBO déplacé tel quel (lu aussi par les passes enfichables, src/render/passes.cpp)

// ombres du soleil (WFS-07, valeurs du jeu web : renderer.js applyEnv / stableShadow)
static constexpr float SHADOW_HALF = 40.f;        // demi-largeur de la boîte (m)
static constexpr float SHADOW_DEPTH = 260.f;      // near 1 → far 260 : profondeur couverte (m)
static constexpr float SHADOW_AHEAD = 20.f;       // boîte décalée vers l'avant (m)
static constexpr float SHADOW_BIAS = -0.0003f, SHADOW_NORMAL_BIAS = 0.035f;
static constexpr VkFormat SHADOW_FORMAT = VK_FORMAT_D16_UNORM;

static constexpr VkFormat HDR_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT;
static constexpr size_t UI_MAX_VERTS = 6 * 8192;
static constexpr int QUADS_MAX = 4096;   // WFS-09 : quads transitoires par image (halos, tache des phares)

#if defined(__APPLE__)
// WFS-88 : journal de chaque sous-étape de Renderer::init sur iOS (MoltenVK), temps en ms depuis la précédente
static std::chrono::steady_clock::time_point g_etapeRendu;
static void etapeRendu(const char* nom) {
    const auto t = std::chrono::steady_clock::now();
    WFS_LOG("iOS : rendu, %s %.0f ms", nom, std::chrono::duration<double, std::milli>(t - g_etapeRendu).count());
    g_etapeRendu = t;
}
#define ETAPE_RENDU(nom) etapeRendu(nom)
#else
#define ETAPE_RENDU(nom) ((void)0)
#endif

// WFS-262 (RG-01) : cache écrit dans les données de l'application (fin du chargement, arrière-plan, arrêt)
void Renderer::enregistrerCachePipelines() {
    if (!vk_ || !cachePipelines_ || cache_pipelines::chemin().empty()) return;
    size_t n = 0;
    if (vkGetPipelineCacheData(vk_->device, cachePipelines_, &n, nullptr) != VK_SUCCESS || !n) return;
    std::vector<uint8_t> d(n);
    if (vkGetPipelineCacheData(vk_->device, cachePipelines_, &n, d.data()) != VK_SUCCESS) return;
    d.resize(n);
    const bool ok = cache_pipelines::ecrire(cache_pipelines::chemin(), d);
    WFS_LOG("pipelines (WFS-262) : cache %s (%zu octets, %u pipelines créés depuis le lancement)", ok ? "enregistré" : "NON enregistré", n, pipelinesCrees_);
}

bool Renderer::init(VkCtx& vk) {
    vk_ = &vk;
    if (vk.offscreen) { const char* a = std::getenv("WFS_AVANT_222"); rendu221 = a && !std::strcmp(a, "1"); }   // WFS-222 : banc seulement
    VkDevice d = vk.device;
#if defined(__APPLE__)
    g_etapeRendu = std::chrono::steady_clock::now();
    WFS_LOG("iOS : rendu, début de Renderer::init");
#endif

    // ---------- échantillonneurs ----------
    VkSamplerCreateInfo sci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sci.magFilter = sci.minFilter = VK_FILTER_LINEAR; sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sci.addressModeU = sci.addressModeV = sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sci.maxLod = 16.f;
    VkPhysicalDeviceFeatures f{}; vkGetPhysicalDeviceFeatures(vk.gpu, &f);
    if (f.samplerAnisotropy) { sci.anisotropyEnable = VK_TRUE; sci.maxAnisotropy = std::min(8.f, vk.props.limits.maxSamplerAnisotropy); }
    aniso_ = anisoMax_ = sci.anisotropyEnable ? sci.maxAnisotropy : 1.f;   // WFS-134 (RG-04) : journal du chargement des voitures
    VK_OK(vkCreateSampler(d, &sci, nullptr, &sampler_));
    if (sci.anisotropyEnable && sci.maxAnisotropy > 4.f) {   // WFS-144 (RG-04) : anisotrope ×4 de la Basse
        sci.maxAnisotropy = 4.f; VK_OK(vkCreateSampler(d, &sci, nullptr, &samplerBasse_));
    }
    {   // WFS-262 (RG-01) : cache de pipelines relu du lancement précédent (en-tête vérifié ; refusé → cache vide)
        cache_pipelines::Identite id{vk.props.vendorID, vk.props.deviceID, {}};
        std::memcpy(id.uuid, vk.props.pipelineCacheUUID, 16);
        const std::vector<uint8_t> donnees = cache_pipelines::lire(cache_pipelines::chemin());
        const bool ok = cache_pipelines::enteteValide(donnees.data(), donnees.size(), id);
        VkPipelineCacheCreateInfo ci{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
        if (ok) { ci.initialDataSize = donnees.size(); ci.pInitialData = donnees.data(); }
        if (vkCreatePipelineCache(d, &ci, nullptr, &cachePipelines_) != VK_SUCCESS) {
            ci.initialDataSize = 0; ci.pInitialData = nullptr;
            if (vkCreatePipelineCache(d, &ci, nullptr, &cachePipelines_) != VK_SUCCESS) cachePipelines_ = VK_NULL_HANDLE;
        }
        WFS_LOG("pipelines (WFS-262) : cache %s (%zu octets lus%s)", ok ? "relu" : "vide", donnees.size(),
                donnees.empty() ? "" : ok ? "" : ", refusé : autre appareil, autre pilote ou fichier abîmé");
        debutChargement_ = std::chrono::steady_clock::now();
    }
    msaaMax_ = vk.msaa;   // WFS-144 : MSAA choisi par vk.cpp (×4, ou ×1 : GPU logiciel), plafond de la netteté
    vk.vueDetruiteCtx = this;   // WFS-144 : un matériau dont une texture est détruite n'est plus réécrit (vue invalide)
    vk.vueDetruite = [](void* c, VkImageView v) {
        std::erase_if(static_cast<Renderer*>(c)->matTex_, [v](const MatTex& t) { return t.v[0] == v || t.v[1] == v || t.v[2] == v; });
    };
    sci.anisotropyEnable = VK_FALSE; sci.maxLod = 0;
    sci.addressModeU = sci.addressModeV = sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    VK_OK(vkCreateSampler(d, &sci, nullptr, &clampSampler_));
    ETAPE_RENDU("échantillonneurs");

    // ---------- textures par défaut ----------
    uint8_t w[4] = {255, 255, 255, 255}, nrm[4] = {128, 128, 255, 255};
    white = vk.uploadTexture(w, 1, 1, false);
    flatNormal = vk.uploadTexture(nrm, 1, 1, false);
    ETAPE_RENDU("textures par défaut (premier envoi au GPU)");

    // ---------- descripteurs ----------
    VkDescriptorSetLayoutBinding b0[6] = {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                                          {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},    // carte d'ombre
                                          {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},    // reflet (WFS-16)
                                          {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},    // WFS-151 : sondes IBL A, B
                                          {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                                          {5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};   // WFS-151 : table BRDF
    VkDescriptorSetLayoutCreateInfo lci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    lci.bindingCount = 6; lci.pBindings = b0;
    VK_OK(vkCreateDescriptorSetLayout(d, &lci, nullptr, &frameLayout_));
    VkDescriptorSetLayoutBinding mb[3];
    for (uint32_t i = 0; i < 3; i++) mb[i] = {i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    lci.bindingCount = 3; lci.pBindings = mb;
    VK_OK(vkCreateDescriptorSetLayout(d, &lci, nullptr, &matLayout_));
    lci.bindingCount = 2; lci.pBindings = mb;   // WFS-100 : 0 = image source, 1 = bloom (passe finale) / niveau inférieur (remontée)
    VK_OK(vkCreateDescriptorSetLayout(d, &lci, nullptr, &postLayout_));

    VkDescriptorPoolSize sizes[3] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 8}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 * 1024 + 16 + 48 + 12},   // WFS-151 : + 3 × 4 jeux de l'image
                                     {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 4}};   // WFS-63 : palettes d'os
    VkDescriptorPoolCreateInfo pci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pci.maxSets = 1024 + 16 + 16; pci.poolSizeCount = 3; pci.pPoolSizes = sizes;   // WFS-100 : + 16 sets du bloom
    VK_OK(vkCreateDescriptorPool(d, &pci, nullptr, &pool_));
    ETAPE_RENDU("descripteurs");

    VkPushConstantRange pcr{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PC)};
    VkDescriptorSetLayout sl[2] = {frameLayout_, matLayout_};
    VkPipelineLayoutCreateInfo plci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    plci.setLayoutCount = 2; plci.pSetLayouts = sl; plci.pushConstantRangeCount = 1; plci.pPushConstantRanges = &pcr;
    VK_OK(vkCreatePipelineLayout(d, &plci, nullptr, &sceneLayout_));
    {   // WFS-63 : set 2 = palette d'os (tampon uniforme à offset dynamique) ; sets 0 et 1 identiques à la scène
        VkDescriptorSetLayoutBinding ob{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};   // WFS-72 : apparence lue par scene_skin.frag
        lci.bindingCount = 1; lci.pBindings = &ob;
        VK_OK(vkCreateDescriptorSetLayout(d, &lci, nullptr, &osLayout_));
        VkDescriptorSetLayout sk[3] = {frameLayout_, matLayout_, osLayout_};
        VkPipelineLayoutCreateInfo kci = plci; kci.setLayoutCount = 3; kci.pSetLayouts = sk;
        VK_OK(vkCreatePipelineLayout(d, &kci, nullptr, &skinLayout_));
        for (int f = 0; f < FRAMES; f++) {
            osBuf_[f] = vk.createBuffer(OS_SLOT * SKIN_MAX_INSTANCES, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, true);
            VkDescriptorSetAllocateInfo oai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            oai.descriptorPool = pool_; oai.descriptorSetCount = 1; oai.pSetLayouts = &osLayout_;
            VK_OK(vkAllocateDescriptorSets(d, &oai, &osSet_[f]));
            VkDescriptorBufferInfo obi{osBuf_[f].buf, 0, OS_PALETTE + OS_APPARENCE};
            VkWriteDescriptorSet ow{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            ow.dstSet = osSet_[f]; ow.dstBinding = 0; ow.descriptorCount = 1; ow.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            ow.pBufferInfo = &obi;
            vkUpdateDescriptorSets(d, 1, &ow, 0, nullptr);
        }
    }
    VkPushConstantRange ppr{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 48};   // WFS-100 : + mode et paramètres du bloom
    plci.setLayoutCount = 1; plci.pSetLayouts = &postLayout_; plci.pPushConstantRanges = &ppr;
    VK_OK(vkCreatePipelineLayout(d, &plci, nullptr, &postPLayout_));
    ETAPE_RENDU("dispositions de pipeline et palettes d'os");

    createShadows();
    ETAPE_RENDU("ombres");

    // ---------- ressources par image en vol ----------
    VkCommandBufferAllocateInfo cai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cai.commandPool = vk.pool; cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; cai.commandBufferCount = 1;
    for (auto& r : fr_) {
        VK_OK(vkAllocateCommandBuffers(d, &cai, &r.cb));
        VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO}; fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VK_OK(vkCreateFence(d, &fci, nullptr, &r.fence));
        VkSemaphoreCreateInfo semi{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        VK_OK(vkCreateSemaphore(d, &semi, nullptr, &r.acquired));
        VK_OK(vkCreateSemaphore(d, &semi, nullptr, &r.done));
        r.ubo = vk.createBuffer(sizeof(FrameUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, true);
        r.uiVb = vk.createBuffer(sizeof(UiVertex) * UI_MAX_VERTS, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, true);
        VkDescriptorSetAllocateInfo dai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        dai.descriptorPool = pool_; dai.descriptorSetCount = 1; dai.pSetLayouts = &frameLayout_;
        VK_OK(vkAllocateDescriptorSets(d, &dai, &r.frameSet));
        VkDescriptorBufferInfo bi{r.ubo.buf, 0, sizeof(FrameUBO)};
        VkDescriptorImageInfo si{shadowSampler_, shadowMap_.view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet wr[2]{};
        for (auto& w : wr) { w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; w.dstSet = r.frameSet; w.descriptorCount = 1; }
        wr[0].dstBinding = 0; wr[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; wr[0].pBufferInfo = &bi;
        wr[1].dstBinding = 1; wr[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; wr[1].pImageInfo = &si;
        vkUpdateDescriptorSets(d, 2, wr, 0, nullptr);
    }
    // WFS-16 : passe de reflet = même jeu de descripteurs, tampon uniforme miroir, sans reflet lisible (texture blanche)
    for (int f = 0; f < FRAMES; f++) {
        refletUbo_[f] = vk.createBuffer(sizeof(FrameUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, true);
        VkDescriptorSetAllocateInfo rai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        rai.descriptorPool = pool_; rai.descriptorSetCount = 1; rai.pSetLayouts = &frameLayout_;
        VK_OK(vkAllocateDescriptorSets(d, &rai, &refletSet_[f]));
        VkDescriptorBufferInfo bi{refletUbo_[f].buf, 0, sizeof(FrameUBO)};
        VkDescriptorImageInfo si{shadowSampler_, shadowMap_.view, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};
        VkDescriptorImageInfo wi{clampSampler_, white.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet wr[3]{};
        for (auto& w : wr) { w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; w.dstSet = refletSet_[f]; w.descriptorCount = 1; }
        wr[0].dstBinding = 0; wr[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; wr[0].pBufferInfo = &bi;
        wr[1].dstBinding = 1; wr[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; wr[1].pImageInfo = &si;
        wr[2].dstBinding = 2; wr[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; wr[2].pImageInfo = &wi;
        vkUpdateDescriptorSets(d, 3, wr, 0, nullptr);
        quadVb_[f] = vk.createBuffer(sizeof(Vertex) * 4 * QUADS_MAX, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, true);
    }
    {   // WFS-09 : indices des quads transitoires (0-1-2, 0-2-3 par quad)
        std::vector<uint32_t> qi((size_t)QUADS_MAX * 6);
        for (uint32_t q = 0; q < (uint32_t)QUADS_MAX; q++) {
            const uint32_t k = q * 4, a[6] = {k, k + 1, k + 2, k, k + 2, k + 3};
            std::memcpy(&qi[(size_t)q * 6], a, sizeof a);
        }
        quadIb_ = vk.uploadBuffer(qi.data(), qi.size() * 4, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    }
    ETAPE_RENDU("ressources par image");
    VkDescriptorSetAllocateInfo dai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    dai.descriptorPool = pool_; dai.descriptorSetCount = 1; dai.pSetLayouts = &postLayout_;
    VK_OK(vkAllocateDescriptorSets(d, &dai, &postSet_));

    createTargets();
    ETAPE_RENDU("cibles de rendu");
    createPipelines();
    ETAPE_RENDU("pipelines");
    passes::creer(*this);   // WFS-150 (RG-01)
    {   // WFS-09 : halos additifs ; tache des phares (MAT.headPool du jeu web : couleur 0xfff0dc, multiplicative)
        Material h; h.flags = MAT_ADD | MAT_BLEND; haloMat_ = makeMaterial(h, nullptr, nullptr, nullptr);
        Material t; t.flags = MAT_POOL | MAT_BLEND;
        t.color = {std::pow(0xff / 255.f, 2.2f), std::pow(0xf0 / 255.f, 2.2f), std::pow(0xdc / 255.f, 2.2f), 1};
        poolMat = makeMaterial(t, nullptr, nullptr, nullptr);
    }
    {   // WFS-134 (RG-05) : ombre de contact des voitures qui n'en dessinent pas (adversaires, duel, police) : même quad que Game
        Material o; o.flags = MAT_SHADOWBLOB | MAT_BLEND; o.color = {0, 0, 0, 0.75f}; ombreMat_ = makeMaterial(o, nullptr, nullptr, nullptr);
        std::vector<Vertex> v = {{-0.5f, 0.02f, -0.5f, 0, 1, 0, 0, 0}, {0.5f, 0.02f, -0.5f, 0, 1, 0, 1, 0},
                                 {0.5f, 0.02f, 0.5f, 0, 1, 0, 1, 1}, {-0.5f, 0.02f, 0.5f, 0, 1, 0, 0, 1}};
        ombreQuad_ = makeMesh(v, {0, 1, 2, 0, 2, 3});
    }
    ETAPE_RENDU("matériaux, fin de Renderer::init");
    WFS_LOG("pipelines (WFS-262) : Renderer::init en %.0f ms, %u pipelines créés (2e lancement plus court si le cache est relu)", msDepuisInit(), pipelinesCrees_);
    return true;
}

// ---------- ombres du soleil (WFS-07) : carte de profondeur fixe (2048, 1024 sur mobile), jamais changée par image ----------
void Renderer::createShadows() {
    VkCtx& vk = *vk_; VkDevice d = vk.device;
#ifdef __ANDROID__
    shadowSize_ = 2048;   // WFS-227 : atlas de 2 × 2 cascades de 1024 (+ 6 Mo, D16) ; avant : 1024
#else
    shadowSize_ = 2048;
#endif
    VkFormatProperties fp; vkGetPhysicalDeviceFormatProperties(vk.gpu, SHADOW_FORMAT, &fp);
    const bool linear = (fp.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0;
    shadowMap_ = vk.createImage(shadowSize_, shadowSize_, 1, SHADOW_FORMAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
    VkSamplerCreateInfo sci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sci.magFilter = sci.minFilter = linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sci.addressModeU = sci.addressModeV = sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sci.compareEnable = VK_TRUE; sci.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;   // 1 = éclairé
    VK_OK(vkCreateSampler(d, &sci, nullptr, &shadowSampler_));

    VkAttachmentDescription at{};
    at.format = SHADOW_FORMAT; at.samples = VK_SAMPLE_COUNT_1_BIT;
    at.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; at.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    at.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; at.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    at.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; at.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    VkAttachmentReference ref{0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sp{};
    sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; sp.pDepthStencilAttachment = &ref;
    VkSubpassDependency dep[2]{};
    dep[0].srcSubpass = VK_SUBPASS_EXTERNAL; dep[0].dstSubpass = 0;
    dep[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dep[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dep[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT; dep[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dep[1].srcSubpass = 0; dep[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dep[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT; dep[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dep[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; dep[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    VkRenderPassCreateInfo rpci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    rpci.attachmentCount = 1; rpci.pAttachments = &at; rpci.subpassCount = 1; rpci.pSubpasses = &sp;
    rpci.dependencyCount = 2; rpci.pDependencies = dep;
    VK_OK(vkCreateRenderPass(d, &rpci, nullptr, &shadowPass_));
    VkFramebufferCreateInfo fci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fci.renderPass = shadowPass_; fci.attachmentCount = 1; fci.pAttachments = &shadowMap_.view;
    fci.width = fci.height = shadowSize_; fci.layers = 1;
    VK_OK(vkCreateFramebuffer(d, &fci, nullptr, &shadowFb_));

    // mesure du coût GPU de la passe (2 horodatages par image en vol)
    if (vk.props.limits.timestampComputeAndGraphics) {
        VkQueryPoolCreateInfo q{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
        q.queryType = VK_QUERY_TYPE_TIMESTAMP; q.queryCount = 5 * FRAMES;   // ombre, reflet (WFS-16), scène ; WFS-246 (RG-06) : fin de la passe finale (bloom, post, interface)
        VK_OK(vkCreateQueryPool(d, &q, nullptr, &tsPool_));
    }
    WFS_LOG("Ombres du soleil : carte %ux%u, filtre %s", shadowSize_, shadowSize_, linear ? "bilinéaire" : "au plus proche");
}

void Renderer::readShadowTiming() {
    if (!tsPool_ || !tsWritten_[cur_]) return;
    uint64_t t[5];
    if (vkGetQueryPoolResults(vk_->device, tsPool_, (uint32_t)cur_ * 5, 5, sizeof t, t, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT) != VK_SUCCESS) return;
    const double k = vk_->props.limits.timestampPeriod * 1e-6;
    shadowMsSum_ += (double)(t[1] - t[0]) * k;
    refletMsSum_ += (double)(t[2] - t[1]) * k;   // WFS-16 : passe de reflet (0 sans route mouillée)
    sceneMsSum_ += (double)(t[3] - t[2]) * k;    // WFS-42 : coût de la passe de scène, pour comparer avant / après
    { const double g = (double)(t[3] - t[0]) * k; if (g > 0 && g < 1000) gpuImageMs_ = gpuImageMs_ < 0 ? g : gpuImageMs_ * 0.9 + g * 0.1; }   // WFS-246
    {   // WFS-246 (RG-06) : temps GPU de chaque passe, lissé (ombre, reflet, scène, fin : bloom + post + interface)
        const double v[4] = {(double)(t[1] - t[0]) * k, (double)(t[2] - t[1]) * k, (double)(t[3] - t[2]) * k, (double)(t[4] - t[3]) * k};
        double* m[4] = {&gpuPasses_.ombre, &gpuPasses_.reflet, &gpuPasses_.scene, &gpuPasses_.post};
        for (int i = 0; i < 4; i++) if (v[i] >= 0 && v[i] < 1000) *m[i] = gpuPasses_.mesuree ? *m[i] * 0.9 + v[i] * 0.1 : v[i];
        gpuPasses_.mesuree = true;
    }
    if (++shadowMsN_ == 120) {
        WFS_LOG("Ombres : passe d'ombre %.2f ms GPU en moyenne (carte %u) ; passe de scène %.2f ms ; reflets %.2f ms (WFS-16)", shadowMsSum_ / shadowMsN_, shadowSize_,
                sceneMsSum_ / shadowMsN_, refletMsSum_ / shadowMsN_);
        shadowMsSum_ = 0; sceneMsSum_ = 0; refletMsSum_ = 0; shadowMsN_ = 0;
    }
}

// passe d'ombre : objets opaques vus du soleil (sauf sol, route, trottoirs : ils reçoivent l'ombre sans en projeter)
void Renderer::shadowPass(VkCommandBuffer cb) {
    if (tsPool_) { vkCmdResetQueryPool(cb, tsPool_, (uint32_t)cur_ * 5, 5); vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, tsPool_, (uint32_t)cur_ * 5); }
    VkClearValue clear{}; clear.depthStencil = {1.f, 0};
    VkRenderPassBeginInfo rbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rbi.renderPass = shadowPass_; rbi.framebuffer = shadowFb_; rbi.renderArea = {{0, 0}, {shadowSize_, shadowSize_}};
    rbi.clearValueCount = 1; rbi.pClearValues = &clear;
    vkCmdBeginRenderPass(cb, &rbi, VK_SUBPASS_CONTENTS_INLINE);
    if (shadowsOn_) {
        VkViewport vpt{0, 0, (float)shadowSize_, (float)shadowSize_, 0, 1};
        VkRect2D sc{{0, 0}, {shadowSize_, shadowSize_}};
        vkCmdSetViewport(cb, 0, 1, &vpt); vkCmdSetScissor(cb, 0, 1, &sc);
        vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, sceneLayout_, 0, 1, &fr_[cur_].frameSet, 0, nullptr);
        constexpr uint32_t NO_CAST = MAT_SHADOWBLOB | MAT_ROAD | MAT_WALK | MAT_GROUND | MAT_BLEND;
        VkPipeline bound = VK_NULL_HANDLE;
        // WFS-227 (accroche E13) : une tuile de l'atlas par cascade (2 × 2 ; une cascade : image entière), cascade dans pc.emissive.x
        const uint32_t tu = cascades_.n > 1 ? shadowSize_ / 2 : shadowSize_;
        for (int k = 0; k < cascades_.n; k++) {
        VkViewport vt{(float)(tu * (k % 2)), (float)(tu * (k / 2)), (float)tu, (float)tu, 0, 1}; VkRect2D st{{(int32_t)(tu * (k % 2)), (int32_t)(tu * (k / 2))}, {tu, tu}};
        vkCmdSetViewport(cb, 0, 1, &vt); vkCmdSetScissor(cb, 0, 1, &st);
        for (Pending p : opaqueQ_) {
            if ((p.mat->flags & NO_CAST) || !(p.vis & (VIS_CASCADE0 << k))) continue;   // WFS-82, WFS-227 : hors de cette cascade
            const bool masque = p.mat->flags & MAT_MASK;
            VkPipeline want = p.osOfs >= 0 ? (masque ? skinShadowMask_ : skinShadow_) : (masque ? shadowMaskPipe_ : shadowPipe_);   // WFS-63
            if (want != bound) { vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, want); bound = want; }
            p.pc.emissive.x = (float)k;
            flushDraw(p);
        }
        }
    }
    vkCmdEndRenderPass(cb);
    if (tsPool_) vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, tsPool_, (uint32_t)cur_ * 5 + 1);
}

void Renderer::rotationMatrix(float r[4]) const {
    switch (vk_->transform) {
        case VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR:  r[0] = 0; r[1] = 1; r[2] = -1; r[3] = 0; break;
        case VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR: r[0] = -1; r[1] = 0; r[2] = 0; r[3] = -1; break;
        case VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR: r[0] = 0; r[1] = -1; r[2] = 1; r[3] = 0; break;
        default: r[0] = 1; r[1] = 0; r[2] = 0; r[3] = 1; break;
    }
}

// ---------- WFS-100 (RG-04, ANO-27) : bloom « dual filter » (UnrealBloomPass du jeu web, js/world/renderer.js l. 652) ----------
// Seuil (1,0, genou 0,5) et réduction de moitié dans v[0], puis pour chacun des 5 niveaux (½, ¼… 1/32 de la scène) : flou
// gaussien séparable à 9 prises (horizontal → h[i], vertical → v[i]) ; le flou horizontal du niveau i lit v[i − 1] (réduction
// bilinéaire). Remontée : h[i] = v[i] × poids + h[i + 1] agrandi (tente 3 × 3), du plus petit au plus grand ; la passe finale
// lit h[0]. Échantillonneur CLAMP_TO_EDGE partout : plus de copie fantôme repliée depuis le bord opposé de l'image.
struct Renderer::Bloom {
    static constexpr int N = 5;
    VkRenderPass pass = VK_NULL_HANDLE;
    VkPipeline pipe = VK_NULL_HANDLE;
    Image h[N], v[N];
    VkFramebuffer fbH[N]{}, fbV[N]{};
    uint32_t w[N]{}, ht[N]{};
    VkDescriptorSet pre = VK_NULL_HANDLE, setH[N]{}, setV[N]{}, setUp[N]{};
    float force = 0;   // force de l'image (0 : chaîne non dessinée)
    // UnrealBloomPass : bloomFactors [1,0 ; 0,8 ; 0,6 ; 0,4 ; 0,2] mélangés au rayon 0,45 (lerpBloomFactor : mix(f, 1,2 − f, r))
    static float poids(int i) { const float f = 1.0f - 0.2f * (float)i, r = 0.45f; return f + (1.2f - f - f) * r; }

    void creer(Renderer& R) {
        VkCtx& vk = *R.vk_; VkDevice d = vk.device;
        if (!pass) {
            VkAttachmentDescription at{};
            at.format = HDR_FORMAT; at.samples = VK_SAMPLE_COUNT_1_BIT;
            at.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; at.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            at.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; at.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            at.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; at.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            VkAttachmentReference ref{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
            VkSubpassDescription sp{};
            sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; sp.colorAttachmentCount = 1; sp.pColorAttachments = &ref;
            VkSubpassDependency dep[2]{};
            dep[0].srcSubpass = VK_SUBPASS_EXTERNAL; dep[0].dstSubpass = 0;
            dep[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dep[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dep[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dep[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dep[1].srcSubpass = 0; dep[1].dstSubpass = VK_SUBPASS_EXTERNAL;
            dep[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; dep[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dep[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; dep[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            VkRenderPassCreateInfo rpci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
            rpci.attachmentCount = 1; rpci.pAttachments = &at; rpci.subpassCount = 1; rpci.pSubpasses = &sp;
            rpci.dependencyCount = 2; rpci.pDependencies = dep;
            VK_OK(vkCreateRenderPass(d, &rpci, nullptr, &pass));
            auto mod = [&](const uint32_t* code, size_t bytes) {
                VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
                ci.codeSize = bytes; ci.pCode = code;
                VkShaderModule m; VK_OK(vkCreateShaderModule(d, &ci, nullptr, &m));
                R.modules_.push_back(m); return m;
            };
            pipe = R.makePipeline(mod(SPV_POST_VERT, sizeof(SPV_POST_VERT)), mod(SPV_POST_FRAG, sizeof(SPV_POST_FRAG)), 0, false, false, false, false,
                                  pass, VK_SAMPLE_COUNT_1_BIT, R.postPLayout_);
            VkDescriptorSetLayout l[4 * N];
            for (auto& x : l) x = R.postLayout_;
            VkDescriptorSetAllocateInfo dai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            dai.descriptorPool = R.pool_; dai.descriptorSetCount = 1; dai.pSetLayouts = l;
            VK_OK(vkAllocateDescriptorSets(d, &dai, &pre));
            dai.descriptorSetCount = N;
            VK_OK(vkAllocateDescriptorSets(d, &dai, setH)); VK_OK(vkAllocateDescriptorSets(d, &dai, setV)); VK_OK(vkAllocateDescriptorSets(d, &dai, setUp));
        }
        VkCommandBuffer cb = vk.beginOneShot();
        for (int i = 0; i < N; i++) {
            w[i] = std::max(1u, R.cw_ >> (i + 1)); ht[i] = std::max(1u, R.ch_ >> (i + 1));   // WFS-246 (RG-07) : capacité
            for (Image* im : {&h[i], &v[i]}) {
                *im = vk.createImage(w[i], ht[i], 1, HDR_FORMAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
                // lisible dès le départ (bloom coupé : la passe finale garde un descripteur valide)
                VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
                b.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                b.image = im->img; b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
                b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
            }
        }
        vk.endOneShot(cb);
        for (int i = 0; i < N; i++) {
            VkFramebufferCreateInfo fci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            fci.renderPass = pass; fci.attachmentCount = 1; fci.width = w[i]; fci.height = ht[i]; fci.layers = 1;
            fci.pAttachments = &h[i].view; VK_OK(vkCreateFramebuffer(d, &fci, nullptr, &fbH[i]));
            fci.pAttachments = &v[i].view; VK_OK(vkCreateFramebuffer(d, &fci, nullptr, &fbV[i]));
        }
        auto ecrire = [&](VkDescriptorSet s, VkImageView a, VkImageView b) {
            VkDescriptorImageInfo ii[2] = {{R.clampSampler_, a, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
                                           {R.clampSampler_, b ? b : R.white.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
            VkWriteDescriptorSet wr[2]{};
            for (int k = 0; k < 2; k++) {
                wr[k].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; wr[k].dstSet = s; wr[k].dstBinding = (uint32_t)k;
                wr[k].descriptorCount = 1; wr[k].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; wr[k].pImageInfo = &ii[k];
            }
            vkUpdateDescriptorSets(d, 2, wr, 0, nullptr);
        };
        ecrire(pre, R.hdr_.view, VK_NULL_HANDLE);
        for (int i = 0; i < N; i++) {
            ecrire(setH[i], v[i ? i - 1 : 0].view, VK_NULL_HANDLE);
            ecrire(setV[i], h[i].view, VK_NULL_HANDLE);
            ecrire(setUp[i], v[i].view, i + 1 < N ? h[i + 1].view : VK_NULL_HANDLE);
        }
        ecrire(R.postSet_, R.hdr_.view, h[0].view);   // passe finale : scène + bloom remonté
    }

    void detruireCibles(Renderer& R) {
        VkDevice d = R.vk_->device;
        for (int i = 0; i < N; i++) {
            if (fbH[i]) vkDestroyFramebuffer(d, fbH[i], nullptr);
            if (fbV[i]) vkDestroyFramebuffer(d, fbV[i], nullptr);
            fbH[i] = fbV[i] = VK_NULL_HANDLE;
            R.vk_->destroy(h[i]); R.vk_->destroy(v[i]);
        }
    }

    void liberer(Renderer& R) {
        VkDevice d = R.vk_->device;
        detruireCibles(R);
        if (pipe) vkDestroyPipeline(d, pipe, nullptr);
        if (pass) vkDestroyRenderPass(d, pass, nullptr);
        pipe = VK_NULL_HANDLE; pass = VK_NULL_HANDLE;
    }

    // une passe plein écran : source(s) du set, mode et paramètres du shader post.frag
    void passe(Renderer& R, VkCommandBuffer cb, VkFramebuffer fb, int i, VkDescriptorSet s, float mode, float poidsNiveau, float z) {
        VkRenderPassBeginInfo rbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rbi.renderPass = pass; rbi.framebuffer = fb; rbi.renderArea = {{0, 0}, {w[i], ht[i]}};
        vkCmdBeginRenderPass(cb, &rbi, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport vpt{0, 0, (float)w[i], (float)ht[i], 0, 1};
        VkRect2D sc{{0, 0}, {w[i], ht[i]}};
        vkCmdSetViewport(cb, 0, 1, &vpt); vkCmdSetScissor(cb, 0, 1, &sc);
        vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, R.postPLayout_, 0, 1, &s, 0, nullptr);
        struct { float rot[4]; float params[4]; float bloom[4]; } pp{{1, 0, 0, 1}, {R.exposure_, 0, 0, 0}, {mode, poidsNiveau, z, mode == 1 ? R.echelleDyn_ : 0.f}};   // WFS-246 : bloom.w = part de la scène lue
        vkCmdPushConstants(cb, R.postPLayout_, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof pp, &pp);
        vkCmdDraw(cb, 3, 1, 0, 0);
        vkCmdEndRenderPass(cb);
    }

    void dessiner(Renderer& R, VkCommandBuffer cb) {
        if (force <= 0.f) return;
        vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
        passe(R, cb, fbV[0], 0, pre, 1, 0, 0);                       // seuil + réduction de moitié
        for (int i = 0; i < N; i++) {                                  // descente : flou séparable à 9 prises par niveau
            passe(R, cb, fbH[i], i, setH[i], 2, 0, i ? 2.f : 1.f);    // pas en texels de la source (2 : source deux fois plus grande)
            passe(R, cb, fbV[i], i, setV[i], 3, 0, 1.f);
        }
        for (int i = N - 1; i >= 0; i--) passe(R, cb, fbH[i], i, setUp[i], 4, poids(i), i + 1 < N ? 1.f : 0.f);   // remontée
    }
};

void Renderer::createTargets() {
    VkCtx& vk = *vk_; VkDevice d = vk.device;
    sceneValide_ = false;   // WFS-329 : nouvelles cibles, scène à redessiner avant de pouvoir la figer
    bool rotated = vk.transform == VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR || vk.transform == VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR;
    lw_ = rotated ? vk.swapExtent.height : vk.swapExtent.width;
    lh_ = rotated ? vk.swapExtent.width : vk.swapExtent.height;
    // scène 3D éventuellement rendue en plus petit (GPU lents), agrandie par la passe finale
    // WFS-246 (RG-07) : cibles de la scène créées à la CAPACITÉ (échelle du niveau, renderScale) ; la résolution dynamique ne
    // change ensuite que la zone dessinée (sw_ × sh_, echelleDynamique) : aucune recréation ni attente du GPU en course
    cw_ = std::max(1u, (uint32_t)(lw_ * vk.renderScale)); ch_ = std::max(1u, (uint32_t)(lh_ * vk.renderScale));
    echelleDynamique(echelleDyn_);
    const bool ms = vk.msaa != VK_SAMPLE_COUNT_1_BIT;

    hdr_ = vk.createImage(cw_, ch_, 1, HDR_FORMAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);   // WFS-150 : relue par le banc (HDR)
    if (ms) msaaColor_ = vk.createImage(cw_, ch_, 1, HDR_FORMAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, vk.msaa, VK_IMAGE_ASPECT_COLOR_BIT, true);
    depth_ = vk.createImage(cw_, ch_, 1, vk.depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, vk.msaa, VK_IMAGE_ASPECT_DEPTH_BIT, true);

    if (!scenePass_) {
        // ---------- passe de scène : couleur HDR (MSAA) + profondeur, résolue dans hdr_ ----------
        VkAttachmentDescription at[3]{};
        VkAttachmentReference colRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}, depRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL},
                              resRef{2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        at[0].format = HDR_FORMAT; at[0].samples = vk.msaa;
        at[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; at[0].storeOp = ms ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
        at[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; at[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        at[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        at[0].finalLayout = ms ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        at[1].format = vk.depthFormat; at[1].samples = vk.msaa;
        at[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; at[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        at[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; at[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        at[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; at[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        at[2].format = HDR_FORMAT; at[2].samples = VK_SAMPLE_COUNT_1_BIT;
        at[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; at[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        at[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; at[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        at[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; at[2].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkSubpassDescription sp{};
        sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sp.colorAttachmentCount = 1; sp.pColorAttachments = &colRef; sp.pDepthStencilAttachment = &depRef;
        if (ms) sp.pResolveAttachments = &resRef;
        VkSubpassDependency dep[2]{};
        dep[0].srcSubpass = VK_SUBPASS_EXTERNAL; dep[0].dstSubpass = 0;
        dep[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dep[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dep[1].srcSubpass = 0; dep[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dep[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; dep[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dep[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; dep[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        VkRenderPassCreateInfo rpci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        rpci.attachmentCount = ms ? 3 : 2; rpci.pAttachments = at;
        rpci.subpassCount = 1; rpci.pSubpasses = &sp; rpci.dependencyCount = 2; rpci.pDependencies = dep;
        VK_OK(vkCreateRenderPass(d, &rpci, nullptr, &scenePass_));

        // ---------- passe finale : image de la swapchain ----------
        VkAttachmentDescription fa{};
        fa.format = vk.swapFormat; fa.samples = VK_SAMPLE_COUNT_1_BIT;
        fa.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; fa.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        fa.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; fa.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        fa.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        fa.finalLayout = vk.offscreen ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        VkAttachmentReference fr{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkSubpassDescription fsp{};
        fsp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; fsp.colorAttachmentCount = 1; fsp.pColorAttachments = &fr;
        VkSubpassDependency fd{};
        fd.srcSubpass = VK_SUBPASS_EXTERNAL; fd.dstSubpass = 0;
        fd.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; fd.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        fd.srcAccessMask = 0; fd.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        rpci.attachmentCount = 1; rpci.pAttachments = &fa; rpci.pSubpasses = &fsp; rpci.dependencyCount = 1; rpci.pDependencies = &fd;
        VK_OK(vkCreateRenderPass(d, &rpci, nullptr, &finalPass_));
    }

    VkImageView views[3] = {ms ? msaaColor_.view : hdr_.view, depth_.view, hdr_.view};
    VkFramebufferCreateInfo fci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fci.renderPass = scenePass_; fci.attachmentCount = ms ? 3 : 2; fci.pAttachments = views;
    fci.width = cw_; fci.height = ch_; fci.layers = 1;
    VK_OK(vkCreateFramebuffer(d, &fci, nullptr, &sceneFb_));
    finalFbs_.resize(vk.swapViews.size());
    for (size_t i = 0; i < vk.swapViews.size(); i++) {
        fci.renderPass = finalPass_; fci.attachmentCount = 1; fci.pAttachments = &vk.swapViews[i];
        fci.width = vk.swapExtent.width; fci.height = vk.swapExtent.height;
        VK_OK(vkCreateFramebuffer(d, &fci, nullptr, &finalFbs_[i]));
    }
    VkDescriptorImageInfo ii{clampSampler_, hdr_.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet wr{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    wr.dstSet = postSet_; wr.dstBinding = 0; wr.descriptorCount = 1; wr.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; wr.pImageInfo = &ii;
    vkUpdateDescriptorSets(d, 1, &wr, 0, nullptr);
    createReflets();
    if (!bloom_) bloom_ = new Bloom();   // WFS-100 : chaîne du bloom (écrit aussi le bloom dans postSet_)
    bloom_->creer(*this);
    WFS_LOG("Cibles de rendu : %ux%u (logique), scène %ux%u, MSAA x%d", lw_, lh_, cw_, ch_, (int)vk.msaa);
}

// WFS-246 (RG-07) : résolution dynamique sans recréation : zone dessinée = e × capacité, plus 2 pixels de vraie image au bord
// (les lectures bilinéaires de la passe finale et du bloom, jusqu'à e × capacité, ne touchent jamais un pixel non dessiné ;
// champ rogné de 2 / sw_ ≈ 0,15 %) ; e = 1 : toute la cible, image identique à avant
void Renderer::echelleDynamique(float e) {
    const float avant = echelleDyn_;
    echelleDyn_ = e >= 0.999f ? 1.f : std::clamp(e, 0.1f, 1.f);
    if (echelleDyn_ != avant) sceneValide_ = false;   // WFS-329 : autre part de la cible dessinée : fond figé à redessiner
    if (echelleDyn_ >= 1.f || !cw_) { sw_ = cw_; sh_ = ch_; return; }
    sw_ = std::min(cw_, (uint32_t)std::ceil(cw_ * echelleDyn_) + 2); sh_ = std::min(ch_, (uint32_t)std::ceil(ch_ * echelleDyn_) + 2);
}

// ---------- WFS-16 : passe de reflet (scène vue dans le miroir de la chaussée), demi-résolution, une cible par image en vol ----------
// Image n : la passe miroir écrit refletColor_[n], puis la passe de scène la lit : jamais lue et écrite dans la même passe.
void Renderer::createReflets() {
    VkCtx& vk = *vk_; VkDevice d = vk.device;
    rw_ = std::max(1u, cw_ / 2); rh_ = std::max(1u, ch_ / 2);   // WFS-246 (RG-07) : capacité (lu en coordonnées d'écran normalisées)
    if (!refletPass_) {
        VkAttachmentDescription at[2]{};
        at[0].format = HDR_FORMAT; at[0].samples = VK_SAMPLE_COUNT_1_BIT;
        at[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; at[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        at[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; at[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        at[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; at[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        at[1].format = vk.depthFormat; at[1].samples = VK_SAMPLE_COUNT_1_BIT;
        at[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; at[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        at[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; at[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        at[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; at[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAttachmentReference colRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}, depRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription sp{};
        sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sp.colorAttachmentCount = 1; sp.pColorAttachments = &colRef; sp.pDepthStencilAttachment = &depRef;
        VkSubpassDependency dep[2]{};
        dep[0].srcSubpass = VK_SUBPASS_EXTERNAL; dep[0].dstSubpass = 0;
        dep[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dep[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dep[1].srcSubpass = 0; dep[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dep[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; dep[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dep[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; dep[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        VkRenderPassCreateInfo rpci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        rpci.attachmentCount = 2; rpci.pAttachments = at; rpci.subpassCount = 1; rpci.pSubpasses = &sp;
        rpci.dependencyCount = 2; rpci.pDependencies = dep;
        VK_OK(vkCreateRenderPass(d, &rpci, nullptr, &refletPass_));
    }
    refletDepth_ = vk.createImage(rw_, rh_, 1, vk.depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, true);
    VkCommandBuffer cb = vk.beginOneShot();
    for (int f = 0; f < FRAMES; f++) {
        refletColor_[f] = vk.createImage(rw_, rh_, 1, HDR_FORMAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        // lisible dès le départ (aucune passe de reflet le jour) : passage en lecture seule
        VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        b.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        b.image = refletColor_[f].img; b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
    }
    vk.endOneShot(cb);
    for (int f = 0; f < FRAMES; f++) {
        VkImageView v[2] = {refletColor_[f].view, refletDepth_.view};
        VkFramebufferCreateInfo fci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fci.renderPass = refletPass_; fci.attachmentCount = 2; fci.pAttachments = v; fci.width = rw_; fci.height = rh_; fci.layers = 1;
        VK_OK(vkCreateFramebuffer(d, &fci, nullptr, &refletFb_[f]));
        VkDescriptorImageInfo ii{clampSampler_, refletColor_[f].view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        w.dstSet = fr_[f].frameSet; w.dstBinding = 2; w.descriptorCount = 1; w.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; w.pImageInfo = &ii;
        vkUpdateDescriptorSets(d, 1, &w, 0, nullptr);
    }
}

void Renderer::destroyReflets() {
    VkDevice d = vk_->device;
    for (int f = 0; f < FRAMES; f++) {
        if (refletFb_[f]) vkDestroyFramebuffer(d, refletFb_[f], nullptr);
        refletFb_[f] = VK_NULL_HANDLE;
        vk_->destroy(refletColor_[f]);
    }
    vk_->destroy(refletDepth_);
}

// passe miroir : ciel, objets opaques (sauf route, trottoirs, sol, ombres de contact, personnages), puis transparents
// (vitres, halos) ; la tache des phares n'y est pas (elle est sur la chaussée)
void Renderer::refletPass(VkCommandBuffer cb) {
    if (!refletsOn_) return;
    VkClearValue clears[2]{};
    clears[0].color = {{0, 0, 0, 1}}; clears[1].depthStencil = {0.f, 0};
    VkRenderPassBeginInfo bi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    bi.renderPass = refletPass_; bi.framebuffer = refletFb_[cur_]; bi.renderArea = {{0, 0}, {rw_, rh_}};
    bi.clearValueCount = 2; bi.pClearValues = clears;
    vkCmdBeginRenderPass(cb, &bi, VK_SUBPASS_CONTENTS_INLINE);
    VkViewport vpt{0, 0, (float)rw_, (float)rh_, 0, 1};
    VkRect2D sc{{0, 0}, {rw_, rh_}};
    vkCmdSetViewport(cb, 0, 1, &vpt); vkCmdSetScissor(cb, 0, 1, &sc);
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, sceneLayout_, 0, 1, &refletSet_[cur_], 0, nullptr);
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, refSky_);
    vkCmdDraw(cb, 3, 1, 0, 0);
    constexpr uint32_t SANS = MAT_ROAD | MAT_WALK | MAT_GROUND | MAT_SHADOWBLOB | MAT_POOL;
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, refOpaque_);
    for (const Pending& p : opaqueQ_) if (p.osOfs < 0 && !(p.mat->flags & SANS) && (p.vis & VIS_REFLET)) flushDraw(p);   // WFS-82
    VkPipeline lie = VK_NULL_HANDLE;
    for (const Pending& p : transparent_) {
        if ((p.mat->flags & SANS) || !(p.vis & VIS_REFLET)) continue;   // WFS-82
        const VkPipeline voulu = (p.mat->flags & MAT_ADD) ? refAdd_ : refBlend_;
        if (voulu != lie) { vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, voulu); lie = voulu; }
        flushDraw(p);
    }
    vkCmdEndRenderPass(cb);
}

void Renderer::destroyTargets() {
    VkDevice d = vk_->device;
    for (auto fb : finalFbs_) vkDestroyFramebuffer(d, fb, nullptr);
    finalFbs_.clear();
    if (sceneFb_) vkDestroyFramebuffer(d, sceneFb_, nullptr);
    sceneFb_ = VK_NULL_HANDLE;
    vk_->destroy(hdr_); vk_->destroy(msaaColor_); vk_->destroy(depth_);
    destroyReflets();
    if (bloom_) bloom_->detruireCibles(*this);   // WFS-100
}

bool Renderer::resize() {
    recreations_++;   // WFS-246 (RG-07) : compté par le banc perf (aucune pendant la résolution dynamique)
    const uint32_t pipelinesAvant = pipelinesCrees_; const auto t0 = std::chrono::steady_clock::now();   // WFS-262
    vk_->waitIdle();
    destroyTargets();
    const bool refaire = appliquerNettete();   // WFS-144 : MSAA / anisotrope du niveau (Basse : ×2 / ×4)
    createTargets();
    if (refaire) createScenePipelines();
    passes::redimensionner(*this);   // WFS-150 : cibles des passes à la taille de la scène
    // WFS-262 (RG-02) : changement de qualité (menu, pause) : pipelines repris du cache, durée au journal
    if (pipelinesCrees_ != pipelinesAvant) WFS_LOG("pipelines (WFS-262 RG-02) : %u pipeline(s) repris du cache en %.0f ms (changement de qualité)", pipelinesCrees_ - pipelinesAvant,
                                                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count());
    return true;
}

VkPipeline Renderer::makePipeline(VkShaderModule vs, VkShaderModule fs, int vertexKind, bool blend, bool premul, bool depthTest, bool depthWrite,
                                  VkRenderPass rp, VkSampleCountFlagBits samples, VkPipelineLayout layout, bool depthOnly, int melange,
                                  bool sansDiscard) {
    VkPipelineShaderStageCreateInfo st[2]{};
    st[0].sType = st[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    st[0].stage = VK_SHADER_STAGE_VERTEX_BIT; st[0].module = vs; st[0].pName = "main";
    st[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT; st[1].module = fs; st[1].pName = "main";
    // WFS-222 : constante de spécialisation 0 (SANS_DISCARD, booléen 32 bits) : shader sans discard → early-Z
    // WFS-263 (RG-01) : constante 1 (LEGER) : variante légère de scene.frag (legerPipeline_)
    const VkBool32 valeurs[2] = {sansDiscard ? VK_TRUE : VK_FALSE, legerPipeline_ ? VK_TRUE : VK_FALSE};
    const VkSpecializationMapEntry entrees[2] = {{0, 0, sizeof(VkBool32)}, {1, sizeof(VkBool32), sizeof(VkBool32)}};
    const VkSpecializationInfo spec{2, entrees, sizeof valeurs, valeurs};
    if (sansDiscard || legerPipeline_) st[1].pSpecializationInfo = &spec;

    VkVertexInputBindingDescription bind{};
    VkVertexInputAttributeDescription attr[6]{};
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    if (vertexKind == 1) {   // scène : position, normale, uv
        bind = {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
        attr[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0};
        attr[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12};
        attr[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, 24};
        attr[3] = {3, 0, VK_FORMAT_R8G8B8A8_SNORM, 32};   // WFS-149 (RG-04) : tangente (format obligatoire des tampons de sommets)
        vi.vertexBindingDescriptionCount = 1; vi.pVertexBindingDescriptions = &bind;
        vi.vertexAttributeDescriptionCount = 4; vi.pVertexAttributeDescriptions = attr;
    } else if (vertexKind == 3) {   // WFS-63 : sommet à squelette (position, normale, uv, 4 os, 4 poids)
        bind = {0, sizeof(SkinVertex), VK_VERTEX_INPUT_RATE_VERTEX};
        attr[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0};
        attr[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12};
        attr[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, 24};
        attr[3] = {3, 0, VK_FORMAT_R8G8B8A8_UINT, 32};
        attr[4] = {4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 36};
        attr[5] = {5, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 52};   // WFS-72 : _cloth du jeu web
        vi.vertexBindingDescriptionCount = 1; vi.pVertexBindingDescriptions = &bind;
        vi.vertexAttributeDescriptionCount = 6; vi.pVertexAttributeDescriptions = attr;
    } else if (vertexKind == 2) {   // interface : position, uv, couleur
        bind = {0, sizeof(UiVertex), VK_VERTEX_INPUT_RATE_VERTEX};
        attr[0] = {0, 0, VK_FORMAT_R32G32_SFLOAT, 0};
        attr[1] = {1, 0, VK_FORMAT_R32G32_SFLOAT, 8};
        attr[2] = {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 16};
        vi.vertexBindingDescriptionCount = 1; vi.pVertexBindingDescriptions = &bind;
        vi.vertexAttributeDescriptionCount = 3; vi.pVertexAttributeDescriptions = attr;
    }
    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vp.viewportCount = 1; vp.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode = VK_POLYGON_MODE_FILL; rs.cullMode = VK_CULL_MODE_NONE; rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; rs.lineWidth = 1.f;
    if (cullPipeline_) {   // WFS-246 : faces arrière retirées ; orientation des faces avant réglable au banc (WFS_FACE_AVANT=cw)
        static const bool cw = std::getenv("WFS_FACE_AVANT") && std::strcmp(std::getenv("WFS_FACE_AVANT"), "cw") == 0;
        rs.cullMode = VK_CULL_MODE_BACK_BIT; rs.frontFace = cw ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = samples;
    VkPipelineDepthStencilStateCreateInfo ds{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    ds.depthTestEnable = depthTest; ds.depthWriteEnable = depthWrite;
    ds.depthCompareOp = depthOnly ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_GREATER_OR_EQUAL;   // profondeur inversée (sauf carte d'ombre)
    VkPipelineColorBlendAttachmentState cb{};
    cb.colorWriteMask = 0xF;
    if (blend) {
        cb.blendEnable = VK_TRUE;
        cb.srcColorBlendFactor = premul ? VK_BLEND_FACTOR_ONE : VK_BLEND_FACTOR_SRC_ALPHA;
        cb.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; cb.colorBlendOp = VK_BLEND_OP_ADD;
        cb.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; cb.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; cb.alphaBlendOp = VK_BLEND_OP_ADD;
        // WFS-09 : 1 = multiplicatif (sol × (1 + source), MAT.headPool : DstColor + One), 2 = additif (halos)
        if (melange == 1) { cb.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR; cb.dstColorBlendFactor = VK_BLEND_FACTOR_ONE; }
        if (melange == 2) { cb.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; cb.dstColorBlendFactor = VK_BLEND_FACTOR_ONE; }
        if (melange) { cb.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; cb.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE; }
    }
    VkPipelineColorBlendStateCreateInfo cbs{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cbs.attachmentCount = depthOnly ? 0 : 1; cbs.pAttachments = &cb;
    VkDynamicState dyn[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dy{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dy.dynamicStateCount = 2; dy.pDynamicStates = dyn;
    VkGraphicsPipelineCreateInfo gp{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    gp.stageCount = fs ? 2 : 1; gp.pStages = st; gp.pVertexInputState = &vi; gp.pInputAssemblyState = &ia;
    gp.pViewportState = &vp; gp.pRasterizationState = &rs; gp.pMultisampleState = &ms; gp.pDepthStencilState = &ds;
    gp.pColorBlendState = &cbs; gp.pDynamicState = &dy; gp.layout = layout; gp.renderPass = rp; gp.subpass = 0;
    VkPipeline p; VK_OK(vkCreateGraphicsPipelines(vk_->device, cachePipelines_, 1, &gp, nullptr, &p));   // WFS-262 (RG-01) : cache
    pipelinesCrees_++;   // WFS-262 (RG-02) : compteur (0 attendu pendant la course)
    ETAPE_RENDU("un pipeline");   // WFS-88 : MoltenVK traduit les shaders en Metal ici
    return p;
}

void Renderer::createPipelines() {
    VkDevice d = vk_->device;
    auto mod = [&](const uint32_t* code, size_t bytes) {
        VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        ci.codeSize = bytes; ci.pCode = code;
        VkShaderModule m; VK_OK(vkCreateShaderModule(d, &ci, nullptr, &m));
        modules_.push_back(m); return m;
    };
    VkShaderModule sv = mod(SPV_SCENE_VERT, sizeof(SPV_SCENE_VERT)), sf = mod(SPV_SCENE_FRAG, sizeof(SPV_SCENE_FRAG));
    VkShaderModule kv = mod(SPV_SKY_VERT, sizeof(SPV_SKY_VERT)), kf = mod(SPV_SKY_FRAG, sizeof(SPV_SKY_FRAG));
    VkShaderModule pv = mod(SPV_POST_VERT, sizeof(SPV_POST_VERT)), pf = mod(SPV_POST_FRAG, sizeof(SPV_POST_FRAG));
    VkShaderModule uv = mod(SPV_UI_VERT, sizeof(SPV_UI_VERT)), uf = mod(SPV_UI_FRAG, sizeof(SPV_UI_FRAG));
    VkShaderModule kv2 = mod(SPV_SCENE_SKIN_VERT, sizeof(SPV_SCENE_SKIN_VERT)), kh = mod(SPV_SHADOW_SKIN_VERT, sizeof(SPV_SHADOW_SKIN_VERT));
    VkShaderModule kf2 = mod(SPV_SCENE_SKIN_FRAG, sizeof(SPV_SCENE_SKIN_FRAG));   // WFS-72 : vêtements (pedMaterial)
    modScene_[0] = sv; modScene_[1] = sf; modScene_[2] = kv; modScene_[3] = kf; modScene_[4] = kv2; modScene_[5] = kf2;
    createScenePipelines();   // WFS-144 : refaites quand le MSAA change (appliquerNettete)
    prechaufferMsaa();   // WFS-262 (RG-02)
    post_ = makePipeline(pv, pf, 0, false, false, false, false, finalPass_, VK_SAMPLE_COUNT_1_BIT, postPLayout_);
    ui_ = makePipeline(uv, uf, 2, true, true, false, false, finalPass_, VK_SAMPLE_COUNT_1_BIT, postPLayout_);
    VkShaderModule hv = mod(SPV_SHADOW_VERT, sizeof(SPV_SHADOW_VERT)), hf = mod(SPV_SHADOW_FRAG, sizeof(SPV_SHADOW_FRAG));
    shadowPipe_ = makePipeline(hv, VK_NULL_HANDLE, 1, false, false, true, true, shadowPass_, VK_SAMPLE_COUNT_1_BIT, sceneLayout_, true);
    shadowMaskPipe_ = makePipeline(hv, hf, 1, false, false, true, true, shadowPass_, VK_SAMPLE_COUNT_1_BIT, sceneLayout_, true);
    // WFS-63 : maillages à squelette (même scene.frag / shadow.frag que les objets rigides)
    skinShadow_ = makePipeline(kh, VK_NULL_HANDLE, 3, false, false, true, true, shadowPass_, VK_SAMPLE_COUNT_1_BIT, skinLayout_, true);
    skinShadowMask_ = makePipeline(kh, hf, 3, false, false, true, true, shadowPass_, VK_SAMPLE_COUNT_1_BIT, skinLayout_, true);
    // WFS-16 : passe de reflet (1 échantillon)
    const VkSampleCountFlagBits s1 = VK_SAMPLE_COUNT_1_BIT;
    refOpaque_ = makePipeline(sv, sf, 1, false, false, true, true, refletPass_, s1, sceneLayout_);
    refBlend_ = makePipeline(sv, sf, 1, true, false, true, false, refletPass_, s1, sceneLayout_);
    refSky_ = makePipeline(kv, kf, 0, false, false, false, false, refletPass_, s1, sceneLayout_);
    refAdd_ = makePipeline(sv, sf, 1, true, false, true, false, refletPass_, s1, sceneLayout_, false, 2);
}

// pipelines dessinés dans la passe de scène, au nombre d'échantillons du MSAA courant (vk.msaa)
void Renderer::createScenePipelines() {
    const VkShaderModule sv = modScene_[0], sf = modScene_[1], kv = modScene_[2], kf = modScene_[3], kv2 = modScene_[4], kf2 = modScene_[5];
    const VkSampleCountFlagBits s = vk_->msaa;
    // WFS-222 : opaques sans découpe : shader spécialisé sans discard (test de profondeur avant le shader, LRZ / early-Z des
    // GPU mobiles) ; avec découpe (alpha masqué, fondu des voitures) : shader complet, comme avant
    opaque_ = makePipeline(sv, sf, 1, false, false, true, true, scenePass_, s, sceneLayout_, false, 0, true);
    opaqueDecoupe_ = makePipeline(sv, sf, 1, false, false, true, true, scenePass_, s, sceneLayout_);
    blend_ = makePipeline(sv, sf, 1, true, false, true, false, scenePass_, s, sceneLayout_);
    sky_ = makePipeline(kv, kf, 0, false, false, false, false, scenePass_, s, sceneLayout_);
    skyApres_ = makePipeline(kv, kf, 0, false, false, true, false, scenePass_, s, sceneLayout_);   // WFS-222 : profondeur 0 = vide
    skinOpaque_ = makePipeline(kv2, kf2, 3, false, false, true, true, scenePass_, s, skinLayout_, false, 0, true);   // WFS-63 ; WFS-222
    skinOpaqueDecoupe_ = makePipeline(kv2, kf2, 3, false, false, true, true, scenePass_, s, skinLayout_);
    cullPipeline_ = 1;   // WFS-246 (levier 1)
    opaqueCull_ = makePipeline(sv, sf, 1, false, false, true, true, scenePass_, s, sceneLayout_, false, 0, true);
    skinOpaqueCull_ = makePipeline(kv2, kf2, 3, false, false, true, true, scenePass_, s, skinLayout_, false, 0, true);
    legerPipeline_ = true;   // WFS-263 (RG-01) : décor entièrement au-delà de 70 m, avec et sans faces arrière
    opaqueLeger_ = makePipeline(sv, sf, 1, false, false, true, true, scenePass_, s, sceneLayout_, false, 0, true);
    cullPipeline_ = 0;
    opaqueLegerDouble_ = makePipeline(sv, sf, 1, false, false, true, true, scenePass_, s, sceneLayout_, false, 0, true);
    legerPipeline_ = false;
    { const char* f = std::getenv("WFS_LEGER"); legerActif_ = !(f && f[0] == '0'); }   // banc : WFS_LEGER=0 (comparaison)
    { const char* f = std::getenv("WFS_FACES"); facesArriere_ = !(f && f[0] == '0'); }
    // WFS-09 : tache des phares (multiplicative) et halos (additifs)
    poolPipe_ = makePipeline(sv, sf, 1, true, false, true, false, scenePass_, s, sceneLayout_, false, 1);
    addPipe_ = makePipeline(sv, sf, 1, true, false, true, false, scenePass_, s, sceneLayout_, false, 2);
}

// WFS-262 (RG-02) : variantes MSAA (×1, ×2, ×4 que le GPU accepte, hors celle en service) des pipelines de scène créées une
// fois au chargement dans une passe compatible (mêmes formats et échantillons), puis détruites : elles restent dans le cache
// de pipelines (RG-01). Un changement de « Graphismes » (menu ou pause) les reprend du cache au lieu de recompiler.
void Renderer::prechaufferMsaa() {
    if (!cachePipelines_ || !scenePass_) return;
    VkDevice d = vk_->device;
    const VkSampleCountFlags sc = vk_->props.limits.framebufferColorSampleCounts & vk_->props.limits.framebufferDepthSampleCounts;
    const VkSampleCountFlagBits enService = vk_->msaa;
    const uint32_t avant = pipelinesCrees_;
    VkPipeline* membres[] = {&opaque_, &opaqueDecoupe_, &blend_, &sky_, &skyApres_, &skinOpaque_, &skinOpaqueDecoupe_, &opaqueCull_, &skinOpaqueCull_, &poolPipe_, &addPipe_, &opaqueLeger_, &opaqueLegerDouble_};
    VkPipeline garde[std::size(membres)];
    for (size_t i = 0; i < std::size(membres); i++) garde[i] = *membres[i];
    const VkRenderPass passeGardee = scenePass_;
    int variantes = 0;
    for (VkSampleCountFlagBits e : {VK_SAMPLE_COUNT_1_BIT, VK_SAMPLE_COUNT_2_BIT, VK_SAMPLE_COUNT_4_BIT}) {
        if (e == enService || !(sc & e) || e > msaaMax_) continue;
        const bool ms = e != VK_SAMPLE_COUNT_1_BIT;
        VkAttachmentDescription at[3]{};
        for (VkAttachmentDescription& a : at) { a.loadOp = a.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; a.storeOp = a.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; a.finalLayout = VK_IMAGE_LAYOUT_GENERAL; }
        at[0].format = HDR_FORMAT; at[0].samples = e; at[1].format = vk_->depthFormat; at[1].samples = e; at[2].format = HDR_FORMAT; at[2].samples = VK_SAMPLE_COUNT_1_BIT;
        VkAttachmentReference colRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}, depRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL}, resRef{2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkSubpassDescription sp{}; sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sp.colorAttachmentCount = 1; sp.pColorAttachments = &colRef; sp.pDepthStencilAttachment = &depRef; if (ms) sp.pResolveAttachments = &resRef;
        VkRenderPassCreateInfo rpci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        rpci.attachmentCount = ms ? 3 : 2; rpci.pAttachments = at; rpci.subpassCount = 1; rpci.pSubpasses = &sp;
        VkRenderPass p = VK_NULL_HANDLE;
        if (vkCreateRenderPass(d, &rpci, nullptr, &p) != VK_SUCCESS) continue;
        vk_->msaa = e; scenePass_ = p;
        createScenePipelines();
        for (VkPipeline* m : membres) { if (*m) vkDestroyPipeline(d, *m, nullptr); *m = VK_NULL_HANDLE; }
        vkDestroyRenderPass(d, p, nullptr);
        variantes++;
    }
    vk_->msaa = enService; scenePass_ = passeGardee;
    for (size_t i = 0; i < std::size(membres); i++) *membres[i] = garde[i];
    WFS_LOG("pipelines (WFS-262 RG-02) : %d variante(s) MSAA de la scène mises en cache au chargement (%u pipelines)", variantes, pipelinesCrees_ - avant);
}

// WFS-144 (RG-04, WFS-134 RG-03 / RG-04) : netteté de la Basse = MSAA ×2 et anisotrope ×4 ; Moyenne et Haute : MSAA ×4,
// anisotrope ×8 (inchangés). Netteté retenue (WFS-134 RG-02) : une Basse due à la rétrogradation automatique ou à la chauffe
// garde la netteté du niveau quitté. GPU sans MSAA ×2 (lavapipe du banc) : ×4 gardé. Appelé GPU à l'arrêt (resize).
bool Renderer::appliquerNettete() {
    const int q = qualiteBasse ? qualite::BASSE : qualite::HAUTE;
    const bool basse = qualite::netteteBasse(q);
    const VkSampleCountFlags sc = vk_->props.limits.framebufferColorSampleCounts & vk_->props.limits.framebufferDepthSampleCounts;
    const VkSampleCountFlagBits ms = (VkSampleCountFlagBits)qualite::msaaScene(q, (int)msaaMax_, (sc & VK_SAMPLE_COUNT_2_BIT) != 0);
    const bool texBasse = samplerBasse_ && qualite::anisoScene(q, anisoMax_) < anisoMax_;
    const bool changeTex = texBasse != texBasse_;
    if (changeTex) {   // échantillonneur de toutes les textures des matériaux réécrit
        texBasse_ = texBasse; aniso_ = texBasse ? 4.f : anisoMax_;
        const VkSampler smp = texBasse ? samplerBasse_ : sampler_;
        for (const MatTex& t : matTex_) {
            VkDescriptorImageInfo ii[3]; VkWriteDescriptorSet wr[3]{};
            for (uint32_t i = 0; i < 3; i++) {
                ii[i] = {smp, t.v[i], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                wr[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; wr[i].dstSet = t.set; wr[i].dstBinding = i;
                wr[i].descriptorCount = 1; wr[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; wr[i].pImageInfo = &ii[i];
            }
            vkUpdateDescriptorSets(vk_->device, 3, wr, 0, nullptr);
        }
    }
    const bool refaire = ms != vk_->msaa;
    if (refaire) {   // passe de scène et ses pipelines refaits au nouveau nombre d'échantillons (createTargets puis createScenePipelines)
        VkDevice d = vk_->device;
        for (VkPipeline* p : {&opaque_, &blend_, &sky_, &skinOpaque_, &poolPipe_, &addPipe_, &opaqueDecoupe_, &skinOpaqueDecoupe_, &skyApres_, &opaqueCull_, &skinOpaqueCull_, &opaqueLeger_, &opaqueLegerDouble_}) { if (*p) vkDestroyPipeline(d, *p, nullptr); *p = VK_NULL_HANDLE; }
        if (scenePass_) vkDestroyRenderPass(d, scenePass_, nullptr);
        scenePass_ = VK_NULL_HANDLE;
        vk_->msaa = ms;
    }
    if (refaire || changeTex)
        WFS_LOG("rendu (WFS-144) : netteté %s : MSAA ×%d, aniso ×%.0f", basse ? "Basse" : "Moyenne / Haute", (int)vk_->msaa, aniso_);
    return refaire;
}

Material Renderer::makeMaterial(const Material& src, const Image* color, const Image* normal, const Image* mr) {
    creationsMateriaux_++;   // WFS-268 (RG-04)
    Material m = src;
    if (normal && normal->view) m.flags |= MAT_HASNORMAL;
    if (mr && mr->view) m.flags |= MAT_HASMR;
    VkDescriptorSetAllocateInfo dai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    dai.descriptorPool = pool_; dai.descriptorSetCount = 1; dai.pSetLayouts = &matLayout_;
    VK_OK(vkAllocateDescriptorSets(vk_->device, &dai, &m.set));
    VkDescriptorImageInfo ii[3] = {
        {texBasse_ ? samplerBasse_ : sampler_, (color && color->view) ? color->view : white.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {texBasse_ ? samplerBasse_ : sampler_, (normal && normal->view) ? normal->view : flatNormal.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {texBasse_ ? samplerBasse_ : sampler_, (mr && mr->view) ? mr->view : white.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
    matTex_.push_back({m.set, {ii[0].imageView, ii[1].imageView, ii[2].imageView}});   // WFS-144 : netteté changeable
    jeuxMat_.insert(m.set);   // WFS-187
    VkWriteDescriptorSet wr[3]{};
    for (uint32_t i = 0; i < 3; i++) {
        wr[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; wr[i].dstSet = m.set; wr[i].dstBinding = i;
        wr[i].descriptorCount = 1; wr[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; wr[i].pImageInfo = &ii[i];
    }
    vkUpdateDescriptorSets(vk_->device, 3, wr, 0, nullptr);
    return m;
}

// WFS-82 : sphère englobante des sommets (boîte → sphère) ; aucun sommet : inconnue (jamais masquée)
template <class V> static masquage::Sphere bornes(const std::vector<V>& v) {
    if (v.empty()) return {};
    vec3 mn{1e30f, 1e30f, 1e30f}, mx{-1e30f, -1e30f, -1e30f};
    for (const V& s : v) { mn = {std::min(mn.x, s.px), std::min(mn.y, s.py), std::min(mn.z, s.pz)}; mx = {std::max(mx.x, s.px), std::max(mx.y, s.py), std::max(mx.z, s.pz)}; }
    return masquage::deBoite(mn, mx);
}

// WFS-82 : distance (petits objets au loin), puis caméra, miroir des reflets et boîte d'ombre
uint8_t Renderer::classer(const masquage::Sphere& s, bool sansDistance) {
    constexpr uint8_t CASC = 0xF0;   // WFS-227 : toutes les cascades d'ombre
    if (s.r < 0) return VIS_CAM | VIS_REFLET | VIS_OMBRE | CASC;
    // WFS-150 : image d'une capture d'environnement : tout objet reste en file pour la capture (6 directions) ;
    // WFS-222 : mais la passe de scène, l'ombre et le reflet de cette image gardent leur masquage (VIS_CAPTURE seul sinon)
    if (sansMasquage_ && rendu221) return VIS_CAM | VIS_REFLET | VIS_OMBRE | CASC;   // banc : comparaison avec l'avant
    const uint8_t tout = sansMasquage_ ? VIS_CAPTURE : 0;
    if (!sansDistance && length(s.c - camPos_) - s.r > masquage::distanceMax(s.r, qualiteBasse, finBrouillard_)) { if (!tout) masques_++; return tout; }
    // WFS-264 (RG-03) : petit objet (rayon < 1,5 m : panneau, borne, piéton, mobilier) à plus de 40 m : pas d'ombre portée
    const bool ombrePetit = !(length(s.c - camPos_) > 40.f && s.r < 1.5f);
    const uint8_t v = (masquage::visible(fCam_, s) ? VIS_CAM : 0) | (refletsOn_ && masquage::visible(fReflet_, s) ? VIS_REFLET : 0) |
                      (shadowsOn_ && ombrePetit && masquage::visible(fOmbre_, s) ? VIS_OMBRE : 0);
    uint8_t c = 0;   // WFS-227 (accroche) : cascades d'ombre que l'objet touche (passe d'ombre : une tuile seulement si besoin)
    // tranche de la cascade (le long du regard) + portée de l'ombre de l'objet (≈ 3 × son rayon + 5 m) : un objet près de la
    // caméra (la voiture) n'est pas redessiné dans les cascades lointaines, ni un petit objet (< 2 texels) dans une cascade grossière
    const float d = dot(s.c - camPos_, regardCasc_), portee = 3 * s.r + 5;
    if (v & VIS_OMBRE) for (int k = 0; k < cascades_.n; k++)
        if (s.r >= 2 * texelCasc_[k] && d + s.r + portee >= (k ? cascades_.fin[k - 1] : -1e9f) && d - s.r <= cascades_.fin[k] && masquage::visible(fCasc_[k], s)) c |= (uint8_t)(VIS_CASCADE0 << k);
    if (!v && !tout) masques_++;
    return v | tout | c;
}

Mesh Renderer::makeMesh(const std::vector<Vertex>& v, const std::vector<uint32_t>& idx, bool dynamic) {
    Mesh m; m.dynamic = dynamic; m.indexCount = (uint32_t)idx.size();
    creationsMaillages_++;   // WFS-268 (RG-04)
    memoire::compteurs().objets3d++;   // WFS-297 (RG-06)
    { const masquage::Sphere b = bornes(v); m.centre = b.c; m.rayon = b.r; }   // WFS-82
    // WFS-149 (RG-04) : géométrie générée (route, décor, charte…) sans tangentes : calculées ici, une fois, sur une copie
    // (tampon réutilisé). updateMesh (halos, particules : jamais de carte de normales) n'en calcule pas.
    static thread_local std::vector<Vertex> avecTangentes;
    const Vertex* src = v.data();
    if (idx.size() >= 3 && !tangentes::auMoinsUne(v.data(), v.size())) {
        const auto t0 = std::chrono::steady_clock::now();
        avecTangentes.assign(v.begin(), v.end());
        tangentes::calculer(avecTangentes.data(), (uint32_t)v.size(), idx.data(), (uint32_t)idx.size());
        src = avecTangentes.data();
        tangentesMs_ += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count(); tangentesSommets_ += v.size();
    }
    if (dynamic) {
        m.vb = vk_->createBuffer(std::max<size_t>(v.size(), 1) * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, true);
        m.ib = vk_->createBuffer(std::max<size_t>(idx.size(), 1) * 4, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, true);
        if (!v.empty()) std::memcpy(m.vb.mapped, src, v.size() * sizeof(Vertex));
        if (!idx.empty()) std::memcpy(m.ib.mapped, idx.data(), idx.size() * 4);
    } else {
        m.vb = vk_->uploadBuffer(src, v.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        m.ib = vk_->uploadBuffer(idx.data(), idx.size() * 4, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    }
    return m;
}

void Renderer::updateMesh(Mesh& m, const std::vector<Vertex>& v, const std::vector<uint32_t>& idx) {
    if (v.size() * sizeof(Vertex) > m.vb.size || idx.size() * 4 > m.ib.size) { vk_->waitIdle(); destroy(m); m = makeMesh(v, idx, true); return; }
    std::memcpy(m.vb.mapped, v.data(), v.size() * sizeof(Vertex));
    std::memcpy(m.ib.mapped, idx.data(), idx.size() * 4);
    m.indexCount = (uint32_t)idx.size();
    { const masquage::Sphere b = bornes(v); m.centre = b.c; m.rayon = b.r; }   // WFS-82
}

void Renderer::destroy(Mesh& m) { if (m.vb.buf) memoire::compteurs().objets3d--; vk_->destroy(m.vb); vk_->destroy(m.ib); m.indexCount = 0; }   // WFS-297 : objets 3D vivants

SkinnedMesh Renderer::makeSkinnedMesh(const std::vector<SkinVertex>& v, const std::vector<uint32_t>& idx) {
    SkinnedMesh m; m.indexCount = (uint32_t)idx.size();
    memoire::compteurs().objets3d++;   // WFS-297 (RG-06)
    { const masquage::Sphere b = bornes(v); m.centre = b.c; m.rayon = b.r < 0 ? b.r : b.r * 1.5f; }   // WFS-82 : marge pour l'animation
    m.vb = vk_->uploadBuffer(v.data(), v.size() * sizeof(SkinVertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    m.i16 = v.size() <= 65536;   // WFS-210 (RG-05) : moins de 65 536 sommets → indices 16 bits (mémoire GPU ÷ 2 pour les indices)
    if (m.i16) { std::vector<uint16_t> c(idx.begin(), idx.end()); m.ib = vk_->uploadBuffer(c.data(), c.size() * 2, VK_BUFFER_USAGE_INDEX_BUFFER_BIT); }
    else m.ib = vk_->uploadBuffer(idx.data(), idx.size() * 4, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    return m;
}

void Renderer::destroy(SkinnedMesh& m) { if (m.vb.buf) memoire::compteurs().objets3d--; vk_->destroy(m.vb); vk_->destroy(m.ib); m.indexCount = 0; }   // WFS-297

void Renderer::destroy(Model& m) {
    for (auto& mat : m.mats) if (mat.set) {
        std::erase_if(matTex_, [&](const MatTex& t) { return t.set == mat.set; });   // WFS-144
        if (jeuxMat_.erase(mat.set)) vkFreeDescriptorSets(vk_->device, pool_, 1, &mat.set);   // WFS-187 : une seule fois
    }
    for (auto& t : m.textures) vk_->destroy(t);
    vk_->destroy(m.vb); vk_->destroy(m.ib);
    m = Model{};
}

// WFS-187 (RG-01) : libère le jeu de descripteurs d'un matériau fait par makeMaterial (le pool a FREE_DESCRIPTOR_SET_BIT).
// À appeler quand le GPU n'utilise plus ce jeu (shutdown des systèmes : Game::shutdown attend le GPU avant). Matériau vide
// ou déjà libéré : sans effet. Les matériaux d'un Model sont libérés par destroy(Model&).
// Une copie d'un matériau (atlas recopié dans mats_, anneau du garage…) partage le jeu : seul le premier appel le rend.
void Renderer::destroy(Material& m) {
    if (m.set == VK_NULL_HANDLE || !vk_ || !pool_) return;
    if (jeuxMat_.erase(m.set)) {
        std::erase_if(matTex_, [&](const MatTex& t) { return t.set == m.set; });
        vkFreeDescriptorSets(vk_->device, pool_, 1, &m.set);
    }
    m.set = VK_NULL_HANDLE;
}

bool Renderer::beginFrame(const FrameParams& fp) {
    VkCtx& vk = *vk_; VkDevice d = vk.device;
    FrameRes& r = fr_[cur_];
    const double tAttente = nowSeconds();   // WFS-222 (RG-01) : attente du GPU (image en vol), beginFrame entier
    VK_OK(vkWaitForFences(d, 1, &r.fence, VK_TRUE, UINT64_MAX));
    if (vk.offscreen) imageIndex_ = 0;
    else {
        VkResult res = vkAcquireNextImageKHR(d, vk.swapchain, UINT64_MAX, r.acquired, VK_NULL_HANDLE, &imageIndex_);
        if (res == VK_ERROR_OUT_OF_DATE_KHR) { vk.waitIdle(); if (vk.createSwapchain()) resize(); return false; }
        if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) { WFS_ERR("acquisition d'image : %d", (int)res); return false; }
    }
    VK_OK(vkResetFences(d, 1, &r.fence));

    // données de l'image
    FrameUBO u{};
    u.viewProj = fp.proj * fp.view; u.invViewProj = inverse(u.viewProj);
    u.camPos = {fp.camPos.x, fp.camPos.y, fp.camPos.z, fp.time};
    vec3 sd = normalize(fp.sunDir);
    u.sunDir = {sd.x, sd.y, sd.z, fp.sunIntensity};
    u.sunColor = {fp.sunColor.x, fp.sunColor.y, fp.sunColor.z, 0};
    u.skyTop = {fp.skyTop.x, fp.skyTop.y, fp.skyTop.z, 0};
    u.skyHorizon = {fp.skyHorizon.x, fp.skyHorizon.y, fp.skyHorizon.z, 0};
    u.groundCol = {fp.ground.x, fp.ground.y, fp.ground.z, 0};
    u.fog = {fp.fogColor.x, fp.fogColor.y, fp.fogColor.z, fp.fogDensity};
    u.misc = {fp.exposure, fp.night ? 1.f : 0.f, fp.ambient, fp.reflections};
    // boîte d'ombre (stableShadow du jeu web) : centrée sur la voiture, 20 m devant dans le sens de la caméra,
    // repère du soleil fixe et centre calé sur la grille des texels → bords d'ombre stables quand on roule
    shadowsOn_ = fp.shadows && !fp.night && !forceNoShadows; night_ = fp.night || forceNoShadows;
    // WFS-264 (RG-03, ANO-145) : mesure d'une cascade unique en Basse (40 m, toute la carte) ; la Basse reste sans ombre du
    // soleil (décision du PO, regles-de-gestion/WFS-227.md) : WFS_OMBRES_BASSE=1 au banc seulement, pour le chiffre au journal
    static const bool ombresBasse = [] { const char* e = std::getenv("WFS_OMBRES_BASSE"); return e && e[0] == '1'; }();
    if (qualiteBasse && ombresBasse) { shadowsOn_ = fp.shadows && !fp.night; night_ = fp.night; }
    nuitScene_ = fp.night; appelsPrec_ = appels_; appels_ = 0;   // WFS-15
    comptesPrec_ = comptes_; comptes_ = ComptesImage{};   // WFS-317 (I9)
    releve_.clear();   // WFS-335 (RG-01)
    chronoPrec_ = chrono_; chrono_.triangles = 0;   // WFS-222 : image précédente (attenteGpuMs : celle-ci, fin de beginFrame)
    // WFS-15 / WFS-100 : force d'ENV.bloom du jeu web (nuit 0,8, jour 0,3, garage 0,55 par fp.bloom) ; coupé en Basse et en GPU logiciel
    if (bloom_) bloom_->force = gpuLogiciel() ? 0.f : bloomForce * (fp.bloom >= 0.f ? fp.bloom : nuitScene_ ? 0.8f : 0.3f);
    {
        const vec3 L = sd, up = std::fabs(L.y) > 0.99f ? vec3{1, 0, 0} : vec3{0, 1, 0};
        const mat4 V = lookAt(L, {0, 0, 0}, up);
        vec3 fwd{-fp.view.at(2, 0), 0, -fp.view.at(2, 2)};
        vec3 c = fp.hasFocus ? fp.focus : fp.camPos;
        if (length(fwd) > 1e-4f) c = c + normalize(fwd) * SHADOW_AHEAD;
        const vec3 lc = V.transformPoint(c);
        const float texel = 2 * SHADOW_HALF / (float)shadowSize_;
        const float cx = std::round(lc.x / texel) * texel, cy = std::round(lc.y / texel) * texel;
        const float nearD = -lc.z - SHADOW_DEPTH * 0.5f;   // distance le long du soleil (vue : -z)
        mat4 P{};
        P.at(0, 0) = 1 / SHADOW_HALF; P.at(0, 3) = -cx / SHADOW_HALF;
        P.at(1, 1) = 1 / SHADOW_HALF; P.at(1, 3) = -cy / SHADOW_HALF;
        P.at(2, 2) = -1 / SHADOW_DEPTH; P.at(2, 3) = -nearD / SHADOW_DEPTH;
        P.at(3, 3) = 1;
        u.shadowMat = P * V;
        u.shadowP = {shadowsOn_ ? 1.f : 0.f, 1.f / (float)shadowSize_, SHADOW_NORMAL_BIAS, SHADOW_BIAS};
        // WFS-227 (accroche E13) : cascades stabilisées (eclairage_ombres.h) ; shadowMat = boîte de toutes les cascades (masquage)
        const vec3 regard{-fp.view.at(2, 0), -fp.view.at(2, 1), -fp.view.at(2, 2)};
        regardCasc_ = normalize(regard);
        cascades_ = ombres::calculer(L, fp.camPos, regard, niveauCascades(), shadowSize_);
        for (int k = 0; k < 4; k++) { u.ombreCasc[k] = cascades_.m[std::min(k, cascades_.n - 1)]; (&u.ombreFin.x)[k] = cascades_.fin[std::min(k, cascades_.n - 1)]; (&u.ombreDemi.x)[k] = cascades_.demi[std::min(k, cascades_.n - 1)]; }
        u.ombreP = {(float)cascades_.n, (float)std::max(niveauCascades(), 1), (float)(cascades_.n > 1 ? shadowSize_ / 2 : shadowSize_), (float)ombres::tanSoleil()};
        u.shadowMat = cascades_.union_;
    }
    {   // WFS-45 : projecteurs, lumière hémisphère, brouillard linéaire (three.js r128, physicallyCorrectLights non activé)
        const vec3 fwd = normalize(vec3{-fp.view.at(2, 0), -fp.view.at(2, 1), -fp.view.at(2, 2)});
        u.camFwd = {fwd.x, fwd.y, fwd.z, 0};
        u.fogLin = {fp.fogNear, fp.fogFar, 0, 0};
        if (fp.hemiIntensity > 0) {
            const vec3 a = fp.hemiSky * fp.hemiIntensity, b = fp.hemiGround * fp.hemiIntensity;
            u.hemiSky = {a.x, a.y, a.z, 1}; u.hemiGround = {b.x, b.y, b.z, 0};
        }
        const int n = std::clamp(fp.spotCount, 0, FrameParams::SPOTS_MAX);   // WFS-226
        // WFS-263 (RG-02) : y = lumières par pixel au plus (4 en Basse, 12 sinon) ; z = lectures du reflet de la route mouillée (4 / 12)
        u.spotN = {(float)n, qualiteBasse ? 4.f : 12.f, qualiteBasse ? 4.f : 12.f, 0};
        // WFS-263 (RG-02) : en Basse (4 lumières par pixel au plus), lumières rangées de la plus proche à la plus lointaine de
        // la caméra : le plafond garde celles qui éclairent le premier plan ; Haute : ordre inchangé (image identique)
        int ordre[FrameParams::SPOTS_MAX];
        for (int i = 0; i < n; i++) ordre[i] = i;
        if (qualiteBasse) std::stable_sort(ordre, ordre + n, [&](int a, int b) { return length(fp.spots[a].pos - fp.camPos) < length(fp.spots[b].pos - fp.camPos); });
        for (int i = 0; i < n; i++) {
            const FrameParams::Spot& s = fp.spots[ordre[i]];
            vec3 dir = normalize(s.target - s.pos);
            if (length(dir) < 0.5f) dir = {0, -1, 0};
            // r128 sans lumières physiques : irradiance × PI (lights_fragment_begin), compensée par le 1/PI du Lambert
            const vec3 c = s.color * (s.intensity * PI);
            u.spotPos[i] = {s.pos.x, s.pos.y, s.pos.z, std::max(s.distance, 0.f)};
            u.spotDir[i] = {dir.x, dir.y, dir.z, std::max(s.decay, 0.f)};
            u.spotCol[i] = {c.x, c.y, c.z, 0};
            u.spotCone[i] = {std::cos(s.angle), std::cos(s.angle * (1.f - std::clamp(s.penumbra, 0.f, 1.f))), s.croisement ? 1.f : 0.f, 0};   // WFS-218
            if (s.angle >= PI - 1e-3f) u.spotCone[i] = {-2.f, -1.5f, 0, 0};   // WFS-09 : PointLight (toutes directions)
        }
    }
    // WFS-16 : chaussée mouillée, flaques, reflets de la scène (passe miroir autour de y = refletY)
    // WFS-264 (RG-02) : reflet temps réel en Haute seulement (nuit.cpp : fp.reflets = Haute, déjà avant ce ticket) ; coupé en Basse
    // ici aussi, quoi que demande la scène (ciel et sonde seuls)
    refletsOn_ = fp.mouille && fp.reflets && !forceSansReflets && !qualiteBasse;   // WFS-222 (RG-03)
    u.nuitA = {fp.mouille ? 1.f : 0.f, fp.mouille && fp.flaques ? 1.f : 0.f, refletsOn_ ? 1.f : 0.f, 0};
    // WFS-142 (RG-04, RG-08) : w = force du reflet sur la chaussée (configgame) ; WFS_REFLETS=0 : −1, aucun reflet (mesure du banc)
    const char* eR = std::getenv("WFS_REFLETS");
    const bool sansReflet = eR && !std::strcmp(eR, "0");
    u.nuitB = {fp.refletY, 1.f / (float)sw_, 1.f / (float)sh_, sansReflet ? -1.f : config().qualite.refletRouteMouillee};
    eclairage::avantImage(vk, u, r.frameSet, refletSet_[cur_]);   // WFS-151 (accroche E13) : IBL, exposition, ciel
    std::memcpy(r.ubo.mapped, &u, sizeof(u));
    if (refletsOn_) {
        FrameUBO m = u;
        mat4 R = mat4::identity(); R.at(1, 1) = -1; R.at(1, 3) = 2 * fp.refletY;   // y → 2h − y
        m.viewProj = fp.proj * fp.view * R; m.invViewProj = inverse(m.viewProj);
        m.camPos.y = 2 * fp.refletY - fp.camPos.y;
        m.camFwd.y = -m.camFwd.y;
        m.nuitA = {0, 0, 0, 1};   // passe de reflet : pas de route mouillée ni de reflet lu, rien sous le plan
        std::memcpy(refletUbo_[cur_].mapped, &m, sizeof(m));
    }
    {   // WFS-82 : plans de la caméra, du miroir des reflets et de la boîte d'ombre ; fin du brouillard
        fCam_ = masquage::depuis(u.viewProj);
        mat4 R = mat4::identity(); R.at(1, 1) = -1; R.at(1, 3) = 2 * fp.refletY;
        fReflet_ = masquage::depuis(fp.proj * fp.view * R);
        fOmbre_ = masquage::depuis(u.shadowMat);
        for (int k = 0; k < cascades_.n; k++) { fCasc_[k] = masquage::depuis(cascades_.m[k]); texelCasc_[k] = k ? 2 * cascades_.demi[k] / (float)(cascades_.n > 1 ? shadowSize_ / 2 : shadowSize_) : 0.f; }   // WFS-227
        finBrouillard_ = masquage::finBrouillard(fp.fogFar, fp.fogDensity);
        masquesPrec_ = masques_; masques_ = 0;
    }
    feuxNuit_ = FeuxVoiture{}; feuxNuit_.nuit = fp.feux; feuxSuivantPose_ = false;   // WFS-09
    ombresContact_ = fp.ombresContact; ombreFaite_ = false; fantomeSuivant_ = false; adversaireSuivant_ = false;   // WFS-134 ; WFS-243
    quadsUsed_ = 0; view_ = fp.view;
    camPos_ = fp.camPos; exposure_ = fp.exposure;
    focalePx_ = std::fabs(fp.proj.at(1, 1)) * (float)sh_ * 0.5f;   // WFS-158 (RG-04)
    transparent_.clear(); opaqueQ_.clear(); ui_vtx_.clear(); skinUsed_ = 0;
    readShadowTiming();
    passes::debutImage(*this);   // WFS-150 (RG-05) : coût des passes de l'image précédente sur ce tampon ; capture : aucun masquage

    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_OK(vkResetCommandBuffer(r.cb, 0));
    VK_OK(vkBeginCommandBuffer(r.cb, &bi));
    chronoPrec_.attenteGpuMs = (nowSeconds() - tAttente) * 1000;   // WFS-222 : tout beginFrame (lavapipe attend aussi dans vkUpdateDescriptorSets)
    inFrame_ = true;
    return true;
}

void Renderer::flushDraw(const Pending& p) {
    appels_++;   // WFS-15
    chrono_.triangles += p.count / 3;   // WFS-222
    comptes_.appels[p.famille]++; comptes_.triangles[p.famille] += p.count / 3;   // WFS-317 (I9)
    VkCommandBuffer cb = fr_[cur_].cb;
#if defined(__APPLE__)
    // WFS-88 : le GPU du simulateur iOS (famille Apple 2) refuse un sommet de base non nul (MoltenVK : VK_ERROR_FEATURE_NOT_PRESENT,
    // puis vkEndCommandBuffer échoue) : le décalage passe dans l'adresse du tampon de sommets, même résultat
    int32_t vofs = p.vofs;
    VkDeviceSize vbOfs = 0;
    if (vofs > 0) { vbOfs = (VkDeviceSize)vofs * (p.osOfs >= 0 ? sizeof(SkinVertex) : sizeof(Vertex)); vofs = 0; }
    vkCmdBindVertexBuffers(cb, 0, 1, &p.vb, &vbOfs);
#else
    const int32_t vofs = p.vofs;
    VkDeviceSize zero = 0;
    vkCmdBindVertexBuffers(cb, 0, 1, &p.vb, &zero);
#endif
    vkCmdBindIndexBuffer(cb, p.ib, 0, p.i16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);   // WFS-210 (RG-05)
    const VkPipelineLayout lay = p.osOfs >= 0 ? skinLayout_ : sceneLayout_;   // WFS-63 : palette d'os en set 2
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, lay, 1, 1, &p.mat->set, 0, nullptr);
    if (p.osOfs >= 0) { const uint32_t ofs = (uint32_t)p.osOfs; vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, skinLayout_, 2, 1, &osSet_[cur_], 1, &ofs); }
    vkCmdPushConstants(cb, lay, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PC), &p.pc);
    vkCmdDrawIndexed(cb, p.count, 1, p.first, vofs, 0);
}

// WFS-84 : feux de nuit naturels. Réglages en un seul endroit (RG-06), à ajuster après un essai sur téléphone.
// Chaque feu = un point lumineux net (cœur, quelques cm, qui reste un point au loin) + une lueur douce d'environ
// 2,5 × la taille du feu (jamais un disque géant), éteinte progressivement entre 60 et 120 m (RG-04) ; un feu ne
// brille que du côté où on le voit (pas de halo de phare autour d'une voiture vue de dos, ni au-dessus du toit).
// Le feu lui-même (matériau MAT_TAIL) s'allume aussi : émission plus forte au freinage (draw, plus haut).
namespace feux {
constexpr float FEUX_HDR = 1.6f;                       // sprites additifs du jeu web (sans tonemapping) → passe HDR + ACES
constexpr float COEUR = 0.07f, LUEUR = 0.20f;          // feu arrière : rayons du cœur et de la lueur (m)
constexpr float I_COEUR = 2.0f, I_COEUR_STOP = 4.0f;   // intensités (× couleur linéaire) : le stop est plus vif, même taille (RG-02)
constexpr float I_LUEUR = 0.25f, I_LUEUR_STOP = 0.5f;
constexpr float PHARE_COEUR = 0.09f, PHARE_LUEUR = 0.30f, I_PHARE_COEUR = 2.4f, I_PHARE_LUEUR = 0.35f;
constexpr float RECUL_COEUR = 0.05f, RECUL_LUEUR = 0.15f, I_RECUL_COEUR = 1.4f, I_RECUL_LUEUR = 0.22f;   // feu blanc discret
constexpr float RENTRE = 0.88f;                        // feux rapprochés du centre (le bord extérieur du feu dépassait)
constexpr float POINT_MIN = 0.0011f;                   // rayon minimal du cœur = 0,0011 × distance : un point rouge au loin
constexpr float LUEUR_DEBUT = 60.f, LUEUR_FIN = 120.f; // lueur pleine jusqu'à 60 m, éteinte à 120 m
// WFS-171 (RG-01, ANO-70) : lueur rouge au sol calculée (feux_modele::eclairementSol) sur une grille de SOL_NX × SOL_NZ
// quads, du dessous du pare-chocs (fondu de SOL_SOUS m, caché par la caisse) jusqu'à SOL_PORTEE m derrière ; plus de
// disque. Le recul n'a plus de quad blanc : sa lumière au sol est la vraie lumière du feu de recul (nuit.cpp).
constexpr int SOL_NX = 8, SOL_NZ = 6;
constexpr float SOL_SOUS = 0.25f, SOL_PORTEE = 2.6f, SOL_BORD = 0.45f;   // m ; SOL_BORD : marge de côté (m)
constexpr float SOL_DISTANCE = 80.f;                                      // au-delà : < 1 % de la chaussée éclairée, pas dessinée
inline const vec3 ROUGE_SOL{1.f, 0.03f, 0.005f};   // rouge CEE (R48 § 2.29 : y ≤ 0,335) d'un feu à ampoule : x ≈ 0,68, y ≈ 0,32
// WFS-109 : émission du feu du modèle (× rouge) : position la nuit 1,6, stop 4 (= setBrake du web, 0xff2040 × 2,5 → × 1,6
// pour la passe HDR, comme FEUX_HDR), éteint le jour hors freinage (RG-15 a) ; le niveau du stop passe de 0 à 1 en 80 ms
constexpr float E_POSITION = 1.6f, E_STOP = 4.0f;
inline float emissionArriere(bool position, float stop) { const float base = position ? E_POSITION : 0.f; return base + (E_STOP - base) * stop; }
inline const vec3 VERRE{1.f, 0.0103f, 0.0513f};   // RG-13 : verre rouge du feu découpé (0xff2040 du web, linéaire)
}

void Renderer::draw(const Model& m, const mat4& world, const vec3* paint, const RouesPose* roues) {
    if (!inFrame_ || sceneFigee() || !m.ok) return;   // WFS-329 : fond figé : dessins 3D ignorés
    // WFS-134 (RG-05, RG-07) : ombre de contact (≈ 1,1 × la longueur) si la voiture n'a pas déjà la sienne ; fantôme sans
    const bool fantome = fantomeSuivant_, ombreDejaLa = ombreFaite_, lots = lotsSuivant_ && qualiteBasse;
    const bool adversaire = adversaireSuivant_;   // WFS-243 (ANO-117) : lu et remis à faux pour chaque voiture
    fantomeSuivant_ = false; lotsSuivant_ = false; adversaireSuivant_ = false;
    if (ombresContact_ && !fantome && !ombreDejaLa) {
        const vec3 c = (m.bmin + m.bmax) * 0.5f, t = m.bmax - m.bmin;
        draw(ombreQuad_, ombreMat_, world * translate({c.x, m.bmin.y, c.z}) * scale({t.x * 1.12f, 1.f, t.z * 1.1f}));
    }
    ombreFaite_ = false;
    // WFS-09 : feux de cette voiture (le joueur : feuxProchaineVoiture ; les autres : état de nuit de l'image)
    const FeuxVoiture fx = feuxSuivantPose_ ? feuxSuivant_ : feuxNuit_;
    feuxSuivantPose_ = false;
    const uint8_t vis = classer(masquage::monde(world, masquage::deBoite(m.bmin, m.bmax)), adversaire);   // WFS-82 ; WFS-243 : adversaire sans limite de distance
    if (!vis) return;   // RG-06 : les feux disparaissent avec leur voiture, jamais seuls
    // WFS-109 (RG-07, RG-15) : de jour aussi, le feu du modèle s'allume au freinage et le feu de recul en marche arrière
    const float stopK = fx.frein >= 0 ? std::clamp(fx.frein, 0.f, 1.f) : (fx.stop ? 1.f : 0.f);
    if (fx.nuit || stopK > 0 || fx.recul) feuxVoiture(m, world, fx);
    // WFS-134 (RG-11) : Basse, au-delà de 15 m : roues fixes (invisibles à cette distance), pièces regroupées ci-dessous
    const bool basseLoin = qualiteBasse && length(world.transformPoint((m.bmin + m.bmax) * 0.5f) - camPos_) > 15.f;
    mat4 wr[4]; const bool anim = roues && m.nRoues == 4 && !basseLoin;   // WFS-60 : matrice de chaque roue (braquage, roulage)
    if (anim) for (int k = 0; k < 4; k++) wr[k] = world * roues::matrice(m.roues[k], *roues);
    vec3 c = world.transformPoint((m.bmin + m.bmax) * 0.5f);
    float depth = length(c - camPos_);
    // WFS-134 (RG-11, RG-12), WFS-158 (RG-04) : 4 niveaux de détail choisis par la taille de la voiture à l'écran, fondu tramé
    // écran complémentaire entre deux niveaux (scene.frag) ; jamais de cubes ; Basse, fantôme, police en lots : vehicule_lod.h
    // WFS-265 (2e retour du lead dev, PR #268) : la voiture du joueur est toujours complète, sans fondu (WFS-134), quelle que soit
    // sa taille à l'écran (vue haute, résolution dynamique) ; les autres suivent les niveaux de détail
    const auto lod = dessinJoueur ? vehicule_lod::Dessin<Model>{&m.parts, &m.parts, 0.f, 0} : vehicule_lod::dessin(m, depth, focalePx_, qualiteBasse, lots, fantome);
    auto emettre = [&](const std::vector<ModelPart>& parts, float tramage) {   // tramage : 0 aucun, sinon code de scene.frag
    for (size_t pi = 0; pi < parts.size(); pi++) {
        const ModelPart& part = parts[pi];
        const Material& mat = m.mats[part.material];
        Pending p{m.vb.buf, m.ib.buf, part.firstIndex, part.indexCount, part.vertexOffset, &mat, {}, depth};
        p.joueur = dessinJoueur;   // WFS-322 (accroche) : la sonde posée au-dessus de la voiture ne la capture pas
        p.famille = FAM_VOITURES;   // WFS-317 (I9)
        // WFS-134 (RG-11) : roues fixes → pièces d'un même matériau qui se suivent dans les indices (partie fixe + 4 roues,
        // WFS-60) dessinées en un seul appel
        if (!anim) while (pi + 1 < parts.size() && parts[pi + 1].material == part.material && parts[pi + 1].vertexOffset == part.vertexOffset && parts[pi + 1].feuArriere == part.feuArriere &&
                          parts[pi + 1].firstIndex == p.first + p.count) p.count += parts[++pi].indexCount;
        p.vis = vis;   // WFS-82
        p.pc.model = anim && part.roue >= 0 ? wr[part.roue] : world;
        p.pc.color = mat.color;
        if (paint && (mat.flags & MAT_PAINT)) p.pc.color = {paint->x, paint->y, paint->z, 1};
        p.pc.params = {mat.metal, mat.rough, mat.cutoff, (float)mat.flags};
        p.pc.emissive = {mat.emissive.x, mat.emissive.y, mat.emissive.z, tramage != 0 ? tramage : mat.detail};   // w : finesse (WFS-42) ; WFS-134 : tramage (< 0)
        // WFS-134 (RG-11) : Basse : au-delà de 15 m, pièces très fines (< 4 mm : logos, grilles, fils) non dessinées
        if (qualiteBasse && depth > 15.f && mat.detail > 0) continue;
        // WFS-134 (RG-11) : Basse, au-delà de 15 m : petites pièces (< 30 cm : logos, rétroviseurs, grilles, pièces de
        // l'habitacle) non dessinées, sauf feux, peinture et vitres (silhouette inchangée, moins d'appels de dessin)
        if (qualiteBasse && depth > 15.f && mat.taille > 0 && mat.taille < 0.3f && !part.feuArriere &&
            !(mat.flags & (MAT_TAIL | MAT_HEAD | MAT_PAINT | MAT_GLASS))) continue;
        if (fantome) {   // WFS-134 (RG-07) : toutes les pièces translucides (scene.frag, F_FANTOME), après les opaques
            p.pc.params.w = (float)(mat.flags | MAT_BLEND | MAT_FANTOME);
            transparent_.push_back(p);
            continue;
        }
        // WFS-09 : pièces des feux (repérées par matériau) allumées la nuit : rouge (plus fort au freinage), phares blancs
        // WFS-109 : feu arrière découpé (RG-13) traité comme un MAT_TAIL : verre rouge, sans l'émissif blanc d'origine ;
        // 3e feu stop (RG-03) : éteint hors freinage ; stop : rouge vif de jour comme de nuit, transition de 80 ms (RG-02)
        if (part.feuArriere == 1) { p.pc.emissive = {0, 0, 0, p.pc.emissive.w}; p.pc.color = {p.pc.color.x * feux::VERRE.x, p.pc.color.y * feux::VERRE.y, p.pc.color.z * feux::VERRE.z, p.pc.color.w}; }
        if ((mat.flags & MAT_TAIL) || part.feuArriere) {
            const float k = feux::emissionArriere(fx.nuit && part.feuArriere != 2, stopK);
            p.pc.emissive.x += k; p.pc.emissive.y += k * 0.02f; p.pc.emissive.z += k * 0.04f;
        }
        if (fx.nuit && (mat.flags & MAT_HEAD)) { p.pc.emissive.x += 2.4f; p.pc.emissive.y += 2.2f; p.pc.emissive.z += 1.9f; }
        if (mat.filament) {   // WFS-42 : fils plus fins qu'un pixel → mélangés selon leur couverture, avant les vitres
            p.pc.params.w = (float)(mat.flags | MAT_BLEND); p.depth += 0.05f;
            transparent_.push_back(p);
        } else if (mat.flags & MAT_BLEND) transparent_.push_back(p);
        else opaqueQ_.push_back(p);
    }
    };
    const size_t o0 = opaqueQ_.size(), t0 = transparent_.size();
    emettre(*lod.a, lod.t > 0.f ? -(3.f + lod.t) : 0.f);   // niveau a : pixels où le seuil ≥ t
    if (lod.t > 0.f) emettre(*lod.b, -(1.f + lod.t));    // niveau b : pixels où le seuil < t
    if (releverVoitures) {   // WFS-335 (RG-01) : banc seulement
        ReleveVoiture rv; rv.distance = depth; rv.niveau = lod.niveau; rv.joueur = dessinJoueur;
        rv.px = vehicule_lod::pixels(std::max({m.bmax.x - m.bmin.x, m.bmax.y - m.bmin.y, m.bmax.z - m.bmin.z}), focalePx_, depth);
        for (size_t i = o0; i < opaqueQ_.size(); i++) { rv.triangles += opaqueQ_[i].count / 3; rv.appels++; }
        for (size_t i = t0; i < transparent_.size(); i++) { rv.triangles += transparent_[i].count / 3; rv.appels++; }
        releve_.push_back(rv);
    }
}

void Renderer::draw(const Mesh& m, const Material& mat, const mat4& world, float roadHalfWidth) {
    if (!inFrame_ || sceneFigee() || !m.indexCount) return;   // WFS-329 : fond figé : dessins 3D ignorés
    const masquage::Sphere sp = masquage::monde(world, {m.centre, m.rayon});   // WFS-265 : calculée une fois (classement, shader léger, fondu)
    const uint8_t vis = classer(sp);   // WFS-82
    if (!vis) return;
    Pending p{m.vb.buf, m.ib.buf, 0, m.indexCount, 0, &mat, {}, length(world.transformPoint({0, 0, 0}) - camPos_)};
    p.vis = vis;
    {   // WFS-263 (RG-01) : variante légère si l'objet entier est au-delà de 70 m (fin du fondu de scene.frag) et n'est ni
        // voiture, ni route, ni transparent, ni vitre d'immeuble (mêmes exclusions que F_COMPLET de scene.frag)
        static_assert(shader_leger::COMPLET == (MAT_PAINT | MAT_GLASS | MAT_WHEEL | MAT_TAIL | MAT_HEAD | MAT_FANTOME | MAT_ROAD | MAT_BLEND | MAT_INTERIEUR),
                      "shader_leger.h : exclusions différentes des drapeaux du moteur");
        p.loin = shader_leger::eligible(mat.flags, length(sp.c - camPos_), sp.r);   // WFS-264 (RG-06) : règle isolée et testée
    }
    p.pc.model = world; p.pc.color = mat.color;
    p.pc.params = {mat.metal, mat.rough, mat.cutoff, (float)mat.flags};
    p.pc.emissive = {mat.emissive.x, mat.emissive.y, mat.emissive.z, roadHalfWidth};
    {   // WFS-265 (RG-01) : petit objet sur ses 15 derniers mètres avant sa distance maximale (masquage::distanceMax) : fondu tramé
        // au lieu d'une coupure nette ; route, trottoirs et sol jamais (bandes de la route, toujours entières)
        if (sp.r >= 0 && !(mat.flags & (MAT_ROAD | MAT_WALK | MAT_GROUND))) {
            const float part = lod_distance::partVisible(length(sp.c - camPos_) - sp.r, masquage::distanceMax(sp.r, qualiteBasse, finBrouillard_));
            if (part < 1.f) p.pc.emissive.w = lod_distance::combinerFondu(0.f, part);
        }
    }
    // ombre de contact sous les voitures (RG-05) ; WFS-134 (RG-05) : opacité 0,45 partout, ombre du soleil active ou non
    if (mat.flags & MAT_SHADOWBLOB) { p.pc.color.w *= 0.45f / 0.75f; ombreFaite_ = true; } else ombreFaite_ = false;
    if (mat.flags & MAT_BLEND) transparent_.push_back(p); else opaqueQ_.push_back(p);
}

void Renderer::drawSkinned(const SkinnedMesh& m, const Material& mat, const mat4& world, const mat4* os, int nOs, uint32_t first, uint32_t count,
                           const ApparencePersonnage* ap, float fondu) {
    if (!inFrame_ || sceneFigee() || !m.indexCount || !os || nOs <= 0) return;   // WFS-329 : fond figé : dessins 3D ignorés
    const masquage::Sphere sp = masquage::monde(world, {m.centre, m.rayon});
    // WFS-265 (RG-01) : personnes au loin effacées en fondu tramé (lod_distance.h) ; la simulation et leur nombre ne changent pas
    const float part = lod_distance::partVisible(length(sp.c - camPos_), lod_distance::personnesMax(qualiteBasse));
    if (part <= 0.f) { masques_++; return; }   // compté comme objet masqué (journal du banc, remarque du lead dev, PR #268)
    fondu = lod_distance::combinerFondu(fondu, part);
    const uint8_t vis = classer(sp);   // WFS-82 : avant de prendre une place
    if (!vis) return;
    if (skinUsed_ >= SKIN_MAX_INSTANCES) return;   // au-delà : ignorée (RG-05 de WFS-63)
    const int n = std::min(nOs, SKIN_MAX_OS);
    const VkDeviceSize ofs = OS_SLOT * (VkDeviceSize)skinUsed_++;
    std::memcpy((uint8_t*)osBuf_[cur_].mapped + ofs, os, sizeof(mat4) * (size_t)n);
    {   // WFS-72 : apparence (look[0].w = 1 : active ; 0 : texture seule, rendu de WFS-63)
        const vec3 z{};
        const vec3* c[6] = {ap ? &ap->peau : &z, ap ? &ap->cheveux : &z, ap ? &ap->haut : &z, ap ? &ap->bas : &z,
                            ap ? &ap->chaussures : &z, ap ? &ap->maillot : &z};
        vec4 look[8];
        for (int i = 0; i < 6; i++) look[i] = {c[i]->x, c[i]->y, c[i]->z, 0};
        look[0].w = ap ? 1.f : 0.f;
        look[6] = ap ? ap->vetement : vec4{}; look[7] = ap ? ap->corps : vec4{};
        std::memcpy((uint8_t*)osBuf_[cur_].mapped + ofs + OS_PALETTE, look, sizeof(look));
    }
    Pending p{m.vb.buf, m.ib.buf, first, count ? count : m.indexCount - first, 0, &mat, {}, length(world.transformPoint({0, 0, 0}) - camPos_)};
    p.famille = FAM_PERSONNES;   // WFS-317 (I9)
    p.pc.model = world; p.pc.color = mat.color;
    p.pc.params = {mat.metal, mat.rough, mat.cutoff, (float)mat.flags};
    p.pc.emissive = {mat.emissive.x, mat.emissive.y, mat.emissive.z, fondu};   // WFS-257 : tramage du fondu (fonduLod), 0 sinon
    p.osOfs = (int32_t)ofs; p.vis = vis; p.i16 = m.i16;
    opaqueQ_.push_back(p);   // maillages à squelette : opaques (ou alpha découpé) seulement
}

// ---------- WFS-09 : quads transitoires, halos, feux des voitures ----------
void Renderer::drawQuads(const Vertex* v, int nQuads, const Material& mat) {
    if (!inFrame_ || sceneFigee() || nQuads <= 0 || quadsUsed_ + nQuads > QUADS_MAX) return;   // WFS-329 : fond figé : dessins 3D ignorés
    std::memcpy((Vertex*)quadVb_[cur_].mapped + (size_t)quadsUsed_ * 4, v, sizeof(Vertex) * 4 * (size_t)nQuads);
    vec3 c{};
    for (int i = 0; i < nQuads * 4; i++) c += vec3{v[i].px, v[i].py, v[i].pz};
    c = c * (1.f / (float)(nQuads * 4));
    Pending p{quadVb_[cur_].buf, quadIb_.buf, 0, (uint32_t)nQuads * 6, quadsUsed_ * 4, &mat, {}, length(c - camPos_)};
    p.pc.model = mat4::identity(); p.pc.color = mat.color;
    p.pc.params = {mat.metal, mat.rough, mat.cutoff, (float)mat.flags};
    p.pc.emissive = {0, 0, 0, 0};
    quadsUsed_ += nQuads;
    transparent_.push_back(p);
}

void Renderer::halo(const vec3& c, float taille, const vec3& col) {
    const vec3 r = vec3{view_.at(0, 0), view_.at(0, 1), view_.at(0, 2)} * taille, u = vec3{view_.at(1, 0), view_.at(1, 1), view_.at(1, 2)} * taille;
    const vec3 a = c - r - u, b = c + r - u, d = c + r + u, e = c - r + u;
    const Vertex q[4] = {{a.x, a.y, a.z, col.x, col.y, col.z, 0, 0}, {b.x, b.y, b.z, col.x, col.y, col.z, 1, 0},
                         {d.x, d.y, d.z, col.x, col.y, col.z, 1, 1}, {e.x, e.y, e.z, col.x, col.y, col.z, 0, 1}};
    drawQuads(q, 1, haloMat_);
}

void Renderer::haloSol(const vec3& c, const vec3& r, const vec3& f, float hr, float hf, const vec3& col) {
    const vec3 a = c - r * hr - f * hf, b = c + r * hr - f * hf, d = c + r * hr + f * hf, e = c - r * hr + f * hf;
    const Vertex q[4] = {{a.x, a.y, a.z, col.x, col.y, col.z, 0, 0}, {b.x, b.y, b.z, col.x, col.y, col.z, 1, 0},
                         {d.x, d.y, d.z, col.x, col.y, col.z, 1, 1}, {e.x, e.y, e.z, col.x, col.y, col.z, 0, 1}};
    drawQuads(q, 1, haloMat_);
}


// setLights du jeu web (js/cars/models.js : tailGlow, tailGlowBrake, revGlow) revu pour un rendu naturel (WFS-84)
void Renderer::feuxVoiture(const Model& m, const mat4& w, const FeuxVoiture& f) {
    using namespace feux;
    auto lin = [](unsigned h, float k) {
        auto c = [&](int sh) { return std::pow(((h >> sh) & 255) / 255.f, 2.2f); };
        return vec3{c(16), c(8), c(0)} * (k * FEUX_HDR);
    };
    const FeuxModele& F = m.feux;
    const float ech = length(w.transformDir({1, 0, 0}));
    const vec3 arriere = normalize(w.transformDir({0, 0, 1})), avant = arriere * -1.f;   // avant de la voiture vers −Z
    if (f.nuit && !f.joueur) phares::noterAutre({w.transformPoint({0, F.avantY, F.avant}), avant, normalize(w.transformDir({1, 0, 0})), F.avantX * ech,   // WFS-226
                                                 (F.avantY - m.bmin.y) * ech, w.transformPoint({0, m.bmin.y, F.avant})});
    // un feu : cœur net + lueur douce, selon la distance et le côté d'où on le regarde
    auto feu = [&](const vec3& p, const vec3& dehors, float coeur, float lueur, const vec3& col, float iCoeur, float iLueur) {
        const vec3 v = camPos_ - p;
        const float d = length(v);
        const float face = std::clamp((dot(v * (1.f / std::max(d, 1e-3f)), dehors) + 0.1f) / 0.45f, 0.f, 1.f);   // de dos : rien
        if (face <= 0.f) return;
        halo(p, std::max(coeur * ech, d * POINT_MIN), col * (iCoeur * face));
        const float kl = 1.f - std::clamp((d - LUEUR_DEBUT) / (LUEUR_FIN - LUEUR_DEBUT), 0.f, 1.f);
        if (kl > 0.f) halo(p, lueur * ech, col * (iLueur * face * kl));
    };
    const vec3 rouge = lin(0xff1a30, 1.f), blanc = lin(0xf4f6ff, 1.f), phare = lin(0xfff1d6, 1.f);
    // WFS-109 : niveau du stop (transition de 80 ms, RG-02) ; le jour, seule la lueur courte du stop et du recul (RG-04)
    const float s = f.frein >= 0 ? std::clamp(f.frein, 0.f, 1.f) : (f.stop ? 1.f : 0.f);
    // RG-01 : le jour, aucun halo plaqué : seul le feu du modèle s'allume. WFS-182 (RG-02) : sans pièce de feu reconnue,
    // aucun halo, ni de jour ni de nuit (l'ancien repli aux proportions par défaut flottait dans le vide) ; le chargement
    // écrit « feu non trouvé : <voiture> » au journal
    const float iCoeur = F.pieceArriere && f.nuit ? I_COEUR + (I_COEUR_STOP - I_COEUR) * s : 0.f;
    const float iLueur = f.nuit ? I_LUEUR + (I_LUEUR_STOP - I_LUEUR) * s : I_LUEUR_STOP * s;
    for (int side : {-1, 1}) {
        // RG-01 : lueur courte centrée sur la pièce du feu (arrière du verre), plus sur le pare-chocs
        const float zF = F.arriereZ + 0.04f;   // WFS-171 : arrière de la pièce du feu, ou de la caisse à sa hauteur (repli)
        if (iCoeur > 0) feu(w.transformPoint({side * F.arriereX * RENTRE, F.arriereY, zF}), arriere, COEUR, LUEUR, rouge, iCoeur, iLueur);
        // WFS-171 (RG-03, ANO-78) : feu de recul dans la partie intérieure basse du bloc découpé (mesurée au chargement),
        // plus sur la tôle du hayon ; sans bloc mesuré, position d'avant
        const vec3 pr = F.reculTrouve ? vec3{side * F.recul.x, F.recul.y, F.recul.z + 0.01f}
                                      : vec3{side * std::max(0.15f, F.arriereX - 0.24f), F.arriereY - 0.04f, zF + 0.01f};
        if (f.recul) feu(w.transformPoint(pr), arriere, RECUL_COEUR, RECUL_LUEUR, blanc, I_RECUL_COEUR, I_RECUL_LUEUR);
        if (f.nuit) feu(w.transformPoint({side * F.avantX, F.avantY, F.avant - 0.02f}), avant, PHARE_COEUR, PHARE_LUEUR, phare, I_PHARE_COEUR, I_PHARE_LUEUR);
    }
    if (!f.nuit) return;   // RG-04 : le jour, pas de reflet sur la route
    // WFS-171 (RG-02) : route mouillée avec reflets : seule la traînée floue du reflet de WFS-142, pas de lueur en plus
    if (refletsOn_) return;
    if (length(w.transformPoint({0, 0, F.arriere}) - camPos_) > SOL_DISTANCE) return;
    // WFS-171 (RG-01) : lueur des deux feux sur la chaussée, collée au pare-chocs, bord progressif : éclairement réel
    // (cd du feu × eclairementSol) changé en luminance d'asphalte, fenêtré à zéro sur les bords de la grille
    using namespace feux_modele;
    const float cd = CD_POSITION_SOL + (CD_STOP_SOL - CD_POSITION_SOL) * s;
    const float h = std::max(0.3f, F.arriereY - m.bmin.y), xl = F.arriereX * RENTRE;
    const float zG = (F.pieceArriere ? F.arriereZ : F.arriere - 0.1f);   // glace du feu (≈ 10 cm devant le pare-chocs)
    const float hw = F.largeur * 0.5f + SOL_BORD, z0 = F.arriere - SOL_SOUS, z1 = F.arriere + SOL_PORTEE;
    auto lisse = [](float a, float b, float x) { const float t = std::clamp((x - a) / (b - a), 0.f, 1.f); return t * t * (3 - 2 * t); };
    Vertex v[SOL_NX * SOL_NZ * 4];
    vec3 p[SOL_NZ + 1][SOL_NX + 1], col[SOL_NZ + 1][SOL_NX + 1];
    for (int k = 0; k <= SOL_NZ; k++) for (int i = 0; i <= SOL_NX; i++) {
        const float x = -hw + 2 * hw * i / SOL_NX, z = z0 + (z1 - z0) * k / SOL_NZ;
        const float e = cd * (eclairementSol(z - zG, x - xl, h) + eclairementSol(z - zG, x + xl, h));
        const float fen = lisse(z0, F.arriere, z) * lisse(z1, F.arriere + 0.5f, z) * lisse(hw, 0.35f * hw, std::abs(x));
        p[k][i] = w.transformPoint({x, m.bmin.y + 0.03f, z});
        col[k][i] = ROUGE_SOL * (luminanceSol(e) * fen);
    }
    int n = 0;
    auto sommet = [&](int k, int i) { const vec3& q = p[k][i]; const vec3& c = col[k][i]; v[n++] = {q.x, q.y, q.z, c.x, c.y, c.z, 0.5f, 0.5f}; };
    for (int k = 0; k < SOL_NZ; k++) for (int i = 0; i < SOL_NX; i++) { sommet(k, i); sommet(k, i + 1); sommet(k + 1, i + 1); sommet(k + 1, i); }
    drawQuads(v, SOL_NX * SOL_NZ, haloMat_);   // uv au centre : dégradé radial du halo = 1, la couleur des sommets fait le profil
}

void Renderer::uiRect(float x, float y, float w, float h, const vec4& c) {
    UiVertex a{x, y, -1, -1, c.x, c.y, c.z, c.w}, b{x + w, y, -1, -1, c.x, c.y, c.z, c.w},
             cc{x + w, y + h, -1, -1, c.x, c.y, c.z, c.w}, d{x, y + h, -1, -1, c.x, c.y, c.z, c.w};
    if (ui_vtx_.size() + 6 > UI_MAX_VERTS) return;
    ui_vtx_.insert(ui_vtx_.end(), {a, b, cc, a, cc, d});
}

void Renderer::uiSetFontAtlas(const Image& atlas) {
    if (!fontSet_) {
        VkDescriptorSetAllocateInfo dai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        dai.descriptorPool = pool_; dai.descriptorSetCount = 1; dai.pSetLayouts = &postLayout_;
        VK_OK(vkAllocateDescriptorSets(vk_->device, &dai, &fontSet_));
    }
    // WFS-100 (RG-W2, VUID-vkCmdDraw-None-08114) : jamais de vue nulle liée. Au redémarrage du moteur (changement de carte),
    // l'atlas n'existe pas encore : texture blanche 1 × 1 en attendant. Binding 1 (bloom de postLayout_) : blanc aussi.
    VkDescriptorImageInfo ii[2] = {{sampler_, atlas.view ? atlas.view : white.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
                                   {clampSampler_, white.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
    VkWriteDescriptorSet wr[2]{};
    for (int k = 0; k < 2; k++) {
        wr[k].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; wr[k].dstSet = fontSet_; wr[k].dstBinding = (uint32_t)k;
        wr[k].descriptorCount = 1; wr[k].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; wr[k].pImageInfo = &ii[k];
    }
    vkUpdateDescriptorSets(vk_->device, 2, wr, 0, nullptr);
}

void Renderer::uiGlyph(float x, float y, float w, float h, float u0, float v0, float u1, float v1, const vec4& c, float biais) {
    // uv décalés de +2 : le shader d'interface sait qu'il doit lire l'atlas de police ; biais : haut décalé (WFS-48)
    UiVertex a{x + biais, y, u0 + 2, v0, c.x, c.y, c.z, c.w}, b{x + w + biais, y, u1 + 2, v0, c.x, c.y, c.z, c.w},
             cc{x + w, y + h, u1 + 2, v1, c.x, c.y, c.z, c.w}, d{x, y + h, u0 + 2, v1, c.x, c.y, c.z, c.w};
    if (ui_vtx_.size() + 6 > UI_MAX_VERTS) return;
    ui_vtx_.insert(ui_vtx_.end(), {a, b, cc, a, cc, d});
}

// WFS-65 : quadrilatère libre (sommets dans l'ordre), couleurs interpolées entre les sommets (vignette, lignes)
void Renderer::uiQuad(const UiVertex q[4]) {
    if (ui_vtx_.size() + 6 > UI_MAX_VERTS) return;
    ui_vtx_.insert(ui_vtx_.end(), {q[0], q[1], q[2], q[0], q[2], q[3]});
}

void Renderer::uiCircle(float cx, float cy, float r, const vec4& c) {
    UiVertex a{cx - r, cy - r, 0, 0, c.x, c.y, c.z, c.w}, b{cx + r, cy - r, 1, 0, c.x, c.y, c.z, c.w},
             cc{cx + r, cy + r, 1, 1, c.x, c.y, c.z, c.w}, d{cx - r, cy + r, 0, 1, c.x, c.y, c.z, c.w};
    if (ui_vtx_.size() + 6 > UI_MAX_VERTS) return;
    ui_vtx_.insert(ui_vtx_.end(), {a, b, cc, a, cc, d});
}

void Renderer::endFrame() {
    if (!inFrame_) return;
    const double tFin = nowSeconds();   // WFS-222 (RG-01)
    VkCtx& vk = *vk_;
    FrameRes& r = fr_[cur_];
    const bool figee = sceneFigee();   // WFS-329 : fond figé : de l'horodatage 0 à 3 directement, rien avant la passe finale
    if (figee) {
        if (tsPool_) {
            vkCmdResetQueryPool(r.cb, tsPool_, (uint32_t)cur_ * 5, 5);
            for (uint32_t q = 0; q < 4; q++) vkCmdWriteTimestamp(r.cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, tsPool_, (uint32_t)cur_ * 5 + q);
        }
    } else {
    shadowPass(r.cb);
    // objets transparents (vitres, halos) : du plus loin au plus proche, triés une fois pour les deux passes
    std::stable_sort(transparent_.begin(), transparent_.end(), [](const Pending& a, const Pending& b) { return a.depth > b.depth; });
    refletPass(r.cb);   // WFS-16 : reflet de la scène (route mouillée seulement)
    if (tsPool_) vkCmdWriteTimestamp(r.cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, tsPool_, (uint32_t)cur_ * 5 + 2);
    passes::avantScene(*this, r.cb);   // WFS-150 (RG-01) : ombres → pré-passe → passes d'écran (capture d'environnement)
    // passe de scène : ciel, objets opaques (dans l'ordre d'appel), puis transparents
    VkClearValue clears[3]{};
    clears[0].color = {{0, 0, 0, 1}}; clears[1].depthStencil = {0.f, 0};   // profondeur inversée : 0 = loin
    VkRenderPassBeginInfo sbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    sbi.renderPass = scenePass_; sbi.framebuffer = sceneFb_; sbi.renderArea = {{0, 0}, {sw_, sh_}};
    sbi.clearValueCount = 2; sbi.pClearValues = clears;
    vkCmdBeginRenderPass(r.cb, &sbi, VK_SUBPASS_CONTENTS_INLINE);
    {
        VkViewport vpt{0, 0, (float)sw_, (float)sh_, 0, 1};
        VkRect2D sc{{0, 0}, {sw_, sh_}};
        vkCmdSetViewport(r.cb, 0, 1, &vpt); vkCmdSetScissor(r.cb, 0, 1, &sc);
    }
    vkCmdBindDescriptorSets(r.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, sceneLayout_, 0, 1, &r.frameSet, 0, nullptr);
    if (rendu221) {   // banc : ordre d'avant WFS-222 (ciel plein écran d'abord)
        vkCmdBindPipeline(r.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, sky_);
        vkCmdDraw(r.cb, 3, 1, 0, 0);
    }
    // WFS-222 : opaques du plus proche au plus loin (les pixels cachés sont rejetés avant le shader), puis le ciel dans les
    // seuls pixels restés vides (profondeur 0) ; à profondeur égale, l'ordre d'appel est gardé (tri stable)
    ordreOpaques_.clear();
    for (uint32_t i = 0; i < (uint32_t)opaqueQ_.size(); i++) if (opaqueQ_[i].vis & VIS_CAM) ordreOpaques_.push_back(i);   // WFS-82 : hors champ (gardé pour l'ombre ou le reflet)
    if (!rendu221) std::stable_sort(ordreOpaques_.begin(), ordreOpaques_.end(), [&](uint32_t a, uint32_t b) { return opaqueQ_[a].depth < opaqueQ_[b].depth; });
    VkPipeline lie = VK_NULL_HANDLE;
    for (uint32_t i : ordreOpaques_) {
        const Pending& p = opaqueQ_[i];
        const bool d = rendu221 || aDecoupe(p);   // WFS-222 : shader sans discard si l'objet n'a pas de découpe
        // WFS-246 (levier 1) : sans découpe et sans MAT_DOUBLE : faces arrière retirées
        // route, trottoirs et sol (bandes de game.cpp, tournées dans l'autre sens, toujours vus de dessus) : rien à retirer
        // RG-02.1 : objet posé en miroir (échelle négative : déterminant < 0, ordre des sommets inversé) : deux faces gardées
        const mat4& mo = p.pc.model;
        const float det = mo.at(0, 0) * (mo.at(1, 1) * mo.at(2, 2) - mo.at(2, 1) * mo.at(1, 2)) - mo.at(0, 1) * (mo.at(1, 0) * mo.at(2, 2) - mo.at(2, 0) * mo.at(1, 2)) +
                          mo.at(0, 2) * (mo.at(1, 0) * mo.at(2, 1) - mo.at(2, 0) * mo.at(1, 1));
        const bool cull = !d && facesArriere_ && det > 0 && p.mat && !(p.mat->flags & (MAT_DOUBLE | MAT_GLASS | MAT_ROAD | MAT_WALK | MAT_GROUND));
        const bool leger = p.loin && !d && legerActif_;   // WFS-263 (RG-01) : décor entièrement au-delà de 70 m
        const VkPipeline voulu = p.osOfs >= 0 ? (d ? skinOpaqueDecoupe_ : cull ? skinOpaqueCull_ : skinOpaque_)
                                              : (d ? opaqueDecoupe_ : leger ? (cull ? opaqueLeger_ : opaqueLegerDouble_) : cull ? opaqueCull_ : opaque_);   // WFS-63
        if (voulu != lie) { vkCmdBindPipeline(r.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, voulu); lie = voulu; }
        flushDraw(p);
    }
    if (!rendu221) {
        vkCmdBindPipeline(r.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, skyApres_);
        vkCmdDraw(r.cb, 3, 1, 0, 0);
    }
    passes::dansScene(*this, r.cb, EtapeRendu::Scene);   // WFS-150 : passes enfichables de la scène (opaques)
    // objets transparents (vitres) : du plus loin au plus proche
    if (!transparent_.empty()) {
        VkPipeline lieT = VK_NULL_HANDLE;
        for (const Pending& p : transparent_) {
            if (!(p.vis & VIS_CAM)) continue;   // WFS-82
            // WFS-09 : tache des phares (multiplicative), halos (additifs), sinon mélange alpha
            const VkPipeline voulu = (p.mat->flags & MAT_POOL) ? poolPipe_ : (p.mat->flags & MAT_ADD) ? addPipe_ : blend_;
            if (voulu != lieT) { vkCmdBindPipeline(r.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, voulu); lieT = voulu; }
            flushDraw(p);
        }
    }
    passes::dansScene(*this, r.cb, EtapeRendu::Transparents);   // WFS-150
    vkCmdEndRenderPass(r.cb);
    if (bloom_) bloom_->dessiner(*this, r.cb);   // WFS-100 : chaîne du bloom, comptée avec la passe de scène
    // WFS-09 : coût mesuré aussi la nuit (sans ombres) : passes de reflet et de scène (+ bloom, WFS-100)
    if (tsPool_) vkCmdWriteTimestamp(r.cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, tsPool_, (uint32_t)cur_ * 5 + 3);
    passes::apresScene(*this, r.cb);   // WFS-150 : post-traitement enfichable (avant la passe finale)
    sceneValide_ = true;   // WFS-329 : scène HDR de cette image gardée (relue telle quelle si le fond est figé ensuite)
    }

    // passe finale : tonemapping puis interface
    VkRenderPassBeginInfo rbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rbi.renderPass = finalPass_; rbi.framebuffer = finalFbs_[imageIndex_]; rbi.renderArea = {{0, 0}, vk.swapExtent};
    vkCmdBeginRenderPass(r.cb, &rbi, VK_SUBPASS_CONTENTS_INLINE);
    VkViewport vpt{0, 0, (float)vk.swapExtent.width, (float)vk.swapExtent.height, 0, 1};
    VkRect2D sc{{0, 0}, vk.swapExtent};
    vkCmdSetViewport(r.cb, 0, 1, &vpt); vkCmdSetScissor(r.cb, 0, 1, &sc);
    struct { float rot[4]; float params[4]; float bloom[4]; } pp{};   // bloom.x = 0 : passe finale (WFS-100)
    rotationMatrix(pp.rot);
    pp.params[0] = exposure_; pp.params[1] = vk.swapSrgb ? 1.f : 0.f;
    // WFS-15 / WFS-100 : force du bloom (calculée en début d'image) ; seuil 1,0 et genou 0,5 dans post.frag
    pp.params[2] = bloom_ ? bloom_->force : 0.f;
    pp.params[3] = eclairage::kVisionNuit();   // WFS-230 (accroche E13) : vision de nuit (Purkinje) ; 0 de jour
    pp.bloom[3] = echelleDyn_;   // WFS-246 (RG-07) : part de la cible de scène dessinée, agrandie à tout l'écran
    vkCmdBindPipeline(r.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, post_);
    vkCmdBindDescriptorSets(r.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, postPLayout_, 0, 1, &postSet_, 0, nullptr);
    vkCmdPushConstants(r.cb, postPLayout_, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof pp, &pp);
    vkCmdDraw(r.cb, 3, 1, 0, 0);
    if (!ui_vtx_.empty()) {
        std::memcpy(r.uiVb.mapped, ui_vtx_.data(), ui_vtx_.size() * sizeof(UiVertex));
        pp.params[0] = 1.f / width(); pp.params[1] = 1.f / height();
        vkCmdBindPipeline(r.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, ui_);
        if (fontSet_) vkCmdBindDescriptorSets(r.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, postPLayout_, 0, 1, &fontSet_, 0, nullptr);
        vkCmdPushConstants(r.cb, postPLayout_, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 32, &pp);
        VkDeviceSize zero = 0;
        vkCmdBindVertexBuffers(r.cb, 0, 1, &r.uiVb.buf, &zero);
        vkCmdDraw(r.cb, (uint32_t)ui_vtx_.size(), 1, 0, 0);
    }
    vkCmdEndRenderPass(r.cb);
    if (tsPool_) { vkCmdWriteTimestamp(r.cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, tsPool_, (uint32_t)cur_ * 5 + 4); tsWritten_[cur_] = true; }   // WFS-246 (RG-06)
    VK_OK(vkEndCommandBuffer(r.cb));

    VkPipelineStageFlags wait = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    si.commandBufferCount = 1; si.pCommandBuffers = &r.cb;
    if (!vk.offscreen) {
        si.waitSemaphoreCount = 1; si.pWaitSemaphores = &r.acquired; si.pWaitDstStageMask = &wait;
        si.signalSemaphoreCount = 1; si.pSignalSemaphores = &r.done;
    }
    VK_OK(vkQueueSubmit(vk.queue, 1, &si, r.fence));
    if (!vk.offscreen) {
        VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
        pi.waitSemaphoreCount = 1; pi.pWaitSemaphores = &r.done;
        pi.swapchainCount = 1; pi.pSwapchains = &vk.swapchain; pi.pImageIndices = &imageIndex_;
        VkResult res = vkQueuePresentKHR(vk.queue, &pi);
        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) { vk.waitIdle(); if (vk.createSwapchain()) resize(); }
    }
    inFrame_ = false;
    cur_ = (cur_ + 1) % FRAMES;
    chrono_.finImageMs = (nowSeconds() - tFin) * 1000;
}

bool Renderer::capturePng(const std::string& path) {
    VkCtx& vk = *vk_;
    if (!vk.offscreen) return false;
    vk.waitIdle();
    uint32_t w = vk.offW, h = vk.offH;
    Buffer buf = vk.createBuffer((VkDeviceSize)w * h * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true);
    VkCommandBuffer cb = vk.beginOneShot();
    VkBufferImageCopy r{};
    r.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}; r.imageExtent = {w, h, 1};
    vkCmdCopyImageToBuffer(cb, vk.offTarget.img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buf.buf, 1, &r);
    vk.endOneShot(cb);
    int ok = stbi_write_png(path.c_str(), (int)w, (int)h, 4, buf.mapped, (int)w * 4);
    vk.destroy(buf);
    return ok != 0;
}

void Renderer::shutdown() {
    if (!vk_) return;
    VkDevice d = vk_->device;
    vk_->waitIdle();
    enregistrerCachePipelines();   // WFS-262 (RG-01)
    if (cachePipelines_) vkDestroyPipelineCache(d, cachePipelines_, nullptr);
    cachePipelines_ = VK_NULL_HANDLE;
    passes::detruire(*this);   // WFS-150
    destroyTargets();
    if (bloom_) { bloom_->liberer(*this); delete bloom_; bloom_ = nullptr; }   // WFS-100
    for (auto& r : fr_) {
        vkDestroyFence(d, r.fence, nullptr); vkDestroySemaphore(d, r.acquired, nullptr); vkDestroySemaphore(d, r.done, nullptr);
        vk_->destroy(r.ubo); vk_->destroy(r.uiVb);
    }
    for (VkPipeline p : {opaque_, blend_, sky_, post_, ui_, shadowPipe_, shadowMaskPipe_, skinOpaque_, skinShadow_, skinShadowMask_, opaqueDecoupe_, skinOpaqueDecoupe_, skyApres_,
                         poolPipe_, addPipe_, refOpaque_, refBlend_, refSky_, refAdd_, opaqueCull_, skinOpaqueCull_, opaqueLeger_, opaqueLegerDouble_})
        if (p) vkDestroyPipeline(d, p, nullptr);
    for (int f = 0; f < FRAMES; f++) { vk_->destroy(refletUbo_[f]); vk_->destroy(quadVb_[f]); }
    vk_->destroy(quadIb_);
    if (refletPass_) vkDestroyRenderPass(d, refletPass_, nullptr);
    for (auto& b : osBuf_) vk_->destroy(b);
    if (skinLayout_) vkDestroyPipelineLayout(d, skinLayout_, nullptr);
    if (osLayout_) vkDestroyDescriptorSetLayout(d, osLayout_, nullptr);
    if (shadowFb_) vkDestroyFramebuffer(d, shadowFb_, nullptr);
    if (shadowPass_) vkDestroyRenderPass(d, shadowPass_, nullptr);
    if (shadowSampler_) vkDestroySampler(d, shadowSampler_, nullptr);
    if (tsPool_) vkDestroyQueryPool(d, tsPool_, nullptr);
    vk_->destroy(shadowMap_);
    for (auto m : modules_) vkDestroyShaderModule(d, m, nullptr);
    vkDestroyPipelineLayout(d, sceneLayout_, nullptr); vkDestroyPipelineLayout(d, postPLayout_, nullptr);
    vkDestroyRenderPass(d, scenePass_, nullptr); vkDestroyRenderPass(d, finalPass_, nullptr);
    vkDestroyDescriptorPool(d, pool_, nullptr);
    for (auto l : {frameLayout_, matLayout_, postLayout_}) vkDestroyDescriptorSetLayout(d, l, nullptr);
    vkDestroySampler(d, sampler_, nullptr); vkDestroySampler(d, clampSampler_, nullptr);
    if (samplerBasse_) vkDestroySampler(d, samplerBasse_, nullptr);   // WFS-144
    samplerBasse_ = VK_NULL_HANDLE; texBasse_ = false; matTex_.clear(); jeuxMat_.clear();   // WFS-187 : pool détruit
    vk_->vueDetruite = nullptr; vk_->vueDetruiteCtx = nullptr;
    vk_->msaa = msaaMax_;   // MSAA du GPU rendu tel quel (init suivant : retour d'arrière-plan)
    vk_->destroy(white); vk_->destroy(flatNormal);
    destroy(ombreQuad_);   // WFS-134
    // WFS-100 (RG-W1, ANO-52) : objet remis à neuf, pour qu'un init() sur le même Renderer recrée tout (retour d'arrière-plan)
    scenePass_ = finalPass_ = shadowPass_ = refletPass_ = VK_NULL_HANDLE;
    sceneFb_ = shadowFb_ = VK_NULL_HANDLE; finalFbs_.clear(); refletFb_[0] = refletFb_[1] = VK_NULL_HANDLE;
    frameLayout_ = matLayout_ = postLayout_ = osLayout_ = VK_NULL_HANDLE;
    sceneLayout_ = postPLayout_ = skinLayout_ = VK_NULL_HANDLE;
    opaque_ = blend_ = sky_ = post_ = ui_ = shadowPipe_ = shadowMaskPipe_ = skinOpaque_ = skinShadow_ = skinShadowMask_ = VK_NULL_HANDLE;
    opaqueDecoupe_ = skinOpaqueDecoupe_ = skyApres_ = VK_NULL_HANDLE;   // WFS-222
    opaqueCull_ = skinOpaqueCull_ = VK_NULL_HANDLE;   // WFS-246
    opaqueLeger_ = opaqueLegerDouble_ = VK_NULL_HANDLE;   // WFS-263
    poolPipe_ = addPipe_ = refOpaque_ = refBlend_ = refSky_ = refAdd_ = VK_NULL_HANDLE;
    pool_ = VK_NULL_HANDLE; sampler_ = clampSampler_ = shadowSampler_ = VK_NULL_HANDLE; tsPool_ = VK_NULL_HANDLE;
    postSet_ = fontSet_ = VK_NULL_HANDLE; osSet_[0] = osSet_[1] = refletSet_[0] = refletSet_[1] = VK_NULL_HANDLE;
    for (auto& r : fr_) { r.cb = VK_NULL_HANDLE; r.fence = VK_NULL_HANDLE; r.acquired = r.done = VK_NULL_HANDLE; r.frameSet = VK_NULL_HANDLE; }
    modules_.clear(); transparent_.clear(); opaqueQ_.clear(); ui_vtx_.clear();
    cur_ = 0; imageIndex_ = 0; inFrame_ = false; quadsUsed_ = 0; skinUsed_ = 0; tsWritten_[0] = tsWritten_[1] = false;
    vk_ = nullptr;
}

} // namespace wfs
