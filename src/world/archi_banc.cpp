// WaForSpeed — WFS-161 (ST-1) : banc du kit d'architecture (WFS_BANC=archi). Une rangée des 6 styles posée au bord de la
// route de la Campagne (aucun immeuble de la carte devant), vue depuis la chaussée de près (niveau de détail 0), de loin
// (niveaux 1 et 2) et de dessus (toitures). Matériaux provisoires : albédos du référentiel (references/reel/materiaux.json),
// béton à la place de la pierre, du crépi et du zinc tant que ST-4 ne les a pas sourcés. Inactif hors banc.
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "../game/game.h"
#include "../game/track.h"
#include "../render/bench_realisme.h"   // référentiel (albédos)
#include "../render/renderer.h"
#include "archi_geo.h"
#include "decor_carte.h"

namespace wfs {
namespace {

constexpr int NB = (int)archi::Style::Nombre;
constexpr float LARGEUR[NB] = {16.f, 14.f, 12.f, 9.f, 18.f, 13.f};    // façades de la rangée (m)
constexpr float HAUTEUR[NB] = {20.f, 24.f, 13.f, 7.f, 10.f, 6.5f};    // hauteurs voulues (m)
constexpr float PROF = 12.f, RECUL = 6.f, ECART = 9.f;   // écart large : la route tourne, des façades droites ne doivent pas se chevaucher               // profondeur, recul depuis le bord de la chaussée, écart (m)
constexpr int64_t SEGMENT = 40;                                      // début de la rangée (segments depuis le départ)

struct EtatArchi {
    bool actif = false;
    int lod = 0;
    int vue = 0;   // 0 : de près, depuis la chaussée ; 1 : de loin ; 2 : de dessus ; 3 : face au style « style »
    int style = 0;
} g_archi;

class ArchiBanc : public System {
public:
    const char* name() const override { return "archi-banc"; }

    void init(Renderer& r, const GameView& g) override {
        if (!g_archi.actif || !g.track) return;
        reel::Referentiel R;
        if (!R.charger("references/reel")) WFS_ERR("archi : référentiel illisible : %s", R.erreur.c_str());
        auto albedo = [&](const char* id, vec3 defaut) {
            const reel::Json* m = R.mesure(std::string("mat.") + id + ".albedo");
            const reel::Json* v = m ? m->get("valeur") : nullptr;
            return v && v->a.size() == 3 ? vec3{(float)v->a[0].n, (float)v->a[1].n, (float)v->a[2].n} : defaut;
        };
        const vec3 beton = albedo("beton", {0.5f, 0.5f, 0.5f}), brique = albedo("brique", beton), tuile = albedo("terre_cuite", beton),
                   alu = albedo("aluminium", beton), inox = albedo("acier_inox", beton);
        auto mat = [&](vec3 c, float rough, float metal) {
            Material m; m.color = {c.x, c.y, c.z, 1}; m.rough = rough; m.metal = metal; m.flags = MAT_DOUBLE;
            return r.makeMaterial(m, nullptr, nullptr, nullptr);
        };
        Track& t = *g.track;
        float along = 0;   // m le long de la route depuis le début de la rangée
        for (int s = 0; s < NB; s++) {
            const archi::Style st = (archi::Style)s;
            plans_[s] = archi::assembler(st, LARGEUR[s], HAUTEUR[s], 7u + (uint32_t)s);
            const vec3 murC = st == archi::Style::Industriel ? brique : beton;   // pierre, crépi : provisoire (ST-4)
            mats_[s][archi::MUR] = mat(murC, 0.9f, 0);
            mats_[s][archi::ENCADREMENT] = mat(beton, 0.8f, 0);
            mats_[s][archi::VITRE] = mat({0.03f, 0.035f, 0.04f}, 0.05f, 0);   // verre vu de face : presque noir, reflet du ciel
            mats_[s][archi::MENUISERIE] = mat({0.18f, 0.12f, 0.07f}, 0.6f, 0);
            mats_[s][archi::TOIT] = plans_[s].toit == archi::Toit::Tuiles ? mat(tuile, 0.8f, 0)
                                  : plans_[s].toit == archi::Toit::Zinc ? mat(inox * 0.6f, 0.45f, 1) : mat(beton * 0.6f, 0.95f, 0);
            mats_[s][archi::METAL] = mat(alu * 0.3f, 0.4f, 1);
            mats_[s][archi::VOLET] = mat({0.09f, 0.2f, 0.13f}, 0.6f, 0);   // volets peints (vert) : couleur de conception, provisoire (ST-4)
            // repère : façade face à la route, côté droit ; x local = sens inverse de la route, z local = vers la chaussée
            const int64_t a = SEGMENT + (int64_t)((along + LARGEUR[s]) / SEG_M);
            const carte::Pose p = carte::poseAt(t, a);
            const vec3 o = carte::lateral(p, (float)RW + RECUL);
            const vec3 x = p.f * -1.f, z = p.r * -1.f;
            mat4 w = mat4::identity();
            w.at(0, 0) = x.x; w.at(1, 0) = x.y; w.at(2, 0) = x.z;
            w.at(0, 1) = 0; w.at(1, 1) = 1; w.at(2, 1) = 0;
            w.at(0, 2) = z.x; w.at(1, 2) = z.y; w.at(2, 2) = z.z;
            w.at(0, 3) = o.x; w.at(1, 3) = o.y; w.at(2, 3) = o.z;
            monde_[s] = w;
            centre_[s] = o + x * (LARGEUR[s] / 2);   // milieu de la façade, au sol
            along += LARGEUR[s] + ECART;
            for (int l = 0; l < 3; l++) {
                archi::Batiment b; archi::construire(plans_[s], PROF, l, b);
                size_t tri = 0;
                for (int k = 0; k < archi::NB_PARTS; k++) {
                    std::vector<Vertex> v; v.reserve(b.part[k].v.size());
                    for (const archi::Sommet& q : b.part[k].v) { Vertex x{}; x.px = q.p.x; x.py = q.p.y; x.pz = q.p.z; x.nx = q.n.x; x.ny = q.n.y; x.nz = q.n.z; x.u = q.uv.x; x.v = q.uv.y; v.push_back(x); }
                    if (!v.empty()) meshes_[s][l][k] = r.makeMesh(v, b.part[k].i);
                    tri += b.part[k].triangles();
                }
                if (l == 0) WFS_LOG("archi : %s, %.1f × %.1f m, %d travées, RDC %.2f m, étage %.2f m × %d, toit %d, %zu triangles (lod 0)", archi::regles(st).nom,
                                    plans_[s].largeur, plans_[s].hauteur(), plans_[s].travees, plans_[s].hRdc, plans_[s].hEtage, plans_[s].etages, (int)plans_[s].toit, tri);
            }
        }
        longueur_ = along;
        pret_ = true;
    }

    void ambiance(FrameParams& fp, const GameView& g) override {
        if (!pret_ || !g.track) return;
        Track& t = *g.track;
        const int64_t milieu = SEGMENT + (int64_t)(longueur_ / 2 / SEG_M);
        const carte::Pose pm = carte::poseAt(t, milieu);
        vec3 oeil, cible = carte::lateral(pm, (float)RW + RECUL + PROF / 2, 6.f);
        if (g_archi.vue == 0) { oeil = carte::lateral(carte::poseAt(t, milieu - (int64_t)(30 / SEG_M)), -(float)RW + 1.f, 1.7f); cible = carte::lateral(pm, (float)RW + RECUL, 8.f); }
        else if (g_archi.vue == 1) oeil = carte::lateral(carte::poseAt(t, milieu - (int64_t)(70 / SEG_M)), -4.f, 6.f);
        else if (g_archi.vue == 2) oeil = carte::lateral(pm, -30.f, 55.f);
        else {   // face au style : depuis le bord opposé de la chaussée, à hauteur d'yeux, cadré sur toute la façade
            const int s = std::clamp(g_archi.style, 0, NB - 1);
            const vec3 n = vec3{monde_[s].at(0, 2), 0, monde_[s].at(2, 2)};   // normale de la façade (vers la route)
            const float h = plans_[s].hauteur();
            cible = centre_[s] + vec3{0, h * 0.45f, 0};
            oeil = centre_[s] + n * std::max(16.f, h * 1.15f) + vec3{0, 1.7f, 0};
        }
        fp.view = lookAt(oeil, cible, {0, 1, 0});
        fp.proj = perspectiveVk((g_archi.vue == 0 ? 60.f : 55.f) * PI / 180.f, g.uiWidth / std::max(1.f, g.uiHeight), 0.1f);
        fp.camPos = oeil;
    }

    void render(Renderer& r, const GameView&) override {
        if (!pret_) return;
        const int l = std::clamp(g_archi.lod, 0, 2);
        for (int s = 0; s < NB; s++)
            for (int k = 0; k < archi::NB_PARTS; k++)
                if (meshes_[s][l][k].indexCount) r.draw(meshes_[s][l][k], mats_[s][k], monde_[s]);
    }

    void shutdown(Renderer& r) override {
        if (!pret_) return;
        for (int s = 0; s < NB; s++) {
            for (int l = 0; l < 3; l++) for (Mesh& m : meshes_[s][l]) if (m.indexCount) r.destroy(m);
            for (Material& m : mats_[s]) r.destroy(m);
        }
        *this = ArchiBanc{};
    }

private:
    bool pret_ = false;
    float longueur_ = 0;
    archi::Plan plans_[NB];
    mat4 monde_[NB]{};
    vec3 centre_[NB]{};
    Mesh meshes_[NB][3][archi::NB_PARTS]{};
    Material mats_[NB][archi::NB_PARTS]{};
};

} // namespace
WFS_SYSTEM(ArchiBanc, 985)   // banc (900-999)

// captures : archi-pres-jour / -nuit (lod 0), archi-loin-lod0 / -lod1 / -lod2 (même vue, coût comparé), archi-dessus (toitures)
void benchArchi(Game& g, Renderer& r, const std::string& out) {
    struct Cap { const char* nom; const char* moment; int vue, lod; };
    static const Cap CAPS[] = {{"archi-pres-jour", "jour", 0, 0}, {"archi-pres-nuit", "nuit", 0, 0}, {"archi-loin-lod0", "jour", 1, 0},
                               {"archi-loin-lod1", "jour", 1, 1}, {"archi-loin-lod2", "jour", 1, 2}, {"archi-dessus", "jour", 2, 0}};
    g_archi.actif = true;
    std::vector<Cap> caps(std::begin(CAPS), std::end(CAPS));
    static std::string noms[NB];
    for (int s = 0; s < NB; s++) { noms[s] = std::string("archi-style-") + archi::regles((archi::Style)s).nom; caps.push_back({noms[s].c_str(), "jour", 3, 0}); }
    int k = 0;
    for (const Cap& c : caps) {
        if (c.vue == 3) g_archi.style = k++;
        setenv("WFS_MOMENT", c.moment, 1);
        setenv("WFS_DEPART", "0", 1);
        g_archi.vue = c.vue; g_archi.lod = c.lod;
        if (!g.init(r, "golf", "campagne")) { WFS_ERR("archi : chargement de la Campagne impossible"); break; }
        g.allerA(Ecran::Course);
        for (int i = 0; i < 30; i++) { g.update(1.0 / 60); if (i % 10 == 0) g.render(500 + i / 60.0); }
        const double t0 = nowSeconds();
        for (int i = 0; i < 4; i++) g.render(501 + i * 0.01);
        r.capturePng(out + "/" + c.nom + ".png");
        WFS_LOG("capture %s : %s, vue %d, niveau de détail %d, %.0f ms par image", c.nom, c.moment, c.vue, c.lod, (nowSeconds() - t0) / 4 * 1000);
        g.shutdown();
    }
    g_archi = EtatArchi{};
    unsetenv("WFS_MOMENT"); unsetenv("WFS_DEPART");
}

} // namespace wfs
