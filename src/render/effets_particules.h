// WaForSpeed — particules des effets de conduite (WFS-65, épic E4) : étincelles des chocs et fumée.
// Port de js/world/particles.js du jeu web (makeParticleSystem, emit, emitSparks, emitSmoke, updateParticles) :
// mêmes tailles de réserve (220 étincelles, 140 bouffées), mêmes vitesses, durées, couleurs, gravité, frottement
// et rebond des étincelles sur le sol. Calcul CPU seul (aucun appel Vulkan) : testé par tests/test_effets.cpp.
// Les effets ne lisent que l'état du jeu : ils ne touchent ni la physique ni le chrono (RG-06).
#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>
#include "../core/math.h"

namespace wfs::effets {

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
    // étincelles au point de contact ; vel = vitesse du véhicule dans le monde (m/s) ; floor = hauteur du sol
    void etincelles(const vec3& at, float floor, const vec3& vel, int count = 40);
    void fumee(const vec3& at, int count = 2);
    void update(float dt);
    void vider();
    const Reserve& sparks() const { return sparks_; }
    const Reserve& smoke() const { return smoke_; }
    uint32_t graine = 0x2545F491u;   // tirages déterministes (Math.random du jeu web)

private:
    float rnd();                     // [0, 1)
    void emit(Reserve& s, const vec3& at, float floor, int count, float spread, const vec3& base, int couleur, float life);
    Reserve sparks_, smoke_;
    bool reduit_ = false;
};

} // namespace wfs::effets
