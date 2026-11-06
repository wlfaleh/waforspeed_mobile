// WaForSpeed — WFS-148 : banc de référence du réalisme (WFS_BANC=realisme) : voir bench_realisme.h.
//
// 12 vues fixes × jour / nuit × Basse / Haute = 48 captures realisme-<vue>-<moment>-<qualite>.png ; scène d'étalonnage
// realisme-etalon.png (charte ColorChecker 24 cases et charte grise 18 % posées sur la chaussée, soleil du jeu) ;
// realisme-fidelite.txt (rapport de fidélité, indice par domaine) et realisme-mesures.txt (images/s, ms, ms GPU, mémoire).
//
// Le système « RealismeBanc » est inactif hors de ce banc : il ne dessine rien et ne touche à aucune image du jeu
// (RG-08). Pendant le banc, il place la caméra dans le repère de la route au joueur (ambiance(), appelée après toutes les
// ambiances des cartes) et dessine la charte d'étalonnage. Le joueur est posé par WFS_DEPART (lu par les cartes), à
// l'écran Menu (voiture immobile, aucune commande, aucun menu dessiné sans police chargée) ; trafic et piétons sont
// figés : un nombre fixe de pas de simulation, puis plus aucun avant la capture (graines fixes : images reproductibles).
#include "bench_realisme.h"
#include <array>
#include <cstdlib>
#include <sstream>
#include <tuple>
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "../game/cars.h"
#include "../game/game.h"
#include "../game/track.h"
#include "../platform/plantage.h"   // anneau du journal : coût GPU des passes écrit par le Renderer (horodatages Vulkan)
#include "../ui/ecrans/reglages.h"
#include "../world/zones.h"
#include "renderer.h"
#include "ktx2.h"   // WFS-149 : texture d'essai
#include "bench_passes.h"   // WFS-150
#include "../../third_party/stb/stb_image.h"   // implémentation : model_loader.cpp

namespace wfs {
namespace vie { extern double g_premiereTerrasse; }   // src/world/commerces.cpp : abscisse (segments) de la première terrasse
static_assert(RW == 7.5, "mesurerRoute (bench_realisme.h) suppose RW = 7,5 m");

namespace {

// point du repère de la route au joueur : a = mètres devant (le long de la route), l = mètres à droite, h = hauteur
struct Repere { float a = 0, l = 0, h = 0; };
enum class Choix { Depart, LigneDroite, Virage, Terrasse };
struct Vue { const char* nom; const char* carte; const char* zone; Choix choix; Repere oeil, cible; float fov; const char* note; };

// RG-03 : les 12 vues de référence. Choix de cadrage de WFS-148 (à garder identiques d'une US à l'autre : avant / après).
const Vue VUES[] = {
    {"rue-cafes", "ville", nullptr, Choix::Terrasse, {-6, 3.5f, 1.7f}, {22, 11, 1.4f}, 60, "première terrasse de café (côté droit)"},
    {"carrefour", "ville", nullptr, Choix::Depart, {36, -5, 4}, {52, 0, 0}, 60, "la carte Ville n'a pas de carrefour : vue du premier passage piéton (≈ 50 m du départ)"},
    {"ville-large", "ville", nullptr, Choix::Depart, {-40, 0, 45}, {120, 0, 0}, 60, "vue large, 45 m de haut"},
    {"campagne-droite", "campagne", "Champs", Choix::LigneDroite, {-8, 0, 2}, {80, 0, 0.5f}, 60, "ligne droite la plus longue des Champs"},
    {"campagne-virage", "campagne", "Champs", Choix::Virage, {-10, -2, 2.5f}, {40, 0, 0}, 60, "virage le plus serré des Champs"},
    {"corniche-mer", "plage", "Corniche", Choix::Depart, {-8, 0, 3}, {60, 0, 0}, 60, "corniche au bord de mer"},
    {"falaise", "plage", "Côte rocheuse", Choix::Depart, {-10, 0, 6}, {50, 0, 0}, 60, "côte rocheuse (falaise)"},
    {"circuit-droite", "circuit", "Ligne droite des tribunes", Choix::LigneDroite, {-8, 0, 2}, {80, 0, 0.5f}, 60, "ligne droite des tribunes"},
    {"circuit-tribunes", "circuit", "Ligne droite des tribunes", Choix::LigneDroite, {0, 0, 3}, {25, 21, 4}, 60, "tribune de droite (21,5 m du centre)"},
    {"voiture-34-avant", "ville", nullptr, Choix::Depart, {4.5f, 3.2f, 1.25f}, {0, 0, 0.65f}, 50, "Golf du joueur, trois quarts avant"},
    {"pietons", "ville", nullptr, Choix::Terrasse, {-3, 5.5f, 1.6f}, {12, 10, 1.2f}, 55, "trottoir de droite, piétons et clients de la terrasse"},
    {"chaussee-1m5", "ville", nullptr, Choix::Depart, {6, -2.5f, 1.5f}, {16, -2.5f, 0}, 60, "chaussée vue à 1,5 m du sol"},
};
constexpr int PAS_AVANT_CAPTURE = 90;   // pas de simulation (1/60 s) avant la capture : chargement des morceaux, puis figé
// WFS-149 (RG-A, ANO-85) : vues prises sans aucune interface. Écran Chargement : comme le Menu hors course (voiture immobile,
// aucune commande, scène dessinée), mais aucun système n'y dessine d'interface (le Menu dessinait le bouton « RÉGLAGES »)
constexpr Ecran ECRAN_BANC = Ecran::Chargement;
// WFS-149 (RG-01) : texture d'essai KTX2 (tools/textures/essai) : panneau de 3 × 3 m au bord de la route, 8 m devant, et
// bande de 3 m de large sur 80 m de chaussée (1 m de texture par mètre : 4 carreaux de 25 cm) pour juger mipmaps et anisotrope
constexpr const char* ESSAI_TEXTURES[3] = {"textures/essai/essai-couleur", "textures/essai/essai-normale", "textures/essai/essai-orm"};

struct EtatBanc {
    bool actif = false, etalon = false;
    bool essai = false; std::string essaiInfo;   // WFS-149 : panneau de la texture d'essai KTX2 ; variantes chargées (rapport)
    Repere oeil, cible; float fov = 60;
    FrameParams lumiere; bool lumiereLue = false;   // lumière finale de la dernière image (après toutes les ambiances)
    mat4 view = mat4::identity(), proj = mat4::identity();
    std::vector<vec3> albedos;                       // 24 cases puis la charte grise (sRGB linéaire, références/reel)
};
EtatBanc g_banc;

// charte d'étalonnage (RG-04) : 6 × 4 cases de 0,30 m (pas 0,35 m) centrées 10 m devant le joueur, charte grise 0,6 m plus loin
constexpr float CHARTE_A = 10, CASE = 0.30f, PAS = 0.35f, GRISE = 0.6f, GRISE_A = CHARTE_A + 1.25f, SOL = 0.02f;
vec3 casePos(int k) {   // centre de la case k (0..23, ordre de la charte : 4 rangées de 6) ou de la charte grise (k = 24), repère route
    if (k == 24) return {GRISE_A, 0, 0};
    const int col = k % 6, rang = k / 6;
    return {CHARTE_A + 1.5f * PAS - rang * PAS, -2.5f * PAS + col * PAS, 0};   // rangée 1 de la charte en haut de l'image (vers l'avant)
}

vec3 pointRoute(Track& t, double s, const Repere& p) {
    const TrackPoint q = t.point(s + p.a / M, p.l);
    return {(float)q.x, (float)q.y + p.h, (float)q.z};
}

class RealismeBanc : public System {
public:
    const char* name() const override { return "realisme-banc"; }
    void init(Renderer& r, const GameView& g) override {
        if (!g_banc.actif || !g_banc.etalon || g_banc.albedos.size() != 25 || !g.track || !g.player) return;
        const double s = g.player->dist;
        for (int k = 0; k < 25; k++) {
            const vec3 c = casePos(k); const float d = (k == 24 ? GRISE : CASE) / 2;
            std::vector<Vertex> v;
            for (const auto& [da, dl] : {std::pair{-d, -d}, std::pair{-d, d}, std::pair{d, d}, std::pair{d, -d}}) {
                const vec3 p = pointRoute(*g.track, s, {c.x + da, c.y + dl, SOL});
                v.push_back({p.x, p.y, p.z, 0, 1, 0, 0, 0});
            }
            // sens des triangles : face vers le haut, quel que soit le cap de la route (normale comparée à +y)
            const vec3 n = cross(vec3{v[1].px - v[0].px, v[1].py - v[0].py, v[1].pz - v[0].pz}, vec3{v[2].px - v[0].px, v[2].py - v[0].py, v[2].pz - v[0].pz});
            const std::vector<uint32_t> idx = n.y > 0 ? std::vector<uint32_t>{0, 1, 2, 0, 2, 3} : std::vector<uint32_t>{0, 2, 1, 0, 3, 2};
            maillages_[k] = r.makeMesh(v, idx);
            Material m; m.color = {g_banc.albedos[k].x, g_banc.albedos[k].y, g_banc.albedos[k].z, 1};
            m.rough = 0.9f; m.metal = 0;   // charte mate (Physically Based, « Gray Card » : rugosité 0,9)
            mats_[k] = r.makeMaterial(m, &r.white, nullptr, nullptr);
        }
        cree_ = true;
    }
    void ambiance(FrameParams& fp, const GameView& g) override {
        if (!g_banc.actif || !g.track || !g.player) return;
        const double s = g.player->dist;
        const vec3 oeil = pointRoute(*g.track, s, g_banc.oeil), cible = pointRoute(*g.track, s, g_banc.cible);
        // vue de dessus (étalonnage) : le haut de l'image est l'avant de la route
        const vec3 avant = normalize(pointRoute(*g.track, s, {g_banc.cible.a + 1, g_banc.cible.l, g_banc.cible.h}) - cible);
        const bool dessus = std::fabs(normalize(cible - oeil).y) > 0.95f;
        const float aspect = fp.proj.at(0, 0) != 0 ? -fp.proj.at(1, 1) / fp.proj.at(0, 0) : 16.f / 9.f;
        fp.view = lookAt(oeil, cible, dessus ? avant : vec3{0, 1, 0});
        fp.proj = perspectiveVk(g_banc.fov * PI / 180.f, aspect, 0.1f);
        fp.camPos = oeil;
        g_banc.view = fp.view; g_banc.proj = fp.proj; g_banc.lumiere = fp; g_banc.lumiereLue = true;
    }
    void render(Renderer& r, const GameView& g) override {
        if (!g_banc.actif) return;
        // texture d'essai : créée à la première image, quand le joueur est posé au départ demandé (WFS_DEPART)
        if (g_banc.essai && !essaiTente_ && g.track && g.player) { essaiTente_ = true; creerEssai(r, g); }
        const mat4 I = mat4::identity();
        if (essaiCree_) { r.draw(panneau_, essaiMat_, I); r.draw(bande_, essaiMat_, I); }
        if (!cree_) return;
        for (int k = 0; k < 25; k++) r.draw(maillages_[k], mats_[k], I);
    }
    void shutdown(Renderer& r) override {
        if (essaiTente_) {
            r.vk().waitIdle();
            if (essaiCree_) { r.destroy(panneau_); r.destroy(bande_); }
            for (ktx2::Texture& t : essaiTex_) { r.vk().destroy(t.image); t = ktx2::Texture{}; }
            essaiCree_ = essaiTente_ = false;
        }
        if (!cree_) return;
        for (Mesh& m : maillages_) r.destroy(m);
        cree_ = false;
    }
private:
    // WFS-149 : panneau et bande de la texture d'essai (tangentes calculées par makeMesh : géométrie générée)
    void creerEssai(Renderer& r, const GameView& g) {
        const double s = g.player->dist;
        std::ostringstream info;
        for (int k = 0; k < 3; k++) {
            if (!ktx2::chargerTexture(r.vk(), ESSAI_TEXTURES[k], essaiTex_[k])) {
                info << ESSAI_TEXTURES[k] << " : absente (lancer tools/textures/ci_textures.sh)\n";
                continue;
            }
            info << ESSAI_TEXTURES[k] << " : " << essaiTex_[k].variante << ", " << ktx2::nomFormat(essaiTex_[k].format) << ", "
                 << essaiTex_[k].image.w << "x" << essaiTex_[k].image.h << ", " << essaiTex_[k].image.mips << " niveaux, "
                 << reel::fmt(essaiTex_[k].octets / 1024.0, 1) << " Ko\n";
        }
        g_banc.essaiInfo = info.str();
        if (!essaiTex_[0].ok) return;
        auto P = [&](float a, float l, float h) { return pointRoute(*g.track, s, {a, l, h}); };
        auto quad = [&](std::vector<Vertex>& v, std::vector<uint32_t>& ix, const vec3 c[4], const vec2 uv[4], const vec3& vers) {
            const vec3 n = normalize(cross(c[1] - c[0], c[2] - c[0]));
            const vec3 nn = dot(n, vers) >= 0 ? n : n * -1.f;   // normale tournée vers « vers » (caméra, ou le ciel)
            const uint32_t b = (uint32_t)v.size();
            for (int k = 0; k < 4; k++) v.push_back({c[k].x, c[k].y, c[k].z, nn.x, nn.y, nn.z, uv[k].x, uv[k].y});
            // sens des triangles : face avant vers « vers » (scene.frag retourne la normale des faces arrière)
            if (dot(n, vers) >= 0) ix.insert(ix.end(), {b, b + 1, b + 2, b, b + 2, b + 3}); else ix.insert(ix.end(), {b, b + 2, b + 1, b, b + 3, b + 2});
        };
        {   // panneau : l de 1,5 à 4,5 m, h de 0,2 à 3,2 m, à 8 m ; uv (0, 0) en haut à gauche vu de la caméra
            std::vector<Vertex> v; std::vector<uint32_t> ix;
            const vec3 c[4] = {P(8, 1.5f, 3.2f), P(8, 4.5f, 3.2f), P(8, 4.5f, 0.2f), P(8, 1.5f, 0.2f)};
            const vec2 uv[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
            quad(v, ix, c, uv, P(-3, 4.2f, 1.7f) - P(8, 3, 1.7f));
            panneau_ = r.makeMesh(v, ix);
        }
        {   // bande sur la chaussée : l de −1,5 à 1,5 m, a de 2 à 82 m, par tronçons de 2 m (suit la route)
            std::vector<Vertex> v; std::vector<uint32_t> ix;
            for (float a = 2; a < 82; a += 2) {
                const vec3 c[4] = {P(a, -1.5f, SOL), P(a, 1.5f, SOL), P(a + 2, 1.5f, SOL), P(a + 2, -1.5f, SOL)};
                const vec2 uv[4] = {{0, -a}, {3, -a}, {3, -a - 2}, {0, -a - 2}};
                quad(v, ix, c, uv, vec3{0, 1, 0});
            }
            bande_ = r.makeMesh(v, ix);
        }
        Material m; m.color = {1, 1, 1, 1}; m.metal = 1; m.rough = 1;   // × ORM (G rugosité, B métal) : valeurs de la texture
        m.flags = MAT_TANGENTE;   // + MAT_HASNORMAL / MAT_HASMR posés par makeMaterial (cartes présentes)
        essaiMat_ = r.makeMaterial(m, &essaiTex_[0].image, essaiTex_[1].ok ? &essaiTex_[1].image : nullptr, essaiTex_[2].ok ? &essaiTex_[2].image : nullptr);
        essaiCree_ = true;
    }
    Mesh maillages_[25]; Material mats_[25]; bool cree_ = false;
    ktx2::Texture essaiTex_[3]; Mesh panneau_, bande_; Material essaiMat_; bool essaiCree_ = false, essaiTente_ = false;
};
WFS_SYSTEM(RealismeBanc, 970)   // banc (900-999) : après toutes les ambiances (nuit 950, ciel couvert 960)

// ---------------------------------------------------------------- outils du banc
// segment de départ d'une vue (WFS_DEPART, lu par les cartes) ; −1 si la zone n'existe pas
int64_t segmentDepart(const Vue& v) {
    Track t; t.build(mapById(v.carte));
    const int64_t L = (int64_t)t.segmentCount();
    int64_t z0 = 0, z1 = L;
    if (v.zone) {
        z0 = -1;
        for (int64_t a = 0; a < L; a++) { const bool dans = !std::strcmp(zoneName(*t.map(), a, L), v.zone); if (dans && z0 < 0) z0 = a; if (!dans && z0 >= 0) { z1 = a; break; } }
        if (z0 < 0) return -1;
    }
    if (v.choix == Choix::Depart) return v.zone ? z0 + 40 : 0;   // + 40 (≈ 50 m) : dans le décor de la zone (comme benchCartes)
    // ligne droite / virage : somme des |courbures| sur 100 m devant le joueur, la plus faible ou la plus forte de la zone
    const int64_t n = 84;
    int64_t meilleur = z0 + 40; double vMeilleur = v.choix == Choix::LigneDroite ? 1e30 : -1;
    for (int64_t a = z0 + 40; a + n < z1 - 10; a++) {
        double c = 0; for (int64_t k = 0; k < n; k++) c += std::fabs(t.segAt(a + k).curve);
        if ((v.choix == Choix::LigneDroite && c < vMeilleur) || (v.choix == Choix::Virage && c > vMeilleur)) { vMeilleur = c; meilleur = a; }
    }
    return meilleur;
}
long memoireKo(const char* cle) {   // /proc/self/status (VmRSS, VmHWM) : mémoire du processus en Ko ; −1 hors Linux
    std::ifstream f("/proc/self/status"); std::string l;
    while (std::getline(f, l)) if (!l.compare(0, std::strlen(cle), cle)) return std::atol(l.c_str() + std::strlen(cle) + 1);
    return -1;
}
// dernière ligne du Renderer « Ombres : passe d'ombre … ms GPU … ; passe de scène … ms ; reflets … ms » (toutes les 120 images)
bool coutGpu(double& ombre, double& scene, double& reflets) {
    for (int i = plantage::anneauNombre() - 1; i >= 0; i--) {
        const char* l = plantage::anneauLigne(i);
        if (!l || !std::strstr(l, "passe d'ombre")) continue;
        float a, b, c; unsigned carte;
        if (std::sscanf(std::strstr(l, "passe d'ombre"), "passe d'ombre %f ms GPU en moyenne (carte %u) ; passe de scène %f ms ; reflets %f ms", &a, &carte, &b, &c) == 4) {
            ombre = a; scene = b; reflets = c; return true;
        }
    }
    return false;
}
// température de couleur proximale d'une couleur sRGB linéaire (formule de C. S. McCamy, « Correlated color temperature as
// an explicit function of chromaticity coordinates », Color Res. Appl. 17, 1992 ; valable ≈ 2000 à 12 500 K)
double tcpMcCamy(const vec3& c) {
    const double rgb[3] = {c.x, c.y, c.z}; double xyz[3]; reel::lineaireVersXyz(rgb, xyz);
    const double sum = xyz[0] + xyz[1] + xyz[2]; if (sum <= 0) return 0;
    const double x = xyz[0] / sum, y = xyz[1] / sum, n = (x - 0.3320) / (0.1858 - y);
    return 449 * n * n * n + 3525 * n * n + 6823.3 * n + 5520.33;
}
std::string carteDuLieu(const char* carte) { return std::string("lieu.") + carte; }

} // namespace

int benchRealisme(Game& g, Renderer& r, const std::string& out) {
    using reel::Etat; using reel::LigneRapport; using reel::fmt;
    int erreurs = 0;
    if (std::getenv("WFS_PASSES_SEUL")) {   // WFS-150 : essai local du seul banc des passes (passes-mesures.txt)
        std::ostringstream m, i; const int e = benchPasses(g, r, out, m, i);
        std::ofstream(out + "/passes-mesures.txt") << m.str() << "\n" << i.str();
        return e;
    }
    reel::Referentiel R;
    if (!R.charger("references/reel")) { WFS_ERR("réalisme : référentiel illisible : %s", R.erreur.c_str()); return 1; }
    ecrans::Reglages& reglages = ecrans::reglages();
    const int q0 = reglages.qualite;
    std::ostringstream mesures;
    mesures << "WaForSpeed — mesures du banc de référence du réalisme (WFS-148, RG-06)\n"
            << "Banc PC de la CI : Vulkan logiciel (lavapipe) : le « GPU » est le processeur de la machine ; ms et images/s ne\n"
            << "représentent PAS un téléphone, ils servent à comparer une US à l'autre sur la même machine (avant / après).\n"
            << "Taille " << r.width() << "x" << r.height() << ". Mémoire textures : compteur VkCtx::octetsTextures (WFS-149 : textures envoyées,\n"
            << "mipmaps comprises) ; mémoire du processus (VmRSS, VmHWM) en plus.\n\n"
            << "== captures (5 images rendues, scène figée) ==\n  capture | ms par image (mur) | appels de dessin | objets masqués | VmRSS (Mo) | textures (Mo)"
            << " | tangentes de la géométrie générée depuis le lancement (ms CPU, sommets)\n";
    struct Lumiere { vec3 soleilDir, soleilCouleur; float intensite, exposition, ambiance; bool ok = false; };
    std::map<std::string, Lumiere> lumieres;   // carte + moment → lumière du jeu (vue en Haute)
    g_banc = EtatBanc{}; g_banc.actif = true;
    double t = 2000;
    const char* qualites[2] = {"basse", "haute"};
    for (const Vue& v : VUES) {
        int64_t depart = segmentDepart(v);
        if (depart < 0) { WFS_ERR("réalisme : zone %s absente de %s", v.zone, v.carte); erreurs++; continue; }
        if (v.choix == Choix::Terrasse) {   // première terrasse : connue une fois la ville construite (commerces.cpp)
            unsetenv("WFS_DEPART"); setenv("WFS_MOMENT", "jour", 1);
            if (g.init(r, "golf", v.carte)) { depart = vie::g_premiereTerrasse > 0 ? (int64_t)vie::g_premiereTerrasse - 4 : 40; g.shutdown(); }
        }
        g_banc.oeil = v.oeil; g_banc.cible = v.cible; g_banc.fov = v.fov;
        for (const char* moment : {"jour", "nuit"}) for (int q = 0; q < 2; q++) {
            setenv("WFS_MOMENT", moment, 1); setenv("WFS_DEPART", std::to_string(depart).c_str(), 1);
            reglages.qualite = q ? ecrans::QUALITE_HAUTE : ecrans::QUALITE_BASSE; ecrans::appliquerQualiteNiveau(r);
            const std::string nom = std::string("realisme-") + v.nom + "-" + moment + "-" + qualites[q];
            if (!g.init(r, "golf", v.carte)) { WFS_ERR("réalisme : %s : chargement de %s impossible", nom.c_str(), v.carte); erreurs++; continue; }
            g.setAutopilot(false); g.camMode = 0; g.allerA(ECRAN_BANC);
            for (int i = 0; i < PAS_AVANT_CAPTURE; i++) { g.update(1.0 / 60); if (i % 10 == 0) g.render(t); }
            const double t0 = nowSeconds();
            for (int i = 0; i < 5; i++) g.render(t);   // scène figée : même temps, aucune mise à jour
            const double ms = (nowSeconds() - t0) / 5 * 1000;
            r.capturePng(out + "/" + nom + ".png");
            mesures << "  " << nom << " | " << fmt(ms, 1) << " | " << r.appelsDessin() << " | " << r.objetsMasques() << " | " << fmt(memoireKo("VmRSS:") / 1024.0, 0)
                    << " | " << fmt(r.vk().octetsTextures / 1048576.0, 1) << " | " << fmt(r.tangentesMs(), 1) << " ms, " << r.tangentesSommets() << "\n";
            WFS_LOG("capture %s : %s, départ %lld segments (%s), %.0f m, %.1f ms par image, %u appels de dessin", nom.c_str(), v.carte, (long long)depart,
                    v.note, g.player().dist * M, ms, r.appelsDessin());
            if (q == 1 && g_banc.lumiereLue) {
                Lumiere& l = lumieres[std::string(v.carte) + "." + moment];
                if (!l.ok) l = {g_banc.lumiere.sunDir, g_banc.lumiere.sunColor, g_banc.lumiere.sunIntensity, g_banc.lumiere.exposure, g_banc.lumiere.ambient, true};
            }
            g.shutdown();
        }
    }

    // ---------------- RG-06 : coût par carte (jour), Basse et Haute : 130 images rendues (le Renderer écrit le coût GPU
    // moyen de ses passes toutes les 120 images, horodatages Vulkan)
    mesures << "\n== coût par carte (jour, scène figée, 130 images) ==\n  carte | qualité | ms par image (mur) | images/s (banc) | ms GPU ombre | ms GPU scène | ms GPU reflets | VmRSS / VmHWM (Mo)\n";
    for (const char* carte : {"ville", "campagne", "plage", "circuit"}) for (int q = 0; q < 2; q++) {
        setenv("WFS_MOMENT", "jour", 1); unsetenv("WFS_DEPART");
        reglages.qualite = q ? ecrans::QUALITE_HAUTE : ecrans::QUALITE_BASSE; ecrans::appliquerQualiteNiveau(r);
        g_banc.oeil = {-8, 0, 2}; g_banc.cible = {80, 0, 0.5f}; g_banc.fov = 60;
        if (!g.init(r, "golf", carte)) { erreurs++; continue; }
        g.allerA(ECRAN_BANC);
        for (int i = 0; i < PAS_AVANT_CAPTURE; i++) { g.update(1.0 / 60); if (i % 10 == 0) g.render(t); }
        const double t0 = nowSeconds();
        for (int i = 0; i < 130; i++) g.render(t);
        const double ms = (nowSeconds() - t0) / 130 * 1000;
        double go = -1, gs = -1, gr = -1;
        const bool gpu = coutGpu(go, gs, gr);
        mesures << "  " << carte << " | " << qualites[q] << " | " << fmt(ms, 1) << " | " << fmt(1000 / std::max(ms, 1e-3), 1) << " | "
                << (gpu ? fmt(go, 2) : "non disponible") << " | " << (gpu ? fmt(gs, 2) : "non disponible") << " | " << (gpu ? fmt(gr, 2) : "non disponible") << " | "
                << fmt(memoireKo("VmRSS:") / 1024.0, 0) << " / " << fmt(memoireKo("VmHWM:") / 1024.0, 0) << "\n";
        WFS_LOG("réalisme : coût %s %s : %.1f ms par image, GPU %s", carte, qualites[q], ms, gpu ? (fmt(go, 2) + " + " + fmt(gs, 2) + " + " + fmt(gr, 2) + " ms").c_str() : "non disponible");
        g.shutdown();
    }

    // ---------------- RG-04 : scène d'étalonnage (Campagne, champs ouverts en plein soleil, jour, Haute, départ) : charte vue de dessus, 3,2 m au-dessus
    reel::Rapport rap;
    std::ostringstream infos;
    g_banc.albedos.clear();
    for (int k = 1; k <= 25; k++) {
        char id[32]; std::snprintf(id, sizeof id, "cc.%02d.albedo", k);
        const reel::Json* m = k == 25 ? R.mesure("mat.charte_grise_18.albedo") : R.mesure(id);
        const reel::Json* v = m ? m->get("valeur") : nullptr;
        if (!v || v->a.size() != 3) { WFS_ERR("réalisme : %s absent du référentiel", k == 25 ? "mat.charte_grise_18.albedo" : id); erreurs++; break; }
        g_banc.albedos.push_back({(float)v->a[0].n, (float)v->a[1].n, (float)v->a[2].n});
    }
    if (g_banc.albedos.size() == 25) {
        setenv("WFS_MOMENT", "jour", 1); unsetenv("WFS_DEPART");
        reglages.qualite = ecrans::QUALITE_HAUTE; ecrans::appliquerQualiteNiveau(r);
        g_banc.etalon = true; g_banc.oeil = {CHARTE_A + 0.35f, 0, 3.2f}; g_banc.cible = {CHARTE_A + 0.35f, 0, 0}; g_banc.fov = 50;
        if (g.init(r, "golf", "campagne")) {   // en Ville, la chaussée du départ est à l'ombre des immeubles
            g.allerA(ECRAN_BANC);
            for (int i = 0; i < PAS_AVANT_CAPTURE; i++) { g.update(1.0 / 60); if (i % 10 == 0) g.render(t); }
            for (int i = 0; i < 3; i++) g.render(t);
            const std::string chemin = out + "/realisme-etalon.png";
            r.capturePng(chemin);
            int W = 0, H = 0, n = 0;
            unsigned char* px = stbi_load(chemin.c_str(), &W, &H, &n, 3);
            const FrameParams& L = g_banc.lumiere;
            infos << "Scène d'étalonnage (realisme-etalon.png) : soleil du jeu dir (" << fmt(L.sunDir.x, 3) << ", " << fmt(L.sunDir.y, 3) << ", " << fmt(L.sunDir.z, 3)
                  << "), couleur (" << fmt(L.sunColor.x, 3) << ", " << fmt(L.sunColor.y, 3) << ", " << fmt(L.sunColor.z, 3) << "), intensité " << fmt(L.sunIntensity, 2)
                  << " (sans unité), exposition " << fmt(L.exposure, 2) << ", ambiance " << fmt(L.ambient, 2) << ".\n"
                  << "Couleur relevée après tonemapping (PNG, moyenne de 9 × 9 pixels au centre de chaque case). Avant tonemapping (HDR) : non mesurable\n"
                  << "dans WFS-148 (le Renderer ne relit pas la cible HDR ; ajout proposé pour WFS-150, passes enfichables).\n";
            if (!px) { rap.nonMesurable("couleurs", "cc.*", "charte rendue", "capture realisme-etalon.png illisible"); erreurs++; }
            else {
                const double s = g.player().dist;
                Track piste; piste.build(mapById("campagne"));
                std::vector<reel::Lab> mesure(25); std::vector<std::array<double, 3>> lin(25);
                for (int k = 0; k < 25; k++) {
                    const vec3 c = casePos(k);
                    const vec3 p = pointRoute(piste, s, {c.x, c.y, SOL});
                    const mat4 vp = g_banc.proj * g_banc.view;
                    const float x = vp.at(0, 0) * p.x + vp.at(0, 1) * p.y + vp.at(0, 2) * p.z + vp.at(0, 3), y = vp.at(1, 0) * p.x + vp.at(1, 1) * p.y + vp.at(1, 2) * p.z + vp.at(1, 3);
                    const float w = vp.at(3, 0) * p.x + vp.at(3, 1) * p.y + vp.at(3, 2) * p.z + vp.at(3, 3);
                    const int cx = (int)((x / w * 0.5f + 0.5f) * W), cy = (int)((y / w * 0.5f + 0.5f) * H);
                    double somme[3] = {0, 0, 0}; int nb = 0;
                    for (int dy = -4; dy <= 4; dy++) for (int dx = -4; dx <= 4; dx++) {
                        const int X = cx + dx, Y = cy + dy; if (X < 0 || Y < 0 || X >= W || Y >= H || w <= 0) continue;
                        for (int c3 = 0; c3 < 3; c3++) somme[c3] += px[(Y * W + X) * 3 + c3];
                        nb++;
                    }
                    if (!nb) { mesure[k] = {}; lin[k] = {0, 0, 0}; continue; }
                    const double s8[3] = {somme[0] / nb, somme[1] / nb, somme[2] / nb};
                    mesure[k] = reel::srgb8VersLab(s8);
                    lin[k] = {reel::srgbVersLineaire(s8[0]), reel::srgbVersLineaire(s8[1]), reel::srgbVersLineaire(s8[2])};
                    char id[32]; std::snprintf(id, sizeof id, "cc.%02d", k + 1);
                    if (k == 24) {   // charte grise 18 % : luminance relative Y rendue contre 0,18 (± 1 %, DA-realisme.md § 4)
                        double xyz[3]; const double l3[3] = {lin[k][0], lin[k][1], lin[k][2]}; reel::lineaireVersXyz(l3, xyz);
                        rap.comparer(R, "exposition", "mat.charte_grise_18.albedo", xyz[1], 1, "Y", "charte grise 18 % : luminance relative rendue (Y, sRGB linéaire)");
                        // la mesure du référentiel est un triplet : comparaison écrite à la main (Y contre 0,18 ± 1 %)
                        rap.lignes.back().reel = "0.1800 (18 % de réflectance)"; rap.lignes.back().tolerance = "± 1.0 %";
                        rap.lignes.back().etat = std::fabs(xyz[1] - 0.18) <= 0.0018 ? Etat::Conforme : Etat::NonConforme; rap.lignes.back().raison.clear();
                        continue;
                    }
                    const reel::Json* ref = R.mesure(id);
                    const auto& rv = ref->get("valeur")->a;
                    const reel::Lab attendu{rv[0].n, rv[1].n, rv[2].n};
                    const double dE = reel::deltaE2000(mesure[k], attendu);
                    LigneRapport l; l.domaine = "couleurs"; l.id = id; l.grandeur = ref->texte("grandeur");
                    l.jeu = "L*a*b* " + fmt(mesure[k].L, 1) + " " + fmt(mesure[k].a, 1) + " " + fmt(mesure[k].b, 1) + " (sRGB " + fmt(s8[0], 0) + " " + fmt(s8[1], 0) + " " + fmt(s8[2], 0) + "), ΔE2000 " + fmt(dE, 2);
                    l.reel = "L*a*b* " + fmt(attendu.L, 2) + " " + fmt(attendu.a, 2) + " " + fmt(attendu.b, 2); l.tolerance = "ΔE2000 ≤ 2"; l.statutReel = ref->texte("statut");
                    l.etat = dE <= 2 ? Etat::Conforme : Etat::NonConforme;
                    rap.ajouter(l);
                }
                // information (hors indice) : écarts après balance des blancs et exposition corrigées sur la case 22 (neutre 5) :
                // part de l'écart due à l'exposition et à la teinte du soleil, et part due au reste (tonemapping, matériaux)
                const reel::Json* n5 = R.mesure("cc.22.albedo");
                if (n5 && lin[21][0] > 0 && lin[21][1] > 0 && lin[21][2] > 0) {
                    const auto& a = n5->get("valeur")->a;
                    const double k3[3] = {a[0].n / lin[21][0], a[1].n / lin[21][1], a[2].n / lin[21][2]};
                    double somme = 0, pire = 0;
                    infos << "\nInformation hors indice : ΔE2000 après balance des blancs et exposition sur la case 22 (gains R " << fmt(k3[0], 3) << ", V " << fmt(k3[1], 3) << ", B " << fmt(k3[2], 3) << ") :\n ";
                    for (int k = 0; k < 24; k++) {
                        char id[32]; std::snprintf(id, sizeof id, "cc.%02d", k + 1);
                        const auto& rv = R.mesure(id)->get("valeur")->a;
                        const double c3[3] = {lin[k][0] * k3[0], lin[k][1] * k3[1], lin[k][2] * k3[2]};
                        const double d = reel::deltaE2000(reel::lineaireVersLab(c3), {rv[0].n, rv[1].n, rv[2].n});
                        somme += d; pire = std::max(pire, d); infos << " " << (k + 1) << ":" << fmt(d, 1);
                    }
                    infos << "\n  moyenne " << fmt(somme / 24, 2) << ", pire " << fmt(pire, 2) << "\n";
                }
                stbi_image_free(px);
            }
            g.shutdown();
        } else erreurs++;
        g_banc.etalon = false;
    }
    // ---------------- WFS-149 (RG-01) : texture d'essai KTX2 (Campagne, ligne droite des Champs, jour) : Haute (anisotrope ×8),
    // puis Basse (×4, WFS-144)
    {
        const Vue v{"texture-essai", "campagne", "Champs", Choix::LigneDroite, {-3, 4.2f, 1.7f}, {12, 1.8f, 1.0f}, 60, "texture d'essai KTX2"};
        const int64_t depart = segmentDepart(v);
        g_banc.essai = true; g_banc.fov = v.fov;
        mesures << "\n== texture d'essai KTX2 (WFS-149) ==\n";
        for (int q = 1; q >= 0 && depart >= 0; q--) {
            setenv("WFS_MOMENT", "jour", 1); setenv("WFS_DEPART", std::to_string(depart).c_str(), 1);
            reglages.qualite = q ? ecrans::QUALITE_HAUTE : ecrans::QUALITE_BASSE; ecrans::appliquerQualiteNiveau(r);
            g_banc.oeil = v.oeil; g_banc.cible = v.cible;
            if (!g.init(r, "golf", v.carte)) { erreurs++; continue; }
            g.setAutopilot(false); g.camMode = 0; g.allerA(ECRAN_BANC);
            for (int i = 0; i < PAS_AVANT_CAPTURE; i++) { g.update(1.0 / 60); if (i % 10 == 0) g.render(t); }
            for (int i = 0; i < 3; i++) g.render(t);
            const std::string nom = std::string("realisme-texture-essai") + (q ? "" : "-basse");
            r.capturePng(out + "/" + nom + ".png");
            mesures << "  " << nom << " : anisotrope ×" << fmt(q ? r.aniso() : std::min(r.aniso(), 4.f), 0) << ", mémoire textures "
                    << fmt(r.vk().octetsTextures / 1048576.0, 2) << " Mo\n" << g_banc.essaiInfo;
            WFS_LOG("capture %s : texture d'essai KTX2, %s", nom.c_str(), g_banc.essaiInfo.empty() ? "aucune" : "chargée");
            if (g_banc.essaiInfo.find("absente") != std::string::npos) { WFS_ERR("réalisme : texture d'essai KTX2 absente"); erreurs++; }
            g.shutdown();
        }
        g_banc.essai = false;
    }
    g_banc = EtatBanc{};   // système inactif : plus rien ne change dans les images suivantes
    reglages.qualite = q0; ecrans::appliquerQualiteNiveau(r);
    unsetenv("WFS_MOMENT"); unsetenv("WFS_DEPART");
    erreurs += benchPasses(g, r, out, mesures, infos);   // WFS-150 : passes, pré-passe, capture d'environnement, Ultra (après toutes les vues)

    // ---------------- soleil et lumière du jeu contre le lieu et le moment de chaque carte (lieux.json)
    for (const char* carte : {"ville", "campagne", "plage", "circuit"}) {
        const reel::Json *lat = R.mesure(carteDuLieu(carte) + ".latitude"), *lon = R.mesure(carteDuLieu(carte) + ".longitude"), *jour = R.mesure(carteDuLieu(carte) + ".jour");
        const auto lj = lumieres.find(std::string(carte) + ".jour"), ln = lumieres.find(std::string(carte) + ".nuit");
        int a, mo, j; double h;
        if (!lat || !lon || !jour || !reel::lireDateUtc(jour->get("valeur") ? jour->get("valeur")->s : "", a, mo, j, h) || lj == lumieres.end()) {
            rap.nonMesurable("soleil du jeu", std::string("jeu.") + carte, "hauteur du soleil", "lieu, moment ou lumière de la carte absents"); continue;
        }
        const reel::Soleil s = reel::soleil(lat->nombre("valeur", 0), lon->nombre("valeur", 0), a, mo, j, h);
        const vec3 d = normalize(lj->second.soleilDir);
        const double hj = reel::deg(std::asin(std::clamp((double)d.y, -1.0, 1.0)));
        LigneRapport l; l.domaine = "soleil du jeu"; l.id = std::string("jeu.") + carte + ".soleil.hauteur";
        l.grandeur = std::string("hauteur du soleil, ") + carte + ", jour (" + jour->get("valeur")->s + " UTC, " + fmt(lat->nombre("valeur", 0), 1) + "° N " + fmt(lon->nombre("valeur", 0), 1) + "° E)";
        l.jeu = fmt(hj, 2) + " °"; l.reel = fmt(s.hauteurApparente, 2) + " ° (soleil(), lieu et moment proposés : lieux.json)"; l.tolerance = "± 0.5 °";
        l.statutReel = "conception"; l.etat = std::fabs(hj - s.hauteurApparente) <= 0.5 ? Etat::Conforme : Etat::NonConforme;
        rap.ajouter(l);
        rap.nonMesurable("soleil du jeu", std::string("jeu.") + carte + ".soleil.azimut", "azimut du soleil, " + std::string(carte),
                         "le monde du jeu n'a pas de nord défini (à fixer avec le ciel physique, WFS-152)");
        // température de couleur (McCamy) de la lumière directionnelle : soleil le jour, lune la nuit
        for (const auto& [it, idReel, quoi] : {std::tuple{lj, "lum.soleil_midi.temperature", "soleil (jour)"}, std::tuple{ln, "lum.pleine_lune.temperature", "lune (nuit)"}}) {
            if (it == lumieres.end()) { rap.nonMesurable("photometrie", std::string("jeu.") + carte, quoi, "lumière non relevée"); continue; }
            rap.comparer(R, "photometrie", idReel, tcpMcCamy(it->second.soleilCouleur), 1, "K",
                         std::string("température de couleur de la lumière directionnelle, ") + quoi + ", " + carte + " (McCamy 1992, couleur du jeu)");
            const double tcp = tcpMcCamy(it->second.soleilCouleur);
            if (tcp < 2000 || tcp > 12500) {   // WFS-149 (RG-B, ANO-86) : hors du domaine de la formule → non mesurable (hors indice)
                rap.lignes.back().raison = "hors du domaine de validité de la formule de McCamy (2000 à 12 500 K) : couleur très saturée";
                rap.lignes.back().etat = Etat::NonMesurable;
            }
        }
        rap.nonMesurable("photometrie", std::string("jeu.") + carte + ".eclairement", std::string("éclairement du soleil (lux), ") + carte,
                         "intensité du soleil du jeu sans unité (" + fmt(lj->second.intensite, 2) + ") et exposition " + fmt(lj->second.exposition, 2) + " sans EV100 : exposition physique à venir (WFS-151)");
    }
    rap.nonMesurable("materiaux", "mat.*", "albédo, rugosité et F0 des surfaces du jeu", "matériaux procéduraux (scene.frag) : mesure dans l'image HDR non disponible (WFS-150) ; mesurés par WFS-155 (routes) et WFS-159 (véhicules)");
    rap.nonMesurable("anthropometrie", "humain.*", "tailles et proportions des piétons", "mesurées par WFS-164 (humains réalistes) sur les nouveaux modèles");

    // ---------------- mesures géométriques (voitures, routes) et soleil calculé : les mêmes que test_fidelite
    std::vector<std::string> cars; for (int i = 0; i < CAR_COUNT; i++) cars.push_back(CARS[i].id);
    reel::mesuresDonnees(R, rap, ".", cars);

    // ---------------- écriture des rapports
    std::string titre = "WaForSpeed — rapport de fidélité au réel (WFS-148, RG-05) : ÉTAT ZÉRO de l'axe Réalisme\n";
    if (const char* sha = std::getenv("GITHUB_SHA")) titre += std::string("Commit ") + sha + "\n";
    titre += "Avant (main) = après (branche) : WFS-148 ne change pas l'image du jeu ; ce rapport est la base des US suivantes (RG-07).\n"
             "Valeurs réelles : references/reel/*.json (sources : references/reel/SOURCES.md) ; statut « a_confirmer » = page de la source non ouverte.\n";
    const std::string texte = rap.texte(titre) + "\n== Informations (hors indice) ==\n" + infos.str();
    std::ofstream(out + "/realisme-fidelite.txt") << texte;
    std::ofstream(out + "/realisme-mesures.txt") << mesures.str();
    for (const auto& [d, i] : rap.indices())
        WFS_LOG("réalisme : indice %s = %s (%d / %d conformes, %d non mesurables)", d.c_str(), i.enTexte().c_str(), i.conformes, i.conformes + i.nonConformes, i.nonMesurables);
    WFS_LOG("réalisme : %zu mesures, rapports realisme-fidelite.txt et realisme-mesures.txt, %d erreur(s)", rap.lignes.size(), erreurs);
    return erreurs;
}

} // namespace wfs
