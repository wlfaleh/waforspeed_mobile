// WaForSpeed — particules des effets de conduite (WFS-65, épic E4) : étincelles des chocs et fumée.
// Port de js/world/particles.js du jeu web (makeParticleSystem, emit, emitSparks, emitSmoke, updateParticles) :
// mêmes tailles de réserve (220 étincelles, 140 bouffées), mêmes vitesses, durées, couleurs, gravité, frottement
// et rebond des étincelles sur le sol. Calcul CPU seul (aucun appel Vulkan) : testé par tests/test_effets.cpp.
// Les effets ne lisent que l'état du jeu : ils ne touchent ni la physique ni le chrono (RG-06).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include "../core/math.h"

namespace wfs::effets {

// WFS-101 (RG-01) : nombre d'étincelles par choc, comme les appels d'emitSparks de js/game/update.js
enum class Choc { ArrierePercute, PercuteParArriere, Frottement, Decor };
inline int nombreEtincelles(Choc c, double impact) {
    impact = std::max(0.0, impact);
    switch (c) {
        case Choc::ArrierePercute: return 20 + (int)std::lround(impact * 80);      // hitCar, on percute l'arrière (dz > 0)
        case Choc::PercuteParArriere: return 16 + (int)std::lround(impact * 50);   // hitCar, une voiture nous rentre dedans
        case Choc::Frottement: return 14;                                          // contact latéral
        default: return 50;                                                        // crash() : lampadaire, panneau
    }
}

// WFS-101 : couleur d'affichage (0-1, réglée comme les couleurs du jeu web) → valeur HDR qui ressort à l'écran avec
// cette couleur. Jeu web (renderer.js, makeGradeShader) : écran = (aces(c^2,2) / aces(1,6))^(1/2,2) ; natif
// (shaders/post.frag) : écran = aces(x × exposition)^(1/2,2) → x = aces⁻¹(aces(c^2,2) / aces(1,6)) / exposition.
// L'ancienne approximation aces(x) ≈ 0,214·x ne vaut que tout près du noir : la fumée grise des pneus sortait presque
// blanche (« feu blanc » en tournant aux flèches).
inline float aces(float x) { return std::clamp(x * (2.51f * x + 0.03f) / (x * (2.43f * x + 0.59f) + 0.14f), 0.f, 1.f); }
inline float acesInverse(float y) {   // racine positive de (2,51 − 2,43y)x² + (0,03 − 0,59y)x − 0,14y = 0
    y = std::clamp(y, 0.f, 0.999f);
    const float a = 2.51f - 2.43f * y, b = 0.03f - 0.59f * y, c = -0.14f * y;
    return (-b + std::sqrt(b * b - 4 * a * c)) / (2 * a);
}
inline float versHdr(float c, float exposition) {
    const float web = aces(std::pow(std::clamp(c, 0.f, 1.f), 2.2f)) / aces(1.6f);
    return acesInverse(web) / exposition;
}
inline vec3 versHdr(const vec3& c, float exposition) { return {versHdr(c.x, exposition), versHdr(c.y, exposition), versHdr(c.z, exposition)}; }

struct Particule {
    float life = 0, max = 1;     // durée restante, durée totale (s)
    vec3 p{0, -999, 0}, v{};     // position (m), vitesse (m/s)
    vec3 c{};                    // couleur d'origine (0-1, comme les couleurs de sommet du jeu web)
    float floor = -1e9f;         // hauteur du sol sous le point d'émission (rebond des étincelles)
    bool vivante() const { return life > 0; }
    float fondu() const { return max > 0 ? std::max(0.f, life / max) : 0.f; }   // k du jeu web
};

struct Reserve {
    std::vector<Particule> p;
    int next = 0;
    float grav = 0, drag = 0, taille = 0, opacite = 1;
    bool rebond = false;
    int vivantes() const;
};

class Particules {
public:
    Particules();
    // qualité basse (RG-05) : moitié moins de particules émises
    void setReduit(bool r) { reduit_ = r; }
    // étincelles au point de contact ; vel = vitesse du véhicule dans le monde (m/s) ; floor = hauteur du sol ;
    // rend le nombre réellement émis (moitié en qualité basse)
    int etincelles(const vec3& at, float floor, const vec3& vel, int count = 40);
    // sol : hauteur de la route sous la bouffée (affichage : le carré ne passe pas sous la route) ; par défaut at.y − 0,5
    void fumee(const vec3& at, int count = 2, float sol = -1e9f);
    void update(float dt);
    void vider();
    const Reserve& sparks() const { return sparks_; }
    const Reserve& smoke() const { return smoke_; }
    uint32_t graine = 0x2545F491u;   // tirages déterministes (Math.random du jeu web)

private:
    float rnd();                     // [0, 1)
    int emit(Reserve& s, const vec3& at, float floor, int count, float spread, const vec3& base, int couleur, float life);
    Reserve sparks_, smoke_;
    bool reduit_ = false;
};

} // namespace wfs::effets
