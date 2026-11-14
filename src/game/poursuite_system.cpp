// WaForSpeed — mode « Poursuite police » (WFS-111) branché sur le moteur par les systèmes enfichables (WFS-30) :
// voir poursuite.h. Le départ, le décompte 3-2-1-GO et la ligne d'arrivée (10 km) sont ceux de la course
// (course_system, Course::duelFinish, comme la course à 5) ; ce module tient le score (100 → 0), fait rouler la police
// (gyrophares, sirène, renforts), densifie le trafic (accroche de traffic_system.cpp), place les embouteillages et les
// surprises, dessine le HUD de la poursuite et l'écran de fin (victoire ou « ARRÊTÉ », « Rejouer », « Menu »).
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "score_fin.h"   // WFS-537 (accroche) : bloc « Score » de l'écran de fin
#include "../audio/sfx.h"
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "../render/effets_particules.h"   // WFS-141 : effets::versHdr (gyrophares)
#include "../render/model_loader.h"
#include "../render/renderer.h"
#include "../render/vehicule_plaques_autres.h"   // WFS-484 : plaques des autres voitures
#include "../ui/appui.h"
#include "../ui/ecrans/reglages.h"
#include "../ui/ecrans/terrain.h"   // RG-12 : « Rouler »
#include "../ui/garage.h"           // RG-12 : « Choisir »   // banc : appels de dessin en qualité Basse
#include "../ui/text.h"
#include "../world/cartes_v2/carte_donnees.h"   // WFS-540 : adhérence de la chaussée (saison) pour la police
#include "../world/conducteurs.h"   // WFS-338 (RG-06) : un policier au volant de chaque voiture de police
#include "course5.h"
#include "course_system.h"
#include "duel.h"
#include "duel_system.h"
#include "game.h"
#include "player.h"
#include "poursuite.h"
#include "police_bandes.h"   // WFS-460 : bandes « POLICE » posées sur la tôle
#include "camera_vue.h"      // WFS-460 (banc) : vues Cockpit et Poursuite
#include "../render/vehicule_mesure.h"   // WFS-460 : triangles de la peinture relus dans le .wfm
#include "profil_system.h"   // WFS-480
#include "sauvegarde.h"   // WFS-119 : redémarrage simulé
#include "roues.h"   // WFS-452 : roues de la police qui tournent et braquent (glisse comprise)
#include "traffic.h"
#include "traffic_system.h"
#include "../audio/voitures_sons.h"   // WFS-272 (accroche) : déclaration des lectures pour le son des voitures
#include "textes_course.h"   // WFS-506 : poursuite en français ou en anglais

namespace wfs {

int poursuiteTrafic(int trafficN, bool enCourse) {
    return enCourse && modeCourse() == ModeCourse::Poursuite ? (int)std::lround(trafficN * poursuite::DENSITE) : 0;
}

namespace {

using namespace poursuite;

bool g_banc = false;                 // banc d'essai : programme fixe (captures stables)
int g_bancFin = 0;                   // banc : 1 = arrêter tout de suite, 2 = sauter près de l'arrivée
double g_bancSautKm = 0;
bool g_bancBandesAvant = false;      // WFS-460 (banc seulement) : bandes de main (quadrilatère plat à la boîte), pour l'avant / après
bool g_bancSans338 = false;          // WFS-338 (banc) : sans les ajouts de WFS-338 (bandes, socle, policier, chantier), pour en mesurer le coût             // WFS-141 : banc : saut en avant du joueur jusqu'à ce point (km)
// voiture existante repeinte (RG-03) : la 208 (la Golf si le joueur roule en 208)
const char* policeCar(const Player* pl) { return pl && pl->car && !std::strcmp(pl->car->id, "p208") ? "golf" : "p208"; }

const vec4 FOND{10 / 255.f, 11 / 255.f, 16 / 255.f, 0.82f}, SURFACE{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.92f};
const vec4 TEXTE{242 / 255.f, 244 / 255.f, 248 / 255.f, 1}, MUTED{138 / 255.f, 144 / 255.f, 166 / 255.f, 1};
const vec4 DANGER{1.f, 31 / 255.f, 75 / 255.f, 1}, BLEU{0.25f, 0.45f, 1.f, 1};
vec3 lin(unsigned h) {
    auto c = [&](int sh) { return std::pow(((h >> sh) & 255) / 255.f, 2.2f); };
    return {c(16), c(8), c(0)};
}
// WFS-506 (RG-01) : annonces et raisons des pertes de points. poursuite.cpp les garde en français (textes de référence de
// test_poursuite et du journal) ; l'affichage passe par les clés des tables de textes
const char* cleSurprise(Surprise s) {
    switch (s) {
        case Surprise::Barrage: return "police.barrage";
        case Surprise::Deboite: return "police.deboite";
        case Surprise::Travaux: return "police.travaux";
        case Surprise::Accident: return "police.accident";
        case Surprise::Camion: return "police.camion";
        case Surprise::Ralentisseur: return "police.ralentisseur";
        default: return "";
    }
}
constexpr const char* ANNONCE_RENFORT = "RENFORT DE POLICE", *ANNONCE_BOUCHON = "EMBOUTEILLAGE", *RAISON_BANC = "ARRÊT DU BANC";
const std::string& annonceAffichee(const std::string& fr) {
    if (fr == ANNONCE_RENFORT) return ui::tr("police.renfort");
    if (fr == ANNONCE_BOUCHON) return ui::tr("police.embouteillage");
    for (int k = 0; k < TYPES_SURPRISE; k++) if (fr == bandeau((Surprise)k)) return ui::tr(cleSurprise((Surprise)k));
    return fr;
}
const std::string& raisonAffichee(const char* r) {
    static const std::pair<const char*, const char*> T[] = {{RAISON_TRAFIC, "police.raison.trafic"}, {RAISON_GLISSIERE, "police.raison.glissiere"},
        {RAISON_POLICE, "police.raison.police"}, {RAISON_COLLE, "police.raison.colle"}, {RAISON_RALENTISSEUR, "police.raison.ralentisseur"},
        {RAISON_PLOT, "police.raison.plot"}, {RAISON_CAMION, "police.raison.camion"}, {RAISON_BANC, "police.raison.banc"}};
    static const std::string vide;
    if (!r) return vide;
    for (const auto& [fr, cle] : T) if (!std::strcmp(r, fr)) return ui::tr(cle);
    static std::string autre; autre = r; return autre;
}

struct Zone { float x = 0, y = 0, w = 0, h = 0; bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; } };
struct Plot { double s = 0, x = 0; bool tombe = false; };   // WFS-338 : un cône de chantier (K5a)
struct Panneau { double s = 0, x = 0; };                     // WFS-338 (RG-01) : panneau « travaux » AK5 sur pied
struct Camion { double s = 0; Travers tv; int fileLibre = 2; double cd = 0; double age = -1; };   // WFS-119 : camion en travers ; WFS-542 : age (s) depuis qu'il est à portée de vue (< 0 : pas encore)
struct Bande { double s = 0; bool passe = false; };                              // WFS-119 : ralentisseur

class PoursuiteSystem : public System {
public:
    static PoursuiteSystem* actif;
    void bancPolice(bool a, double ecartM, double x) { bancPoliceActive_ = a; bancPoliceEcart_ = ecartM; bancPoliceX_ = x; }   // WFS-452 (banc)
    const char* name() const override { return "poursuite"; }
    void init(Renderer& r, const GameView&) override {
        r_ = &r; actif_ = false; police_.clear(); plots_.clear(); actif = this;
        std::vector<Vertex> v; std::vector<uint32_t> idx;   // cube unité : x, z ∈ [−0,5 ; 0,5], y ∈ [0 ; 1]
        const vec3 nrm[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
        for (const vec3& f : nrm) {
            const vec3 a = std::abs(f.y) > 0.5f ? vec3{1, 0, 0} : vec3{0, 1, 0}, b = cross(f, a);
            const uint32_t k = (uint32_t)v.size();
            for (int i = 0; i < 4; i++) {
                const float sa = (i == 1 || i == 2) ? 0.5f : -0.5f, sb = i >= 2 ? 0.5f : -0.5f;
                const vec3 q = f * 0.5f + a * sa + b * sb + vec3{0, 0.5f, 0};
                v.push_back({q.x, q.y, q.z, f.x, f.y, f.z, sa + 0.5f, sb + 0.5f});
            }
            idx.insert(idx.end(), {k, k + 1, k + 2, k, k + 2, k + 3});
        }
        cube_ = r.makeMesh(v, idx);
        auto mat = [&](unsigned c, vec3 em, float rough) { Material m; const vec3 l = lin(c); m.color = {l.x, l.y, l.z, 1}; m.rough = rough; m.emissive = em; return r.makeMaterial(m, nullptr, nullptr, nullptr); };
        // WFS-141 (RG-01, ANO-56) : lampes allumées à leur couleur (versHdr, comme les clignotants de WFS-109), sans le × 6
        auto web = [](unsigned h) { return effets::versHdr(vec3{((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f}, 1.f); };
        matRouge_ = mat(0x200000, web(GYRO_ROUGE), 0.4f);
        matBleu_ = mat(0x000020, web(GYRO_BLEU), 0.4f);
        haloRouge_ = web(GYRO_ROUGE); haloBleu_ = web(GYRO_BLEU);
        matPlot_ = mat(0xff6a00, lin(0xff6a00) * 0.4f, 0.6f);
        matBlanc_ = mat(0xf2f4f8, {0, 0, 0}, 0.5f);
        matBleuCaisse_ = mat(0x14224a, {0, 0, 0}, 0.45f);
        matVitre_ = mat(0x15181e, {0, 0, 0}, 0.15f);
        matJaune_ = mat(0xffc400, lin(0xffc400) * 0.7f, 0.6f);   // WFS-119 : ralentisseur jaune et noir
        matNoir_ = mat(0x101010, {0, 0, 0}, 0.7f);
        initChantierPolice(r);   // WFS-338
    }
    // WFS-338 (RG-01, RG-07) : maillages du cône K5a, du panneau AK5, des bandes « POLICE » ; matériaux de la rampe lumineuse
    void initChantierPolice(Renderer& r) {
        auto mat = [&](unsigned c, vec3 em, float rough, float metal = 0) { Material m; const vec3 l = lin(c); m.color = {l.x, l.y, l.z, 1}; m.rough = rough; m.metal = metal; m.emissive = em; return r.makeMaterial(m, nullptr, nullptr, nullptr); };
        // cône K5a de 75 cm (voies rapides) : socle noir de 40 cm, corps orange (sans lumière propre : avant, bloc lumineux),
        // deux bandes blanches rétroréfléchissantes ; tronc de cône de 17 cm de rayon en bas à 4 cm en haut
        auto tronc = [](std::vector<Vertex>& v, std::vector<uint32_t>& idx, float y0, float y1, float u) {
            constexpr int N = 14; constexpr float R0 = 0.17f, R1 = 0.04f, Y0 = 0.04f, Y1 = 0.75f;
            auto rayon = [&](float y) { return R0 + (R1 - R0) * (y - Y0) / (Y1 - Y0); };
            const float pente = (R0 - R1) / (Y1 - Y0);
            const uint32_t k = (uint32_t)v.size();
            for (int i = 0; i <= N; i++) {
                const float a = 6.2831853f * i / N, c = std::cos(a), sn = std::sin(a);
                const vec3 n = normalize(vec3{c, pente, sn});
                for (float y : {y0, y1}) v.push_back({rayon(y) * c, y, rayon(y) * sn, n.x, n.y, n.z, u, 0.5f});
            }
            for (uint32_t i = 0; i < (uint32_t)N; i++) idx.insert(idx.end(), {k + 2 * i, k + 2 * i + 1, k + 2 * i + 3, k + 2 * i, k + 2 * i + 3, k + 2 * i + 2});
        };
        {   // WFS-338 (décision PO, RG-04 de WFS-119) : un cône = socle + corps + bandes dans un seul maillage, couleurs lues
            // dans une palette de 4 texels (orange, blanc, noir) : un seul dessin par cône (avant : 3)
            coneV_.clear(); coneI_.clear();
            for (auto [a, b] : {std::pair{0.04f, 0.30f}, {0.42f, 0.52f}, {0.62f, 0.75f}}) tronc(coneV_, coneI_, a, b, U_ORANGE);
            for (auto [a, b] : {std::pair{0.30f, 0.42f}, {0.52f, 0.62f}}) tronc(coneV_, coneI_, a, b, U_BLANC);
            for (size_t f = 0; f < 6; f++) {   // socle noir de 40 × 40 × 4 cm (faces du cube unité)
                const vec3 nrm[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
                const vec3 fN = nrm[f], a = std::abs(fN.y) > 0.5f ? vec3{1, 0, 0} : vec3{0, 1, 0}, b = cross(fN, a);
                const uint32_t k = (uint32_t)coneV_.size();
                for (int i = 0; i < 4; i++) {
                    const float sa = (i == 1 || i == 2) ? 0.5f : -0.5f, sb = i >= 2 ? 0.5f : -0.5f;
                    const vec3 q = fN * 0.5f + a * sa + b * sb + vec3{0, 0.5f, 0};
                    coneV_.push_back({q.x * 0.40f, q.y * 0.04f, q.z * 0.40f, fN.x, fN.y, fN.z, U_NOIR, 0.5f});
                }
                coneI_.insert(coneI_.end(), {k, k + 1, k + 2, k, k + 2, k + 3});
            }
            const uint8_t pal[16] = {0xe8, 0x50, 0x1a, 255, 0xee, 0xee, 0xee, 255, 0x10, 0x10, 0x10, 255, 0x10, 0x10, 0x10, 255};
            paletteCones_ = r.vk().uploadTexture(pal, 4, 1, true);
            coneMesh_ = r.makeMesh(coneV_, coneI_);
            Material m; m.rough = 0.5f;
            matCones_ = r.makeMaterial(m, &paletteCones_, nullptr, nullptr);
        }
        {   // triangle équilatéral d'1 m de côté, pointe en haut, face vers +z (vers le conducteur qui arrive)
            std::vector<Vertex> v = {{-0.5f, 0, 0, 0, 0, 1, 0, 0}, {0.5f, 0, 0, 0, 0, 1, 1, 0}, {0, 0.866f, 0, 0, 0, 1, 0.5f, 1}};
            triangle_ = r.makeMesh(v, {0, 1, 2});
        }
        {   // WFS-460 (RG-01, RG-02) : texture des bandes (960 × 96) : bleu de la police nationale, liseré blanc et rouge en bas,
            // « POLICE » en capitales blanches à traits pleins (avant : 320 × 32, lettres de 5 × 7 carrés, en escalier de près).
            // Le maillage des bandes est posé sur la tôle de la voiture au chargement de son modèle (poserBandes, police_bandes.h)
            constexpr int TW = 960, TH = 96;
            std::vector<uint8_t> px(TW * TH * 4);
            auto pose = [&](int x, int y, unsigned h) { if (x < 0 || y < 0 || x >= TW || y >= TH) return; uint8_t* q = &px[(size_t)(y * TW + x) * 4]; q[0] = (h >> 16) & 255; q[1] = (h >> 8) & 255; q[2] = h & 255; q[3] = 255; };
            for (int y = 0; y < TH; y++) for (int x = 0; x < TW; x++) pose(x, y, y >= TH - 9 ? 0xd2232a : y >= TH - 15 ? 0xf2f4f8 : 0x1d3a8c);
            // lettres sur une grille de 12 rangées, traits de 2 cases, angles adoucis (lettrage bâton des véhicules de police)
            struct Lettre { int largeur; const char* r[12]; };
            static const Lettre LETTRES[6] = {
                {10, {"#########.", "##########", "##......##", "##......##", "##......##", "##########", "#########.", "##........", "##........", "##........", "##........", "##........"}},
                {10, {".########.", "##########", "##......##", "##......##", "##......##", "##......##", "##......##", "##......##", "##......##", "##......##", "##########", ".########."}},
                {9,  {"##.......", "##.......", "##.......", "##.......", "##.......", "##.......", "##.......", "##.......", "##.......", "##.......", "#########", "#########"}},
                {2,  {"##", "##", "##", "##", "##", "##", "##", "##", "##", "##", "##", "##"}},
                {10, {".#########", "##########", "##........", "##........", "##........", "##........", "##........", "##........", "##........", "##........", "##########", ".#########"}},
                {9,  {"#########", "#########", "##.......", "##.......", "##.......", "########.", "########.", "##.......", "##.......", "##.......", "#########", "#########"}}};
            constexpr int E = 6, ESPACE = 3, Y0 = 6;    // case de 6 px : lettres de 72 px de haut sur 81 px de bleu (≈ 13 cm)
            int larg = 0; for (const Lettre& l : LETTRES) larg += (l.largeur + ESPACE) * E;
            int x0 = (TW - (larg - ESPACE * E)) / 2;
            for (const Lettre& l : LETTRES) {
                for (int ly = 0; ly < 12; ly++) for (int lx = 0; lx < l.largeur; lx++)
                    if (l.r[ly][lx] == '#') for (int dy = 0; dy < E; dy++) for (int dx = 0; dx < E; dx++) pose(x0 + lx * E + dx, Y0 + ly * E + dy, 0xffffff);
                x0 += (l.largeur + ESPACE) * E;
            }
            bandesTex_ = r.vk().uploadTexture(px.data(), TW, TH, true);
            Material m; m.rough = 0.35f;   // film adhésif : satiné, sous le vernis de la caisse
            matBandes_ = r.makeMaterial(m, &bandesTex_, nullptr, nullptr);
        }
        matPanneauRouge_ = mat(0xc8102e, {0, 0, 0}, 0.4f);
        matPanneauJaune_ = mat(0xf4c400, {0, 0, 0}, 0.45f);
        matSupport_ = mat(0x2a2c30, {0, 0, 0}, 0.5f, 0.6f);          // rampe lumineuse : socle gris anthracite
        matVitreBleue_ = mat(0x0a1430, {0, 0, 0}, 0.15f);              // feu bleu éteint : verre bleu sombre
    }
    void shutdown(Renderer& r) override {
        r.vk().waitIdle();
        if (modele_.ok) r.destroy(modele_);
        modele_ = Model{};
        if (camionModele_.ok) r.destroy(camionModele_);
        camionModele_ = Model{};
        r.destroy(cube_);
        for (Mesh* me : {&coneMesh_, &triangle_, &bandesMesh_, &bandesAvant_}) if (me->indexCount) r.destroy(*me);   // WFS-338 ; WFS-460
        bandesMesh_ = Mesh{}; bandesAvant_ = Mesh{};
        coneMesh_ = Mesh{};
        r.vk().destroy(bandesTex_); r.vk().destroy(paletteCones_);
        for (Material* m : {&matRouge_, &matBleu_, &matPlot_, &matBlanc_, &matBleuCaisse_, &matVitre_, &matJaune_, &matNoir_, &matCones_,
                            &matPanneauRouge_, &matPanneauJaune_, &matSupport_, &matVitreBleue_, &matBandes_}) r.destroy(*m);   // WFS-187 (accroche) : jeux de descripteurs rendus
        police_.clear(); plots_.clear(); panneaux_.clear(); actif_ = false;
        if (actif == this) actif = nullptr;
    }
    void onEcran(Ecran ancien, Ecran nouveau, const GameView& g) override {
        // « Rejouer » : Course → Menu → Course (le menu n'est jamais dessiné), comme la course à 5
        if (ancien == Ecran::Course && nouveau == Ecran::Menu && rejouer_) g.allerA(Ecran::Course);
        if (nouveau == Ecran::Menu && !rejouer_) actif_ = false;
        if (nouveau == Ecran::Course) rejouer_ = false;
    }
    void update(double, const GameView& g) override {
        if (actif_ && policeCar_ != policeCar(g.player) && modele_.ok) { r_->vk().waitIdle(); r_->destroy(modele_); modele_ = Model{}; modeleEssaye_ = false;
            if (bandesMesh_.indexCount) { r_->destroy(bandesMesh_); bandesMesh_ = Mesh{}; } }   // WFS-460 : bandes de l'ancien modèle
        if (actif_) policeCar_ = policeCar(g.player);
        if (r_ && actif_ && !modele_.ok && !modeleEssaye_) { modeleEssaye_ = true; loadCarModel(*r_, std::string(carById(policeCar_).file), modele_); WFS_LOG("poursuite : police %s, %zu pièces", policeCar_, modele_.parts.size());
            // WFS-338 (RG-07) : caisse de la police en peinture blanche unie, non métallisée (mat.peinture_voiture : métal 0,
            // F0 0,04, vernis par-dessus) : le modèle a une peinture métallisée qui reflète le ciel et paraît grise. Seule la
            // copie de la police change (modele_), pas le modèle du trafic ni celui du joueur
            metalModele_.clear();
            for (Material& mt : modele_.mats) { metalModele_.push_back(mt.metal); if (mt.flags & MAT_PAINT) { WFS_LOG("poursuite : peinture de la police : métal %.2f → 0, rugosité %.2f", mt.metal, mt.rough); mt.metal = 0; } }
            if (modele_.ok) poserBandes();   // WFS-460 (RG-01) : bandes « POLICE » posées sur la tôle de ce modèle
        }
        // WFS-119 : camion en travers ; WFS-542 : un vrai camion porteur (le même pour toutes les voitures du joueur), chargé une fois
        if (r_ && actif_ && !camionModele_.ok && !camionEssaye_) { camionEssaye_ = true; loadCarModel(*r_, CAMION_MODELE, camionModele_); WFS_LOG("poursuite : camion %s, %zu pièces", CAMION_MODELE, camionModele_.parts.size()); }
    }

    // après la course (210), le duel (220) et la course à 5 (222)
    void collide(double dt, Player& pl, const GameView& g) override {
        Course* c = activeCourseMut();
        if (modeCourse() != ModeCourse::Poursuite || g.ecran != Ecran::Course || !c || duelSession().inRoom()) { actif_ = false; return; }
        if (c->duelFinish <= 0) demarrer(*c, pl);   // course remise à zéro (départ du menu, « Rejouer »)
        if (!actif_ || c->state == RaceState::Count) return;   // décompte : la police attend derrière
        Track& t = *g.track;
        if (g_bancFin == 2) {   // banc : saut à 150 m de l'arrivée (capture de la victoire)
            const double d = c->duelFinish - 150 / M - pl.dist;
            if (d > 0) { pl.dist += d; pl.position = t.wrap(pl.position + d); for (Police& p : police_) poursuite::poser(p, p.rd + d, p.x, p.v); }
            // WFS-145 : le trafic n'a pas sauté avec la voiture : celui de la zone d'arrivée (−40 à +220 m) passe 400 m plus
            // loin, sinon la voiture atterrit dans une file et le pilote automatique du banc reste coincé (CI rouge au hasard)
            if (Traffic* trs = activeTrafficMut())
                for (TrafficCar& tc : trs->cars) if (const double rel = t.relZ(tc.z - pl.position) * M; rel > -40 && rel < 220) tc.z = t.wrap(tc.z + 400 / M);
            g_bancFin = 0; distAvant_ = pl.dist;
        }
        if (g_bancSautKm > 0) {   // WFS-141 : banc : saut après le barrage et l'embouteillage (le pilote automatique y reste bloqué)
            const double d = g_bancSautKm * duel::KM - pl.dist;
            if (d > 0) { pl.dist += d; pl.position = t.wrap(pl.position + d); }   // la police reste derrière et rattrape
            g_bancSautKm = 0; distAvant_ = pl.dist;
        }
        Traffic* tr = activeTrafficMut();
        if (c->state == RaceState::Play) {
            // RG-02 : chocs avec le trafic (compteurs du module trafic), quasi-contacts (+1)
            if (const TrafficStats* st = activeTrafficStats()) {
                const int chocs = st->bumps + st->accidents + st->scrapes;
                if (chocs > chocsVus_) chocTrafic(etat_);
                for (int k = quasiVus_; k < st->nearMisses; k++) quasiContact(etat_);
                chocsVus_ = chocs; quasiVus_ = st->nearMisses;
            }
            // glissière : au moment où la voiture touche la bordure (Player::update la bloque à map->edge)
            const double bord = g.map ? g.map->edge : 1.6;
            const bool contre = std::abs(pl.playerX) >= bord - 1e-6;
            if (contre && !contreBord_) chocGlissiere(etat_);
            contreBord_ = contre;
            plotsTouches(pl);
            camionsTouches(dt, pl, *c, t);   // WFS-119
            bandesPassees(pl);
        }
        distAvant_ = pl.dist;
        camionsTrafic(pl, t, tr);
        camionsRetires(dt, pl);   // WFS-542 (RG-04)
        // police : trafic autour d'elle, conduite, contacts (règle du duel : duel::contacter)
        obs_.clear();
        if (tr) for (const TrafficCar& v : tr->cars) {
            const double s = pl.dist + t.relZ(v.z - pl.position);
            if (s - pl.dist > -300 / M && s - pl.dist < 400 / M) obs_.push_back({s, v.offset, v.crashed ? 0.0 : v.speed});
        }
        for (const Plot& p : plots_) if (!p.tombe) obs_.push_back({p.s, p.x, 0});
        for (const Camion& k : camions_) for (int f = 0; f < 3; f++) if (f != k.fileLibre) obs_.push_back({k.s, FILES[f], 0});   // WFS-119
        const FichePolice fiche = fichePolice(policeCar(&pl), pl.car ? pl.car->id : nullptr);   // WFS-461 (RG-03) : fiche de sa voiture
        // WFS-540 (RG-00) : la police roule avec le moteur de conduite de la voiture du joueur, sur la chaussée de la carte (saison)
        poursuite::avancer(police_, dt, c->raceTime, pl.dist, pl.playerX, pl.speed, obs_, fiche, t, cartes_v2::saisonPartie().muChaussee, pl.heading + pl.spinYaw);
        if (bancPoliceActive_ && !police_.empty()) {   // WFS-452 (banc « roues-suivies » seulement) : 1re voiture tenue à côté du joueur
            Police& p0 = police_[0];   // rdPrec / xPrec : place du pas précédent (poursuite::avancer les a notées)
            const double rdP = p0.rdPrec, xP = p0.xPrec;
            poursuite::poser(p0, pl.dist + bancPoliceEcart_ / M, bancPoliceX_, pl.speed); p0.rdPrec = rdP; p0.xPrec = xP;
            for (size_t i = 1; i < police_.size(); i++) poursuite::poser(police_[i], pl.dist - 400 / M, police_[i].x, 0);   // les autres loin derrière
        }
        if (suivi_.size() != police_.size()) suivi_.resize(police_.size());   // WFS-452 (RG-03, RG-08) : braquage d'après le vrai
        for (size_t i = 0; i < police_.size(); i++)                          // mouvement ; WFS-540 : avec le vrai cap de la caisse
            suivi_[i].pas(police_[i].rd * M, police_[i].x * RW, police_[i].h, roues::courbureRoute(t, police_[i].rd), carById(policeCar_).d.wb, dt);
        if (c->state == RaceState::Play) {
            for (Police& p : police_) {
                // WFS-540 : vitesse de la police le long de la route (sa caisse a son propre cap)
                duel::Ghost gh; gh.rd = p.rd; gh.x = p.x; gh.v = p.v * std::cos(p.h); gh.car = policeCar_;
                const duel::ContactEvents e = duel::contacter(dt, pl, t, *g.car, gh);
                if ((e.chocs || e.frottements) && p.hitCd <= 0) {   // WFS-540 : la vitesse de la police change par l'impulsion du choc
                    p.hitCd = 1.5; chocPolice(etat_);                // (poursuite::avancer), plus à la main (avant : × 0,85)
                    if (e.accident) c->accident(); else c->bump(std::max(0.1, e.impact));
                }
            }
            // WFS-540 (RG-03, RG-04) : arrestation (joueur arrêté près de la police), pénalité seulement au contact réel
            poursuite::pas(etat_, dt, plusProcheM(police_, pl.dist, pl.playerX), ecartCarrosseriesM(police_, pl.dist, pl.playerX, fiche), pl.kmh());
            // RG-03 : un renfort tous les 2 km (6 au plus)
            const double km = pl.dist * M / 1000;
            int mobiles = 0; for (const Police& p : police_) mobiles += !p.fixe;
            if (mobiles < policeVoulue(km)) {
                police_.push_back(renfort(mobiles, pl.dist, pl.speed, graine_));
                annoncer(ANNONCE_RENFORT);
                WFS_LOG("poursuite : renfort à %.2f km, %d voitures de police", km, mobiles + 1);
            }
            surprises(pl, t, tr);
            if (g_bancFin == 1) { perdre(etat_, etat_.score, RAISON_BANC); g_bancFin = 0; }
            arrivee(etat_, km);
            if (etat_.arrete && c->state != RaceState::Over) {
                c->state = RaceState::Over;
                WFS_LOG("poursuite : ARRÊTÉ%s à %.2f km (%d chocs trafic, %d glissière, %d police)", etat_.parPolice ? " PAR LA POLICE" : "", km, etat_.chocsTrafic, etat_.chocsGlissiere, etat_.chocsPolice);
            }
        }
        bouchons(pl, t, tr);
        {   // WFS-131 (RG-07) : sirène continue du module son : police mobile la plus proche (m) et vitesse vers le joueur (m/s)
            double dSir = 1e9, vSir = 0;
            for (const Police& p : police_) if (!p.fixe) {
                const double dz = (p.rd - pl.dist) * M, dx = (p.x - pl.playerX) * RW, d = std::sqrt(dz * dz + dx * dx);
                if (d < dSir) { dSir = d; vSir = (dz < 0 ? p.v - pl.speed : pl.speed - p.v) * M; }
            }
            sfxSirene(dSir, vSir);
        }
        if (banT_ > 0) banT_ -= dt;
        // WFS-338 (RG-03) : chaque perte de points s'écrit sous le score (cumulée tant que la raison ne change pas)
        if (perteT_ > 0) perteT_ -= dt;
        if (etat_.pertes != pertesVues_) {
            const bool meme = perteT_ > 0 && perteRaison_ == etat_.raison;
            perteCumul_ = (meme ? perteCumul_ : 0) + etat_.dernierePerte; perteRaison_ = etat_.raison;
            perteTexte_ = textePerte(perteCumul_, etat_.raison); perteT_ = PERTE_S; pertesVues_ = etat_.pertes;
        }
        if (c->state == RaceState::Over) {
            if (!fini_) {
                fini_ = true; ftJoueur_ = c->raceTime; arrivee(etat_, pl.dist * M / 1000);
                sfxInterface(etat_.gagne ? SfxId::PoursuiteEchappe : SfxId::PoursuiteArrete);   // WFS-131 (RG-06) : jingle de fin
                WFS_LOG("poursuite : fin — %s, score %d, %.2f km, %.1f s", etat_.gagne ? "victoire" : "ARRÊTÉ", etat_.score, pl.dist * M / 1000, ftJoueur_);
                { CourseJouee rc; rc.mode = ModeJoue::Poursuite; rc.victoire = etat_.gagne; rc.score = etat_.score; profilNoterResultat(rc); }   // WFS-480
                // WFS-125 (RG-02) : record Poursuite (victoire > « ARRÊTÉ », puis score ou distance), écrit seulement s'il est battu
                nouveauRecord_ = false;
                Sauvegarde& sv = sauvegarde();
                if (sv.actif()) {
                    nouveauRecord_ = sv.battreRecordPoursuite(etat_.gagne, etat_.score, std::llround(pl.dist * M));
                    WFS_LOG("poursuite : record %s%s", texteRecord().c_str(), nouveauRecord_ ? " (NOUVEAU RECORD, enregistré)" : "");
                    if (nouveauRecord_) sv.enregistrer();
                }
            }
            overT_ += dt;
        }
    }

    void render(Renderer& r, const GameView& g) override {
        if (!actif_ || g.sceneDediee || !g.player || !g.track) return;
        const double sP = g.player->dist;
        {   // banc : angle de la caméra « vitrine » placée devant la voiture (police visible derrière)
            const TrackPoint a = g.track->point(sP, 0), b = g.track->point(sP + 5 / M, 0);
            orbitAvant_ = (float)std::atan2(b.x - a.x, b.z - a.z);
        }
        for (const Police& p : police_) {
            const double rel = (p.rd - sP) * M;
            // WFS-338 (RG-05) : dessinée jusqu'à 160 m derrière (avant : 60 m, elle surgissait puis « s'effaçait » à 60 m)
            if (rel < -DESSIN_DERRIERE_M || rel > DESSIN_DEVANT_M) continue;
            if (r.qualiteBasse && std::abs(rel) > 90) continue;   // WFS-134 (RG-11) : Basse : police dessinée jusqu'à 90 m (coût)
            const double al = std::abs(p.rd - p.rdPrec) * M > 20 ? 1.0 : g.alpha;   // WFS-221 (accroche) : rendu interpolé
            const TrackPoint q = g.track->pointLisse(p.rdPrec + (p.rd - p.rdPrec) * al, (p.xPrec + (p.x - p.xPrec) * al) * RW);
            // WFS-540 (RG-08, ANO-272) : la caisse est dessinée avec SON cap (avant : celui de la route : elle glissait de côté sans tourner)
            const double hR = p.hPrec + std::remainder(p.h - p.hPrec, 6.283185307179586) * al;
            const mat4 m = translate({(float)q.x, (float)q.y, (float)q.z}) * rotateY((float)-(q.th + hR)) * rotateX((float)q.slope);
            const CarDef& cd = carById(policeCar_);
            const float W = cd.d.W;
            const int i = (int)(&p - police_.data());
            // WFS-134 (RG-11, remplace RG-08 / WFS-119 RG-04) : jamais de cubes, Basse comprise : toujours le vrai modèle (version
            // allégée au-delà de 60 m, avec fondu, choisie par Renderer::draw) ; sans modèle chargé : rien plutôt qu'une boîte
            if (!modele_.ok || i >= 16) continue;
            peintures_[i] = lin(blancOrigine_ ? 0xf2f4f8u : p.peinture); // WFS-452 (RG-08) : lots (Basse) seulement au-delà de 15 m de la caméra : les pièces des lots (4 pièces) n'ont pas de roues
            // séparées, la police glissait roues figées ; à moins de 15 m, Renderer::draw anime les roues (même en Basse)
            if (roues::g_avant452 || length(vec3{(float)q.x, (float)q.y, (float)q.z} - g.camPos) > 15.f) r.lotsProchaineVoiture();
            // WFS-452 (RG-02, RG-03, RG-08) : roues qui tournent (distance interpolée, rayon de la roue du modèle) et qui braquent
            static const roues::SuiviRoues droites{};
            const RouesPose rp = roues::poseSuivi((size_t)i < suivi_.size() ? suivi_[(size_t)i] : droites, al, p.rdPrec + (p.rd - p.rdPrec) * al,
                                                  modele_.nRoues ? modele_.roues[0].rayon : roues::RW_DEFAUT);
            if (roues::g_journal452 && i == 0) WFS_LOG("roues (banc) : police : roulage %.3f rad, braquage %.3f rad", rp.roulage, rp.braquage);
            r.draw(modele_, m, &peintures_[i], &rp);
            plaques_autres::dessiner(r, policeCar_, m, 1.f, plaques_autres::POLICE_0 + i % plaques_autres::POLICE_N, plaques_autres::Genre::Police, g.night, g.camPos);   // WFS-484 (accroche, RG-04)
            if (std::abs(rel) <= 80 && !g_bancSans338) conducteurs::ajouter(modele_, m, policeCar_, 0x50000000u + (uint32_t)i, true);   // WFS-338 (RG-06) : policier au volant
            // repère du modèle (m) : sol bmin.y, centre (cx, cz), avant vers −z ; pavillon mesuré au-dessus de la place du
            // conducteur (Model::siege, WFS-73), sinon le haut du modèle
            const vec3 b0 = modele_.bmin, b1 = modele_.bmax;
            const float cx = (b0.x + b1.x) / 2, cz = (b0.z + b1.z) / 2;
            const float toit = modele_.siege.w > 0.5f ? modele_.siege.x : b1.y, zToit = modele_.siege.w > 0.5f ? modele_.siege.z + 0.25f : cz + 0.1f;
            const bool proche = std::abs(rel) <= 80;   // WFS-338 (décision PO) : au-delà de 80 m, halos seuls (rampe, bandes, policier : de près)
            const bool detail = (!r.qualiteBasse || std::abs(rel) <= 40) && !g_bancSans338;   // Basse : bandes et socle jusqu'à 40 m (coût)
            // WFS-338 (RG-07, ANO-197) : bandes « POLICE » sur les portières (bleu, liseré blanc et rouge), sans marque ;
            // WFS-460 (RG-01, ANO-250) : maillage posé sur la tôle des portières, dans le repère du modèle (un seul dessin, comme avant)
            if (detail && g_bancBandesAvant) {   // banc seulement : les bandes de main, même scène, même image (avant / après)
                if (!bandesAvant_.indexCount) bandesAvant_ = bandesPlatesDeMain(r);
                const vec3 b0 = modele_.bmin, b1 = modele_.bmax; const float Hm = b1.y - b0.y;
                r.draw(bandesAvant_, matBandes_, m * translate({cx, b0.y + 0.40f * Hm, cz - 0.04f * cd.d.L}) * scale({W + 0.012f, 0.17f * Hm, 0.50f * cd.d.L}));
            } else if (detail && bandesMesh_.indexCount) r.draw(bandesMesh_, matBandes_, m);
            // WFS-338 (RG-07, ANO-197) : rampe lumineuse posée sur le pavillon : socle gris de 1 m × 30 cm, deux feux bleus qui
            // clignotent l'un après l'autre (gyrophare(), 2 Hz, une voiture sur deux en opposition) ; feu éteint : verre bleu
            // sombre ; avant : deux boîtes rouge / bleue de 0,56 m posées sous le haut du modèle, qui traversaient le toit
            const Gyrophare gy = gyrophare(g.time, i);
            const float barre = std::min(1.0f, W * 0.58f), feu = barre * 0.40f, xFeu = barre * 0.27f, demi = HALO_GYRO * feu / 2;
            if (proche) {
                if (detail) r.draw(cube_, matSupport_, m * translate({cx, toit - 0.01f, zToit}) * scale({barre, 0.05f, 0.30f}));
                r.draw(cube_, gy.rouge ? matBleu_ : matVitreBleue_, m * translate({cx - xFeu, toit + 0.04f, zToit}) * scale({feu, 0.07f, 0.26f}));
                r.draw(cube_, gy.bleu ? matBleu_ : matVitreBleue_, m * translate({cx + xFeu, toit + 0.04f, zToit}) * scale({feu, 0.07f, 0.26f}));
            }
            if (gy.rouge) r.halo(m.transformPoint({cx - xFeu, toit + 0.08f, zToit}), demi, haloBleu_);
            if (gy.bleu) r.halo(m.transformPoint({cx + xFeu, toit + 0.08f, zToit}), demi, haloBleu_);
        }
        for (const Plot& p : plots_) {   // RG-05 : travaux ; WFS-338 (RG-01) : cônes K5a (socle, corps, bandes : un seul dessin par cône)
            const double rel = (p.s - sP) * M;
            if (g_bancSans338 || p.tombe || rel < -30 || rel > DESSIN_DEVANT_M) continue;
            const TrackPoint q = g.track->point(p.s, p.x * RW);
            r.draw(coneMesh_, matCones_, translate({(float)q.x, (float)q.y, (float)q.z}) * rotateY((float)-(q.th)));
        }
        for (const Panneau& pn : panneaux_) {   // WFS-338 (RG-01) : panneau AK5 « travaux » (triangle rouge, fond jaune) sur pied
            const double rel = (pn.s - sP) * M;
            if (g_bancSans338 || rel < -30 || rel > DESSIN_DEVANT_M) continue;
            const TrackPoint q = g.track->point(pn.s, pn.x * RW);
            const mat4 m = translate({(float)q.x, (float)q.y, (float)q.z}) * rotateY((float)-(q.th));
            r.draw(cube_, matSupport_, m * translate({0, 0, -0.03f}) * scale({0.05f, 0.40f, 0.05f}));
            r.draw(cube_, matSupport_, m * translate({0, 0, 0}) * scale({0.70f, 0.03f, 0.40f}));   // pied lesté
            r.draw(triangle_, matPanneauRouge_, m * translate({0, 0.38f, 0}));
            r.draw(triangle_, matPanneauJaune_, m * translate({0, 0.38f + 0.289f * 0.30f, 0.008f}) * scale({0.70f, 0.70f, 1}));
        }
        for (const Camion& k : camions_) {   // WFS-119 : camion en travers ; WFS-542 : vrai camion porteur à l'échelle 1 (allégé au loin, vehicule_lod)
            const double rel = (k.s - sP) * M;
            if (rel < -40 || rel > DESSIN_DEVANT_M) continue;
            const TrackPoint q = g.track->point(k.s, k.tv.xM);
            // en travers, perpendiculaire à la route : la cabine (avant, −Z du modèle) tournée vers le milieu de la route et la file
            // libre, le hayon de la caisse vers le bord fermé (le camion a tourné pour reculer vers le trottoir : livraison)
            const double quart = k.fileLibre == 0 ? -1.5707963 : 1.5707963;
            const mat4 m = translate({(float)q.x, (float)q.y, (float)q.z}) * rotateY((float)-(q.th + quart)) * rotateX((float)q.slope);
            if (camionModele_.ok) { peintureCamion_ = lin(0xe8eaea); r.draw(camionModele_, m, &peintureCamion_); }   // caisse blanche de loueur
        }
        for (const Bande& b : bandes_) {   // WFS-119 : ralentisseur jaune (bandes noires hors qualité Basse) et 2 plots
            const double rel = (b.s - sP) * M;
            if (rel < -30 || rel > 250) continue;
            const TrackPoint q = g.track->point(b.s, 0);
            const mat4 m = translate({(float)q.x, (float)q.y, (float)q.z}) * rotateY((float)-(q.th));
            r.draw(cube_, matJaune_, m * scale({(float)(2 * RW), 0.10f, 0.8f}));
            if (!r.qualiteBasse)
                for (int i = -3; i <= 3; i++) r.draw(cube_, matNoir_, m * translate({(float)(i * RW * 2 / 7), 0, 0}) * scale({1.0f, 0.105f, 0.82f}));
            for (int sg = -1; sg <= 1; sg += 2) r.draw(cube_, matPlot_, m * translate({(float)(sg * (RW + 0.4)), 0, 0}) * scale({0.38f, 0.75f, 0.38f}));
        }
    }

    void drawUi(Renderer& r, const GameView& g) override {
        if (!actif_ || !ui::fontReady() || g.ecran != Ecran::Course || !g.player) return;
        const float W = g.uiWidth, H = g.uiHeight, u = H / 100.f;
        const Course* c = activeCourse();
        if (fini_ && overT_ >= 0.9) { resultat(r, W, H, u); return; }
        if (!c || c->state == RaceState::Over) return;
        // RG-06 : en haut au centre : score (100 → 1), police (nombre de voitures), bandeau de surprise
        const float cx = (g.marges.gauche + W - g.marges.droite) / 2, y0 = g.marges.haut + 2 * u, pw = 40 * u, ph = 15.5f * u;
        r.uiRect(cx - pw / 2, y0, pw, ph, SURFACE);
        const vec4 coul = etat_.score > 50 ? TEXTE : etat_.score > 20 ? ui::ORANGE : DANGER;
        static textes::Capitales libScore{"police.score"}, libPolice{"police.police"};   // WFS-506 (RG-01)
        ui::text(r, cx - pw / 2 + 2 * u, y0 + 1.6f * u, 2.4f * u, libScore(), MUTED, ui::GAUCHE, 0.16f, false);
        ui::text(r, cx - pw / 2 + 2 * u, y0 + 4.6f * u, 9 * u, std::to_string(etat_.score), coul, ui::GAUCHE, 0.f, false, true);
        int mobiles = 0; for (const Police& p : police_) mobiles += !p.fixe;
        ui::text(r, cx + pw / 2 - 2 * u, y0 + 1.6f * u, 2.4f * u, libPolice(), MUTED, ui::DROITE, 0.16f, false);
        const bool flash = std::fmod(g.time, 0.5) < 0.25;
        for (int i = 0; i < POLICE_MAX; i++) {   // une pastille par voiture (allumées : rouge / bleu)
            const float px = cx + pw / 2 - 2 * u - (POLICE_MAX - i) * 2.8f * u + 0.6f * u;
            r.uiCircle(px + 1.f * u, y0 + 6.6f * u, 1.f * u, i < mobiles ? ((i % 2) ^ flash ? DANGER : BLEU) : vec4{1, 1, 1, 0.12f});
        }
        // WFS-338 (RG-02, ANO-192) : distance à la police la plus proche (avant : la distance à l'arrivée, déjà écrite à gauche)
        ui::text(r, cx + pw / 2 - 2 * u, y0 + 9.6f * u, 3 * u, texteDistancePolice(plusProcheM(police_, g.player->dist, g.player->playerX)), TEXTE, ui::DROITE, 0.06f, false, true);
        const float k = (float)std::clamp(g.player->dist * M / 1000 / KM_ARRIVEE, 0.0, 1.0);
        r.uiRect(cx - pw / 2 + 2 * u, y0 + ph - 1.8f * u, pw - 4 * u, 0.6f * u, vec4{1, 1, 1, 0.12f});
        r.uiRect(cx - pw / 2 + 2 * u, y0 + ph - 1.8f * u, (pw - 4 * u) * k, 0.6f * u, ui::ORANGE);
        if (perteT_ > 0 && !perteTexte_.empty()) {   // WFS-338 (RG-03, ANO-193) : raison de la dernière perte, sous l'encart
            // WFS-506 (RG-01, RG-04) : « -5 CHOC AVEC LE TRAFIC » / « -5 HIT TRAFFIC », dans un tampon réutilisé
            char n[16]; std::snprintf(n, sizeof n, "%d", perteCumul_);
            static std::string perte;
            textes::remplir(perte, "police.perte", n, raisonAffichee(perteRaison_));
            const float tw = ui::width(3 * u, perte, 0.06f) + 4 * u, ty = y0 + ph + 1.f * u;
            r.uiRect(cx - tw / 2, ty, tw, 4.6f * u, SURFACE);
            ui::text(r, cx, ty + 0.8f * u, 3 * u, perte, DANGER, ui::CENTRE, 0.06f, false, true);
        }
        if (const int n = compteArrestation(etat_)) {   // WFS-540 (RG-03) : « ARRESTATION 4 » … « 1 », à la place du bandeau de surprise
            char b[8]; std::snprintf(b, sizeof b, "%d", n);
            static std::string arr;
            textes::remplir(arr, "police.arrestation", b);
            const float k = H / 720.f, bw = std::max(46 * u, ui::width(3.6f * u, arr, 0.1f) + 8 * u), bx = g.marges.gauche + 16 * k, by = g.marges.haut + 234 * k;
            r.uiRect(bx, by, bw, 7 * u, vec4{DANGER.x, DANGER.y, DANGER.z, 0.88f});
            ui::text(r, bx + bw / 2, by + 1.7f * u, 3.6f * u, arr, TEXTE, ui::CENTRE, 0.1f, false, true);
        } else if (banT_ > 0 && !ban_.empty()) {   // RG-05 : bandeau court
            // WFS-141 (RG-02, ANO-57) : à gauche sous le chrono et les boutons « Menu » et de vue (14 + 112 + 10 + 40 + 8 + 40 px
            // de 720, pause.cpp et camera_vue.cpp, + 10) : il ne cache plus l'obstacle, au centre de la route ; effacé après
            // 1,5 s. Taille inchangée (ordre permanent du 2026-11-06 sur la taille des textes : pas de passage à 5 %)
            const std::string& ban = annonceAffichee(ban_);   // WFS-506 (RG-01)
            const float k = H / 720.f, bw = std::max(46 * u, ui::width(3.6f * u, ban, 0.1f) + 8 * u), bx = g.marges.gauche + 16 * k, by = g.marges.haut + 234 * k;
            r.uiRect(bx, by, bw, 7 * u, vec4{DANGER.x, DANGER.y, DANGER.z, 0.88f});
            ui::text(r, bx + bw / 2, by + 1.7f * u, 3.6f * u, ban, TEXTE, ui::CENTRE, 0.1f, false, true);
        }
    }
    bool onTouch(const Touch& t, const GameView& g) override {
        if (g.ecran != Ecran::Course) return false;
        if (appui_.consomme(t.id, t.down)) return true;   // relâcher du doigt de « Rejouer » (déjà dans la course suivante)
        if (!actif_ || !fini_ || overT_ < 0.9) return false;
        if (!t.down) return true;
        if (rejouerZ_.contient(t.x, t.y)) { appui_.prendre(t.id); WFS_LOG("poursuite : « Rejouer »"); rejouer_ = true; g.allerA(Ecran::Menu); }
        else if (menuZ_.contient(t.x, t.y)) { appui_.prendre(t.id); WFS_LOG("poursuite : « Menu »"); g.allerA(Ecran::Menu); }
        return true;   // l'écran de fin garde tous les touchers
    }

    // banc d'essai et test de parcours
    const Etat& etat() const { return etat_; }
    bool fini() const { return fini_ && overT_ >= 0.9; }
    int policeMobiles() const { int n = 0; for (const Police& p : police_) n += !p.fixe; return n; }
    int police(double* rd, bool* fixe, int max) const { int n = 0; if (actif_) for (const Police& p : police_) if (n < max) { rd[n] = p.rd; fixe[n++] = p.fixe; } return n; }   // WFS-173
    const std::vector<Police>* voitures(const char** car) const { if (car) *car = policeCar_; return actif_ ? &police_ : nullptr; }   // WFS-272 (accroche)
    int policeDerriere(double sJ) const { int n = 0; for (const Police& p : police_) n += !p.fixe && p.rd < sJ && (sJ - p.rd) * M < 40; return n; }
    const std::string& bandeauActuel() const { return ban_; }
    // banc (WFS-172, RG-02) : voitures de police mobiles entre a et b m du joueur (sJ, xJ)
    int policeEntre(double sJ, double xJ, double a, double b) const {
        int n = 0;
        for (const Police& p : police_) { const double d = plusProcheM({p}, sJ, xJ); n += d >= a && d <= b; }
        return n;
    }
    void bancScore(int score) { etat_.score = score; }
    void bancMessages() { annoncer(ANNONCE_RENFORT); chocTrafic(etat_); }   // WFS-506 (banc « textes »)
    // banc (WFS-338) : renfort tout de suite (même chemin qu'au passage des 2 km) ; distance (m) derrière le joueur de la
    // police mobile la plus proche (négatif : devant)
    void bancRenfort(const Player& pl) { const int n = policeMobiles(); police_.push_back(renfort(n, pl.dist, pl.speed, graine_)); annoncer(ANNONCE_RENFORT); }
    // banc (WFS-338, revue du lead dev) : toute la police mobile repart à APPARITION_M derrière (aucune à moins de 160 m),
    // à la vitesse du joueur : elle revient ensuite par la conduite normale
    void bancEloigner(const Player& pl) {
        int k = 0;
        for (Police& p : police_) if (!p.fixe) { const double rd = pl.dist - (APPARITION_M + 12 * k++) / M; poursuite::poser(p, rd, FILES[k % 3], pl.speed); p.cibleX = p.x; }
    }
    // banc (WFS-338, RG-06) : une voiture de police posée à côté du joueur (dzM m devant, x en demi-largeurs), arrêtée
    void bancPoser(const Player& pl, double dzM, double x) {
        for (Police& p : police_) if (!p.fixe) { poursuite::poser(p, pl.dist + dzM / M, x, 0); p.cibleX = x; return; }
    }
    // banc (WFS-338, RG-07) : peinture métallisée d'origine du modèle (true) ou peinture unie de la police (false), pour l'avant / après
    void bancPeintureOrigine(bool origine) {
        for (size_t k = 0; k < modele_.mats.size() && k < metalModele_.size(); k++) if (modele_.mats[k].flags & MAT_PAINT) modele_.mats[k].metal = origine ? metalModele_[k] : 0.f;
        blancOrigine_ = origine;
    }
    double bancProche(const Player& pl) const { return plusProcheM(police_, pl.dist, pl.playerX); }
    double bancDerniere(const Player& pl) const { for (size_t i = police_.size(); i-- > 0;) if (!police_[i].fixe) return (pl.dist - police_[i].rd) * M; return 1e9; }   // banc (WFS-119) : score remis avant les captures camion / ralentisseur
    float orbitAvant_ = 0;
    Zone rejouerZone() const { return rejouerZ_; }
    bool nouveauRecord() const { return nouveauRecord_; }
    // WFS-125 : « Victoire · 87 pts » ou « Arrêté · 3,42 km » ; « — » tant que le record est vide (RG-03)
    static std::string texteRecord() {
        const Sauvegarde& sv = sauvegarde();
        if (!sv.aRecordPoursuite()) return "—";
        return sv.recPVictoire ? ui::tr("police.record_victoire", std::to_string(sv.recPScore)) : ui::tr("police.record_arrete", ui::decimal(sv.recPDistM / 1000.0, 2));   // WFS-506
    }
    Zone menuZone() const { return menuZ_; }

private:
    void demarrer(Course& c, const Player& pl) {
        graine_ = g_banc ? 20261105u : (uint32_t)(nowSeconds() * 1000.0) * 2654435761u + 29u;
        c.duelFinish = KM_ARRIVEE * duel::KM;
        etat_ = Etat{}; prog_ = programme(graine_, g_banc);
        police_.clear(); plots_.clear(); panneaux_.clear(); camions_.clear(); bandes_.clear(); distAvant_ = pl.dist; pertesVues_ = 0; perteT_ = 0; perteCumul_ = 0; perteTexte_.clear();
        for (int i = 0; i < POLICE_DEPART; i++) { Police p = renfort(i, pl.dist, 0, graine_); poursuite::poser(p, pl.dist - (22 + 14 * i) / M, p.x, 0); police_.push_back(p); }
        const TrafficStats* st = activeTrafficStats();
        chocsVus_ = st ? st->bumps + st->accidents + st->scrapes : 0; quasiVus_ = st ? st->nearMisses : 0;
        actif_ = true; fini_ = false; nouveauRecord_ = false; ftJoueur_ = 0; overT_ = 0; banT_ = 0; ban_.clear(); contreBord_ = false; sireneT_ = 0;
        WFS_LOG("poursuite : graine %u, %.0f km, score %d, %d voitures de police, %zu embouteillages, %zu surprises", graine_, KM_ARRIVEE,
                etat_.score, POLICE_DEPART, prog_.bouchons.size(), prog_.surprises.size());
    }
    void annoncer(const std::string& t) { ban_ = t; banT_ = BANDEAU_S; }   // WFS-141 (RG-02) : 1,5 s (avant : 4 s)

    // place une voiture du trafic loin du joueur (hors de vue) à la distance s, file f, vitesse v (unités/s)
    TrafficCar* deplacer(Traffic& tr, const Player& pl, Track& t, double s, int f, double v) {
        TrafficCar* best = nullptr; double loin = 0;
        for (TrafficCar& c : tr.cars) {
            const double rel = t.relZ(c.z - pl.position) * M;
            const double d = rel < 0 ? -rel * 3 : rel;   // derrière d'abord, puis le plus loin devant
            if ((rel < -80 || rel > 900) && d > loin && !reserve(&c)) { loin = d; best = &c; }
        }
        if (!best) return nullptr;
        best->z = t.wrap(pl.position + (s - pl.dist)); best->lane = best->target = f; best->offset = best->prevOff = FILES[f];
        best->speed = v; best->pending = false; best->signal = 0; best->crashed = false; best->laneYaw = 0;
        best->prevDz = t.relZ(best->z - pl.position);
        reserves_.push_back(best);
        return best;
    }
    bool reserve(const TrafficCar* c) const { return std::find(reserves_.begin(), reserves_.end(), c) != reserves_.end(); }

    // RG-04 : embouteillages : files lentes sur les voies sauf le passage, remplies 650 m avant
    void bouchons(const Player& pl, Track& t, Traffic* tr) {
        if (!tr) return;
        for (Bouchon& b : prog_.bouchons) {
            const double s0 = b.km * duel::KM, s1 = s0 + b.longueurM / M, devant = (s0 - pl.dist) * M;
            if (!b.place && devant < 650 && devant > 280) {
                b.place = true; reserves_.clear();
                Mulberry32 R(graine_ + (uint32_t)(b.km * 1000));
                int n = 0;
                for (int f = 0; f < 3; f++) {
                    if (f == b.passage) continue;
                    for (double d = 4 + R.next() * 4; d < b.longueurM && n < 48; d += 8.5 + R.next() * 3, n++)   // file serrée en tête
                        deplacer(*tr, pl, t, s0 + d / M, f, (b.kmhMin + R.next() * (b.kmhMax - b.kmhMin)) / 3.6 / M);
                }
                WFS_LOG("poursuite : embouteillage à %.2f km, %.0f m, %d voitures, passage file %d", b.km, b.longueurM, n, b.passage);
            }
            if (devant < 160 && devant > 120 && b.place && ban_ != ANNONCE_BOUCHON) annoncer(ANNONCE_BOUCHON);
            if (!b.place || pl.dist > s1 + 200 / M) continue;
            // dans la zone : 20 à 40 km/h, pas de changement de file vers le passage (il reste étroit)
            for (TrafficCar& c : tr->cars) {
                const double s = pl.dist + t.relZ(c.z - pl.position);
                if (s < s0 || s > s1 || c.crashed || c.lane == b.passage) continue;
                const double vmax = (b.kmhMin + (c.rng % 1000) / 1000.0 * (b.kmhMax - b.kmhMin)) / 3.6 / M;
                c.speed = std::min(c.speed, vmax);
                if (c.target == b.passage) { c.target = c.lane; c.pending = false; c.signal = 0; }
            }
        }
    }

    // RG-05 : surprises annoncées par un bandeau, déclenchées 260 m avant
    void surprises(const Player& pl, Track& t, Traffic* tr) {
        for (Evenement& e : prog_.surprises) {
            const double s = e.km * duel::KM;
            // WFS-338 (RG-01) : « TRAVAUX » reste affiché tant que le chantier est devant et visible (ANNONCE_TRAVAUX_M → biseau)
            if (e.declenche && e.type == Surprise::Travaux && pl.dist < s && ban_ == bandeau(e.type)) banT_ = std::max(banT_, 0.2);
            if (e.declenche && !e.rappel && (e.type == Surprise::Camion || e.type == Surprise::Ralentisseur) && pl.dist >= s - RAPPEL_M / M && pl.dist < s) {
                e.rappel = true; annoncer(bandeau(e.type));   // WFS-119 : rappel du bandeau à l'approche
            }
            // WFS-338 (RG-01, RG-05) : obstacles posés PLACE_M avant (hors de la distance de dessin), bandeau ANNONCE_M avant
            const bool avance = sePlaceAvant(e.type);
            if (avance && !e.place && pl.dist >= s - PLACE_M / M) { e.place = true; poser(e, s, pl, t, tr); }
            if (e.declenche || pl.dist < s - (e.type == Surprise::Travaux ? ANNONCE_TRAVAUX_M : ANNONCE_M) / M) continue;
            e.declenche = true;
            if (!avance || !e.place) { e.place = true; poser(e, s, pl, t, tr); }
            annoncer(bandeau(e.type));
            WFS_LOG("poursuite : surprise « %s » à %.2f km (file %d)", bandeau(e.type), e.km, e.file);
        }
    }
    void poser(const Evenement& e, double s, const Player& pl, Track& t, Traffic* tr) {
            reserves_.clear();
            switch (e.type) {
                case Surprise::Barrage:   // deux voitures de police en travers de deux files ; la file e.file reste libre
                    for (int f = 0; f < 3; f++) if (f != e.file) { Police p; p.fixe = true; poursuite::poser(p, s, FILES[f], 0); p.cibleX = FILES[f]; p.peinture = 0xe6e8ea; police_.push_back(p); }
                    break;
                case Surprise::Deboite: {   // une voiture d'une file voisine se rabat devant le joueur
                    if (!tr) break;
                    int fj = 0; double bd = 9; for (int f = 0; f < 3; f++) if (std::abs(FILES[f] - pl.playerX) < bd) { bd = std::abs(FILES[f] - pl.playerX); fj = f; }
                    const int fv = fj == 1 ? (e.file % 2 ? 0 : 2) : 1;
                    TrafficCar* c = deplacer(*tr, pl, t, pl.dist + 45 / M, fv, std::max(pl.speed * 0.7, 8 / M));
                    if (c) { c->target = fj; c->signal = fj > fv ? 1 : -1; }
                    break;
                }
                case Surprise::Travaux: {  // WFS-338 (RG-01) : vrai chantier : panneau AK5, biseau de cônes, cônes sur 120 m
                    for (const Cone& c : chantier(e.file)) plots_.push_back({s + c.dM / M, c.x, false});
                    const double xp = FILES[e.file] + (e.file == 2 ? 0.22 : -0.22);
                    panneaux_.push_back({s - PANNEAU_AVANT_M / M, std::clamp(xp, -0.95, 0.95)});
                    break;
                }
                case Surprise::Accident:  // deux voitures accidentées en travers d'une file (feux de détresse du trafic vivant)
                    if (!tr) break;
                    for (int i = 0; i < 2; i++)
                        if (TrafficCar* c = deplacer(*tr, pl, t, s + i * 6 / M, e.file, 0)) {
                            c->crashed = true; c->crashT = -12; c->crashYaw = i ? -0.9 : 0.7; c->crashSpin = 0; c->crashVx = 0;
                        }
                    break;
                case Surprise::Camion: {  // WFS-119 : gros véhicule en travers de 2 files, la file e.file reste libre
                    Camion k; k.s = s; k.fileLibre = fileLibreCamion(e.file); k.tv = camionTravers(k.fileLibre);   // WFS-542 : vraies mesures
                    camions_.push_back(k);
                    break;
                }
                case Surprise::Ralentisseur: bandes_.push_back({s, false}); break;   // WFS-119 : toute la largeur
                default: break;
            }
            WFS_LOG("poursuite : obstacles de « %s » posés à %.0f m devant (%.2f km)", bandeau(e.type), (s - pl.dist) * M, e.km);
    }
    // plots percutés : −3 comme la glissière, la voiture ralentit, le plot tombe
    void plotsTouches(Player& pl) {
        const double demiL = pl.car->d.L / 2 / M, demiW = pl.car->d.W / 2 / RW;
        for (Plot& p : plots_) {
            if (p.tombe || std::abs(p.s - pl.dist) > demiL + 0.2 / M || std::abs(p.x - pl.playerX) > demiW + 0.03) continue;
            p.tombe = true; pl.speed *= 0.9; chocGlissiere(etat_, RAISON_PLOT);
        }
    }

    // WFS-119 : camion en travers : boîte (demi-largeur tv.demiX, demi-épaisseur tv.demiZ) ; de face : arrêté net
    // derrière lui (comme un choc avec le trafic, −5) ; par le bout : repoussé de côté (frottement)
    void camionsTouches(double dt, Player& pl, Course& c, Track& t) {
        for (Camion& k : camions_) {
            k.cd = std::max(0.0, k.cd - dt);
            const double dzM = (k.s - pl.dist) * M, demiZ = k.tv.demiZ + pl.car->d.L / 2;
            const double dxM = pl.playerX * RW - k.tv.xM, demiX = k.tv.demiX + pl.car->d.W / 2;
            if (std::abs(dzM) >= demiZ || std::abs(dxM) >= demiX) continue;
            const double ovZ = demiZ - std::abs(dzM), ovX = demiX - std::abs(dxM), impact = std::abs(pl.speed) / MAX_SPEED;
            const int cote = dxM > 0 ? 1 : -1;
            if (ovX < ovZ * 0.7) { pl.playerX += cote * (ovX + 0.05) / RW; pl.speed *= 1 - 0.8 * dt; }
            else {
                const double rec = (dzM > 0 ? 1 : -1) * (ovZ + 0.15) / M;
                pl.position = t.wrap(pl.position - rec); pl.dist = std::max(0.0, pl.dist - rec);
                pl.speed = std::min(pl.speed, pl.speed * 0.15);
                if (k.cd <= 0 && impact > 0.3) { pl.accident(std::min(1.0, impact), cote); c.accident(); }
            }
            if (k.cd <= 0) { k.cd = 1.0; chocTrafic(etat_, RAISON_CAMION); c.bump(std::max(0.1, impact)); WFS_LOG("poursuite : choc contre le camion (%.0f km/h)", impact * 260); }
        }
    }
    // WFS-542 (RG-04) : le camion ne reste pas toute la course : retiré 50 m derrière le joueur, ou 60 s après son apparition
    // (compté dès qu'il est à portée de vue, DESSIN_DEVANT_M : posé 450 m avant, il ne compte pas tant qu'on ne le voit pas)
    void camionsRetires(double dt, const Player& pl) {
        for (Camion& k : camions_) {
            const double derriere = (pl.dist - k.s) * M;
            if (k.age < 0 && -derriere <= DESSIN_DEVANT_M) k.age = 0;
            else if (k.age >= 0) k.age += dt;
        }
        const size_t n = camions_.size();
        camions_.erase(std::remove_if(camions_.begin(), camions_.end(), [&](const Camion& k) { return camionARetirer((pl.dist - k.s) * M, std::max(0.0, k.age)); }), camions_.end());
        if (camions_.size() != n) WFS_LOG("poursuite : camion retiré (%zu restant)", camions_.size());
    }
    // WFS-119 : le trafic des 2 files fermées s'arrête derrière le camion (la file libre roule) ; une voiture posée
    // dans le camion au déclenchement passe devant lui
    void camionsTrafic(const Player& pl, Track& t, Traffic* tr) {
        if (!tr) return;
        for (const Camion& k : camions_) {
            const double devant = (k.s - pl.dist) * M;
            if (devant < -150 || devant > 700) continue;
            for (TrafficCar& c : tr->cars) {
                if (c.crashed || c.lane == k.fileLibre) continue;
                if (std::abs(c.offset * RW - k.tv.xM) >= k.tv.demiX + c.w * RW) continue;
                const double s = pl.dist + t.relZ(c.z - pl.position);
                const double ecart = (k.s - s) * M - k.tv.demiZ - c.len * M / 2;   // > 0 : derrière le camion
                if (ecart < -2 * k.tv.demiZ - c.len * M) continue;                  // déjà passée
                if (ecart < 0) { c.z = t.wrap(c.z + (-ecart + 2 * k.tv.demiZ + c.len * M + 2) / M); continue; }
                if (ecart < 60) c.speed = std::min(c.speed, std::max(0.0, ecart - 3) * 0.6 / M);
            }
        }
    }
    // banc seulement (WFS-460, avant / après) : les bandes de main, un quadrilatère de chaque côté d'un cube unité (x = ±0,5),
    // recopiées telles quelles (même enroulement : le côté droit n'était jamais dessiné). Jamais créées en jeu.
    static Mesh bandesPlatesDeMain(Renderer& r) {
        std::vector<Vertex> v; std::vector<uint32_t> idx;
        for (int sg = -1; sg <= 1; sg += 2) {
            const uint32_t k = (uint32_t)v.size();
            for (int i = 0; i < 4; i++) {
                const float z = (i == 1 || i == 2) ? 0.5f : -0.5f, y = i >= 2 ? 1.f : 0.f, uu = sg > 0 ? 0.5f - z : z + 0.5f;
                v.push_back({sg * 0.5f, y, z, (float)sg, 0, 0, uu, 1.f - y});
            }
            idx.insert(idx.end(), {k, k + 1, k + 2, k, k + 2, k + 3});
        }
        return r.makeMesh(v, idx);
    }
    // WFS-460 (RG-01, RG-02, ANO-250) : bandes « POLICE » posées sur la tôle des portières du modèle de la police (modele_),
    // une fois à son chargement : triangles de la peinture relus dans le .wfm (le GPU ne garde pas la géométrie), passages de
    // roue mesurés par la découpe des roues (Model::roues, WFS-60), puis police_bandes::construire. Sans bande trouvée : rien
    // n'est dessiné (jamais un quadrilatère posé à la boîte de la voiture)
    void poserBandes() {
        if (bandesMesh_.indexCount) { r_->destroy(bandesMesh_); bandesMesh_ = Mesh{}; }
        const CarDef& cd = carById(policeCar_);
        std::vector<uint8_t> octets; vehicule::ModeleWfm w; std::string erreur;
        if (!readAsset(cd.file, octets) || !vehicule::lireWfmOctets(octets.data(), octets.size(), w, erreur)) {
            WFS_ERR("poursuite : bandes de la police : %s illisible (%s), pas de bandes", cd.file, erreur.c_str()); return;
        }
        std::vector<police_bandes::Triangle> tris;
        for (size_t k = 0; k < w.sommets.size(); k++) {
            if (!(w.drapeaux[k] & MAT_PAINT)) continue;   // tôle peinte seulement (pas les vitres, joints, feux, roues)
            const float* v = w.sommets[k].data(); const std::vector<uint32_t>& I = w.indices[k];
            for (size_t t = 0; t + 2 < I.size(); t += 3) {
                auto P = [&](uint32_t s) { return vec3{v[8 * s], v[8 * s + 1], v[8 * s + 2]}; };
                tris.push_back({P(I[t]), P(I[t + 1]), P(I[t + 2])});
            }
        }
        // passages de roue : pneus du modèle (avant vers −z) ; sans roues séparées : empattement et rayon de la fiche
        float zPneuAvant, zPneuArriere, sol = modele_.bmin.y;
        if (modele_.nRoues == 4) {
            const RoueModele* R = modele_.roues;
            zPneuAvant = std::max(R[roues::LF].centre.z + R[roues::LF].rayon, R[roues::RF].centre.z + R[roues::RF].rayon);
            zPneuArriere = std::min(R[roues::LB].centre.z - R[roues::LB].rayon, R[roues::RB].centre.z - R[roues::RB].rayon);
            sol = std::min({R[0].centre.y - R[0].rayon, R[1].centre.y - R[1].rayon, R[2].centre.y - R[2].rayon, R[3].centre.y - R[3].rayon});
        } else {
            const float cz = (modele_.bmin.z + modele_.bmax.z) / 2;
            zPneuAvant = cz - cd.d.wb / 2 + roues::RW_DEFAUT; zPneuArriere = cz + cd.d.wb / 2 - roues::RW_DEFAUT;
        }
        const police_bandes::Bandes b = police_bandes::construire(tris, zPneuAvant, zPneuArriere, sol, cd.d.H);
        if (!b.ok()) { WFS_ERR("poursuite : bandes de la police : aucune tôle trouvée sur %s (%zu triangles peints), pas de bandes", policeCar_, tris.size()); return; }
        std::vector<Vertex> vs; vs.reserve(b.sommets.size());
        for (const police_bandes::Sommet& s : b.sommets) vs.push_back({s.p.x, s.p.y, s.p.z, s.n.x, s.n.y, s.n.z, s.u, s.v});
        bandesMesh_ = r_->makeMesh(vs, b.indices);
        WFS_LOG("poursuite : bandes de la police (%s) posées sur la tôle : z %.2f → %.2f m (%.2f m), y %.2f → %.2f m, %zu sommets, "
                "%zu triangles, %d point(s) sans tôle, %d dans un jour de porte (WFS-460)", policeCar_, b.zAvant, b.zArriere,
                b.zArriere - b.zAvant, b.yBas, b.yHaut, vs.size(), b.indices.size() / 3, b.manques, b.creux);
    }
    // WFS-119 : ralentisseur franchi : −3 au-dessus de 90 km/h (poursuite::ralentisseur), la voiture tressaute et ralentit
    void bandesPassees(Player& pl) {
        for (Bande& b : bandes_) {
            if (b.passe || distAvant_ >= b.s || pl.dist < b.s) continue;
            b.passe = true;
            const double kmh = std::abs(pl.kmh());
            const int perdu = ralentisseur(etat_, kmh);
            if (perdu > 0) pl.speed *= 0.85;
            WFS_LOG("poursuite : ralentisseur passé à %.0f km/h : %s", kmh, perdu > 0 ? "−3" : "rien");
        }
    }

    // RG-07 : écran de fin : victoire (score final, temps) ou « ARRÊTÉ » ; « Rejouer » / « Menu » ;
    // WFS-141 (RG-03, ANO-59) : boutons parallélogrammes et liseré orange de la fin Checkpoint (biseauFin) ; tailles des
    // boutons et des textes inchangées (ordre permanent de l'utilisateur du 2026-11-06)
    void resultat(Renderer& r, float W, float H, float u) {
        r.uiRect(0, 0, W, H, FOND);
        const bool rec = sauvegarde().actif();   // WFS-125 (RG-04) : badge et ligne « RECORD » sous les lignes du résultat
        const float pw = 66 * u, ph = (58 + (rec ? 13 : 0)) * u, px0 = (W - pw) / 2, py0 = (H - ph) / 2;
        r.uiRect(px0, py0, pw, ph, SURFACE);
        score_fin::poser(px0, py0, pw, ph);   // WFS-537 (accroche) : bloc « Score » à côté du panneau
        r.uiRect(px0, py0, pw, 0.6f * u, ui::ORANGE);   // WFS-141 : liseré néon de la fin Checkpoint
        static std::string sur;   // WFS-506 (RG-03) : « POURSUITE POLICE · 10 KM » / « POLICE PURSUIT · 10 KM »
        textes::remplir(sur, "police.surtitre", "10");   // valeur affichée inchangée (10 km)
        ui::text(r, W / 2, py0 + 3 * u, 2.6f * u, sur, ui::CYAN, ui::CENTRE, 0.16f, false);
        const bool v = etat_.gagne;
        ui::text(r, W / 2, py0 + 7 * u, 9 * u, ui::tr(v ? "fin.victoire" : "police.arrete"), v ? ui::ORANGE : DANGER, ui::CENTRE, 0.04f, false, true);
        if (!v && etat_.parPolice) ui::text(r, W / 2, py0 + 16.2f * u, 2.6f * u, ui::tr("police.arrete_police"), TEXTE, ui::CENTRE, 0.16f, false);   // WFS-540 (RG-03)
        float y = py0 + 20 * u;
        auto ligne = [&](const std::string& a, const std::string& b) {
            ui::text(r, px0 + 6 * u, y, 3.4f * u, a, MUTED, ui::GAUCHE, 0.f, false);
            ui::text(r, px0 + pw - 6 * u, y, 3.4f * u, b, TEXTE, ui::DROITE, 0.f, false, true);
            y += 5.6f * u;
        };
        ligne(ui::tr("police.score_final"), std::to_string(etat_.score));   // WFS-506 (RG-03)
        ligne(ui::tr("fin.temps"), ui::chrono(ftJoueur_));
        ligne(ui::tr("police.chocs"), std::to_string(etat_.chocsTrafic) + " / " + std::to_string(etat_.chocsGlissiere) + " / " + std::to_string(etat_.chocsPolice));
        ligne(ui::tr("police.voitures"), std::to_string(policeMobiles()));
        const float bw = 24 * u, bh = 7 * u, gap = 3 * u, by = py0 + ph - bh - 3 * u, bx = (W - (2 * bw + gap)) / 2;
        if (rec) dessinerRecord(r, W / 2, y + 0.5f * u, by - 2 * u, u, texteRecord(), nouveauRecord_);
        rejouerZ_ = {bx, by, bw, bh}; menuZ_ = {bx + bw + gap, by, bw, bh};
        biseauFin(r, rejouerZ_.x, by, bw, bh, 14 * H / 720, ui::ORANGE);
        // WFS-506 (RG-03, RC-E20-04) : boutons de taille fixe, texte réduit de 20 % au plus s'il dépasse
        static textes::Capitales libRejouer{"fin.rejouer"}, libMenu{"fin.menu"};
        ui::textBoite(r, rejouerZ_.x + bw / 2, by + 1.9f * u, bw - 28 * H / 720, 3.4f * u, 3.4f * u, libRejouer(), vec4{0.04f, 0.04f, 0.06f, 1}, ui::CENTRE, 0.08f, false);
        biseauFin(r, menuZ_.x, by, bw, bh, 14 * H / 720, {30 / 255.f, 33 / 255.f, 48 / 255.f, 1});   // « Menu » de la fin Checkpoint (#1E2130)
        ui::textBoite(r, menuZ_.x + bw / 2, by + 1.9f * u, bw - 28 * H / 720, 3.4f * u, 3.4f * u, libMenu(), TEXTE, ui::CENTRE, 0.08f, false);
    }

    Renderer* r_ = nullptr;
    Model modele_; std::vector<float> metalModele_; bool blancOrigine_ = false; bool modeleEssaye_ = false; const char* policeCar_ = "p208";
    Mesh cube_; Material matRouge_, matBleu_, matPlot_, matBlanc_, matBleuCaisse_, matVitre_, matJaune_, matNoir_;
    vec3 haloRouge_{}, haloBleu_{};   // WFS-141 : couleurs HDR des halos des gyrophares
    Model camionModele_; bool camionEssaye_ = false; vec3 peintureCamion_{};   // WFS-119
    std::vector<Camion> camions_;
    std::vector<Bande> bandes_;
    double distAvant_ = 0;
    vec3 peintures_[16];   // Renderer::draw garde l'adresse de la peinture jusqu'à la fin de l'image
    std::vector<Police> police_;
    std::vector<roues::SuiviRoues> suivi_;   // WFS-452 : braquage des roues avant, une par voiture de police (même indice)
    bool bancPoliceActive_ = false; double bancPoliceEcart_ = 0, bancPoliceX_ = 0;   // WFS-452 : banc « roues-suivies »
    std::vector<Obstacle> obs_;
    std::vector<Plot> plots_;
    std::vector<Panneau> panneaux_;   // WFS-338
    Mesh bandesAvant_;   // WFS-460 (banc seulement) : quadrilatère plat de main, créé seulement par le banc avant / après
    Mesh coneMesh_, triangle_, bandesMesh_; Image bandesTex_{}, paletteCones_{};   // WFS-338
    std::vector<Vertex> coneV_; std::vector<uint32_t> coneI_;
    static constexpr float U_ORANGE = 0.125f, U_BLANC = 0.375f, U_NOIR = 0.625f;   // centres des texels de la palette des cônes
    Material matCones_, matPanneauRouge_, matPanneauJaune_, matSupport_, matVitreBleue_, matBandes_;
    int pertesVues_ = 0, perteCumul_ = 0; double perteT_ = 0; std::string perteTexte_; const char* perteRaison_ = "";   // WFS-338 (RG-03)
    std::vector<const TrafficCar*> reserves_;
    Programme prog_;
    Etat etat_;
    uint32_t graine_ = 0;
    int chocsVus_ = 0, quasiVus_ = 0;
    bool actif_ = false, fini_ = false, rejouer_ = false, contreBord_ = false, nouveauRecord_ = false;
    double ftJoueur_ = 0, overT_ = 0, banT_ = 0, sireneT_ = 0;
    std::string ban_;
    Zone rejouerZ_, menuZ_;
    ui::FiltreAppui appui_;
};
PoursuiteSystem* PoursuiteSystem::actif = nullptr;

} // namespace

WFS_SYSTEM(PoursuiteSystem, 224)   // gameplay (200-299) : après la course (210), le duel (220) et la course à 5 (222)

// WFS-125 : banc d'essai (parcours RG-09)
int poursuitePolice(double* rd, bool* fixe, int max) { return PoursuiteSystem::actif ? PoursuiteSystem::actif->police(rd, fixe, max) : 0; }   // WFS-173
const std::vector<poursuite::Police>* poursuiteVoitures(const char** car) { return PoursuiteSystem::actif ? PoursuiteSystem::actif->voitures(car) : nullptr; }   // WFS-272 (accroche)
bool poursuiteBouton(const std::string& nom, float& x, float& y) {
    const PoursuiteSystem* s = PoursuiteSystem::actif;
    if (!s) return false;
    const Zone z = nom == "rejouer" ? s->rejouerZone() : s->menuZone();
    if (z.w <= 0) return false;
    x = z.x + z.w / 2; y = z.y + z.h / 2; return true;
}
bool poursuiteNouveauRecord() { return PoursuiteSystem::actif && PoursuiteSystem::actif->nouveauRecord(); }
void poursuiteBancFin(int mode) { g_bancFin = mode; }
// WFS-506 (banc « textes ») : annonce « RENFORT DE POLICE » et perte de points « -5 CHOC AVEC LE TRAFIC » tout de suite (mêmes chemins qu'en course)
void poursuiteBancMessages() { if (PoursuiteSystem* s = PoursuiteSystem::actif) s->bancMessages(); }
double poursuiteBancProche(const Player& pl) { return PoursuiteSystem::actif ? PoursuiteSystem::actif->bancProche(pl) : 1e9; }   // WFS-460 (banc)
void poursuiteBancFixe(bool oui) { g_banc = oui; }
// WFS-452 (banc « roues-suivies ») : 1re voiture de police tenue à ecartM du joueur (m, < 0 : derrière), au latéral x (demi-largeurs)
void poursuiteBancPolice(bool actif, double ecartM, double x) {
    if (!PoursuiteSystem::actif) return;
    PoursuiteSystem::actif->bancPolice(actif, ecartM, x);
}

// ---------- banc d'essai (WFS_BANC=course5) ----------
// police avec gyrophares derrière le joueur, bandeau de surprise (barrage), embouteillage, HUD du score, fin « ARRÊTÉ »
// puis « Rejouer » et victoire à 10 km
void benchPoursuite(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    g_banc = true;
    modeCourse() = ModeCourse::Poursuite;
    if (!g.init(r, "golf", "ville")) { g_banc = false; courseBenchRules(false); modeCourse() = ModeCourse::Checkpoint; return; }
    double t = 1200;
    auto images = [&](int n, int tous) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); if (i % tous == 0) g.render(t += tous / 60.0); } };
    auto capture = [&](const char* nom) {
        g.render(t += 0.02); g.render(t += 0.02); r.capturePng(out + "/" + nom + ".png");
        const PoursuiteSystem* s = PoursuiteSystem::actif;
        const Traffic* tr = activeTraffic();
        WFS_LOG("capture %s : %.2f km, %.0f km/h, score %d, police %d (dont %d à moins de 40 m derrière), trafic %zu, bandeau « %s », %u appels de dessin",
                nom, g.player().dist * M / 1000, g.player().kmh(), s ? s->etat().score : -1, s ? s->policeMobiles() : 0,
                s ? s->policeDerriere(g.player().dist) : 0, tr ? tr->cars.size() : (size_t)0, s ? s->bandeauActuel().c_str() : "", r.appelsDessin());
    };
    auto jusqua = [&](double km, int max) { for (int i = 0; i < max && g.player().dist * M / 1000 < km; i++) { g.update(1.0 / 60); if (i % 12 == 0) g.render(t += 0.2); } };
    struct Mesure { const char* ou; double km; unsigned appels; };
    std::vector<Mesure> mesures;   // WFS-119 RG-04 : comparées plus bas à une course Checkpoint, mêmes distances
    // appels de dessin en qualité Basse (téléphone), sans capture ; WFS-119 : moyenne de 10 images sur 0,5 s (le trafic
    // autour du joueur fait varier le compte d'une image à l'autre)
    auto appelsBasse = [&]() {
        ecrans::Reglages& R = ecrans::reglages(); const int q0 = R.qualite;
        R.qualite = 0; ecrans::appliquerQualiteNiveau(r);
        unsigned somme = 0;
        for (int i = 0; i < 10; i++) {
            if (i) for (int k = 0; k < 3; k++) g.update(1.0 / 60);
            g.render(t += 0.02); g.render(t += 0.02); somme += r.appelsDessin();
        }
        const unsigned n = (somme + 5) / 10;
        R.qualite = q0; ecrans::appliquerQualiteNiveau(r);
        return n;
    };
    auto basse = [&](const char* ou) {
        const unsigned n = appelsBasse();
        mesures.push_back({ou, g.player().dist * M / 1000, n});
        WFS_LOG("poursuite (RG-08) : %s en qualité Basse : %u appels de dessin", ou, n);
        {   // WFS-338 (décision PO) : coût des ajouts de WFS-338 à cet endroit (même scène, même image : ajouts retirés)
            ecrans::Reglages& R = ecrans::reglages(); const int q0 = R.qualite;
            R.qualite = 0; ecrans::appliquerQualiteNiveau(r);
            g_bancSans338 = false; g.render(t); g.render(t); const unsigned avec = r.appelsDessin();
            g_bancSans338 = true; g.render(t); g.render(t); const unsigned sans = r.appelsDessin();
            g_bancSans338 = false; R.qualite = q0; ecrans::appliquerQualiteNiveau(r);
            WFS_LOG("poursuite (WFS-338) : %s en qualité Basse : %u appels avec WFS-338, %u sans : %+d", ou, avec, sans, (int)avec - (int)sans);
        }
    };
    // RG-12 : parcours au doigt : accueil → « Course solo » (panneau des modes) → carte « Poursuite » → « Continuer » →
    // garage → « Choisir » → terrain → « Rouler » → course de poursuite (la fin et « Menu » → accueil : plus bas)
    int ecarts = 0, doigt = 60;
    auto appuyer = [&](float x, float y) {   // doigt Android : down, 4 mouvements, up (une image dessinée entre chaque)
        const int id = doigt++;
        g.render(t += 1.0 / 60);
        g.onTouch({id, x, y, true}); g.update(1.0 / 60); g.render(t += 1.0 / 60);
        for (int k = 1; k <= 4; k++) { g.onTouch({id, x + 0.4f * k, y, true}); g.update(1.0 / 60); g.render(t += 1.0 / 60); }
        g.onTouch({id, x + 2, y, false}); images(4, 2);
    };
    auto attendu = [&](Ecran e, const char* etape) {
        if (g.ecran() == e) WFS_LOG("poursuite (parcours) : %s → écran %s (attendu)", etape, nomEcran(e));
        else { ecarts++; WFS_ERR("poursuite (parcours) : %s → écran %s, attendu %s", etape, nomEcran(g.ecran()), nomEcran(e)); }
    };
    // WFS-517 : zones lues sur le panneau lui-même (avant : mise en page à 4 cartes recopiée ici)
    auto panneau = [&](const char* zone) {
        float zx = 0, zy = 0;
        if (choixModeCentre(zone, zx, zy)) appuyer(zx, zy); else { ecarts++; WFS_ERR("poursuite (parcours) : zone %s du panneau absente", zone); }
    };
    {   // cartes et « Continuer » du panneau des modes
        g.allerA(Ecran::Menu); images(30, 10);
        choixModeBanc(true); ouvrirChoixMode(); images(10, 5);
        if (!choixModeOuvert()) { ecarts++; WFS_ERR("poursuite (parcours) : panneau des modes fermé"); }
        panneau("carte2");      // carte 03 « Poursuite »
        panneau("continuer");
        attendu(Ecran::Garage, "« Poursuite » puis « Continuer »");
        if (modeCourse() != ModeCourse::Poursuite) { ecarts++; WFS_ERR("poursuite (parcours) : mode %s après « Continuer »", nomMode(modeCourse())); }
        else WFS_LOG("poursuite (parcours) : mode %s retenu", nomMode(modeCourse()));
        float x = 0, y = 0;
        for (int i = 0; i < 600 && !ecrans::garageBouton("choisir", x, y); i++) images(1, 4);
        images(30, 4);
        if (ecrans::garageBouton("choisir", x, y)) appuyer(x, y); else { ecarts++; WFS_ERR("poursuite (parcours) : « Choisir » absent"); }
        attendu(Ecran::Terrain, "garage puis « Choisir »");
        images(20, 4);
        if (ecrans::centreRouler(x, y)) appuyer(x, y); else { ecarts++; WFS_ERR("poursuite (parcours) : « Rouler » absent"); }
        for (int i = 0; i < 120 && g.ecran() != Ecran::Course; i++) images(1, 1);
        attendu(Ecran::Course, "terrain puis « Rouler »");
        choixModeBanc(false);
        if (g.ecran() != Ecran::Course || modeCourse() != ModeCourse::Poursuite) {   // parcours raté : la suite des captures quand même
            modeCourse() = ModeCourse::Poursuite; g.allerA(Ecran::Menu); images(2, 1); g.allerA(Ecran::Course); images(2, 1);
        }
    }
    g.camMode = 0; g.setAutopilot(true);
    images(3 * 60, 8);        // décompte (3 s)
    for (int i = 0; i < 20 * 60; i++) { const Course* c = activeCourse(); if (c && c->raceTime >= 8.6) break; g.update(1.0 / 60); if (i % 12 == 0) g.render(t += 0.2); }
    // la 2e voiture de police tente de pousser (attaque de 7,6 à 10,6 s) : vue de devant, police derrière le joueur
    if (const PoursuiteSystem* s = PoursuiteSystem::actif) { g.camMode = 2; g.render(t += 0.02); g.orbit = s->orbitAvant_ + 0.5f; }
    capture("poursuite-police");
    {   // WFS-172 (RG-02, ANO-87) : même vue en qualité Basse (téléphone), dessinée à la même heure (aucun pas de temps
        // pour la caméra ni les animations) : la suite du banc est inchangée
        ecrans::Reglages& R = ecrans::reglages(); const int q0 = R.qualite;
        R.qualite = 0; ecrans::appliquerQualiteNiveau(r);
        const PoursuiteSystem* s = PoursuiteSystem::actif;
        const int n = s ? s->policeEntre(g.player().dist, g.player().playerX, 10, 30) : 0;
        WFS_LOG("poursuite (banc) : qualité Basse : %d voiture(s) de police à 10-30 m", n);
        if (n == 0) WFS_ERR("poursuite (banc) : aucune police à 10-30 m pour poursuite-police-basse");
        g.render(t); g.render(t); r.capturePng(out + "/poursuite-police-basse.png");
        WFS_LOG("capture poursuite-police-basse : %.2f km, police %d, %u appels de dessin", g.player().dist * M / 1000, s ? s->policeMobiles() : 0, r.appelsDessin());
        R.qualite = q0; ecrans::appliquerQualiteNiveau(r);
    }
    g.camMode = 0; g.render(t += 0.02);
    basse("police au contact");
    jusqua(0.21, 60 * 60);    // barrage annoncé 260 m avant (0,45 km) ; WFS-141 (RG-02) : 20 m après l'annonce (bandeau de 1,5 s visible)
    capture("poursuite-surprise");
    jusqua(0.93, 60 * 60);    // embouteillage à 0,9 km
    capture("poursuite-bouchon");
    basse("embouteillage");
    images(5 * 60, 10);
    capture("poursuite-hud");
    basse("HUD (police au contact, embouteillage)");
    // WFS-119 : ralentisseur puis camion en travers (score remis à 100 : l'embouteillage du banc l'a entamé)
    if (PoursuiteSystem* s0 = PoursuiteSystem::actif) { WFS_LOG("poursuite (banc) : score %d remis à %d", s0->etat().score, SCORE_DEPART); s0->bancScore(SCORE_DEPART); }
    // WFS-141 (RG-02) : le pilote automatique reste bloqué derrière le barrage (0,45 km) : saut à 150 m du ralentisseur
    g_bancSautKm = BANC_RALENTISSEUR_KM - 0.15; images(2, 1);
    WFS_LOG("poursuite (banc) : saut à %.2f km (ralentisseur à %.2f km)", g.player().dist * M / 1000, BANC_RALENTISSEUR_KM);
    jusqua(BANC_RALENTISSEUR_KM - 0.018, 60 * 60);   // WFS-141 (RG-02) : capture à 18 m de l'obstacle (15 à 20 m)
    capture("poursuite-ralentisseur");
    basse("ralentisseur");
    jusqua(BANC_RALENTISSEUR_KM + 0.03, 20 * 60);
    if (const PoursuiteSystem* s0 = PoursuiteSystem::actif) WFS_LOG("poursuite (banc) : après le ralentisseur : score %d, %d ralentisseur(s) passé(s) trop vite", s0->etat().score, s0->etat().ralentisseurs);
    jusqua(BANC_CAMION_KM - 0.018, 60 * 60);
    capture("poursuite-camion");
    basse("camion en travers");
    // WFS-338 (RG-01, ANO-191) : chantier sur la file de gauche : bandeau quand les cônes se voient (180 m), puis de près
    g_bancSautKm = BANC_TRAVAUX_KM - 0.16; images(20, 4);
    jusqua(BANC_TRAVAUX_KM - 0.03, 30 * 60);   // bandeau tenu, panneau AK5 tout près, cônes devant
    capture("poursuite-travaux");
    jusqua(BANC_TRAVAUX_KM + 0.005, 30 * 60);   // au biseau : cônes de près
    capture("poursuite-travaux-proche");
    // WFS-338 (RG-05, ANO-195, revue du lead dev), après les mesures d'appels de dessin (mêmes endroits que sur main) : au
    // départ, aucune police à moins de 160 m (toute la police repart à 220 m derrière, hors du rayon de dessin) ; elle revient
    // par la conduite normale, une image par seconde, en vue Arrière du jeu (caméra derrière la voiture) et en vue arrière du
    // banc (caméra devant, tournée vers l'arrière) ; distance dans l'encart POLICE de chaque image et au journal
    if (PoursuiteSystem* s = PoursuiteSystem::actif) {
        s->bancEloigner(g.player()); images(2, 1);
        WFS_LOG("poursuite (banc) : arrivée de la police : départ, plus proche à %.0f m (dessin jusqu'à %.0f m)", s->bancProche(g.player()), DESSIN_DERRIERE_M);
        for (int k = 1; k <= 10; k++) {
            if (k > 1) images(60, 6);
            char nom[64];
            WFS_LOG("poursuite (banc) : arrivée de la police, image %d : la plus proche à %.0f m", k, s->bancProche(g.player()));
            g.camMode = 2; g.render(t += 0.02); g.orbit = s->orbitAvant_ + 0.35f;
            std::snprintf(nom, sizeof nom, "poursuite-arrivee-%02d-arriere", k); capture(nom);
            g.camMode = 0; g.render(t += 0.02);
            std::snprintf(nom, sizeof nom, "poursuite-arrivee-%02d", k); capture(nom);
        }
    }
    g.camMode = 0; g.render(t += 0.02);
    g_bancFin = 1; images(90, 10);
    capture("poursuite-arrete");
    // RG-07 / RG-12 : vrais touchers sur l'écran de fin : « Rejouer » (nouvelle poursuite), saut à 150 m de l'arrivée
    // (victoire), puis « Menu » → accueil
    auto toucher = [&](const Zone& z, const char* nom) {
        const float x = z.x + z.w / 2, y = z.y + z.h / 2;
        g.onTouch({7, x, y, true}); images(2, 1); g.onTouch({7, x, y, false}); images(4, 2);
        WFS_LOG("poursuite (parcours) : toucher « %s » (%.0f, %.0f) → écran %s", nom, x, y, nomEcran(g.ecran()));
    };
    if (PoursuiteSystem* s = PoursuiteSystem::actif) toucher(s->rejouerZone(), "Rejouer");
    images(30, 10);
    const PoursuiteSystem* s = PoursuiteSystem::actif;
    WFS_LOG("poursuite (parcours) : après « Rejouer » : écran %s, score %d, %.2f km", nomEcran(g.ecran()), s ? s->etat().score : -1, g.player().dist * M / 1000);
    images(4 * 60, 8);
    g_bancFin = 2;
    const Course* c = activeCourse();
    for (int i = 0; i < 60 * 60 && c && c->state != RaceState::Over; i++) { g.update(1.0 / 60); c = activeCourse(); }
    images(90, 10);
    capture("poursuite-victoire");
    if (PoursuiteSystem* s2 = PoursuiteSystem::actif) toucher(s2->menuZone(), "Menu");
    images(10, 5);
    attendu(Ecran::Menu, "fin puis « Menu »");
    WFS_LOG("poursuite (parcours) : accueil → Course solo → Poursuite → Continuer → garage → Choisir → terrain → Rouler → poursuite → fin → Menu → accueil : %d écart(s) %s", ecarts, ecarts ? "ÉCHEC" : "OK");
    // WFS-119 RG-03 / RG-06 : redémarrage simulé : la sauvegarde écrite par « Continuer » (clé mode) est relue par un
    // nouveau lancement (mode par défaut : Checkpoint) ; « Course solo » → le panneau rouvre sur Poursuite (« Continuer »
    // sans toucher de carte → garage en Poursuite). Ancienne sauvegarde (sans « poursuite ») : Checkpoint.
    {
        Sauvegarde ecrite; ecrite.mode = cleMode(modeCourse());
        Sauvegarde relue; const bool lu = relue.lire(ecrite.texte());
        modeCourse() = ModeCourse::Checkpoint;   // nouveau lancement
        modeCourse() = modeDeCle(relue.mode);
        WFS_LOG("poursuite (parcours) : redémarrage : sauvegarde « mode=%s » relue (%s) → mode %s", relue.mode.c_str(), lu ? "valide" : "invalide", nomMode(modeCourse()));
        Sauvegarde ancienne; ancienne.lire("version=1\ncar=golf\n");
        Sauvegarde inconnue; inconnue.lire("version=1\nmode=helico\n");
        if (modeDeCle(ancienne.mode) != ModeCourse::Checkpoint || modeDeCle(inconnue.mode) != ModeCourse::Checkpoint) { ecarts++; WFS_ERR("poursuite (parcours) : ancienne sauvegarde : mode %s", nomMode(modeDeCle(ancienne.mode))); }
        else WFS_LOG("poursuite (parcours) : ancienne sauvegarde (sans clé mode, ou mode inconnu) → Checkpoint");
        choixModeBanc(true); ouvrirChoixMode(); images(10, 5);
        if (!choixModeOuvert()) { ecarts++; WFS_ERR("poursuite (parcours) : redémarrage : panneau des modes fermé"); }
        capture("poursuite-redemarrage");
        panneau("continuer");   // « Continuer », sans toucher de carte
        attendu(Ecran::Garage, "redémarrage, Course solo puis « Continuer »");
        if (modeCourse() != ModeCourse::Poursuite) { ecarts++; WFS_ERR("poursuite (parcours) : redémarrage : panneau sur %s, attendu Poursuite", nomMode(modeCourse())); }
        else WFS_LOG("poursuite (parcours) : redémarrage : le panneau des modes rouvre sur Poursuite");
        choixModeBanc(false);
        g.allerA(Ecran::Menu); images(10, 5);
        WFS_LOG("poursuite (parcours) : parcours complet et redémarrage : %d écart(s) %s", ecarts, ecarts ? "ÉCHEC" : "OK");
    }
    // WFS-119 RG-04 : même terrain, même heure, course Checkpoint : appels de dessin en Basse aux mêmes distances
    {
        modeCourse() = ModeCourse::Checkpoint;
        g.allerA(Ecran::Course); images(3 * 60, 8);
        int pire = -100000; double kmMesure = -1; unsigned nMesure = 0;
        for (const Mesure& ms : mesures) {
            jusqua(ms.km, 60 * 60);
            // WFS-144 : mesures de la poursuite au même endroit (bouchon) : Checkpoint mesuré une fois à cet endroit, pas 50 m plus loin
            const unsigned n = std::abs(ms.km - kmMesure) < 0.01 ? nMesure : (kmMesure = ms.km, nMesure = appelsBasse());
            const int ecart = (int)ms.appels - (int)n;
            pire = std::max(pire, ecart);
            WFS_LOG("poursuite (RG-04) : %s, %.2f km : poursuite %u, Checkpoint %u (%.2f km) : écart %+d", ms.ou, ms.km, ms.appels, n, g.player().dist * M / 1000, ecart);
        }
        WFS_LOG("poursuite (RG-04) : écart maximal en qualité Basse : %+d appels de dessin (cible ≤ 60) : %s", pire, pire <= 60 ? "OK" : "ÉCHEC");
    }
    // WFS-338 (revue du lead dev) : de jour puis de nuit : policier au volant et caisse blanche (gros plan, RG-06, RG-07, avec
    // l'avant du correctif de la peinture dans la même image), chantier et son bandeau (RG-01)
    for (const char* moment : {"jour", "nuit"}) {
        setenv("WFS_MOMENT", moment, 1);
        modeCourse() = ModeCourse::Poursuite;
        const bool jour = !std::strcmp(moment, "jour");
        if (g.init(r, "golf", "ville")) {
            g.allerA(Ecran::Course); g.setAutopilot(false); images(60, 10);   // décompte : la police attend derrière
            if (PoursuiteSystem* s = PoursuiteSystem::actif) {
                s->bancPoser(g.player(), -3.5, std::clamp(g.player().playerX - COTE_X, -BORD_X, BORD_X)); images(2, 1);
                char nom[64];
                g.camMode = 2; g.render(t += 0.02); g.orbit = s->orbitAvant_ + 0.45f;
                for (int avant = 1; avant >= 0; avant--) {   // même image : peinture métallisée d'origine, puis blanc uni
                    s->bancPeintureOrigine(avant == 1);
                    std::snprintf(nom, sizeof nom, "poursuite-police-gros-plan-%s%s", moment, avant ? "-avant" : "");
                    capture(nom);
                }
                s->bancPeintureOrigine(false);
                if (jour) { g.orbit = s->orbitAvant_ - 0.45f; g.render(t += 0.02); capture("poursuite-policier-jour"); }
            }
            if (jour) {
                g.camMode = 0; g.setAutopilot(true);
                for (int i = 0; i < 6 * 60; i++) { const Course* c = activeCourse(); if (c && c->state == RaceState::Play) break; g.update(1.0 / 60); }
                g_bancSautKm = BANC_TRAVAUX_KM - 0.16; images(20, 4);
                jusqua(BANC_TRAVAUX_KM - 0.03, 30 * 60);   // bandeau tenu, panneau AK5 tout près, cônes devant
                capture("poursuite-travaux-jour");
                jusqua(BANC_TRAVAUX_KM + 0.005, 30 * 60);   // au biseau : cônes de près
                capture("poursuite-travaux-jour-proche");
            }
            g.setAutopilot(false); g.allerA(Ecran::Menu); images(4, 2);
        }
        unsetenv("WFS_MOMENT");
    }
    g.setAutopilot(false);
    g_banc = false; g_bancFin = 0;
    modeCourse() = ModeCourse::Checkpoint;
    courseBenchRules(false);
    g.shutdown();
}

// ---------- WFS-460 (banc « police-bandes ») : bandes « POLICE » sur la tôle, une seule peinture blanche ----------
// Corniche (terrain « plage »), joueur à 30 km/h, 1re voiture de police tenue par le banc (poursuiteBancPolice). Chaque
// capture est faite deux fois dans la même scène, à la même heure : « -avant » (bandes de main : quadrilatère plat posé à la
// boîte de la voiture) puis sans suffixe (bandes posées sur la tôle). Captures : profil gauche et profil droit de jour, 3/4
// avant de nuit, puis approche de la police de 80 m à 5 m (une image toutes les 0,1 s), en vue Cockpit puis Poursuite, puis
// les deux en qualité Basse ; coût mesuré en Basse (appels de dessin, triangles, temps du rendu sur le processeur). Rend le nombre d'écarts.
int benchPoliceBandes(Game& g, Renderer& r, const std::string& out) {
    int ecarts = 0;
    const ModeCourse mode0 = modeCourse();
    const VueCamera vue0 = vueCamera();
    const int qualite0 = ecrans::reglages().qualite;
    constexpr double PAS = 1.0 / 60;
    double t = 1300;
    courseBenchRules(true);
    auto demarrer = [&](bool nuit, bool basse, VueCamera vue) {
        setenv("WFS_DEPART", "0", 1); setenv("WFS_MOMENT", nuit ? "nuit" : "jour", 1);
        modeCourse() = ModeCourse::Poursuite; choisirVue(vue);
        ecrans::reglages().qualite = basse ? ecrans::QUALITE_BASSE : qualite0; ecrans::appliquerQualiteNiveau(r);
        if (!g.init(r, "golf", "plage")) { WFS_ERR("police-bandes : chargement de la Corniche impossible"); ecarts++; return false; }
        g.allerA(Ecran::Course); g.modeManette = false; g.setAutopilot(true); g.camMode = 0;
        for (int i = 0; i < 4 * 60; i++) { g.update(PAS); t += PAS; if (i % 12 == 0) g.render(t); }   // décompte
        for (int i = 0; i < 60 * 25 && g.player().kmh() < 29; i++) { g.setAutopilot(g.player().kmh() < 30); g.update(PAS); t += PAS; if (i % 12 == 0) g.render(t); }
        return true;
    };
    auto rouler = [&](int n) { for (int i = 0; i < n; i++) { g.setAutopilot(g.player().kmh() < 30); g.update(PAS); t += PAS; } };
    // caméra « vitrine » autour du joueur : point de vue à avantM devant et droiteM à droite de sa voiture, à hauteur m
    auto viser = [&](double avantM, double droiteM, float hauteur) {
        const Game::Vue v = g.calculerVue(t);
        const vec3 f = normalize(vec3{v.fwd.x, 0, v.fwd.z}), d = cross(f, {0, 1, 0});
        const vec3 o = f * (float)avantM + d * (float)droiteM;
        g.orbit = std::atan2(o.x, o.z); g.orbitRecul = length(o); g.orbitHauteur = hauteur;
    };
    // même image deux fois : bandes de main (« -avant »), puis bandes posées sur la tôle
    auto paire = [&](const std::string& nom) {
        for (int avant = 1; avant >= 0; avant--) {
            g_bancBandesAvant = avant == 1; r.bancPeintureFineMain = avant == 1;
            g.render(t); g.render(t);
            r.capturePng(out + "/" + nom + (avant ? "-avant" : "") + ".png");
        }
        g_bancBandesAvant = false; r.bancPeintureFineMain = false;
        const PoursuiteSystem* s = PoursuiteSystem::actif;
        WFS_LOG("capture %s : %.0f km/h, police la plus proche à %.1f m, %u appels de dessin", nom.c_str(), g.player().kmh(), s ? s->bancProche(g.player()) : -1.0, r.appelsDessin());
    };
    // ---------- 1. et 2. profils de jour : police à gauche (flanc gauche vu de la gauche), puis à droite ----------
    for (int cote : {-1, 1}) {
        if (!demarrer(false, false, VueCamera::Poursuite)) break;
        const double px = std::clamp(g.player().playerX, -(BORD_X - COTE_X), BORD_X - COTE_X);
        poursuiteBancPolice(true, 0.0, px + cote * COTE_X);
        rouler(40);
        g.camMode = 2;
        viser(0.0, cote * (COTE_X * RW + 5.2), 1.05f);   // perpendiculaire à la police, à hauteur des portières
        rouler(2);
        paire(cote < 0 ? "police-bandes-gauche-jour" : "police-bandes-droite-jour");
        poursuiteBancPolice(false, 0, 0); g.camMode = 0; g.setAutopilot(false); g.shutdown();
    }
    // ---------- 3. 3/4 avant de nuit (police à gauche) ----------
    if (demarrer(true, false, VueCamera::Poursuite)) {
        const double px = std::clamp(g.player().playerX, -(BORD_X - COTE_X), BORD_X - COTE_X);
        poursuiteBancPolice(true, 0.0, px - COTE_X);
        rouler(40);
        g.camMode = 2;
        viser(6.0, -(COTE_X * RW + 3.6), 1.25f);
        rouler(2);
        paire("police-bandes-3-4-nuit");
        poursuiteBancPolice(false, 0, 0); g.camMode = 0; g.setAutopilot(false); g.shutdown();
    }
    // ---------- 4. approche : police devant, de 80 m à 5 m, une image toutes les 0,1 s ; Cockpit, Poursuite, Poursuite en Basse ----------
    struct Approche { const char* nom; VueCamera vue; bool basse; };
    for (const Approche& a : {Approche{"cockpit", VueCamera::Cockpit, false}, Approche{"poursuite", VueCamera::Poursuite, false}, Approche{"basse", VueCamera::Poursuite, true}, Approche{"cockpit-basse", VueCamera::Cockpit, true}}) {
        if (!demarrer(false, a.basse, a.vue)) break;
        const double px = std::clamp(g.player().playerX, -(BORD_X - COTE_X), BORD_X - COTE_X);
        constexpr int N = 16;
        // coût sur la même image (rendu sur le processeur, lavapipe) : 0 = main (bandes plates, peinture très fine retirée en
        // Basse), 1 = bandes sur la tôle seules (RG-06 : un dessin de plus au plus par voiture de police), 2 = branche entière
        // (la carrosserie retrouvée en Basse : celle de la police et des voitures du trafic à peinture très fine)
        double ms[3] = {0, 0, 0}, appels[3] = {0, 0, 0}, tri[3] = {0, 0, 0};
        for (int k = 0; k < N; k++) {
            const double ecart = 80.0 - 75.0 * k / (N - 1);
            poursuiteBancPolice(true, ecart, px - COTE_X);
            rouler(6);   // 0,1 s
            char nom[96]; std::snprintf(nom, sizeof nom, "police-bandes-approche-%s-%02d", a.nom, k);
            for (int e = 0; e < 3; e++) {
                g_bancBandesAvant = e == 0; r.bancPeintureFineMain = e <= 1;
                g.render(t);
                const auto t0 = std::chrono::steady_clock::now();
                g.render(t);
                ms[e] += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
                uint32_t ap = 0, tr = 0; r.fileScene(ap, tr); appels[e] += ap; tri[e] += tr;
                if (e != 1) r.capturePng(out + "/" + nom + (e == 0 ? "-avant" : "") + ".png");
            }
            g_bancBandesAvant = false; r.bancPeintureFineMain = false;
            WFS_LOG("capture %s : police à %.1f m devant (voulu %.1f m), %.0f km/h", nom, -poursuiteBancProche(g.player()), ecart, g.player().kmh());
        }
        WFS_LOG("police-bandes (WFS-460, RG-06) : approche %s, moyenne par image : appels de dessin main %.1f, bandes sur la tôle %.1f (%+.1f), "
                "branche %.1f (%+.1f : carrosserie retrouvée en Basse) ; triangles main %.0f, bandes %.0f, branche %.0f ; rendu (processeur) main %.1f ms, "
                "bandes %.1f ms, branche %.1f ms", a.nom, appels[0] / N, appels[1] / N, (appels[1] - appels[0]) / N, appels[2] / N, (appels[2] - appels[1]) / N,
                tri[0] / N, tri[1] / N, tri[2] / N, ms[0] / N, ms[1] / N, ms[2] / N);
        if (appels[1] > appels[0] + N) { WFS_ERR("police-bandes (WFS-460, RG-06) : bandes : plus d'un dessin de plus par voiture de police"); ecarts++; }
        poursuiteBancPolice(false, 0, 0); g.setAutopilot(false); g.shutdown();
    }
    choisirVue(vue0); modeCourse() = mode0; courseBenchRules(false);
    ecrans::reglages().qualite = qualite0; ecrans::appliquerQualiteNiveau(r);
    unsetenv("WFS_DEPART"); unsetenv("WFS_MOMENT");
    WFS_LOG("police-bandes (WFS-460) : %d écart(s) %s", ecarts, ecarts ? "ÉCHEC" : "OK");
    return ecarts;
}

// ---------- WFS-119 RG-08 (ANO-53) : course à 5, « Rejouer » puis appui suivant (groupe course5) ----------
// Le doigt de « Rejouer » se relève dans la course suivante : avant la correction, le filtre d'appui de la course à 5 le
// gardait (relâcher ignoré hors écran de fin), et le premier appui de ce doigt sur l'écran de fin suivant était perdu.
int benchCourse5Rejouer(Game& g, Renderer& r, const std::string& out) {
    courseBenchRules(true);
    modeCourse() = ModeCourse::Course5; kmCourse5() = 2;
    if (!g.init(r, "golf", "ville")) { courseBenchRules(false); modeCourse() = ModeCourse::Checkpoint; kmCourse5() = 4; return 1; }
    double t = 900;
    int ecarts = 0;
    auto images = [&](int n, int tous) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); if (i % tous == 0) g.render(t += tous / 60.0); } };
    auto jusquaFin = [&]() {
        const Course* c = activeCourse();
        for (int i = 0; i < 150 * 60 && c && c->state != RaceState::Over; i++) { g.update(1.0 / 60); c = activeCourse(); }
        images(90, 10);
        return c && c->state == RaceState::Over;
    };
    // WFS-141 : place des boutons de l'écran de fin de la course à 5 lue sur l’écran dessiné (course5Bouton) et non plus recalculée
    const float W = (float)r.width(), H = (float)r.height();
    const int doigt = 0;   // Android : le premier doigt posé a toujours l'identifiant 0
    auto toucher = [&](float x, float y, const char* nom) {
        g.onTouch({doigt, x, y, true}); images(2, 1); g.onTouch({doigt, x, y, false}); images(4, 2);
        WFS_LOG("course à 5 (parcours) : toucher « %s » (%.0f, %.0f) → écran %s", nom, x, y, nomEcran(g.ecran()));
    };
    g.allerA(Ecran::Menu); images(10, 5);
    g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(true);
    if (!jusquaFin()) { ecarts++; WFS_ERR("course à 5 (parcours) : première course pas finie"); }
    float bx = 0, by = 0;
    if (!course5Bouton("rejouer", bx, by)) { ecarts++; WFS_ERR("course à 5 (parcours) : « Rejouer » absent"); }
    toucher(bx, by, "Rejouer");
    const Course* c = activeCourse();
    if (g.ecran() != Ecran::Course || !c || c->state == RaceState::Over) { ecarts++; WFS_ERR("course à 5 (parcours) : « Rejouer » → pas de nouvelle course"); }
    else WFS_LOG("course à 5 (parcours) : « Rejouer » → nouvelle course");
    g.setAutopilot(false);
    images(4 * 60, 10);   // décompte 3-2-1-GO
    const double v0 = g.player().kmh();
    g.onTouch({doigt, W * 0.89f, H * 0.74f, true}); images(90, 10);   // gaz tenu 1,5 s, même doigt
    const double v1 = g.player().kmh();
    g.onTouch({doigt, W * 0.89f, H * 0.74f, false}); images(2, 1);
    if (v1 < v0 + 20) { ecarts++; WFS_ERR("course à 5 (parcours) : gaz non pris (%.0f → %.0f km/h)", v0, v1); }
    else WFS_LOG("course à 5 (parcours) : appui suivant (gaz) pris : %.0f → %.0f km/h", v0, v1);
    g.setAutopilot(true);
    if (!jusquaFin()) { ecarts++; WFS_ERR("course à 5 (parcours) : deuxième course pas finie"); }
    {
        g.render(t += 0.02); g.render(t += 0.02); r.capturePng(out + "/course5-rejouer.png");
        WFS_LOG("capture course5-rejouer : écran de fin de la 2e course (après « Rejouer »)");
    }
    if (!course5Bouton("menu", bx, by)) { ecarts++; WFS_ERR("course à 5 (parcours) : « Menu » absent"); }
    toucher(bx, by, "Menu");
    if (g.ecran() != Ecran::Menu) { ecarts++; WFS_ERR("course à 5 (parcours) : « Menu » (même doigt que « Rejouer ») → écran %s, attendu menu", nomEcran(g.ecran())); }
    WFS_LOG("course à 5 (parcours) : fin → « Rejouer » → nouvelle course → gaz pris → fin → « Menu » → accueil : %d écart(s) %s", ecarts, ecarts ? "ÉCHEC" : "OK");
    g.setAutopilot(false);
    modeCourse() = ModeCourse::Checkpoint; kmCourse5() = 4;
    courseBenchRules(false);
    g.shutdown();
    return ecarts;
}

} // namespace wfs
