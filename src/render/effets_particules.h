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

// WFS-107 (RG-04) : lumière émise par la fumée (couleur du jeu web × fondu, k du jeu web) : gris moyen des bouffées
// (k = 0,25 à 0,40 → 0,32, bleu × 1,1) converti par versHdr, comme les étincelles : ressort à l'écran avec le gris du jeu
// web, jamais blanc, sous le seuil du bloom de nuit
inline vec3 fumeeHdr(float fondu, float exposition) { return versHdr(vec3{0.32f, 0.32f, 0.352f} * std::clamp(fondu, 0.f, 1.f), exposition); }

// WFS-224 (RG-02, ANO-120) : texture des volutes de fumée. Le profil TEXS.radial (alpha 1 au centre, 0,45 à 35 % du
// rayon) donnait des disques nets et pleins (« boules grises ») : profil gaussien exp(−4 r²) ramené à 0 au bord (aucune
// marche), modulé par un bruit de valeur lisse (4 × 4 cellules, interpolation cubique) qui casse le rond en volutes.
// (x, y) dans [−1, 1] ; rend l'alpha de la texture (0..1), 0 hors du cercle unité.
inline float bruitVolute(float x, float y) {
    auto h = [](int i, int j) {   // valeur pseudo-aléatoire 0..1 d'un nœud (hachage entier, déterministe)
        uint32_t n = (uint32_t)(i * 374761393 + j * 668265263); n = (n ^ (n >> 13)) * 1274126177u;
        return ((n ^ (n >> 16)) & 0xffff) / 65535.f;
    };
    const float u = (x + 1) * 2, v = (y + 1) * 2;   // 4 cellules sur le carré
    const int i = (int)std::floor(u), j = (int)std::floor(v);
    float fu = u - i, fv = v - j; fu = fu * fu * (3 - 2 * fu); fv = fv * fv * (3 - 2 * fv);
    const float a = h(i, j) + (h(i + 1, j) - h(i, j)) * fu, b = h(i, j + 1) + (h(i + 1, j + 1) - h(i, j + 1)) * fu;
    return a + (b - a) * fv;
}
inline float voluteAlpha(float x, float y) {
    const float r2 = x * x + y * y;
    if (r2 >= 1) return 0;
    const float e1 = std::exp(-4.f);
    const float profil = (std::exp(-4.f * r2) - e1) / (1 - e1);
    return profil * (0.55f + 0.45f * bruitVolute(x, y));
}
// WFS-224 (RG-02) : vie d'une volute (age = 1 − fondu, 0 → 1) : elle apparaît en 0,15 de sa vie (pas de disque qui surgit),
// puis s'efface en douceur (fondu^1,5) ; sa taille grandit de 0,5 à 2 fois la taille de base (la fumée se dilate)
inline float voluteOpacite(float fondu) {
    fondu = std::clamp(fondu, 0.f, 1.f);
    const float age = 1 - fondu;
    return std::min(1.f, age / 0.15f) * std::pow(fondu, 1.5f);
}
inline float voluteEchelle(float fondu) { return 0.5f + 1.5f * (1 - std::clamp(fondu, 0.f, 1.f)); }
// WFS-224 (RG-02, ANO-120) : une volute qui passe près de la caméra (la voiture la laisse derrière elle) remplissait
// l'écran jusqu'au compteur : effacée en dessous de 1,5 m de profondeur, entière au-delà de 4 m
inline float voluteProche(float profondeur) {
    const float t = std::clamp((profondeur - 1.5f) / 2.5f, 0.f, 1.f);
    return t * t * (3 - 2 * t);
}

// WFS-224 (RG-01) : vrai dérapage, seul cas où les pneus fument (plus à chaque coup de volant ni à chaque freinage) :
// - essieu arrière au-delà du pic d'adhérence (sigmaAr > 1, conduite_monde.h) ET dérive de la caisse > 8° (pic du pneu
//   vers 7°, C_PNEU de conduite_monde.h) ET vitesse > 15 m/s (54 km/h) ;
// - ou roues bloquées (glissement > 0,5 : ABS coupé) au-dessus de 8 m/s ;
// - ou tête-à-queue d'accident (lacet ajouté > 0,3 rad, comme avant).
inline bool vraiDerapage(double vitesse, double derive, double sigmaAr, double glissementBloque, double lacetAccident) {
    if (std::abs(lacetAccident) > 0.3) return true;
    if (glissementBloque > 0.5 && vitesse > 8) return true;
    return vitesse > 15 && std::abs(derive) > 0.14 && sigmaAr > 1;
}

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
    // WFS-107 (RG-06) : « réduire les mouvements » (reduceMotion du jeu web) : plus aucune particule émise, ni étincelle
    // ni fumée (emit() du jeu web) ; celles déjà en vie finissent leur course
    void setReduireMouvements(bool r) { reduireMouvements_ = r; }
    // étincelles au point de contact ; vel = vitesse du véhicule dans le monde (m/s) ; floor = hauteur du sol ;
    // rend le nombre réellement émis (moitié en qualité basse, aucune si « réduire les mouvements »)
    int etincelles(const vec3& at, float floor, const vec3& vel, int count = 40);
    // sol : hauteur de la route sous la bouffée (affichage : le carré ne passe pas sous la route) ; par défaut at.y − 0,5 ;
    // rend le nombre de bouffées émises
    // WFS-224 : vel = vitesse de départ ajoutée (m/s), par défaut nulle (jeu web)
    int fumee(const vec3& at, int count = 2, float sol = -1e9f, const vec3& vel = {});
    void update(float dt);
    void vider();
    const Reserve& sparks() const { return sparks_; }
    const Reserve& smoke() const { return smoke_; }
    uint32_t graine = 0x2545F491u;   // tirages déterministes (Math.random du jeu web)

private:
    float rnd();                     // [0, 1)
    int emit(Reserve& s, const vec3& at, float floor, int count, float spread, const vec3& base, int couleur, float life);
    Reserve sparks_, smoke_;
    bool reduit_ = false, reduireMouvements_ = false;
};

} // namespace wfs::effets
