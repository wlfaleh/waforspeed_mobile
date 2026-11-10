// WaForSpeed — WFS-381 (épic E13) : banc de la brume au loin (WFS_BANC=brume), voir atmosphere_brume_banc.h.
//
// Captures (Haute) : brume-corniche-jour, brume-campagne-jour, brume-ville-jour, brume-corniche-nuit (même cadrage que le
// jour), et une vue haute de chaque carte de jour (brume-<carte>-haut : lointains à plusieurs kilomètres).
// brume-mesures.txt : réglage de chaque carte (β0, H, visibilité), contraste restant calculé à 200 m, 1, 2 et 5 km (RG-02),
// et coût GPU de la passe de scène en Basse, de jour, scène figée (RG-05 : comparer la valeur de main et celle de la branche).
//
// Le système « BrumeBanc » est inactif hors de ce banc : il ne dessine rien ; pendant le banc, il place la caméra dans le
// repère de la route au joueur (ambiance(), après toutes les ambiances des cartes), comme le banc du réalisme (WFS-148).
#include "atmosphere_brume_banc.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "../game/game.h"
#include "../game/track.h"
#include "../platform/plantage.h"   // anneau du journal : coût GPU des passes écrit par le Renderer (horodatages Vulkan)
#include "../ui/ecrans/reglages.h"
#include "../world/zones.h"
#include "atmosphere_brume.h"
#include "eclairage_banc.h"   // attendreSondes (WFS-151)
#include "eclairage_physique.h"   // Forcage::brume, etat() : brume envoyée au shader
#include "passes.h"   // lireHdr : cible HDR avant tonemapping
#include "../../third_party/stb/stb_image.h"   // relecture des captures PNG (implémentation : model_loader.cpp)
#include <cmath>
#include <vector>
#include "renderer.h"

namespace wfs {
namespace {

// point du repère de la route au joueur : a = mètres devant (le long de la route), l = mètres à droite, h = hauteur
struct Repere { float a = 0, l = 0, h = 0; };
struct Vue { const char* nom; const char* carte; const char* zone; bool droite; Repere oeil, cible; float fov; const char* note; };

// cadrages : œil à hauteur d'une caméra de poursuite (≈ 2 m), regard vers le loin ; vues hautes : 40 m au-dessus de la route
const Vue VUES[] = {
    {"corniche", "plage", "Corniche", false, {-6, 0, 2.2f}, {400, 0, 8}, 60, "Corniche, vue de poursuite, collines et mer au loin"},
    {"campagne", "campagne", nullptr, true, {-6, 0, 2.2f}, {600, 0, 6}, 60, "Campagne, ligne droite la plus longue, vers les reliefs"},
    {"ville", "ville", nullptr, true, {-6, 0, 2.2f}, {500, 0, 6}, 60, "Ville, rue la plus longue, immeubles du fond"},
    {"corniche-haut", "plage", "Corniche", false, {-20, 0, 40}, {800, 0, 0}, 60, "Corniche vue de 40 m, lointains à plusieurs kilomètres"},
    {"campagne-haut", "campagne", nullptr, true, {-20, 0, 40}, {800, 0, 0}, 60, "Campagne vue de 40 m"},
    {"ville-haut", "ville", nullptr, true, {-20, 0, 40}, {800, 0, 0}, 60, "Ville vue de 40 m"},
};
constexpr int PAS_AVANT_CAPTURE = 90;   // pas de simulation (1/60 s) avant la capture : chargement des morceaux, puis figé
constexpr Ecran ECRAN_BANC = Ecran::Chargement;   // aucune interface dessinée (comme WFS-149)

struct EtatBanc { bool actif = false; Repere oeil, cible; float fov = 60; float densite = 0, invH = 0; vec3 brume{}; };
EtatBanc g_b;

vec3 pointRoute(Track& t, double s, const Repere& p) {
    const TrackPoint q = t.point(s + p.a / M, p.l);
    return {(float)q.x, (float)q.y + p.h, (float)q.z};
}

class BrumeBanc : public System {
public:
    const char* name() const override { return "brume-banc"; }
    void ambiance(FrameParams& fp, const GameView& g) override {
        if (!g_b.actif || !g.track || !g.player) return;
        const double s = g.player->dist;
        const vec3 oeil = pointRoute(*g.track, s, g_b.oeil), cible = pointRoute(*g.track, s, g_b.cible);
        const float aspect = fp.proj.at(0, 0) != 0 ? -fp.proj.at(1, 1) / fp.proj.at(0, 0) : 16.f / 9.f;
        fp.view = lookAt(oeil, cible, vec3{0, 1, 0});
        fp.proj = perspectiveVk(g_b.fov * PI / 180.f, aspect, 0.1f);
        fp.camPos = oeil;
    }
};
WFS_SYSTEM(BrumeBanc, 971)   // banc (900-999) : après toutes les ambiances (nuit 950, ciel couvert 960)

// segment de départ : + 40 dans la zone demandée, ou début de la ligne droite la plus longue de la carte (100 m les plus droits)
int64_t segmentDepart(const Vue& v) {
    Track t; t.build(mapById(v.carte));
    const int64_t L = (int64_t)t.segmentCount();
    int64_t z0 = 0, z1 = L;
    if (v.zone) {
        z0 = -1;
        for (int64_t a = 0; a < L; a++) { const bool dans = !std::strcmp(zoneName(*t.map(), a, L), v.zone); if (dans && z0 < 0) z0 = a; if (!dans && z0 >= 0) { z1 = a; break; } }
        if (z0 < 0) return -1;
    }
    if (!v.droite) return z0 + 40;
    const int64_t n = 250;   // ≈ 300 m de route
    int64_t meilleur = z0 + 40; double vMeilleur = 1e30;
    for (int64_t a = z0 + 40; a + n < z1 - 10; a++) {
        double c = 0; for (int64_t k = 0; k < n; k++) c += std::fabs(t.segAt(a + k).curve);
        if (c < vMeilleur) { vMeilleur = c; meilleur = a; }
    }
    return meilleur;
}
// dernière ligne du Renderer « passe d'ombre … ms GPU … ; passe de scène … ms ; reflets … ms » (toutes les 120 images)
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
std::string fmt(double v, int n) { char b[32]; std::snprintf(b, sizeof b, "%.*f", n, v); return b; }

// ---- mesure du contraste à l'écran (2e décision du PO, 07:30:48) ----
// Cinq rendus de la même image figée, exposition fixée : N (brume de la carte), M (brume de main avant WFS-381), S (sans brume),
// T (brume totale : sa couleur seule, lobe du soleil compris) et P (sonde : brume homogène β = 10⁻³ m⁻¹, sans hauteur ni lobe).
// Dans la cible HDR (avant tonemapping), un pixel brumé vaut (1 − f) S + f B : f = (N − S) / (T − S) est la perte de contraste du
// pixel ; la sonde donne sa distance d = −ln(1 − fP) / 10⁻³. Pixels gardés : géométrie (pas le ciel), objet assez différent
// de la brume (|B − S| ≥ 15 % de B). Moyennes par bandes de distance (0,8-1,2 km et 1,6-2,4 km).
struct Hdr { std::vector<float> px; uint32_t w = 0, h = 0; };
double lum(const float* c) { return 0.2126 * c[0] + 0.7152 * c[1] + 0.0722 * c[2]; }
struct Bande { double somme = 0; long n = 0; double moyenne() const { return n ? somme / n : -1; } };
struct Mesure { Bande carte1, carte2, main1, main2; double ecartPng = -1; long pixelsLoin = 0; bool ok = false; };

void rendreMode(Game& g, int mode, double ev, double t) {
    eclairage::Forcage f; f.actif = true; f.ev = ev; f.brume = mode; eclairage::forcer(f);
    for (int i = 0; i < 2; i++) g.render(t);
}
Mesure mesurerContraste(Game& g, Renderer& r, double t, const std::string& pngCarte, const std::string& pngMain) {
    Mesure m;
    const double ev = eclairage::etat().ev;
    Hdr N, M, S, T, P;
    rendreMode(g, 0, ev, t); if (!passes::lireHdr(r, N.px, N.w, N.h)) return m;
    r.capturePng(pngCarte);
    rendreMode(g, 4, ev, t); if (!passes::lireHdr(r, M.px, M.w, M.h)) return m;
    r.capturePng(pngMain);
    rendreMode(g, 1, ev, t); if (!passes::lireHdr(r, S.px, S.w, S.h)) return m;
    rendreMode(g, 2, ev, t); if (!passes::lireHdr(r, T.px, T.w, T.h)) return m;
    rendreMode(g, 3, ev, t); const vec3 bp = eclairage::etat().brumeCouleur; if (!passes::lireHdr(r, P.px, P.w, P.h)) return m;
    eclairage::forcer({});
    const size_t n = (size_t)N.w * N.h;
    if (M.px.size() != N.px.size() || S.px.size() != N.px.size() || T.px.size() != N.px.size() || P.px.size() != N.px.size()) return m;
    const float bpc[3] = {bp.x, bp.y, bp.z};
    const double yBp = lum(bpc);
    std::vector<float> dist(n, -1);
    for (size_t i = 0; i < n; i++) {
        const double yS = lum(&S.px[i * 4]), yT = lum(&T.px[i * 4]), yP = lum(&P.px[i * 4]), yN = lum(&N.px[i * 4]), yM = lum(&M.px[i * 4]);
        if (std::fabs(yBp - yS) < 0.15 * yBp || std::fabs(yT - yS) < 0.15 * yT) continue;   // ciel, ou objet de la couleur de la brume
        const double fp = (yP - yS) / (yBp - yS);
        if (!(fp > 0.001 && fp < 0.995)) continue;
        const double d = -std::log(1 - fp) / 1e-3;
        dist[i] = (float)d;
        const double fN = (yN - yS) / (yT - yS), fM = (yM - yS) / (yBp - yS);
        if (d >= 800 && d <= 1200) { m.carte1.somme += fN; m.carte1.n++; m.main1.somme += fM; m.main1.n++; }
        if (d >= 1600 && d <= 2400) { m.carte2.somme += fN; m.carte2.n++; m.main2.somme += fM; m.main2.n++; }
    }
    // critère 2 : écart moyen à l'écran (PNG, 0-255, moyenne des 3 canaux) entre main et la branche, zone des lointains (≥ 1 km)
    int w1, h1, c1, w2, h2, c2;
    unsigned char* a = stbi_load(pngCarte.c_str(), &w1, &h1, &c1, 3);
    unsigned char* b = stbi_load(pngMain.c_str(), &w2, &h2, &c2, 3);
    if (a && b && w1 == w2 && h1 == h2 && w1 > 0 && h1 > 0) {
        double s = 0; long k = 0;
        for (int y = 0; y < h1; y++) for (int x = 0; x < w1; x++) {
            const size_t i = (size_t)((long)y * N.h / h1) * N.w + (size_t)((long)x * N.w / w1);   // même point de la cible HDR
            if (dist[i] < 1000) continue;
            const size_t q = ((size_t)y * w1 + x) * 3;
            s += (std::abs(a[q] - b[q]) + std::abs(a[q + 1] - b[q + 1]) + std::abs(a[q + 2] - b[q + 2])) / 3.0; k++;
        }
        if (k) { m.ecartPng = s / k; m.pixelsLoin = k; }
    }
    if (a) stbi_image_free(a);
    if (b) stbi_image_free(b);
    m.ok = true;
    return m;
}

bool preparer(Game& g, Renderer& r, const Vue& v, int64_t depart, const char* moment, int qualite, double t) {
    setenv("WFS_MOMENT", moment, 1); setenv("WFS_DEPART", std::to_string(depart).c_str(), 1);
    ecrans::reglages().qualite = qualite; ecrans::appliquerQualiteNiveau(r);
    g_b.oeil = v.oeil; g_b.cible = v.cible; g_b.fov = v.fov;
    if (!g.init(r, "golf", v.carte)) return false;
    g.setAutopilot(false); g.camMode = 0; g.allerA(ECRAN_BANC);
    for (int i = 0; i < PAS_AVANT_CAPTURE; i++) { g.update(1.0 / 60); if (i % 10 == 0) g.render(t); }
    attendreSondes(g, t);   // sondes IBL capturées et stables avant la capture
    return true;
}

} // namespace

int benchBrume(Game& g, Renderer& r, const std::string& out) {
    int erreurs = 0;
    std::ostringstream m;
    m << "WFS-381 — brume au loin (perspective atmosphérique)\n";
    if (const char* sha = std::getenv("GITHUB_SHA")) m << "Commit " << sha << "\n";
    m << "\n== réglage par carte (RG-04) et contraste restant calculé, œil et objet au même niveau (RG-02) ==\n"
         "  carte | β0 (m⁻¹) | H (m) | visibilité (km) | contraste restant 200 m | 1 km | 2 km | 5 km\n";
    for (const char* c : {"plage", "ville", "campagne", "circuit"}) {
        const brume::Reglage b = brume::reglageCarte(c);
        m << "  " << c << " | " << fmt(b.beta0 * 1e4, 2) << "e-4 | " << fmt(b.hauteur, 0) << " | " << fmt(brume::visibilite(b) / 1000, 1)
          << " | " << fmt(100 * brume::contraste(b, 200, 2, 2), 1) << " % | " << fmt(100 * brume::contraste(b, 1000, 2, 2), 1)
          << " % | " << fmt(100 * brume::contraste(b, 2000, 2, 2), 1) << " % | " << fmt(100 * brume::contraste(b, 5000, 2, 2), 1) << " %\n";
    }
    g_b = EtatBanc{}; g_b.actif = true;
    const double t = 2000;
    auto& reglages = ecrans::reglages();
    const int qualiteAvant = reglages.qualite;
    std::ostringstream mc;
    mc << "\n== contraste mesuré à l'écran (cible HDR, Haute), perte de contraste moyenne : main → branche (pixels ; calcul à l'horizontale) ==\n"
          "  vue | 0,8-1,2 km | 1,6-2,4 km | écart moyen main / branche à l'écran, lointains ≥ 1 km (niveaux sur 255)\n";
    // ---- captures (Haute) ; nuit : la Corniche, même cadrage que le jour (RG-03 : la nuit reste sombre)
    const char* seule = std::getenv("WFS_BRUME_VUE");   // essai local : une seule vue, sans la mesure du coût
    for (const Vue& v : VUES) {
        if ((seule && std::strcmp(seule, v.nom)) || std::getenv("WFS_BRUME_COUT_SEUL")) continue;   // essai local : coût seul
        const int64_t depart = segmentDepart(v);
        if (depart < 0) { WFS_ERR("brume : zone %s absente de %s", v.zone, v.carte); erreurs++; continue; }
        for (const char* moment : {"jour", "nuit"}) {
            if (!std::strcmp(moment, "nuit") && std::strcmp(v.nom, "corniche")) continue;
            const std::string nom = std::string("brume-") + v.nom + "-" + moment;
            if (!preparer(g, r, v, depart, moment, ecrans::QUALITE_HAUTE, t)) { WFS_ERR("brume : %s : chargement de %s impossible", nom.c_str(), v.carte); erreurs++; continue; }
            for (int i = 0; i < 3; i++) g.render(t);
            if (!std::strcmp(moment, "jour")) {   // capture de la branche + capture « main » (même image, brume d'avant) et mesures
                const Mesure x = mesurerContraste(g, r, t, out + "/" + nom + ".png", out + "/main-" + nom + ".png");
                if (!x.ok) { WFS_ERR("brume : %s : cible HDR illisible", nom.c_str()); erreurs++; }
                const brume::Reglage b = brume::reglageCarte(v.carte);
                mc << "  " << v.nom << " | " << fmt(100 * x.main1.moyenne(), 1) << " % → " << fmt(100 * x.carte1.moyenne(), 1) << " % (" << x.carte1.n
                   << " px ; calcul " << fmt(100 * brume::part(b, 1000, 2, 2), 1) << " %) | " << fmt(100 * x.main2.moyenne(), 1) << " % → "
                   << fmt(100 * x.carte2.moyenne(), 1) << " % (" << x.carte2.n << " px ; calcul " << fmt(100 * brume::part(b, 2000, 2, 2), 1)
                   << " %) | " << fmt(x.ecartPng, 1) << " (" << x.pixelsLoin << " px)\n";
            } else r.capturePng(out + "/" + nom + ".png");
            WFS_LOG("capture %s : %s (%s), départ %lld segments", nom.c_str(), v.carte, v.note, (long long)depart);
            g.shutdown();
        }
    }
    m << mc.str();
    // ---- RG-05 : coût GPU en Basse, de jour, vue de poursuite de chaque carte, 250 images de la scène figée
    m << "\n== coût GPU en Basse (jour, scène figée, moyenne des 120 dernières images, horodatages Vulkan) ==\n"
         "  vue | ms GPU passe de scène | ms GPU ombre | ms par image (mur)\n";
    for (const Vue& v : VUES) {
        if (seule || std::strstr(v.nom, "-haut")) continue;
        const int64_t depart = segmentDepart(v);
        if (depart < 0 || !preparer(g, r, v, depart, "jour", ecrans::QUALITE_BASSE, t)) { erreurs++; continue; }
        const double t0 = nowSeconds();
        for (int i = 0; i < 250; i++) g.render(t);
        const double ms = (nowSeconds() - t0) / 250 * 1000;
        double o = -1, s = -1, rf = -1;
        coutGpu(o, s, rf);
        m << "  " << v.nom << " | " << fmt(s, 2) << " | " << fmt(o, 2) << " | " << fmt(ms, 1) << "\n";
        g.shutdown();
    }
    reglages.qualite = qualiteAvant; ecrans::appliquerQualiteNiveau(r);
    unsetenv("WFS_MOMENT"); unsetenv("WFS_DEPART");
    g_b.actif = false;
    std::ofstream(out + "/brume-mesures.txt") << m.str();
    WFS_LOG("%s", m.str().c_str());
    return erreurs;
}

} // namespace wfs
