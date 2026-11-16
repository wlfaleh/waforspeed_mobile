// WaForSpeed — WFS-557 : coffre en bois du cadeau du jour (voir coffre_cadeau.h).
//
// Mesures du réel (coffre de transport en bois résineux, emballage industriel courant, normes d'emballage EN 12246 / NF H 50-001) :
//   - empreinte d'une palette Europe 1 200 × 800 mm ; patins (semelles) 95 × 95 mm sous les grands côtés ;
//   - planches de pin rabotées de 21 mm d'épaisseur et 145 mm de large, joints de 5 mm, arêtes adoucies (chanfrein de 2 mm) ;
//   - montants d'angle 22 × 95 mm vissés sur les faces avant et arrière ;
//   - couvercle en planches de 21 mm sur deux traverses 22 × 70 mm, débord de 10 mm, charnière à l'arrière ;
//   - feuillards de cerclage en acier laqué noir 32 × 0,8 mm (montrés à 2 mm pour rester visibles à l'écran du téléphone).
// Albédo du pin raboté : 0,45 à 0,60 (DA-realisme.md § 3, bois clair), rugosité 0,7 ; grain (cernes et nœuds) en texture.
#include "coffre_cadeau.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include "../core/log.h"
#include "../world/mobilier_ville.h"
#include "garage_commun.h"

namespace wfs::ecrans::coffre {
namespace {

using mobilier::Repere;
using mobilier::Sortie;
using garage::lin;

struct Geo { std::vector<Vertex> v; std::vector<uint32_t> i; };
const Repere MONDE{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

uint32_t g_graine = 0xC0FF7Eu;
float hasard() { g_graine ^= g_graine << 13; g_graine ^= g_graine >> 17; g_graine ^= g_graine << 5; return (float)(g_graine & 0xFFFF) / 65535.f; }

// pièce de bois chanfreinée ; uv en mètres projetés sur le plan le plus face à la normale (le grain suit la longueur de la
// pièce : `fil` = axe de la longueur, 0 x, 1 y, 2 z), décalés au hasard pour que deux planches n'aient pas le même dessin
void piece(Geo& g, vec3 c, vec3 h, float ch, int fil) {
    const size_t d = g.v.size();
    mobilier::pave(Sortie(g), MONDE, c, h, ch, {0, 0});
    const float du = hasard() * 7.f, dv = hasard() * 3.f;
    for (size_t k = d; k < g.v.size(); k++) {
        Vertex& s = g.v[k];
        const float p[3] = {s.px, s.py, s.pz};
        const float ax = std::fabs(s.nx), ay = std::fabs(s.ny), az = std::fabs(s.nz);
        const int n = (ax >= ay && ax >= az) ? 0 : (ay >= az ? 1 : 2);   // axe de la normale
        // u le long du fil quand le fil est dans le plan de la face (sinon bout de la pièce : premier axe du plan), v l'autre axe
        s.u = (fil != n ? p[fil] : p[(n + 1) % 3]) + du;
        s.v = (fil != n ? p[3 - n - fil] : p[(n + 2) % 3]) * 3.f + dv;   // cernes fins (≈ 70 au mètre)
    }
}

Mesh envoyer(Renderer& r, const Geo& g, bool& ok) {
    if (g.i.empty()) return {};
    Mesh m = r.essayerMesh(g.v, g.i);
    if (m.vb.buf == VK_NULL_HANDLE || m.ib.buf == VK_NULL_HANDLE) ok = false;
    return m;
}

} // namespace

Coffre& commun() { static Coffre c; return c; }

bool Coffre::init(Renderer& r) {
    if (pret_) return true;
    g_graine = 0xC0FF7Eu;
    Geo corps[NB_MATS], couv[NB_MATS];
    auto bois = [&](int i) { return (Mat)(PIN_A + i % 3); };
    constexpr float L2 = LARGEUR / 2, P2 = PROFONDEUR / 2, E = EPAISSEUR, ch = 0.002f;
    int n = 0;
    // ---- patins sous les grands côtés
    for (float z : {-P2 + 0.0475f + 0.02f, P2 - 0.0475f - 0.02f}) piece(corps[bois(n++)], {0, PATIN / 2, z}, {L2, PATIN / 2, 0.0475f}, 0.003f, 0);
    // ---- fond (planches le long de x), vu de l'intérieur : éclairé par la lumière du coffre
    for (int k = 0; k < 5; k++) {
        const float z0 = -P2 + E + (PROFONDEUR - 2 * E) * k / 5.f, z1 = -P2 + E + (PROFONDEUR - 2 * E) * (k + 1) / 5.f - JOINT;
        piece(corps[INTERIEUR], {0, PATIN + E / 2, (z0 + z1) / 2}, {L2 - E, E / 2, (z1 - z0) / 2}, ch, 0);
    }
    // ---- parois : 4 rangs de planches ; avant / arrière sur toute la largeur, côtés entre elles
    for (int rang = 0; rang < RANGS; rang++) {
        const float y0 = PATIN + rang * (PLANCHE + JOINT), yc = y0 + PLANCHE / 2;
        for (float z : {-P2 + E / 2, P2 - E / 2}) piece(corps[bois(n++)], {0, yc, z}, {L2, PLANCHE / 2, E / 2}, ch, 0);
        for (float x : {-L2 + E / 2, L2 - E / 2}) piece(corps[bois(n++)], {x, yc, 0}, {E / 2, PLANCHE / 2, P2 - E}, ch, 2);
    }
    // ---- montants d'angle 22 × 95 sur les faces avant et arrière, et lisses haute et basse entre eux
    constexpr float MW = 0.095f, ME = 0.022f;
    for (float sz : {-1.f, 1.f}) {
        const float z = sz * (P2 + ME / 2);
        for (float sx : {-1.f, 1.f}) piece(corps[bois(n++)], {sx * (L2 - MW / 2), (PATIN + HAUT_CORPS) / 2, z}, {MW / 2, (HAUT_CORPS - PATIN) / 2, ME / 2}, 0.003f, 1);
        for (float y : {PATIN + 0.05f, HAUT_CORPS - 0.05f}) piece(corps[bois(n++)], {0, y, z}, {L2 - MW, 0.035f, ME / 2 - 0.001f}, 0.003f, 0);
    }
    // ---- feuillards d'acier noir : deux cerclages verticaux (avant, dessous, arrière), posés sur les lisses et les planches
    constexpr float FL = 0.032f, FE = 0.002f;
    for (float x : {-0.30f, 0.30f}) {
        for (float sz : {-1.f, 1.f}) {
            piece(corps[ACIER], {x, (PATIN + HAUT_CORPS) / 2, sz * (P2 + FE / 2)}, {FL / 2, (HAUT_CORPS - PATIN) / 2, FE / 2}, 0.0005f, 1);
            piece(corps[ACIER], {x, PATIN + 0.05f, sz * (P2 + ME + FE / 2)}, {FL / 2, 0.035f, FE / 2}, 0.0005f, 1);   // sur la lisse basse
            piece(corps[ACIER], {x, HAUT_CORPS - 0.05f, sz * (P2 + ME + FE / 2)}, {FL / 2, 0.035f, FE / 2}, 0.0005f, 1);   // sur la lisse haute
        }
    }
    // ---- couvercle, dans le repère de sa charnière (arête arrière du dessus : y = 0, z = 0 ; le couvercle s'étend vers −z) :
    // 5 planches le long de x, débord de 10 mm, deux traverses dessous, deux feuillards sur le dessus
    constexpr float D = 0.010f, PL = (PROFONDEUR + 2 * D);
    for (int k = 0; k < 5; k++) {
        const float z0 = -PL * k / 5.f, z1 = -PL * (k + 1) / 5.f + JOINT;
        piece(couv[bois(n++)], {0, E / 2, (z0 + z1) / 2}, {L2 + D, E / 2, (z0 - z1) / 2}, ch, 0);
    }
    for (float x : {-0.45f, 0.45f}) piece(couv[bois(n++)], {x, -0.011f, -PL / 2}, {0.035f, 0.011f, P2 - E - 0.002f}, 0.003f, 2);
    for (float x : {-0.30f, 0.30f}) piece(couv[ACIER], {x, E + FE / 2, -PL / 2}, {FL / 2, FE / 2, PL / 2}, 0.0005f, 2);
    // ---- envoi
    bool ok = true;
    triangles_ = 0;
    for (int k = 0; k < NB_MATS; k++) {
        corps_[k] = envoyer(r, corps[k], ok); couvercle_[k] = envoyer(r, couv[k], ok);
        triangles_ += (corps[k].i.size() + couv[k].i.size()) / 3;
    }
    // ---- grain du pin (256 × 256, déterministe) : cernes le long de u (bandes de largeur variable, bois d'été plus foncé),
    // ondulés, deux nœuds ; couleur × 0,72 à 1,08 ; rugosité plus forte dans le bois de printemps
    constexpr int N = 256;
    std::vector<uint8_t> c((size_t)N * N * 4), mr((size_t)N * N * 4);
    const float nx[2] = {0.27f, 0.71f}, ny[2] = {0.38f, 0.82f};
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            const float u = (float)x / N, v = (float)y / N;
            float dv = 0.012f * std::sin(6.2832f * (u * 2.f + v * 0.5f)) + 0.006f * std::sin(6.2832f * u * 7.f);
            for (int k = 0; k < 2; k++) {   // nœuds : les cernes contournent le nœud
                const float du = (u - nx[k]) * 3.f, dw = v - ny[k], d2 = du * du + dw * dw * 9.f;
                dv += 0.03f * std::exp(-d2 * 60.f) * (dw >= 0 ? 1.f : -1.f);
            }
            const float cerne = std::fmod((v + dv) * 23.f + 100.f, 1.f), ete = std::pow(std::max(0.f, std::sin(cerne * 3.14159f)), 6.f);
            float k = 1.02f - 0.24f * ete + 0.05f * (hasard() - 0.5f);
            for (int q = 0; q < 2; q++) {
                const float du = (u - nx[q]) * 3.f, dw = (v - ny[q]) * 3.f;
                if (du * du + dw * dw < 0.0009f) k *= 0.55f;   // cœur du nœud, brun foncé
            }
            const size_t i = ((size_t)y * N + (size_t)x) * 4;
            const uint8_t g = (uint8_t)std::lround(255.f * std::clamp(k * 0.92f, 0.f, 1.f));
            c[i] = c[i + 1] = c[i + 2] = g; c[i + 3] = 255;
            mr[i] = 255; mr[i + 1] = (uint8_t)std::lround(255.f * (0.85f + 0.15f * (1 - ete))); mr[i + 2] = 255; mr[i + 3] = 255;
        }
    grain_ = r.vk().uploadTexture(c.data(), N, N, true);
    grainMR_ = r.vk().uploadTexture(mr.data(), N, N, false);
    auto mat = [&](int k, vec3 col, float rough, float metal, bool grain, vec3 em = {0, 0, 0}) {
        Material m; m.color = {col.x, col.y, col.z, 1}; m.rough = rough; m.metal = metal; m.emissive = em;
        mat_[k] = grain && grain_.view ? r.makeMaterial(m, &grain_, nullptr, &grainMR_) : r.makeMaterial(m, nullptr, nullptr, nullptr);
    };
    mat(PIN_A, lin(0xD2A06A), 0.72f, 0, true);    // pin raboté, teintes de trois planches différentes
    mat(PIN_B, lin(0xC48F59), 0.72f, 0, true);
    mat(PIN_C, lin(0xDAAB76), 0.72f, 0, true);
    mat(ACIER, lin(0x26272A), 0.38f, 0.85f, false);   // feuillard laqué noir
    mat(INTERIEUR, lin(0xD8A870), 0.70f, 0, true, vec3{1.0f, 0.62f, 0.28f} * 6.f);   // fond éclairé par la lumière du cadeau
    if (!ok) { WFS_ERR("coffre du cadeau (WFS-557) : mémoire graphique insuffisante, coffre non dessiné"); shutdown(r); return false; }
    WFS_LOG("coffre du cadeau (WFS-557) : %zu triangles, grain du pin 256 x 256", triangles_);
    return pret_ = true;
}

void Coffre::shutdown(Renderer& r) {
    r.vk().waitIdle();
    for (Mesh& m : corps_) if (m.indexCount) r.destroy(m);
    for (Mesh& m : couvercle_) if (m.indexCount) r.destroy(m);
    for (Material& m : mat_) if (m.set != VK_NULL_HANDLE) r.destroy(m);
    if (grain_.view) r.vk().destroy(grain_);
    if (grainMR_.view) r.vk().destroy(grainMR_);
    for (Mesh& m : corps_) m = Mesh{};
    for (Mesh& m : couvercle_) m = Mesh{};
    for (Material& m : mat_) m = Material{};
    grain_ = Image{}; grainMR_ = Image{};
    pret_ = false;
}

void Coffre::dessiner(Renderer& r, const mat4& place, float ouverture) {
    if (!pret_) return;
    for (int k = 0; k < NB_MATS; k++) if (corps_[k].indexCount) r.draw(corps_[k], mat_[k], place);
    // charnière : arête arrière du dessus du corps ; le devant du couvercle monte (rotation autour de x)
    const mat4 c = place * translate({0, HAUT_CORPS, PROFONDEUR / 2 + 0.010f}) * rotateX(std::clamp(ouverture, 0.f, 1.f) * OUVERTURE_MAX);
    for (int k = 0; k < NB_MATS; k++) if (couvercle_[k].indexCount) r.draw(couvercle_[k], mat_[k], c);
}

void Coffre::eclairer(FrameParams& fp, const mat4& place, float ouverture) const {
    const float o = std::clamp(ouverture, 0.f, 1.f);
    if (!pret_ || o <= 0 || fp.spotCount >= FrameParams::SPOTS_MAX) return;
    const vec3 p = place.transformPoint({0, HAUT_CORPS + 0.25f, -0.10f});   // au-dessus de l'ouverture : éclaire le couvercle et le devant, pas le sol à travers le coffre
    FrameParams::Spot sp;
    sp.pos = p; sp.target = p + vec3{0, 1, 0};
    sp.color = {1.0f, 0.66f, 0.32f};   // lumière chaude (≈ 2 200 K) du cadeau
    sp.intensity = 3.f * o; sp.distance = 3.f; sp.angle = PI; sp.penumbra = 0; sp.decay = 2;
    fp.spots[fp.spotCount++] = sp;
    // reflet chaud du cadeau sur la face avant (maquette 05 : devant du coffre éclairé), lumière ponctuelle basse devant le coffre
    if (fp.spotCount < FrameParams::SPOTS_MAX) {
        FrameParams::Spot av;
        av.pos = place.transformPoint({0, HAUT_CORPS * 0.6f, -PROFONDEUR / 2 - 0.9f}); av.target = place.transformPoint({0, HAUT_CORPS * 0.5f, 0});
        av.color = {1.0f, 0.78f, 0.52f}; av.intensity = 4.f * o; av.distance = 3.f; av.angle = PI; av.decay = 2;
        fp.spots[fp.spotCount++] = av;
    }
}

} // namespace wfs::ecrans::coffre
