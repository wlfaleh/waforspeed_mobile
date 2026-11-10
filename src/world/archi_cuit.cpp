// WaForSpeed — WFS-416 : bâtiments du kit cuits dans le paquet de la carte (voir archi_cuit.h).
#include "archi_cuit.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <array>
#include <map>
#include "archi_ouvertures.h"

namespace wfs::archi::cuit {

int slot(int part, const Plan& p) {
    switch (part) {
    case MUR: return S_MUR + (int)p.style;
    case TOIT: return S_TOIT + (int)p.toit;
    case ENCADREMENT: return S_ENCADREMENT;
    case VITRE: return S_VITRE;
    case MENUISERIE: return S_MENUISERIE;
    case METAL: return S_METAL;
    case FIN: return S_FIN;
    case CHASSIS: return S_CHASSIS;   // WFS-402
    case IMAGE: return S_IMAGE;       // WFS-416
    default: return S_VOLET;
    }
}

uint64_t signature(const std::vector<Bati>& bats) {
    uint64_t h = 1469598103934665603ull;
    auto mel = [&](uint64_t x) { for (int k = 0; k < 8; k++) { h ^= (x >> (8 * k)) & 255u; h *= 1099511628211ull; } };
    mel(VERSION_BLOC); mel((uint64_t)MORCEAU); mel(NB_SLOTS); mel(bats.size());
    for (const Bati& b : bats) {
        const Plan& p = b.plan;
        mel((uint64_t)b.seg); mel((uint64_t)p.style); mel(p.graine); mel((uint64_t)p.etages); mel((uint64_t)p.travees);
        mel((uint64_t)p.rdc); mel((uint64_t)p.etage); mel((uint64_t)p.toit); mel(p.edicules ? 1 : 0); mel(p.baies.size());
    }
    return h;
}

void ajouter(const Bati& b, int lod, Baies mode, Lot* lots, size_t* premierMur) {
    Batiment loc;
    construire(b.plan, b.profondeur, lod, loc, mode);
    const vec3 up{0, 1, 0};
    if (premierMur) *premierMur = lots[slot(MUR, b.plan)].v.size();
    for (int k = 0; k < NB_PARTS; k++) {
        const Maillage& m = loc.part[k];
        if (m.i.empty()) continue;
        Lot& out = lots[slot(k, b.plan)];
        const uint32_t base = (uint32_t)out.v.size();
        for (const archi::Sommet& q : m.v) {
            const vec3 p = b.o + b.x * q.p.x + up * q.p.y + b.z * q.p.z, n = b.x * q.n.x + up * q.n.y + b.z * q.n.z;
            Sommet v{}; v.px = p.x; v.py = p.y; v.pz = p.z; v.nx = n.x; v.ny = n.y; v.nz = n.z; v.u = q.uv.x; v.v = q.uv.y;
            out.v.push_back(v);
        }
        for (uint32_t i : m.i) out.i.push_back(base + i);
    }
}

namespace {

// ---------------------------------------------------------------- image avec relief d'un modèle de menuiserie (RG-04, RG-05)
constexpr int SUR = 3;   // suréchantillonnage (3 × 3 par pixel) : découpe du vitrage et bords adoucis
constexpr float PENTE_MAX = 2.5f;   // pente maximale du relief (≈ 68°) : une marche franche de 2 cm reste lisible sans aberration

uint8_t versSrgb(float c) {
    c = std::clamp(c, 0.f, 1.f);
    const float s = c <= 0.0031308f ? 12.92f * c : 1.055f * std::pow(c, 1 / 2.4f) - 0.055f;
    return (uint8_t)std::lround(s * 255);
}

// dessine la menuiserie 3D de la baie b du plan p, vue de face, dans la tuile (colonne, rangée) de l'atlas
void dessinerTuile(const Plan& p, const Baie& b, Atlas& a, uint32_t col, uint32_t rang) {
    const float d = profondeurTableau(p, b);
    Batiment t;
    ouvertures::menuiserie(p, b, d, 0, t);   // le vrai objet de WFS-402 (dormant, ouvrants, petits bois, panneaux, quincaillerie)
    const int W = (int)TUILE_L * SUR, H = (int)TUILE_H * SUR;
    const float x0 = b.x - b.l / 2, y0 = b.y;
    std::vector<float> z((size_t)W * H, -1e9f);
    std::vector<int8_t> part((size_t)W * H, -1);
    static const int PARTS[] = {CHASSIS, METAL, MENUISERIE, ENCADREMENT, VOLET};   // VITRE : laissée à jour (vitre du fond)
    for (int k : PARTS) {
        const Maillage& m = t.part[k];
        for (size_t i = 0; i + 2 < m.i.size(); i += 3) {
            const archi::Sommet &A = m.v[m.i[i]], &B = m.v[m.i[i + 1]], &C = m.v[m.i[i + 2]];
            if (A.n.z + B.n.z + C.n.z <= 1e-3f) continue;   // face tournée vers le fond ou vue par la tranche
            // coordonnées en échantillons (centre de l'échantillon i à i + 0,5)
            const float ax = (A.p.x - x0) / b.l * W, ay = (A.p.y - y0) / b.h * H, bx = (B.p.x - x0) / b.l * W, by = (B.p.y - y0) / b.h * H,
                        cx = (C.p.x - x0) / b.l * W, cy = (C.p.y - y0) / b.h * H;
            const float aire = (bx - ax) * (cy - ay) - (cx - ax) * (by - ay);
            if (std::fabs(aire) < 1e-6f) continue;
            const int u0 = std::max(0, (int)std::floor(std::min({ax, bx, cx}))), u1 = std::min(W - 1, (int)std::ceil(std::max({ax, bx, cx})));
            const int v0 = std::max(0, (int)std::floor(std::min({ay, by, cy}))), v1 = std::min(H - 1, (int)std::ceil(std::max({ay, by, cy})));
            for (int v = v0; v <= v1; v++) for (int u = u0; u <= u1; u++) {
                const float px = u + 0.5f, py = v + 0.5f;
                const float wa = ((bx - px) * (cy - py) - (cx - px) * (by - py)) / aire, wb = ((cx - px) * (ay - py) - (ax - px) * (cy - py)) / aire, wc = 1 - wa - wb;
                if (wa < 0 || wb < 0 || wc < 0) continue;
                const float zz = wa * A.p.z + wb * B.p.z + wc * C.p.z;
                const size_t q = (size_t)v * W + u;
                if (zz > z[q]) { z[q] = zz; part[q] = (int8_t)k; }
            }
        }
    }
    auto couleur = [](int k) -> vec3 {
        switch (k) {
        case CHASSIS: return {COUL_CHASSIS.r, COUL_CHASSIS.g, COUL_CHASSIS.b};
        case METAL: return {ALB_ALUMINIUM.r * K_METAL, ALB_ALUMINIUM.g * K_METAL, ALB_ALUMINIUM.b * K_METAL};
        case MENUISERIE: return {COUL_MENUISERIE.r, COUL_MENUISERIE.g, COUL_MENUISERIE.b};
        case VOLET: return {COUL_VOLET.r, COUL_VOLET.g, COUL_VOLET.b};
        default: return {ALB_BETON.r, ALB_BETON.g, ALB_BETON.b};
        }
    };
    // réduction : couleur moyenne des échantillons couverts, alpha = part couverte, hauteur moyenne (fond du tableau = 0)
    std::vector<float> haut((size_t)TUILE_L * TUILE_H, 0), alpha((size_t)TUILE_L * TUILE_H, 0);
    std::vector<vec3> coul((size_t)TUILE_L * TUILE_H);
    vec3 moyenne{0, 0, 0}; float poids = 0;
    for (uint32_t y = 0; y < TUILE_H; y++) for (uint32_t x = 0; x < TUILE_L; x++) {
        vec3 c{0, 0, 0}; float n = 0, hz = 0;
        for (int j = 0; j < SUR; j++) for (int i = 0; i < SUR; i++) {
            const size_t q = (size_t)(y * SUR + j) * W + (x * SUR + i);
            if (part[q] < 0) continue;
            c = c + couleur(part[q]); n += 1; hz += z[q] + d;
        }
        const size_t k = (size_t)y * TUILE_L + x;
        alpha[k] = n / (SUR * SUR); haut[k] = hz / (SUR * SUR);
        if (n > 0) { coul[k] = c * (1 / n); moyenne = moyenne + c; poids += n; }
    }
    if (poids > 0) moyenne = moyenne * (1 / poids);
    const float pasX = b.l / TUILE_L, pasY = b.h / TUILE_H;   // mètres par pixel
    auto h = [&](int x, int y) { return haut[(size_t)std::clamp(y, 0, (int)TUILE_H - 1) * TUILE_L + (size_t)std::clamp(x, 0, (int)TUILE_L - 1)]; };
    for (uint32_t y = 0; y < TUILE_H; y++) for (uint32_t x = 0; x < TUILE_L; x++) {
        const size_t k = (size_t)y * TUILE_L + x, q = ((size_t)(rang * TUILE_H + y) * a.l + col * TUILE_L + x) * 4;
        const vec3 c = alpha[k] > 0 ? coul[k] : moyenne;   // sous le vitrage : couleur du cadre (aucune frange sombre au filtrage)
        a.couleur[q] = versSrgb(c.x); a.couleur[q + 1] = versSrgb(c.y); a.couleur[q + 2] = versSrgb(c.z);
        a.couleur[q + 3] = (uint8_t)std::lround(alpha[k] * 255);
        // relief : normale de la surface de hauteur (différences centrées, en mètres), pente bornée
        const float gx = std::clamp((h((int)x + 1, (int)y) - h((int)x - 1, (int)y)) / (2 * pasX), -PENTE_MAX, PENTE_MAX);
        const float gy = std::clamp((h((int)x, (int)y + 1) - h((int)x, (int)y - 1)) / (2 * pasY), -PENTE_MAX, PENTE_MAX);
        const float inv = 1 / std::sqrt(gx * gx + gy * gy + 1);
        a.relief[q] = (uint8_t)std::lround((-gx * inv * 0.5f + 0.5f) * 255); a.relief[q + 1] = (uint8_t)std::lround((-gy * inv * 0.5f + 0.5f) * 255);
        a.relief[q + 2] = (uint8_t)std::lround((inv * 0.5f + 0.5f) * 255);
        a.relief[q + 3] = (uint8_t)std::lround(std::clamp(haut[k] / std::max(d, 0.01f), 0.f, 1.f) * 255);
    }
}

// ---------------------------------------------------------------- lecture / écriture petit-boutiste
struct Ecrivain {
    std::vector<uint8_t> o;
    void u32(uint32_t x) { for (int k = 0; k < 4; k++) o.push_back((uint8_t)(x >> (8 * k))); }
    void u64(uint64_t x) { for (int k = 0; k < 8; k++) o.push_back((uint8_t)(x >> (8 * k))); }
    void f32(float x) { uint32_t u; std::memcpy(&u, &x, 4); u32(u); }
    void v3(vec3 p) { f32(p.x); f32(p.y); f32(p.z); }
    void octets(const void* p, size_t n) { const uint8_t* b = (const uint8_t*)p; o.insert(o.end(), b, b + n); }
    void lot(const Lot& l) {
        u32((uint32_t)l.v.size()); u32((uint32_t)l.i.size());
        octets(l.v.data(), l.v.size() * sizeof(Sommet)); octets(l.i.data(), l.i.size() * 4);
    }
};
struct Lecteur {
    const std::vector<uint8_t>& o; size_t pos = 0; bool ok = true;
    bool reste(size_t n) { if (!ok || n > o.size() - pos) ok = false; return ok; }
    uint32_t u32() { if (!reste(4)) return 0; uint32_t x = 0; for (int k = 0; k < 4; k++) x |= (uint32_t)o[pos + k] << (8 * k); pos += 4; return x; }
    uint64_t u64() { if (!reste(8)) return 0; uint64_t x = 0; for (int k = 0; k < 8; k++) x |= (uint64_t)o[pos + k] << (8 * k); pos += 8; return x; }
    float f32() { const uint32_t u = u32(); float x; std::memcpy(&x, &u, 4); return x; }
    vec3 v3() { vec3 p; p.x = f32(); p.y = f32(); p.z = f32(); return p; }
    void sauter(size_t n) { if (reste(n)) pos += n; }
    // un lot : vue dans le bloc ; chaque indice doit désigner un sommet du lot
    bool lot(Vue& l) {
        l.nv = u32(); l.ni = u32(); l.pos = pos;
        if (!ok || l.ni % 3 != 0 || !reste((size_t)l.nv * sizeof(Sommet) + (size_t)l.ni * 4)) return ok = false;
        pos += (size_t)l.nv * sizeof(Sommet);
        for (uint32_t k = 0; k < l.ni; k++, pos += 4) { uint32_t i; std::memcpy(&i, o.data() + pos, 4); if (i >= l.nv) return ok = false; }
        return true;
    }
};

vec3 versMonde(const Repere& r, float x, float y, float z) { return r.o + r.x * x + vec3{0, 1, 0} * y + r.z * z; }
vec3 tourner(const Repere& r, float x, float y, float z) { return r.x * x + vec3{0, 1, 0} * y + r.z * z; }

// deux modèles de baie identiques : mêmes maillages, sommets à 0,2 mm près (bruit des flottants d'une travée à l'autre)
bool pareils(const Modele& a, const Modele& b) {
    for (int s = 0; s < NB_SLOTS; s++) {
        const Lot &x = a.lot[s], &y = b.lot[s];
        if (x.v.size() != y.v.size() || x.i != y.i) return false;
        for (size_t k = 0; k < x.v.size(); k++) {
            const Sommet &p = x.v[k], &q = y.v[k];
            if (std::fabs(p.px - q.px) > 2e-4f || std::fabs(p.py - q.py) > 2e-4f || std::fabs(p.pz - q.pz) > 2e-4f) return false;
            if (std::fabs(p.nx - q.nx) > 1e-3f || std::fabs(p.ny - q.ny) > 1e-3f || std::fabs(p.nz - q.nz) > 1e-3f) return false;
        }
    }
    return true;
}

}  // namespace

Decor cuire(const std::vector<Bati>& bats) {
    Decor d;
    d.signature = signature(bats);
    std::map<int64_t, size_t> rang;   // morceau → place dans d.morceaux (ordre croissant)
    for (const Bati& b : bats) rang[b.seg / MORCEAU] = 0;
    d.morceaux.resize(rang.size());
    { size_t k = 0; for (auto& [c, r] : rang) { r = k; d.morceaux[k++].c = c; } }
    std::map<int, std::pair<const Bati*, const Baie*>> gabarits;   // tuile → première baie du modèle (elle sert de gabarit à l'image)
    std::map<std::array<long, 4>, std::vector<uint32_t>> seaux;    // (bâtiment, type, largeur mm, hauteur mm) → modèles candidats
    for (size_t kb = 0; kb < bats.size(); kb++) {
        const Bati& b = bats[kb];
        const Repere rep{b.o, b.x, b.z};
        d.batis.push_back(rep);
        Morceau& m = d.morceaux[rang[b.seg / MORCEAU]];
        ajouter(b, 0, Baies::Aucune, m.lot[0]);         // niveau 0 : gros œuvre (murs percés, toiture, corniche, traits des planchers)
        ajouter(b, 1, Baies::Image, m.lot[1]);
        ajouter(b, 2, Baies::Interrupteur, m.lot[2]);   // volume seul : l'interrupteur n'y change rien
        for (const Baie& x : b.plan.baies) {
            gabarits.emplace(tuileImage(b.plan, x), std::make_pair(&b, &x));
            Batiment loc;
            baie(b.plan, x, 0, loc, Baies::Reelles);    // niveau 0 : la baie complète de WFS-402 (code existant)
            Modele cand;
            for (int k = 0; k < NB_PARTS; k++) {
                const Maillage& src = loc.part[k];
                if (src.i.empty()) continue;
                const bool vitre = k == VITRE;   // la vitre porte la graine de SA pièce de nuit dans ses uv : jamais partagée
                Lot& l = vitre ? m.lot[0][S_VITRE] : cand.lot[slot(k, b.plan)];
                const uint32_t base = (uint32_t)l.v.size();
                for (const archi::Sommet& q : src.v) {
                    Sommet v{}; v.u = q.uv.x; v.v = q.uv.y;
                    if (vitre) {
                        const vec3 p = versMonde(rep, q.p.x, q.p.y, q.p.z), n = tourner(rep, q.n.x, q.n.y, q.n.z);
                        v.px = p.x; v.py = p.y; v.pz = p.z; v.nx = n.x; v.ny = n.y; v.nz = n.z;
                    } else { v.px = q.p.x - x.x; v.py = q.p.y - x.y; v.pz = q.p.z; v.nx = q.n.x; v.ny = q.n.y; v.nz = q.n.z; }
                    l.v.push_back(v);
                }
                for (uint32_t i : src.i) l.i.push_back(base + i);
            }
            // RG-05 : modèle déjà cuit pour ce bâtiment (même type, mêmes mesures, même état des volets…) → une instance de plus
            std::vector<uint32_t>& seau = seaux[{(long)kb, (long)x.type, std::lround(x.l * 1000), std::lround(x.h * 1000)}];
            uint32_t id = UINT32_MAX;
            for (uint32_t c : seau) if (pareils(d.modeles[c], cand)) { id = c; break; }
            if (id == UINT32_MAX) { id = (uint32_t)d.modeles.size(); d.modeles.push_back(std::move(cand)); seau.push_back(id); }
            m.instances.push_back({id, (uint32_t)kb, x.x, x.y});
        }
    }
    for (Morceau& m : d.morceaux) {   // sphère englobante (niveau 1 : toute la géométrie hors balcons ; marge de 3 m)
        vec3 lo{1e30f, 1e30f, 1e30f}, hi{-1e30f, -1e30f, -1e30f};
        for (const Lot& l : m.lot[1]) for (const Sommet& v : l.v) {
            lo = {std::min(lo.x, v.px), std::min(lo.y, v.py), std::min(lo.z, v.pz)}; hi = {std::max(hi.x, v.px), std::max(hi.y, v.py), std::max(hi.z, v.pz)};
        }
        m.centre = (lo + hi) * 0.5f; m.rayon = length(hi - lo) * 0.5f + 3.f;
    }
    Atlas& a = d.atlas;
    a.l = TUILE_L * COLONNES;
    a.h = TUILE_H * (uint32_t)std::max<size_t>(1, (gabarits.size() + COLONNES - 1) / COLONNES);
    a.couleur.assign((size_t)a.l * a.h * 4, 0);
    a.relief.assign((size_t)a.l * a.h * 4, 0);
    for (size_t q = 0; q < a.relief.size(); q += 4) { a.relief[q] = a.relief[q + 1] = 128; a.relief[q + 2] = 255; }
    std::map<int, uint32_t> place;
    for (const auto& [t, bb] : gabarits) {
        const uint32_t k = (uint32_t)a.tuiles.size();
        place[t] = k; a.tuiles.push_back(t);
        dessinerTuile(bb.first->plan, *bb.second, a, k % COLONNES, k / COLONNES);
    }
    // uv des rectangles d'image : (tuile + 0..0,999 ; 0..1) → place de la tuile dans l'atlas, un demi-pixel à l'intérieur
    for (Morceau& m : d.morceaux) for (Sommet& s : m.lot[1][S_IMAGE].v) {
        const int t = (int)std::floor(s.u);
        const float fu = (s.u - (float)t) / 0.999f, fv = s.v;
        const uint32_t k = place.count(t) ? place[t] : 0;
        s.u = ((float)(k % COLONNES * TUILE_L) + 0.5f + fu * (float)(TUILE_L - 1)) / (float)a.l;
        s.v = ((float)(k / COLONNES * TUILE_H) + 0.5f + fv * (float)(TUILE_H - 1)) / (float)a.h;
    }
    return d;
}

// Bloc (petit-boutiste) : « WFSB », u32 version, u64 signature, u32 MORCEAU, u32 NB_SLOTS ;
//   atlas : u32 largeur, u32 hauteur, u32 nombre de tuiles, les numéros (u32), couleur (RGBA8), relief (RGBA8) ;
//   u32 bâtiments, par bâtiment 9 f32 (o, x, z) ; u32 modèles, par modèle NB_SLOTS lots ;
//   u32 morceaux, par morceau : u64 c, 4 f32 (centre, rayon), NIVEAUX × NB_SLOTS lots, u32 instances, par instance u32 modèle,
//   u32 bâtiment, f32 dx, f32 dy. Un lot : u32 sommets, u32 indices, sommets (36 octets), indices (u32).
std::vector<uint8_t> ecrire(const Decor& d) {
    Ecrivain e;
    e.octets("WFSB", 4); e.u32(VERSION_BLOC); e.u64(d.signature); e.u32((uint32_t)MORCEAU); e.u32(NB_SLOTS);
    e.u32(d.atlas.l); e.u32(d.atlas.h); e.u32((uint32_t)d.atlas.tuiles.size());
    for (int t : d.atlas.tuiles) e.u32((uint32_t)t);
    e.octets(d.atlas.couleur.data(), d.atlas.couleur.size()); e.octets(d.atlas.relief.data(), d.atlas.relief.size());
    e.u32((uint32_t)d.batis.size());
    for (const Repere& r : d.batis) { e.v3(r.o); e.v3(r.x); e.v3(r.z); }
    e.u32((uint32_t)d.modeles.size());
    for (const Modele& m : d.modeles) for (const Lot& l : m.lot) e.lot(l);
    e.u32((uint32_t)d.morceaux.size());
    for (const Morceau& m : d.morceaux) {
        e.u64((uint64_t)m.c); e.v3(m.centre); e.f32(m.rayon);
        for (int n = 0; n < NIVEAUX; n++) for (const Lot& l : m.lot[n]) e.lot(l);
        e.u32((uint32_t)m.instances.size());
        for (const Instance& in : m.instances) { e.u32(in.modele); e.u32(in.bati); e.f32(in.dx); e.f32(in.dy); }
    }
    return std::move(e.o);
}

bool lire(std::shared_ptr<const std::vector<uint8_t>> octets, uint64_t signatureAttendue, Lu& d, const char** raison) {
    auto refus = [&](const char* r) { if (raison) *raison = r; d = Lu{}; return false; };
    if (!octets) return refus("bloc des bâtiments absent");
    d = Lu{};
    const std::vector<uint8_t>& o = *octets;
    Lecteur L{o};
    if (!L.reste(4) || std::memcmp(o.data(), "WFSB", 4) != 0) return refus("bloc des bâtiments illisible");
    L.pos = 4;
    if (L.u32() != VERSION_BLOC) return refus("bloc des bâtiments d'une autre version");
    d.signature = L.u64();
    if (!L.ok || d.signature != signatureAttendue) return refus("bâtiments cuits pour un autre plan de la carte");
    if (L.u32() != (uint32_t)MORCEAU || L.u32() != (uint32_t)NB_SLOTS) return refus("bloc des bâtiments d'un autre découpage");
    d.atlasL = L.u32(); d.atlasH = L.u32();
    const uint32_t nt = L.u32();
    if (!L.ok || d.atlasL != TUILE_L * COLONNES || d.atlasH == 0 || d.atlasH % TUILE_H != 0 || d.atlasH > TUILE_H * 64 || nt > (uint32_t)NB_TUILES)
        return refus("atlas des menuiseries faux");
    d.tuiles.resize(nt);
    for (int& t : d.tuiles) t = (int)L.u32();
    const size_t pixels = (size_t)d.atlasL * d.atlasH * 4;
    d.posCouleur = L.pos; L.sauter(pixels); d.posRelief = L.pos; L.sauter(pixels);
    if (!L.ok) return refus("atlas des menuiseries tronqué");
    const uint32_t nb = L.u32();
    if (!L.reste((size_t)nb * 36)) return refus("bloc des bâtiments tronqué");
    d.batis.resize(nb);
    for (Repere& r : d.batis) { r.o = L.v3(); r.x = L.v3(); r.z = L.v3(); }
    const uint32_t nmod = L.u32();
    if (!L.reste((size_t)nmod * NB_SLOTS * 8)) return refus("bloc des bâtiments tronqué");
    d.modeles.resize(nmod);
    for (auto& m : d.modeles) for (Vue& l : m) if (!L.lot(l)) return refus("modèle de baie faux (taille ou indice)");
    const uint32_t nm = L.u32();
    if (!L.reste((size_t)nm * (8 + 16 + NIVEAUX * NB_SLOTS * 8 + 4))) return refus("bloc des bâtiments tronqué");
    d.morceaux.resize(nm);
    for (MorceauLu& m : d.morceaux) {
        m.c = (int64_t)L.u64(); m.centre = L.v3(); m.rayon = L.f32();
        for (int n = 0; n < NIVEAUX; n++) for (Vue& l : m.lot[n]) if (!L.lot(l)) return refus("morceau faux (taille ou indice)");
        const uint32_t ni = L.u32();
        if (!L.reste((size_t)ni * 16)) return refus("bloc des bâtiments tronqué");
        m.premiereInstance = d.instances.size(); m.nbInstances = ni;
        for (int s = 0; s < NB_SLOTS; s++) { m.nv0[s] = m.lot[0][s].nv; m.ni0[s] = m.lot[0][s].ni; }
        for (uint32_t k = 0; k < ni; k++) {
            Instance in; in.modele = L.u32(); in.bati = L.u32(); in.dx = L.f32(); in.dy = L.f32();
            if (in.modele >= nmod || in.bati >= nb) return refus("instance de baie fausse");
            for (int s = 0; s < NB_SLOTS; s++) {
                const Vue& l = d.modeles[in.modele][(size_t)s];
                if ((uint64_t)m.nv0[s] + l.nv > 0x7fffffffu || (uint64_t)m.ni0[s] + l.ni > 0x7fffffffu) return refus("morceau trop grand");
                m.nv0[s] += l.nv; m.ni0[s] += l.ni;
            }
            d.instances.push_back(in);
        }
        for (int s = 0; s < NB_SLOTS; s++) { d.nvMax[s] = std::max(d.nvMax[s], m.nv0[s]); d.niMax[s] = std::max(d.niMax[s], m.ni0[s]); }
    }
    for (size_t k = 1; k < d.morceaux.size(); k++) if (d.morceaux[k].c <= d.morceaux[k - 1].c) return refus("morceaux en désordre");
    if (!L.ok || L.pos != o.size()) return refus("bloc des bâtiments de taille fausse");
    d.octets = std::move(octets);
    return true;
}

void Lu::copier(const Vue& l, void* sommets, uint32_t* i, uint32_t base) const {
    const uint8_t* src = octets->data() + l.pos;
    if (l.nv && sommets) std::memcpy(sommets, src, (size_t)l.nv * sizeof(Sommet));
    src += (size_t)l.nv * sizeof(Sommet);
    if (i) for (uint32_t k = 0; k < l.ni; k++) { uint32_t x; std::memcpy(&x, src + (size_t)k * 4, 4); i[k] = base + x; }
}

Vue Lu::developper(const Instance& in, int s, void* sommets, uint32_t* i, uint32_t base) const {
    const Vue& l = modeles[in.modele][(size_t)s];
    if (!l.ni) return l;
    const Repere& r = batis[in.bati];
    const uint8_t* src = octets->data() + l.pos;
    for (uint32_t k = 0; k < l.nv; k++) {
        Sommet q; std::memcpy(&q, src + (size_t)k * sizeof(Sommet), sizeof(Sommet));
        const vec3 p = versMonde(r, q.px + in.dx, q.py + in.dy, q.pz), n = tourner(r, q.nx, q.ny, q.nz);
        q.px = p.x; q.py = p.y; q.pz = p.z; q.nx = n.x; q.ny = n.y; q.nz = n.z;
        std::memcpy((uint8_t*)sommets + (size_t)k * sizeof(Sommet), &q, sizeof(Sommet));
    }
    src += (size_t)l.nv * sizeof(Sommet);
    for (uint32_t k = 0; k < l.ni; k++) { uint32_t x; std::memcpy(&x, src + (size_t)k * 4, 4); i[k] = base + x; }
    return l;
}

size_t Lu::triangles(int niveau) const {
    size_t n = 0;
    for (const MorceauLu& m : morceaux) for (int s = 0; s < NB_SLOTS; s++) n += (niveau == 0 ? m.ni0[s] : m.lot[niveau][s].ni) / 3;
    return n;
}

}  // namespace wfs::archi::cuit
