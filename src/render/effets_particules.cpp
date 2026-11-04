// WaForSpeed — particules des effets de conduite (WFS-65) : voir effets_particules.h.
#include "effets_particules.h"
#include <algorithm>
#include <cmath>

namespace wfs::effets {

int Reserve::vivantes() const { int n = 0; for (const auto& q : p) n += q.vivante(); return n; }

Particules::Particules() {
    // setupParticles : makeParticleSystem(220, 0.22, additif, 1) et makeParticleSystem(140, 2.4, normal, 0.45)
    sparks_.p.resize(220); sparks_.taille = 0.22f; sparks_.opacite = 1.f;
    sparks_.grav = 14.f; sparks_.drag = 1.5f; sparks_.rebond = true;
    smoke_.p.resize(140); smoke_.taille = 2.4f; smoke_.opacite = 0.45f;
    smoke_.grav = -0.4f; smoke_.drag = 0.8f; smoke_.rebond = false;
}

// xorshift32 → [0, 1)
float Particules::rnd() {
    graine ^= graine << 13; graine ^= graine >> 17; graine ^= graine << 5;
    return (graine >> 8) * (1.f / 16777216.f);
}

// emit : vitesse de base + dispersion, durée × (0,6 à 1,2), couleur tirée au moment de l'émission
void Particules::emit(Reserve& s, const vec3& at, float floor, int count, float spread, const vec3& base, int couleur, float life) {
    if (reduit_) count = (count + 1) / 2;
    for (int i = 0; i < count; i++) {
        Particule& q = s.p[(size_t)s.next]; s.next = (s.next + 1) % (int)s.p.size();
        q.p = at;
        q.v = {base.x + (rnd() - 0.5f) * spread, base.y + rnd() * spread * 0.6f, base.z + (rnd() - 0.5f) * spread};
        q.max = q.life = life * (0.6f + rnd() * 0.6f);
        if (couleur == 0) q.c = rnd() < 0.3f ? vec3{1, 0.95f, 0.8f} : vec3{1, 0.45f + rnd() * 0.3f, 0.1f};   // étincelles
        else { const float k = 0.25f + rnd() * 0.15f; q.c = {k, k, k * 1.1f}; }                            // fumée
        q.floor = floor;
    }
}

// emitSparks(at, vel, count) : dispersion 9, durée 0,55 s
void Particules::etincelles(const vec3& at, float floor, const vec3& vel, int count) { emit(sparks_, at, floor, count, 9.f, vel, 0, 0.55f); }
// emitSmoke(at, count) : dispersion 0,8, montée de 1,2 m/s, durée 2,2 s ; sol : at.y − 0,5 (sans rebond)
void Particules::fumee(const vec3& at, int count, float sol) { emit(smoke_, at, sol > -1e8f ? sol : at.y - 0.5f, count, 0.8f, {0, 1.2f, 0}, 1, 2.2f); }

// updateParticles : gravité (14 vers le bas pour les étincelles, 0,4 vers le haut pour la fumée), frottement de l'air,
// rebond des étincelles sur le sol (vitesse verticale × −0,35)
void Particules::update(float dt) {
    for (Reserve* s : {&sparks_, &smoke_}) {
        for (Particule& q : s->p) {
            if (q.life <= 0) continue;
            q.life -= dt;
            q.v.y -= s->grav * dt; q.v.x *= 1 - s->drag * dt; q.v.z *= 1 - s->drag * dt;
            q.p += q.v * dt;
            if (s->rebond && q.p.y < q.floor) { q.p.y = q.floor; q.v.y *= -0.35f; }
        }
    }
}

void Particules::vider() { for (Reserve* s : {&sparks_, &smoke_}) for (Particule& q : s->p) q.life = 0; }

} // namespace wfs::effets
