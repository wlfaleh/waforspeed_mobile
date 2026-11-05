#include <cstdlib>
#include "renderer.h"
#include <algorithm>
#include <cmath>
#include <cstring>
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

struct FrameUBO {
    mat4 viewProj, invViewProj;
    vec4 camPos, sunDir, sunColor, skyTop, skyHorizon, groundCol, fog, misc;
    mat4 shadowMat; vec4 shadowP;
    // WFS-45 : même ordre que shaders/common.glsl
    vec4 camFwd, fogLin, hemiSky, hemiGround, spotN;
    vec4 spotPos[8], spotDir[8], spotCol[8], spotCone[8];   // WFS-09 : 8 lumières
    vec4 nuitA, nuitB;                                      // WFS-09 / WFS-16 : route mouillée, reflets
};

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

bool Renderer::init(VkCtx& vk) {
    vk_ = &vk;
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
    VK_OK(vkCreateSampler(d, &sci, nullptr, &sampler_));
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
    VkDescriptorSetLayoutBinding b0[3] = {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                                          {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},    // carte d'ombre
                                          {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};   // reflet (WFS-16)
    VkDescriptorSetLayoutCreateInfo lci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    lci.bindingCount = 3; lci.pBindings = b0;
    VK_OK(vkCreateDescriptorSetLayout(d, &lci, nullptr, &frameLayout_));
    VkDescriptorSetLayoutBinding mb[3];
    for (uint32_t i = 0; i < 3; i++) mb[i] = {i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    lci.bindingCount = 3; lci.pBindings = mb;
    VK_OK(vkCreateDescriptorSetLayout(d, &lci, nullptr, &matLayout_));
    lci.bindingCount = 2; lci.pBindings = mb;   // WFS-100 : 0 = image source, 1 = bloom (passe finale) / niveau inférieur (remontée)
    VK_OK(vkCreateDescriptorSetLayout(d, &lci, nullptr, &postLayout_));

    VkDescriptorPoolSize sizes[3] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 8}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 * 1024 + 16 + 48},
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
    {   // WFS-09 : halos additifs ; tache des phares (MAT.headPool du jeu web : couleur 0xfff0dc, multiplicative)
        Material h; h.flags = MAT_ADD | MAT_BLEND; haloMat_ = makeMaterial(h, nullptr, nullptr, nullptr);
        Material t; t.flags = MAT_POOL | MAT_BLEND;
        t.color = {std::pow(0xff / 255.f, 2.2f), std::pow(0xf0 / 255.f, 2.2f), std::pow(0xdc / 255.f, 2.2f), 1};
        poolMat = makeMaterial(t, nullptr, nullptr, nullptr);
    }
    ETAPE_RENDU("matériaux, fin de Renderer::init");
    return true;
}

// ---------- ombres du soleil (WFS-07) : carte de profondeur fixe (2048, 1024 sur mobile), jamais changée par image ----------
void Renderer::createShadows() {
    VkCtx& vk = *vk_; VkDevice d = vk.device;
#ifdef __ANDROID__
    shadowSize_ = 1024;
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
        q.queryType = VK_QUERY_TYPE_TIMESTAMP; q.queryCount = 4 * FRAMES;   // ombre, reflet (WFS-16), scène
        VK_OK(vkCreateQueryPool(d, &q, nullptr, &tsPool_));
    }
    WFS_LOG("Ombres du soleil : carte %ux%u, filtre %s", shadowSize_, shadowSize_, linear ? "bilinéaire" : "au plus proche");
}

void Renderer::readShadowTiming() {
    if (!tsPool_ || !tsWritten_[cur_]) return;
    uint64_t t[4];
    if (vkGetQueryPoolResults(vk_->device, tsPool_, (uint32_t)cur_ * 4, 4, sizeof t, t, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT) != VK_SUCCESS) return;
    const double k = vk_->props.limits.timestampPeriod * 1e-6;
    shadowMsSum_ += (double)(t[1] - t[0]) * k;
    refletMsSum_ += (double)(t[2] - t[1]) * k;   // WFS-16 : passe de reflet (0 sans route mouillée)
    sceneMsSum_ += (double)(t[3] - t[2]) * k;    // WFS-42 : coût de la passe de scène, pour comparer avant / après
    if (++shadowMsN_ == 120) {
        WFS_LOG("Ombres : passe d'ombre %.2f ms GPU en moyenne (carte %u) ; passe de scène %.2f ms ; reflets %.2f ms (WFS-16)", shadowMsSum_ / shadowMsN_, shadowSize_,
                sceneMsSum_ / shadowMsN_, refletMsSum_ / shadowMsN_);
        shadowMsSum_ = 0; sceneMsSum_ = 0; refletMsSum_ = 0; shadowMsN_ = 0;
    }
}

// passe d'ombre : objets opaques vus du soleil (sauf sol, route, trottoirs : ils reçoivent l'ombre sans en projeter)
void Renderer::shadowPass(VkCommandBuffer cb) {
    if (tsPool_) { vkCmdResetQueryPool(cb, tsPool_, (uint32_t)cur_ * 4, 4); vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, tsPool_, (uint32_t)cur_ * 4); }
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
        for (const Pending& p : opaqueQ_) {
            if ((p.mat->flags & NO_CAST) || !(p.vis & VIS_OMBRE)) continue;   // WFS-82 : hors de la boîte d'ombre
            const bool masque = p.mat->flags & MAT_MASK;
            VkPipeline want = p.osOfs >= 0 ? (masque ? skinShadowMask_ : skinShadow_) : (masque ? shadowMaskPipe_ : shadowPipe_);   // WFS-63
            if (want != bound) { vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, want); bound = want; }
            flushDraw(p);
        }
    }
    vkCmdEndRenderPass(cb);
    if (tsPool_) vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, tsPool_, (uint32_t)cur_ * 4 + 1);
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
            w[i] = std::max(1u, R.sw_ >> (i + 1)); ht[i] = std::max(1u, R.sh_ >> (i + 1));
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
        struct { float rot[4]; float params[4]; float bloom[4]; } pp{{1, 0, 0, 1}, {R.exposure_, 0, 0, 0}, {mode, poidsNiveau, z, 0}};
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
    bool rotated = vk.transform == VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR || vk.transform == VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR;
    lw_ = rotated ? vk.swapExtent.height : vk.swapExtent.width;
    lh_ = rotated ? vk.swapExtent.width : vk.swapExtent.height;
    // scène 3D éventuellement rendue en plus petit (GPU lents), agrandie par la passe finale
    sw_ = std::max(1u, (uint32_t)(lw_ * vk.renderScale)); sh_ = std::max(1u, (uint32_t)(lh_ * vk.renderScale));
    const bool ms = vk.msaa != VK_SAMPLE_COUNT_1_BIT;

    hdr_ = vk.createImage(sw_, sh_, 1, HDR_FORMAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    if (ms) msaaColor_ = vk.createImage(sw_, sh_, 1, HDR_FORMAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, vk.msaa, VK_IMAGE_ASPECT_COLOR_BIT, true);
    depth_ = vk.createImage(sw_, sh_, 1, vk.depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, vk.msaa, VK_IMAGE_ASPECT_DEPTH_BIT, true);

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
    fci.width = sw_; fci.height = sh_; fci.layers = 1;
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
    WFS_LOG("Cibles de rendu : %ux%u (logique), scène %ux%u, MSAA x%d", lw_, lh_, sw_, sh_, (int)vk.msaa);
}

// ---------- WFS-16 : passe de reflet (scène vue dans le miroir de la chaussée), demi-résolution, une cible par image en vol ----------
// Image n : la passe miroir écrit refletColor_[n], puis la passe de scène la lit : jamais lue et écrite dans la même passe.
void Renderer::createReflets() {
    VkCtx& vk = *vk_; VkDevice d = vk.device;
    rw_ = std::max(1u, sw_ / 2); rh_ = std::max(1u, sh_ / 2);
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
    vk_->waitIdle();
    destroyTargets();
    createTargets();
    return true;
}

VkPipeline Renderer::makePipeline(VkShaderModule vs, VkShaderModule fs, int vertexKind, bool blend, bool premul, bool depthTest, bool depthWrite,
                                  VkRenderPass rp, VkSampleCountFlagBits samples, VkPipelineLayout layout, bool depthOnly, int melange) {
    VkPipelineShaderStageCreateInfo st[2]{};
    st[0].sType = st[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    st[0].stage = VK_SHADER_STAGE_VERTEX_BIT; st[0].module = vs; st[0].pName = "main";
    st[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT; st[1].module = fs; st[1].pName = "main";

    VkVertexInputBindingDescription bind{};
    VkVertexInputAttributeDescription attr[6]{};
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    if (vertexKind == 1) {   // scène : position, normale, uv
        bind = {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
        attr[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0};
        attr[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12};
        attr[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, 24};
        vi.vertexBindingDescriptionCount = 1; vi.pVertexBindingDescriptions = &bind;
        vi.vertexAttributeDescriptionCount = 3; vi.pVertexAttributeDescriptions = attr;
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
    VkPipeline p; VK_OK(vkCreateGraphicsPipelines(vk_->device, VK_NULL_HANDLE, 1, &gp, nullptr, &p));
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
    VkSampleCountFlagBits s = vk_->msaa;
    opaque_ = makePipeline(sv, sf, 1, false, false, true, true, scenePass_, s, sceneLayout_);
    blend_ = makePipeline(sv, sf, 1, true, false, true, false, scenePass_, s, sceneLayout_);
    sky_ = makePipeline(kv, kf, 0, false, false, false, false, scenePass_, s, sceneLayout_);
    post_ = makePipeline(pv, pf, 0, false, false, false, false, finalPass_, VK_SAMPLE_COUNT_1_BIT, postPLayout_);
    ui_ = makePipeline(uv, uf, 2, true, true, false, false, finalPass_, VK_SAMPLE_COUNT_1_BIT, postPLayout_);
    VkShaderModule hv = mod(SPV_SHADOW_VERT, sizeof(SPV_SHADOW_VERT)), hf = mod(SPV_SHADOW_FRAG, sizeof(SPV_SHADOW_FRAG));
    shadowPipe_ = makePipeline(hv, VK_NULL_HANDLE, 1, false, false, true, true, shadowPass_, VK_SAMPLE_COUNT_1_BIT, sceneLayout_, true);
    shadowMaskPipe_ = makePipeline(hv, hf, 1, false, false, true, true, shadowPass_, VK_SAMPLE_COUNT_1_BIT, sceneLayout_, true);
    // WFS-63 : maillages à squelette (même scene.frag / shadow.frag que les objets rigides)
    VkShaderModule kv2 = mod(SPV_SCENE_SKIN_VERT, sizeof(SPV_SCENE_SKIN_VERT)), kh = mod(SPV_SHADOW_SKIN_VERT, sizeof(SPV_SHADOW_SKIN_VERT));
    VkShaderModule kf2 = mod(SPV_SCENE_SKIN_FRAG, sizeof(SPV_SCENE_SKIN_FRAG));   // WFS-72 : vêtements (pedMaterial)
    skinOpaque_ = makePipeline(kv2, kf2, 3, false, false, true, true, scenePass_, s, skinLayout_);
    skinShadow_ = makePipeline(kh, VK_NULL_HANDLE, 3, false, false, true, true, shadowPass_, VK_SAMPLE_COUNT_1_BIT, skinLayout_, true);
    skinShadowMask_ = makePipeline(kh, hf, 3, false, false, true, true, shadowPass_, VK_SAMPLE_COUNT_1_BIT, skinLayout_, true);
    // WFS-09 : tache des phares (multiplicative) et halos (additifs) ; WFS-16 : passe de reflet (1 échantillon)
    poolPipe_ = makePipeline(sv, sf, 1, true, false, true, false, scenePass_, s, sceneLayout_, false, 1);
    addPipe_ = makePipeline(sv, sf, 1, true, false, true, false, scenePass_, s, sceneLayout_, false, 2);
    const VkSampleCountFlagBits s1 = VK_SAMPLE_COUNT_1_BIT;
    refOpaque_ = makePipeline(sv, sf, 1, false, false, true, true, refletPass_, s1, sceneLayout_);
    refBlend_ = makePipeline(sv, sf, 1, true, false, true, false, refletPass_, s1, sceneLayout_);
    refSky_ = makePipeline(kv, kf, 0, false, false, false, false, refletPass_, s1, sceneLayout_);
    refAdd_ = makePipeline(sv, sf, 1, true, false, true, false, refletPass_, s1, sceneLayout_, false, 2);
}

Material Renderer::makeMaterial(const Material& src, const Image* color, const Image* normal, const Image* mr) {
    Material m = src;
    if (normal && normal->view) m.flags |= MAT_HASNORMAL;
    if (mr && mr->view) m.flags |= MAT_HASMR;
    VkDescriptorSetAllocateInfo dai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    dai.descriptorPool = pool_; dai.descriptorSetCount = 1; dai.pSetLayouts = &matLayout_;
    VK_OK(vkAllocateDescriptorSets(vk_->device, &dai, &m.set));
    VkDescriptorImageInfo ii[3] = {
        {sampler_, (color && color->view) ? color->view : white.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {sampler_, (normal && normal->view) ? normal->view : flatNormal.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {sampler_, (mr && mr->view) ? mr->view : white.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
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
uint8_t Renderer::classer(const masquage::Sphere& s) {
    if (s.r < 0) return VIS_CAM | VIS_REFLET | VIS_OMBRE;
    if (length(s.c - camPos_) - s.r > masquage::distanceMax(s.r, qualiteBasse, finBrouillard_)) { masques_++; return 0; }
    const uint8_t v = (masquage::visible(fCam_, s) ? VIS_CAM : 0) | (refletsOn_ && masquage::visible(fReflet_, s) ? VIS_REFLET : 0) |
                      (shadowsOn_ && masquage::visible(fOmbre_, s) ? VIS_OMBRE : 0);
    if (!v) masques_++;
    return v;
}

Mesh Renderer::makeMesh(const std::vector<Vertex>& v, const std::vector<uint32_t>& idx, bool dynamic) {
    Mesh m; m.dynamic = dynamic; m.indexCount = (uint32_t)idx.size();
    { const masquage::Sphere b = bornes(v); m.centre = b.c; m.rayon = b.r; }   // WFS-82
    if (dynamic) {
        m.vb = vk_->createBuffer(std::max<size_t>(v.size(), 1) * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, true);
        m.ib = vk_->createBuffer(std::max<size_t>(idx.size(), 1) * 4, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, true);
        if (!v.empty()) std::memcpy(m.vb.mapped, v.data(), v.size() * sizeof(Vertex));
        if (!idx.empty()) std::memcpy(m.ib.mapped, idx.data(), idx.size() * 4);
    } else {
        m.vb = vk_->uploadBuffer(v.data(), v.size() * sizeof(Vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
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

void Renderer::destroy(Mesh& m) { vk_->destroy(m.vb); vk_->destroy(m.ib); m.indexCount = 0; }

SkinnedMesh Renderer::makeSkinnedMesh(const std::vector<SkinVertex>& v, const std::vector<uint32_t>& idx) {
    SkinnedMesh m; m.indexCount = (uint32_t)idx.size();
    { const masquage::Sphere b = bornes(v); m.centre = b.c; m.rayon = b.r < 0 ? b.r : b.r * 1.5f; }   // WFS-82 : marge pour l'animation
    m.vb = vk_->uploadBuffer(v.data(), v.size() * sizeof(SkinVertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    m.ib = vk_->uploadBuffer(idx.data(), idx.size() * 4, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    return m;
}

void Renderer::destroy(SkinnedMesh& m) { vk_->destroy(m.vb); vk_->destroy(m.ib); m.indexCount = 0; }

void Renderer::destroy(Model& m) {
    for (auto& mat : m.mats) if (mat.set) vkFreeDescriptorSets(vk_->device, pool_, 1, &mat.set);
    for (auto& t : m.textures) vk_->destroy(t);
    vk_->destroy(m.vb); vk_->destroy(m.ib);
    m = Model{};
}

bool Renderer::beginFrame(const FrameParams& fp) {
    VkCtx& vk = *vk_; VkDevice d = vk.device;
    FrameRes& r = fr_[cur_];
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
    nuitScene_ = fp.night; appelsPrec_ = appels_; appels_ = 0;   // WFS-15
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
    }
    {   // WFS-45 : projecteurs, lumière hémisphère, brouillard linéaire (three.js r128, physicallyCorrectLights non activé)
        const vec3 fwd = normalize(vec3{-fp.view.at(2, 0), -fp.view.at(2, 1), -fp.view.at(2, 2)});
        u.camFwd = {fwd.x, fwd.y, fwd.z, 0};
        u.fogLin = {fp.fogNear, fp.fogFar, 0, 0};
        if (fp.hemiIntensity > 0) {
            const vec3 a = fp.hemiSky * fp.hemiIntensity, b = fp.hemiGround * fp.hemiIntensity;
            u.hemiSky = {a.x, a.y, a.z, 1}; u.hemiGround = {b.x, b.y, b.z, 0};
        }
        const int n = std::clamp(fp.spotCount, 0, 8);
        u.spotN = {(float)n, 0, 0, 0};
        for (int i = 0; i < n; i++) {
            const FrameParams::Spot& s = fp.spots[i];
            vec3 dir = normalize(s.target - s.pos);
            if (length(dir) < 0.5f) dir = {0, -1, 0};
            // r128 sans lumières physiques : irradiance × PI (lights_fragment_begin), compensée par le 1/PI du Lambert
            const vec3 c = s.color * (s.intensity * PI);
            u.spotPos[i] = {s.pos.x, s.pos.y, s.pos.z, std::max(s.distance, 0.f)};
            u.spotDir[i] = {dir.x, dir.y, dir.z, std::max(s.decay, 0.f)};
            u.spotCol[i] = {c.x, c.y, c.z, 0};
            u.spotCone[i] = {std::cos(s.angle), std::cos(s.angle * (1.f - std::clamp(s.penumbra, 0.f, 1.f))), 0, 0};
            if (s.angle >= PI - 1e-3f) u.spotCone[i] = {-2.f, -1.5f, 0, 0};   // WFS-09 : PointLight (toutes directions)
        }
    }
    // WFS-16 : chaussée mouillée, flaques, reflets de la scène (passe miroir autour de y = refletY)
    refletsOn_ = fp.mouille && fp.reflets;
    u.nuitA = {fp.mouille ? 1.f : 0.f, fp.mouille && fp.flaques ? 1.f : 0.f, refletsOn_ ? 1.f : 0.f, 0};
    u.nuitB = {fp.refletY, 1.f / (float)sw_, 1.f / (float)sh_, 0};
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
        finBrouillard_ = masquage::finBrouillard(fp.fogFar, fp.fogDensity);
        masquesPrec_ = masques_; masques_ = 0;
    }
    feuxNuit_ = FeuxVoiture{}; feuxNuit_.nuit = fp.feux; feuxSuivantPose_ = false;   // WFS-09
    quadsUsed_ = 0; view_ = fp.view;
    camPos_ = fp.camPos; exposure_ = fp.exposure;
    transparent_.clear(); opaqueQ_.clear(); ui_vtx_.clear(); skinUsed_ = 0;
    readShadowTiming();

    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_OK(vkResetCommandBuffer(r.cb, 0));
    VK_OK(vkBeginCommandBuffer(r.cb, &bi));
    inFrame_ = true;
    return true;
}

void Renderer::flushDraw(const Pending& p) {
    appels_++;   // WFS-15
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
    vkCmdBindIndexBuffer(cb, p.ib, 0, VK_INDEX_TYPE_UINT32);
    const VkPipelineLayout lay = p.osOfs >= 0 ? skinLayout_ : sceneLayout_;   // WFS-63 : palette d'os en set 2
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, lay, 1, 1, &p.mat->set, 0, nullptr);
    if (p.osOfs >= 0) { const uint32_t ofs = (uint32_t)p.osOfs; vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, skinLayout_, 2, 1, &osSet_[cur_], 1, &ofs); }
    vkCmdPushConstants(cb, lay, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PC), &p.pc);
    vkCmdDrawIndexed(cb, p.count, 1, p.first, vofs, 0);
}

void Renderer::draw(const Model& m, const mat4& world, const vec3* paint, const RouesPose* roues) {
    if (!inFrame_ || !m.ok) return;
    // WFS-09 : feux de cette voiture (le joueur : feuxProchaineVoiture ; les autres : état de nuit de l'image)
    const FeuxVoiture fx = feuxSuivantPose_ ? feuxSuivant_ : feuxNuit_;
    feuxSuivantPose_ = false;
    const uint8_t vis = classer(masquage::monde(world, masquage::deBoite(m.bmin, m.bmax)));   // WFS-82
    if (!vis) return;   // RG-06 : les feux disparaissent avec leur voiture, jamais seuls
    if (fx.nuit) feuxVoiture(m, world, fx);
    mat4 wr[4]; const bool anim = roues && m.nRoues == 4;   // WFS-60 : matrice de chaque roue (braquage, roulage)
    if (anim) for (int k = 0; k < 4; k++) wr[k] = world * roues::matrice(m.roues[k], *roues);
    vec3 c = world.transformPoint((m.bmin + m.bmax) * 0.5f);
    float depth = length(c - camPos_);
    for (size_t pi = 0; pi < m.parts.size(); pi++) {
        const ModelPart& part = m.parts[pi];
        const Material& mat = m.mats[part.material];
        Pending p{m.vb.buf, m.ib.buf, part.firstIndex, part.indexCount, part.vertexOffset, &mat, {}, depth};
        p.vis = vis;   // WFS-82
        p.pc.model = anim && part.roue >= 0 ? wr[part.roue] : world;
        p.pc.color = mat.color;
        if (paint && (mat.flags & MAT_PAINT)) p.pc.color = {paint->x, paint->y, paint->z, 1};
        p.pc.params = {mat.metal, mat.rough, mat.cutoff, (float)mat.flags};
        p.pc.emissive = {mat.emissive.x, mat.emissive.y, mat.emissive.z, mat.detail};   // w : finesse (WFS-42)
        if (std::getenv("WFS_DBG_MAT")) { const int k = (int)part.material; p.pc.emissive = {(float)(k % 4) * 1.f, (float)((k / 4) % 4) * 1.f, (float)((k / 16) % 4) * 1.f, 0}; p.pc.color = {0, 0, 0, 1}; p.pc.params.x = 0; p.pc.params.y = 1; p.pc.params.w = (float)(mat.flags & ~(1u|64u)); }
        // WFS-09 : pièces des feux (repérées par matériau) allumées la nuit : rouge (plus fort au freinage), phares blancs
        if (fx.nuit && (mat.flags & MAT_TAIL)) { const float k = fx.stop ? 4.f : 1.6f; p.pc.emissive.x += k; p.pc.emissive.y += k * 0.02f; p.pc.emissive.z += k * 0.04f; }
        if (fx.nuit && (mat.flags & MAT_HEAD)) { p.pc.emissive.x += 2.4f; p.pc.emissive.y += 2.2f; p.pc.emissive.z += 1.9f; }
        if (mat.filament) {   // WFS-42 : fils plus fins qu'un pixel → mélangés selon leur couverture, avant les vitres
            p.pc.params.w = (float)(mat.flags | MAT_BLEND); p.depth += 0.05f;
            transparent_.push_back(p);
        } else if (mat.flags & MAT_BLEND) transparent_.push_back(p);
        else opaqueQ_.push_back(p);
    }
}

void Renderer::draw(const Mesh& m, const Material& mat, const mat4& world, float roadHalfWidth) {
    if (!inFrame_ || !m.indexCount) return;
    const uint8_t vis = classer(masquage::monde(world, {m.centre, m.rayon}));   // WFS-82
    if (!vis) return;
    Pending p{m.vb.buf, m.ib.buf, 0, m.indexCount, 0, &mat, {}, length(world.transformPoint({0, 0, 0}) - camPos_)};
    p.vis = vis;
    p.pc.model = world; p.pc.color = mat.color;
    p.pc.params = {mat.metal, mat.rough, mat.cutoff, (float)mat.flags};
    p.pc.emissive = {mat.emissive.x, mat.emissive.y, mat.emissive.z, roadHalfWidth};
    // ombre de contact sous les voitures (RG-05) : plus légère quand les ombres du soleil sont actives
    if (mat.flags & MAT_SHADOWBLOB) p.pc.color.w *= (shadowsOn_ ? 0.55f : night_ ? 0.7f : 0.62f) / 0.75f;
    if (mat.flags & MAT_BLEND) transparent_.push_back(p); else opaqueQ_.push_back(p);
}

void Renderer::drawSkinned(const SkinnedMesh& m, const Material& mat, const mat4& world, const mat4* os, int nOs, uint32_t first, uint32_t count,
                           const ApparencePersonnage* ap) {
    if (!inFrame_ || !m.indexCount || !os || nOs <= 0) return;
    const uint8_t vis = classer(masquage::monde(world, {m.centre, m.rayon}));   // WFS-82 : avant de prendre une place
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
    p.pc.model = world; p.pc.color = mat.color;
    p.pc.params = {mat.metal, mat.rough, mat.cutoff, (float)mat.flags};
    p.pc.emissive = {mat.emissive.x, mat.emissive.y, mat.emissive.z, 0};
    p.osOfs = (int32_t)ofs; p.vis = vis;
    opaqueQ_.push_back(p);   // maillages à squelette : opaques (ou alpha découpé) seulement
}

// ---------- WFS-09 : quads transitoires, halos, feux des voitures ----------
void Renderer::drawQuads(const Vertex* v, int nQuads, const Material& mat) {
    if (!inFrame_ || nQuads <= 0 || quadsUsed_ + nQuads > QUADS_MAX) return;
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
constexpr float SOL_LARGEUR = 0.9f, SOL_LONGUEUR = 1.1f, SOL_RECUL = 0.45f;   // reflet sur la route (× largeur ; m ; m derrière)
constexpr float I_SOL = 0.07f, I_SOL_STOP = 0.18f, I_SOL_RECUL = 0.2f;        // tailGlow, tailGlowBrake, revGlow (atténués)
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
    for (int side : {-1, 1}) {
        feu(w.transformPoint({side * F.arriereX * RENTRE, F.arriereY, F.arriere + 0.04f}), arriere, COEUR, LUEUR, rouge,
            f.stop ? I_COEUR_STOP : I_COEUR, f.stop ? I_LUEUR_STOP : I_LUEUR);
        if (f.recul) feu(w.transformPoint({side * std::max(0.15f, F.arriereX - 0.24f), F.arriereY - 0.04f, F.arriere + 0.05f}), arriere,
                         RECUL_COEUR, RECUL_LUEUR, blanc, I_RECUL_COEUR, I_RECUL_LUEUR);
        feu(w.transformPoint({side * F.avantX, F.avantY, F.avant - 0.02f}), avant, PHARE_COEUR, PHARE_LUEUR, phare, I_PHARE_COEUR, I_PHARE_LUEUR);
    }
    // reflet discret des feux sur la route, juste derrière la voiture (RG-05) ; aucun faisceau arrière
    const vec3 r = normalize(w.transformDir({1, 0, 0}));
    const vec3 c = w.transformPoint({0, 0.03f, F.arriere + SOL_RECUL});
    const vec3 col = f.recul ? lin(0xdfe6ff, I_SOL_RECUL) : lin(0xff2040, f.stop ? I_SOL_STOP : I_SOL);
    haloSol(c, r, arriere, F.largeur * SOL_LARGEUR * 0.5f * ech, SOL_LONGUEUR * 0.5f * ech, col);
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
    VkDescriptorImageInfo ii{sampler_, atlas.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet wr{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    wr.dstSet = fontSet_; wr.dstBinding = 0; wr.descriptorCount = 1; wr.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; wr.pImageInfo = &ii;
    vkUpdateDescriptorSets(vk_->device, 1, &wr, 0, nullptr);
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
    VkCtx& vk = *vk_;
    FrameRes& r = fr_[cur_];
    shadowPass(r.cb);
    // objets transparents (vitres, halos) : du plus loin au plus proche, triés une fois pour les deux passes
    std::stable_sort(transparent_.begin(), transparent_.end(), [](const Pending& a, const Pending& b) { return a.depth > b.depth; });
    refletPass(r.cb);   // WFS-16 : reflet de la scène (route mouillée seulement)
    if (tsPool_) vkCmdWriteTimestamp(r.cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, tsPool_, (uint32_t)cur_ * 4 + 2);
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
    vkCmdBindPipeline(r.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, sky_);
    vkCmdDraw(r.cb, 3, 1, 0, 0);
    vkCmdBindPipeline(r.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, opaque_);
    VkPipeline lie = opaque_;
    for (const Pending& p : opaqueQ_) {
        if (!(p.vis & VIS_CAM)) continue;   // WFS-82 : hors champ (gardé pour l'ombre ou le reflet)
        const VkPipeline voulu = p.osOfs >= 0 ? skinOpaque_ : opaque_;   // WFS-63
        if (voulu != lie) { vkCmdBindPipeline(r.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, voulu); lie = voulu; }
        flushDraw(p);
    }
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
    vkCmdEndRenderPass(r.cb);
    if (bloom_) bloom_->dessiner(*this, r.cb);   // WFS-100 : chaîne du bloom, comptée avec la passe de scène
    // WFS-09 : coût mesuré aussi la nuit (sans ombres) : passes de reflet et de scène (+ bloom, WFS-100)
    if (tsPool_) { vkCmdWriteTimestamp(r.cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, tsPool_, (uint32_t)cur_ * 4 + 3); tsWritten_[cur_] = true; }

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
    destroyTargets();
    if (bloom_) { bloom_->liberer(*this); delete bloom_; bloom_ = nullptr; }   // WFS-100
    for (auto& r : fr_) {
        vkDestroyFence(d, r.fence, nullptr); vkDestroySemaphore(d, r.acquired, nullptr); vkDestroySemaphore(d, r.done, nullptr);
        vk_->destroy(r.ubo); vk_->destroy(r.uiVb);
    }
    for (VkPipeline p : {opaque_, blend_, sky_, post_, ui_, shadowPipe_, shadowMaskPipe_, skinOpaque_, skinShadow_, skinShadowMask_,
                         poolPipe_, addPipe_, refOpaque_, refBlend_, refSky_, refAdd_})
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
    vk_->destroy(white); vk_->destroy(flatNormal);
    vk_ = nullptr;
}

} // namespace wfs
