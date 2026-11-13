// WaForSpeed — WFS-484 (RG-01, RG-04, RG-05) : dessin des plaques des autres voitures (thread principal).
// Ressources, toutes faites au chargement de la carte (preparer) :
//   - un atlas de 64 plaques (couleur sRGB + normales, 2048 × 512 px, ≈ 11 Mo avec les mipmaps), images de WFS-483 à 256 px ;
//   - par case, la plaque WFS-483 (complète et simple) dont les uv pointent dans sa case : 1 appel de dessin par pièce, comme
//     le joueur, aucun changement de shader ;
//   - par modèle de voiture connu (vehicule_plaque_defs.cpp), ses porte-plaques avant / arrière (complet et simple), rangés par
//     indice : aucune allocation à l'image.
// Fantôme (RG-03) : variantes translucides (MAT_BLEND | MAT_FANTOME, comme Renderer::draw le fait pour la voiture fantôme).
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "../core/log.h"
#include "../core/platform.h"
#include "../game/sauvegarde.h"
#include "renderer.h"
#include "vehicule_plaque.h"
#include "vehicule_plaques_autres.h"

namespace wfs::plaques_autres {
namespace {

namespace pj = plaque_joueur;

struct PiecesVoiture { Mesh support[2][2]; };   // [côté][niveau]

struct Ressources {
    bool pret = false;
    Image couleur, normales;
    std::vector<uint8_t> pxCouleur, pxNormales;   // atlas sur le processeur (gardé pour refaire une case entre deux courses)
    std::array<std::string, CASES> textes;
    struct Duel { std::string id, texte; uint32_t vu = 0; };   // case DUEL_0 + k : adversaire id, plaque reçue (vide : au hasard)
    std::array<Duel, DUEL_N> duel;
    uint32_t horloge = 0;
    std::array<std::string, DUEL_N> duelHasard;
    Mesh plaque[CASES][2], vis;
    std::vector<PiecesVoiture> voitures;           // même ordre que plaque_joueur::voiture(i)
    Material matSupport, matVis, matPlaque, matPlaqueArriere;               // opaques
    Material matSupportF, matVisF, matPlaqueF;                              // fantôme (translucides)
    std::vector<uint8_t> ttf;
    std::string joueur;                            // plaque du joueur au dernier tirage
};
Ressources g;

Mesh versGpu(Renderer& r, const pj::Maillage& m, int cas) {
    std::vector<Vertex> v; v.reserve(m.sommets.size());
    for (const pj::Sommet& s : m.sommets) {
        Vertex x{}; x.px = s.p.x; x.py = s.p.y; x.pz = s.p.z; x.nx = s.n.x; x.ny = s.n.y; x.nz = s.n.z;
        if (cas >= 0) uvCase(cas, s.u, s.v, x.u, x.v); else { x.u = s.u; x.v = s.v; }
        v.push_back(x);
    }
    return r.makeMesh(v, m.indices);
}

std::string plaqueJoueur() {
    if (const char* e = std::getenv("WFS_PLAQUE")) return e;   // banc : texte imposé (comme WFS-483)
    const std::string& p = sauvegarde().profil.plaque;
    return p.empty() ? std::string("WFS") : p;
}

// image de la case cas (texte) recopiée dans l'atlas, marges comprises (bord recopié)
void dessinerCase(int cas, const std::string& texte) {
    pj::ImagePlaque im;
    if (!pj::dessinerImage(texte, g.ttf.data(), im, CASE_L) || im.largeur != CASE_L || im.hauteur != PLAQUE_PX_H) return;
    const int col = cas % COLONNES, lig = cas / COLONNES;
    for (int y = 0; y < CASE_H; y++) {
        const int ys = std::clamp(y - MARGE_PX, 0, PLAQUE_PX_H - 1);
        const size_t dst = (((size_t)(lig * CASE_H + y)) * ATLAS_L + (size_t)col * CASE_L) * 4, src = (size_t)ys * CASE_L * 4;
        std::memcpy(&g.pxCouleur[dst], &im.couleur[src], (size_t)CASE_L * 4);
        std::memcpy(&g.pxNormales[dst], &im.normales[src], (size_t)CASE_L * 4);
    }
    g.textes[(size_t)cas] = texte;
}

void libererAtlas(Renderer& r) {
    if (!g.couleur.view && !g.matPlaque.set) return;
    r.vk().waitIdle();   // rare : chargement de la carte ou entre deux courses ; l'atlas n'est plus lu par une image en vol
    for (Material* m : {&g.matPlaque, &g.matPlaqueArriere, &g.matPlaqueF}) { r.destroy(*m); *m = Material{}; }
    r.vk().destroy(g.couleur); r.vk().destroy(g.normales);
    g.couleur = Image{}; g.normales = Image{};
}

void envoyerAtlas(Renderer& r) {
    libererAtlas(r);
    g.couleur = r.vk().uploadTexture(g.pxCouleur.data(), ATLAS_L, ATLAS_H, true);
    g.normales = r.vk().uploadTexture(g.pxNormales.data(), ATLAS_L, ATLAS_H, false);
    Material m; m.color = {1, 1, 1, 1}; m.metal = 0; m.rough = pj::PLAQUE_RUGOSITE;   // même film blanc mat que le joueur
    g.matPlaque = r.makeMaterial(m, &g.couleur, &g.normales, nullptr);
    Material a = m; a.flags |= MAT_EMTEX;                                             // arrière : éclaireur de plaque
    g.matPlaqueArriere = r.makeMaterial(a, &g.couleur, &g.normales, nullptr);
    Material f = m; f.flags |= MAT_BLEND | MAT_FANTOME;                               // fantôme (RG-03)
    g.matPlaqueF = r.makeMaterial(f, &g.couleur, &g.normales, nullptr);
}

// indice du modèle dans plaque_joueur::voiture(i) : recherche directe (≈ 40 modèles, quelques µs par image, sans allocation)
int indiceVoiture(const char* id) {
    if (!id) return -1;
    for (int k = 0; k < pj::nombreVoitures(); k++) if (std::strcmp(pj::voiture(k).id, id) == 0) return k;
    return -1;
}

} // namespace

void preparer(Renderer& r, const char* idCarte) {
    liberer(r);
    if (!readAsset("fonts/Rajdhani-Bold.ttf", g.ttf)) { WFS_ERR("plaques des autres voitures : police introuvable"); return; }
    // atlas (RG-05) : 64 textes tirés depuis la graine de la carte
    g.joueur = plaqueJoueur();
    const std::array<std::string, CASES> textes = tirerAtlas(graineCarte(idCarte), g.joueur);
    for (int k = 0; k < DUEL_N; k++) g.duelHasard[(size_t)k] = textes[(size_t)(DUEL_0 + k)];
    g.pxCouleur.assign((size_t)ATLAS_L * ATLAS_H * 4, 255);
    g.pxNormales.assign((size_t)ATLAS_L * ATLAS_H * 4, 255);
    for (int c = 0; c < CASES; c++) {
        const int d = c - DUEL_0;
        dessinerCase(c, d >= 0 && d < DUEL_N && !g.duel[(size_t)d].texte.empty() ? g.duel[(size_t)d].texte : textes[(size_t)c]);
    }
    envoyerAtlas(r);
    // maillages : plaque de chaque case (complète, simple), vis ; porte-plaques de chaque modèle
    for (int n = 0; n < 2; n++) {
        const pj::Pieces p = pj::construire(n == 0, nullptr);
        for (int c = 0; c < CASES; c++) g.plaque[c][n] = versGpu(r, p.plaque, c);
        if (n == 0) g.vis = versGpu(r, p.vis, -1);
    }
    g.voitures.resize((size_t)pj::nombreVoitures());
    for (int i = 0; i < pj::nombreVoitures(); i++)
        for (int cote = 0; cote < 2; cote++)
            for (int n = 0; n < 2; n++) {
                const pj::PlacesVoiture& pv = pj::voiture(i);
                g.voitures[(size_t)i].support[cote][n] = versGpu(r, pj::construire(n == 0, cote ? &pv.arriere : &pv.avant).support, -1);
            }
    Material s; s.color = {pj::SUPPORT_RVB[0], pj::SUPPORT_RVB[1], pj::SUPPORT_RVB[2], 1}; s.metal = 0; s.rough = pj::SUPPORT_RUGOSITE;
    g.matSupport = r.makeMaterial(s, nullptr, nullptr, nullptr);
    Material v; v.color = {pj::VIS_RVB[0], pj::VIS_RVB[1], pj::VIS_RVB[2], 1}; v.metal = 1; v.rough = pj::VIS_RUGOSITE;
    g.matVis = r.makeMaterial(v, nullptr, nullptr, nullptr);
    s.flags |= MAT_BLEND | MAT_FANTOME; g.matSupportF = r.makeMaterial(s, nullptr, nullptr, nullptr);
    v.flags |= MAT_BLEND | MAT_FANTOME; g.matVisF = r.makeMaterial(v, nullptr, nullptr, nullptr);
    g.pret = true;
    WFS_LOG("plaques des autres voitures : atlas de %d plaques (%d × %d px), %d modèles ; trafic n° 0 « %s », fantôme « %s »",
            CASES, ATLAS_L, ATLAS_H, pj::nombreVoitures(), g.textes[0].c_str(), g.textes[FANTOME].c_str());
}

void plaqueDuel(const std::string& id, const std::string& brut) {
    const std::string t = filtrerPlaqueRecue(brut);
    g.horloge++;
    int k = -1, vieux = 0;
    for (int i = 0; i < DUEL_N; i++) {
        if (g.duel[(size_t)i].id == id) { k = i; break; }
        if (g.duel[(size_t)i].vu < g.duel[(size_t)vieux].vu) vieux = i;
    }
    if (k < 0) { k = vieux; g.duel[(size_t)k].id = id; }
    g.duel[(size_t)k].texte = t;
    g.duel[(size_t)k].vu = g.horloge;
}

int caseDuel(const std::string& id) {
    for (int i = 0; i < DUEL_N; i++) if (g.duel[(size_t)i].id == id) return DUEL_0 + i;
    return DUEL_0 + (int)(graineCarte(id.c_str()) % DUEL_N);
}

void entreCourses(Renderer& r, bool enCourse) {
    if (!g.pret || enCourse) return;   // RG-05 : jamais pendant la course
    bool change = false;
    const std::string j = plaqueJoueur();
    if (j != g.joueur) {   // plaque changée (écran Profil) : fantôme, et toute case qui aurait le même texte (RG-02)
        g.joueur = j;
        Mulberry32 h(graineCarte(j.c_str()));
        for (int c = 0; c < CASES; c++) {
            if (c == FANTOME) continue;
            if (g.textes[(size_t)c] == j) { dessinerCase(c, tirer(h, j)); change = true; }
        }
        dessinerCase(FANTOME, j); change = true;
    }
    for (int k = 0; k < DUEL_N; k++) {
        const std::string& voulu = g.duel[(size_t)k].texte.empty() ? g.duelHasard[(size_t)k] : g.duel[(size_t)k].texte;
        if (voulu != g.textes[(size_t)(DUEL_0 + k)]) { dessinerCase(DUEL_0 + k, voulu); change = true; }
    }
    if (change) envoyerAtlas(r);
}

void dessiner(Renderer& r, const char* idVoiture, const mat4& monde, float echelle, int cas, Genre genre, bool nuit, const vec3& camera) {
    if (!g.pret || !g.matPlaque.set) return;
    const int i = indiceVoiture(idVoiture);
    if (i < 0) return;   // modèle sans place connue : aucune plaque (comme WFS-483)
    const vec3 centre{monde.at(0, 3), monde.at(1, 3), monde.at(2, 3)};
    const float d = length(camera - centre);
    if (genre == Genre::Trafic && r.qualiteBasse && d > TRAFIC_BASSE_MAX_M) return;   // RG-05
    // même règle de détail que le joueur (WFS-483 RG-06, RG-07) ; en Basse, version simple pour tous (RG-05)
    const int niveau = d > pj::LOD_M || (r.qualiteBasse && (genre == Genre::Trafic || d > pj::LOD_BASSE_M)) ? 1 : 0;
    cas = std::clamp(cas, 0, CASES - 1);
    const bool fantome = genre == Genre::Fantome;
    g.matPlaqueArriere.emissive = nuit ? vec3{pj::ECLAIREUR, pj::ECLAIREUR, pj::ECLAIREUR} : vec3{0, 0, 0};
    const pj::PlacesVoiture& pv = pj::voiture(i);
    for (int cote = 0; cote < 2; cote++) {
        // la place suit l'échelle du modèle dessiné ; la plaque garde ses vraies mesures (RG-01)
        mat4 R = pj::repere(cote ? pv.arriere : pv.avant);
        R.at(0, 3) *= echelle; R.at(1, 3) *= echelle; R.at(2, 3) *= echelle;
        const mat4 M = monde * R;
        if (genre == Genre::Trafic) r.traficProchainDessin();   // WFS-453 : même coupe à distance que la voiture
        r.draw(g.voitures[(size_t)i].support[cote][niveau], fantome ? g.matSupportF : g.matSupport, M);
        if (genre == Genre::Trafic) r.traficProchainDessin();
        r.draw(g.plaque[cas][niveau], fantome ? g.matPlaqueF : cote && nuit ? g.matPlaqueArriere : g.matPlaque, M);
        if (niveau == 0) r.draw(g.vis, fantome ? g.matVisF : g.matVis, M);
    }
}

void liberer(Renderer& r) {
    if (!g.pret && !g.couleur.view) return;
    libererAtlas(r);
    for (auto& c : g.plaque) for (Mesh& m : c) r.destroy(m);
    r.destroy(g.vis);
    for (PiecesVoiture& v : g.voitures) for (auto& c : v.support) for (Mesh& m : c) r.destroy(m);
    g.voitures.clear();
    for (Material* m : {&g.matSupport, &g.matVis, &g.matSupportF, &g.matVisF}) { r.destroy(*m); *m = Material{}; }
    g.pret = false;
}

const std::string& texteCase(int cas) { return g.textes[(size_t)std::clamp(cas, 0, CASES - 1)]; }

} // namespace wfs::plaques_autres
