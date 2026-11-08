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
    PoseSquelette couche;      // RG-05 : après un accident, couché sur le dos au sol (pose debout « Idle_Loop » basculée)
    mat4 placeCouche, casqueCouche;
    const Plage* corps[3] = {nullptr, nullptr, nullptr};
    bool femme = false; int indice = 0;
};
vec3 position(const mat4& m) { return {m.at(0, 3), m.at(1, 3), m.at(2, 3)}; }
Humain g_h[2];
Mesh g_coque{}, g_visiere{};
Material g_matCoque[NB_COQUES]{}, g_matVisiere{};
bool g_pret = false;
struct Entree { mat4 monde, sol; int modele; uint32_t graine; float d2; bool ejecte; int sens; };
Entree g_file[PILOTES_MAX]; int g_n = 0, g_dessines = 0, g_notes = 0, g_notesDerniere = 0;

// ---------- quaternions (buste penché : rotation d'un os autour de l'axe x du personnage, dans le repère du modèle) ----------
Quat qmul(const Quat& a, const Quat& b) {
    return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y, a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w, a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}
Quat qconj(const Quat& q) { return {-q.x, -q.y, -q.z, q.w}; }
Quat qnorm(Quat q) { const float l = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w); return {q.x / l, q.y / l, q.z / l, q.w / l}; }
// rotation (normée) d'une matrice du modèle (colonnes normées : échelle de l'armature retirée)
Quat quatDe(const mat4& m) {
    float c[3][3];
    for (int j = 0; j < 3; j++) {
        const vec3 v{m.at(0, j), m.at(1, j), m.at(2, j)};
        const float l = std::max(1e-8f, std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z));
        c[0][j] = v.x / l; c[1][j] = v.y / l; c[2][j] = v.z / l;
    }
    Quat q; const float tr = c[0][0] + c[1][1] + c[2][2];
    if (tr > 0) { const float s = std::sqrt(tr + 1) * 2; q = {(c[2][1] - c[1][2]) / s, (c[0][2] - c[2][0]) / s, (c[1][0] - c[0][1]) / s, s / 4}; }
    else if (c[0][0] > c[1][1] && c[0][0] > c[2][2]) { const float s = std::sqrt(1 + c[0][0] - c[1][1] - c[2][2]) * 2; q = {s / 4, (c[0][1] + c[1][0]) / s, (c[0][2] + c[2][0]) / s, (c[2][1] - c[1][2]) / s}; }
    else if (c[1][1] > c[2][2]) { const float s = std::sqrt(1 + c[1][1] - c[0][0] - c[2][2]) * 2; q = {(c[0][1] + c[1][0]) / s, s / 4, (c[1][2] + c[2][1]) / s, (c[0][2] - c[2][0]) / s}; }
    else { const float s = std::sqrt(1 + c[2][2] - c[0][0] - c[1][1]) * 2; q = {(c[0][2] + c[2][0]) / s, (c[1][2] + c[2][1]) / s, s / 4, (c[1][0] - c[0][1]) / s}; }
    return qnorm(q);
}
// tourne l'os (et ses enfants) de `angle` autour de l'axe x du modèle (angle > 0 : le haut part vers +Z, vers l'avant du personnage)
void tournerOs(const Personnage& p, PoseSquelette& P, int os, float angle) {
    const int parent = p.os[(size_t)os].parent;
    const Quat rp = parent >= 0 ? quatDe(P.modele[(size_t)parent]) : Quat{};
    const Quat qx{std::sin(angle / 2), 0, 0, std::cos(angle / 2)};
    P.r[(size_t)os] = qnorm(qmul(qmul(qconj(rp), qmul(qx, rp)), P.r[(size_t)os]));
    recalculerPose(p, P);
}
// repère du casque (repère du personnage) pour la pose P : matrice de peau de la tête × centre du casque au repos
mat4 casqueDe(const Personnage& p, const PoseSquelette& P, int tete) {
    const vec3 t = position(inverse(p.os[(size_t)tete].bindInverse));   // articulation de la tête dans le repère du maillage
    return P.modele[(size_t)tete] * p.os[(size_t)tete].bindInverse * translate({t.x, t.y + CASQUE_HAUT, t.z + CASQUE_AVANT});
}

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
    echantillonner(H.p, clipParNom(H.p, "Sitting_Idle_Loop"), 0, P);
    // assise du personnage : articulation de la cuisse − CUISSE_SIEGE × stature, à l'aplomb du bassin
    const float stature = H.p.hauteur > 0 ? H.p.hauteur / 0.9f : 1.7f;   // articulation de la tête ≈ 0,9 × stature (seulement pour CUISSE_SIEGE)
    const vec3 b = position(P.modele[(size_t)bassin]), c = position(P.modele[(size_t)cuisse]);
    const vec3 assise{b.x, c.y - CUISSE_SIEGE * stature, b.z};
    // repère : rotation d'un demi-tour (le personnage regarde +Z, le deux-roues −Z), assise du personnage sur la selle
    H.place[k] = translate({0, G.selleY, G.selleZ}) * rotateY(PI_F) * translate({-assise.x, -assise.y, -assise.z});
    // point du .wfm → repère du personnage (inverse du repère ci-dessus : demi-tour = (x, y, z) → (−x, y, −z))
    auto versPerso = [&](float x, float y, float z) { return vec3{assise.x - x, assise.y + (y - G.selleY), assise.z - (z - G.selleZ)}; };
    // buste penché en avant (position de conduite), réparti sur les 3 vertèbres ; la tête se relève d'autant (regard vers la route)
    if (G.penche > 0) {
        const int sp[3] = {osParNom(H.p, "spine_01"), osParNom(H.p, "spine_02"), osParNom(H.p, "spine_03")};
        const float part[3] = {0.40f, 0.35f, 0.25f};
        for (int i = 0; i < 3; i++) if (sp[i] >= 0) tournerOs(H.p, P, sp[i], G.penche * part[i]);
        const int cou = osParNom(H.p, "neck_01");
        if (cou >= 0) tournerOs(H.p, P, cou, -G.penche * 0.5f);
        tournerOs(H.p, P, tete, -G.penche * 0.5f);
    }
    for (int cote : {-1, 1}) {   // cote −1 : gauche du deux-roues (x < 0) = gauche du personnage (_l)
        const char* s = cote < 0 ? "_l" : "_r";
        const int epaule = osParNom(H.p, std::string("upperarm") + s), coude = osParNom(H.p, std::string("lowerarm") + s), main = osParNom(H.p, std::string("hand") + s);
        const int hanche = osParNom(H.p, std::string("thigh") + s), genou = osParNom(H.p, std::string("calf") + s), pied = osParNom(H.p, std::string("foot") + s);
        if (epaule >= 0 && coude >= 0 && main >= 0) atteindre(H.p, P, epaule, coude, main, versPerso(cote * G.poigneeX, G.poigneeY, G.poigneeZ));
        if (hanche >= 0 && genou >= 0 && pied >= 0) atteindre(H.p, P, hanche, genou, pied, versPerso(cote * G.piedX, G.piedY, G.piedZ));
    }
    // casque posé dans le repère du maillage (au-dessus et un peu en avant de l'articulation de la tête), puis porté par la matrice de
    // peau de la tête (modèle × liaison inverse) : il tourne et se déplace avec la tête dessinée, quelle que soit la pose
    H.casque[k] = casqueDe(H.p, P, tete);
    return true;
}

// RG-05 : pilote éjecté, couché sur le dos : pose debout basculée de −90° autour de x (visage vers le ciel, tête vers −Z), bassin à
// l'origine, dos posé sur la chaussée (épaisseur du dos ≈ 0,13 m derrière l'axe du corps)
bool poserCouche(Humain& H) {
    preparerPose(H.p, H.couche);
    const int clip = clipParNom(H.p, "Idle_Loop"), bassin = osParNom(H.p, "pelvis"), tete = osParNom(H.p, "Head");
    if (bassin < 0 || tete < 0) return false;
    echantillonner(H.p, clip, 0, H.couche);
    const vec3 b = position(H.couche.modele[(size_t)bassin]);
    H.placeCouche = translate({0, 0.13f, 0}) * rotateX(-PI_F / 2) * translate({-b.x, -b.y, -b.z});
    H.casqueCouche = casqueDe(H.p, H.couche, tete);
    return true;
}

} // namespace

bool preparer(Renderer& r) {
    if (g_pret) return true;
    // échec (fichier absent ou incomplet) : les personnages déjà chargés sont rendus tout de suite (detruire ne voit que g_pret)
    auto echec = [&r]() { for (Humain& h : g_h) if (h.p.ok) { detruirePersonnage(r, h.p); h.p.ok = false; } return false; };
    for (int i = 0; i < 2; i++) {
        Humain& H = g_h[i];
        H.indice = i ? HUMAIN_F : HUMAIN_H; H.femme = conducteurs::humaineFemme(H.indice);
        const std::string chemin = std::string("people/humains/") + conducteurs::HUMAINS[H.indice] + ".wfp";
        if (!chargerPersonnage(r, chemin, H.p)) { WFS_ERR("deux-roues : %s illisible : pas de pilote (WFS-232)", chemin.c_str()); return echec(); }
        for (int lod = 0; lod < 3; lod++) H.corps[lod] = plageParNom(H.p, "corps-" + std::to_string(lod));
        for (int k = 0; k < 3; k++)
            if (!poser(H, k) || !H.corps[0]) { WFS_ERR("deux-roues : %s : os ou corps absents : pas de pilote (WFS-232)", chemin.c_str()); return echec(); }
        if (!poserCouche(H)) { WFS_ERR("deux-roues : %s : pose couchée impossible (WFS-232)", chemin.c_str()); return echec(); }
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

void noter(const mat4& monde, int modele, uint32_t graine, bool ejecte, const mat4& sol, int sens) {
    g_notes++;
    if (g_pret && g_n < PILOTES_MAX) g_file[g_n++] = {monde, sol, std::clamp(modele, 0, 2), graine, 0.f, ejecte, sens >= 0 ? 1 : -1};
}

// RG-02 : un pilote par deux-roues noté (le trafic en note 14 au plus, PILOTES_MAX = 16), à toute distance ; du plus proche au plus
// loin (ordre des niveaux de détail) ; accident : couché sur le dos, en travers, dans le repère au sol donné par le trafic
void dessinerFile(Renderer& r, const vec3& cam) {
    g_dessines = 0; g_notesDerniere = g_notes; g_notes = 0;
    if (!g_pret) { g_n = 0; return; }
    for (int i = 0; i < g_n; i++) {
        const vec3 d = position(g_file[i].monde) - cam;
        g_file[i].d2 = d.x * d.x + d.y * d.y + d.z * d.z;
    }
    std::sort(g_file, g_file + g_n, [](const Entree& a, const Entree& b) { return a.d2 < b.d2; });
    for (int i = 0; i < g_n; i++) {
        const Entree& e = g_file[i];
        Mulberry32 R(e.graine * 2654435761u + 0x2d2u);
        const Humain& H = g_h[R.next() < 0.7 ? 0 : 1];   // 7 pilotes sur 10 sont des hommes (choix de conception gardé par le PO le 2026-11-08, WFS-232)
        ApparencePersonnage app;
        app.peau = peauApparence(conducteurs::albedoPeau(H.indice)); app.corps = H.p.corps;
        tenue::tenueLieu(tenue::carteCourante(), e.graine, H.femme, app);
        app.vetement.x = 1.95f; app.vetement.y = 1.9f;   // manches longues et pantalon (tenue de deux-roues)
        const mat4 monde = e.ejecte ? e.sol * rotateY(PI_F / 2 * (float)e.sens) * H.placeCouche   // en travers, tête vers le deux-roues
                                    : e.monde * H.place[e.modele];
        const int lod = e.d2 < LOD_PILOTE_M[0] * LOD_PILOTE_M[0] ? 0 : e.d2 < LOD_PILOTE_M[1] * LOD_PILOTE_M[1] ? 1 : 2;
        const Plage* c = H.corps[lod] ? H.corps[lod] : H.corps[0];
        const PoseSquelette& P = e.ejecte ? H.couche : H.pose[e.modele];
        r.drawSkinned(H.p.maillage, H.p.materiau, monde, P.palette.data(), (int)P.palette.size(), c->premier, c->nombre, &app);
        const mat4 casque = monde * (e.ejecte ? H.casqueCouche : H.casque[e.modele]);
        r.draw(g_coque, g_matCoque[(int)(R.next() * NB_COQUES) % NB_COQUES], casque);
        r.draw(g_visiere, g_matVisiere, casque);
        g_dessines++;
    }
    g_n = 0;
}

int dessinesDerniereImage() { return g_dessines; }
int notesDerniereImage() { return g_notesDerniere; }
bool pret() { return g_pret; }

} // namespace wfs::deux_roues
