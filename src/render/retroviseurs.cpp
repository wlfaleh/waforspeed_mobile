// WaForSpeed — WFS-324 : état de l'image, images des reflets et passe « rétroviseurs » (voir retroviseurs.h).
#include "retroviseurs.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "../core/log.h"
#include "passes.h"

namespace wfs::retro {
namespace {

constexpr VkFormat HDR = VK_FORMAT_R16G16B16A16_SFLOAT;   // = HDR_FORMAT du Renderer (passe de reflet compatible)
constexpr int IMAGES = AccesRendu::images();
// taille des images de reflet (texels) : proportions des glaces (intérieur 236 × 56 mm ≈ 4,2 : 1 ; extérieurs ≈ 1,7 : 1). Vue
// Cockpit sur 2400 × 1080 : la glace intérieure fait ≈ 330 × 80 pixels, une extérieure ≈ 190 × 110 (gauche) : un texel par pixel
// environ ; la droite, plus loin, est plus petite
constexpr uint32_t TAILLE[NB_GLACES][2] = {{352, 88}, {192, 112}, {192, 112}};
// personnes (maillages à squelette) redessinées dans la glace jusqu'à cette distance (RG-03 : pas de piétons au loin)
constexpr float PERSONNES_MAX_M = 25.f;
// habitacle de la voiture du joueur vu dans la glace intérieure : part de la lumière qu'il reçoit (voir executer)
constexpr float OCCLUSION_HABITACLE = 0.25f;
// ordre de mise à jour, une glace par image (RG-03) : l'intérieur une image sur deux, chaque extérieur une sur quatre
constexpr int ORDRE[4] = {INTERIEUR, GAUCHE, INTERIEUR, DROITE};

struct Etat {
    // image en cours
    int glace = -1; mat4 vp{}; vec3 oeilReflet{}, regard{0, 0, 1};
    uint64_t compteur = 0, images = 0; int force = -1; bool desactive = false;
    uint64_t rendues[NB_GLACES] = {};
    Comptes comptes[NB_GLACES]{};
    mat4 vpCamera{}; Fenetre fen[NB_GLACES]{};   // banc : caméra et glaces de la dernière image préparée
    // ressources (créées à la première demande)
    Renderer* r = nullptr;
    Image couleur[NB_GLACES], profondeur;
    VkFramebuffer fb[NB_GLACES] = {};
    VkDescriptorPool pool = VK_NULL_HANDLE; VkDescriptorSet set[IMAGES]{};
    Buffer ubo[IMAGES];
    VkPipeline squelette = VK_NULL_HANDLE, tache = VK_NULL_HANDLE;
    bool pret = false;
};
Etat g;

void creer(Renderer& r) {
    if (g.pret) return;
    VkCtx& vk = AccesRendu::vk(r); VkDevice d = vk.device;
    vk.waitIdle();
    uint32_t wMax = 1, hMax = 1;
    for (const auto& t : TAILLE) { wMax = std::max(wMax, t[0]); hMax = std::max(hMax, t[1]); }
    g.profondeur = vk.createImage(wMax, hMax, 1, vk.depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_DEPTH_BIT, true);
    VkCommandBuffer cb = vk.beginOneShot();
    for (int k = 0; k < NB_GLACES; k++) {
        g.couleur[k] = vk.createImage(TAILLE[k][0], TAILLE[k][1], 1, HDR, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        // lisible dès la création : gris sombre (≈ ciel couvert vu dans une glace) jusqu'à la première image du reflet
        VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        b.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; b.image = g.couleur[k].img;
        b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}; b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
        VkClearColorValue gris{}; gris.float32[0] = gris.float32[1] = gris.float32[2] = 0.02f; gris.float32[3] = 1;
        vkCmdClearColorImage(cb, g.couleur[k].img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &gris, 1, &b.subresourceRange);
        b.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
    }
    vk.endOneShot(cb);
    for (int k = 0; k < NB_GLACES; k++) {
        VkImageView v[2] = {g.couleur[k].view, g.profondeur.view};
        VkFramebufferCreateInfo fi{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fi.renderPass = AccesRendu::passeReflet(r); fi.attachmentCount = 2; fi.pAttachments = v;
        fi.width = TAILLE[k][0]; fi.height = TAILLE[k][1]; fi.layers = 1;
        VK_OK(vkCreateFramebuffer(d, &fi, nullptr, &g.fb[k]));
    }
    VkDescriptorPoolSize tailles[2] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, IMAGES}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5 * IMAGES}};
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
    g.pret = true;
    double mo = 0;
    for (const auto& t : TAILLE) mo += t[0] * (double)t[1] * 8;
    WFS_LOG("rétroviseurs (WFS-324) : 3 images de reflet HDR (%ux%u, %ux%u, %ux%u), %.2f Mo", TAILLE[0][0], TAILLE[0][1], TAILLE[1][0], TAILLE[1][1],
            TAILLE[2][0], TAILLE[2][1], mo / 1048576.0);
}

void detruireTout(Renderer& r) {
    if (!g.pret) return;
    VkCtx& vk = AccesRendu::vk(r); VkDevice d = vk.device;
    for (VkFramebuffer& f : g.fb) { if (f) vkDestroyFramebuffer(d, f, nullptr); f = VK_NULL_HANDLE; }
    for (Image& i : g.couleur) vk.destroy(i);
    vk.destroy(g.profondeur);
    for (VkPipeline* p : {&g.squelette, &g.tache}) { if (*p) vkDestroyPipeline(d, *p, nullptr); *p = VK_NULL_HANDLE; }
    for (Buffer& b : g.ubo) vk.destroy(b);
    if (g.pool) vkDestroyDescriptorPool(d, g.pool, nullptr);
    g.pool = VK_NULL_HANDLE;
    for (VkDescriptorSet& s : g.set) s = VK_NULL_HANDLE;
    g.pret = false; g.glace = -1;
}

// passe d'écran (avant la scène) : le reflet de la glace de l'image
class PasseRetroviseurs : public PasseRendu {
public:
    const char* nom() const override { return "rétroviseurs (1 glace)"; }
    EtapeRendu etape() const override { return EtapeRendu::Ecran; }
    const char* cibles() const override { return "écrit le reflet d'une glace (HDR, 352 × 88 ou 192 × 112 texels)"; }
    bool active(const ImagePasse&) const override { return g.glace >= 0 && g.pret; }
    void creer(Renderer& r) override { g.r = &r; }
    void detruire(Renderer& r) override { detruireTout(r); }
    void executer(const ImagePasse& img) override {
        Renderer& r = img.r;
        const int k = g.glace, f = img.image;
        // données de l'image avec la caméra du reflet : soleil, ciel, brouillard et lumières inchangés
        FrameUBO u = img.ubo;
        u.viewProj = g.vp; u.invViewProj = inverse(g.vp);
        u.camPos = {g.oeilReflet.x, g.oeilReflet.y, g.oeilReflet.z, img.ubo.camPos.w};
        u.camFwd = {g.regard.x, g.regard.y, g.regard.z, 0};
        // vue miroir (faces inversées, comme la passe de reflet de la chaussée : nuitA.w), rien découpé sous un plan (nuitB.x très
        // bas), reflet de la chaussée non lu (c'est celui de la caméra du joueur) ; sans ombres (RG-03 : les cascades sont celles du
        // regard vers l'avant)
        u.nuitA.z = 0; u.nuitA.w = 1; u.nuitB.x = -1e30f;
        u.shadowP.x = 0;
        std::memcpy(g.ubo[f].mapped, &u, sizeof u);
        {
            VkDescriptorBufferInfo bi{g.ubo[f].buf, 0, sizeof(FrameUBO)};
            VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            w.dstSet = g.set[f]; w.dstBinding = 0; w.descriptorCount = 1; w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; w.pBufferInfo = &bi;
            VkCopyDescriptorSet c[5]{};   // liaisons 1 à 5 de l'image (ombre, reflet, sondes IBL A et B, table BRDF)
            for (uint32_t b = 1; b <= 5; b++) {
                c[b - 1].sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET; c[b - 1].srcSet = AccesRendu::setImage(r, f); c[b - 1].srcBinding = b;
                c[b - 1].dstSet = g.set[f]; c[b - 1].dstBinding = b; c[b - 1].descriptorCount = 1;
            }
            vkUpdateDescriptorSets(AccesRendu::vk(r).device, 1, &w, 5, c);
        }
        const uint32_t W = TAILLE[k][0], H = TAILLE[k][1];
        VkClearValue cl[2]{}; cl[0].color = {{0, 0, 0, 1}}; cl[1].depthStencil = {0.f, 0};
        VkRenderPassBeginInfo bi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        bi.renderPass = AccesRendu::passeReflet(r); bi.framebuffer = g.fb[k];
        bi.renderArea = {{0, 0}, {W, H}}; bi.clearValueCount = 2; bi.pClearValues = cl;
        VkCommandBuffer cb = img.cb;
        vkCmdBeginRenderPass(cb, &bi, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport vp{0, 0, (float)W, (float)H, 0, 1}; VkRect2D sc{{0, 0}, {W, H}};
        vkCmdSetViewport(cb, 0, 1, &vp); vkCmdSetScissor(cb, 0, 1, &sc);
        vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, AccesRendu::dispositionScene(r), 0, 1, &g.set[f], 0, nullptr);
        vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, AccesRendu::pipeline(r, 0));
        vkCmdDraw(cb, 3, 1, 0, 0);   // ciel
        // objets vus par la glace (bit VIS_RETRO, classés avant le dessin) : opaques dans l'ordre d'appel, puis transparents (déjà
        // triés du plus loin au plus proche de l'œil, qui est aussi l'œil du reflet à quelques cm près)
        VkPipeline lie = VK_NULL_HANDLE;
        uint32_t appels = 0, triangles = 0, trianglesJoueur = 0;
        // vitres et pièces translucides de la voiture du joueur (hors feux) : jamais dans la glace. Vues de l'intérieur, leur reflet du ciel
        // (shader des vitres, fait pour la vue de dehors) voilait toute la glace intérieure de bleu ; une vraie vitre vue de l'habitacle ne
        // renvoie que l'habitacle, sombre (les vitres de certains modèles n'ont que MAT_BLEND, sans MAT_GLASS)
        auto vitreJoueur = [](const auto& p) {
            const uint32_t f = AccesRendu::drapeaux(p);
            return AccesRendu::joueur(p) && (f & (MAT_GLASS | MAT_BLEND)) != 0 && (f & (MAT_TAIL | MAT_HEAD)) == 0;
        };
        for (const auto& p : AccesRendu::opaques(r)) {
            if (!AccesRendu::visibleRetro(p) || vitreJoueur(p)) continue;
            const bool sq = AccesRendu::squelette(p);
            if (sq && p.depth > PERSONNES_MAX_M) continue;
            const VkPipeline v = sq ? g.squelette : AccesRendu::pipeline(r, 1);
            if (v != lie) { vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, v); lie = v; }
            if (k == INTERIEUR && AccesRendu::joueur(p)) {
                // glace intérieure : on y voit l'habitacle de la voiture (pavillon, sièges, montants arrière) par l'intérieur, à l'ombre du
                // toit ; sans occlusion, il prenait tout le ciel et le soleil et voilait la glace de bleu (vu au banc). Albédo × 0,25 :
                // occlusion d'un habitacle fermé (le ciel ne s'y voit que par les vitres, ≈ 0,2 à 0,3, E)
                auto q = p;
                q.pc.color.x *= OCCLUSION_HABITACLE; q.pc.color.y *= OCCLUSION_HABITACLE; q.pc.color.z *= OCCLUSION_HABITACLE;
                AccesRendu::dessiner(r, q);
            } else AccesRendu::dessiner(r, p);
            appels++; triangles += AccesRendu::triangles(p); if (AccesRendu::joueur(p)) trianglesJoueur += AccesRendu::triangles(p);
        }
        for (const auto& p : AccesRendu::transparents(r)) {
            if (!AccesRendu::visibleRetro(p) || vitreJoueur(p)) continue;
            const uint32_t fl = AccesRendu::drapeaux(p);
            const VkPipeline v = (fl & MAT_POOL) ? g.tache : (fl & MAT_ADD) ? AccesRendu::pipeline(r, 3) : AccesRendu::pipeline(r, 2);
            if (v != lie) { vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, v); lie = v; }
            AccesRendu::dessiner(r, p); appels++; triangles += AccesRendu::triangles(p); if (AccesRendu::joueur(p)) trianglesJoueur += AccesRendu::triangles(p);
        }
        vkCmdEndRenderPass(cb);   // fin de passe : SHADER_READ_ONLY, lisible par la passe de scène de la même image
        g.rendues[k]++;
        g.comptes[k] = {appels, triangles, trianglesJoueur};
    }
};
WFS_PASSE(PasseRetroviseurs, 20)

} // namespace

void preparer(FrameParams& fp, bool actifImage, const std::array<Miroir, NB_GLACES>& m, const mat4& repereInterieur, bool basse) {
    g.glace = -1;
    if (!actifImage || g.desactive) return;
    if (basse && (g.images++ & 1u)) return;   // Basse : une glace toutes les deux images (RG-03 : coût mesuré au journal)
    g.vpCamera = fp.proj * fp.view;
    for (int i = 0; i < NB_GLACES; i++) g.fen[i] = fenetre(m[(size_t)i], repereInterieur);
    const int k = g.force >= 0 ? g.force : ORDRE[g.compteur++ % 4];
    // reflet vu depuis la glace du modèle 3D (decalageVue) par un œil décalé d'autant : même vue de la glace, flanc à sa vraie place
    const Fenetre f = fenetre(m[(size_t)k], repereInterieur, true);
    const vec3 oeil = fp.camPos + repereInterieur.transformDir(m[(size_t)k].decalageVue);
    vec3 oeilR; mat4 vp;
    if (!vueProjMiroir(f, oeil, m[(size_t)k].recul, vp, oeilR)) return;   // œil derrière la glace
    // regard du reflet : de l'œil réfléchi vers le centre de la glace
    g.glace = k; g.vp = vp; g.oeilReflet = oeilR;
    g.regard = normalize((f.pb + f.pc) * 0.5f - oeilR);
    fp.retro = true; fp.retroVueProj = vp;
}
bool actif() { return g.glace >= 0; }
int glaceDeLImage() { return g.glace; }
Comptes comptesGlace(int k) { return k >= 0 && k < NB_GLACES ? g.comptes[k] : Comptes{}; }
bool lireImage(Renderer& r, int k, std::vector<float>& rgba, uint32_t& w, uint32_t& h) {
    if (k < 0 || k >= NB_GLACES || !g.pret) return false;
    w = TAILLE[k][0]; h = TAILLE[k][1];
    std::vector<uint8_t> o;
    if (!passes::lireCouche(AccesRendu::vk(r), g.couleur[k].img, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, w, h, 8, o)) return false;
    rgba.resize((size_t)w * h * 4);
    const uint16_t* s = reinterpret_cast<const uint16_t*>(o.data());
    for (size_t i = 0; i < rgba.size(); i++) rgba[i] = passes::demiVersFloat(s[i]);
    return true;
}
uint64_t imagesRendues(int k) { return k >= 0 && k < NB_GLACES ? g.rendues[k] : 0; }
void forcerGlace(int k) { g.force = k >= 0 && k < NB_GLACES ? k : -1; }
void desactiver(bool d) { g.desactive = d; }
bool rectEcran(int k, float W, float H, float& x0, float& y0, float& x1, float& y1) {
    if (k < 0 || k >= NB_GLACES) return false;
    const Fenetre& f = g.fen[k];
    const vec3 c[4] = {f.pa, f.pb, f.pc, f.pb + f.pc - f.pa};
    x0 = y0 = 1e9f; x1 = y1 = -1e9f;
    for (const vec3& p : c) {
        const mat4& M = g.vpCamera;
        const float qx = M.at(0, 0) * p.x + M.at(0, 1) * p.y + M.at(0, 2) * p.z + M.at(0, 3), qy = M.at(1, 0) * p.x + M.at(1, 1) * p.y + M.at(1, 2) * p.z + M.at(1, 3);
        const float qw = M.at(3, 0) * p.x + M.at(3, 1) * p.y + M.at(3, 2) * p.z + M.at(3, 3);
        if (qw <= 1e-4f) return false;
        const float x = (qx / qw * 0.5f + 0.5f) * W, y = (qy / qw * 0.5f + 0.5f) * H;
        x0 = std::min(x0, x); x1 = std::max(x1, x); y0 = std::min(y0, y); y1 = std::max(y1, y);
    }
    return x1 > 0 && y1 > 0 && x0 < W && y0 < H;
}

const Image* image(Renderer& r, int k) {
    if (k < 0 || k >= NB_GLACES) return nullptr;
    creer(r);
    return g.couleur[k].view ? &g.couleur[k] : nullptr;
}

Material materiau(Renderer& r, int k, float reflexion) {
    Material m;
    m.flags = MAT_EMTEX | MAT_DOUBLE;
    m.color = {1, 1, 1, 1}; m.metal = 0; m.rough = 0.05f;   // verre poli : 4 % de reflet de la face avant (le ciel, le soleil)
    m.cutoff = CUTOFF_RETRO;
    m.emissive = {reflexion, reflexion, reflexion};
    return r.makeMaterial(m, image(r, k), nullptr, nullptr);
}

} // namespace wfs::retro
