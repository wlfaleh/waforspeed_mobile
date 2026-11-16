// WaForSpeed — WFS-558 : scène 3D du duel en ligne (voir salon_parking.h).
//
// Mesures du réel (sources) :
//   - digicode d'interphone mural antivandale en inox (fiches de claviers à codes pour contrôle d'accès, modèles muraux en
//     saillie) : façade 120 × 180 mm, épaisseur 25 à 35 mm, inox 304 brossé de 2 à 3 mm, touches inox de 15 à 19 mm en saillie
//     de 2 à 4 mm, pas de 19 mm, légendes gravées, vis inviolables à tête bombée (Torx à téton) aux 4 coins, afficheur à cristaux
//     liquides derrière une vitre, posé à 1,40-1,70 m du sol (hauteur d'usage d'un interphone, NF P 99-611 et guides PMR) ;
//   - gaine électrique en saillie : tube IRL Ø 20 mm gris RAL 7035, colliers tous les 0,60 m au plus (NF C 15-100) ;
//   - plaque de place réservée : aluminium de 1,5 mm, 300 × 100 mm, bord embouti, film rétroréfléchissant blanc, fixée par
//     4 rivets (Ø 4 mm) ;
//   - parking : celui du menu (menu_parking.cpp, NF P 91-100) ; places de 2,50 m, poteau carré de 50 cm.
// Albédos et rugosités : regles-de-gestion/DA-realisme.md § 3 (inox brossé, plastique, PVC).
#include "salon_parking.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <map>
#include <vector>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/cars.h"
#include "../game/duel.h"
#include "../game/duel_system.h"
#include "../game/player.h"
#include "../game/track.h"
#include "../game/sauvegarde.h"
#include "../render/model_loader.h"
#include "../render/vehicule_plaques_autres.h"
#include "../render/renderer.h"
#include "../world/mobilier_ville.h"
#include "garage_commun.h"
#include "menu_parking.h"
#include "text.h"
#include "../game/commandes_tactiles.h"   // dessin2d : icône ⌫ gravée

namespace wfs::ecrans::duel3d {


Vue& vue() { static Vue v; return v; }
void rapprocher(bool proche) { vue().proche = proche; WFS_LOG("salon (WFS-558) : vue %s du digicode", proche ? "rapprochée" : "large"); }

// ---------------------------------------------------------------- décor (digicode, plaque, gaine, 2e voiture)
namespace {

using mobilier::Repere;
using mobilier::Sortie;
using garage::lin;

struct Geo { std::vector<Vertex> v; std::vector<uint32_t> i; };
constexpr vec2 UV0{0, 0};

enum Mat : int { INOX, TOUCHE, CADRE, LCD, VIS, PVC, PLAQUE, BANDE, BORD, NB_MATS };

// repère du digicode dans le repère de la voiture : origine au centre de sa base contre le poteau, x vers la droite vu de face
// (−x de la voiture), y vers le haut, z sorti du poteau (−z de la voiture)
Repere repereDigicode() { return {{POTEAU_X, DIGI_Y, POTEAU_FACE}, {-1, 0, 0}, {0, 1, 0}, {0, 0, -1}}; }
// même origine, axe y du tour (mobilier::tour) le long de la sortie du poteau : têtes de vis et rivets
Repere repereFace(const Repere& R) { return {R.o, R.x, R.z, R.y * -1.f}; }

void pave(Geo& g, const Repere& R, vec3 c, vec3 h, float ch) { mobilier::pave(Sortie(g), R, c, h, ch, UV0); }

// tête de vis inviolable Ø 7 mm (inox) : tête bombée basse ISO 7380 (hauteur 0,2 × Ø), empreinte creusée (Ø 0,55 × la tête,
// 6 lobes de la Torx à téton) et téton central ; rivet Ø 4 mm (sans empreinte), posés sur la surface z
void tete(Geo& g, const Repere& R, float x, float y, float z, float rayon, bool empreinte = true) {
    const Repere F = repereFace(R);
    const float h = rayon * 0.40f;
    if (!empreinte) { mobilier::tour(Sortie(g), F, {x, z, -y}, {{rayon, 0.f}, {rayon, h * 0.2f}, {rayon * 0.75f, h * 0.8f}, {0.f, h}}, 14, UV0); return; }
    const float re = rayon * 0.55f, fond = h * 0.25f, teton = rayon * 0.16f;
    // tête : flanc, dôme, puis bord plat de l'empreinte (anneau jusqu'à 0,80 × son rayon)
    mobilier::tour(Sortie(g), F, {x, z, -y}, {{rayon, 0.f}, {rayon, h * 0.20f}, {rayon * 0.82f, h * 0.75f}, {re + rayon * 0.06f, h},
                                               {re * 0.80f, h * 0.97f}}, 24, UV0);
    // empreinte à 6 lobes (rayon 0,78 à 0,98 × re) : paroi qui descend (normales vers l'intérieur), fond, téton central
    mobilier::tour(Sortie(g), F, {x, z, -y}, {{re * 0.78f, h * 0.97f}, {re * 0.78f, fond}, {teton, fond}, {teton, h * 0.80f}, {0.f, h * 0.84f}}, 36, UV0, 6, 0.25f);
}

class Decor {
public:
    bool init(Renderer& r) {
        Geo g[NB_MATS];
        const Repere R = repereDigicode();
        // ---- boîtier : inox brossé, arêtes chanfreinées de 3 mm (tôle pliée), plaque de fond un peu plus grande (joint mural)
        pave(g[CADRE], R, {0, 0, 0.002f}, {FACE_L / 2 + 0.003f, FACE_H / 2 + 0.003f, 0.002f}, 0.0015f);   // joint noir contre le poteau
        pave(g[INOX], R, {0, 0, 0.004f + (EPAIS - 0.004f) / 2}, {FACE_L / 2, FACE_H / 2, (EPAIS - 0.004f) / 2}, 0.003f);
        // ---- afficheur : cadre en plastique noir affleurant (1 mm), cristaux liquides 2 mm en retrait derrière la vitre
        pave(g[CADRE], R, {0, Y_AFF, EPAIS + 0.0005f}, {AFF_L / 2, AFF_H / 2, 0.0005f}, 0.0008f);
        pave(g[LCD], R, {0, Y_AFF, EPAIS + 0.0011f}, {AFF_L / 2 - 0.004f, AFF_H / 2 - 0.004f, 0.0002f}, 0.0002f);
        // ---- touches : inox poli, chanfrein de 1,2 mm, saillie de 4 mm ; collerette noire d'étanchéité (1 mm) autour de chacune
        for (int k = 0; k < NB_TOUCHES; k++) {
            vec3 c; vec2 d; touche(k, c, d);
            pave(g[CADRE], R, {c.x, c.y, EPAIS + 0.0004f}, {d.x + 0.0012f, d.y + 0.0012f, 0.0004f}, 0.0004f);
            pave(g[TOUCHE], R, {c.x, c.y, EPAIS + 0.002f}, {d.x, d.y, 0.002f}, 0.0012f);
        }
        // ---- vis inviolables aux 4 coins (Ø 7 mm)
        for (float sx : {-1.f, 1.f}) for (float sy : {-1.f, 1.f}) tete(g[VIS], R, sx * (FACE_L / 2 - 0.009f), sy * (FACE_H / 2 - 0.009f), EPAIS, 0.0035f);
        // ---- gaine IRL Ø 20 mm : sort du flanc droit du boîtier, monte le long du poteau jusqu'à la poutre (2,20 m) ; colliers
        // à 25 mm du bord de la plaque (x = 0,150 m) ; sortie horizontale par un presse-étoupe, coude cintré de 60 mm de rayon
        const float xg = 0.185f, zg = 0.011f, haut = 2.20f - DIGI_Y, yg = 0.040f, RC = 0.060f, xp = FACE_L / 2 + 0.016f;
        {   // presse-étoupe PG 16 en polyamide gris : écrou six pans (24 mm sur plats) puis chapeau de serrage, axe le long de x
            const Repere P{R.p({FACE_L / 2, yg, zg}), R.y, R.x, R.z * -1.f};
            mobilier::tour(Sortie(g[PVC]), P, {0, 0, 0}, {{0.0139f, 0.f}, {0.0139f, 0.006f}, {0.0125f, 0.0068f}, {0.0115f, 0.0068f}, {0.0115f, 0.0075f},
                                                          {0.0118f, 0.0080f}, {0.0118f, 0.0145f}, {0.0108f, 0.0158f}, {0.0100f, 0.016f}}, 6, UV0);
        }
        std::vector<vec3> trace{{xp - 0.001f, yg, zg}, {xg - RC, yg, zg}};
        for (int k = 1; k <= 10; k++) {   // quart de cercle : de l'horizontale à la verticale
            const float a = (PI / 2) * (float)k / 10.f;
            trace.push_back({xg - RC + RC * std::sin(a), yg + RC - RC * std::cos(a), zg});
        }
        trace.push_back({xg, haut, zg});
        mobilier::tube(Sortie(g[PVC]), R, trace, 0.010f, 12, UV0, true);
        for (float y = 0.16f; y < haut - 0.05f; y += 0.20f) {
            pave(g[PVC], R, {xg, y, zg}, {0.0125f, 0.006f, 0.0115f}, 0.002f);                 // collier à embase
            tete(g[VIS], R, xg + 0.017f, y, 0.0f, 0.0025f);                                  // vis de l'embase
        }
        // ---- plaque du pseudo : aluminium 300 × 100 × 1,5 mm sur entretoises de 8 mm, bord embouti peint, bande orange « WFS »,
        // film blanc, 4 rivets
        const float py = PLAQUE_DY, pz = 0.008f, PL = 0.150f, PH = 0.050f;
        pave(g[BORD], R, {0, py, pz + 0.00075f}, {PL, PH, 0.00075f}, 0.0007f);
        pave(g[PLAQUE], R, {0, py, pz + 0.0016f}, {PL - 0.006f, PH - 0.006f, 0.0001f}, 0.0001f);
        pave(g[BANDE], R, {-PL + 0.006f + 0.016f, py, pz + 0.0018f}, {0.016f, PH - 0.006f, 0.0001f}, 0.0001f);
        for (float sx : {-1.f, 1.f}) for (float sy : {-1.f, 1.f}) tete(g[VIS], R, sx * (PL - 0.012f), py + sy * (PH - 0.012f), pz + 0.0019f, 0.002f, false);
        for (float sx : {-1.f, 1.f}) pave(g[PVC], R, {sx * (PL - 0.012f), py, pz / 2}, {0.004f, 0.004f, pz / 2}, 0.001f);   // entretoises
        // ---- inox brossé : uv planes en mètres dans le repère du digicode (façade, touches), × 20 : une texture pour 5 cm ; le
        // brossage suit x (stries horizontales, comme une tôle brossée en long)
        for (int k : {INOX, TOUCHE})
            for (Vertex& v : g[k].v) {
                const vec3 d{v.px - R.o.x, v.py - R.o.y, v.pz - R.o.z};
                v.u = dot(d, R.x) * 20.f; v.v = dot(d, R.y) * 20.f;
            }
        // ---- envoi : un maillage par matériau ; mémoire refusée → rien n'est gardé (repli : écran 2D d'avant)
        bool ok = true; triangles_ = 0;
        for (int k = 0; k < NB_MATS; k++) {
            if (g[k].i.empty()) continue;
            mesh_[k] = r.essayerMesh(g[k].v, g[k].i);
            if (mesh_[k].vb.buf == VK_NULL_HANDLE || mesh_[k].ib.buf == VK_NULL_HANDLE) ok = false;
            triangles_ += g[k].i.size() / 3;
        }
        if (!ok) { WFS_ERR("salon (WFS-558) : mémoire graphique refusée pour le digicode : écran 2D gardé (repli)"); liberer(r); return false; }
        brossage(r);
        auto mat = [&](int k, vec3 c, float rough, float metal, vec3 em = {0, 0, 0}, bool brosse = false) {
            Material m; m.color = {c.x, c.y, c.z, 1}; m.rough = rough; m.metal = metal; m.emissive = em;
            mat_[k] = brosse && grain_.view ? r.makeMaterial(m, &grain_, nullptr, &grainMR_) : r.makeMaterial(m, nullptr, nullptr, nullptr);
        };
        mat(INOX, {0.62f, 0.62f, 0.63f}, 0.44f, 1, {}, true);   // inox 304 brossé (F0 ≈ 0,56-0,62 ; rugosité 0,25-0,44 selon la strie)
        mat(TOUCHE, {0.65f, 0.65f, 0.66f}, 0.44f, 1, {}, true); // touches en inox brossé (même finition que la façade)
        mat(CADRE, lin(0x141518), 0.55f, 0);                    // plastique ABS noir, joint en EPDM
        mat(LCD, {0.010f, 0.030f, 0.032f}, 0.08f, 0, vec3{0.003f, 0.016f, 0.018f});   // cristaux liquides à rétroéclairage cyan (éteint : vert-noir)
        mat(VIS, {0.55f, 0.55f, 0.56f}, 0.48f, 1);              // têtes de vis et rivets inox (tête décolletée, mate)
        mat(PVC, lin(0xB4B6B2), 0.65f, 0);                      // PVC gris clair RAL 7035
        mat(PLAQUE, {0.78f, 0.78f, 0.76f}, 0.35f, 0);           // film rétroréfléchissant blanc
        mat(BANDE, lin(0xE85A12), 0.40f, 0);                    // bande orange (RAL 2004)
        mat(BORD, lin(0x1A1B1E), 0.45f, 0);                     // bord embouti peint noir
        // 2e voiture garée tant qu'aucun adversaire n'est connu (RG-03) : une voiture du jeu, sans marque, noire
        ok = loadCarModel(r, std::string(carById("m4").file), garee_);
        if (!ok) WFS_WARN("salon (WFS-558) : voiture garée non chargée : place voisine vide");
        WFS_LOG("salon (WFS-558) : digicode, plaque et gaine préparés : %zu triangles, %d appels de dessin", triangles_, appels());
        return pret_ = true;
    }
    void liberer(Renderer& r) {
        r.vk().waitIdle();
        for (Mesh& m : mesh_) { if (m.vb.buf != VK_NULL_HANDLE || m.ib.buf != VK_NULL_HANDLE) r.destroy(m); m = Mesh{}; }
        for (Material& m : mat_) { if (m.set != VK_NULL_HANDLE) r.destroy(m); m = Material{}; }
        if (grain_.view) r.vk().destroy(grain_);
        if (grainMR_.view) r.vk().destroy(grainMR_);
        grain_ = Image{}; grainMR_ = Image{};
        if (garee_.ok) r.destroy(garee_);
        garee_ = Model{};
        for (auto& [id, m] : adversaires_) r.destroy(m);
        adversaires_.clear();
        pret_ = false;
    }
    bool pret() const { return pret_; }
    int appels() const { int n = 0; for (const Mesh& m : mesh_) if (m.indexCount) n++; return n; }
    size_t triangles() const { return triangles_; }
    // modèle de la voiture d'un adversaire : chargé hors de l'image (update), gardé jusqu'à la fin de la partie
    void prevoir(Renderer& r, const std::string& car) {
        if (car.empty() || adversaires_.count(car)) return;
        Model m;
        if (!loadCarModel(r, std::string(carById(car.c_str()).file), m)) { WFS_WARN("salon (WFS-558) : voiture de l'adversaire %s non chargée", car.c_str()); return; }
        adversaires_[car] = std::move(m);
    }
    void dessiner(Renderer& r, const mat4& M, const std::string& car, const std::string& peinture) {
        for (int k = 0; k < NB_MATS; k++) if (mesh_[k].indexCount) r.draw(mesh_[k], mat_[k], M);
        // place voisine : la voiture de l'adversaire (sa peinture), sinon la voiture garée ; même sens que celle du joueur
        const auto it = adversaires_.find(car);
        const Model* m = it != adversaires_.end() && it->second.ok ? &it->second : garee_.ok ? &garee_ : nullptr;
        if (!m) return;
        const CarDef& cd = carById(m == &garee_ ? "m4" : car.c_str());
        const PeintureDef* p = m == &garee_ ? nullptr : peintureDef(peinture);
        const vec3 paint = m == &garee_ ? lin(0x0f1014) : lin(p ? p->hex : cd.paint);
        const mat4 V = M * translate({VOISINE_X, 0, 0.05f});
        r.draw(*m, V, &paint, nullptr);
        plaques_autres::dessiner(r, cd.id, V, 1.f, plaques_autres::caseDuel(car.empty() ? "parking" : car), plaques_autres::Genre::Adversaire, false, r_camera_);
    }
    vec3 r_camera_{};
private:
    // grain de l'inox brossé (256 × 256, généré au chargement, déterministe) : stries fines le long de u, chacune d'une longueur
    // et d'une profondeur au hasard ; couleur (sRGB, ± 6 %) et rugosité (canal g : 0,55 à 1 × la rugosité du matériau)
    void brossage(Renderer& r) {
        constexpr int N = 256;
        std::vector<uint8_t> c((size_t)N * N * 4), mr((size_t)N * N * 4);
        uint32_t graine = 0x5A17D16u;
        auto hasard = [&]() { graine ^= graine << 13; graine ^= graine >> 17; graine ^= graine << 5; return (float)(graine & 0xFFFF) / 65535.f; };
        std::vector<float> ligne(N), phase(N), longueur(N);
        for (int y = 0; y < N; y++) { ligne[(size_t)y] = hasard(); phase[(size_t)y] = hasard() * 6.2832f; longueur[(size_t)y] = 1.f + hasard() * 5.f; }
        for (int y = 0; y < N; y++)
            for (int x = 0; x < N; x++) {
                // strie de la rangée y, modulée le long de x (rayures qui commencent et s'arrêtent), plus un grain très fin
                const float le = 0.5f + 0.5f * std::sin(phase[(size_t)y] + 6.2832f * longueur[(size_t)y] * (float)x / N);
                const float s = std::clamp(0.55f * ligne[(size_t)y] + 0.30f * le * ligne[(size_t)y] + 0.15f * hasard(), 0.f, 1.f);
                const size_t i = ((size_t)y * N + (size_t)x) * 4;
                const uint8_t v = (uint8_t)std::lround(255.f * (0.94f + 0.06f * s));
                c[i] = c[i + 1] = c[i + 2] = v; c[i + 3] = 255;
                mr[i] = 255; mr[i + 1] = (uint8_t)std::lround(255.f * (0.55f + 0.45f * (1.f - s))); mr[i + 2] = 255; mr[i + 3] = 255;
            }
        grain_ = r.vk().uploadTexture(c.data(), N, N, true);
        grainMR_ = r.vk().uploadTexture(mr.data(), N, N, false);
    }
    Mesh mesh_[NB_MATS]{};
    Material mat_[NB_MATS]{};
    Image grain_{}, grainMR_{};
    Model garee_{};
    std::map<std::string, Model> adversaires_;
    bool pret_ = false;
    size_t triangles_ = 0;
};

Decor& decor() { static Decor d; return d; }

// projection d'un point du monde vers l'écran (pixels de l'interface) ; faux derrière la caméra
bool projeter(const mat4& VP, vec3 p, float W, float H, vec2& out) {
    const float x = VP.at(0, 0) * p.x + VP.at(0, 1) * p.y + VP.at(0, 2) * p.z + VP.at(0, 3);
    const float y = VP.at(1, 0) * p.x + VP.at(1, 1) * p.y + VP.at(1, 2) * p.z + VP.at(1, 3);
    const float w = VP.at(3, 0) * p.x + VP.at(3, 1) * p.y + VP.at(3, 2) * p.z + VP.at(3, 3);
    if (w <= 1e-4f) return false;
    out = {(x / w * 0.5f + 0.5f) * W, (y / w * 0.5f + 0.5f) * H};   // y de Vulkan vers le bas (perspectiveVk)
    return true;
}
// rectangle englobant à l'écran d'un rectangle du repère du digicode (centre c, demi-mesures d, sur le plan z)
Rect rectEcran(const mat4& VP, const mat4& M, const Repere& R, vec3 c, vec2 d, float W, float H) {
    Rect q; float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
    for (float sx : {-1.f, 1.f}) for (float sy : {-1.f, 1.f}) {
        vec2 e; if (!projeter(VP, M.transformPoint(R.p({c.x + sx * d.x, c.y + sy * d.y, c.z})), W, H, e)) return q;
        x0 = std::min(x0, e.x); y0 = std::min(y0, e.y); x1 = std::max(x1, e.x); y1 = std::max(y1, e.y);
    }
    q.x = x0; q.y = y0; q.w = x1 - x0; q.h = y1 - y0; q.ok = true;
    return q;
}

vec4 hexa(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }

// voiture et peinture du premier adversaire du salon (vides : aucun adversaire connu)
void adversaire(std::string& car, std::string& peinture) {
    car.clear(); peinture.clear();
    const duel::Session& s = duelSession();
    if (!s.inRoom()) return;
    for (const duel::PeerView& p : s.others()) { car = duel::peerCar(p.f); peinture = duel::str(p.f, "paint"); return; }
}

// repère de la voiture du joueur posée sur la piste (même calcul que le menu, menu.cpp)
mat4 matriceVoiture(const GameView& g) {
    const TrackPoint p = g.track->pointLisse(g.player->dist, g.player->playerX * RW);
    return translate({(float)p.x, (float)p.y, (float)p.z}) * rotateY((float)-(p.th + g.player->heading + g.player->spinYaw)) * rotateX((float)p.slope);
}

// ---------------------------------------------------------------- système
class Salon3d : public System {
public:
    const char* name() const override { return "salon3d"; }
    void init(Renderer& r, const GameView&) override {
        r_ = &r;
        if (std::getenv("WFS_SALON_SANS_3D")) return;   // banc : captures « avant » de l'écran 2D (et repli)
        parking::Parking& pk = parking::commun();
        if (!pk.pret() && !std::getenv("WFS_MENU_SANS_PARKING")) pk.init(r);
        if (pk.pret() && !decor().pret()) decor().init(r);
        vue().pret = pk.pret() && decor().pret();
    }
    void shutdown(Renderer& r) override {
        if (decor().pret()) decor().liberer(r);
        parking::commun().shutdown(r);   // sans effet si le menu l'a déjà libéré
        vue() = Vue{};
    }
    void onEcran(Ecran, Ecran nouveau, const GameView&) override {
        if (nouveau == Ecran::Salon) { vue().proche = false; vue().t = 0; }   // on arrive toujours sur le cadrage large
    }
    void update(double dt, const GameView& g) override {
        Vue& v = vue();
        if (g.ecran != Ecran::Salon) return;
        if (duelSession().inRoom()) v.proche = false;   // salon (maquette 23) : cadrage large derrière le verre
        v.t = avancer(v.t, v.proche, (float)dt);
        std::string car, peinture; adversaire(car, peinture);
        if (r_ && decor().pret()) decor().prevoir(*r_, car);   // hors de l'image (pas de chargement entre beginFrame et endFrame)
    }
    bool scene(FrameParams& fp, SceneDediee& s, const GameView& g) override {
        Vue& v = vue();
        v.active = false;
        if (g.ecran != Ecran::Salon || !v.pret || !g.car || !g.player || !g.track) return false;
        M_ = matriceVoiture(g);
        const float aspect = g.uiWidth / std::max(1.f, g.uiHeight);
        const Camera c = camera(v.t);
        const vec3 oeil = M_.transformPoint(c.oeil), cible = M_.transformPoint(c.cible), haut = normalize(M_.transformDir({0, 1, 0}));
        fp.view = lookAt(oeil, cible, haut);
        fp.proj = perspectiveVk(c.fovY, aspect, 0.05f);
        fp.camPos = oeil;
        parking::commun().eclairer(fp, M_);
        // lumière de la plaque et du digicode : la réglette du plafond au-dessus du poteau (maquette : plaque bien éclairée)
        if (fp.spotCount < FrameParams::SPOTS_MAX) {
            FrameParams::Spot sp;
            sp.pos = M_.transformPoint({POTEAU_X - 0.1f, 2.50f, POTEAU_FACE - 0.9f}); sp.target = M_.transformPoint({POTEAU_X, DIGI_Y + 0.15f, POTEAU_FACE});
            sp.color = {1.0f, 0.93f, 0.84f}; sp.intensity = 2.2f; sp.distance = 6; sp.angle = 0.55f; sp.penumbra = 0.8f; sp.decay = 1.2f;
            fp.spots[fp.spotCount++] = sp;
        }
        s.route = false; s.joueur = true; s.commandes = false; s.cible = cible; s.interface = true;
        // zones à l'écran (touchers de l'écran Salon à l'image suivante, textes de drawUi)
        const mat4 VP = fp.proj * fp.view;
        const Repere R = repereDigicode();
        const float W = g.uiWidth, H = g.uiHeight;
        v.digicode = rectEcran(VP, M_, R, {0, 0, EPAIS}, {FACE_L / 2, FACE_H / 2}, W, H);
        const bool proche = v.rapprochee() && !duelSession().inRoom();
        for (int k = 0; k < NB_TOUCHES; k++) {
            vec3 c3; vec2 d; touche(k, c3, d);
            // zone touchable : la case entière du pas (19 mm), pas seulement la touche (15 mm)
            touches_[k] = rectEcran(VP, M_, R, {c3.x, c3.y, c3.z}, {d.x + (PAS / 2 - DEMI_TOUCHE), d.y + (PAS / 2 - DEMI_TOUCHE)}, W, H);
            legendes_[k] = rectEcran(VP, M_, R, {c3.x, c3.y, c3.z + 0.002f}, d, W, H);
            v.touches[k] = proche ? touches_[k] : Rect{};
        }
        afficheur_ = rectEcran(VP, M_, R, {0, Y_AFF, EPAIS + 0.0011f}, {AFF_L / 2 - 0.004f, AFF_H / 2 - 0.004f}, W, H);
        plaque_ = rectEcran(VP, M_, R, {0.024f, PLAQUE_DY, 0.0098f}, {0.150f - 0.054f, 0.050f - 0.008f}, W, H);   // à 34 mm de la bande orange
        bande_ = rectEcran(VP, M_, R, {-0.150f + 0.022f, PLAQUE_DY, 0.0098f}, {0.016f, 0.044f}, W, H);
        decor().r_camera_ = oeil;
        v.active = true;
        return true;
    }
    void render(Renderer& r, const GameView& g) override {
        if (g.ecran != Ecran::Salon || !vue().active || !g.sceneDediee) return;
        parking::commun().dessiner(r, M_);
        std::string car, peinture; adversaire(car, peinture);
        decor().dessiner(r, M_, car, peinture);
    }
    // légendes gravées des touches, code tapé sur l'afficheur, pseudo sur la plaque : projetés sur leur pièce 3D (sous l'interface
    // de l'écran Salon, ordre 452 < 453)
    void drawUi(Renderer& r, const GameView& g) override {
        if (g.ecran != Ecran::Salon || !vue().active || !ui::fontReady()) return;
        for (int k = 0; k < NB_TOUCHES; k++) {
            const Rect& q = legendes_[k];
            if (!q.ok || q.h < 3.f) continue;   // plus petit que 3 px : invisible de toute façon
            const float px = q.h * 0.58f;
            const vec4 grave = hexa(0x141518, 0.92f);
            if (k == EFFACER) {   // icône « retour arrière » gravée (même dessin que la touche de l'écran 2D, WFS-146), la police n'a pas ⌫
                const float iw = q.h * 0.62f, ih = q.h * 0.44f, ix = q.x + (q.w - iw) / 2, iy = q.y + (q.h - ih) / 2, p = ih * 0.38f, ep = std::max(1.f, q.h * 0.06f);
                const vec2 penta[5] = {{ix, iy + ih / 2}, {ix + p, iy}, {ix + iw, iy}, {ix + iw, iy + ih}, {ix + p, iy + ih}};
                dessin2d::contour(r, penta, 5, ep, grave);
                const float mx = ix + p + (iw - p) / 2, my = iy + ih / 2, c = ih * 0.2f;
                const vec2 d1[2] = {{mx - c, my - c}, {mx + c, my + c}}, d2[2] = {{mx - c, my + c}, {mx + c, my - c}};
                dessin2d::trait(r, d1, 2, ep, grave); dessin2d::trait(r, d2, 2, ep, grave);
                continue;
            }
            const std::string t = k == OK ? "OK" : std::string(1, (char)std::toupper((unsigned char)caractere(k)));
            ui::text(r, q.x + q.w / 2, q.y + (q.h - px) / 2, px, t, grave, ui::CENTRE, 0, false, false);
        }
        if (afficheur_.ok && afficheur_.h >= 3.f) {   // afficheur : 4 cases, code tapé en cyan, case suivante soulignée
            const std::string& saisie = codeSaisi();
            const float px = afficheur_.h * 0.72f, cw = afficheur_.w / 4;
            for (int i = 0; i < 4; i++) {
                const float cx = afficheur_.x + cw * ((float)i + 0.5f), cy = afficheur_.y + (afficheur_.h - px) / 2;
                if (i < (int)saisie.size()) ui::text(r, cx, cy, px, std::string(1, (char)std::toupper((unsigned char)saisie[(size_t)i])), hexa(0x6FF3FF), ui::CENTRE, 0, false, false);
                else r.uiRect(cx - cw * 0.22f, afficheur_.y + afficheur_.h * 0.80f, cw * 0.44f, std::max(1.f, afficheur_.h * 0.06f), hexa(0x6FF3FF, i == (int)saisie.size() ? 0.85f : 0.25f));
            }
        }
        if (plaque_.ok && plaque_.h >= 3.f) {   // pseudo du joueur (RG-07), gravé noir sur le film blanc ; « WFS » sur la bande
            const float px = plaque_.h * 0.70f;
            ui::textBoite(r, plaque_.x + plaque_.w / 2, plaque_.y, plaque_.w, plaque_.h, px, ui::majuscules(sauvegarde().nick), hexa(0x141518), ui::CENTRE, 0.04f, false, false);
            if (bande_.ok && bande_.h >= 6.f) ui::textBoite(r, bande_.x + bande_.w / 2, bande_.y + bande_.h * 0.62f, bande_.w, bande_.h * 0.3f, bande_.h * 0.22f, "WFS", hexa(0xFFFFFF, 0.95f), ui::CENTRE, 0, false, false);
        }
    }
private:
    Renderer* r_ = nullptr;
    mat4 M_ = mat4::identity();
    Rect touches_[NB_TOUCHES], legendes_[NB_TOUCHES], afficheur_, plaque_, bande_;
};

} // namespace
} // namespace wfs::ecrans::duel3d

WFS_SYSTEM(wfs::ecrans::duel3d::Salon3d, 452)   // interface (400-499) : juste avant l'écran Salon (453), dont l'interface passe par-dessus
