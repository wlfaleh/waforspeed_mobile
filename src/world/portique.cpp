// WaForSpeed — portique du prochain checkpoint (WFS-71) : port de setGantry (js/world/scene.js) du jeu web.
// Deux piliers de 9 m de part et d'autre de la route, poutre sombre de 2,4 m, deux barres néon cyan, une bande au sol,
// panneau « CHECKPOINT n » lumineux des deux côtés. Placé au prochain checkpoint de la course (lecture seule de
// activeCourse()->nextCp), reconstruit à chaque checkpoint passé. Ville et Corniche (RG-09) ; la fonction
// construirePortique est indépendante de la carte (Grand Prix : WFS-53).
#include <cmath>
#include <cstring>
#include <string>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/course_system.h"
#include "../game/player.h"
#include "../game/track.h"
#include "enseignes.h"

namespace wfs {
namespace {
using namespace vie;

constexpr unsigned CYAN = 0x00E5FF;

class Portique : public System {
public:
    const char* name() const override { return "portique"; }
    void init(Renderer& r, const GameView& g) override {
        actif_ = false; cle_ = -1;
        if (!g.map) return;
        if (std::strcmp(g.map->id, "ville") && std::strcmp(g.map->id, "plage")) return;   // RG-09
        atlas_.ajouter("CHECKPOINT");
        for (int d = 0; d <= 9; d++) atlas_.ajouter(std::to_string(d));
        if (!atlas_.construire(r)) return;
        uvPoteau_ = palette_.uv(0x2a2d33); uvFond_ = palette_.uv(0x0b0c14);
        palette_.construire(r);
        neon_ = neon(r, CYAN); atlas_.materiau(r, CYAN, 2.0f);
        actif_ = true;
    }
    void render(Renderer& r, const GameView& g) override {
        if (!actif_ || g.sceneDediee || !g.player || !g.track) return;
        const Course* c = activeCourse();
        // solo en course (décompte compris) : portique au prochain checkpoint ; sinon rien (menu, fin, scènes du banc)
        const bool visible = c && g.ecran == Ecran::Course && c->state != RaceState::Over && c->timeLeft < 1e6;
        if (!visible) return;
        const Player& pl = *g.player;
        const double s = pl.position + (c->nextCp - pl.dist);   // abscisse du checkpoint sur la piste (unités)
        const long long cle = (long long)c->level * 100000 + (long long)std::llround(c->nextCp);
        if (cle != cle_) { construire(r, *g.track, s, c->level + 1); cle_ = cle; }
        const mat4 I = mat4::identity();
        if (fixe_.indexCount) r.draw(fixe_, palette_.materiau, I);
        if (lumiere_.indexCount) r.draw(lumiere_, neon_, I);
        if (texte_.indexCount) r.draw(texte_, atlas_.materiau(r, CYAN, 2.0f), I);
    }
    void shutdown(Renderer& r) override {
        r.vk().waitIdle();
        liberer(r);
        atlas_.detruire(r); palette_.detruire(r);
        actif_ = false;
    }
private:
    void liberer(Renderer& r) { for (Mesh* m : {&fixe_, &lumiere_, &texte_}) if (m->indexCount) { r.destroy(*m); *m = Mesh{}; } }
    // setGantry(`CHECKPOINT ${level + 1}`, '#00E5FF', nextCp)
    void construire(Renderer& r, Track& t, double s, int numero) {
        r.vk().waitIdle();   // une fois par checkpoint : les anciens maillages ne sont plus utilisés par une image en vol
        liberer(r);
        Geo B, N, T;
        const Repere p = repere(t, s);
        const float R = (float)RW;
        boite(B, lateral(p, -(R + 1), 0), p.r, p.f, 0.6f, 9, 0.6f, uvPoteau_);
        boite(B, lateral(p, R + 1, 0), p.r, p.f, 0.6f, 9, 0.6f, uvPoteau_);
        boite(B, lateral(p, 0, 9), p.r, p.f, (R + 1.6f) * 2, 2.4f, 0.7f, uvFond_);
        boite(N, lateral(p, 0, 11.4f), p.r, p.f, (R + 1.6f) * 2, 0.16f, 0.8f, {0, 0});
        boite(N, lateral(p, 0, 8.86f), p.r, p.f, (R + 1.6f) * 2, 0.16f, 0.8f, {0, 0});
        boite(N, lateral(p, 0, 0.02f), p.r, p.f, R * 2, 0.02f, 0.5f, {0, 0});
        // panneau : « CHECKPOINT » puis le numéro, centrés, sur les deux faces de la poutre (hauteur des lettres 1,5 m)
        const std::string num = std::to_string(numero);
        const float h = 1.5f, esp = 0.5f;
        float larg = atlas_.texte("CHECKPOINT").aspect * h + esp;
        for (char ch : num) larg += atlas_.texte(std::string(1, ch)).aspect * h;
        for (int face : {-1, 1}) {
            const vec3 droite = p.r * (float)-face, n = p.f * (float)face;
            float x = -larg / 2;
            auto mot = [&](const std::string& m) {
                const Texte& tx = atlas_.texte(m);
                const float w = tx.aspect * h;
                plan(T, lateral(p, 0, 10.2f) + droite * (x + w / 2) + p.f * (0.37f * face), droite, {0, 1, 0}, n, w, h, tx.u0, tx.v0, tx.u1, tx.v1);
                x += w;
            };
            mot("CHECKPOINT"); x += esp;
            for (char ch : num) mot(std::string(1, ch));
        }
        fixe_ = r.makeMesh(B.v, B.i); lumiere_ = r.makeMesh(N.v, N.i); texte_ = r.makeMesh(T.v, T.i);
        WFS_LOG("vie : portique « CHECKPOINT %d »", numero);
    }
    bool actif_ = false;
    long long cle_ = -1;
    AtlasTextes atlas_; Palette palette_;
    vec2 uvPoteau_, uvFond_;
    Material neon_;
    Mesh fixe_, lumiere_, texte_;
};

} // namespace

WFS_SYSTEM(Portique, 108)   // décor (100-199)

} // namespace wfs
