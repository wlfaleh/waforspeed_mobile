// WaForSpeed — WFS-561 : scène 3D des résultats (voir resultats_parking.h).
//
// Le vrai objet (RG-02), d'après les fiches des panneaux d'information à LED d'extérieur et de parc de stationnement (pas de
// 16 mm, IP65) :
//   - caisson en profilés d'aluminium extrudé thermolaqué gris anthracite (RAL 7016 ≈ #383E42, poudre polyester satinée),
//     profondeur 120 à 150 mm, cadre avant de 50 à 60 mm qui tient le masque, coins à chanfrein ;
//   - masque avant en polycarbonate noir mat, une visière pare-soleil de 6 à 8 mm au-dessus de chaque rangée de points (elle
//     coupe la lumière du plafond et donne le contraste du panneau éteint) ;
//   - points : une LED ambre AlInGaP de 5 mm par point (590 nm dominant ≈ sRGB #FFA000), allumée seulement là où le texte passe ;
//     un point éteint reste noir derrière la visière (aucune LED visible, RG-02) ;
//   - fixation : deux pattes en acier galvanisé chevillées dans le voile, 60 mm entre le dos du caisson et le mur.
// Reflet (RG-05) : sol époxy verni (rugosité 0,26) mouillé : l'image miroir de chaque point est posée sur le sol là où l'œil
// la voit (miroir plan, réflexion de Fresnel ≈ 0,1 à incidence rasante sur une surface d'eau mince), étirée vers l'œil comme
// sur une surface mouillée légèrement rugueuse (traînées verticales de la maquette 11).
#include "resultats_parking.h"
#include <algorithm>
#include <cmath>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/cars.h"
#include "../game/game.h"
#include "../game/track.h"
#include "../game/player.h"
#include "../render/renderer.h"
#include "../world/mobilier_ville.h"
#include "garage_commun.h"
#include "langue.h"
#include "menu_parking.h"
#include "text.h"

namespace wfs::resultats3d {
namespace {

using mobilier::Repere;
using mobilier::Sortie;
using ecrans::garage::lin;

struct Geo { std::vector<Vertex> v; std::vector<uint32_t> i; };

static_assert(Z_FOND == ecrans::parking::PLACE_P / 2 + 0.25f, "voile du fond du parking de WFS-552 (menu_parking.cpp : Z_FOND)");
constexpr float CADRE = 0.055f;                           // largeur du cadre avant
constexpr float SAILLIE_CADRE = 0.012f;                   // le cadre dépasse du masque
constexpr float VISIERE = 0.007f;                         // profondeur des visières
constexpr float RAYON_POINT = 0.0048f;                    // LED de 5 mm (lentille diffusante)
constexpr int POINTS_MAX = 5200;                          // points allumés au plus (31 × 6 caractères, ≈ 28 points chacun)
const Repere MONDE{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

// ambre 590 nm (sRGB #FFA000 en linéaire) : couleur des points et de leur lumière
const vec3 AMBRE{1.0f, 0.352f, 0.0f};

void pave(Geo& g, vec3 c, vec3 h, float ch) { mobilier::pave(Sortie(g), MONDE, c, h, ch, {0, 0}); }

// ---- état partagé avec les modes (thread principal seulement)
Tableau g_tableau;          // dernier tableau demandé
bool g_change = false;      // le contenu a changé depuis les derniers points écrits
// demande « vivante » : un mode la renouvelle à chaque pas (1/120 s) ; elle tient 4 pas (une image à 30 i/s), puis s'éteint
long long g_pas = 0, g_pasDemande = -1000;
bool g_actifPas = false;    // demandé dans les 4 derniers pas (lu par scene())
bool g_affiche = false;     // la scène a pris l'image en cours
bool g_prete = false;
std::string g_vide;

bool memes(const Tableau& a, const Tableau& b) {
    if (a.titre != b.titre || a.lignes.size() != b.lignes.size()) return false;
    for (size_t k = 0; k < a.lignes.size(); k++) {
        const led::Ligne &x = a.lignes[k], &y = b.lignes[k];
        if (x.pos != y.pos || x.pilote != y.pilote || x.temps != y.temps || x.ecart != y.ecart || x.moi != y.moi) return false;
    }
    return true;
}

class Scene : public System {
public:
    const char* name() const override { return "resultats3d"; }
    // RG-C5 : parking, caisson et tampons des points préparés au chargement de la partie ; mémoire refusée → repli 2D des modes
    void init(Renderer& r, const GameView&) override {
        if (g_prete || std::getenv("WFS_RESULTATS_SANS_3D")) return;
        if (!parking_.pret() && !parking_.init(r)) { WFS_ERR("résultats (WFS-561) : parking refusé : fenêtres 2D des modes (repli)"); return; }
        if (!construire(r)) { liberer(r); parking_.shutdown(r); WFS_ERR("résultats (WFS-561) : mémoire graphique refusée : fenêtres 2D des modes (repli)"); return; }
        g_prete = true;
        WFS_LOG("résultats (WFS-561) : scène préparée : parking %zu triangles, tableau %zu triangles, %d points au plus", parking_.triangles(), trianglesTableau_, POINTS_MAX);
    }
    void shutdown(Renderer& r) override { r.vk().waitIdle(); liberer(r); parking_.shutdown(r); g_prete = false; g_affiche = false; }
    void onEcran(Ecran, Ecran, const GameView&) override { effacer(); }
    void update(double, const GameView&) override { g_pas++; g_actifPas = g_pas - g_pasDemande <= 4; }
    bool scene(FrameParams& fp, SceneDediee& s, const GameView& g) override {
        g_affiche = false;
        if (!g_prete || !g_actifPas || g.ecran != Ecran::Course || !g.car || !g.player || !g.track) return false;
        voiture_ = matriceVoiture(g);
        parking_.regler(fp, s, g, *g.car, voiture_);   // lumière du parking (réglettes, ambiance), comme le menu
        const float aspect = g.uiWidth / std::max(1.f, g.uiHeight);
        const Camera c = camera(aspect);
        const vec3 oeil = voiture_.transformPoint(c.oeil), cible = voiture_.transformPoint(c.cible);
        fp.view = lookAt(oeil, cible, normalize(voiture_.transformDir({0, 1, 0})));
        fp.proj = translate({0, -c.decalageY, 0}) * perspectiveVk(c.fovY, aspect, 0.1f);
        fp.camPos = oeil;
        s.cible = cible;
        // maquette 11 : parking plus sombre que le menu (seules les réglettes de l'allée, le panneau allumé fait la lumière
        // chaude) : exposition baissée d'un tiers de diaphragme × 2 (0,63) ; même lumière de jour et de nuit (souterrain)
        fp.exposure *= 0.63f;
        // lumière ambre du panneau sur le sol : portée par le reflet des points (ecrirePoints) ; pas de projecteur ponctuel, dont le
        // reflet sur le sol verni faisait une traînée de lampe sous le panneau (un panneau LED est une source étendue)
        oeilParking_ = c.oeil;
        s.interface = true;   // l'interface des modes (score, boutons) reste dessinée par-dessus
        g_affiche = true;
        return true;
    }
    void render(Renderer& r, const GameView& g) override {
        if (!g_affiche || !g.sceneDediee) return;
        // RG-C5 : points réécrits seulement quand le texte change, dans le tampon que l'image d'avant (peut-être encore en vol)
        // n'utilise pas ; render() est appelé après l'attente de la barrière de l'image d'il y a deux images (beginFrame)
        if (g_change || !ecrit_) { ecrirePoints(r); g_change = false; ecrit_ = true; }
        parking_.dessiner(r, voiture_);
        for (int k = 0; k < NB_MATS; k++) if (mesh_[k].indexCount) r.draw(mesh_[k], mat_[k], voiture_);
        if (points_[courant_].indexCount) r.draw(points_[courant_], matPoint_, voiture_);
        if (reflets_[courant_].indexCount) r.draw(reflets_[courant_], matReflet_, voiture_);
    }
    void drawUi(Renderer& r, const GameView& g) override {
        if (!g_affiche || g_tableau.titre.empty()) return;
        // maquette 11 : « 1er · 2:05,4 » en haut à gauche, chiffres gras en italique (unités de la maquette 1600 × 720, k = H / 720)
        const float k = g.uiHeight / 720.f, ox = (g.uiWidth - 1600 * k) / 2;
        ui::text(r, std::max(g.marges.gauche + 16 * k, ox + 42 * k), std::max(g.marges.haut + 8 * k, 26 * k), 44 * k, g_tableau.titre, ui::BLANC, ui::GAUCHE, 0.f, false, true);
    }

    size_t trianglesTableau() const { return trianglesTableau_; }

private:
    enum Mat : int { CAISSON, MASQUE, VISIERES, PATTES, NB_MATS };

    static mat4 matriceVoiture(const GameView& g) {   // même matrice que le menu (menu.cpp : matriceVoiture)
        const TrackPoint p = g.track->pointLisse(g.player->dist, g.player->playerX * RW);
        return translate({(float)p.x, (float)p.y, (float)p.z}) * rotateY((float)-(p.th + g.player->heading + g.player->spinYaw)) * rotateX((float)p.slope);
    }

    bool construire(Renderer& r) {
        Geo g[NB_MATS];
        const float cx = CENTRE_TABLEAU.x, cy = CENTRE_TABLEAU.y, L = CAISSON_L / 2, H = CAISSON_H / 2;
        // ---- caisson : corps en profilés (chanfrein de 6 mm), cadre avant qui dépasse du masque
        pave(g[CAISSON], {cx, cy, (Z_DOS + Z_AVANT + 0.004f) / 2}, {L - 0.004f, H - 0.004f, (Z_DOS - Z_AVANT - 0.004f) / 2}, 0.006f);
        const float zc = Z_AVANT - SAILLIE_CADRE / 2;
        pave(g[CAISSON], {cx, cy + H - CADRE / 2, zc}, {L, CADRE / 2, SAILLIE_CADRE / 2 + 0.002f}, 0.004f);   // traverse haute
        pave(g[CAISSON], {cx, cy - H + CADRE / 2, zc}, {L, CADRE / 2, SAILLIE_CADRE / 2 + 0.002f}, 0.004f);   // traverse basse
        pave(g[CAISSON], {cx - L + CADRE / 2, cy, zc}, {CADRE / 2, H - CADRE, SAILLIE_CADRE / 2 + 0.002f}, 0.004f);   // montants
        pave(g[CAISSON], {cx + L - CADRE / 2, cy, zc}, {CADRE / 2, H - CADRE, SAILLIE_CADRE / 2 + 0.002f}, 0.004f);
        // vis de fixation du cadre (tête bombée de 8 mm, 6 par traverse, 3 par montant), en inox
        const float zv = Z_AVANT - SAILLIE_CADRE - 0.0015f;
        for (int k = 0; k < 6; k++) {
            const float x = cx - L + 0.12f + (CAISSON_L - 0.24f) * (float)k / 5.f;
            for (float y : {cy + H - CADRE / 2, cy - H + CADRE / 2}) pave(g[PATTES], {x, y, zv}, {0.004f, 0.004f, 0.0015f}, 0.0014f);
        }
        for (int k = 0; k < 3; k++)
            for (float x : {cx - L + CADRE / 2, cx + L - CADRE / 2}) pave(g[PATTES], {x, cy - H + 0.25f + (CAISSON_H - 0.5f) * (float)k / 2.f, zv}, {0.004f, 0.004f, 0.0015f}, 0.0014f);
        // ---- masque noir : plaque de 3 mm entre les montants, visière au-dessus de chaque rangée de points
        pave(g[MASQUE], {cx, cy, Z_MASQUE + 0.0015f}, {L - CADRE + 0.004f, H - CADRE + 0.004f, 0.0015f}, 0.0005f);
        const float largeurActive = led::COLONNES * led::PAS;
        for (int rr = 0; rr < led::RANGEES; rr++) {
            const vec3 p = positionPoint(0, rr);
            pave(g[VISIERES], {cx, p.y + led::PAS * 0.42f, Z_MASQUE - VISIERE / 2}, {largeurActive / 2 + 0.004f, 0.0007f, VISIERE / 2}, 0.0003f);
        }
        // ---- pattes de fixation en acier galvanisé (cornières 60 × 40 × 4 mm, vues au-dessus et au-dessous du caisson)
        for (float x : {cx - L * 0.62f, cx + L * 0.62f})
            for (float s : {1.f, -1.f}) {
                const float y = cy + s * (H + 0.02f);
                pave(g[PATTES], {x, y, (Z_DOS + Z_FOND) / 2 + 0.01f}, {0.03f, 0.002f, (Z_FOND - Z_DOS) / 2 + 0.01f}, 0.0008f);   // aile contre le caisson
                pave(g[PATTES], {x, y + s * 0.018f, Z_FOND - 0.002f}, {0.03f, 0.02f, 0.002f}, 0.0008f);                      // aile chevillée au voile
                pave(g[PATTES], {x - 0.012f, y + s * 0.018f, Z_FOND - 0.006f}, {0.006f, 0.006f, 0.003f}, 0.002f);           // écrou de la cheville
                pave(g[PATTES], {x + 0.012f, y + s * 0.018f, Z_FOND - 0.006f}, {0.006f, 0.006f, 0.003f}, 0.002f);
            }
        bool ok = true;
        trianglesTableau_ = 0;
        for (int k = 0; k < NB_MATS; k++) {
            mesh_[k] = r.essayerMesh(g[k].v, g[k].i);
            if (mesh_[k].vb.buf == VK_NULL_HANDLE) ok = false;
            trianglesTableau_ += g[k].i.size() / 3;
        }
        // ---- tampons des points allumés et de leur reflet : taille maximale réservée d'avance (rien n'est créé pendant l'écran)
        std::vector<Vertex> vp((size_t)POINTS_MAX * 7), vr((size_t)POINTS_MAX * 4);
        std::vector<uint32_t> ip((size_t)POINTS_MAX * 18), ir((size_t)POINTS_MAX * 6);
        for (int b = 0; b < 2 && ok; b++) {
            points_[b] = r.essayerMesh(vp, ip, true);
            reflets_[b] = r.essayerMesh(vr, ir, true);
            if (points_[b].vb.buf == VK_NULL_HANDLE || reflets_[b].vb.buf == VK_NULL_HANDLE) ok = false;
            points_[b].indexCount = reflets_[b].indexCount = 0;
        }
        if (!ok) return false;
        sommetsP_.reserve(vp.size()); indicesP_.reserve(ip.size()); sommetsR_.reserve(vr.size()); indicesR_.reserve(ir.size());
        allumes_.reserve(POINTS_MAX);
        // ---- matériaux (albédo linéaire, DA-realisme § 3)
        auto mat = [&](int k, vec3 c, float rough, float metal) {
            Material m; m.color = {c.x, c.y, c.z, 1}; m.rough = rough; m.metal = metal;
            mat_[k] = r.makeMaterial(m, nullptr, nullptr, nullptr);
        };
        mat(CAISSON, lin(0x383E42), 0.45f, 0);   // aluminium thermolaqué RAL 7016, poudre satinée (diélectrique)
        mat(MASQUE, lin(0x0C0C0D), 0.70f, 0);    // polycarbonate noir mat
        mat(VISIERES, lin(0x101012), 0.60f, 0);  // visières noires injectées
        mat(PATTES, {0.56f, 0.57f, 0.58f}, 0.45f, 1);   // acier galvanisé, inox des vis
        Material mp; mp.color = {0.20f, 0.09f, 0.02f, 1}; mp.rough = 0.35f; mp.emissive = AMBRE * 7.f;   // LED allumée (lentille ambre)
        matPoint_ = r.makeMaterial(mp, nullptr, nullptr, nullptr);
        Material mr; mr.flags = MAT_ADD | MAT_BLEND; mr.color = {1, 1, 1, 1};   // reflet : lumière ajoutée, couleur dans la normale
        matReflet_ = r.makeMaterial(mr, nullptr, nullptr, nullptr);
        matReflet_.flags |= MAT_ADD | MAT_BLEND;
        return true;
    }

    void liberer(Renderer& r) {
        for (Mesh& m : mesh_) if (m.vb.buf != VK_NULL_HANDLE || m.ib.buf != VK_NULL_HANDLE) r.destroy(m);
        for (Mesh* t : {points_, reflets_}) for (int b = 0; b < 2; b++) if (t[b].vb.buf != VK_NULL_HANDLE || t[b].ib.buf != VK_NULL_HANDLE) r.destroy(t[b]);
        for (Material& m : mat_) if (m.set != VK_NULL_HANDLE) r.destroy(m);
        if (matPoint_.set != VK_NULL_HANDLE) r.destroy(matPoint_);
        if (matReflet_.set != VK_NULL_HANDLE) r.destroy(matReflet_);
        for (Mesh& m : mesh_) m = Mesh{};
        for (int b = 0; b < 2; b++) { points_[b] = Mesh{}; reflets_[b] = Mesh{}; }
        for (Material& m : mat_) m = Material{};
        matPoint_ = matReflet_ = Material{};
        ecrit_ = false;
    }

    // points allumés du tableau → hexagones émissifs devant le masque, et leur reflet sur le sol
    void ecrirePoints(Renderer& r) {
        const std::string entete[4] = {ui::tr("resultats.led.pos"), ui::tr("resultats.led.pilote"), ui::tr("resultats.led.temps"), ui::tr("resultats.led.ecart")};
        led::composer(entete, g_tableau.lignes, allumes_);
        if (allumes_.size() > (size_t)POINTS_MAX) allumes_.resize(POINTS_MAX);
        sommetsP_.clear(); indicesP_.clear(); sommetsR_.clear(); indicesR_.clear();
        const vec3 E = oeilParking_;
        for (const led::Point& p : allumes_) {
            const vec3 c = positionPoint(p.c, p.r);
            // LED : hexagone face à l'allée (normale −z), centre un peu bombé vers l'avant (lentille)
            const uint32_t k0 = (uint32_t)sommetsP_.size();
            sommetsP_.push_back({c.x, c.y, c.z - 0.0012f, 0, 0, -1, 0.5f, 0.5f});
            for (int a = 0; a < 6; a++) {
                const float t = PI / 3 * (float)a;
                sommetsP_.push_back({c.x + RAYON_POINT * std::cos(t), c.y + RAYON_POINT * std::sin(t), c.z, 0, 0, -1, 0.5f, 0.5f});
            }
            for (uint32_t a = 0; a < 6; a++) indicesP_.insert(indicesP_.end(), {k0, k0 + 1 + (a + 1) % 6, k0 + 1 + a});
            // reflet : image miroir du point sur le sol mouillé, vue de l'œil ; traînée vers l'œil, plus faible loin du mur
            const vec3 q = refletSol(E, c);
            vec3 vers{E.x - q.x, 0, E.z - q.z};
            const float dh = std::sqrt(vers.x * vers.x + vers.z * vers.z);
            if (dh < 0.1f) continue;
            vers = vers * (1.f / dh);
            const vec3 lat{-vers.z, 0, vers.x};
            const float allonge = RAYON_POINT * 2.6f * dh / std::max(0.3f, E.y);   // plan presque rasant : la tache s'étire vers l'œil
            const float large = RAYON_POINT * 2.2f;
            const float loin = std::max(0.f, Z_FOND - q.z);
            const vec3 col = AMBRE * (0.55f * std::exp(-loin / 3.2f));
            const vec3 a0 = q - vers * allonge - lat * large, a1 = q - vers * allonge + lat * large, a2 = q + vers * allonge + lat * large, a3 = q + vers * allonge - lat * large;
            const uint32_t j = (uint32_t)sommetsR_.size();
            const float y = 0.003f;   // juste au-dessus du vernis (aucun scintillement de profondeur)
            sommetsR_.push_back({a0.x, y, a0.z, col.x, col.y, col.z, 0, 0});
            sommetsR_.push_back({a1.x, y, a1.z, col.x, col.y, col.z, 1, 0});
            sommetsR_.push_back({a2.x, y, a2.z, col.x, col.y, col.z, 1, 1});
            sommetsR_.push_back({a3.x, y, a3.z, col.x, col.y, col.z, 0, 1});
            indicesR_.insert(indicesR_.end(), {j, j + 2, j + 1, j, j + 3, j + 2});
        }
        courant_ = 1 - courant_;   // jamais le tampon de l'image d'avant
        if (sommetsP_.empty()) { points_[courant_].indexCount = 0; reflets_[courant_].indexCount = 0; return; }
        r.updateMesh(points_[courant_], sommetsP_, indicesP_);
        if (!sommetsR_.empty()) r.updateMesh(reflets_[courant_], sommetsR_, indicesR_); else reflets_[courant_].indexCount = 0;
    }

    ecrans::parking::Parking parking_;
    Mesh mesh_[NB_MATS]{};
    Material mat_[NB_MATS]{};
    Mesh points_[2]{}, reflets_[2]{};
    Material matPoint_{}, matReflet_{};
    int courant_ = 0;
    bool ecrit_ = false;
    size_t trianglesTableau_ = 0;
    mat4 voiture_ = mat4::identity();
    vec3 oeilParking_{};
    std::vector<led::Point> allumes_;
    std::vector<Vertex> sommetsP_, sommetsR_;
    std::vector<uint32_t> indicesP_, indicesR_;
};

} // namespace

void demander(const Tableau& t) {
    g_pasDemande = g_pas; g_actifPas = true;
    if (memes(t, g_tableau)) return;
    g_tableau = t; g_change = true;
}
void effacer() { g_pasDemande = -1000; g_actifPas = false; g_tableau = Tableau{}; g_change = true; }
bool affiche() { return g_affiche; }
bool prete() { return g_prete; }
const std::string& titre() { return g_affiche ? g_tableau.titre : g_vide; }

WFS_SYSTEM(Scene, 408)   // interface (400-499) : après le score (405) dont elle laisse le panneau, avant le HUD (420)

} // namespace wfs::resultats3d
