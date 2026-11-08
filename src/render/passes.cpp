// WaForSpeed — passes de rendu enfichables (WFS-150) : ordonnanceur, horodatages GPU, pré-passe. Voir passes.h.
#include "passes.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iterator>
#include "../core/log.h"
#include "../ui/ecrans/reglages.h"
#include "qualite_auto.h"

namespace wfs {

static const uint32_t SPV_PREPASSE_FRAG[] = {
#include "prepasse.frag.inc"
};

const char* nomEtape(EtapeRendu e) {
    static const char* N[] = {"ombres", "pré-passe", "écran", "scène", "transparents", "post-traitement"};
    const int i = (int)e;
    return i >= 0 && i < (int)EtapeRendu::N ? N[i] : "?";
}

VkPipeline AccesRendu::pipeline(const Renderer& r, int quoi) {
    switch (quoi) {
    case 0: return r.refSky_;
    case 1: return r.refOpaque_;
    case 2: return r.refBlend_;
    default: return r.refAdd_;
    }
}

namespace passes {
namespace {

struct Inscrite { Fabrique f; int ordre; };
std::vector<Inscrite>& inscrites() { static std::vector<Inscrite> v; return v; }

constexpr int MAX_PASSES = 16;            // passes enfichables horodatées (2 requêtes chacune par image en vol)
constexpr int IMAGES = 2;                 // = Renderer::FRAMES
static_assert(IMAGES == AccesRendu::images(), "passes (WFS-150) : une ressource par image en vol");
constexpr int FENETRE = 120;              // images par fenêtre de la ligne « Rendu » (comme le journal des ombres)
enum { R_OMBRES = 0, R_REFLETS = 1, R_SCENE = 2, N_RENDERER = 3 };

struct Somme { double ms = 0; int n = 0; };
struct Etat {
    std::vector<PasseRendu*> passes;      // triées : étape puis ordre
    bool construit = false;
    Renderer* r = nullptr;
    VkQueryPool ts = VK_NULL_HANDLE; double periode = 0;   // ms par graduation
    uint32_t ecrites[IMAGES] = {0, 0};    // bits : passes horodatées dans l'image en vol
    bool actives[IMAGES][MAX_PASSES]{};   // passe active à cette image (étapes Scene / Transparents soustraites de la scène)
    Somme cumul[MAX_PASSES + N_RENDERER], fen[MAX_PASSES + N_RENDERER], fenPrec[MAX_PASSES + N_RENDERER];
    int imagesFen = 0; bool fenPrete = false;
    bool prePasseVoulue = false, prePasseFaite = false;
    bool essai = false;
};
Etat g;

void construire() {
    if (g.construit) return;
    g.construit = true;
    std::vector<Inscrite> v = inscrites();
    std::vector<std::pair<PasseRendu*, int>> p;
    for (const Inscrite& i : v) p.push_back({i.f(), i.ordre});
    std::stable_sort(p.begin(), p.end(), [](const auto& a, const auto& b) {
        return a.first->etape() != b.first->etape() ? a.first->etape() < b.first->etape() : a.second < b.second;
    });
    for (auto& [pp, o] : p) {
        if ((int)g.passes.size() >= MAX_PASSES) { WFS_ERR("passes (WFS-150) : plus de %d passes, %s ignorée", MAX_PASSES, pp->nom()); delete pp; continue; }
        g.passes.push_back(pp);
    }
}

int niveauCourant() { return qualite::niveauAffiche(ecrans::reglages().qualite); }

bool tourne(const PasseRendu& p, const ImagePasse& img) { return img.niveau >= p.qualiteMin() && p.active(img); }

void executerEtape(EtapeRendu e, const ImagePasse& img) {
    for (size_t i = 0; i < g.passes.size(); i++) {
        PasseRendu& p = *g.passes[i];
        if (p.etape() != e || !tourne(p, img)) continue;
        if (e == EtapeRendu::PrePasse && !g.prePasseVoulue) continue;
        const uint32_t q = (uint32_t)(img.image * MAX_PASSES * 2 + (int)i * 2);
        if (g.ts) vkCmdWriteTimestamp(img.cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, g.ts, q);
        p.executer(img);
        if (g.ts) { vkCmdWriteTimestamp(img.cb, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, g.ts, q + 1); g.ecrites[img.image] |= 1u << i; }
        g.actives[img.image][i] = true;
        if (e == EtapeRendu::PrePasse) g.prePasseFaite = true;
    }
}

void ajouter(int k, double ms) {
    g.cumul[k].ms += ms; g.cumul[k].n++;
    g.fen[k].ms += ms; g.fen[k].n++;
}

std::vector<Cout> couts(const Somme* s) {
    std::vector<Cout> c;
    const bool mes = g.r && AccesRendu::horodatages(*g.r);
    const char* noms[N_RENDERER] = {"ombres du soleil", "reflets de la chaussée", "scène + bloom (hors passes enfichables)"};
    const EtapeRendu et[N_RENDERER] = {EtapeRendu::Ombres, EtapeRendu::Scene, EtapeRendu::Scene};
    for (int k = 0; k < N_RENDERER; k++) {
        Cout x; x.nom = noms[k]; x.etape = et[k]; x.renderer = true; x.mesurable = mes;
        const Somme& v = s[MAX_PASSES + k]; x.images = v.n; x.ms = v.n ? v.ms / v.n : 0;
        c.push_back(x);
    }
    for (size_t i = 0; i < g.passes.size(); i++) {
        Cout x; x.nom = g.passes[i]->nom(); x.etape = g.passes[i]->etape(); x.mesurable = g.ts != VK_NULL_HANDLE;
        x.images = s[i].n; x.ms = s[i].n ? s[i].ms / s[i].n : 0;
        c.push_back(x);
    }
    return c;
}

// ---------------------------------------------------------------- RG-02 : pré-passe profondeur + normales (demi-résolution)
// Passe séparée à la moitié de la scène (¼ des pixels) : elle ne peut pas rester en mémoire de tuile (les passes d'écran
// la lisent dans une autre passe Vulkan). Format compact : normales A2B10G10R10 (4 octets), profondeur du GPU (D32 : 4 octets,
// D16 si le format de la scène n'est pas échantillonnable). Créée à la première demande (aucune mémoire sans demande).
class PrePasse;
PrePasse* g_pre = nullptr;
class PrePasse : public PasseRendu {
public:
    PrePasse() { g_pre = this; }
    const char* nom() const override { return "pré-passe profondeur + normales (½ résolution)"; }
    EtapeRendu etape() const override { return EtapeRendu::PrePasse; }
    const char* cibles() const override { return "écrit normales A2B10G10R10 + profondeur, ½ × ½ de la scène"; }
    bool active(const ImagePasse&) const override { return g.prePasseVoulue; }
    void redimensionner(Renderer& r) override { liberer(r); }
    void detruire(Renderer& r) override {
        liberer(r);
        VkDevice d = AccesRendu::vk(r).device;
        for (VkPipeline* p : {&opaque_, &squelette_}) { if (*p) vkDestroyPipeline(d, *p, nullptr); *p = VK_NULL_HANDLE; }
        if (rp_) vkDestroyRenderPass(d, rp_, nullptr);
        if (smp_) vkDestroySampler(d, smp_, nullptr);
        if (frag_) vkDestroyShaderModule(d, frag_, nullptr);
        rp_ = VK_NULL_HANDLE; smp_ = VK_NULL_HANDLE; frag_ = VK_NULL_HANDLE;
    }
    void executer(const ImagePasse& img) override {
        Renderer& r = img.r;
        if (!pret_ && !preparer(r)) return;
        VkClearValue cl[2]{}; cl[0].color = {{0, 0, 0, 0}}; cl[1].depthStencil = {0.f, 0};   // profondeur inversée : 0 = loin
        VkRenderPassBeginInfo bi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        bi.renderPass = rp_; bi.framebuffer = fb_; bi.renderArea = {{0, 0}, {w_, h_}}; bi.clearValueCount = 2; bi.pClearValues = cl;
        vkCmdBeginRenderPass(img.cb, &bi, VK_SUBPASS_CONTENTS_INLINE);
        VkViewport vp{0, 0, (float)w_, (float)h_, 0, 1}; VkRect2D sc{{0, 0}, {w_, h_}};
        vkCmdSetViewport(img.cb, 0, 1, &vp); vkCmdSetScissor(img.cb, 0, 1, &sc);
        VkDescriptorSet s0 = AccesRendu::setImage(r, img.image);
        vkCmdBindDescriptorSets(img.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, AccesRendu::dispositionScene(r), 0, 1, &s0, 0, nullptr);
        constexpr uint32_t SANS = MAT_BLEND | MAT_SHADOWBLOB | MAT_POOL | MAT_ADD | MAT_FANTOME;
        VkPipeline lie = VK_NULL_HANDLE;
        for (const auto& p : AccesRendu::opaques(r)) {
            if (!AccesRendu::visibleCamera(p) || (AccesRendu::drapeaux(p) & SANS)) continue;
            const VkPipeline v = AccesRendu::squelette(p) ? squelette_ : opaque_;
            if (v != lie) { vkCmdBindPipeline(img.cb, VK_PIPELINE_BIND_POINT_GRAPHICS, v); lie = v; }
            AccesRendu::dessiner(r, p);
        }
        vkCmdEndRenderPass(img.cb);
    }
    VkImageView normales() const { return pret_ ? n_.view : VK_NULL_HANDLE; }
    VkImageView profondeur() const { return pret_ ? z_.view : VK_NULL_HANDLE; }
    VkSampler echantillonneur() const { return smp_; }
    uint32_t largeur() const { return w_; }
    uint32_t hauteur() const { return h_; }
private:
    bool preparer(Renderer& r) {
        VkCtx& vk = AccesRendu::vk(r); VkDevice d = vk.device;
        w_ = std::max(1u, AccesRendu::largeurScene(r) / 2); h_ = std::max(1u, AccesRendu::hauteurScene(r) / 2);
        if (!rp_) {
            VkFormatProperties fp; vkGetPhysicalDeviceFormatProperties(vk.gpu, vk.depthFormat, &fp);
            zf_ = (fp.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) ? vk.depthFormat : VK_FORMAT_D16_UNORM;
            VkAttachmentDescription at[2]{};
            at[0].format = NF; at[0].samples = VK_SAMPLE_COUNT_1_BIT;
            at[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; at[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            at[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; at[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            at[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; at[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            at[1] = at[0]; at[1].format = zf_; at[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            VkAttachmentReference cr{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}, dr{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
            VkSubpassDescription sp{}; sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            sp.colorAttachmentCount = 1; sp.pColorAttachments = &cr; sp.pDepthStencilAttachment = &dr;
            VkSubpassDependency dep[2]{};
            dep[0].srcSubpass = VK_SUBPASS_EXTERNAL; dep[0].dstSubpass = 0;
            dep[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dep[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dep[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            dep[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dep[1].srcSubpass = 0; dep[1].dstSubpass = VK_SUBPASS_EXTERNAL;
            dep[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            dep[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            dep[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dep[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            VkRenderPassCreateInfo ci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
            ci.attachmentCount = 2; ci.pAttachments = at; ci.subpassCount = 1; ci.pSubpasses = &sp; ci.dependencyCount = 2; ci.pDependencies = dep;
            VK_OK(vkCreateRenderPass(d, &ci, nullptr, &rp_));
            VkShaderModuleCreateInfo mi{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
            mi.codeSize = sizeof(SPV_PREPASSE_FRAG); mi.pCode = SPV_PREPASSE_FRAG;
            VK_OK(vkCreateShaderModule(d, &mi, nullptr, &frag_));
            // mêmes sommets que la scène (scene.vert, scene_skin.vert : position, normale, uv aux mêmes emplacements)
            opaque_ = AccesRendu::faire(r, AccesRendu::module(r, 0), frag_, 1, false, true, true, rp_, AccesRendu::dispositionScene(r));
            squelette_ = AccesRendu::faire(r, AccesRendu::module(r, 4), frag_, 3, false, true, true, rp_, AccesRendu::dispositionSquelette(r));
            VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
            si.magFilter = si.minFilter = VK_FILTER_NEAREST; si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            si.addressModeU = si.addressModeV = si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            VK_OK(vkCreateSampler(d, &si, nullptr, &smp_));
            WFS_LOG("pré-passe (WFS-150) : normales A2B10G10R10, profondeur format %d, ½ résolution", (int)zf_);
        }
        n_ = vk.createImage(w_, h_, 1, NF, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        z_ = vk.createImage(w_, h_, 1, zf_, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);
        VkImageView v[2] = {n_.view, z_.view};
        VkFramebufferCreateInfo fi{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fi.renderPass = rp_; fi.attachmentCount = 2; fi.pAttachments = v; fi.width = w_; fi.height = h_; fi.layers = 1;
        VK_OK(vkCreateFramebuffer(d, &fi, nullptr, &fb_));
        pret_ = true;
        WFS_LOG("pré-passe (WFS-150) : cibles %ux%u (%.2f Mo)", w_, h_, (double)w_ * h_ * 8 / 1048576.0);
        return true;
    }
    void liberer(Renderer& r) {
        if (!pret_) return;
        VkCtx& vk = AccesRendu::vk(r);
        if (fb_) vkDestroyFramebuffer(vk.device, fb_, nullptr);
        fb_ = VK_NULL_HANDLE; vk.destroy(n_); vk.destroy(z_); pret_ = false;
    }
    static constexpr VkFormat NF = VK_FORMAT_A2B10G10R10_UNORM_PACK32;
    VkFormat zf_ = VK_FORMAT_D16_UNORM;
    VkRenderPass rp_ = VK_NULL_HANDLE; VkFramebuffer fb_ = VK_NULL_HANDLE;
    VkPipeline opaque_ = VK_NULL_HANDLE, squelette_ = VK_NULL_HANDLE; VkShaderModule frag_ = VK_NULL_HANDLE;
    VkSampler smp_ = VK_NULL_HANDLE;
    Image n_, z_; uint32_t w_ = 0, h_ = 0; bool pret_ = false;
};
WFS_PASSE(PrePasse, 0)

PrePasse* pre() { construire(); return g_pre; }

} // namespace

void enregistrer(Fabrique f, int ordre) { inscrites().push_back({f, ordre}); }
const std::vector<PasseRendu*>& liste() { construire(); return g.passes; }

void creer(Renderer& r) {
    construire();
    g.r = &r;
    VkCtx& vk = AccesRendu::vk(r);
    if (vk.props.limits.timestampComputeAndGraphics && !g.ts) {
        VkQueryPoolCreateInfo q{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
        q.queryType = VK_QUERY_TYPE_TIMESTAMP; q.queryCount = IMAGES * MAX_PASSES * 2;
        VK_OK(vkCreateQueryPool(vk.device, &q, nullptr, &g.ts));
        g.periode = vk.props.limits.timestampPeriod * 1e-6;
    }
    for (PasseRendu* p : g.passes) p->creer(r);
    std::string l;
    for (PasseRendu* p : g.passes) l += std::string(l.empty() ? "" : " ; ") + nomEtape(p->etape()) + " : " + p->nom();
    WFS_LOG("passes (WFS-150) : %zu enregistrées (%s), horodatages GPU %s", g.passes.size(), l.c_str(), g.ts ? "oui" : "non disponibles");
}
void redimensionner(Renderer& r) { for (PasseRendu* p : g.passes) p->redimensionner(r); }
void detruire(Renderer& r) {
    for (PasseRendu* p : g.passes) p->detruire(r);
    if (g.ts) vkDestroyQueryPool(AccesRendu::vk(r).device, g.ts, nullptr);
    g.ts = VK_NULL_HANDLE; g.ecrites[0] = g.ecrites[1] = 0; g.r = nullptr;
    std::memset(g.actives, 0, sizeof g.actives);
}

void debutImage(Renderer& r) {
    const int f = AccesRendu::image(r);
    // coût de l'image précédente sur ce tampon (sa barrière vient d'être attendue)
    double dansScene = 0;
    if (g.ts && g.ecrites[f]) {
        for (size_t i = 0; i < g.passes.size(); i++) {   // requêtes lues passe par passe : celles des passes absentes ne sont pas écrites
            if (!(g.ecrites[f] & (1u << i))) continue;
            uint64_t t[2];
            if (vkGetQueryPoolResults(AccesRendu::vk(r).device, g.ts, (uint32_t)(f * MAX_PASSES * 2 + 2 * (int)i), 2, sizeof t, t, sizeof(uint64_t),
                                      VK_QUERY_RESULT_64_BIT) == VK_SUCCESS) {
                const double ms = (double)(t[1] - t[0]) * g.periode;
                ajouter((int)i, ms);
                const EtapeRendu e = g.passes[i]->etape();
                if (e == EtapeRendu::Scene || e == EtapeRendu::Transparents) dansScene += ms;
            }
        }
    }
    g.ecrites[f] = 0;
    if (VkQueryPool p = AccesRendu::horodatages(r); p && AccesRendu::horodatagesEcrits(r, f)) {
        uint64_t t[4];   // WFS-246 (RG-06) : 5 horodatages par image (le 5e : fin de la passe finale)
        if (vkGetQueryPoolResults(AccesRendu::vk(r).device, p, (uint32_t)f * Renderer::HORODATAGES, 4, sizeof t, t, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT) == VK_SUCCESS) {
            const double k = AccesRendu::vk(r).props.limits.timestampPeriod * 1e-6;
            ajouter(MAX_PASSES + R_OMBRES, (double)(t[1] - t[0]) * k);
            ajouter(MAX_PASSES + R_REFLETS, (double)(t[2] - t[1]) * k);
            ajouter(MAX_PASSES + R_SCENE, std::max(0.0, (double)(t[3] - t[2]) * k - dansScene));
        }
    }
    std::memset(g.actives[f], 0, sizeof g.actives[f]);
    if (++g.imagesFen >= FENETRE) {
        std::copy(std::begin(g.fen), std::end(g.fen), g.fenPrec); std::fill(std::begin(g.fen), std::end(g.fen), Somme{});
        g.imagesFen = 0; g.fenPrete = true;
    }
    // RG-03 : une passe qui doit tout voir (capture d'environnement) : aucun objet masqué à cette image
    bool tout = false;
    for (PasseRendu* p : g.passes) tout = tout || p->toutVoir();
    AccesRendu::sansMasquage(r, tout);
}

void avantScene(Renderer& r, VkCommandBuffer cb) {
    if (g.passes.empty()) return;
    const int f = AccesRendu::image(r);
    if (g.ts) vkCmdResetQueryPool(cb, g.ts, (uint32_t)(f * MAX_PASSES * 2), MAX_PASSES * 2);
    ImagePasse ip{r, cb, f, AccesRendu::largeurScene(r), AccesRendu::hauteurScene(r), AccesRendu::ubo(r), niveauCourant()};
    // RG-02 : la pré-passe ne tourne que si une passe active de cette image la demande
    g.prePasseVoulue = false; g.prePasseFaite = false;
    for (PasseRendu* p : g.passes) if (p->demandePrePasse() && tourne(*p, ip)) g.prePasseVoulue = true;
    executerEtape(EtapeRendu::Ombres, ip);
    executerEtape(EtapeRendu::PrePasse, ip);
    executerEtape(EtapeRendu::Ecran, ip);
}
void dansScene(Renderer& r, VkCommandBuffer cb, EtapeRendu e) {
    if (g.passes.empty()) return;
    ImagePasse ip{r, cb, AccesRendu::image(r), AccesRendu::largeurScene(r), AccesRendu::hauteurScene(r), AccesRendu::ubo(r), niveauCourant()};
    executerEtape(e, ip);
}
void apresScene(Renderer& r, VkCommandBuffer cb) {
    if (g.passes.empty()) return;
    ImagePasse ip{r, cb, AccesRendu::image(r), AccesRendu::largeurScene(r), AccesRendu::hauteurScene(r), AccesRendu::ubo(r), niveauCourant()};
    executerEtape(EtapeRendu::Post, ip);
}

std::vector<Cout> coutsCumules() { return couts(g.cumul); }
std::vector<Cout> coutsFenetre() { return couts(g.fenPrete ? g.fenPrec : g.fen); }
void remettreAZero() { std::fill(std::begin(g.cumul), std::end(g.cumul), Somme{}); }
bool horodatagesDisponibles() { return g.ts != VK_NULL_HANDLE; }

// ligne courte (une ligne des Réglages) : « GPU ms omb 0,4 refl 0,0 scène 3,1 pré-passe 0,2 » ; passes enfichables par le
// premier mot de leur nom, passes qui n'ont pas tourné omises
std::string texteCouts(const std::vector<Cout>& c) {
    if (!g.ts) return "GPU : non mesurable";
    std::string s = "GPU ms";
    for (const Cout& x : c) {
        if (!x.images) continue;
        std::string court = x.nom.substr(0, x.nom.find(' '));
        if (x.renderer) court = x.nom.rfind("ombres", 0) == 0 ? "omb" : x.nom.rfind("reflets", 0) == 0 ? "refl" : "scène";
        char b[64];
        std::snprintf(b, sizeof b, " %s %.1f", court.c_str(), x.ms);
        s += b;
    }
    for (char& ch : s) if (ch == '.') ch = ',';
    return s;
}

bool prePasseFaite() { return g.prePasseFaite; }
VkImageView prePasseNormales() { return pre() ? pre()->normales() : VK_NULL_HANDLE; }
VkImageView prePasseProfondeur() { return pre() ? pre()->profondeur() : VK_NULL_HANDLE; }
VkSampler echantillonneurPrePasse() { return pre() ? pre()->echantillonneur() : VK_NULL_HANDLE; }
uint32_t prePasseLargeur() { return pre() ? pre()->largeur() : 0; }
uint32_t prePasseHauteur() { return pre() ? pre()->hauteur() : 0; }

void activerEssai(bool actif) { g.essai = actif; }
bool essaiActif() { return g.essai; }

float demiVersFloat(uint16_t h) {
    const uint32_t s = (h >> 15) & 1, e = (h >> 10) & 31, m = h & 1023;
    float v;
    if (e == 0) v = (float)m * (1.f / 16777216.f);                    // sous-normal : m × 2^-24
    else if (e == 31) v = m ? 0.f : 65504.f;                          // infini / NaN : borné (aucun dans une image saine)
    else { uint32_t b = ((e + 112) << 23) | (m << 13); std::memcpy(&v, &b, 4); }
    return s ? -v : v;
}

bool lireCouche(VkCtx& vk, VkImage img, VkImageLayout layout, uint32_t couche, uint32_t w, uint32_t h, size_t opp, std::vector<uint8_t>& out) {
    if (!img || !w || !h) return false;
    vk.waitIdle();
    Buffer buf = vk.createBuffer((VkDeviceSize)w * h * opp, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true);
    VkCommandBuffer cb = vk.beginOneShot();
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    b.oldLayout = layout; b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = img; b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, couche, 1};
    b.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT; b.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
    VkBufferImageCopy c{}; c.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, couche, 1}; c.imageExtent = {w, h, 1};
    vkCmdCopyImageToBuffer(cb, img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buf.buf, 1, &c);
    std::swap(b.oldLayout, b.newLayout); b.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT; b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
    vk.endOneShot(cb);
    out.assign((const uint8_t*)buf.mapped, (const uint8_t*)buf.mapped + (size_t)w * h * opp);
    vk.destroy(buf);
    return true;
}

bool lireHdr(Renderer& r, std::vector<float>& rgba, uint32_t& w, uint32_t& h) {
    const Image& im = AccesRendu::hdr(r);
    w = im.w; h = im.h;
    std::vector<uint8_t> o;
    if (!lireCouche(AccesRendu::vk(r), im.img, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, w, h, 8, o)) return false;
    rgba.resize((size_t)w * h * 4);
    const uint16_t* s = reinterpret_cast<const uint16_t*>(o.data());
    for (size_t i = 0; i < rgba.size(); i++) rgba[i] = demiVersFloat(s[i]);
    return true;
}

} // namespace passes
} // namespace wfs
