// WaForSpeed — WFS-73 : conducteurs dans les voitures (voir conducteurs.h).
#include "pietons.h"   // WFS-210 : tenue::tenueLieu
#include "conducteurs.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include "../core/log.h"
#include "../core/platform.h"   // readAsset : sommet du corps lu dans le .wfp
#include "../game/game.h"
#include "../render/renderer.h"
#include "../render/squelette.h"
#include "../render/vehicule_cockpit.h"   // WFS-201 : assise de chaque modèle (lecture seule)

namespace wfs::conducteurs {

namespace {

// stature d'ANSUR-II (references/reel/anthropometrie.json : humain.hommes.stature 1756,2 ± 68,6 mm, humain.femmes.stature
// 1628,5 ± 64,2 mm) : échelle 1 = stature réelle ÷ sommet du corps du personnage (mesuré dans le .wfp) ; chaque conducteur tiré à ± 1 écart-type
constexpr float STATURE_M[2] = {1.7562f, 1.6285f}, ECART_M[2] = {0.0686f, 0.0642f};
// repères du visage pour un adulte de 1,75 m (E, proportions : œil à 0,936 de la stature ; crâne 0,125 au-dessus de l'œil ;
// œil 0,09 devant l'articulation de la tête ; contact du siège 0,08 sous l'articulation de la cuisse), mis à la taille du personnage
constexpr float OEIL_STATURE = 0.936f, CRANE_SUR_OEIL = 0.125f / 1.75f, OEIL_AVANT = 0.09f / 1.75f, CUISSE_SIEGE = 0.08f / 1.75f;

// mêmes palettes que pedLook (src/world/pietons.cpp, WFS-19 ; codes sRGB) : pietons.* est hors de la zone de ce ticket
vec3 hx(unsigned h) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f}; }
// WFS-211 (RG-03) : multiplicateur d'apparence qui donne l'albédo visé : le shader (scene_skin.frag, bloc WFS-72) travaille sur
// les codes sRGB : albédo = (texture × peau)^2,2 ; texture des humains : teinte unie (222, 178, 150) (convert_people_makehuman.py)
vec3 peauApparence(const vec3& albedo) {
    const vec3 tex{222 / 255.f, 178 / 255.f, 150 / 255.f};
    return {std::pow(albedo.x, 1 / 2.2f) / tex.x, std::pow(albedo.y, 1 / 2.2f) / tex.y, std::pow(albedo.z, 1 / 2.2f) / tex.z};
}
const vec3 PEAU[] = {{1.04f, 1.02f, 1.0f}, {1.0f, 0.96f, 0.93f}, {0.93f, 0.84f, 0.76f}, {0.8f, 0.66f, 0.55f}, {0.6f, 0.45f, 0.36f}, {0.44f, 0.32f, 0.25f}};
const unsigned CHEVEUX[] = {0x17110d, 0x2a1b12, 0x3e2817, 0x5b3b22, 0x8a6a3e, 0xb89a68, 0x5a2414, 0x7d7a76};
const unsigned HAUTS[] = {0xf2f2ee, 0xe9e4d8, 0x1c2333, 0x2b4a7a, 0x8f1d24, 0x2f5a43, 0xd8b04a, 0x141416, 0x6a4b8c, 0xc96f3b, 0x7fa7c9, 0xb9b2a6, 0xd06a8a, 0x3b4048, 0x556b2f, 0xefe0c4};
const unsigned BAS[] = {0x2b3a55, 0x25324a, 0x3a4a66, 0x17181c, 0x2d2f33, 0x6b6f75, 0xc2b08c, 0x8a7a5c, 0xe8e4da, 0x4a3b2c, 0x1f2a3a};
const char* const COIFFURES[2][3] = {{"Hair_SimpleParted", "Hair_Buzzed", "Hair_Beard"}, {"Hair_Long", "Hair_Buns", "Hair_BuzzedFemale"}};

struct Modele {
    Personnage p; PoseSquelette pose;
    float stature = 1.75f;       // sommet du corps en pose de repos (sommets « corps », hors cheveux), unités du personnage
    vec3 oeil{};                 // œil dans la pose assise (repère du personnage, m)
    float oeilRel = 0.75f, crane = 0.1f;   // œil − contact du siège ; crâne − œil (pose assise)
    const Plage* corps[4] = {nullptr, nullptr, nullptr, nullptr};   // WFS-317 : corps-3 (Basse, au-delà de 30 m)
    const Plage* coiffure[3][4] = {};
};
Modele g_m[2];   // 0 homme, 1 femme (repli)
Modele g_h[NB_HUMAINS];   // WFS-211 : humains réalistes
bool g_pret = false, g_humains = false;
Attribution g_attr;

struct Entree { const Model* m; mat4 monde; const char* car; uint32_t graine; float d2; };
Entree g_file[FILE_MAX]; int g_n = 0, g_dessines = 0;

int osParNom(const Personnage& p, const char* nom) {
    for (size_t i = 0; i < p.os.size(); i++) if (p.os[i].nom == nom) return (int)i;
    return -1;
}
vec3 position(const mat4& m) { return {m.at(0, 3), m.at(1, 3), m.at(2, 3)}; }

// sommet du corps (y max des sommets de partie 0 : ni yeux ni cheveux) lu dans le .wfp (en-tête documenté dans
// tools/convert_people.mjs) : Personnage::hauteur est la hauteur de l'articulation de la tête (1,60 m pour l'homme Quaternius,
// dont le corps mesure 1,81 m) et ne donne pas la stature
float sommetCorps(const char* chemin) {
    std::vector<uint8_t> b;
    if (!readAsset(chemin, b) || b.size() < 12) return 0;
    size_t o = 4;
    auto u32 = [&](uint32_t& v) { if (o + 4 > b.size()) return false; std::memcpy(&v, &b[o], 4); o += 4; return true; };
    uint32_t version = 0, nTex = 0;
    if (!u32(version) || (version != 2 && version != 3) || !u32(nTex) || nTex > 8) return 0;
    for (uint32_t i = 0; i < nTex; i++) { uint32_t n = 0; if (!u32(n) || o + n > b.size()) return 0; o += n; }
    // WFS-266 (accroche) : humain réaliste (version 3) : stature écrite par convert_people_makehuman.py (le crâne est en partie 2)
    if (float h = 0; version == 3 && o + 12 <= b.size() && (std::memcpy(&h, &b[o + 8], 4), h > 0.5f && h < 2.5f)) return h;
    o += 7 * 4;   // rugosité, métal, hauteur, corps[4]
    uint32_t nV = 0;
    if (!u32(nV)) return 0;
    o += 4 * 4;   // nIndices, nPlages, nOs, nClips
    const size_t pas = version == 2 ? 68 : 84, partie = pas - 4;   // partie : dernier flottant de cloth
    if (o + (size_t)nV * pas > b.size()) return 0;
    float haut = 0;
    for (uint32_t v = 0; v < nV; v++) {
        float y = 0, part = 0;
        std::memcpy(&y, &b[o + v * pas + 4], 4); std::memcpy(&part, &b[o + v * pas + partie], 4);
        if (part < 0.5f) haut = std::max(haut, y);
    }
    return haut;
}

bool chargerModele(Renderer& r, const char* chemin, Modele& M, int sexe) {
    if (!chargerPersonnage(r, chemin, M.p)) return false;
    preparerPose(M.p, M.pose);
    const int tete = osParNom(M.p, "Head"), cuisse = osParNom(M.p, "thigh_l");
    if (tete < 0 || cuisse < 0) { WFS_ERR("conducteurs : %s : os Head / thigh_l absents", chemin); return false; }
    M.stature = sommetCorps(chemin);
    if (M.stature < 0.5f) { WFS_ERR("conducteurs : %s : sommet du corps illisible", chemin); return false; }
    const float h = M.stature;
    echantillonner(M.p, -1, 0, M.pose);   // repos (debout) : hauteur de l'œil au-dessus de l'articulation de la tête
    const float oeilSurTete = h * OEIL_STATURE - position(M.pose.modele[(size_t)tete]).y;
    echantillonner(M.p, clipParNom(M.p, "Sitting_Idle_Loop"), 0, M.pose);   // RG-02 : pose assise figée (palette une fois)
    const vec3 t = position(M.pose.modele[(size_t)tete]), c = position(M.pose.modele[(size_t)cuisse]);
    M.oeil = {t.x, t.y + oeilSurTete, t.z + h * OEIL_AVANT};
    M.oeilRel = M.oeil.y - (c.y - h * CUISSE_SIEGE);
    M.crane = h * CRANE_SUR_OEIL;
    for (int lod = 0; lod < 4; lod++) {
        M.corps[lod] = plageParNom(M.p, "corps-" + std::to_string(lod));
        for (int k = 0; k < 3 && sexe >= 0; k++) M.coiffure[k][lod] = plageParNom(M.p, std::string(COIFFURES[sexe][k]) + "-" + std::to_string(lod));
    }
    // WFS-317 : niveau 3 absent (modèle sans niveau 2) : niveau 2 (même repli que les piétons, pietons.cpp)
    if (!M.corps[3]) { M.corps[3] = M.corps[2]; for (int k = 0; k < 3; k++) M.coiffure[k][3] = M.coiffure[k][2]; }
    if (sexe < 0) return M.corps[0] && M.corps[1] && M.corps[2] && M.oeilRel > 0.3f;   // WFS-211 : humain réaliste (pas de journal par modèle)
    WFS_LOG("conducteurs : %s : corps de %.3f m, œil assis %.3f m au-dessus du siège, crâne %.3f m au-dessus de l'œil (pose Sitting_Idle_Loop) (WFS-73)",
            chemin, M.stature, M.oeilRel, M.crane);
    return M.corps[0] && M.corps[1] && M.oeilRel > 0.3f;
}

} // namespace

bool preparer(Renderer& r) {
    if (g_pret) return true;
    g_attr = Attribution{};
    // WFS-211 (RG-02) : les 12 humains réalistes ; un seul manquant → repli complet sur homme / femme (RG-05 de WFS-209)
    g_humains = true;
    float sMin = 9, sMax = 0;
    for (int i = 0; i < NB_HUMAINS && g_humains; i++) {
        g_humains = chargerModele(r, (std::string("people/humains/") + HUMAINS[i] + ".wfp").c_str(), g_h[i], -1);
        sMin = std::min(sMin, g_h[i].stature); sMax = std::max(sMax, g_h[i].stature);
    }
    if (g_humains) {
        WFS_LOG("conducteurs (WFS-211) : 12 humains réalistes, statures %.3f à %.3f m, 3 niveaux de détail (corps-0 < %.0f m ≤ corps-1 < %.0f m ≤ corps-2)",
                sMin, sMax, LOD_M[0], LOD_M[1]);
        return g_pret = true;
    }
    WFS_ERR("conducteurs (WFS-211) : humains réalistes illisibles, repli sur homme / femme");
    for (Modele& M : g_h) { if (M.p.ok) detruirePersonnage(r, M.p); M = Modele{}; }
    g_pret = chargerModele(r, "people/homme.wfp", g_m[0], 0) && chargerModele(r, "people/femme.wfp", g_m[1], 1);
    if (!g_pret) { WFS_ERR("conducteurs (WFS-73) : personnages illisibles, voitures sans conducteur"); liberer(r); }
    return g_pret;
}

void liberer(Renderer& r) {
    for (Modele& M : g_m) { if (M.p.ok) detruirePersonnage(r, M.p); M = Modele{}; }
    for (Modele& M : g_h) { if (M.p.ok) detruirePersonnage(r, M.p); M = Modele{}; }
    g_pret = g_humains = false; g_n = 0; g_attr = Attribution{};
}

void ajouter(const Model& m, const mat4& monde, const char* carId, uint32_t graine) {
    if (!g_pret || !m.ok || g_n >= FILE_MAX) return;
    g_file[g_n++] = {&m, monde, carId, graine, 0.f};
}

void dessinerFile(Renderer& r, const vec3& cam) {
    g_dessines = 0;
    if (!g_pret) { g_n = 0; return; }
    const float dMax2 = DISTANCE_MAX_M * DISTANCE_MAX_M;
    int n = 0;
    for (int i = 0; i < g_n; i++) {   // RG-05 : à moins de 50 m seulement
        const vec3 p = position(g_file[i].monde) - cam;
        g_file[i].d2 = p.x * p.x + p.y * p.y + p.z * p.z;
        if (g_file[i].d2 <= dMax2) g_file[n++] = g_file[i];
    }
    std::sort(g_file, g_file + n, [](const Entree& a, const Entree& b) { return a.d2 < b.d2; });   // RG-06 : les plus loin omis
    for (int i = 0; i < n; i++) {
        const Entree& e = g_file[i];
        Mulberry32 R(e.graine * 2654435761u + 0x73u);
        auto tirage = [&]() { return (float)R.next(); };
        auto choix = [&](auto& a) { return a[std::min((size_t)(tirage() * (float)(sizeof(a) / sizeof(a[0]))), sizeof(a) / sizeof(a[0]) - 1)]; };
        int sexe = tirage() < 0.5f ? 1 : 0;
        const int coiffure = std::min((int)(tirage() * 3), 2);
        ApparencePersonnage app;
        app.peau = choix(PEAU); app.cheveux = hx(choix(CHEVEUX)); app.haut = hx(choix(HAUTS)); app.bas = hx(choix(BAS));
        app.chaussures = hx(0x18181a); app.maillot = app.haut;
        app.vetement = {tirage() < 0.3f ? 1.95f : 0.45f, 1.9f, 1.f, 0.f};   // manches longues ou courtes, pantalon (pedLook)
        const float lacet = PI + (tirage() - 0.5f) * 0.12f;                   // RG-03 : π ± 0,06
        float stature = STATURE_M[sexe] + (tirage() * 2.f - 1.f) * ECART_M[sexe];
        const Modele* pm = &g_m[sexe];
        if (g_humains) {   // WFS-211 : morphotype stable par voiture (RG-02), stature du modèle (déjà celle d'ANSUR-II), peau M-03
            const int h = g_attr.choisir(e.graine);
            pm = &g_h[h]; sexe = humaineFemme(h) ? 1 : 0; stature = pm->stature;
            app.peau = peauApparence(albedoPeau(h));
        }
        tenue::tenueLieu(tenue::carteCourante(), e.graine, sexe == 1, app);   // WFS-210 (RG-01) : tenue selon la carte (pietons.cpp)
        const Modele& M = *pm;
        app.corps = M.p.corps;
        // place : œil de la vue cockpit ; assise du modèle (WFS-201), sinon 0,42 m (berline) ; pavillon mesuré (RG-01)
        const Model& mo = *e.m;
        const cockpit::InterieurDef* def = e.car ? cockpit::interieurPour(e.car) : nullptr;
        const float sol = mo.bmin.y, assise = sol + (def ? def->assise : 0.42f);
        const float cible = stature / M.stature * M.oeilRel;   // œil − siège à l'échelle 1 (pose du personnage)
        const Placement P = placer(assise, cible, M.oeilRel, M.crane, mo.siege.x, mo.siege.w > 0.5f);
        const mat4 monde = e.monde * translate({mo.siege.y, P.oeilY, mo.siege.z}) * rotateY(lacet) * scale({P.echelle, P.echelle, P.echelle})
                           * translate({-M.oeil.x, -M.oeil.y, -M.oeil.z});
        const int lod = g_humains ? g_attr.niveau(e.graine, e.d2, r.qualiteBasse) : (e.d2 < 15.f * 15.f ? 0 : 1);   // WFS-317 : 3e seuil en Basse
        const Plage* c = M.corps[lod] ? M.corps[lod] : M.corps[2];
        r.drawSkinned(M.p.maillage, M.p.materiau, monde, M.pose.palette.data(), (int)M.pose.palette.size(), c->premier, c->nombre, &app);
        if (const Plage* h = M.coiffure[coiffure][lod])
            r.drawSkinned(M.p.maillage, M.p.materiau, monde, M.pose.palette.data(), (int)M.pose.palette.size(), h->premier, h->nombre, &app);
        g_dessines++;
    }
    g_attr.finImage();
    g_n = 0;
}

int dessinesDerniereImage() { return g_dessines; }

// banc d'essai : voiture à l'arrêt vue de côté (vitrine, caméra à 6,5 m), jour et nuit : berline (Golf), SUV haut (Range Rover
// Sport), supercar basse (SF90) : conducteurs-<voiture>-<moment>.png ; une vue arrière gauche (trois quarts) de la Golf
void benchConducteurs(Game& g, Renderer& r, const std::string& out) {
    struct Cas { const char* car; float orbit; };
    const Cas cas[] = {{"golf", 1.5708f}, {"rrsport", 1.5708f}, {"sf90", 1.5708f}, {"golf", 2.4f}};
    for (const char* moment : {"jour", "nuit"}) {
        setenv("WFS_MOMENT", moment, 1);
        int k = 0;
        for (const Cas& c : cas) {
            if (!g.init(r, c.car, "ville")) { WFS_ERR("conducteurs : chargement %s impossible", c.car); continue; }
            g.setAutopilot(false); g.camMode = 2; g.orbit = c.orbit;
            double t = 1200;
            for (int i = 0; i < 60; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 0) g.render(t); }
            g.render(t); g.render(t + 0.01);
            const std::string nom = std::string("conducteurs-") + c.car + (k++ == 3 ? "-arriere" : "") + "-" + moment;
            r.capturePng(out + "/" + nom + ".png");
            WFS_LOG("capture %s : %d conducteur(s) dessiné(s) (WFS-73)", nom.c_str(), g_dessines);
            g.shutdown();
        }
    }
    unsetenv("WFS_MOMENT");
}

} // namespace wfs::conducteurs
