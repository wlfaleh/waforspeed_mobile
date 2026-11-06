// WaForSpeed — passe d'essai du socle (WFS-150, critère d'acceptation) : enregistrée HORS de renderer.cpp par WFS_PASSE,
// comme le feront les passes de WFS-151, 153 et 154. Étape Ecran, demande la pré-passe (RG-02), lit ses normales et sa
// profondeur et écrit une image de contrôle RGBA8 de la même taille (moitié gauche normales, moitié droite distance),
// jamais affichée : l'image du jeu ne change pas (RG-06). Active seulement sur demande du banc (passes::activerEssai).
#include <algorithm>
#include "../core/log.h"
#include "passes.h"

namespace wfs {
namespace {

static const uint32_t SPV_PLEIN_ECRAN_VERT[] = {
#include "plein_ecran.vert.inc"
};
static const uint32_t SPV_PASSE_ESSAI_FRAG[] = {
#include "passe_essai.frag.inc"
};

constexpr int IMAGES = AccesRendu::images();   // = Renderer::FRAMES
constexpr VkFormat FORMAT = VK_FORMAT_R8G8B8A8_UNORM;

class PasseEssai : public PasseRendu {
public:
    const char* nom() const override { return "passe d'essai (lit la pré-passe)"; }
    EtapeRendu etape() const override { return EtapeRendu::Ecran; }
    const char* cibles() const override { return "lit pré-passe (normales, profondeur) ; écrit une image de contrôle RGBA8 non affichée"; }
    bool demandePrePasse() const override { return true; }
    bool active(const ImagePasse&) const override { return passes::essaiActif(); }
    void redimensionner(Renderer& r) override { liberer(r); }
    void detruire(Renderer& r) override {
        liberer(r);
        VkDevice d = AccesRendu::vk(r).device;
        if (pipe_) vkDestroyPipeline(d, pipe_, nullptr);
        if (lay_) vkDestroyPipelineLayout(d, lay_, nullptr);
        if (dsl_) vkDestroyDescriptorSetLayout(d, dsl_, nullptr);
        if (pool_) vkDestroyDescriptorPool(d, pool_, nullptr);
        if (rp_) vkDestroyRenderPass(d, rp_, nullptr);
        for (VkShaderModule m : {vs_, fs_}) if (m) vkDestroyShaderModule(d, m, nullptr);
        pipe_ = VK_NULL_HANDLE; lay_ = VK_NULL_HANDLE; dsl_ = VK_NULL_HANDLE; pool_ = VK_NULL_HANDLE; rp_ = VK_NULL_HANDLE;
        vs_ = fs_ = VK_NULL_HANDLE; set_[0] = set_[1] = VK_NULL_HANDLE;
        g_moi = nullptr;
    }
    void executer(const ImagePasse& img) override {
        if (!passes::prePasseFaite()) return;   // pré-passe absente (aucune cible) : rien à lire
        Renderer& r = img.r;
        if (!rp_) fixes(r);
        const uint32_t w = passes::prePasseLargeur(), h = passes::prePasseHauteur();
        if (!cible_.img || cible_.w != w || cible_.h != h) { liberer(r); creerCible(r, w, h); }
        // descripteurs de cette image en vol (libres : sa barrière a été attendue) ; vues de la pré-passe relues à chaque image
        VkDescriptorImageInfo ii[2] = {{passes::echantillonneurPrePasse(), passes::prePasseNormales(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
                                       {passes::echantillonneurPrePasse(), passes::prePasseProfondeur(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL}};
        VkWriteDescriptorSet wr[2]{};
        for (uint32_t b = 0; b < 2; b++) {
            wr[b].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; wr[b].dstSet = set_[img.image]; wr[b].dstBinding = b;
            wr[b].descriptorCount = 1; wr[b].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; wr[b].pImageInfo = &ii[b];
        }
        vkUpdateDescriptorSets(AccesRendu::vk(r).device, 2, wr, 0, nullptr);
        VkRenderPassBeginInfo bi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        bi.renderPass = rp_; bi.framebuffer = fb_; bi.renderArea = {{0, 0}, {w, h}};
        vkCmdBeginRenderPass(img.cb, &bi, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport vp{0, 0, (float)w, (float)h, 0, 1}; VkRect2D sc{{0, 0}, {w, h}};
        vkCmdSetViewport(img.cb, 0, 1, &vp); vkCmdSetScissor(img.cb, 0, 1, &sc);
        vkCmdBindPipeline(img.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe_);
        vkCmdBindDescriptorSets(img.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, lay_, 0, 1, &set_[img.image], 0, nullptr);
        // profondeur inversée à l'infini : ligne 2 de vue-projection = (0, 0, 0, proche) → plan proche de la caméra
        const float pc[4] = {img.ubo.viewProj.at(2, 3), 0, 0, 0};
        vkCmdPushConstants(img.cb, lay_, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof pc, pc);
        vkCmdDraw(img.cb, 3, 1, 0, 0);
        vkCmdEndRenderPass(img.cb);
        g_moi = this;
    }
    bool lire(Renderer& r, std::vector<uint8_t>& rgba, uint32_t& w, uint32_t& h) {
        w = cible_.w; h = cible_.h;
        return cible_.img && passes::lireCouche(AccesRendu::vk(r), cible_.img, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, w, h, 4, rgba);
    }
    static PasseEssai* g_moi;
private:
    void fixes(Renderer& r) {
        VkCtx& vk = AccesRendu::vk(r); VkDevice d = vk.device;
        VkAttachmentDescription at{};
        at.format = FORMAT; at.samples = VK_SAMPLE_COUNT_1_BIT;
        at.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; at.storeOp = VK_ATTACHMENT_STORE_OP_STORE;   // triangle plein écran : tout est écrit
        at.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; at.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        at.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; at.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkAttachmentReference cr{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkSubpassDescription sp{}; sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; sp.colorAttachmentCount = 1; sp.pColorAttachments = &cr;
        VkSubpassDependency dep[2]{};
        dep[0].srcSubpass = VK_SUBPASS_EXTERNAL; dep[0].dstSubpass = 0;
        dep[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dep[0].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;   // pré-passe écrite
        dep[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dep[1].srcSubpass = 0; dep[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dep[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dep[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
        dep[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;   // pré-passe réécrite ensuite
        VkRenderPassCreateInfo ci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        ci.attachmentCount = 1; ci.pAttachments = &at; ci.subpassCount = 1; ci.pSubpasses = &sp; ci.dependencyCount = 2; ci.pDependencies = dep;
        VK_OK(vkCreateRenderPass(d, &ci, nullptr, &rp_));
        VkDescriptorSetLayoutBinding b[2] = {{0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
                                             {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};
        VkDescriptorSetLayoutCreateInfo li{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO}; li.bindingCount = 2; li.pBindings = b;
        VK_OK(vkCreateDescriptorSetLayout(d, &li, nullptr, &dsl_));
        VkPushConstantRange pcr{VK_SHADER_STAGE_FRAGMENT_BIT, 0, 16};
        VkPipelineLayoutCreateInfo pli{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        pli.setLayoutCount = 1; pli.pSetLayouts = &dsl_; pli.pushConstantRangeCount = 1; pli.pPushConstantRanges = &pcr;
        VK_OK(vkCreatePipelineLayout(d, &pli, nullptr, &lay_));
        VkDescriptorPoolSize ps{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2 * IMAGES};
        VkDescriptorPoolCreateInfo pi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO}; pi.maxSets = IMAGES; pi.poolSizeCount = 1; pi.pPoolSizes = &ps;
        VK_OK(vkCreateDescriptorPool(d, &pi, nullptr, &pool_));
        for (int f = 0; f < IMAGES; f++) {
            VkDescriptorSetAllocateInfo a{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO}; a.descriptorPool = pool_; a.descriptorSetCount = 1; a.pSetLayouts = &dsl_;
            VK_OK(vkAllocateDescriptorSets(d, &a, &set_[f]));
        }
        VkShaderModuleCreateInfo mi{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        mi.codeSize = sizeof(SPV_PLEIN_ECRAN_VERT); mi.pCode = SPV_PLEIN_ECRAN_VERT; VK_OK(vkCreateShaderModule(d, &mi, nullptr, &vs_));
        mi.codeSize = sizeof(SPV_PASSE_ESSAI_FRAG); mi.pCode = SPV_PASSE_ESSAI_FRAG; VK_OK(vkCreateShaderModule(d, &mi, nullptr, &fs_));
        pipe_ = AccesRendu::faire(r, vs_, fs_, 0, false, false, false, rp_, lay_);
    }
    void creerCible(Renderer& r, uint32_t w, uint32_t h) {
        VkCtx& vk = AccesRendu::vk(r);
        cible_ = vk.createImage(w, h, 1, FORMAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        VkFramebufferCreateInfo fi{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fi.renderPass = rp_; fi.attachmentCount = 1; fi.pAttachments = &cible_.view; fi.width = w; fi.height = h; fi.layers = 1;
        VK_OK(vkCreateFramebuffer(vk.device, &fi, nullptr, &fb_));
    }
    void liberer(Renderer& r) {
        VkCtx& vk = AccesRendu::vk(r);
        if (fb_) vkDestroyFramebuffer(vk.device, fb_, nullptr);
        fb_ = VK_NULL_HANDLE; vk.destroy(cible_);
    }
    VkRenderPass rp_ = VK_NULL_HANDLE; VkFramebuffer fb_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout dsl_ = VK_NULL_HANDLE; VkPipelineLayout lay_ = VK_NULL_HANDLE; VkPipeline pipe_ = VK_NULL_HANDLE;
    VkDescriptorPool pool_ = VK_NULL_HANDLE; VkDescriptorSet set_[IMAGES]{};
    VkShaderModule vs_ = VK_NULL_HANDLE, fs_ = VK_NULL_HANDLE;
    Image cible_;
};
PasseEssai* PasseEssai::g_moi = nullptr;
WFS_PASSE(PasseEssai, 900)

} // namespace

bool passes::lireEssai(Renderer& r, std::vector<uint8_t>& rgba, uint32_t& w, uint32_t& h) {
    return PasseEssai::g_moi && PasseEssai::g_moi->lire(r, rgba, w, h);
}

} // namespace wfs
