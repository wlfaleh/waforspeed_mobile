// WaForSpeed — WFS-232 (RG-02) : pilotes casqués des deux-roues du trafic : voir deux_roues.h.
#include "deux_roues.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include "../core/log.h"
#include "../render/renderer.h"
#include "../render/squelette.h"
#include "../world/conducteurs.h"   // HUMAINS, albedoPeau, humaineFemme (lecture seule)
#include "../world/pietons.h"       // tenue::tenueLieu, tenue::carteCourante (lecture seule)

namespace wfs::deux_roues {
namespace {

constexpr float PI_F = 3.14159265f;
constexpr float CUISSE_SIEGE = 0.08f / 1.75f;   // articulation de la cuisse au-dessus de la selle (× stature), comme conducteurs.cpp
constexpr int HUMAIN_H = 1, HUMAIN_F = 7;       // h02 et f02 (assets/people/humains), 2 statures d'ANSUR-II
// coque du casque (peintures courantes) : noir, blanc, argent, rouge, bleu, gris mat ; visière fumée
constexpr unsigned COQUES[] = {0x111214, 0xe8e8e6, 0x9a9ca0, 0xa8121c, 0x1c3f8f, 0x3a3c40};
constexpr int NB_COQUES = sizeof(COQUES) / sizeof(COQUES[0]);

struct Humain {
    Personnage p;
    PoseSquelette pose[3];     // une pose par modèle de deux-roues (mains au guidon, pieds posés)
    mat4 place[3];             // repère du personnage → repère du .wfm (assis sur la selle, tourné vers −Z)
    mat4 casque[3];            // repère du casque (repère du personnage) : suit l'os de la tête comme le maillage
    const Plage* corps[3] = {nullptr, nullptr, nullptr};
    bool femme = false; int indice = 0;
};
Humain g_h[2];
Mesh g_coque{}, g_visiere{};
Material g_matCoque[NB_COQUES]{}, g_matVisiere{};
bool g_pret = false;
struct Entree { mat4 monde; int modele; uint32_t graine; float d2; };
Entree g_file[16]; int g_n = 0, g_dessines = 0;

vec3 position(const mat4& m) { return {m.at(0, 3), m.at(1, 3), m.at(2, 3)}; }
vec3 lin(unsigned h) { auto c = [&](int s) { return std::pow(((h >> s) & 255) / 255.f, 2.2f); }; return {c(16), c(8), c(0)}; }
// multiplicateur d'apparence de la peau (même conversion que conducteurs.cpp, WFS-211 RG-03 : texture unie 222, 178, 150)
vec3 peauApparence(const vec3& albedo) {
    const vec3 tex{222 / 255.f, 178 / 255.f, 150 / 255.f};
    return {std::pow(albedo.x, 1 / 2.2f) / tex.x, std::pow(albedo.y, 1 / 2.2f) / tex.y, std::pow(albedo.z, 1 / 2.2f) / tex.z};
}

// ellipsoïde (demi-axes rx, ry, rz) : azimut a0..a1 (0 = devant, +Z), élévation e0..e1 (rad) ; normales de l'ellipsoïde
void ellipsoide(std::vector<Vertex>& v, std::vector<uint32_t>& ix, float rx, float ry, float rz, float a0, float a1, float e0, float e1, int na, int ne) {
    const uint32_t base = (uint32_t)v.size();
    for (int j = 0; j <= ne; j++) for (int i = 0; i <= na; i++) {
        const float e = e0 + (e1 - e0) * j / ne, a = a0 + (a1 - a0) * i / na;
        const vec3 u{std::cos(e) * std::sin(a), std::sin(e), std::cos(e) * std::cos(a)};
        const vec3 n = normalize(vec3{u.x / rx, u.y / ry, u.z / rz});
        v.push_back({u.x * rx, u.y * ry, u.z * rz, n.x, n.y, n.z, (float)i / na, (float)j / ne});
    }
    for (int j = 0; j < ne; j++) for (int i = 0; i < na; i++) {
        const uint32_t k = base + (uint32_t)(j * (na + 1) + i);
        // sens direct vu de l'extérieur (comme le cube du trafic) : faces avant vers l'extérieur, sinon la coque serait éliminée
        // côté caméra et l'on ne verrait que son intérieur, derrière la tête
        ix.insert(ix.end(), {k, k + 1, k + (uint32_t)na + 1, k + 1, k + (uint32_t)na + 2, k + (uint32_t)na + 1});
    }
}

// pose du pilote sur le modèle k : pose assise figée, place sur la selle, mains et pieds par IK (repère du personnage)
bool poser(Humain& H, int k) {
    const Gabarit& G = GABARITS[k];
    PoseSquelette& P = H.pose[k];
    preparerPose(H.p, P);
    const int bassin = osParNom(H.p, "pelvis"), cuisse = osParNom(H.p, "thigh_l"), tete = osParNom(H.p, "Head");
    if (bassin < 0 || cuisse < 0 || tete < 0) return false;
    const vec3 teteRepos = position(inverse(H.p.os[(size_t)tete].bindInverse));   // articulation de la tête dans le repère du maillage
    echantillonner(H.p, clipParNom(H.p, "Sitting_Idle_Loop"), 0, P);
    // assise du personnage : articulation de la cuisse − CUISSE_SIEGE × stature, à l'aplomb du bassin
    const float stature = H.p.hauteur > 0 ? H.p.hauteur / 0.9f : 1.7f;   // articulation de la tête ≈ 0,9 × stature (seulement pour CUISSE_SIEGE)
    const vec3 b = position(P.modele[(size_t)bassin]), c = position(P.modele[(size_t)cuisse]);
    const vec3 assise{b.x, c.y - CUISSE_SIEGE * stature, b.z};
    // repère : rotation d'un demi-tour (le personnage regarde +Z, le deux-roues −Z), assise du personnage sur la selle
    H.place[k] = translate({0, G.selleY, G.selleZ}) * rotateY(PI_F) * translate({-assise.x, -assise.y, -assise.z});
    // point du .wfm → repère du personnage (inverse du repère ci-dessus : demi-tour = (x, y, z) → (−x, y, −z))
    auto versPerso = [&](float x, float y, float z) { return vec3{assise.x - x, assise.y + (y - G.selleY), assise.z - (z - G.selleZ)}; };
    for (int cote : {-1, 1}) {   // cote −1 : gauche du deux-roues (x < 0) = gauche du personnage (_l)
        const char* s = cote < 0 ? "_l" : "_r";
        const int epaule = osParNom(H.p, std::string("upperarm") + s), coude = osParNom(H.p, std::string("lowerarm") + s), main = osParNom(H.p, std::string("hand") + s);
        const int hanche = osParNom(H.p, std::string("thigh") + s), genou = osParNom(H.p, std::string("calf") + s), pied = osParNom(H.p, std::string("foot") + s);
        if (epaule >= 0 && coude >= 0 && main >= 0) atteindre(H.p, P, epaule, coude, main, versPerso(cote * G.poigneeX, G.poigneeY, G.poigneeZ));
        if (hanche >= 0 && genou >= 0 && pied >= 0) atteindre(H.p, P, hanche, genou, pied, versPerso(cote * G.piedX, G.piedY, G.piedZ));
    }
    // casque posé dans le repère du maillage (au-dessus et un peu en avant de l'articulation de la tête), puis porté par la matrice de peau de la
    // tête (modèle × liaison inverse) : il tourne et se déplace avec la tête dessinée, quelle que soit la pose
    H.casque[k] = P.modele[(size_t)tete] * H.p.os[(size_t)tete].bindInverse * translate({teteRepos.x, teteRepos.y + CASQUE_HAUT, teteRepos.z + CASQUE_AVANT});
    return true;
}

} // namespace

bool preparer(Renderer& r) {
    if (g_pret) return true;
    for (int i = 0; i < 2; i++) {
        Humain& H = g_h[i];
        H.indice = i ? HUMAIN_F : HUMAIN_H; H.femme = conducteurs::humaineFemme(H.indice);
        const std::string chemin = std::string("people/humains/") + conducteurs::HUMAINS[H.indice] + ".wfp";
        if (!chargerPersonnage(r, chemin, H.p)) { WFS_ERR("deux-roues : %s illisible : pas de pilote (WFS-232)", chemin.c_str()); return false; }
        for (int lod = 0; lod < 3; lod++) H.corps[lod] = plageParNom(H.p, "corps-" + std::to_string(lod));
        for (int k = 0; k < 3; k++)
            if (!poser(H, k) || !H.corps[0]) { WFS_ERR("deux-roues : %s : os ou corps absents : pas de pilote (WFS-232)", chemin.c_str()); return false; }
    }
    // casque : coque (ellipsoïde ouverte en bas, au niveau du cou) et visière fumée devant les yeux, un peu en avant de la coque
    std::vector<Vertex> v; std::vector<uint32_t> ix;
    ellipsoide(v, ix, CASQUE_RX, CASQUE_RY, CASQUE_RZ, -PI_F, PI_F, -0.62f, PI_F / 2, 24, 12);
    g_coque = r.makeMesh(v, ix);
    v.clear(); ix.clear();
    ellipsoide(v, ix, CASQUE_RX * 1.012f, CASQUE_RY * 1.012f, CASQUE_RZ * 1.012f, -0.95f, 0.95f, -0.12f, 0.30f, 12, 4);
    g_visiere = r.makeMesh(v, ix);
    for (int i = 0; i < NB_COQUES; i++) {   // peinture de casque : vernis brillant (rugosité 0,25), sauf le gris mat
        Material m; const vec3 c = lin(COQUES[i]); m.color = {c.x, c.y, c.z, 1}; m.rough = i == NB_COQUES - 1 ? 0.6f : 0.25f; m.metal = i == 2 ? 0.6f : 0.f;
        g_matCoque[i] = r.makeMaterial(m, nullptr, nullptr, nullptr);
    }
    { Material m; const vec3 c = lin(0x0b0c0e); m.color = {c.x, c.y, c.z, 1}; m.rough = 0.05f; m.metal = 0.f; g_matVisiere = r.makeMaterial(m, nullptr, nullptr, nullptr); }
    g_pret = true;
    WFS_LOG("deux-roues : pilotes %s et %s (pose assise, mains au guidon, pieds posés), casque %.0f × %.0f × %.0f cm (WFS-232)",
            conducteurs::HUMAINS[HUMAIN_H], conducteurs::HUMAINS[HUMAIN_F], CASQUE_RX * 200, CASQUE_RY * 200, CASQUE_RZ * 200);
    return true;
}

void detruire(Renderer& r) {
    if (!g_pret) return;
    for (Humain& H : g_h) detruirePersonnage(r, H.p);
    r.destroy(g_coque); r.destroy(g_visiere);
    for (Material& m : g_matCoque) r.destroy(m);
    r.destroy(g_matVisiere);
    g_pret = false; g_n = 0;
}

void noter(const mat4& monde, int modele, uint32_t graine) {
    if (g_pret && g_n < (int)(sizeof(g_file) / sizeof(g_file[0]))) g_file[g_n++] = {monde, std::clamp(modele, 0, 2), graine, 0.f};
}

void dessinerFile(Renderer& r, const vec3& cam) {
    g_dessines = 0;
    if (!g_pret) { g_n = 0; return; }
    int n = 0;
    for (int i = 0; i < g_n; i++) {
        const vec3 d = position(g_file[i].monde) - cam;
        g_file[i].d2 = d.x * d.x + d.y * d.y + d.z * d.z;
        if (g_file[i].d2 <= DISTANCE_MAX_M * DISTANCE_MAX_M) g_file[n++] = g_file[i];
    }
    std::sort(g_file, g_file + n, [](const Entree& a, const Entree& b) { return a.d2 < b.d2; });
    for (int i = 0; i < n && i < PILOTES_MAX; i++) {
        const Entree& e = g_file[i];
        Mulberry32 R(e.graine * 2654435761u + 0x2d2u);
        const Humain& H = g_h[R.next() < 0.7 ? 0 : 1];   // 7 pilotes sur 10 sont des hommes (choix de conception, à valider par le PO)
        ApparencePersonnage app;
        app.peau = peauApparence(conducteurs::albedoPeau(H.indice)); app.corps = H.p.corps;
        tenue::tenueLieu(tenue::carteCourante(), e.graine, H.femme, app);
        app.vetement.x = 1.95f; app.vetement.y = 1.9f;   // manches longues et pantalon (tenue de deux-roues)
        const mat4 monde = e.monde * H.place[e.modele];
        const int lod = e.d2 < 8.f * 8.f ? 0 : e.d2 < 20.f * 20.f ? 1 : 2;
        const Plage* c = H.corps[lod] ? H.corps[lod] : H.corps[0];
        const PoseSquelette& P = H.pose[e.modele];
        r.drawSkinned(H.p.maillage, H.p.materiau, monde, P.palette.data(), (int)P.palette.size(), c->premier, c->nombre, &app);
        const mat4 casque = monde * H.casque[e.modele];
        r.draw(g_coque, g_matCoque[(int)(R.next() * NB_COQUES) % NB_COQUES], casque);
        r.draw(g_visiere, g_matVisiere, casque);
        g_dessines++;
    }
    g_n = 0;
}

int dessinesDerniereImage() { return g_dessines; }

} // namespace wfs::deux_roues
