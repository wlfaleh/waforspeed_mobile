// WaForSpeed — WFS-151 : éclairage physique, exposition automatique et sondes IBL (voir eclairage_physique.h).
#include "atmosphere_physique.h"   // WFS-207 : astres calculés, atmosphère physique
#include "eclairage_physique.h"
#include "eclairage_exposition.h"   // WFS-213 : mesures et sondes valides (testé par fidelite_lumiere)
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <thread>
#include "../core/jobs.h"
#include "../core/log.h"
#include "../core/system.h"
#include "../game/player.h"
#include "../game/sauvegarde.h"   // WFS-208 : moment de la dernière course (après un redémarrage)
#include "../game/trace_cartes.h"
#include "atmosphere_brume.h"   // WFS-381
#include "../game/track.h"
#include "../ui/ecrans/reglages.h"
#include "eclairage.h"   // lampadaires (hauteur de la tête)
#include "environnement.h"
#include "passes.h"
#include "qualite_auto.h"
#include "renderer.h"

namespace wfs {
namespace eclairage {
namespace {

using namespace lumiere;
constexpr int IMAGES = AccesRendu::images();
constexpr int NIVEAUX = 6;          // niveaux du cube préfiltré (rugosité 0, 0,2 … 1)
constexpr int ECHANTILLONS = 48;    // échantillons GGX par texel préfiltré
constexpr int LUT = 32;             // table BRDF : 32 × 32 (NoV, rugosité), 256 échantillons par case
constexpr int COTE_CIEL = 32;       // sonde du ciel calculée sur le CPU
constexpr VkFormat F16 = VK_FORMAT_R16G16B16A16_SFLOAT;

struct CelluleDir { vec3 d; double log2L, omega; };

// sonde prête pour le GPU (produite par le traitement CPU)
struct Resultat {
    Sh9 sh; float echelle = 1; int cote = 0, niveaux = 0; vec3 pos{}; bool capture = false; double ms = 0; int generation = 0; bool eclaireLoin = false;
    std::vector<uint16_t> donnees; std::vector<size_t> decalages;   // [niveau × 6 + face] en octets
    std::vector<CelluleDir> carte;                                 // luminances (exposition automatique)
};
struct Sonde {
    bool ok = false, capture = false; vec3 pos{}; Sh9 sh; float echelle = 1; int niveaux = 1;
    Image gpu; std::vector<CelluleDir> carte;
};
struct Retrait { int images; Image img; Buffer tampon; };

struct Etat_ {
    bool desactive = false; Forcage forc; Etat etat;
    vec3 astreCache{0, -2, 0}; bool cielCalcule = false; atmosphere::Ciel atmo;   // WFS-207 : atmosphère de l'astre courant
    // WFS-208 : moment choisi (écran Terrain) ou relu dans la sauvegarde, heures des moments (cache), temps écoulé (accéléré)
    atmosphere::Moment moment = atmosphere::Moment::Midi; bool momentChoisi = false, momentLu = false;
    double heureMoment[(int)atmosphere::Moment::NB] = {-100, -100, -100, -100, -100, -100, -100}, ecoule = 0, acceleration = 1;
    // image courante (physique → avantImage)
    bool imageActive = false; double k = 0, facteur = 1; vec4 cielP{};
    bool releverNuit = true;   // WFS-276 : basses lumières relevées la nuit (pas dans la Ville éclairée)
    double eRecu = 0;   // WFS-230 : éclairement horizontal reçu par le conducteur la nuit (lx)
    bool capturesSuspendues = false;   // WFS-222 (RG-03)
    // scène
    bool init = false; double tPrec = -1; vec3 pointPrec{}; bool nuitPrec = false;
    const void* pistePrec = nullptr; const void* cartePrec = nullptr; const void* voiturePrec = nullptr;   // WFS-213 (RG-01)
    double ev = 15; bool evInit = false, recaler = true; double dtImage = 0; int generation = 0;
    double tScene = 0;   // WFS-328 : temps de la dernière nouvelle scène (retard de capture du banc)
    double zPrec = 0;    // WFS-328 : abscisse du joueur à l'image précédente (« Recommencer »)
    bool nuitEstimee = false;   // WFS-328 : image précédente exposée sur l'estimation de nuit (sans sonde locale)
    double hCache = -1000, kCache = 0;
    // ciel (pour la sonde du ciel calculée sur le CPU)
    struct Ciel { bool nuit = false; double K = 0, Lnuit = 0; vec3 haut{}, horizon{}, sol{}, brume{}, soleil{}; } ciel;
    float signatureCiel[16]{}; bool cielAFaire = true;
    // sondes
    Sonde A, B, sondeCiel; double w = 1;
    Sh9 shAffiche; bool shInit = true, sceneNeuve = true;   // WFS-322 : diffus affiché, lissé dans le temps (lisserSh9) ; shInit : prendre la cible telle quelle
    vec3 dernierPoint{}; bool aucuneDemande = true; int typePrec = -1; bool forcerCapture = false;
    bool eclaireLoin = false; int iterations = 0;   // RG-03 : rebonds par recaptures au même point (voir physique())
    // relecture
    // WFS-213 (RG-02) : k de chaque face de la capture en cours (−1 : inconnu) ; copie pour la relecture
    int capturesVues = 0, faceVue = -1; double kFace[6] = {-1, -1, -1, -1, -1, -1}, kRelu[6] = {-1, -1, -1, -1, -1, -1}; bool relecture = false; int relectureImage = -1, relectureAttente = 0;
    Buffer tampon; int coteRelu = 0; vec3 posRelue{}; int generationRelue = 0; bool loinRelu = false;
    int generationDemande = -1;   // WFS-203 : scène (generation) de la dernière capture demandée
    // traitement
    std::atomic<bool> fini{false}; bool enCours = false; Resultat res;
    std::vector<Retrait> retraits;
    // ressources fixes
    VkSampler smp = VK_NULL_HANDLE; Image noir, lut; VkCtx* vk = nullptr;
    vec3 avant{0, 0, -1};
};
Etat_ g;

bool synchrone() { return std::getenv("WFS_BANC") != nullptr; }   // banc : images reproductibles (traitement sur le fil principal)

// ---------------------------------------------------------------- traitement CPU d'une sonde
// src : niveau 0 rempli (luminances physiques, cd/m²) ; rend le cube préfiltré (demi-flottants), SH9 et carte des luminances
Resultat traiter(CubeF& src, const vec3& pos, bool capture, bool parallele) {
    const auto t0 = std::chrono::steady_clock::now();
    Resultat r; r.pos = pos; r.capture = capture;
    const int N = src.cotes[0];
    src.reduire();
    r.sh = projeterSh9(src);
    // luminance moyenne (échelle de stockage : les demi-flottants gardent la précision de la nuit au plein soleil)
    double somme = 0, omega = 0;
    for (int f = 0; f < 6; f++) for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {
        const float* p = &src.face(0, f)[((size_t)y * N + x) * 3]; const double c[3] = {p[0], p[1], p[2]}, w = angleSolideTexel(x, y, N);
        somme += luminanceRgb(c) * w; omega += w;
    }
    r.echelle = (float)(1.0 / std::max(somme / omega, 1e-6));
    // carte des luminances : niveau de 4 × 4 texels par face (96 cellules)
    for (int m = 0; m < src.niveaux(); m++) if (src.cotes[m] == 4 || (m == src.niveaux() - 1 && r.carte.empty())) {
        const int c = src.cotes[m];
        for (int f = 0; f < 6; f++) for (int y = 0; y < c; y++) for (int x = 0; x < c; x++) {
            const float* p = &src.face(m, f)[((size_t)y * c + x) * 3]; const double rgb[3] = {p[0], p[1], p[2]};
            double d[3]; directionTexel(f, (x + 0.5) / c, (y + 0.5) / c, d);
            r.carte.push_back({{(float)d[0], (float)d[1], (float)d[2]}, std::log2(std::max(luminanceRgb(rgb), 1e-6)), angleSolideTexel(x, y, c)});
        }
        break;
    }
    // cube préfiltré : niveau m = rugosité m / (M − 1)
    int M = 1; while (M < NIVEAUX && (N >> M) >= 1) M++;
    r.cote = N; r.niveaux = M;
    size_t total = 0;
    for (int m = 0; m < M; m++) for (int f = 0; f < 6; f++) { r.decalages.push_back(total * 2); const size_t c = (size_t)std::max(1, N >> m); total += c * c * 4; }
    r.donnees.assign(total, 0);
    for (int m = 0; m < M; m++) {
        const int c = std::max(1, N >> m); const double rug = rugositeNiveau(m, M);
        auto lignes = [&](int debut, int fin) {
            for (int fy = debut; fy < fin; fy++) {
                const int f = fy / c, y = fy % c;
                uint16_t* o = &r.donnees[r.decalages[(size_t)m * 6 + f] / 2 + (size_t)y * c * 4];
                for (int x = 0; x < c; x++) {
                    double d[3], v[3]; directionTexel(f, (x + 0.5) / c, (y + 0.5) / c, d);
                    if (m == 0) { const float* p = &src.face(0, f)[((size_t)y * c + x) * 3]; v[0] = p[0]; v[1] = p[1]; v[2] = p[2]; }
                    else prefiltrerTexel(src, d, rug, ECHANTILLONS, v);
                    for (int q = 0; q < 3; q++) o[x * 4 + q] = versDemi((float)(v[q] * r.echelle));
                    o[x * 4 + 3] = versDemi(1.f);
                }
            }
        };
        if (parallele && c >= 16) jobs().parallelFor(6 * c, 8, lignes); else lignes(0, 6 * c);
    }
    r.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    return r;
}

// ---------------------------------------------------------------- ciel (même modèle que shaders/eclairage_ciel.glsl)
vec3 cielCpu(const Etat_::Ciel& c, const vec3& d) {
    const double h = d.y;
    const float tt = (float)std::pow(std::clamp(h, 0.0, 1.0), 0.45);
    vec3 teinte = lerp(c.horizon, c.haut, tt);
    const double y = std::max(0.2126729 * teinte.x + 0.7151522 * teinte.y + 0.0721750 * teinte.z, 1e-6);
    teinte = teinte * (float)(1.0 / y);
    double L = c.Lnuit;
    if (!c.nuit) { const double dd[3] = {d.x, d.y, d.z}, s[3] = {c.soleil.x, c.soleil.y, c.soleil.z}; L = c.K * cieF(std::acos(std::clamp(dd[0] * s[0] + dd[1] * s[1] + dd[2] * s[2], -1.0, 1.0))) * ciePhi(std::acos(std::clamp(h, 0.0, 1.0))); }
    auto lisse = [](double a, double b, double x) { const double t = std::clamp((x - a) / (b - a), 0.0, 1.0); return t * t * (3 - 2 * t); };
    vec3 col = lerp(teinte * (float)L, c.sol, (float)lisse(0.0, -0.25, h));
    return lerp(col, c.brume, (float)(lisse(0.12, -0.02, h) * 0.7));   // brume à l'horizon (sky.frag)
}
Resultat sondeDuCiel(const Etat_::Ciel& c) {
    CubeF cube; int n = 1; while ((COTE_CIEL >> (n - 1)) > 1) n++;
    cube.allouer(COTE_CIEL, n);
    for (int f = 0; f < 6; f++) for (int y = 0; y < COTE_CIEL; y++) for (int x = 0; x < COTE_CIEL; x++) {
        double d[3]; directionTexel(f, (x + 0.5) / COTE_CIEL, (y + 0.5) / COTE_CIEL, d);
        const vec3 v = cielCpu(c, {(float)d[0], (float)d[1], (float)d[2]});
        float* o = &cube.face(0, f)[((size_t)y * COTE_CIEL + x) * 3]; o[0] = v.x; o[1] = v.y; o[2] = v.z;
    }
    return traiter(cube, {}, false, false);
}

// WFS-322 (RG-01, avis de expert-realisme-3d) : diagnostic d'une sonde capturée (banc, WFS_DIAG_SONDE) : texels « ciel » (même valeur que
// le ciel calculé à 2 % près) et « surfaces » ; pour 4 normales horizontales, éclairement de chaque part, part des surfaces et teinte (x, y)
void diagnostiquerSonde(const CubeF& src, const vec3& pos) {
    const int N = src.cotes[0];
    const vec3 normales[4] = {{1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}};
    for (const vec3& n : normales) {
        double Ec[3] = {}, Es[3] = {}, Esol[3] = {};
        for (int f = 0; f < 6; f++) for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {
            double d[3]; directionTexel(f, (x + 0.5) / N, (y + 0.5) / N, d);
            const double c = d[0] * n.x + d[1] * n.y + d[2] * n.z;
            if (c <= 0) continue;
            const float* p = &src.face(0, f)[((size_t)y * N + x) * 3];
            const vec3 ciel = cielCpu(g.ciel, {(float)d[0], (float)d[1], (float)d[2]});
            const double cc[3] = {ciel.x, ciel.y, ciel.z}, pp[3] = {p[0], p[1], p[2]};
            const double lc = std::max(luminanceRgb(cc), 1e-9), lp = luminanceRgb(pp);
            const bool estCiel = d[1] > 0 && std::fabs(lp - lc) / lc < 0.02;
            const double w = angleSolideTexel(x, y, N) * c;
            double* E = estCiel ? Ec : Es;
            for (int q = 0; q < 3; q++) E[q] += p[q] * w;
            if (d[1] < 0) for (int q = 0; q < 3; q++) Esol[q] += p[q] * w;   // surfaces sous l'horizon (sol, chaussée)
        }
        auto xy = [](const double* e, double& X, double& Y) {   // sRGB linéaire → XYZ → xy
            const double Xx = 0.4124 * e[0] + 0.3576 * e[1] + 0.1805 * e[2], Yy = 0.2126 * e[0] + 0.7152 * e[1] + 0.0722 * e[2], Zz = 0.0193 * e[0] + 0.1192 * e[1] + 0.9505 * e[2];
            const double s = std::max(Xx + Yy + Zz, 1e-12); X = Xx / s; Y = Yy / s;
        };
        double xc, yc, xs, ys, xt, yt; const double Et[3] = {Ec[0] + Es[0], Ec[1] + Es[1], Ec[2] + Es[2]};
        xy(Ec, xc, yc); xy(Es, xs, ys); xy(Et, xt, yt);
        double Eh[3] = {Es[0] - Esol[0], Es[1] - Esol[1], Es[2] - Esol[2]}, xso, yso, xh, yh; xy(Esol, xso, yso); xy(Eh, xh, yh);
        WFS_LOG("sonde (WFS-322 diagnostic) : sous l'horizon %.0f lx (x %.3f y %.3f), surfaces au-dessus %.0f lx (x %.3f y %.3f)", luminanceRgb(Esol), xso, yso, luminanceRgb(Eh), xh, yh);
        const double lC = luminanceRgb(Ec), lS = luminanceRgb(Es);
        WFS_LOG("sonde (WFS-322 diagnostic) en (%.1f %.1f %.1f), normale (%g %g) : ciel %.0f lx (x %.3f y %.3f), surfaces %.0f lx (x %.3f y %.3f), part des surfaces %.0f %%, total x %.3f y %.3f",
                pos.x, pos.y, pos.z, n.x, n.z, lC, xc, yc, lS, xs, ys, 100 * lS / std::max(lC + lS, 1e-9), xt, yt);
    }
}

// ---------------------------------------------------------------- ressources GPU
Image creerCube(VkCtx& vk, int cote, int niveaux) {
    Image i; i.w = i.h = (uint32_t)cote; i.mips = (uint32_t)niveaux; i.format = F16;
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ci.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; ci.imageType = VK_IMAGE_TYPE_2D; ci.format = F16; ci.extent = {(uint32_t)cote, (uint32_t)cote, 1};
    ci.mipLevels = (uint32_t)niveaux; ci.arrayLayers = 6; ci.samples = VK_SAMPLE_COUNT_1_BIT; ci.tiling = VK_IMAGE_TILING_OPTIMAL;
    ci.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VK_OK(vkCreateImage(vk.device, &ci, nullptr, &i.img));
    VkMemoryRequirements mr; vkGetImageMemoryRequirements(vk.device, i.img, &mr);
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    ai.allocationSize = mr.size; ai.memoryTypeIndex = vk.memoryType(mr.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_OK(vkAllocateMemory(vk.device, &ai, nullptr, &i.mem));
    VK_OK(vkBindImageMemory(vk.device, i.img, i.mem, 0));
    VkImageViewCreateInfo vi{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    vi.image = i.img; vi.viewType = VK_IMAGE_VIEW_TYPE_CUBE; vi.format = F16; vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, (uint32_t)niveaux, 0, 6};
    VK_OK(vkCreateImageView(vk.device, &vi, nullptr, &i.view));
    return i;
}
// copie d'un tampon vers une image (couches × niveaux), puis lecture par les shaders
void enregistrerEnvoi(VkCommandBuffer cb, VkImage img, VkBuffer tampon, uint32_t couches, uint32_t niveaux, const std::vector<VkBufferImageCopy>& copies) {
    VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    b.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; b.image = img;
    b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, niveaux, 0, couches}; b.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
    vkCmdCopyBufferToImage(cb, tampon, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, (uint32_t)copies.size(), copies.data());
    b.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; b.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    b.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
}
// sonde → image cube : tampon de transfert rempli, commandes enregistrées dans cb (le tampon est rendu par retirer())
Sonde envoyer(VkCtx& vk, VkCommandBuffer cb, const Resultat& r, Buffer& tampon) {
    Sonde s; s.ok = true; s.capture = r.capture; s.pos = r.pos; s.sh = r.sh; s.echelle = r.echelle; s.niveaux = r.niveaux; s.carte = r.carte;
    s.gpu = creerCube(vk, r.cote, r.niveaux);
    tampon = vk.createBuffer(r.donnees.size() * 2, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
    std::memcpy(tampon.mapped, r.donnees.data(), r.donnees.size() * 2);
    std::vector<VkBufferImageCopy> c;
    for (int m = 0; m < r.niveaux; m++) for (int f = 0; f < 6; f++) {
        VkBufferImageCopy x{}; x.bufferOffset = r.decalages[(size_t)m * 6 + f];
        x.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, (uint32_t)m, (uint32_t)f, 1};
        const uint32_t cc = (uint32_t)std::max(1, r.cote >> m); x.imageExtent = {cc, cc, 1};
        c.push_back(x);
    }
    enregistrerEnvoi(cb, s.gpu.img, tampon.buf, 6, (uint32_t)r.niveaux, c);
    return s;
}
void retirer(Image img, Buffer b) { g.retraits.push_back({IMAGES + 1, img, b}); }

void creerFixes(VkCtx& vk) {
    if (g.smp) return;
    g.vk = &vk;
    VkSamplerCreateInfo si{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    si.magFilter = si.minFilter = VK_FILTER_LINEAR; si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR; si.maxLod = 16;
    si.addressModeU = si.addressModeV = si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    VK_OK(vkCreateSampler(vk.device, &si, nullptr, &g.smp));
    // cube noir 1 × 1 (avant la première sonde) et table BRDF (split-sum, calculée une fois)
    std::vector<uint16_t> noir(6 * 4, 0), table((size_t)LUT * LUT * 4, 0);
    for (int y = 0; y < LUT; y++) for (int x = 0; x < LUT; x++) {
        double A, B; tableBrdf((x + 0.5) / LUT, (y + 0.5) / LUT, 256, A, B);
        uint16_t* o = &table[((size_t)y * LUT + x) * 4]; o[0] = versDemi((float)A); o[1] = versDemi((float)B); o[3] = versDemi(1.f);
    }
    g.noir = creerCube(vk, 1, 1);
    g.lut = vk.createImage(LUT, LUT, 1, F16, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    Buffer b = vk.createBuffer((noir.size() + table.size()) * 2, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
    std::memcpy(b.mapped, noir.data(), noir.size() * 2); std::memcpy((uint8_t*)b.mapped + noir.size() * 2, table.data(), table.size() * 2);
    VkCommandBuffer cb = vk.beginOneShot();
    std::vector<VkBufferImageCopy> c(6);
    for (uint32_t f = 0; f < 6; f++) { c[f] = {}; c[f].bufferOffset = f * 8; c[f].imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, f, 1}; c[f].imageExtent = {1, 1, 1}; }
    enregistrerEnvoi(cb, g.noir.img, b.buf, 6, 1, c);
    VkBufferImageCopy t{}; t.bufferOffset = noir.size() * 2; t.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}; t.imageExtent = {LUT, LUT, 1};
    enregistrerEnvoi(cb, g.lut.img, b.buf, 1, 1, {t});
    vk.endOneShot(cb);
    vk.destroy(b);
    WFS_LOG("éclairage (WFS-151) : table BRDF %dx%d et cube noir prêts", LUT, LUT);
}
void detruireTout(VkCtx& vk) {
    vk.waitIdle();
    for (Retrait& x : g.retraits) { vk.destroy(x.img); vk.destroy(x.tampon); }
    g.retraits.clear();
    for (Sonde* s : {&g.A, &g.B, &g.sondeCiel}) { vk.destroy(s->gpu); *s = Sonde{}; }
    vk.destroy(g.noir); vk.destroy(g.lut); vk.destroy(g.tampon);
    if (g.smp) vkDestroySampler(vk.device, g.smp, nullptr);
    g.smp = VK_NULL_HANDLE; g.relecture = false; g.cielAFaire = true; g.vk = nullptr;
    while (g.enCours && !g.fini.load()) std::this_thread::yield();
    g.enCours = false; g.fini = false;
}

// remplace la sonde B par s (A ← B), A retirée ; mélange continu (w : part de B)
void installer(Sonde s, bool premiere) {
    if (g.A.gpu.img && g.A.gpu.img != g.B.gpu.img && g.A.gpu.img != g.sondeCiel.gpu.img) retirer(g.A.gpu, Buffer{});
    g.A = g.B; g.B = std::move(s);
    g.w = premiere ? 1.0 : std::clamp(1.0 - g.w, 0.0, 1.0);
}

// ---------------------------------------------------------------- passe : relecture des captures, envoi des sondes
class SondesIbl : public PasseRendu {
public:
    const char* nom() const override { return "sondes IBL (relecture, envoi)"; }
    EtapeRendu etape() const override { return EtapeRendu::Ecran; }
    const char* cibles() const override { return "lit la cubemap de la capture d'environnement ; écrit les cubes préfiltrés des sondes"; }
    bool active(const ImagePasse&) const override { return !g.desactive && (g.imageActive || g.relecture || g.enCours); }
    void detruire(Renderer& r) override { detruireTout(AccesRendu::vk(r)); }
    void executer(const ImagePasse& img) override {
        VkCtx& vk = AccesRendu::vk(img.r);
        // 1. pré-exposition de chaque face rendue (la capture dure 6 images : k peut changer d'une face à l'autre)
        const int face = environnement::faceSuivante(), captures = environnement::capturesFaites();
        const double k = img.ubo.iblB.x > 0 ? img.ubo.iblB.x : 1.0;
        if (captures > g.capturesVues) {
            g.kFace[5] = k; g.capturesVues = captures; g.faceVue = -1;
            // seulement une capture demandée ici (pas celle d'un autre banc), au point demandé
            // WFS-203 : et dans la scène en cours : une capture demandée avant remettreAZero (course de jour, puis course de nuit
            // au même départ) a des faces rendues dans l'autre scène ; relue, elle donnait à la nuit des luminances de jour
            // (≈ 2600 cd/m² au lieu de ≈ 0,1) : exposition bloquée à EV100 5, image blanche. Elle est abandonnée, une autre est demandée.
            if (!g.relecture && environnement::image() && g.imageActive && length(environnement::point() - g.dernierPoint) < 0.01f &&
                g.generationDemande == g.generation) { lancerRelecture(vk, img); for (int f = 0; f < 6; f++) g.kRelu[f] = g.kFace[f]; }
            for (double& x : g.kFace) x = -1;   // WFS-213 : la capture suivante enregistre ses propres k
        } else if (face > 0 && face != g.faceVue) { g.kFace[face - 1] = k; g.faceVue = face; }
        // 2. relecture lue deux images plus tard (barrière de cette image attendue par Renderer::beginFrame)
        if (g.relecture && img.image == g.relectureImage && --g.relectureAttente <= 0) lireRelecture(vk);
        // 3. sonde traitée : envoi
        if (g.enCours && g.fini.load()) {
            g.enCours = false; g.fini = false;
            if (g.res.generation != g.generation) { g.res = Resultat{}; return; }   // scène changée pendant le traitement
            // WFS-213 (RG-02) : sonde non finie ou absurde (luminance moyenne hors de 10⁻⁷ à 10⁵ cd/m², disque du soleil exclu des
            // captures) : abandonnée, la dernière valeur juste est gardée, une autre capture est demandée
            const double Lmoy = g.res.echelle > 0 ? 1.0 / g.res.echelle : -1;
            if (!luminanceValide(Lmoy)) {
                WFS_ERR("éclairage (WFS-213) : sonde abandonnée (luminance moyenne %.3g cd/m²)", Lmoy);
                g.res = Resultat{}; g.aucuneDemande = true; return;
            }
            Buffer t; Sonde s = envoyer(vk, img.cb, g.res, t); retirer(Image{}, t);
            const bool premiere = !g.B.capture;
            g.etat.msTraitement = g.res.ms; g.etat.sondesCapturees++;
            // capture éclairée par une sonde lointaine (ou celle du ciel) : la même place est recapturée, éclairée cette fois par
            // elle-même (un rebond de plus à chaque fois, 3 au plus) ; dans un tunnel, la lumière du dehors s'éteint ainsi par ρ³
            if (g.res.eclaireLoin && g.iterations < 2) { g.forcerCapture = true; g.iterations++; g.dernierPoint = g.res.pos; } else g.iterations = 0;
            WFS_LOG("éclairage (WFS-151) : sonde %d (%d px, %.1f ms CPU) en (%.1f %.1f %.1f), rebond %d, EV100 %.2f", g.etat.sondesCapturees, g.res.cote, g.res.ms,
                    g.res.pos.x, g.res.pos.y, g.res.pos.z, g.iterations, g.ev);
            installer(std::move(s), premiere);
            // WFS-322 (ANO-174) : exposition et diffus recalés d'un coup à la 1re capture d'une nouvelle scène seulement (carte, moment,
            // voiture : sous le chargement et le décompte) ; après un saut dans la même scène (plus de 80 m), ils suivent en douceur
            if (premiere && g.sceneNeuve) { g.recaler = true; g.shInit = true; }
            if (premiere) g.sceneNeuve = false;
            g.res = Resultat{};
        }
    }
private:
    void lancerRelecture(VkCtx& vk, const ImagePasse& img) {
        const int N = environnement::taille(); if (N <= 0) return;
        vk.destroy(g.tampon);
        g.tampon = vk.createBuffer((VkDeviceSize)N * N * 8 * 6, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true);
        VkImageMemoryBarrier b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        b.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; b.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        b.srcQueueFamilyIndex = b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; b.image = environnement::image();
        b.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6};
        b.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; b.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        vkCmdPipelineBarrier(img.cb, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
        VkBufferImageCopy c{}; c.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 6}; c.imageExtent = {(uint32_t)N, (uint32_t)N, 1};
        vkCmdCopyImageToBuffer(img.cb, b.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, g.tampon.buf, 1, &c);
        std::swap(b.oldLayout, b.newLayout); b.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT; b.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        vkCmdPipelineBarrier(img.cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &b);
        g.relecture = true; g.relectureImage = img.image; g.relectureAttente = 2; g.coteRelu = N; g.posRelue = environnement::point();
        g.generationRelue = g.generation; g.loinRelu = g.eclaireLoin;
    }
    void lireRelecture(VkCtx& vk) {
        g.relecture = false;
        if (g.enCours) { vk.destroy(g.tampon); return; }   // une sonde déjà en traitement : celle-ci est abandonnée
        const int N = g.coteRelu;
        auto cube = std::make_shared<CubeF>();
        int n = 1; while ((N >> (n - 1)) > 1) n++;
        cube->allouer(N, n);
        const uint16_t* s = static_cast<const uint16_t*>(g.tampon.mapped);
        if (!kFacesValides(g.kRelu)) {   // WFS-213 (RG-02) : une face sans exposition connue
            WFS_ERR("éclairage (WFS-213) : capture abandonnée (face sans exposition connue)");
            vk.destroy(g.tampon); g.aucuneDemande = true; return;
        }
        for (int f = 0; f < 6; f++) {
            const float inv = (float)(1.0 / g.kRelu[f]);
            float* o = cube->face(0, f).data();
            for (size_t i = 0; i < (size_t)N * N; i++) for (int q = 0; q < 3; q++) o[i * 3 + q] = passes::demiVersFloat(s[((size_t)f * N * N + i) * 4 + q]) * inv;
        }
        vk.destroy(g.tampon);
        g.enCours = true; g.fini = false;
        const vec3 pos = g.posRelue; const int gen = g.generationRelue; const bool loin = g.loinRelu;
        if (synchrone() && std::getenv("WFS_DIAG_SONDE")) diagnostiquerSonde(*cube, pos);   // WFS-322 (RG-01) : part ciel / surfaces
        if (synchrone()) { g.res = traiter(*cube, pos, true, true); g.res.generation = gen; g.res.eclaireLoin = loin; g.fini = true; }
        else jobs().run([cube, pos, gen, loin] { g.res = traiter(*cube, pos, true, false); g.res.generation = gen; g.res.eclaireLoin = loin; g.fini = true; });
    }
};
WFS_PASSE(SondesIbl, 10)   // étape Ecran, après la capture d'environnement (ordre 0)

// exposition automatique : histogramme pondéré des cellules des sondes (A et B selon le mélange), centre = vue
double mesurerLuminance(const vec3& avant) {
    std::vector<Cellule> c;
    auto ajouter = [&](const Sonde& s, double w) {
        if (!s.ok || w <= 1e-4) return;
        for (const CelluleDir& x : s.carte) { const double d = std::max(0.f, dot(x.d, avant)); c.push_back({x.log2L, w * x.omega * (0.2 + 0.8 * d * d)}); }
    };
    if (g.B.capture) { ajouter(g.A.capture ? g.A : g.B, 1 - g.w); ajouter(g.B, g.w); } else ajouter(g.sondeCiel, 1);
    return c.empty() ? 0 : moyenneHistogramme(c);
}

int espacement(int niveau) { return niveau >= qualite::ULTRA ? 40 : niveau >= qualite::HAUTE ? 60 : niveau >= qualite::MOYENNE ? 100 : 200; }

} // namespace

const Etat& etat() { return g.etat; }
// WFS-230 (RG-04) : exposition k pour la vision de nuit de la passe finale (post.frag) ; 0 de jour ou sans éclairage physique
// WFS-276 : signe = relèvement des basses lumières (post.frag) : + Campagne, Corniche, Grand Prix ; − Ville éclairée (inchangée, RG-04)
float kVisionNuit() { return g.imageActive && !g.desactive && g.etat.actif && g.etat.moment == (int)atmosphere::Moment::Nuit ? (float)(g.releverNuit ? g.k : -g.k) : 0.f; }   // heure bleue : inchangée (RG-05)
float lAdaptationNuit() { return kVisionNuit() != 0.f ? (float)luminanceAdaptationNuit(g.ev) : 0.f; }   // WFS-214 (RG-05) : vision de nuit loin des lumières seulement
void forcer(const Forcage& f) { g.forc = f; }
void choisirMoment(int m) {   // WFS-208 ; m < 0 : aucun choix (sauvegarde, sinon midi / nuit selon l'ambiance de la carte)
    g.momentChoisi = m >= 0; g.moment = m >= 0 ? (atmosphere::Moment)std::min(m, (int)atmosphere::Moment::NB - 1) : atmosphere::Moment::Midi;
    g.momentLu = false; g.ecoule = 0; g.heureMoment[(int)atmosphere::Moment::Reel] = -100;
}
int momentChoisi() { return (int)g.moment; }
void accelererTemps(double f) { g.acceleration = std::max(0.0, f); }   // WFS-208 (RG-02)
void desactiver(bool oui) { g.desactive = oui; if (oui) g.imageActive = false; }
void suspendreCaptures(bool oui) { g.capturesSuspendues = oui; }   // WFS-222
bool sondeCapturePrete() { return g.B.capture && !g.forcerCapture && !g.enCours && !g.relecture && !environnement::enCours(); }
void remettreAZero() {
    g.recaler = g.sceneNeuve || !g.evInit;   // WFS-322 : saut dans la même scène : l'exposition s'adapte (pas de recalage d'un coup)
    g.aucuneDemande = true; g.typePrec = -1; g.forcerCapture = false; g.generation++; g.iterations = 0;
    if (g.A.gpu.img && g.A.gpu.img != g.B.gpu.img) retirer(g.A.gpu, Buffer{});
    if (g.B.gpu.img) retirer(g.B.gpu, Buffer{});
    g.A = Sonde{}; g.B = Sonde{};
    g.cielAFaire = true; g.w = 1;
    g.shInit = true;   // WFS-322 : remise à zéro (saut de plus de 80 m) : lumière reprise sans fondu
}

void irradiance(const vec3& n, double E[3]) {
    double a[3] = {0, 0, 0}, b[3] = {0, 0, 0};
    const Sonde& B = g.B.capture ? g.B : g.sondeCiel;
    irradianceSh9(B.sh, n.x, n.y, n.z, b);
    const double w = g.B.capture && g.A.capture ? g.w : 1.0;
    if (w < 1) irradianceSh9(g.A.sh, n.x, n.y, n.z, a);
    for (int q = 0; q < 3; q++) E[q] = a[q] * (1 - w) + b[q] * w;
}

// WFS-208 (RG-04) : couverture nuageuse de la carte (part du ciel couverte, été méditerranéen : littoral plus dégagé)
// WFS-230 (RG-01) : éclairement horizontal reçu au point p par les lampadaires de la carte (copie la plus proche du joueur de
// chaque lampadaire du tour, à moins de 160 m : au-delà, moins de 0,02 lx) : pilote la borne basse de l'exposition de nuit
double eclairementLampes(const GameView& gv, vec3 p) {
    if (!gv.track || lampadaires().empty()) return 0;
    Track& t = *gv.track;
    // WFS-328 (RG-05, ANO-179) : distance parcourue (dist), pas l'abscisse ramenée au tour (position) : le point regardé p est sur
    // la route du tour en cours (camera_vue.cpp : track.point(dist)), et la route de chaque tour est tournée par rapport au
    // précédent (WFS-268) ; avec position, les lampes étaient cherchées sur le 1er tour, loin de la voiture dès le 2e tour : 0,1 lx
    // reçus (lune seule), exposition de rase campagne (EV −3,5) en pleine ville, image blanche / rose jusqu'à la fin de la course
    const double L = t.length(), z = gv.player ? gv.player->dist : 0;
    if (!(L > 0)) return 0;
    double E = 0;
    for (const Lampadaire& l : lampadaires()) {
        double s = (double)l.seg * SEG;
        s += std::round((z - s) / L) * L;
        if (std::fabs(s - z) * M > 160) continue;
        const TrackPoint q = t.point(s, l.lat);
        E += eclairementLampe(E_LAMPE_SOL, std::hypot(q.x - p.x, q.z - p.z), l.y);
    }
    return E;
}

double couvertureNuages(const GameView& gv) {
    const char* id = gv.map ? gv.map->id : "";
    return !std::strcmp(id, "plage") ? 0.15 : !std::strcmp(id, "ville") ? 0.25 : !std::strcmp(id, "circuit") ? 0.30 : 0.35;
}

void physique(FrameParams& fp, bool dediee, const GameView& gv) {
    g.imageActive = false;
    g.releverNuit = !(gv.map && !std::strcmp(gv.map->id, "ville"));   // WFS-276 (RG-04)
    if (dediee || g.desactive) { g.etat.actif = false; return; }
    // ---- scène : temps, remise à zéro (changement de carte, de moment, téléportation)
    const double t = fp.time;
    double dt = g.tPrec < 0 ? 0 : t - g.tPrec;
    const vec3 point = g.forc.actif && g.forc.pointForce ? g.forc.point : fp.hasFocus ? fp.focus + vec3{0, 1.5f, 0} : fp.camPos;
    // WFS-213 (RG-01) : nouvelle scène aussi quand la piste, la carte ou la voiture change (init, changement de carte ou de voiture) :
    // sondes oubliées et exposition remise à la valeur du moment, recalée sur la première mesure valide
    const bool autreScene = gv.track != g.pistePrec || gv.map != g.cartePrec || gv.car != g.voiturePrec;
    // WFS-328 (RG-02) : « Recommencer » tôt dans la course ramène la voiture de moins de 80 m : un recul de plus de 20 m sur la piste
    // en une image (impossible en roulant, même en marche arrière) est aussi une nouvelle scène
    const double zPiste = gv.player ? gv.player->dist : 0;   // distance parcourue (position revient à 0 à chaque tour)
    const bool recul = g.init && gv.track == g.pistePrec && (g.zPrec - zPiste) * M > 20;
    g.zPrec = zPiste;
    if (!g.init || dt < 0 || fp.night != g.nuitPrec || length(point - g.pointPrec) > 80.f || autreScene || recul) {
        if (!g.init || autreScene || fp.night != g.nuitPrec) g.ev = fp.night ? EV_REF_NUIT : EV_REF_JOUR;
        // WFS-328 (RG-02) : nouvelle scène et « Recommencer » recalés dès leur 1re image ; WFS-322 : un saut en avant dans la même
        // scène (plus de 80 m) garde l'adaptation douce
        if (!g.init || autreScene || fp.night != g.nuitPrec || recul) { g.evInit = false; g.sceneNeuve = true; }
        remettreAZero(); g.init = true; dt = 0; g.tScene = t;
    }
    g.pistePrec = gv.track; g.cartePrec = gv.map; g.voiturePrec = gv.car;
    dt = std::clamp(dt, 0.0, 0.25);
    g.tPrec = t; g.pointPrec = point; g.nuitPrec = fp.night;
    const bool nuit = fp.night;
    // ---- RG-01 : soleil (lune la nuit) en lux
    // WFS-207 (RG-01) : astre calculé pour le lieu et la date fictifs (atmosphere_physique.h) : soleil de la NOAA le jour
    // (heure forcée par le banc possible), lune de Meeus la nuit (sa fraction éclairée règle son éclairement)
    // WFS-208 (RG-01, RG-02) : moment choisi (sinon celui de la sauvegarde) ; l'ambiance de la carte (jour / nuit) l'emporte
    // si elle le contredit ; l'heure avance avec la course (× accélération, × 1 par défaut) ; heure réelle : horloge UTC
    if (!g.momentChoisi && !g.momentLu && sauvegarde().actif()) {
        g.moment = atmosphere::momentDepuisNom(sauvegarde().moment.empty() ? sauvegarde().tod.c_str() : sauvegarde().moment.c_str()); g.momentLu = true;
    }
    atmosphere::Moment mom = g.moment;
    if (atmosphere::momentDeNuit(mom) != nuit) mom = nuit ? atmosphere::Moment::Nuit : atmosphere::Moment::Midi;
    const bool lune = mom == atmosphere::Moment::Nuit;
    g.ecoule += dt * g.acceleration / 3600.0;
    double& hm = g.heureMoment[(int)mom];
    if (hm < -50) {
        if (mom == atmosphere::Moment::Reel) { const auto n = std::chrono::system_clock::now().time_since_epoch(); hm = std::fmod(std::chrono::duration<double>(n).count() / 3600.0, 24.0); }
        else hm = atmosphere::heureMoment(mom);
    }
    const double heure = g.forc.actif && g.forc.heureUtc > -50 ? g.forc.heureUtc : hm + g.ecoule;
    const atmosphere::Astre astre = lune ? atmosphere::lune(heure) : atmosphere::soleil(heure);
    vec3 s = !lune && g.forc.actif && g.forc.azimutSoleil > -500 ? atmosphere::directionMonde(astre.hauteur, g.forc.azimutSoleil) : astre.dir;
    if (g.forc.actif && g.forc.hauteurSoleil > -90) {
        vec3 hz{s.x, 0, s.z}; hz = length(hz) > 1e-4f ? normalize(hz) : vec3{1, 0, 0};
        const float h = (float)(g.forc.hauteurSoleil * PI_L / 180.0);
        s = hz * std::cos(h) + vec3{0, std::sin(h), 0};
    }
    const double h = std::asin(std::clamp((double)s.y, -1.0, 1.0)) * 180.0 / PI_L;
    const bool crep = !lune && h <= 0;   // WFS-208 : soleil sous l'horizon (aube, heure bleue) : ciel du crépuscule, pas de soleil direct
    const double E = lune ? E_PLEINE_LUNE * astre.fraction * (astre.hauteur > 0 ? 1.0 : 0.0) : crep ? 0.0 : eclairementSoleilNormal(h);
    // WFS-207 (RG-02) : atmosphère calculée quand l'astre change de place (cache) : couleur du soleil au sol (transmittance),
    // teintes du ciel au zénith et à l'horizon, extinction au sol (brouillard)
    // crépuscule : teintes de l'atmosphère au lever / coucher (soleil ramené à 0,5°, même azimut ; le soleil sous l'horizon n'éclaire plus le modèle)
    const vec3 sAtmo = !lune && h < 0.5 ? atmosphere::directionMonde(0.5, astre.azimut) : s;
    if (length(sAtmo - g.astreCache) > 1e-5f || !g.cielCalcule) { g.astreCache = sAtmo; g.atmo = atmosphere::ciel(sAtmo); g.cielCalcule = true; }
    const atmosphere::Ciel& A = g.atmo;
    double coul[3];
    if (lune || (nuit && crep)) couleurTemperature(T_PLEINE_LUNE, coul);
    else { const double y = std::max(luminanceRgb(A.soleil), 1e-9); for (int q = 0; q < 3; q++) coul[q] = A.soleil[q] / y; }
    const vec3 couleur{(float)coul[0], (float)coul[1], (float)coul[2]};
    // crépuscule raccordé au ciel diffus au coucher (aucun saut, RG-02) : valeurs publiées × E_ciel(0°) / 750 lx
    // fondu entre le ciel du jour (CIE) et le ciel uniforme du crépuscule de −1° à +2° : aucune bascule au coucher (RG-02)
    const double hj = std::max(h, 0.05), u = std::clamp((h + 1.0) / 3.0, 0.0, 1.0), w0 = u * u * (3 - 2 * u);
    const double wj = lune ? 0.0 : w0 > 0.999 ? 1.0 : w0 < 0.001 ? 0.0 : w0;   // poids du ciel du jour (bords francs : pas de reste numérique)
    // WFS-214 (RG-02, condition 3 du PO) : soleil sous l'horizon : crépuscule et nuit inchangés ; le ciel entier de Krochmann ne s'applique
    // qu'au jour : raccord au niveau d'avant (× 6 000 / 15 825), le fondu de −1° à +2° reste continu
    const double Ecrep = atmosphere::eclairementCrepuscule(std::min(h, 0.0)) * eclairementCielDiffus(0.001) * (E_CIEL_CLAIR_MIDI / E_CIEL_PLEIN_MIDI) / 750.0;
    const double Ed = lune ? E_NUIT_SANS_LUNE : wj * eclairementCielDiffus(hj) + (1 - wj) * Ecrep;
    if (std::fabs(hj - g.hCache) > 1e-4) { g.hCache = hj; g.kCache = cieFacteur(hj); }
    const double K = lune ? 0 : g.kCache * wj, Lnuit = lune ? E_NUIT_SANS_LUNE / PI_L : (1 - wj) * Ecrep / PI_L;
    // ---- RG-02 : exposition (mesure sur les sondes, bornée par moment, adaptation de l'œil)
    g.avant = normalize(vec3{-fp.view.at(2, 0), -fp.view.at(2, 1), -fp.view.at(2, 2)});
    // WFS-328 (ANO-177, RG-01) : avant la première sonde locale d'une scène, la mesure lisait la sonde du ciel : celle de la scène
    // d'avant à la première image (jour du menu : 3 458 cd/m²), puis le seul ciel de nuit sans lampadaires (0,025 cd/m²) :
    // exposition 1,3 IL plus claire d'un coup pendant le décompte au banc (image blanche / rose sur le téléphone) jusqu'à la capture (≈ 3 s).
    // La nuit, la mesure réfléchie attend donc la sonde locale (la valeur du moment, EV_REF_NUIT, la remplace, mêlée au
    // posemètre incident des lampadaires, connu dès la 1re image) ; de jour, elle attend que la sonde du ciel soit à jour.
    const bool mesureSure = g.B.capture || (!nuit && !g.cielAFaire);
    double Lmes = mesureSure ? mesurerLuminance(g.avant) : 0;
    if (!luminanceValide(Lmes)) Lmes = 0;   // WFS-213 (RG-02) : mesure invalide : la dernière exposition juste est gardée
    double cible = Lmes > 0 ? ev100Luminance(Lmes) : g.B.capture ? g.ev : nuit ? EV_REF_NUIT : EV_REF_JOUR;
    // WFS-208 : bornes selon la hauteur du soleil : crépuscule EV 3 à 15, heure dorée 8 à 15, jour 10 à 15, nuit 3 à 5
    const double evMin = lune ? EV_NUIT_MIN : h < -1 ? EV_NUIT_MIN : h < 6 ? 8.0 : (nuit ? EV_NUIT_MIN : EV_JOUR_MIN), evMax = lune ? EV_NUIT_MAX : EV_JOUR_MAX;
    // WFS-230 (RG-01) : nuit au clair de lune : l'exposition dépend de l'éclairement reçu par le conducteur (lune, ciel et lampadaires
    // proches). Loin des lampes, posemètre incident (evIncidentNuit) jusqu'à −3,5 (WFS-249).
    // La mesure réfléchie des sondes n'est pas prise la nuit : la tache des phares la tirait vers EV 2 (essai du 2026-11-07 :
    // 0,49 cd/m² en rase campagne) et le paysage restait noir. Passage continu par l'adaptation de l'œil (RG-02).
    // Rue éclairée (≥ 3 lx reçus) : bornes et mesure d'avant (EV 3 à 5, RG-01) ; rase campagne (≤ 0,63 lx) : posemètre incident
    // (jusqu'à −3,5) ; entre les deux, fondu en EV (partRue).
    double evMinLu = evMin;
    if (lune) {
        g.eRecu = E * std::max(std::sin(h * PI_L / 180.0), 0.0) + Ed + eclairementLampes(gv, point);
        // WFS-356 (RG-01, ANO-202) : dans la Ville, éclairée partout, l'exposition reste celle d'une rue éclairée (EV 3 à 5) ; avant,
        // 1,57 lx de lampadaires au départ donnaient EV 2,27 pendant tout le décompte (façades en blocs blancs) : partRueNuit
        const double w = partRueNuit(g.eRecu, !g.releverNuit);
        cible = w * std::clamp(cible, EV_NUIT_MIN, evMax) + (1 - w) * evIncidentNuit(g.eRecu);
        evMinLu = EV_LUNE_MIN;
    }
    cible = std::clamp(cible, evMinLu, evMax);
    if (g.forc.actif && g.forc.ev > -50) cible = g.forc.ev;
    // recalage sans adaptation : exposition fixe (banc), première image, nouvelle scène jusqu'à sa première capture
    // WFS-328 (RG-02, RG-03) : la nuit, l'image tourne déjà sur l'estimation (valeur du moment) quand la première sonde arrive :
    // passage doux vers la mesure (adaptation de l'œil, pas de saut visible) ; à temps arrêté (banc) ou de jour, recalage direct
    const bool doux = g.nuitEstimee && dt > 0;
    if ((g.forc.actif && g.forc.ev > -50) || !g.evInit || (g.recaler && Lmes > 0 && !doux)) { g.ev = cible; g.evInit = true; if (g.B.capture) g.recaler = false; }
    else { g.ev = adapter(g.ev, cible, dt); if (doux && g.B.capture) g.recaler = false; }
    g.nuitEstimee = nuit && !mesureSure;
    if (!std::isfinite(g.ev)) g.ev = std::clamp(nuit ? EV_REF_NUIT : EV_REF_JOUR, evMin, evMax);   // WFS-213 (RG-02) : jamais de valeur non finie
    g.dtImage = dt;
    const double k = exposition(g.ev);
    const double expOrig = fp.exposure;
    // WFS-230 (RG-03) : adaptation locale de l'œil : les lumières artificielles (phares, lampadaires, néons, halos, voile des villes)
    // ne suivent l'exposition que jusqu'à EV 3 (rue éclairée) ; en dessous (clair de lune), elles restent comme à EV 3 : le paysage
    // se lit au clair de lune sans que les phares et les lampes ne deviennent « tout blanc » (Thompson, Shirley et Ferwerda 2002 :
    // l'œil adapté à la nuit ne voit pas les lampes brûlées). De jour (EV ≥ 10), rien ne change (RG-05).
    const double kLumieres = lune ? exposition(std::max(g.ev, EV_LUMIERES_MIN)) : k;   // heure bleue et jour : inchangés (RG-05)
    const double facteur = expOrig * kLumieres / exposition(nuit ? EV_REF_NUIT : EV_REF_JOUR);
    // ---- ciel, sol, brouillard (cd/m²)
    // WFS-207 (RG-02) : de jour, teintes du ciel tirées de l'atmosphère (zénith, horizon) ; la nuit, celles de la carte (lot 2)
    auto teinte = [](const double c[3]) { const double y = std::max(luminanceRgb(c), 1e-12); return vec3{(float)(c[0] / y), (float)(c[1] / y), (float)(c[2] / y)}; };
    const vec3 haut0 = nuit ? fp.skyTop : teinte(A.zenith), horizon0 = nuit ? fp.skyHorizon : teinte(A.horizon);
    const double Esol = E * std::max(std::sin(h * PI_L / 180.0), 0.0) + Ed;
    const vec3 sol = fp.ground * (float)(Esol / PI_L);   // couleur du sol du ciel = albédo
    vec3 brume;
    if (!nuit) {   // brouillard de jour : teinte du jeu, luminance moyenne du ciel à 5° de hauteur (modèle) ; WFS-208 : + ciel du crépuscule
        double L5 = 0; const double z = 85 * PI_L / 180; const vec3 sc = atmosphere::directionMonde(hj, astre.azimut);
        for (int j = 0; j < 16; j++) { const double a = 2 * PI_L * (j + 0.5) / 16, d[3] = {std::sin(z) * std::cos(a), std::cos(z), std::sin(z) * std::sin(a)}, ss[3] = {sc.x, sc.y, sc.z}; L5 += K * cieRelative(d, ss) / 16; }
        brume = horizon0 * (float)(L5 + Lnuit);   // WFS-207 (RG-03) : perspective aérienne au loin = couleur de l'horizon de la même atmosphère
        // brouillard de distance physique ; un brouillard linéaire (pluie de meteo.cpp) reste tel quel
        // WFS-381 (RG-01, RG-02, RG-04, ANO-221) : l'extinction au sol du modèle de ciel (A.extY ≈ 8,7·10⁻⁵ m⁻¹ : 8 % à 1 km) ne
        // donnait presque aucune perspective atmosphérique ; brume mesurée de la carte (couche limite chargée d'aérosols), qui
        // décroît avec la hauteur (atmosphere_brume.h) ; jamais moins que l'extinction du ciel lui-même
        if (fp.fogFar <= 0) {
            const wfs::brume::Reglage b = wfs::brume::reglageCarte(gv.map ? gv.map->id : nullptr);
            fp.fogDensity = (float)std::max(b.beta0, A.extY); fp.fogHauteurInv = (float)(1.0 / b.hauteur); fp.fogRef = (float)b.yRef;
        }
    } else brume = fp.fogColor * (float)(facteur / k);   // nuit : lumière des villes diffusée (intensité artistique, RG-06)
    // sonde du ciel (CPU) quand le ciel change
    Etat_::Ciel c{nuit, K, Lnuit, haut0, horizon0, sol, brume, s};
    const float sig[16] = {(float)K, (float)Lnuit, haut0.x, haut0.y, haut0.z, horizon0.x, horizon0.y, horizon0.z, sol.x, sol.y, sol.z, brume.x, brume.y, brume.z, s.x, s.y + s.z};
    if (g.cielAFaire || std::memcmp(sig, g.signatureCiel, sizeof sig) != 0) { std::memcpy(g.signatureCiel, sig, sizeof sig); g.ciel = c; g.cielAFaire = true; }
    // ---- fp pré-exposé (RG-01, RG-04, RG-06)
    fp.sunDir = s; fp.sunIntensity = (float)(E * k); fp.sunColor = couleur;
    fp.ground = sol * (float)k; fp.fogColor = brume * (float)k;
    fp.exposure = 1; fp.ambient = 1;
    if (!nuit) { fp.skyTop = haut0; fp.skyHorizon = horizon0; }   // WFS-207 (RG-02) : teintes du ciel de l'atmosphère (sky.frag)
    g.etat.eLampadaireSol = -1;
    for (int i = 0; i < std::clamp(fp.spotCount, 0, FrameParams::SPOTS_MAX); i++) {
        FrameParams::Spot& sp = fp.spots[i];
        sp.intensity *= (float)facteur;
        // RG-06 : éclairement au sol sous le premier lampadaire (lumière ponctuelle, tête à la hauteur déclarée par le décor) :
        // E = intensité × π × (1 − h / portée)^décroissance × luminance de la couleur (Renderer::beginFrame, three.js r128) ÷ k
        if (g.etat.eLampadaireSol < 0 && sp.angle >= PI - 1e-3f && sp.distance > 0 && !lampadaires().empty()) {
            const double hl = lampadaires().front().y, c[3] = {sp.color.x, sp.color.y, sp.color.z};
            g.etat.eLampadaireSol = sp.intensity * PI_L * std::pow(std::max(0.0, 1 - hl / sp.distance), sp.decay) * luminanceRgb(c) / k;
        }
    }
    g.k = k; g.facteur = facteur;
    g.cielP = {(float)(K * k), (float)(Lnuit * k), (float)std::min(E / angleSolideSoleil() * k, 60000.0), lune ? (float)atmosphere::rad(astre.phase) : (g.forc.actif && g.forc.sansNuages ? 0.f : (float)couvertureNuages(gv))};   // WFS-208 : w = phase de la lune, ou couverture nuageuse de jour   // WFS-207 : w = angle de phase de la lune
    // ---- RG-03 / ST-4 : sondes locales
    const int niveau = qualite::niveauAffiche(ecrans::reglages().qualite);
    double esp = espacement(niveau);
    int type = -1;
    if (gv.track) {
        const double z = g.forc.actif && g.forc.abscisseForcee ? g.forc.abscisse : gv.player ? gv.player->position : 0;
        if (const trace::Section* sec = trace::sectionA(*gv.track, z)) type = (int)sec->type;
    }
    const bool ouvrage = type == (int)trace::Type::Tunnel || type == (int)trace::Type::Pont;
    if (ouvrage) esp = std::min(esp, 40.0);
    if (type != g.typePrec && g.typePrec != -1 && (ouvrage || g.typePrec == (int)trace::Type::Tunnel || g.typePrec == (int)trace::Type::Pont)) g.forcerCapture = true;
    g.typePrec = type;
    // WFS-222 (RG-03) : captures suspendues par la baisse automatique, sauf la toute première (aucune sonde encore)
    // WFS-264 (RG-01) : en Basse, une capture par zone seulement : la première (départ, sous le chargement et le décompte) et
    // à chaque changement de zone (entrée et sortie de tunnel ou de pont : forcerCapture) ; plus de capture tous les « esp » m
    const bool parZone = niveau == qualite::BASSE;
    const bool retardee = g.forc.actif && t - g.tScene < g.forc.retardCapture;   // WFS-328 : banc, sonde lente comme sur le téléphone
    if (!retardee && !environnement::enCours() && !g.relecture && !g.enCours && (!g.capturesSuspendues || !g.B.capture)) {
        if (g.aucuneDemande || g.forcerCapture || (!parZone && length(point - g.dernierPoint) > esp)) {
            // la capture est éclairée par les sondes en place : si elles sont loin (ou si c'est la sonde du ciel), la place
            // sera recapturée (rebonds) ; une recapture forcée garde le point de la précédente
            const vec3 p = g.forcerCapture && g.iterations > 0 ? g.dernierPoint : point;
            g.eclaireLoin = !g.B.capture || length(g.B.pos - p) > 2.5f * (float)esp;
            environnement::demander(p); g.dernierPoint = p; g.aucuneDemande = false; g.forcerCapture = false; g.generationDemande = g.generation;
        }
    }
    g.imageActive = true;
    // ---- état (banc, journal)
    Etat& e = g.etat;
    e.actif = true; e.nuit = nuit; e.hauteurSoleil = h; e.eSoleilNormal = E; e.eSoleilHorizontal = E * std::max(std::sin(h * PI_L / 180.0), 0.0);
    e.eCielDiffus = Ed; e.facteurCie = K; e.luminanceZenith = nuit ? Lnuit : K * cieF((90 - h) * PI_L / 180) * ciePhi(0);
    e.ev = g.ev; e.evCible = cible; e.k = k; e.facteurArtistique = facteur; e.couleurSoleil = couleur;
    e.astre = s; e.astreHauteur = h; e.astreAzimut = astre.azimut; e.astrePhase = lune ? astre.phase : 0;   // WFS-207
    e.moment = (int)mom; e.heureUtc = heure; e.eSol = E * std::max(std::sin(h * PI_L / 180.0), 0.0) + Ed;   // WFS-208
    e.eRecu = lune ? g.eRecu : -1; e.evMin = evMinLu;   // WFS-230
    e.sondeCaptureActive = g.B.capture; e.positionSonde = g.B.pos; e.poidsB = g.w; e.luminanceMesuree = Lmes;
}

void avantImage(VkCtx& vk, FrameUBO& u, VkDescriptorSet image, VkDescriptorSet reflet) {
    creerFixes(vk);
    for (size_t i = 0; i < g.retraits.size();) {   // ressources retirées : détruites quand plus aucune image en vol ne les lit
        if (--g.retraits[i].images <= 0) { vk.destroy(g.retraits[i].img); vk.destroy(g.retraits[i].tampon); g.retraits.erase(g.retraits.begin() + (long)i); }
        else i++;
    }
    const bool actif = g.imageActive && !g.desactive;
    if (actif && g.cielAFaire) {   // sonde du ciel : calcul et envoi immédiats (rares : changement de carte ou de moment)
        g.cielAFaire = false;
        Resultat r = sondeDuCiel(g.ciel);
        VkCommandBuffer cb = vk.beginOneShot(); Buffer t;
        Sonde s = envoyer(vk, cb, r, t);
        vk.endOneShot(cb); vk.destroy(t);
        if (g.sondeCiel.gpu.img) retirer(g.sondeCiel.gpu, Buffer{});
        g.sondeCiel = std::move(s);
    }
    // sondes lues par les shaders : captures A et B, sinon la sonde du ciel
    const Sonde& B = g.B.capture ? g.B : g.sondeCiel;
    const Sonde& A = g.B.capture && g.A.capture ? g.A : B;
    const double w = &A == &B ? 1.0 : g.w;
    if (g.B.capture && g.A.capture && g.imageActive) {   // mélange par distance (lissé 0,3 s)
        const vec3 p = g.pointPrec; const double dA = length(p - g.A.pos), dB = length(p - g.B.pos);
        const double cible = dA + dB > 1e-3 ? dA / (dA + dB) : 1.0;
        g.w += (cible - g.w) * (1.0 - std::exp(-g.dtImage / 0.3));
        if (g.w > 0.999) g.w = 1;   // une seule lecture du cube dans les shaders hors mélange
    }
    u.iblA = {actif ? 1.f : 0.f, (float)w, (float)std::max(0, A.niveaux - 1), (float)std::max(0, B.niveaux - 1)};
    u.iblB = {(float)g.k, (float)(g.k / std::max(A.echelle, 1e-12f)), (float)(g.k / std::max(B.echelle, 1e-12f)), actif ? (float)g.facteur : 1.f};
    u.cielP = actif ? g.cielP : vec4{};
    {   // WFS-322 (ANO-174) : mélange A / B lissé dans le temps (une nouvelle sonde ne change plus le diffus en une image)
        Sh9 cible;
        for (int i = 0; i < 9; i++) for (int q = 0; q < 3; q++) cible.c[i][q] = A.sh.c[i][q] * (1 - w) + B.sh.c[i][q] * w;
        lisserSh9(g.shAffiche, cible, g.dtImage, SH_TAU_S, g.shInit || !actif);
        g.shInit = false;
    }
    for (int i = 0; i < 9; i++) {
        double c[3];
        for (int q = 0; q < 3; q++) c[q] = g.shAffiche.c[i][q] * SH_A[i] / PI_L * g.k;
        u.iblSH[i] = actif ? vec4{(float)c[0], (float)c[1], (float)c[2], 0} : vec4{};
    }
    // liaisons 3 à 5 des jeux de cette image (leur image précédente est terminée : barrière attendue)
    const VkImageView vA = actif && A.gpu.view ? A.gpu.view : g.noir.view, vB = actif && B.gpu.view ? B.gpu.view : g.noir.view;
    VkDescriptorImageInfo ii[3] = {{g.smp, vA, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}, {g.smp, vB, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
                                   {g.smp, g.lut.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};
    VkWriteDescriptorSet wr[6]{};
    int n = 0;
    for (VkDescriptorSet set : {image, reflet}) {
        if (!set) continue;
        for (uint32_t b = 0; b < 3; b++) {
            VkWriteDescriptorSet& x = wr[n++]; x.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET; x.dstSet = set; x.dstBinding = 3 + b;
            x.descriptorCount = 1; x.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; x.pImageInfo = &ii[b];
        }
    }
    vkUpdateDescriptorSets(vk.device, (uint32_t)n, wr, 0, nullptr);
}

} // namespace eclairage
} // namespace wfs
