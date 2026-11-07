// WaForSpeed — piétons animés (WFS-19) : voir pietons.h.
#include "pietons.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <tuple>
#include "../core/log.h"
#include "../core/platform.h"   // WFS-199 : readAsset (sommets des personnages lus sur le processeur)
#include "../core/system.h"
#include "../game/course_system.h"
#include "../game/game.h"
#include "../game/track.h"
#include "../game/trace_cartes.h"   // WFS-179
#include "../render/squelette.h"
#include "zones.h"
#include "enseignes.h"   // WFS-71 : clients attablés, serviettes, baigneurs
#include "ville_quartiers.h"   // WFS-137 : piétons des passages zébrés

namespace wfs {
namespace plage { const std::vector<vie::Baigneur>& gens(const MapDef& map, int64_t len); }   // WFS-180 (plage.cpp)
namespace plage { bool vueBanc(); }   // WFS-216 (RG-02, accroche E11) : vue de banc de la Corniche (fête vue de la promenade)
namespace {

// valeurs du jeu web (people.js)
constexpr double PED_RANGE0 = -10, PED_RANGE1 = 115;   // tronçons (1,2 m) derrière / devant le joueur
constexpr double PED_WALK_SPEED = 1.1, PED_JOG_SPEED = 2.9;   // m/s des animations d'origine
constexpr float PED_LOD_M[2] = {16.f, 50.f};
constexpr int PED_FULL = 8;            // maillages complets
constexpr int PEOPLE_MAX = 220;        // life.js
// affichés : 8 au plus — chaque personne = 2 dessins (corps + coiffure) et le renderer en accepte SKIN_MAX_INSTANCES (16)
constexpr int AFFICHES = std::min(PED_FULL, SKIN_MAX_INSTANCES / 2);

// palettes de pedLook (people.js), codes sRGB
vec3 hx(unsigned h) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f}; }
const vec3 SKIN[] = {{1.04f, 1.02f, 1.0f}, {1.0f, 0.96f, 0.93f}, {0.93f, 0.84f, 0.76f}, {0.8f, 0.66f, 0.55f}, {0.6f, 0.45f, 0.36f}, {0.44f, 0.32f, 0.25f}};
const unsigned HAIR[] = {0x17110d, 0x2a1b12, 0x3e2817, 0x5b3b22, 0x8a6a3e, 0xb89a68, 0x5a2414, 0x7d7a76};
const unsigned TOPS[] = {0xf2f2ee, 0xe9e4d8, 0x1c2333, 0x2b4a7a, 0x8f1d24, 0x2f5a43, 0xd8b04a, 0x141416, 0x6a4b8c, 0xc96f3b, 0x7fa7c9, 0xb9b2a6, 0xd06a8a, 0x3b4048, 0x556b2f, 0xefe0c4};
const unsigned BOTTOMS[] = {0x2b3a55, 0x25324a, 0x3a4a66, 0x17181c, 0x2d2f33, 0x6b6f75, 0xc2b08c, 0x8a7a5c, 0xe8e4da, 0x4a3b2c, 0x1f2a3a};
const unsigned SHOES[] = {0xf0f0ec, 0xe6e3dc, 0x18181a, 0x2a2522, 0x5a3a22, 0x8a8f96, 0x1d2a44};
const unsigned SWIM[] = {0xd42a2f, 0x1d6fb8, 0xf2c230, 0x141416, 0x2f8f4f, 0xff6a00, 0xe2558a, 0x1aa3a3, 0xf4f4f0};
// coiffures (plages des .wfp, extras hairRanges du jeu web) : 3 par sexe
const char* const COIFFURES[2][3] = {{"Hair_SimpleParted", "Hair_Buzzed", "Hair_Beard"}, {"Hair_Long", "Hair_Buns", "Hair_BuzzedFemale"}};

struct Look {
    bool femme = false, swim = false; int hair = 0; float scale = 1;
    ApparencePersonnage app; const char* walk = "Walk_Loop"; const char* idle = "Idle_Loop";
};
struct Personne {
    double s = 0, lat = 0, v = 1.1, ph = 0, dodge = 0, y = 0, t = 0; int pose = 0, dir = 1;
    bool look_ = false; Look look; double rel = 0;
    bool assis = false, yawFixe = false, marche = false; float yawRel = 0; int feu = -1;   // WFS-71 : assis / orientation du décor ; WFS-137 : passage zébré (pose 5)
};

// pedLook (people.js) : tirée une fois, déterministe selon la place de la personne dans la liste
Look pedLook(const Personne& P, int i, bool beach) {
    Mulberry32 R((uint32_t)((int32_t)(P.s * 977) + i * 7919 + 13));
    auto r = [&]() { return (float)R.next(); };
    auto pick = [&](auto& a) { return a[(size_t)(r() * (float)(sizeof(a) / sizeof(a[0])))]; };
    Look L;
    L.swim = P.pose == 2 || (beach && P.lat > RW + 8) || (beach && P.pose == 0 && P.lat > 0 && r() < 0.55f);
    L.femme = r() < 0.5f;   // web : R() < 0.5 ? 'femme' : 'homme'
    const bool formal = !L.swim && r() < 0.18f;
    L.hair = (int)(r() * 3);
    L.app.peau = pick(SKIN); L.app.cheveux = hx(pick(HAIR));
    const unsigned topsF[] = {TOPS[2], TOPS[7], TOPS[13], TOPS[0]}, botF[] = {BOTTOMS[3], BOTTOMS[4], BOTTOMS[10]};
    L.app.haut = hx(formal ? pick(topsF) : pick(TOPS));
    L.app.bas = hx(formal ? pick(botF) : pick(BOTTOMS));
    L.app.chaussures = hx(formal ? SHOES[2] : pick(SHOES)); L.app.maillot = hx(pick(SWIM));
    const float sleeve = formal || r() < 0.3f ? 1.95f : (L.femme && r() < 0.25f ? 0.12f : 0.45f);
    const float legs = beach && !formal ? (r() < 0.6f ? 0.72f : 1.9f) : (r() < 0.18f ? 0.72f : 1.9f);
    L.app.vetement = {sleeve, legs, L.swim ? 0.f : 1.f, 0};
    L.walk = formal ? "Walk_Formal_Loop" : "Walk_Loop";
    L.idle = r() < 0.5f ? "Idle_Loop" : "Idle_Talking_Loop";
    r();   // sit (Sitting_*), tiré par le jeu web : ordre des tirages conservé
    L.scale = (L.femme ? 0.95f : 0.97f) + r() * 0.08f;
    return L;
}

class Pietons : public System {
public:
    const char* name() const override { return "pietons"; }

    void init(Renderer& r, const GameView& g) override {
        gens_.clear(); actif_ = false;
        if (!g.map || !g.track) return;
        const char* id = g.map->id;
        tenue::carte() = id;   // WFS-210 : tenue des conducteurs selon la carte
        ville_ = !std::strcmp(id, "ville"); plage_ = !std::strcmp(id, "plage");
        campagne_ = !std::strcmp(id, "campagne"); circuit_ = !std::strcmp(id, "circuit");
        if (!ville_ && !plage_ && !campagne_ && !circuit_) return;
        if (ville_) for (const ville::Promeneur& m : ville::promeneurs(*g.map, *g.track)) { Personne P; P.s = m.s; P.lat = m.lat; P.v = m.v; P.dir = m.dir; gens_.push_back(P); }   // WFS-176 : promeneurs avec chien (marche reproduite par la Ville)
        piste_ = g.track;   // WFS-179
        peupler(*g.map, (int64_t)g.track->segmentCount());
        {   // WFS-71 : clients des terrasses (placeShop), assis sur les serviettes (beachLife), baigneurs debout dans l'eau
            const int64_t L = (int64_t)g.track->segmentCount(); Personne P; P.assis = P.yawFixe = true;
            for (const vie::Client& c : vie::clientsTerrasses(*g.map, L)) { P.s = c.s; P.lat = c.lat; P.pose = 1; P.y = c.y; P.yawRel = c.yaw; gens_.push_back(P); }
            for (const vie::Serviette& v : vie::serviettes(*g.map, L)) { P.s = v.s + 0.4; P.lat = v.lat; P.pose = 2; P.y = vie::hauteurSable(v.lat) + 0.03; P.yawRel = 1.5708f + v.rot; gens_.push_back(P); }
            P.assis = false; for (const vie::Baigneur& b : vie::baigneurs(*g.map, L)) { P.s = b.s; P.lat = b.lat; P.pose = 3; P.y = -2.25; P.yawRel = b.yaw; gens_.push_back(P); }
            for (const vie::Baigneur& b : plage::gens(*g.map, L)) { P.s = b.s; P.lat = b.lat; P.pose = 4; P.y = vie::hauteurSable(b.lat) + 0.02; P.yawRel = b.yaw; gens_.push_back(P); }   // WFS-180 : foule des fêtes, joueurs
        }
        if (ville_) for (const ville::Passant& c : ville::passants(*g.map, *g.track)) {   // WFS-137 (RG-02) : piétons des passages zébrés
            Personne P; P.s = c.s; P.lat = c.lat; P.v = c.v; P.dir = c.dir; P.feu = c.feu; P.pose = 5; P.yawFixe = true; P.y = 0.15; gens_.push_back(P); }
        if (gens_.empty()) return;
        if (!chargerPersonnage(r, "people/homme.wfp", modeles_[0]) || !chargerPersonnage(r, "people/femme.wfp", modeles_[1])) {
            WFS_ERR("Piétons indisponibles (assets/people) : aucun piéton affiché");   // pas de silhouettes de repli
            gens_.clear(); return;
        }
        for (int k = 0; k < 2; k++) for (PoseSquelette& p : poses_[k]) preparerPose(modeles_[k], p);
        actif_ = true;
        WFS_LOG("Piétons : %zu personnes (%s)", gens_.size(), id);
    }

    void shutdown(Renderer& r) override {
        for (Personnage& m : modeles_) if (m.ok) detruirePersonnage(r, m);
        gens_.clear(); actif_ = false;
    }

    // les marcheurs avancent tous, même invisibles (updatePeopleReal) ; temps d'animation de chacun
    void update(double dt, const GameView& g) override {
        if (!actif_ || g.ecran != Ecran::Course) return;
        const double len = (double)g.track->segmentCount();
        for (Personne& P : gens_) {
            if (P.pose == 0) { P.s += P.dir * P.v / SEG_M * dt; if (P.s >= len) P.s -= len; if (P.s < 0) P.s += len; }
            if (P.pose == 5) ville::traverser(P.lat, P.dir, P.marche, P.yawRel, P.y, P.feu, P.v, dt);   // WFS-137 (RG-02) : au rouge des voitures
            const double rate = P.pose == 0 ? (P.v > 2.2 ? P.v / PED_JOG_SPEED : P.v / PED_WALK_SPEED) : 1.0;
            P.t += dt * rate / (P.look_ ? P.look.scale : 1.f);
        }
    }

    void render(Renderer& r, const GameView& g) override {
        if (!actif_ || (g.sceneDediee && !plage::vueBanc()) || !g.player) return;   // WFS-216 : la foule de la fête dans les vues du banc
        Track& tr = *g.track;
        const double len = (double)tr.segmentCount(), aP = g.player->position / SEG, aMod = std::fmod(std::fmod(aP, len) + len, len);
        // 1. les plus proches (devant compte plus que derrière), petit bonus pour ceux déjà affichés (pas de clignotement)
        int choisis[AFFICHES]; double cles[AFFICHES]; int n = 0;
        for (int i = 0; i < (int)gens_.size(); i++) {
            Personne& P = gens_[(size_t)i];
            double rel = P.s - aMod; if (rel > len / 2) rel -= len; if (rel < -len / 2) rel += len;
            P.rel = rel;
            if (rel < PED_RANGE0 || rel > PED_RANGE1) continue;
            const double sc = std::max(0.0, (rel >= 0 ? rel : -rel * 2.5) + std::fabs(P.lat) * 0.15 - (affiche_[i % 256] == i ? 6 : 0) - (P.pose == 5 ? 30 : 0));   // WFS-137 : passage zébré en priorité
            int k = n < AFFICHES ? n++ : AFFICHES;   // insertion dans les AFFICHES meilleurs (sans allocation)
            if (k == AFFICHES) { if (sc >= cles[AFFICHES - 1]) continue; k = AFFICHES - 1; }
            while (k > 0 && cles[k - 1] > sc) { cles[k] = cles[k - 1]; choisis[k] = choisis[k - 1]; k--; }
            cles[k] = sc; choisis[k] = i;
        }
        std::fill(std::begin(affiche_), std::end(affiche_), -1);
        int util[2] = {0, 0};
        const double carLat = g.player->playerX * RW;
        for (int k = 0; k < n; k++) {
            const int i = choisis[k];
            Personne& P = gens_[(size_t)i];
            affiche_[i % 256] = i;
            if (!P.look_) { P.look = pedLook(P, i, plage_); P.look_ = true; }
            const Look& L = P.look;
            const int sx = L.femme ? 1 : 0;
            if (util[sx] >= AFFICHES) continue;
            PoseSquelette& pose = poses_[sx][util[sx]++];
            const Personnage& M = modeles_[sx];
            // une voiture arrive sur le trottoir : la personne s'écarte vers les façades (dodge du jeu web)
            const double dt = std::clamp(g.time - tPrec_, 0.0, 0.1);
            if (std::fabs(P.rel) < 6 && std::fabs(carLat - P.lat) < 2.4) P.dodge = std::min(1.0, P.dodge + dt * 4); else P.dodge = std::max(0.0, P.dodge - dt * 0.5);
            const double side = P.lat >= 0 ? 1 : -1, lat = P.lat + side * P.dodge * 2.6;
            const TrackPoint tp = tr.point((aP + P.rel) * SEG, lat);
            const double gy = P.y != 0 ? P.y : sol(side);
            // le personnage regarde vers +Z dans son repère : face au sens de la route = demi-tour (repère natif)
            float yaw = (float)(-tp.th) + (P.yawFixe ? P.yawRel : (P.dir > 0 ? 3.14159265f : 0.f));
            const char* anim = P.assis ? "Sitting_Idle_Loop" : P.pose == 0 || P.marche ? (P.v * (1 + P.dodge * 0.6) > 2.2 ? "Jog_Fwd_Loop" : L.walk) : P.pose == 4 ? (i % 3 ? "Jog_Fwd_Loop" : "Idle_Talking_Loop") : L.idle;   // WFS-180 : on danse, on saute
            echantillonner(M, clipParNom(M, anim), P.t + P.ph / 6.283 * 1.333, pose);
            const mat4 monde = translate({(float)tp.x, (float)(tp.y + gy), (float)tp.z}) * rotateY(yaw) * scale({L.scale, L.scale, L.scale});
            const double dm = std::fabs(P.rel) * SEG_M;
            const int lod = dm < PED_LOD_M[0] && k < PED_FULL ? 0 : dm < PED_LOD_M[1] ? 1 : 2;
            ApparencePersonnage app = L.app; app.corps = M.corps;
            char nom[48];
            std::snprintf(nom, sizeof nom, "corps-%d", lod);
            const Plage* corps = plageParNom(M, nom);
            std::snprintf(nom, sizeof nom, "%s-%d", COIFFURES[sx][L.hair % 3], lod);
            const Plage* coiffure = plageParNom(M, nom);
            for (const Plage* pl : {corps, coiffure})
                if (pl) r.drawSkinned(M.maillage, M.materiau, monde, pose.palette.data(), (int)pose.palette.size(), pl->premier, pl->nombre, &app);
        }
        tPrec_ = g.time;
    }

private:
    // hauteur du sol des piétons (GROUND du jeu web) : trottoir de la ville 0,15 m, promenade de la plage 0,1 m
    double sol(double) const { return ville_ ? 0.15 : 0.1; }

    // addPerson (life.js) : direction, vitesse et phase tirées à la graine (le jeu web utilise Math.random)
    void ajouter(double s, double lat, int pose, Mulberry32& A, double v = 0) {
        if ((int)gens_.size() >= PEOPLE_MAX) return;
        Personne P; P.s = s; P.lat = lat; P.pose = pose;
        P.dir = A.next() < 0.5 ? -1 : 1; P.v = v > 0 ? v : 1.1 + A.next() * 0.6; P.ph = A.next() * 6.28;
        gens_.push_back(P);
    }

    // populatePeople (life.js), même graine mulberry32(seed × 13 + 5) et mêmes tirages
    void peupler(const MapDef& map, int64_t len) {
        Mulberry32 R(map.seed * 13 + 5), A(map.seed * 31 + 11);
        auto rnd = [&]() { return R.next(); };
        for (int64_t n = 0; n < len; n += 3) {
            const char* z = zoneId(map, n, len);
            auto walk = [&](int side, double a, double b, double p) {
                if (rnd() < p) { const double s = n + rnd() * 3; ajouter(s, side * (a + rnd() * (b - a)), 0, A); }
            };
            if (ville_) {
                if (!std::strcmp(z, "port")) { walk(-1, RW + 1.5, RW + 4.5, 0.06); walk(1, RW + 1.5, RW + 4.5, 0.06); continue; }
                const double k = !std::strcmp(z, "centre") ? 0.55 : !std::strcmp(z, "parc") ? 0.35 : 0.4;
                walk(-1, RW + 1.4, RW + 3.2, k); walk(1, RW + 1.4, RW + 3.2, k);
                // joggeurs du parc : côté du parc (parkSide du décor web, absent en natif) tiré à la graine
                if (!std::strcmp(z, "parc") && rnd() < 0.3) { const double s2 = rnd(); ajouter((double)n, (A.next() < 0.5 ? -1 : 1) * (RW + 9 + s2 * 30), 0, A, 2.6); }
            } else if (plage_) {
                walk(1, RW + 1, RW + 4.5, !std::strcmp(z, "rochers") ? 0.1 : 0.5);
                walk(-1, RW + 0.8, RW + 2.6, !std::strcmp(z, "village") || !std::strcmp(z, "marina") ? 0.35 : 0.15);
            } else if (campagne_ && !std::strcmp(z, "village") && !(piste_ && trace::sectionA(*piste_, (double)n * SEG) && trace::sectionA(*piste_, (double)n * SEG)->autoroute)) {   // WFS-179 : personne à pied sur l'autoroute
                walk(-1, RW + 0.3, RW + 1.4, 0.25); walk(1, RW + 0.3, RW + 1.4, 0.25);
            } else if (circuit_ && !std::strcmp(z, "paddock")) {
                if (rnd() < 0.35) { const double s2 = rnd(); ajouter((double)n, (A.next() < 0.5 ? -1 : 1) * (RW + 5 + s2 * 6), 3, A); }
            }
        }
    }

    std::vector<Personne> gens_;
    Personnage modeles_[2];   // 0 homme, 1 femme
    PoseSquelette poses_[2][AFFICHES];
    int affiche_[256];
    double tPrec_ = 0;
    bool actif_ = false, ville_ = false, plage_ = false, campagne_ = false, circuit_ = false;
    const Track* piste_ = nullptr;   // WFS-179
};

} // namespace

WFS_SYSTEM(Pietons, 106)   // décor (100-199) : après la ville (100) et la plage (105)

void benchPietons(Game& g, Renderer& r, const std::string& out) {
    for (const char* carte : {"ville", "plage"}) {
        if (!g.init(r, "golf", carte)) { WFS_ERR("piétons : chargement %s impossible", carte); continue; }
        g.setAutopilot(true); g.camMode = 1;
        double t = 1100;
        for (int i = 0; i < 180; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 0) g.render(t); }
        auto capture = [&](const std::string& nom) {
            g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + ".png");
            WFS_LOG("capture %s : %.0f km/h", nom.c_str(), g.player().kmh());
        };
        if (!std::strcmp(carte, "ville")) {
            capture("pietons-ville-0");
            for (int i = 0; i < 24; i++) { g.update(1.0 / 60); t += 1.0 / 60; }   // 0,4 s : autre instant de la marche
            capture("pietons-ville-1");
        } else capture("pietons-plage");
        g.shutdown();
    }
    // WFS-210 (RG-01) : conducteur vu de côté (vitrine à 6,5 m) sur les 4 cartes, de jour ; Ville aussi de nuit : tenue du lieu,
    // peau, cheveux et yeux (conducteurs-tenue-<carte>[-nuit].png) ; tenue tirée au journal
    struct Cas { const char* carte; const char* moment; };
    for (const Cas& c : {Cas{"ville", "jour"}, Cas{"plage", "jour"}, Cas{"campagne", "jour"}, Cas{"circuit", "jour"}, Cas{"ville", "nuit"}}) {
        setenv("WFS_MOMENT", c.moment, 1);
        if (!g.init(r, "golf", c.carte)) { WFS_ERR("tenues : chargement %s impossible", c.carte); continue; }
        g.setAutopilot(false); g.camMode = 2; g.orbit = 1.5708f;   // de côté, comme le banc des conducteurs (WFS-73)
        double t = 1300;
        for (int i = 0; i < 60; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 0) g.render(t); }
        g.render(t); g.render(t + 0.01);
        const std::string nom = std::string("conducteurs-tenue-") + c.carte + (std::strcmp(c.moment, "nuit") ? "" : "-nuit");
        r.capturePng(out + "/" + nom + ".png");
        ApparencePersonnage a; tenue::tenueLieu(c.carte, 1, false, a);
        WFS_LOG("capture %s : tenue du lieu %s (tissu %d, manches %.2f, jambes %.2f)", nom.c_str(), tenue::carteCourante(), (int)a.vetement.w, a.vetement.x, a.vetement.y);
        g.shutdown();
    }
    unsetenv("WFS_MOMENT");
}

// ---------------------------------------------------------------- WFS-199 : spectateurs (foule allégée, posée une fois)
namespace foule {
namespace {

// sommets, indices, plages, os, palette et clips d'un .wfp (version 2, même lecture que chargerPersonnage) : rien n'est
// envoyé au processeur graphique ; les textures sont sautées (le décor peint des couleurs unies)
struct Brut { std::vector<SkinVertex> v; std::vector<uint32_t> idx; Personnage p; };

bool lire(const char* chemin, Brut& b) {
    std::vector<uint8_t> f;
    if (!readAsset(chemin, f)) return false;
    size_t o = 0; bool ok = true;
    auto prendre = [&](size_t n) -> const uint8_t* { if (!ok || o + n > f.size()) { ok = false; return nullptr; } const uint8_t* q = f.data() + o; o += n; return q; };
    auto u32 = [&]() { uint32_t v = 0; if (const uint8_t* q = prendre(4)) std::memcpy(&v, q, 4); return v; };
    auto f32 = [&]() { float v = 0; if (const uint8_t* q = prendre(4)) std::memcpy(&v, q, 4); return v; };
    auto nom = [&]() { const uint8_t* q = prendre(32); return q ? std::string((const char*)q, strnlen((const char*)q, 32)) : std::string(); };
    const uint8_t* magie = prendre(4);
    if (!magie || std::memcmp(magie, "WFP1", 4) != 0 || u32() != 2) return false;
    for (uint32_t t = 0, n = u32(); ok && t < n; t++) prendre(u32());
    f32(); f32(); b.p.hauteur = f32();
    b.p.corps = {f32(), f32(), f32(), f32()};
    const uint32_t nV = u32(), nI = u32(), nP = u32(), nO = u32(), nC = u32();
    if (!ok || nV > 200000 || nI > 600000 || nO > 256) return false;
    b.v.resize(nV); b.idx.resize(nI);
    if (const uint8_t* q = prendre((size_t)nV * sizeof(SkinVertex))) std::memcpy(b.v.data(), q, (size_t)nV * sizeof(SkinVertex));
    if (const uint8_t* q = prendre((size_t)nI * 4)) std::memcpy(b.idx.data(), q, (size_t)nI * 4);
    b.p.plages.resize(nP);
    for (Plage& pl : b.p.plages) { pl.nom = nom(); pl.premier = u32(); pl.nombre = u32(); }
    b.p.os.resize(nO);
    for (OsSquelette& os : b.p.os) {
        os.nom = nom(); os.parent = (int32_t)u32();
        float v[26]; for (float& x : v) x = f32();
        os.t = {v[0], v[1], v[2]}; os.r = {v[3], v[4], v[5], v[6]}; os.s = {v[7], v[8], v[9]};
        std::memcpy(os.bindInverse.m, v + 10, 64);
    }
    { const uint32_t nPal = u32(); if (!ok || nPal > (uint32_t)SKIN_MAX_OS) return false; b.p.palette.resize(nPal); }
    for (int& x : b.p.palette) x = (int)u32();
    b.p.clips.resize(nC);
    for (ClipAnim& c : b.p.clips) {
        c.nom = nom(); c.duree = f32();
        c.canaux.resize(ok ? std::min<uint32_t>(u32(), 4096) : 0);
        for (CanalAnim& k : c.canaux) {
            k.os = (int)u32(); k.chemin = (int)u32();
            const uint32_t n = u32();
            if (!ok || n > 100000) { ok = false; break; }
            k.temps.resize(n); k.valeurs.resize((size_t)n * (k.chemin == 1 ? 4 : 3));
            for (float& x : k.temps) x = f32();
            for (float& x : k.valeurs) x = f32();
        }
    }
    for (size_t i = 0; ok && i < b.p.os.size(); i++) ok = b.p.os[i].parent < (int)i;
    for (int x : b.p.palette) ok = ok && x >= 0 && x < (int)nO;
    for (const ClipAnim& c : b.p.clips) for (const CanalAnim& k : c.canaux) ok = ok && k.os >= 0 && k.os < (int)nO && k.chemin >= 0 && k.chemin <= 2;
    for (uint32_t i : b.idx) ok = ok && i < nV;
    for (const SkinVertex& v : b.v) for (int k = 0; k < 4; k++) ok = ok && v.os[k] < b.p.palette.size();
    return ok;
}

vec3 point(const mat4& m, vec3 p) {
    return {m.at(0, 0) * p.x + m.at(0, 1) * p.y + m.at(0, 2) * p.z + m.at(0, 3), m.at(1, 0) * p.x + m.at(1, 1) * p.y + m.at(1, 2) * p.z + m.at(1, 3),
            m.at(2, 0) * p.x + m.at(2, 1) * p.y + m.at(2, 2) * p.z + m.at(2, 3)};
}
vec3 direction(const mat4& m, vec3 p) {
    return {m.at(0, 0) * p.x + m.at(0, 1) * p.y + m.at(0, 2) * p.z, m.at(1, 0) * p.x + m.at(1, 1) * p.y + m.at(1, 2) * p.z,
            m.at(2, 0) * p.x + m.at(2, 1) * p.y + m.at(2, 2) * p.z};
}

// zone d'un sommet : bloc « WFS-72 » de scene_skin.frag (pedMaterial du jeu web), avec manches `manche` et bas long (1,9)
Zone zoneDe(const SkinVertex& v, const vec4& corps, float manche) {
    const float part = v.cloth[3];
    if (part > 0.5f) return CHEVEUX;   // 1 yeux, 2 cheveux et sourcils : sombres
    const float arm = v.cloth[0], y = v.cloth[1], leg = v.cloth[2], jambe = 1.9f;
    const bool isArm = arm >= -0.5f, isLeg = leg >= -0.5f;
    const float hd = std::sqrt(v.px * v.px + (v.pz - corps.z) * 1.15f * (v.pz - corps.z) * 1.15f), ny = y - corps.x;
    const bool col = !(ny + 0.05f >= 0 && hd <= 0.085f + std::max(0.f, -ny) * 0.4f);
    const bool haut = col && ny <= 0.05f && ((!isArm && !isLeg && y >= corps.y) || (isArm && arm <= manche));
    const bool bas = !isArm && y <= corps.y + 0.001f && !(isLeg && leg >= jambe);
    if (isLeg && leg >= 1.93f) return CHAUSSURES;
    return haut ? HAUT : bas ? BAS : PEAU;
}

const char* const CLIPS[NB_POSES] = {"Sitting_Idle_Loop", "Sitting_Talking_Loop", "Idle_Loop", "Idle_Talking_Loop"};
const char* const COIFF[2][COIFFURES] = {{"Hair_SimpleParted", "Hair_Buzzed", "Hair_Beard"}, {"Hair_Long", "Hair_Buns", "Hair_BuzzedFemale"}};

// personnage allégé posé : plages « corps-2 » et coiffure « -2 » (version la plus légère du .wfp), sommets regroupés sur une
// grille de `cellule` m en pose de liaison (un représentant par case et par partie : cheveux et peau ne fusionnent pas)
Silhouette construire(const Brut& B, int sx, int coiffure, Pose pose, int variante, float cellule) {
    Silhouette S;
    std::map<std::tuple<int, int, int, int>, uint32_t> cases;
    std::vector<uint32_t> rep;   // sommet d'origine de chaque sommet de la silhouette
    for (const std::string& nom : {std::string("corps-2"), std::string(COIFF[sx][coiffure]) + "-2"}) {
        const Plage* pl = plageParNom(B.p, nom);
        if (!pl) continue;
        for (uint32_t t = pl->premier; t + 2 < pl->premier + pl->nombre && t + 2 < B.idx.size(); t += 3) {
            uint32_t tri[3];
            for (int k = 0; k < 3; k++) {
                const SkinVertex& v = B.v[B.idx[t + k]];
                const auto cle = std::make_tuple((int)std::floor(v.px / cellule), (int)std::floor(v.py / cellule), (int)std::floor(v.pz / cellule), (int)(v.cloth[3] + 0.5f));
                auto it = cases.find(cle);
                if (it == cases.end()) { it = cases.emplace(cle, (uint32_t)rep.size()).first; rep.push_back(B.idx[t + k]); }
                tri[k] = it->second;
            }
            if (tri[0] == tri[1] || tri[1] == tri[2] || tri[0] == tri[2]) continue;   // triangle écrasé par le regroupement
            S.i.insert(S.i.end(), {tri[0], tri[1], tri[2]});
        }
    }
    PoseSquelette ps; preparerPose(B.p, ps);
    const int clip = clipParNom(B.p, CLIPS[pose]);
    const double duree = clip >= 0 ? B.p.clips[(size_t)clip].duree : 0;
    echantillonner(B.p, clip, duree * (variante + 0.37 * sx) / VARIANTES, ps);
    const float manche = variante == 1 ? 1.95f : 0.45f;   // manches longues pour une variante sur trois
    float bas = 1e9f, fesses = 1e9f, pieds = 1e9f;
    for (uint32_t o : rep) {
        const SkinVertex& v = B.v[o];
        vec3 p{}, n{};
        for (int k = 0; k < 4; k++) if (v.poids[k] > 0) {
            const mat4& m = ps.palette[v.os[k]];
            p += point(m, {v.px, v.py, v.pz}) * v.poids[k];
            n += direction(m, {v.nx, v.ny, v.nz}) * v.poids[k];
        }
        S.v.push_back({p, normalize(n), zoneDe(v, B.p.corps, manche)});
        bas = std::min(bas, p.y);
        if (v.cloth[3] < 0.5f && v.cloth[0] < -0.5f && v.cloth[2] < 0.8f) fesses = std::min(fesses, p.y);   // tronc et haut des cuisses
        if (v.cloth[2] >= 1.8f) pieds = std::min(pieds, p.y);
    }
    // repère : assis, fesses en y = 0 et centre du bassin en (0, 0) ; debout, pieds en y = 0, centré
    const bool assis = pose == ASSIS_REPOS || pose == ASSIS_PARLE;
    const float y0 = assis && fesses < 1e8f ? fesses : bas;
    vec3 c{}; int nc = 0;
    for (size_t k = 0; k < S.v.size(); k++) {
        const SkinVertex& v = B.v[rep[k]];
        const bool bassin = assis ? (v.cloth[3] < 0.5f && v.cloth[0] < -0.5f && S.v[k].p.y < y0 + 0.08f) : true;
        if (bassin) { c += S.v[k].p; nc++; }
    }
    if (nc) c = c * (1.f / (float)nc);
    for (Sommet& s : S.v) s.p = s.p - vec3{c.x, y0, c.z};
    if (assis && pieds < 1e8f) S.assise = std::clamp(y0 - pieds, 0.3f, 0.6f);
    return S;
}

struct Cache { std::vector<Silhouette> s; bool ok = false; };
int indice(int sx, int coiffure, int pose, int variante, int proche) { return (((sx * COIFFURES + coiffure) * NB_POSES + pose) * VARIANTES + variante) * 2 + proche; }

Cache batir() {
    Cache c;
    Brut b[2];
    if (!lire("people/homme.wfp", b[0]) || !lire("people/femme.wfp", b[1])) { WFS_ERR("Spectateurs (WFS-199) : personnages illisibles (assets/people)"); return c; }
    c.s.resize((size_t)indice(1, COIFFURES - 1, NB_POSES - 1, VARIANTES - 1, 1) + 1);
    size_t tri[2] = {0, 0};
    for (int sx = 0; sx < 2; sx++) for (int h = 0; h < COIFFURES; h++) for (int p = 0; p < NB_POSES; p++) for (int v = 0; v < VARIANTES; v++)
        for (int pr = 0; pr < 2; pr++) {
            Silhouette& S = c.s[(size_t)indice(sx, h, p, v, pr)];
            S = construire(b[sx], sx, h, (Pose)p, v, pr ? 0.10f : 0.16f);
            tri[pr] = std::max(tri[pr], S.i.size() / 3);
        }
    c.ok = true;
    WFS_LOG("Spectateurs (WFS-199) : %zu silhouettes (homme, femme ; 4 poses ; %d instants), %zu triangles au plus de près, %zu de loin ; assise %.2f m",
            c.s.size(), VARIANTES, tri[1], tri[0], c.s[(size_t)indice(0, 0, ASSIS_REPOS, 0, 1)].assise);
    return c;
}

} // namespace

const Silhouette* silhouette(bool femme, int coiffure, Pose pose, int variante, bool proche) {
    static const Cache c = batir();   // une fois, au premier appel (initialisation sûre entre fils : morceaux construits en parallèle)
    if (!c.ok) return nullptr;
    return &c.s[(size_t)indice(femme ? 1 : 0, ((coiffure % COIFFURES) + COIFFURES) % COIFFURES, pose, ((variante % VARIANTES) + VARIANTES) % VARIANTES, proche ? 1 : 0)];
}

} // namespace foule

} // namespace wfs
