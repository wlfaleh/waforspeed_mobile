// WaForSpeed — personnages animés par squelette (WFS-63) : voir squelette.h et tools/convert_people.mjs (format .wfp).
#include "squelette.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "../core/log.h"
#include "../core/platform.h"

#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#include "../../third_party/stb/stb_image.h"   // implémentation dans model_loader.cpp

namespace wfs {

namespace {
struct Lecteur {
    const uint8_t* p; const uint8_t* fin; bool ok = true;
    template <class T> T lire() { T v{}; if (p + sizeof(T) > fin) { ok = false; return v; } std::memcpy(&v, p, sizeof(T)); p += sizeof(T); return v; }
    const uint8_t* prendre(size_t n) { if (p + n > fin) { ok = false; return nullptr; } const uint8_t* r = p; p += n; return r; }
    std::string nom32() { const uint8_t* b = prendre(32); if (!b) return {}; return std::string((const char*)b, strnlen((const char*)b, 32)); }
    void floats(float* d, size_t n) { const uint8_t* b = prendre(n * 4); if (b) std::memcpy(d, b, n * 4); }
};

Quat normaliser(Quat q) {
    const float l = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    return l > 1e-8f ? Quat{q.x / l, q.y / l, q.z / l, q.w / l} : Quat{};
}
// interpolation sphérique (chemin le plus court) ; linéaire normalisée quand les deux sont presque égales
Quat slerp(Quat a, Quat b, float t) {
    float d = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    if (d < 0) { b = {-b.x, -b.y, -b.z, -b.w}; d = -d; }
    float ka = 1 - t, kb = t;
    if (d < 0.9995f) {
        const float th = std::acos(d), s = std::sin(th);
        ka = std::sin((1 - t) * th) / s; kb = std::sin(t * th) / s;
    }
    return normaliser({a.x * ka + b.x * kb, a.y * ka + b.y * kb, a.z * ka + b.z * kb, a.w * ka + b.w * kb});
}
// T × R × S (colonne-majeure, comme glTF)
mat4 trs(const vec3& t, const Quat& q, const vec3& s) {
    const float x = q.x, y = q.y, z = q.z, w = q.w;
    mat4 m = mat4::identity();
    m.at(0, 0) = (1 - 2 * (y * y + z * z)) * s.x; m.at(0, 1) = (2 * (x * y - z * w)) * s.y;     m.at(0, 2) = (2 * (x * z + y * w)) * s.z;
    m.at(1, 0) = (2 * (x * y + z * w)) * s.x;     m.at(1, 1) = (1 - 2 * (x * x + z * z)) * s.y; m.at(1, 2) = (2 * (y * z - x * w)) * s.z;
    m.at(2, 0) = (2 * (x * z - y * w)) * s.x;     m.at(2, 1) = (2 * (y * z + x * w)) * s.y;     m.at(2, 2) = (1 - 2 * (x * x + y * y)) * s.z;
    m.at(0, 3) = t.x; m.at(1, 3) = t.y; m.at(2, 3) = t.z;
    return m;
}
// clé k telle que temps[k] ≤ t < temps[k+1] (recherche dichotomique) et facteur d'interpolation
void cle(const std::vector<float>& tm, float t, size_t& k, float& f) {
    if (tm.size() < 2 || t <= tm.front()) { k = 0; f = 0; return; }
    if (t >= tm.back()) { k = tm.size() - 1; f = 0; return; }
    k = (size_t)(std::upper_bound(tm.begin(), tm.end(), t) - tm.begin()) - 1;
    const float d = tm[k + 1] - tm[k];
    f = d > 0 ? (t - tm[k]) / d : 0;
}
} // namespace

bool chargerPersonnage(Renderer& r, const std::string& chemin, Personnage& out) {
    std::vector<uint8_t> fichier;
    if (!readAsset(chemin, fichier)) { WFS_ERR("Personnage introuvable : %s", chemin.c_str()); return false; }
    Lecteur l{fichier.data(), fichier.data() + fichier.size()};
    const uint8_t* magie = l.prendre(4);
    // WFS-72 : version 2 (attribut _cloth par sommet, repères du corps) ; la version 1 de WFS-63 n'est plus lue
    if (!magie || std::memcmp(magie, "WFP1", 4) != 0 || l.lire<uint32_t>() != 2) { WFS_ERR("%s : format inconnu (version 2 attendue)", chemin.c_str()); return false; }
    const uint32_t nTex = l.lire<uint32_t>();
    std::vector<std::pair<const uint8_t*, uint32_t>> blobs(nTex);
    for (auto& b : blobs) { const uint32_t n = l.lire<uint32_t>(); b = {l.prendre(n), n}; }
    const float rugo = l.lire<float>(), metal = l.lire<float>();
    out.hauteur = l.lire<float>();
    { float c[4]; l.floats(c, 4); out.corps = {c[0], c[1], c[2], c[3]}; }   // uBody du jeu web
    const uint32_t nV = l.lire<uint32_t>(), nI = l.lire<uint32_t>(), nP = l.lire<uint32_t>(), nO = l.lire<uint32_t>(), nC = l.lire<uint32_t>();
    std::vector<SkinVertex> v(nV);
    if (const uint8_t* b = l.prendre((size_t)nV * sizeof(SkinVertex))) std::memcpy(v.data(), b, (size_t)nV * sizeof(SkinVertex));
    std::vector<uint32_t> idx(nI);
    if (const uint8_t* b = l.prendre((size_t)nI * 4)) std::memcpy(idx.data(), b, (size_t)nI * 4);
    out.plages.resize(nP);
    for (Plage& p : out.plages) { p.nom = l.nom32(); p.premier = l.lire<uint32_t>(); p.nombre = l.lire<uint32_t>(); }
    out.os.resize(nO);
    for (OsSquelette& o : out.os) {
        o.nom = l.nom32(); o.parent = l.lire<int32_t>();
        float f[26]; l.floats(f, 26);
        o.t = {f[0], f[1], f[2]}; o.r = {f[3], f[4], f[5], f[6]}; o.s = {f[7], f[8], f[9]};
        std::memcpy(o.bindInverse.m, f + 10, 64);
    }
    const uint32_t nPal = l.lire<uint32_t>();
    out.palette.resize(nPal);
    for (int& o : out.palette) o = (int)l.lire<uint32_t>();
    out.clips.resize(nC);
    for (ClipAnim& c : out.clips) {
        c.nom = l.nom32(); c.duree = l.lire<float>();
        c.canaux.resize(l.lire<uint32_t>());
        for (CanalAnim& k : c.canaux) {
            k.os = (int)l.lire<uint32_t>(); k.chemin = (int)l.lire<uint32_t>();
            const uint32_t n = l.lire<uint32_t>();
            if (!l.ok || n > 100000) { l.ok = false; break; }
            k.temps.resize(n); k.valeurs.resize((size_t)n * (k.chemin == 1 ? 4 : 3));
            l.floats(k.temps.data(), n); l.floats(k.valeurs.data(), k.valeurs.size());
        }
    }
    bool ok = l.ok && nPal <= (uint32_t)SKIN_MAX_OS && nTex >= 2;
    for (size_t i = 0; ok && i < out.os.size(); i++) ok = out.os[i].parent < (int)i;
    for (int o : out.palette) ok = ok && o >= 0 && o < (int)nO;
    for (const ClipAnim& c : out.clips) for (const CanalAnim& k : c.canaux) ok = ok && k.os >= 0 && k.os < (int)nO && k.chemin >= 0 && k.chemin <= 2;
    for (uint32_t i : idx) ok = ok && i < nV;
    if (!ok) { WFS_ERR("%s : fichier tronqué ou incohérent", chemin.c_str()); return false; }

    // textures : couleur (sRGB) et normales (linéaire)
    for (int t = 0; t < 2; t++) {
        int w, h, c;
        stbi_uc* px = blobs[t].first ? stbi_load_from_memory(blobs[t].first, (int)blobs[t].second, &w, &h, &c, 4) : nullptr;
        if (!px) { WFS_ERR("%s : texture %d illisible", chemin.c_str(), t); return false; }
        out.textures[t] = r.vk().uploadTexture(px, (uint32_t)w, (uint32_t)h, t == 0);
        stbi_image_free(px);
    }
    Material m; m.rough = rugo; m.metal = metal;
    out.materiau = r.makeMaterial(m, &out.textures[0], &out.textures[1], nullptr);
    out.maillage = r.makeSkinnedMesh(v, idx);
    out.ok = true;
    WFS_LOG("%s : %u sommets, %u indices, %u os (palette %u), %u clips, %u plages, %.2f m", chemin.c_str(), nV, nI, nO, nPal, nC, nP, out.hauteur);
    return true;
}

void detruirePersonnage(Renderer& r, Personnage& p) {
    r.destroy(p.maillage);
    r.destroy(p.materiau);   // WFS-187 : jeu de descripteurs rendu
    for (Image& t : p.textures) if (t.img) r.vk().destroy(t);
    p = Personnage{};
}

int clipParNom(const Personnage& p, const std::string& nom) {
    for (size_t i = 0; i < p.clips.size(); i++) if (p.clips[i].nom == nom) return (int)i;
    return -1;
}

const Plage* plageParNom(const Personnage& p, const std::string& nom) {
    for (const Plage& pl : p.plages) if (pl.nom == nom) return &pl;
    return nullptr;
}

void preparerPose(const Personnage& p, PoseSquelette& pose) {
    const size_t n = p.os.size();
    pose.t.resize(n); pose.s.resize(n); pose.r.resize(n); pose.modele.resize(n);
    pose.palette.resize(p.palette.size());
}

void echantillonner(const Personnage& p, int clip, double temps, PoseSquelette& pose) {
    const size_t n = p.os.size();
    if (pose.modele.size() != n || pose.palette.size() != p.palette.size()) return;   // preparerPose oublié
    for (size_t i = 0; i < n; i++) { pose.t[i] = p.os[i].t; pose.r[i] = p.os[i].r; pose.s[i] = p.os[i].s; }   // repos
    if (clip >= 0 && clip < (int)p.clips.size()) {
        const ClipAnim& c = p.clips[(size_t)clip];
        float t = c.duree > 0 ? (float)std::fmod(temps, (double)c.duree) : 0;
        if (t < 0) t += c.duree;
        for (const CanalAnim& k : c.canaux) {
            size_t i; float f; cle(k.temps, t, i, f);
            const size_t j = std::min(i + 1, k.temps.size() - 1);
            if (k.chemin == 1) {
                const float* a = &k.valeurs[i * 4]; const float* b = &k.valeurs[j * 4];
                pose.r[(size_t)k.os] = slerp({a[0], a[1], a[2], a[3]}, {b[0], b[1], b[2], b[3]}, f);
            } else {
                const float* a = &k.valeurs[i * 3]; const float* b = &k.valeurs[j * 3];
                const vec3 v = lerp({a[0], a[1], a[2]}, {b[0], b[1], b[2]}, f);
                if (k.chemin == 0) pose.t[(size_t)k.os] = v; else pose.s[(size_t)k.os] = v;
            }
        }
    }
    for (size_t i = 0; i < n; i++) {   // parents avant enfants
        const mat4 local = trs(pose.t[i], pose.r[i], pose.s[i]);
        pose.modele[i] = p.os[i].parent < 0 ? local : pose.modele[(size_t)p.os[i].parent] * local;
    }
    for (size_t k = 0; k < p.palette.size(); k++) {
        const int o = p.palette[k];
        pose.palette[k] = pose.modele[(size_t)o] * p.os[(size_t)o].bindInverse;
    }
}

} // namespace wfs
