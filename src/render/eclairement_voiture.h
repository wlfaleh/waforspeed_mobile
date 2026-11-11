#pragma once
// WaForSpeed — WFS-423 (RG-02) : éclairement de la chaussée autour d'une voiture la nuit, calculé UNE fois par voiture sur le CPU.
// Même formule que eclairementRouteAutour (shaders/vehicule.glsl, WFS-270) : 3 points au sol (4 m, 6,5 m et 10 m devant le centre
// de la voiture, 2 cm au-dessus), lumières de l'image telles qu'envoyées au shader (tableaux spot* de FrameUBO, déjà pré-exposés).
// Le shader recevait le même résultat pour tous les pixels de la voiture et le recalculait pixel par pixel (3 × 12 lumières) :
// en Basse, il lit maintenant cette valeur dans les constantes de la voiture (Renderer::draw, PC::nuit).
// Fichier sans Vulkan : testé par tests/test_eclairement_voiture.cpp.
#include <algorithm>
#include <cmath>

#include "../core/math.h"
#include "phares.h"

namespace wfs::eclairement_voiture {

constexpr int LUMIERES_MAX = 12;   // taille des tableaux spot* de FrameUBO (FrameParams::SPOTS_MAX)

// lumières de l'image, au format du shader (renderer.cpp, beginFrame) : pos.w = portée, dir.w = décroissance,
// cone = (cos angle, cos pénombre, 1 si feu de croisement, 0)
struct Lumieres {
    int n = 0;
    vec4 pos[LUMIERES_MAX], dir[LUMIERES_MAX], col[LUMIERES_MAX], cone[LUMIERES_MAX];
};

// smoothstep de GLSL ; bords égaux (pénombre nulle) : marche nette au lieu d'une division par zéro
inline float lisse(float a, float b, float x) {
    if (b == a) return x < a ? 0.f : 1.f;
    const float t = std::clamp((x - a) / (b - a), 0.f, 1.f);
    return t * t * (3 - 2 * t);
}

// feuCroisement de shaders/eclairage.glsl (WFS-218, 382, 413) : w = direction phare → point, D = axe du phare (horizontal),
// d = distance, range = portée. La répartition est phares::intensiteRelative (mêmes constantes que le shader).
inline float feuCroisement(const vec3& w, const vec3& D, float d, float range) {
    const vec3 R = normalize(cross(D, vec3{0, 1, 0})), U = cross(R, D);
    constexpr float DEG = 180.f / PI;
    const float el = DEG * std::asin(std::clamp(dot(w, U), -1.f, 1.f)), az = DEG * std::atan2(dot(w, R), dot(w, D));
    return phares::intensiteRelative(el, az) * (1.f - lisse(0.8f * range, range, d)) / std::max(d * d, 0.25f)
         * lisse(0.40f, 0.70f, d * dot(w, D));   // WFS-413 : boîtier et bouclier cachent la lampe à moins de 40 cm devant la glace
}

// éclairement moyen de la chaussée devant la voiture (lux pré-exposés, × couleur) ; model = matrice monde de la pièce dessinée
inline vec3 routeAutour(const Lumieres& L, const mat4& model) {
    const vec3 o{model.at(0, 3), model.at(1, 3), model.at(2, 3)};
    vec3 av{model.at(0, 2), 0, model.at(2, 2)};   // avant du modèle = −Z local (phares en −L/2, nuit.cpp)
    av = dot(av, av) > 1e-8f ? normalize(av) * -1.f : vec3{0, 0, 1};
    const int n = std::clamp(L.n, 0, LUMIERES_MAX);
    constexpr float DEVANT[3] = {4.f, 6.5f, 10.f};   // m devant le centre de la voiture
    vec3 E{0, 0, 0};
    for (float devant : DEVANT) {
        const vec3 p = o + av * devant + vec3{0, 0.02f, 0};
        for (int i = 0; i < n; i++) {
            const vec3 sp{L.pos[i].x, L.pos[i].y, L.pos[i].z}, sd{L.dir[i].x, L.dir[i].y, L.dir[i].z};
            const vec3 Lv = sp - p;
            const float d = length(Lv);
            const vec3 Ld = Lv * (1.f / std::max(d, 1e-4f));
            const float cosT = -dot(Ld, sd), NoL = std::max(Ld.y, 0.f);
            const bool croisement = L.cone[i].z > 0.5f;
            if ((!croisement && cosT <= L.cone[i].x) || (croisement && (d > L.pos[i].w || cosT < 0.17f)) || NoL <= 0.f) continue;
            if (d < 2.f) continue;   // champ proche d'une optique (1/d² non borné) : pas un éclairement de la chaussée voisine
            const float range = L.pos[i].w, decay = L.dir[i].w;
            const float att = (range > 0 && decay > 0) ? std::pow(std::clamp(1.f - d / range, 0.f, 1.f), decay) : 1.f;
            const float k = (croisement ? feuCroisement(Ld * -1.f, sd, d, range) : lisse(L.cone[i].x, L.cone[i].y, cosT) * att) * NoL;
            E = E + vec3{L.col[i].x, L.col[i].y, L.col[i].z} * k;
        }
    }
    return E * (1.f / 3.f);
}

}  // namespace wfs::eclairement_voiture
