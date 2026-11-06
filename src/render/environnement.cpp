// WaForSpeed — capture d'environnement (WFS-150, RG-03) : voir environnement.h.
#include "environnement.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "../core/log.h"
#include "../ui/ecrans/reglages.h"
#include "passes.h"
#include "qualite_auto.h"

namespace wfs {
namespace environnement {
namespace {

constexpr VkFormat HDR = VK_FORMAT_R16G16B16A16_SFLOAT;   // = HDR_FORMAT du Renderer (passe de reflet compatible)
constexpr int IMAGES = AccesRendu::images();               // = Renderer::FRAMES

struct Etat {
    // demande
    vec3 point{}; int tailleVoulue = 0; int face = -1; bool prete = false; int captures = 0;
    // suivi automatique de l'heure (signature de la lumière de l'image)
    bool suivi = false, invalide = false; vec3 suiviPoint{}; float signature[16]{}; bool signatureLue = false;
    // ressources (créées à la première capture)
    int cote = 0;
    VkImage img = VK_NULL_HANDLE; VkDeviceMemory mem = VK_NULL_HANDLE;
    VkImageView cube = VK_NULL_HANDLE, faces[6]{};
    Image profondeur;
    VkFramebuffer fb[6]{};
    VkSampler smp = VK_NULL_HANDLE;
    VkDescriptorPool pool = VK_NULL_HANDLE; VkDescriptorSet set[IMAGES]{};
    Buffer ubo[IMAGES];
    VkPipeline squelette = VK_NULL_HANDLE, tache = VK_NULL_HANDLE;
    Renderer* r = nullptr;
};
Etat g;

void libererCube(VkCtx& vk) {
    VkDevice d = vk.device;
    for (VkFramebuffer& f : g.fb) { if (f) vkDestroyFramebuffer(d, f, nullptr); f = VK_NULL_HANDLE; }
    for (VkImageView& v : g.faces) { if (v) vkDestroyImageView(d, v, nullptr); v = VK_NULL_HANDLE; }
    if (g.cube) vkDestroyImageView(d, g.cube, nullptr);
    if (g.img) vkDestroyImage(d, g.img, nullptr);
    if (g.mem) vkFreeMemory(d, g.mem, nullptr);
    g.cube = VK_NULL_HANDLE; g.img = VK_NULL_HANDLE; g.mem = VK_NULL_HANDLE;
    vk.destroy(g.profondeur);
    g.cote = 0;
}

// image cube (6 couches), vues, profondeur et framebuffers d'une taille donnée ; GPU à l'arrêt
void creerCube(Renderer& r, int cote) {
    VkCtx& vk = AccesRendu::vk(r); VkDevice d = vk.device;
    vk.waitIdle();
    libererCube(vk);
    VkImageCreateInfo ii{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ii.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; ii.imageType = VK_IMAGE_TYPE_2D; ii.format = HDR;
    ii.extent = {(uint32_t)cote, (uint32_t)cote, 1}; ii.mipLevels = 1; ii.arrayLayers = 6; ii.samples = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling = VK_IMAGE_TILING_OPTIMAL;
    ii.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VK_OK(vkCreateImage(d, &ii, nullptr, &g.img));
    VkMemoryRequirements mr; vkGetImageMemoryRequirements(d, g.img, &mr);
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    ai.allocationSize = mr.size; ai.memoryTypeIndex = vk.memoryType(mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_OK(vkAllocateMemory(d, &ai, nullptr, &g.mem));
    VK_OK(vkBindImageMemory(d, g.img, g.mem, 0));
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vi.image = g.img; vi.viewType = VK_IMAGE_VIEW_TYPE_CUBE; vi.format = HDR; vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6};
    VK_OK(vkCreateImageView(d, &vi, nullptr, &g.cube));
    g.profondeur = vk.createImage(cote, cote, 1, vk.depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, true);
    for (uint32_t f = 0; f < 6; f++) {
        vi.viewType = VK_IMAGE_VIEW_TYPE_2D; vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, f, 1};
        VK_OK(vkCreateImageView(d, &vi, nullptr, &g.faces[f]));
        VkImageView v[2] = {g.faces[f], g.profondeur.view};
        VkFramebufferCreateInfo fi{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fi.renderPass = AccesRendu::passeReflet(r); fi.attachmentCount = 2; fi.pAttachments = v;
        fi.width = fi.height = (uint32_t)cote; fi.layers = 1;
        VK_OK(vkCreateFramebuffer(d, &fi, nullptr, &g.fb[f]));
    }
    // lisible dès la création (cube noir) : les 6 couches en lecture seule
    VkCommandBuffer cb = vk.beginOneShot();
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    b.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; b.image = g.img;
    b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6}; b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
    VkClearColorValue noir{}; noir.float32[3] = 1;
    vkCmdClearColorImage(cb, g.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &noir, 1, &b.subresourceRange);
    b.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
    vk.endOneShot(cb);
    g.cote = cote;
    WFS_LOG("environnement (WFS-150) : cubemap HDR %dx%d × 6 (%.2f Mo)", cote, cote, cote * (double)cote * 6 * 8 / 1048576.0);
}

// ressources indépendantes de la taille : échantillonneur, tampons de l'image, descripteurs, pipelines en plus de ceux
// de la passe de reflet (personnages animés, tache des phares)
void creerFixes(Renderer& r) {
    if (g.pool) return;
    VkCtx& vk = AccesRendu::vk(r); VkDevice d = vk.device;
    VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    si.magFilter = si.minFilter = VK_FILTER_LINEAR; si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    si.addressModeU = si.addressModeV = si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    VK_OK(vkCreateSampler(d, &si, nullptr, &g.smp));
    VkDescriptorPoolSize tailles[2] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, IMAGES}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5 * IMAGES}}   /* WFS-151 : + sondes IBL et table BRDF */;
    VkDescriptorPoolCreateInfo pi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pi.maxSets = IMAGES; pi.poolSizeCount = 2; pi.pPoolSizes = tailles;
    VK_OK(vkCreateDescriptorPool(d, &pi, nullptr, &g.pool));
    const VkDescriptorSetLayout lay = AccesRendu::dispositionImage(r);
    for (int f = 0; f < IMAGES; f++) {
        VkDescriptorSetAllocateInfo a{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        a.descriptorPool = g.pool; a.descriptorSetCount = 1; a.pSetLayouts = &lay;
        VK_OK(vkAllocateDescriptorSets(d, &a, &g.set[f]));
        g.ubo[f] = vk.createBuffer(sizeof(FrameUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, true);
    }
    const VkRenderPass rp = AccesRendu::passeReflet(r);
    g.squelette = AccesRendu::faire(r, AccesRendu::module(r, 4), AccesRendu::module(r, 5), 3, false, true, true, rp, AccesRendu::dispositionSquelette(r));
    g.tache = AccesRendu::faire(r, AccesRendu::module(r, 0), AccesRendu::module(r, 1), 1, true, true, false, rp, AccesRendu::dispositionScene(r), 1);
}

void detruireTout(Renderer& r) {
    VkCtx& vk = AccesRendu::vk(r); VkDevice d = vk.device;
    libererCube(vk);
    for (VkPipeline* p : {&g.squelette, &g.tache}) { if (*p) vkDestroyPipeline(d, *p, nullptr); *p = VK_NULL_HANDLE; }
    for (Buffer& b : g.ubo) vk.destroy(b);
    if (g.pool) vkDestroyDescriptorPool(d, g.pool, nullptr);
    if (g.smp) vkDestroySampler(d, g.smp, nullptr);
    g.pool = VK_NULL_HANDLE; g.smp = VK_NULL_HANDLE; g.set[0] = g.set[1] = VK_NULL_HANDLE;
    g.face = -1; g.prete = false; g.r = nullptr;
}

void signature(const FrameUBO& u, float s[16]) {
    const vec4* v[4] = {&u.sunDir, &u.sunColor, &u.skyTop, &u.fog};
    for (int i = 0; i < 4; i++) { s[4 * i] = v[i]->x; s[4 * i + 1] = v[i]->y; s[4 * i + 2] = v[i]->z; s[4 * i + 3] = v[i]->w; }
}

// passe d'écran (avant la scène) : une face de la capture demandée par image
class CaptureEnvironnement : public PasseRendu {
public:
    const char* nom() const override { return "capture d'environnement (1 face)"; }
    EtapeRendu etape() const override { return EtapeRendu::Ecran; }
    const char* cibles() const override { return "écrit une face de la cubemap HDR (256 / 128 / 64 px)"; }
    bool toutVoir() const override { return g.face >= 0; }
    bool active(const ImagePasse& img) const override {
        if (g.suivi) {   // heure changée (soleil, ciel, brouillard) ou carte changée : nouvelle capture, une à la fois
            float s[16]; signature(img.ubo, s);
            bool change = !g.signatureLue || g.invalide;
            for (int i = 0; i < 16 && !change; i++) change = std::fabs(s[i] - g.signature[i]) > 1e-4f;
            if (change && g.face < 0) {
                std::memcpy(g.signature, s, sizeof s); g.signatureLue = true; g.invalide = false;
                demander(g.suiviPoint);
            }
        }
        return g.face >= 0;
    }
    void creer(Renderer& r) override { g.r = &r; }
    void detruire(Renderer& r) override { detruireTout(r); }
    void executer(const ImagePasse& img) override {
        Renderer& r = img.r;
        if (!g.pool) creerFixes(r);
        if (g.cote != g.tailleVoulue) creerCube(r, g.tailleVoulue);   // demande faite avant Renderer::init : cube créé ici (GPU attendu ; cette image ne lit pas le cube)
        const int f = g.face, k = img.image;
        // données de l'image avec la caméra de la face : soleil, ombre, ciel, brouillard et lumières inchangés
        FrameUBO u = img.ubo;
        vec3 avant, haut; axesFace(f, avant, haut);
        u.viewProj = vueProjFace(f, g.point); u.invViewProj = inverse(u.viewProj);
        u.camPos = {g.point.x, g.point.y, g.point.z, img.ubo.camPos.w};
        u.camFwd = {avant.x, avant.y, avant.z, 0};
        // image non retournée (convention des cubemaps) : faces inversées comme la passe miroir (nuitA.w), rien découpé
        // sous un plan (nuitB.x très bas), reflet de la chaussée non lu (il est celui de la caméra du joueur)
        u.nuitA.z = 0; u.nuitA.w = 1; u.nuitB.x = -1e30f;
        std::memcpy(g.ubo[k].mapped, &u, sizeof u);
        {   // set 0 de cette image en vol (libre : sa barrière a été attendue) : notre UBO + carte d'ombre et reflet de l'image
            VkDescriptorBufferInfo bi{g.ubo[k].buf, 0, sizeof(FrameUBO)};
            VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            w.dstSet = g.set[k]; w.dstBinding = 0; w.descriptorCount = 1; w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w.pBufferInfo = &bi;
            VkCopyDescriptorSet c[5]{};   // WFS-151 : liaisons 1 à 5 (ombre, reflet, sondes IBL A et B, table BRDF)
            for (uint32_t b = 1; b <= 5; b++) {
                c[b - 1].sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET; c[b - 1].srcSet = AccesRendu::setImage(r, k); c[b - 1].srcBinding = b;
                c[b - 1].dstSet = g.set[k]; c[b - 1].dstBinding = b; c[b - 1].descriptorCount = 1;
            }
            vkUpdateDescriptorSets(AccesRendu::vk(r).device, 1, &w, 5, c);
        }
        VkClearValue cl[2]{}; cl[0].color = {{0, 0, 0, 1}}; cl[1].depthStencil = {0.f, 0};
        VkRenderPassBeginInfo bi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        bi.renderPass = AccesRendu::passeReflet(r); bi.framebuffer = g.fb[f];
        bi.renderArea = {{0, 0}, {(uint32_t)g.cote, (uint32_t)g.cote}}; bi.clearValueCount = 2; bi.pClearValues = cl;
        VkCommandBuffer cb = img.cb;
        vkCmdBeginRenderPass(cb, &bi, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport vp{0, 0, (float)g.cote, (float)g.cote, 0, 1}; VkRect2D sc{{0, 0}, {(uint32_t)g.cote, (uint32_t)g.cote}};
        vkCmdSetViewport(cb, 0, 1, &vp); vkCmdSetScissor(cb, 0, 1, &sc);
        vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, AccesRendu::dispositionScene(r), 0, 1, &g.set[k], 0, nullptr);
        vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, AccesRendu::pipeline(r, 0));
        vkCmdDraw(cb, 3, 1, 0, 0);
        // mêmes objets et mêmes pipelines que la passe de scène (1 échantillon) : opaques dans l'ordre d'appel, puis transparents
        VkPipeline lie = VK_NULL_HANDLE;
        for (const auto& p : AccesRendu::opaques(r)) {
            const VkPipeline v = AccesRendu::squelette(p) ? g.squelette : AccesRendu::pipeline(r, 1);
            if (v != lie) { vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, v); lie = v; }
            AccesRendu::dessiner(r, p);
        }
        for (const auto& p : AccesRendu::transparents(r)) {
            const uint32_t fl = AccesRendu::drapeaux(p);
            const VkPipeline v = (fl & MAT_POOL) ? g.tache : (fl & MAT_ADD) ? AccesRendu::pipeline(r, 3) : AccesRendu::pipeline(r, 2);
            if (v != lie) { vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, v); lie = v; }
            AccesRendu::dessiner(r, p);
        }
        vkCmdEndRenderPass(cb);
        if (++g.face >= 6) {
            g.face = -1; g.prete = true; g.captures++;
            WFS_LOG("environnement (WFS-150) : capture %d prête (%d px, point %.1f %.1f %.1f)", g.captures, g.cote, g.point.x, g.point.y, g.point.z);
        }
    }
};
WFS_PASSE(CaptureEnvironnement, 0)

} // namespace

void demander(const vec3& p, int t) {
    if (t <= 0) t = qualite::tailleCubemap(qualite::niveauAffiche(ecrans::reglages().qualite));
    t = std::clamp(t, 16, 1024);
    g.point = p; g.tailleVoulue = t; g.face = 0; g.prete = false;
    if (g.r && g.cote != t) creerCube(*g.r, t);   // entre deux images (appel du thread de jeu), GPU attendu
}
bool enCours() { return g.face >= 0; }
bool prete() { return g.prete; }
int faceSuivante() { return g.face; }
int taille() { return g.cote; }
vec3 point() { return g.point; }
void suivre(bool actif, const vec3& p) { g.suivi = actif; g.suiviPoint = p; if (!actif) g.signatureLue = false; }
void invalider() { g.invalide = true; }
int capturesFaites() { return g.captures; }
VkImageView vueCube() { return g.cube; }
VkSampler echantillonneur() { return g.smp; }
VkImage image() { return g.img; }   // WFS-151 (accroche E13) : relecture asynchrone des sondes IBL

bool lireFaces(Renderer& r, std::vector<float> faces[6], int& cote) {
    cote = g.cote;
    if (!g.img || !cote) return false;
    for (uint32_t f = 0; f < 6; f++) {
        std::vector<uint8_t> o;
        if (!passes::lireCouche(AccesRendu::vk(r), g.img, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, f, (uint32_t)cote, (uint32_t)cote, 8, o)) return false;
        faces[f].resize((size_t)cote * cote * 4);
        const uint16_t* s = reinterpret_cast<const uint16_t*>(o.data());
        for (size_t i = 0; i < faces[f].size(); i++) faces[f][i] = passes::demiVersFloat(s[i]);
    }
    return true;
}

} // namespace environnement
} // namespace wfs
