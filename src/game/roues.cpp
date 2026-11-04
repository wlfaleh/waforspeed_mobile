// WaForSpeed — roues animées (WFS-60) : voir roues.h.
#include "roues.h"
#include <algorithm>
#include <cmath>

namespace wfs::roues {

namespace {
struct Boite {
    vec3 mn{1e9f, 1e9f, 1e9f}, mx{-1e9f, -1e9f, -1e9f};
    void ajoute(const vec3& p) {
        mn = {std::min(mn.x, p.x), std::min(mn.y, p.y), std::min(mn.z, p.z)};
        mx = {std::max(mx.x, p.x), std::max(mx.y, p.y), std::max(mx.z, p.z)};
    }
    bool vide() const { return mn.x > mx.x; }
    vec3 taille() const { return mx - mn; }
    vec3 centre() const { return (mn + mx) * 0.5f; }
};
vec3 sommet(const Maillage& m, uint32_t v) { const float* s = m.sommets + (size_t)v * m.stride; return {s[0], s[1], s[2]}; }
bool triangleValide(const Maillage& m, uint32_t t) {
    return m.idx[t * 3] < m.nSommets && m.idx[t * 3 + 1] < m.nSommets && m.idx[t * 3 + 2] < m.nSommets;
}
}

// looksLikeWheel : posée au sol, hauteur d'une roue, ronde (hauteur ≈ longueur), étroite. Largeur 0,5 m au lieu de
// 0,45 : certains modèles ont les roues avant légèrement braquées (911 : 0,46 m)
static bool ressembleRoue(const Boite& b, float rw) {
    const vec3 s = b.taille();
    return b.mn.y < 0.08f && s.y > rw * 1.5f && s.y < rw * 2.6f && std::abs(s.y - s.z) < 0.12f && s.x < 0.5f;
}

Decoupe decouper(const std::vector<Maillage>& ms, float rw) {
    Decoupe d;
    d.roueDuTriangle.resize(ms.size());
    for (size_t mi = 0; mi < ms.size(); mi++) d.roueDuTriangle[mi].assign(ms[mi].nIdx / 3, -1);
    auto coinDe = [](const vec3& c) { return (c.x > 0 ? 1 : 0) + (c.z < 0 ? 0 : 2); };   // LF 0, RF 1, LB 2, RB 3

    // ---------- splitWheels : par maillage ----------
    for (size_t mi = 0; mi < ms.size(); mi++) {
        const Maillage& m = ms[mi];
        const uint32_t nt = m.nIdx / 3;
        if (!nt || !m.sommets || !m.idx) continue;
        Boite bb;
        std::vector<vec3> cen(nt);
        for (uint32_t t = 0; t < nt; t++) {
            if (!triangleValide(m, t)) { cen[t] = {0, 1e9f, 0}; continue; }
            const vec3 a = sommet(m, m.idx[t * 3]), b = sommet(m, m.idx[t * 3 + 1]), c = sommet(m, m.idx[t * 3 + 2]);
            bb.ajoute(a); bb.ajoute(b); bb.ajoute(c);
            cen[t] = (a + b + c) * (1.f / 3);
        }
        if (bb.vide() || bb.mn.y > 0.12f || bb.mx.y > rw * 2.8f) continue;   // pas posé au sol, ou trop haut
        const vec3 sz = bb.taille();
        if (sz.x < 1.0f && sz.z < 1.0f) {                                      // déjà une seule roue
            if (m.nomme || ressembleRoue(bb, rw)) { const int k = coinDe(bb.centre()); for (auto& r : d.roueDuTriangle[mi]) r = (int8_t)k; }
            continue;
        }
        std::vector<uint32_t> parts[4];
        for (uint32_t t = 0; t < nt; t++) if (cen[t].y < 1e8f) parts[coinDe(cen[t])].push_back(t);
        // dans chaque coin, on ne garde que ce qui entoure le centre de la roue (médiane, deux passes)
        const float R = std::min(bb.mx.y, rw * 2.4f) * 0.52f, my = R;
        auto mediane = [&](const std::vector<uint32_t>& l, int axe) {
            std::vector<float> a; a.reserve(l.size());
            for (uint32_t t : l) a.push_back(axe == 0 ? cen[t].x : cen[t].z);
            std::nth_element(a.begin(), a.begin() + a.size() / 2, a.end());
            return a[a.size() / 2];
        };
        int pleins = 0;
        for (auto& l : parts) {
            std::vector<uint32_t> sel = l;
            for (int pass = 0; pass < 2 && !sel.empty(); pass++) {
                const float mz = mediane(sel, 2), mx = mediane(sel, 0);
                std::vector<uint32_t> s2;
                for (uint32_t t : l)
                    if (std::abs(cen[t].y - my) < R && std::abs(cen[t].z - mz) < R && std::abs(cen[t].x - mx) < 0.25f) s2.push_back(t);
                sel.swap(s2);
            }
            l.swap(sel);
            pleins += !l.empty();
        }
        if (pleins < 4) continue;
        // chaque morceau doit avoir la taille d'une roue
        bool ok = true;
        for (const auto& l : parts) {
            Boite b;
            for (uint32_t t : l) for (int k = 0; k < 3; k++) b.ajoute(sommet(m, m.idx[t * 3 + k]));
            const vec3 s = b.taille();
            ok = ok && s.z < 1.0f && s.y < 1.0f && s.x < 0.6f && (m.nomme || ressembleRoue(b, rw));
        }
        if (!ok) continue;
        for (int k = 0; k < 4; k++) for (uint32_t t : parts[k]) d.roueDuTriangle[mi][t] = (int8_t)k;
    }

    // ---------- rigWheels : centre et rayon de chaque roue (boîte de toutes ses pièces) ----------
    Boite coin[4];
    for (size_t mi = 0; mi < ms.size(); mi++)
        for (uint32_t t = 0; t < ms[mi].nIdx / 3; t++) {
            const int k = d.roueDuTriangle[mi][t];
            if (k >= 0) for (int j = 0; j < 3; j++) coin[k].ajoute(sommet(ms[mi], ms[mi].idx[t * 3 + j]));
        }
    bool quatre = true;
    for (int k = 0; k < 4; k++) {
        const vec3 s = coin[k].taille();
        quatre = quatre && !coin[k].vide() && s.y >= 0.25f && s.y <= 1.3f;
    }
    if (!quatre) {   // pas 4 roues : voiture laissée telle quelle
        for (auto& v : d.roueDuTriangle) std::fill(v.begin(), v.end(), (int8_t)-1);
        return d;
    }
    d.n = 4;
    for (int k = 0; k < 4; k++) {
        d.roues[k].centre = coin[k].centre();
        d.roues[k].rayon = coin[k].taille().y * 0.5f;
        d.roues[k].avant = k == LF || k == RF;
    }

    // ---------- jantes et disques fusionnés avec d'autres pièces : triangles dans le cylindre de la roue ----------
    for (size_t mi = 0; mi < ms.size(); mi++) {
        const Maillage& m = ms[mi];
        auto& rt = d.roueDuTriangle[mi];
        if (std::any_of(rt.begin(), rt.end(), [](int8_t r) { return r >= 0; })) continue;   // maillage déjà découpé
        std::vector<uint32_t> cand[4]; uint32_t secteurs[4] = {0, 0, 0, 0};
        for (uint32_t t = 0; t < m.nIdx / 3; t++) {
            if (!triangleValide(m, t)) continue;
            for (int k = 0; k < 4; k++) {
                const RoueModele& r = d.roues[k];
                const float hx = coin[k].taille().x * 0.5f + 0.01f;
                bool dedans = true; vec3 c{};
                for (int j = 0; j < 3 && dedans; j++) {
                    const vec3 p = sommet(m, m.idx[t * 3 + j]);
                    dedans = std::abs(p.x - r.centre.x) <= hx && std::hypot(p.y - r.centre.y, p.z - r.centre.z) <= r.rayon * 0.97f;
                    c += p * (1.f / 3);
                }
                if (!dedans) continue;
                cand[k].push_back(t);
                const float ang = std::atan2(c.y - r.centre.y, c.z - r.centre.z);
                secteurs[k] |= 1u << std::clamp((int)((ang + 3.14159265f) / 6.2831853f * 12), 0, 11);
                break;
            }
        }
        // une jante ou un disque fait le tour de la roue (au moins 10 secteurs sur 12) ; un étrier, non
        for (int k = 0; k < 4; k++) {
            int n = 0; for (int s = 0; s < 12; s++) n += (secteurs[k] >> s) & 1;
            if (n >= 10) for (uint32_t t : cand[k]) rt[t] = (int8_t)k;
        }
    }
    return d;
}

float ackermann(const RoueModele& r, float braquage) {
    if (!r.avant) return 0;
    const bool interieur = (braquage > 0) == (r.centre.x > 0);   // à droite : les roues droites sont à l'intérieur
    return interieur ? 1.12f : 0.9f;
}

mat4 matrice(const RoueModele& r, const RouesPose& p) {
    return translate(r.centre) * rotateY(-p.braquage * ackermann(r, p.braquage)) * rotateX(p.roulage) * translate(-r.centre);
}

static float roulage(double metres, float rayon) {
    const double a = std::fmod(metres / std::max(rayon, 0.05f), 2 * 3.14159265358979);
    return (float)-a;   // w.spin.rotation.x −= meters / rw
}

RouesPose poseJoueur(double steer, double speedUnits, double distUnits, float rayon) {
    const double v = speedUnits * 0.006;   // M
    const double wa = steer * 0.6 / (1 + std::abs(v) / 14);
    return {roulage(distUnits * 0.006, rayon), (float)std::clamp(wa * 1.6, -0.6, 0.6)};
}

RouesPose poseTrafic(double laneYaw, bool crashed, double zUnits, float rayon) {
    return {roulage(zUnits * 0.006, rayon), crashed ? 0.f : (float)(laneYaw * 1.6)};
}

} // namespace wfs::roues
