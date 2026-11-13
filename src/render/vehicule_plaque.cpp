// WaForSpeed — WFS-483 (RG-03 à RG-07) : dessin de la plaque 3D de la voiture du joueur (thread principal).
// Ressources : une texture de couleur et une de normales pour le texte du joueur (RG-07 : refaites seulement quand le texte
// change, GPU arrêté le temps de libérer l'ancienne) ; par voiture, les maillages du porte-plaque (sa jupe suit la caisse de
// cette voiture), préparés au choix de la voiture (Game::setCar, garage), jamais pendant la course ; plaque et vis communes.
// Dessin : 2 plaques × 3 pièces (porte-plaque, plaque, vis) ; au-delà de LOD_M, version simple (sans vis, coins à 3 segments).
// Nuit (RG-05) : la plaque arrière est éclairée par l'éclaireur de plaque (émission × couleur de la texture : le blanc s'éclaire,
// les caractères noirs restent noirs), la plaque avant ne l'est pas.
#include <cstdlib>
#include <map>
#include <string>
#include "../core/log.h"
#include "../core/platform.h"
#include "../game/sauvegarde.h"
#include "renderer.h"
#include "vehicule_plaque.h"

namespace wfs::plaque_joueur {
namespace {

constexpr float LOD_M = 14.f;              // au-delà, porte-plaque et plaque simplifiés, sans vis (RG-06 : jamais absente)
constexpr float LOD_BASSE_M = 4.f;         // qualité Basse : version simple dès 4 m (RG-07 : coût par image)
// RG-05 : luminance de la plaque éclairée ≈ 2 à 20 cd/m² (règlement ONU n° 4 : ≥ 2,5 cd/m² en tout point) ; en unités
// d'émission du moteur (fenêtres éclairées de la ville : 0,9 ; enseignes : 2 à 6), 0,30 × la couleur de la plaque
constexpr float ECLAIREUR = 0.30f;

struct Ressources {
    bool pret = false;
    Mesh plaque[2], vis;                   // [0] complet, [1] simple
    std::map<std::string, Mesh> supports;  // clé : id + côté + niveau (fait au choix de la voiture, jamais à chaque image)
    const PlacesVoiture* courante = nullptr;   // voiture dont les porte-plaques sont dans actuels
    const Mesh* actuels[2][2] = {};            // [côté][niveau] : lus à chaque image sans recherche ni allocation
    Material matSupport, matVis, matPlaque, matPlaqueArriere;
    Image couleur, normales;
    std::string texte;                     // texte de la texture actuelle
    std::vector<uint8_t> ttf;
    const char* imposeBanc = nullptr;
    bool masqueBanc = false;
};
Ressources g;

Mesh versGpu(Renderer& r, const Maillage& m) {
    std::vector<Vertex> v; v.reserve(m.sommets.size());
    for (const Sommet& s : m.sommets) { Vertex x{}; x.px = s.p.x; x.py = s.p.y; x.pz = s.p.z; x.nx = s.n.x; x.ny = s.n.y; x.nz = s.n.z; x.u = s.u; x.v = s.v; v.push_back(x); }
    return r.makeMesh(v, m.indices);
}

// porte-plaques de la voiture pv (faits si besoin) gardés dans actuels : la clé n'est construite qu'au changement de voiture
void choisirVoiture(Renderer& r, const PlacesVoiture& pv);

const Mesh& support(Renderer& r, const PlacesVoiture& pv, int cote, int niveau) {
    const std::string cle = std::string(pv.id) + (cote ? "/ar/" : "/av/") + std::to_string(niveau);
    auto it = g.supports.find(cle);
    if (it == g.supports.end()) {
        const Pieces p = construire(niveau == 0, cote ? &pv.arriere : &pv.avant);
        it = g.supports.emplace(cle, versGpu(r, p.support)).first;
    }
    return it->second;
}

void choisirVoiture(Renderer& r, const PlacesVoiture& pv) {
    if (g.courante == &pv) return;
    for (int cote = 0; cote < 2; cote++) for (int n = 0; n < 2; n++) g.actuels[cote][n] = &support(r, pv, cote, n);
    g.courante = &pv;
}

std::string texteVoulu() {
    if (g.imposeBanc) return g.imposeBanc;
    if (const char* e = std::getenv("WFS_PLAQUE")) return e;   // banc : texte imposé
    const std::string& p = sauvegarde().profil.plaque;
    return p.empty() ? std::string("WFS") : p;                  // profil pas encore créé (1er lancement) : plaque du jeu
}

void libererTexte(Renderer& r) {
    if (!g.couleur.view && !g.matPlaque.set) return;
    r.vk().waitIdle();   // rare (changement de plaque, hors course) : l'ancienne texture n'est plus lue par une image en vol
    r.destroy(g.matPlaque); r.destroy(g.matPlaqueArriere);
    r.vk().destroy(g.couleur); r.vk().destroy(g.normales);
    g.matPlaque = Material{}; g.matPlaqueArriere = Material{}; g.couleur = Image{}; g.normales = Image{};
}

void preparerTexte(Renderer& r, const std::string& texte) {
    libererTexte(r);
    ImagePlaque im;
    if (!dessinerImage(texte, g.ttf.data(), im)) { WFS_ERR("plaque : image impossible (police)"); g.texte = texte; return; }
    g.couleur = r.vk().uploadTexture(im.couleur.data(), IMAGE_L, IMAGE_H, true);
    g.normales = r.vk().uploadTexture(im.normales.data(), IMAGE_L, IMAGE_H, false);
    Material m; m.color = {1, 1, 1, 1}; m.metal = 0; m.rough = 0.62f;   // RG-05 : film blanc mat, rugueux
    g.matPlaque = r.makeMaterial(m, &g.couleur, &g.normales, nullptr);
    m.flags |= MAT_EMTEX;                                                 // arrière : éclaireur de plaque (émission × texture)
    g.matPlaqueArriere = r.makeMaterial(m, &g.couleur, &g.normales, nullptr);
    g.texte = texte;
    WFS_LOG("plaque du joueur : « %s », caractères de %.0f mm, texte de %.0f mm", texte.c_str(), im.hauteurTexteMm, im.largeurTexteMm);
}

} // namespace

void preparer(Renderer& r) {
    if (g.pret) return;
    if (!readAsset("fonts/Rajdhani-Bold.ttf", g.ttf)) WFS_ERR("plaque : police introuvable");
    for (int n = 0; n < 2; n++) {
        const Pieces p = construire(n == 0, nullptr);
        g.plaque[n] = versGpu(r, p.plaque);
        if (n == 0) g.vis = versGpu(r, p.vis);
    }
    // porte-plaque : polypropylène noir grainé (albédo ≈ 0,03, rugueux) ; vis : acier zingué (métal, satiné)
    Material s; s.color = {0.028f, 0.028f, 0.030f, 1}; s.metal = 0; s.rough = 0.68f;
    g.matSupport = r.makeMaterial(s, nullptr, nullptr, nullptr);
    Material v; v.color = {0.62f, 0.64f, 0.66f, 1}; v.metal = 1; v.rough = 0.38f;
    g.matVis = r.makeMaterial(v, nullptr, nullptr, nullptr);
    g.pret = true;
}

void voiturePreparee(Renderer& r, const char* idVoiture) {
    if (!g.pret) preparer(r);
    const PlacesVoiture* pv = places(idVoiture);
    if (!pv) return;
    choisirVoiture(r, *pv);
    const std::string t = texteVoulu();
    if (t != g.texte || !g.matPlaque.set) preparerTexte(r, t);
}

void dessiner(Renderer& r, const char* idVoiture, const mat4& monde, bool nuit, const vec3& camera) {
    const PlacesVoiture* pv = places(idVoiture);
    if (!pv || g.masqueBanc) return;
    if (!g.pret) preparer(r);
    const std::string t = texteVoulu();
    if (t != g.texte || !g.matPlaque.set) preparerTexte(r, t);   // RG-03 : plaque changée au garage (écran Profil)
    if (!g.matPlaque.set) return;
    choisirVoiture(r, *pv);   // déjà fait au choix de la voiture : simple comparaison de pointeur
    const vec3 centre{monde.at(0, 3), monde.at(1, 3), monde.at(2, 3)};
    // RG-06 / RG-07 : version simple au-delà de LOD_M, et en Basse dès LOD_BASSE_M (caméra de poursuite à ≈ 5 m) ; complète au garage
    const float d = length(camera - centre);
    const int niveau = d > LOD_M || (r.qualiteBasse && d > LOD_BASSE_M) ? 1 : 0;
    g.matPlaqueArriere.emissive = nuit ? vec3{ECLAIREUR, ECLAIREUR, ECLAIREUR} : vec3{0, 0, 0};
    for (int cote = 0; cote < 2; cote++) {
        const mat4 M = monde * repere(cote ? pv->arriere : pv->avant);
        r.draw(*g.actuels[cote][niveau], g.matSupport, M);
        r.draw(g.plaque[niveau], cote ? g.matPlaqueArriere : g.matPlaque, M);
        if (niveau == 0) r.draw(g.vis, g.matVis, M);
    }
}

void liberer(Renderer& r) {
    if (!g.pret) return;
    libererTexte(r);
    for (auto& [k, m] : g.supports) r.destroy(m);
    g.supports.clear();
    g.courante = nullptr;
    for (auto& c : g.actuels) for (auto& m : c) m = nullptr;
    for (Mesh& m : g.plaque) r.destroy(m);
    r.destroy(g.vis);
    r.destroy(g.matSupport); r.destroy(g.matVis);
    g.texte.clear();
    g.pret = false;
}

void texteBanc(const char* texte) { g.imposeBanc = texte; }
void masquerBanc(bool masque) { g.masqueBanc = masque; }
const std::string& texteAffiche() { return g.texte; }

} // namespace wfs::plaque_joueur
