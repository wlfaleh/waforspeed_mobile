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
            r.capturePng(out + "/" + nom + ".png");
            WFS_LOG("capture %s : %s (%s), départ %lld segments", nom.c_str(), v.carte, v.note, (long long)depart);
            g.shutdown();
        }
    }
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
