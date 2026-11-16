// WaForSpeed — WFS-559 (épic E23, maquette R19 09 « départ de la course ») : portique de départ en treillis d'aluminium au-dessus
// de la ligne de départ (damier de decor_carte.cpp, s = 0), 5 feux rouges allumés un par un pendant le décompte, verts au « GO »
// (RG-05, RG-06 : Course::feuxRouges / feuxVerts, le décompte existant pilote les feux), consigne sous les feux (RG-08).
// Objet placé par le code (RG-03) : il n'est pas cuit dans les paquets .wfcarte. Construit une fois par carte, au premier
// dessin (la piste est prête) : rien n'est créé pendant la course (RG-C5). Toutes les cartes, sauf le Grand Prix qui a déjà son
// portique de feux (circuit.cpp, WFS-373). La nuit, les feux allumés éclairent le treillis (lumière ponctuelle, RG-04).
#include <algorithm>
#include <cmath>
#include <cstring>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/course_system.h"
#include "../game/track.h"
#include "../ui/langue.h"
#include "../ui/text.h"
#include "enseignes.h"
#include "portique_depart_modele.h"

namespace wfs {
namespace {

using namespace portique_depart;

constexpr float DEGAGEMENT = 1.f;      // RG-02 : largeur = chaussée + 1 m de chaque côté
constexpr float HAUTEUR_LIBRE = 5.5f;  // RG-02 : hauteur libre sous la poutre

// couleurs de l'interface (skill design-street-racing, tokens.md), codes sRGB
const vec4 TEXTE_UI{242 / 255.f, 244 / 255.f, 248 / 255.f, 1};       // --text
const vec4 FOND_UI{10 / 255.f, 11 / 255.f, 16 / 255.f, 0.72f};         // --bg en verre teinté

class PortiqueDepart : public System {
public:
    const char* name() const override { return "portique de départ"; }

    void init(Renderer& r, const GameView& g) override {
        actif_ = false; construit_ = false;
        if (!g.map || !std::strcmp(g.map->id, "circuit")) return;   // le Grand Prix a son propre portique de feux (WFS-373)
        // matériaux PBR (regles-de-gestion/DA-realisme.md, § 3) : aluminium brut (F0 0,91), acier galvanisé terni, aluminium
        // laqué noir (diélectrique, albédo 0,035), polycarbonate des lentilles (éteint : rouge très sombre et brillant)
        Material alu; alu.color = {0.913f, 0.922f, 0.924f, 1}; alu.metal = 1; alu.rough = 0.38f;
        Material acier; acier.color = {0.56f, 0.57f, 0.58f, 1}; acier.metal = 1; acier.rough = 0.6f;
        Material noir; noir.color = {0.035f, 0.035f, 0.038f, 1}; noir.rough = 0.5f; noir.flags = MAT_DOUBLE;   // bague, visière : coques minces
        Material eteint; eteint.color = {0.05f, 0.008f, 0.008f, 1}; eteint.rough = 0.12f;
        Material rouge = eteint; rouge.color = {0.6f, 0.05f, 0.04f, 1}; rouge.emissive = vie::lin(0xff1a1a) * 2.f;   // LED rouge 625 nm
        Material vert = eteint; vert.color = {0.05f, 0.6f, 0.2f, 1}; vert.emissive = vie::lin(0x19ff5a) * 2.f;      // LED verte 525 nm
        mats_[ALU] = r.makeMaterial(alu, nullptr, nullptr, nullptr);
        mats_[ACIER] = r.makeMaterial(acier, nullptr, nullptr, nullptr);
        mats_[NOIR] = r.makeMaterial(noir, nullptr, nullptr, nullptr);
        eteint_ = r.makeMaterial(eteint, nullptr, nullptr, nullptr);
        rouge_ = r.makeMaterial(rouge, nullptr, nullptr, nullptr);
        vert_ = r.makeMaterial(vert, nullptr, nullptr, nullptr);
        actif_ = true;
    }

    void shutdown(Renderer& r) override {
        if (!actif_) return;
        r.vk().waitIdle();
        for (Mesh& m : pieces_) if (m.indexCount) r.destroy(m);
        for (Mesh& m : lentilles_) if (m.indexCount) r.destroy(m);
        for (Material& m : mats_) r.destroy(m);
        r.destroy(eteint_); r.destroy(rouge_); r.destroy(vert_);
        for (Mesh& m : pieces_) m = Mesh{};
        for (Mesh& m : lentilles_) m = Mesh{};
        actif_ = construit_ = false;
    }

    // nuit : les feux allumés éclairent le treillis et la chaussée (une lumière ponctuelle, s'il en reste une de libre)
    void ambiance(FrameParams& fp, const GameView& g) override {
        if (!construit_ || !g.night || g.sceneDediee || fp.spotCount >= FrameParams::SPOTS_MAX) return;
        const int n = feux(nullptr);
        if (n == 0 || length(feuxMonde_ - g.camPos) > 250) return;
        FrameParams::Spot& s = fp.spots[fp.spotCount++];
        // lumière 1,2 m devant le caisson et 0,9 m plus bas (avis de l'agent 3D, PR #431) : elle éclaire le treillis, les pieds et
        // la chaussée sans teinter toute la face du caisson (visières et bagues restent lisibles)
        s.pos = feuxMonde_ + avant_ * 1.2f + vec3{0, -0.9f, 0}; s.target = s.pos + vec3{0, -1, 0}; s.angle = PI; s.decay = 2; s.distance = 22;
        s.color = vert() ? vie::lin(0x19ff5a) : vie::lin(0xff1a1a);
        s.intensity = 0.25f * (float)n;   // 5 lanternes allumées ≈ un lampadaire (1,1 à 26 m) sur une portée plus courte
    }

    void render(Renderer& r, const GameView& g) override {
        if (!actif_ || g.sceneDediee || g.fondFige || !g.track) return;
        if (!construit_) construire(r, *g.track);
        if (!construit_ || length(origine_ - g.camPos) > 900) return;
        const mat4 I = mat4::identity();
        for (int p = 0; p < NB_PIECES - 1; p++) if (pieces_[p].indexCount) r.draw(pieces_[p], mats_[p], I);
        const int n = feux(nullptr);
        const bool v = vert();
        for (int k = 0; k < NB_FEUX; k++) r.draw(lentilles_[k], v ? vert_ : (k < n ? rouge_ : eteint_), I);
    }

    // RG-05, RG-08 : consigne sous les feux pendant le décompte, « GO » à sa place au départ (0,7 s) ; sous le caisson projeté à
    // l'écran (maquette 09), ou en haut de l'écran si le portique n'est pas visible (Grand Prix, caméra sous la poutre)
    void drawUi(Renderer& r, const GameView& g) override {
        if (!ui::fontReady() || g.ecran != Ecran::Course || g.sceneDediee) return;
        const Course* c = activeCourse();
        if (!c || c->timeLeft > 1e8) return;   // banc d'essai sans règles de course
        const bool decompte = c->state == RaceState::Count, go = c->feuxVerts();
        if (!decompte && !go) return;
        const float W = g.uiWidth, H = g.uiHeight, u = H / 100.f;
        float x = W * 0.5f, y = H * 0.26f;
        if (construit_) {
            float sx = 0, sy = 0;
            if (projeter(g, feuxMonde_ - vec3{0, CAISSON_H / 2 + 0.05f, 0}, sx, sy) && sx > 0 && sx < W && sy > -H * 0.2f && sy < H * 0.6f) {
                x = sx; y = std::max(sy + 1.2f * u, g.marges.haut + 2 * u);   // caisson coupé par le haut de l'image : consigne en haut
            }
        }
        if (go) {   // « GO » orange (--accent) en italique, avec le halo de l'ancien décompte
            const float px = 9 * u;
            for (int k = 0; k < 8; k++) {
                const float a = k * PI / 4, rad = px * 0.035f;
                ui::text(r, x + std::cos(a) * rad, y + std::sin(a) * rad, px, "GO", {1.f, 0.416f, 0.f, 0.10f}, ui::CENTRE, 0.04f, false, true);
            }
            ui::text(r, x, y, px, "GO", ui::ORANGE, ui::CENTRE, 0.04f, false, true);
            return;
        }
        const std::string& a = ui::tr("depart.consigne.debut");
        const std::string& b = ui::tr("depart.consigne.gaz");
        const std::string& d = ui::tr("depart.consigne.fin");
        const float px = std::max(14.f, 2.25f * u);
        const float wa = ui::width(px, a, 0.f), wb = ui::width(px, b, 0.f), wd = ui::width(px, d, 0.f), w = wa + wb + wd;
        const float pad = px * 0.9f, h = px * 1.9f, x0 = std::clamp(x - w / 2 - pad, g.marges.gauche + 4.f, W - g.marges.droite - w - 2 * pad - 4.f);
        r.uiRect(x0, y, w + 2 * pad, h, FOND_UI);
        const float ty = y + (h - px) / 2;
        ui::text(r, x0 + pad, ty, px, a, TEXTE_UI, ui::GAUCHE, 0.f, false);
        ui::text(r, x0 + pad + wa, ty, px, b, ui::ORANGE, ui::GAUCHE, 0.f, false);
        ui::text(r, x0 + pad + wa + wb, ty, px, d, TEXTE_UI, ui::GAUCHE, 0.f, false);
    }

private:
    static int feux(const Course** out) {
        const Course* c = activeCourse();
        if (out) *out = c;
        return c ? c->feuxRouges() : 0;
    }
    static bool vert() { const Course* c = activeCourse(); return c && c->feuxVerts(); }

    static bool projeter(const GameView& g, vec3 p, float& sx, float& sy) {
        const mat4 vp = g.proj * g.view;
        const float cx = vp.at(0, 0) * p.x + vp.at(0, 1) * p.y + vp.at(0, 2) * p.z + vp.at(0, 3);
        const float cy = vp.at(1, 0) * p.x + vp.at(1, 1) * p.y + vp.at(1, 2) * p.z + vp.at(1, 3);
        const float cw = vp.at(3, 0) * p.x + vp.at(3, 1) * p.y + vp.at(3, 2) * p.z + vp.at(3, 3);
        if (cw < 0.5f) return false;
        sx = (cx / cw * 0.5f + 0.5f) * g.uiWidth; sy = (cy / cw * 0.5f + 0.5f) * g.uiHeight;
        return true;
    }

    // modèle local (x à droite, y en haut, z vers l'arrière) → monde, au repère de la route en s = 0
    void construire(Renderer& r, Track& t) {
        const vie::Repere p = vie::repere(t, 0);
        const vec3 X = p.r, Y{0, 1, 0}, Z = p.f * -1.f;
        const Modele md = portique_depart::construire(2 * ((float)RW + DEGAGEMENT), HAUTEUR_LIBRE, true);
        auto maillage = [&](const Maille& m) {
            std::vector<Vertex> v; std::vector<uint32_t> i(m.i);
            v.reserve(m.v.size());
            for (const marina::Sommet& s : m.v) {
                const vec3 q = p.o + X * s.p.x + Y * s.p.y + Z * s.p.z, n = X * s.n.x + Y * s.n.y + Z * s.n.z;
                v.push_back({q.x, q.y, q.z, n.x, n.y, n.z, 0.5f, 0.5f});
            }
            return v.empty() ? Mesh{} : r.makeMesh(v, i);
        };
        int tri = 0;
        for (int k = 0; k < NB_PIECES - 1; k++) { pieces_[k] = maillage(md.pieces[k]); tri += md.pieces[k].triangles(); }
        for (int k = 0; k < NB_FEUX; k++) { lentilles_[k] = maillage(md.lentilles[k]); tri += md.lentilles[k].triangles(); }
        origine_ = p.o; avant_ = Z;
        feuxMonde_ = p.o + X * md.centreFeux.x + Y * md.centreFeux.y + Z * md.centreFeux.z;
        construit_ = true;
        WFS_LOG("portique de départ (WFS-559) : ouverture %.2f m, hauteur libre %.2f m, %d triangles, feux à %.2f m", md.ouverture, md.hauteurLibre, tri, md.centreFeux.y);
    }

    bool actif_ = false, construit_ = false;
    Material mats_[NB_PIECES - 1]{}, eteint_{}, rouge_{}, vert_{};
    Mesh pieces_[NB_PIECES - 1]{}, lentilles_[NB_FEUX]{};
    vec3 origine_{}, feuxMonde_{}, avant_{};   // avant_ : vers les voitures qui arrivent (face des feux)
};

} // namespace

WFS_SYSTEM(PortiqueDepart, 112)   // décor (100-199)

} // namespace wfs
