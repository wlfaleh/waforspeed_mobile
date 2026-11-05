// Feux arrière des modèles (WFS-109) : découpe d'un matériau de feux avant + arrière (RG-13), modèle qui a déjà un feu
// MAT_TAIL (inchangé), pièce trop haute (pas de découpe), 3e feu stop marqué (RG-03), transition de 80 ms (RG-02).
// usage : test_feux_modele
#include <cmath>
#include <cstdio>
#include "../src/render/feux_modele.h"
using namespace wfs;
using namespace wfs::feux_modele;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

struct Piece { uint32_t firstIndex, indexCount; int32_t vertexOffset; uint32_t material; int32_t roue = -1; int8_t feuArriere = 0; };
struct Scene {
    std::vector<vec3> v; std::vector<uint32_t> idx; std::vector<Piece> parts; std::vector<Materiau> mats;
    // un triangle de hauteur h centré en (x, y, z), dans une pièce du matériau m (une pièce par appel)
    void tri(uint32_t m, float x, float y, float z, float h = 0.05f) {
        const uint32_t v0 = (uint32_t)v.size();
        v.push_back({x - 0.05f, y, z}); v.push_back({x + 0.05f, y, z}); v.push_back({x, y + h, z});
        parts.push_back({(uint32_t)idx.size(), 3, (int32_t)v0, m});
        idx.insert(idx.end(), {0, 1, 2});
        if (mats.size() <= m) mats.resize(m + 1);
        Materiau& a = mats[m];
        for (uint32_t k = v0; k < v0 + 3; k++) {
            a.bmin = {std::min(a.bmin.x, v[k].x), std::min(a.bmin.y, v[k].y), std::min(a.bmin.z, v[k].z)};
            a.bmax = {std::max(a.bmax.x, v[k].x), std::max(a.bmax.y, v[k].y), std::max(a.bmax.z, v[k].z)};
        }
    }
    Resultat decoupe(float L) { return decouper(idx, parts, [&](size_t k) { return v[k]; }, mats, L); }
};

int main() {
    const float L = 4.3f;   // Golf
    {   // Golf : un seul matériau émissif blanc (1) pour les phares (avant, −z) et les feux arrière (+z) ; peinture (0)
        Scene s; s.tri(0, 0, 0.5f, 0); s.tri(1, -0.7f, 0.7f, -2.0f); s.tri(1, 0.7f, 0.7f, -2.0f); s.tri(1, -0.7f, 0.9f, 2.0f); s.tri(1, 0.7f, 0.9f, 2.0f);
        s.mats[1].flags = 2 | 16; s.mats[1].emissif = {1, 1, 1};
        const size_t avant = s.parts.size();
        const Resultat r = s.decoupe(L);
        VERIFIE(r.materiau == 1 && r.triangles == 2, "Golf : matériau 1 découpé, 2 triangles (trouvé %d, %u)", r.materiau, r.triangles);
        int arr = 0, av = 0;
        for (const Piece& p : s.parts) {
            if (p.material != 1) continue;
            const float z = s.v[(size_t)p.vertexOffset + s.idx[p.firstIndex]].z;
            if (p.feuArriere == PIECE_FEU_ARRIERE) { arr++; VERIFIE(z > 0.4f * L, "pièce feu arrière à l'arrière"); }
            else { av++; VERIFIE(z < 0, "phares restés à l'avant"); }
        }
        VERIFIE(arr == 2 && av == 2 && s.parts.size() == avant, "Golf : 2 pièces arrière, 2 avant (%d, %d), autant de pièces", arr, av);
        VERIFIE(std::abs(r.bmin.y - 0.9f) < 1e-4f && r.bmax.x > 0.7f, "Golf : boîte du feu découpé (position mesurée)");
        std::printf("Golf : feu arrière découpé dans le matériau des phares, phares inchangés : OK\n");
    }
    {   // a45 : un feu MAT_TAIL valide (3) + un matériau émissif à l'arrière : rien n'est découpé
        Scene s; s.tri(3, 0.7f, 0.9f, 2.0f); s.tri(6, 0.7f, 0.9f, 2.0f); s.mats[3].flags = DRAPEAU_TAIL; s.mats[6].emissif = {0.3f, 0, 0};
        const auto idx0 = s.idx; const size_t n0 = s.parts.size();
        const Resultat r = s.decoupe(L);
        VERIFIE(r.materiau < 0 && s.parts.size() == n0 && s.idx == idx0, "a45 : modèle inchangé");
        std::printf("modèle avec un feu MAT_TAIL : inchangé : OK\n");
    }
    {   // X5 : pièce arrière de 0,9 m de haut (hayon entier) : pas de découpe
        Scene s; s.tri(1, 0.7f, 0.7f, 2.0f, 0.9f); s.mats[1].emissif = {0.5f, 0.5f, 0.5f};
        VERIFIE(s.decoupe(L).materiau < 0, "X5 : pièce trop haute, pas de découpe");
        Scene n; n.tri(1, 0.7f, 0.7f, 2.0f); n.tri(2, 0.7f, 0.7f, -2.0f);
        VERIFIE(n.decoupe(L).materiau < 0, "matériaux non émissifs sans drapeau de feu : pas de découpe");
        Scene e; e.tri(1, 0.7f, 0.5f, 1.4f); e.mats[1].flags = DRAPEAU_HEAD;   // étrier « brake » vers l'essieu (0,33 L)
        VERIFIE(e.decoupe(L).materiau < 0, "rien au-delà de 0,4 L : pas de découpe");
        std::printf("pièce trop haute, sans feu, ou avant 0,4 L : pas de découpe : OK\n");
    }
    {   // giulia : feux MAT_TAIL (42) + 3e feu stop étroit et centré, en haut (40)
        Scene s; s.tri(42, -0.7f, 0.8f, 2.1f); s.tri(42, 0.7f, 0.8f, 2.1f); s.tri(40, 0, 1.09f, 1.78f, 0.01f);
        s.mats[42].flags = s.mats[40].flags = DRAPEAU_TAIL;
        const Resultat r = s.decoupe(4.64f);
        VERIFIE(r.materiau < 0 && r.troisieme == 40, "giulia : pas de découpe, 3e feu stop = 40 (%d)", r.troisieme);
        int t3 = 0; for (const Piece& p : s.parts) t3 += p.feuArriere == PIECE_TROISIEME_FEU ? (p.material == 40 ? 1 : 100) : 0;
        VERIFIE(t3 == 1, "giulia : seule la pièce 40 est le 3e feu stop");
        std::printf("3e feu stop repéré : OK\n");
    }
    {   // RG-02 : 80 ms de 0 à 1, puis de 1 à 0 ; première image ou longue pause : niveau atteint tout de suite
        float n = 0; int images = 0;
        while (n < 1 && images < 100) { n = transition(n, true, 1.f / 120); images++; }
        VERIFIE(images >= 9 && images <= 10, "montée en ≈ 80 ms (%d images à 120 i/s)", images);
        VERIFIE(transition(1, true, 1.f / 60) == 1.f, "stop tenu : reste à 1");
        VERIFIE(std::abs(transition(1, false, 0.04f) - 0.5f) < 1e-4f, "lâché : à mi-chemin après 40 ms");
        VERIFIE(transition(0, true, -1) == 1.f && transition(1, false, 2.f) == 0.f, "première image / pause : niveau tout de suite");
        std::printf("transition du feu stop en 80 ms : OK\n");
    }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("feux des modèles : tout est OK\n");
    return 0;
}
