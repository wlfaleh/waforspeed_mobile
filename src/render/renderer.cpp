#include "renderer.h"
#include <algorithm>
#include <cmath>
#include <cstring>

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

struct FrameUBO {
    mat4 viewProj, invViewProj;
    vec4 camPos, sunDir, sunColor, skyTop, skyHorizon, groundCol, fog, misc;
    mat4 shadowMat; vec4 shadowP;
    // WFS-45 : même ordre que shaders/common.glsl
    vec4 camFwd, fogLin, hemiSky, hemiGround, spotN;
    vec4 spotPos[4], spotDir[4], spotCol[4], spotCone[4];
};

// ombres du soleil (WFS-07, valeurs du jeu web : renderer.js applyEnv / stableShadow)
static constexpr float SHADOW_HALF = 40.f;        // demi-largeur de la boîte (m)
static constexpr float SHADOW_DEPTH = 260.f;      // near 1 → far 260 : profondeur couverte (m)
static constexpr float SHADOW_AHEAD = 20.f;       // boîte décalée vers l'avant (m)
static constexpr float SHADOW_BIAS = -0.0003f, SHADOW_NORMAL_BIAS = 0.035f;
static constexpr VkFormat SHADOW_FORMAT = VK_FORMAT_D16_UNORM;

static constexpr VkFormat HDR_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT;
static constexpr size_t UI_MAX_VERTS = 6 * 8192;

bool Renderer::init(VkCtx& vk) {
    vk_ = &vk;
    VkDevice d = vk.device;

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

    // ---------- textures par défaut ----------
    uint8_t w[4] = {255, 255, 255, 255}, nrm[4] = {128, 128, 255, 255};
    white = vk.uploadTexture(w, 1, 1, false);
    flatNormal = vk.uploadTexture(nrm, 1, 1, false);

    // ---------- descripteurs ----------
    VkDescriptorSetLayoutBinding b0[2] = {{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                                          {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};   // carte d'ombre
    VkDescriptorSetLayoutCreateInfo lci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    lci.bindingCount = 2; lci.pBindings = b0;
    VK_OK(vkCreateDescriptorSetLayout(d, &lci, nullptr, &frameLayout_));
    VkDescriptorSetLayoutBinding mb[3];
    for (uint32_t i = 0; i < 3; i++) mb[i] = {i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    lci.bindingCount = 3; lci.pBindings = mb;
    VK_OK(vkCreateDescriptorSetLayout(d, &lci, nullptr, &matLayout_));
    lci.bindingCount = 1; lci.pBindings = mb;
    VK_OK(vkCreateDescriptorSetLayout(d, &lci, nullptr, &postLayout_));

    VkDescriptorPoolSize sizes[2] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 8}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 * 1024 + 16}};
    VkDescriptorPoolCreateInfo pci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pci.maxSets = 1024 + 16; pci.poolSizeCount = 2; pci.pPoolSizes = sizes;
    VK_OK(vkCreateDescriptorPool(d, &pci, nullptr, &pool_));

    VkPushConstantRange pcr{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PC)};
    VkDescriptorSetLayout sl[2] = {frameLayout_, matLayout_};
    VkPipelineLayoutCreateInfo plci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    plci.setLayoutCount = 2; plci.pSetLayouts = sl; plci.pushConstantRangeCount = 1; plci.pPushConstantRanges = &pcr;
    VK_OK(vkCreatePipelineLayout(d, &plci, nullptr, &sceneLayout_));
    VkPushConstantRange ppr{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 32};
    plci.setLayoutCount = 1; plci.pSetLayouts = &postLayout_; plci.pPushConstantRanges = &ppr;
    VK_OK(vkCreatePipelineLayout(d, &plci, nullptr, &postPLayout_));

    createShadows();

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
    VkDescriptorSetAllocateInfo dai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    dai.descriptorPool = pool_; dai.descriptorSetCount = 1; dai.pSetLayouts = &postLayout_;
    VK_OK(vkAllocateDescriptorSets(d, &dai, &postSet_));

    createTargets();
    createPipelines();
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
        q.queryType = VK_QUERY_TYPE_TIMESTAMP; q.queryCount = 3 * FRAMES;
        VK_OK(vkCreateQueryPool(d, &q, nullptr, &tsPool_));
    }
    WFS_LOG("Ombres du soleil : carte %ux%u, filtre %s", shadowSize_, shadowSize_, linear ? "bilinéaire" : "au plus proche");
}

void Renderer::readShadowTiming() {
    if (!tsPool_ || !tsWritten_[cur_]) return;
    uint64_t t[3];
    if (vkGetQueryPoolResults(vk_->device, tsPool_, (uint32_t)cur_ * 3, 3, sizeof t, t, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT) != VK_SUCCESS) return;
    const double k = vk_->props.limits.timestampPeriod * 1e-6;
    shadowMsSum_ += (double)(t[1] - t[0]) * k;
    sceneMsSum_ += (double)(t[2] - t[1]) * k;   // WFS-42 : coût de la passe de scène, pour comparer avant / après
    if (++shadowMsN_ == 120) {
        WFS_LOG("Ombres : passe d'ombre %.2f ms GPU en moyenne (carte %u) ; passe de scène %.2f ms", shadowMsSum_ / shadowMsN_, shadowSize_,
                sceneMsSum_ / shadowMsN_);
        shadowMsSum_ = 0; sceneMsSum_ = 0; shadowMsN_ = 0;
    }
}

// passe d'ombre : objets opaques vus du soleil (sauf sol, route, trottoirs : ils reçoivent l'ombre sans en projeter)
void Renderer::shadowPass(VkCommandBuffer cb) {
    if (tsPool_) { vkCmdResetQueryPool(cb, tsPool_, (uint32_t)cur_ * 3, 3); vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, tsPool_, (uint32_t)cur_ * 3); }
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
            if (p.mat->flags & NO_CAST) continue;
            VkPipeline want = (p.mat->flags & MAT_MASK) ? shadowMaskPipe_ : shadowPipe_;
            if (want != bound) { vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, want); bound = want; }
            flushDraw(p);
        }
    }
    vkCmdEndRenderPass(cb);
    if (tsPool_) vkCmdWriteTimestamp(cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, tsPool_, (uint32_t)cur_ * 3 + 1);
}

void Renderer::rotationMatrix(float r[4]) const {
    switch (vk_->transform) {
        case VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR:  r[0] = 0; r[1] = 1; r[2] = -1; r[3] = 0; break;
        case VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR: r[0] = -1; r[1] = 0; r[2] = 0; r[3] = -1; break;
        case VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR: r[0] = 0; r[1] = -1; r[2] = 1; r[3] = 0; break;
        default: r[0] = 1; r[1] = 0; r[2] = 0; r[3] = 1; break;
    }
}

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
    WFS_LOG("Cibles de rendu : %ux%u (logique), scène %ux%u, MSAA x%d", lw_, lh_, sw_, sh_, (int)vk.msaa);
}

void Renderer::destroyTargets() {
    VkDevice d = vk_->device;
    for (auto fb : finalFbs_) vkDestroyFramebuffer(d, fb, nullptr);
    finalFbs_.clear();
    if (sceneFb_) vkDestroyFramebuffer(d, sceneFb_, nullptr);
    sceneFb_ = VK_NULL_HANDLE;
    vk_->destroy(hdr_); vk_->destroy(msaaColor_); vk_->destroy(depth_);
}

bool Renderer::resize() {
    vk_->waitIdle();
    destroyTargets();
    createTargets();
    return true;
}

VkPipeline Renderer::makePipeline(VkShaderModule vs, VkShaderModule fs, int vertexKind, bool blend, bool premul, bool depthTest, bool depthWrite,
                                  VkRenderPass rp, VkSampleCountFlagBits samples, VkPipelineLayout layout, bool depthOnly) {
    VkPipelineShaderStageCreateInfo st[2]{};
    st[0].sType = st[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    st[0].stage = VK_SHADER_STAGE_VERTEX_BIT; st[0].module = vs; st[0].pName = "main";
    st[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT; st[1].module = fs; st[1].pName = "main";

    VkVertexInputBindingDescription bind{};
    VkVertexInputAttributeDescription attr[3]{};
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    if (vertexKind == 1) {   // scène : position, normale, uv
        bind = {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
        attr[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0};
        attr[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12};
        attr[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, 24};
        vi.vertexBindingDescriptionCount = 1; vi.pVertexBindingDescriptions = &bind;
        vi.vertexAttributeDescriptionCount = 3; vi.pVertexAttributeDescriptions = attr;
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

Mesh Renderer::makeMesh(const std::vector<Vertex>& v, const std::vector<uint32_t>& idx, bool dynamic) {
    Mesh m; m.dynamic = dynamic; m.indexCount = (uint32_t)idx.size();
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
}

void Renderer::destroy(Mesh& m) { vk_->destroy(m.vb); vk_->destroy(m.ib); m.indexCount = 0; }

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
        const int n = std::clamp(fp.spotCount, 0, 4);
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
        }
    }
    std::memcpy(r.ubo.mapped, &u, sizeof(u));
    camPos_ = fp.camPos; exposure_ = fp.exposure;
    transparent_.clear(); opaqueQ_.clear(); ui_vtx_.clear();
    readShadowTiming();

    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_OK(vkResetCommandBuffer(r.cb, 0));
    VK_OK(vkBeginCommandBuffer(r.cb, &bi));
    inFrame_ = true;
    return true;
}

void Renderer::flushDraw(const Pending& p) {
    VkCommandBuffer cb = fr_[cur_].cb;
    VkDeviceSize zero = 0;
    vkCmdBindVertexBuffers(cb, 0, 1, &p.vb, &zero);
    vkCmdBindIndexBuffer(cb, p.ib, 0, VK_INDEX_TYPE_UINT32);
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, sceneLayout_, 1, 1, &p.mat->set, 0, nullptr);
    vkCmdPushConstants(cb, sceneLayout_, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PC), &p.pc);
    vkCmdDrawIndexed(cb, p.count, 1, p.first, p.vofs, 0);
}

void Renderer::draw(const Model& m, const mat4& world, const vec3* paint) {
    if (!inFrame_ || !m.ok) return;
    vec3 c = world.transformPoint((m.bmin + m.bmax) * 0.5f);
    float depth = length(c - camPos_);
    for (size_t pi = 0; pi < m.parts.size(); pi++) {
        const ModelPart& part = m.parts[pi];
        const Material& mat = m.mats[part.material];
        Pending p{m.vb.buf, m.ib.buf, part.firstIndex, part.indexCount, part.vertexOffset, &mat, {}, depth};
        p.pc.model = world;
        p.pc.color = mat.color;
        if (paint && (mat.flags & MAT_PAINT)) p.pc.color = {paint->x, paint->y, paint->z, 1};
        p.pc.params = {mat.metal, mat.rough, mat.cutoff, (float)mat.flags};
        p.pc.emissive = {mat.emissive.x, mat.emissive.y, mat.emissive.z, mat.detail};   // w : finesse (WFS-42)
        if (mat.filament) {   // WFS-42 : fils plus fins qu'un pixel → mélangés selon leur couverture, avant les vitres
            p.pc.params.w = (float)(mat.flags | MAT_BLEND); p.depth += 0.05f;
            transparent_.push_back(p);
        } else if (mat.flags & MAT_BLEND) transparent_.push_back(p);
        else opaqueQ_.push_back(p);
    }
}

void Renderer::draw(const Mesh& m, const Material& mat, const mat4& world, float roadHalfWidth) {
    if (!inFrame_ || !m.indexCount) return;
    Pending p{m.vb.buf, m.ib.buf, 0, m.indexCount, 0, &mat, {}, length(world.transformPoint({0, 0, 0}) - camPos_)};
    p.pc.model = world; p.pc.color = mat.color;
    p.pc.params = {mat.metal, mat.rough, mat.cutoff, (float)mat.flags};
    p.pc.emissive = {mat.emissive.x, mat.emissive.y, mat.emissive.z, roadHalfWidth};
    // ombre de contact sous les voitures (RG-05) : plus légère quand les ombres du soleil sont actives
    if (mat.flags & MAT_SHADOWBLOB) p.pc.color.w *= (shadowsOn_ ? 0.55f : night_ ? 0.7f : 0.62f) / 0.75f;
    if (mat.flags & MAT_BLEND) transparent_.push_back(p); else opaqueQ_.push_back(p);
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
    for (const Pending& p : opaqueQ_) flushDraw(p);
    // objets transparents (vitres) : du plus loin au plus proche
    if (!transparent_.empty()) {
        std::stable_sort(transparent_.begin(), transparent_.end(), [](const Pending& a, const Pending& b) { return a.depth > b.depth; });
        vkCmdBindPipeline(r.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, blend_);
        for (const Pending& p : transparent_) flushDraw(p);
    }
    vkCmdEndRenderPass(r.cb);
    if (tsPool_) { vkCmdWriteTimestamp(r.cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, tsPool_, (uint32_t)cur_ * 3 + 2); tsWritten_[cur_] = shadowsOn_; }

    // passe finale : tonemapping puis interface
    VkRenderPassBeginInfo rbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rbi.renderPass = finalPass_; rbi.framebuffer = finalFbs_[imageIndex_]; rbi.renderArea = {{0, 0}, vk.swapExtent};
    vkCmdBeginRenderPass(r.cb, &rbi, VK_SUBPASS_CONTENTS_INLINE);
    VkViewport vpt{0, 0, (float)vk.swapExtent.width, (float)vk.swapExtent.height, 0, 1};
    VkRect2D sc{{0, 0}, vk.swapExtent};
    vkCmdSetViewport(r.cb, 0, 1, &vpt); vkCmdSetScissor(r.cb, 0, 1, &sc);
    struct { float rot[4]; float params[4]; } pp{};
    rotationMatrix(pp.rot);
    pp.params[0] = exposure_; pp.params[1] = vk.swapSrgb ? 1.f : 0.f;
    vkCmdBindPipeline(r.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, post_);
    vkCmdBindDescriptorSets(r.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, postPLayout_, 0, 1, &postSet_, 0, nullptr);
    vkCmdPushConstants(r.cb, postPLayout_, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 32, &pp);
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
    for (auto& r : fr_) {
        vkDestroyFence(d, r.fence, nullptr); vkDestroySemaphore(d, r.acquired, nullptr); vkDestroySemaphore(d, r.done, nullptr);
        vk_->destroy(r.ubo); vk_->destroy(r.uiVb);
    }
    for (VkPipeline p : {opaque_, blend_, sky_, post_, ui_, shadowPipe_, shadowMaskPipe_}) if (p) vkDestroyPipeline(d, p, nullptr);
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
