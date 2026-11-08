// WaForSpeed — piétons animés (WFS-19) : voir pietons.h.
#include "../ui/ecrans/reglages.h"   // WFS-299 : banc des apparitions en qualité Basse
#include "../core/memoire.h"   // WFS-297 (RG-06)
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
#include "foule_familles.h"   // WFS-233 : enfants et familles
#include "foule_poussette.h"   // WFS-240 : poussettes
#include "foule_apparition.h"  // WFS-257 : apparition naturelle
#include "vegetation.h"        // WFS-240 : géométrie (veg::Geo)
#include "enseignes.h"   // WFS-71 : clients attablés, serviettes, baigneurs
#include "ville_quartiers.h"   // WFS-137 : piétons des passages zébrés

namespace wfs {
namespace plage { const std::vector<vie::Baigneur>& gens(const MapDef& map, int64_t len); }   // WFS-180 (plage.cpp)
namespace plage { bool vueBanc(); }   // WFS-216 (RG-02, accroche E11) : vue de banc de la Corniche (fête vue de la promenade)
namespace {

// valeurs du jeu web (people.js)
// WFS-257 (RG-02) : portée de 48 m derrière (au lieu de 12 m : la caméra Poursuite voit derrière la voiture ; hors du cadre,
// une personne derrière ne coûte rien) à 138 m devant (jeu web), fondu sur les 20 derniers mètres
constexpr double PED_RANGE0 = -40, PED_RANGE1 = 115;   // tronçons (1,2 m) derrière / devant le joueur
constexpr double PED_WALK_SPEED = 1.1, PED_JOG_SPEED = 2.9;   // m/s des animations d'origine
constexpr float PED_LOD_M[3] = {16.f, 50.f, 1e9f};   // Moyenne et Haute : pas de niveau 3 (WFS-299 : inchangées)
// WFS-222 : Basse (corps complet 14-15 k triangles, moyen 5,5 k, loin 1,9 k) ; WFS-299 (RG-01, décision du PO : « très simple »
// au-delà de 25-30 m) : moyen jusqu'à 20 m, loin de 20 à 30 m, puis niveau 3 « très simple » (≈ 0,8 k avec la coiffure, fabriqué au
// chargement par squelette.cpp à partir du niveau 2)
constexpr float PED_LOD_BASSE_M[3] = {8.f, 20.f, 30.f};
constexpr float PED_LOD_BASSE_AVANT_M[3] = {8.f, 30.f, 1e9f};   // banc seulement (WFS_PIETONS_AVANT) : seuils d'avant WFS-299, pour les captures avant / après
constexpr int PEOPLE_MAX = 220;        // life.js
// WFS-257 (cause d'ANO-130) : les 8 places de WFS-19 (8 piétons × 2 dessins) faisaient apparaître et disparaître les gens
// sous les yeux du joueur à chaque changement des « 8 plus proches ». Désormais toute personne dans le cadre est dessinée :
// INST_PIETONS instances (corps + coiffure = 2) pour les nouvelles venues, celles déjà affichées gardent toujours leur place ;
// SKIN_MAX_INSTANCES (renderer.h) = 32 des conducteurs (WFS-73) + piétons
constexpr int INST_PIETONS = 120;   // 60 personnes : la Ville en montre jusqu'à 40 dans le cadre (banc WFS-257), marge pour les fondus croisés
constexpr int DESSINES = 64;           // personnes dessinées par image au plus (tableaux)
constexpr int POSES_ENFANT = 8;        // instances par morphotype d'enfant et par image (WFS-233 : 2)
static_assert(INST_PIETONS + 32 <= SKIN_MAX_INSTANCES, "WFS-257 : places des piétons et des conducteurs");

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
    double latMin = 0, latMax = 0;   // WFS-233 : bande du trottoir où la personne a été tirée (|lat|)
    int enfant = -1, famille = -1;   // WFS-233 : morphotype d'enfant (familles::ENFANTS) ; autre membre de la famille (indice)
    bool poussette = false;          // WFS-240 (RG-01) : enfant assis dans la poussette que pousse son parent (pas de marche)
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

// WFS-233 : une personne prête à dessiner (pose échantillonnée, place dans le monde) : les mains des familles se règlent entre
// l'échantillonnage et le dessin
struct Dessin {
    const Personnage* M = nullptr; PoseSquelette* pose = nullptr; mat4 monde = mat4::identity();
    float yaw = 0, echelle = 1; vec3 pos{}; ApparencePersonnage app; int lod = 0, i = -1; const char* coiffure = nullptr;
    float cheveuxLongs = 0, cheveuxDy = 0;   // WFS-240 (RG-03) : fille aux cheveux longs : échelle et décalage de la coiffure, 0 sinon
    int lodPrec = -1; float lodT = 1, op = 1;   // WFS-257 : fondu croisé des niveaux de détail, opacité (RG-03)
};
vec3 transformer(const mat4& m, const vec3& p) {
    return {m.at(0, 0) * p.x + m.at(0, 1) * p.y + m.at(0, 2) * p.z + m.at(0, 3), m.at(1, 0) * p.x + m.at(1, 1) * p.y + m.at(1, 2) * p.z + m.at(1, 3),
            m.at(2, 0) * p.x + m.at(2, 1) * p.y + m.at(2, 2) * p.z + m.at(2, 3)};
}
// point du monde → repère du modèle d'une personne (monde = translation × rotation Y × échelle uniforme)
vec3 versModele(const Dessin& D, const vec3& p) {
    const vec3 q = p - D.pos;
    const float c = std::cos(D.yaw), s = std::sin(D.yaw);
    return vec3{c * q.x - s * q.z, q.y, s * q.x + c * q.z} * (1.f / D.echelle);
}
struct Bras { int epaule = -1, coude = -1, main = -1; };
Bras bras(const Personnage& M, bool gauche) {
    const char* c = gauche ? "_l" : "_r";
    return {osParNom(M, std::string("upperarm") + c), osParNom(M, std::string("lowerarm") + c), osParNom(M, std::string("hand") + c)};
}
vec3 epauleMonde(const Dessin& D, const Bras& b) { return b.epaule < 0 ? D.pos : transformer(D.monde, transformer(D.pose->modele[(size_t)b.epaule], {0, 0, 0})); }

// WFS-233 : main dans la main (avis expert-realisme-3d) : point de prise commun entre les deux épaules qui se font face, à
// mi-hauteur entre « épaule de l'adulte − 0,62 m » (bras presque tendu) et « épaule de l'enfant − 0,10 m » (bras levé) :
// ≈ 0,76 m pour un enfant de 1,02 m, ≈ 0,84 m pour 1,22 m ; chaque bras l'atteint par IK à 2 os (coude dans son plan d'origine)
void tenirLaMain(Dessin& A, Dessin& E) {
    if (!A.M || !E.M || !A.pose || !E.pose) return;
    const Bras aG = bras(*A.M, true), aD = bras(*A.M, false), eG = bras(*E.M, true), eD = bras(*E.M, false);
    auto d2 = [](const vec3& p, const vec3& q) { return (p.x - q.x) * (p.x - q.x) + (p.z - q.z) * (p.z - q.z); };
    const Bras& ba = d2(epauleMonde(A, aG), E.pos) < d2(epauleMonde(A, aD), E.pos) ? aG : aD;
    const Bras& be = d2(epauleMonde(E, eG), A.pos) < d2(epauleMonde(E, eD), A.pos) ? eG : eD;
    const vec3 sa = epauleMonde(A, ba), se = epauleMonde(E, be);
    const float sol = std::min(A.pos.y, E.pos.y);
    const float h = 0.5f * (sa.y - sol - 0.62f) + 0.5f * (se.y - sol - 0.10f);
    const vec3 prise{(sa.x + se.x) * 0.5f, sol + h, (sa.z + se.z) * 0.5f};
    atteindre(*A.M, *A.pose, ba.epaule, ba.coude, ba.main, versModele(A, prise));
    atteindre(*E.M, *E.pose, be.epaule, be.coude, be.main, versModele(E, prise));
}

// WFS-233 (banc d'essai seulement) : familles de 4 et 7 ans placées sur le trottoir de droite, devant la voiture à l'arrêt
bool g_bancFamilles = false;
bool g_bancCheveux = false;   // WFS-240 (banc) : une fille aux cheveux longs de 7 ou 10 ans et son parent, à 6 m devant la voiture

// WFS-257 (RG-01, RG-06) : mesure des apparitions, disparitions, sauts et changements de détail visibles (banc seulement).
// « Visible » : centre du corps dans l'image (caméra de l'image) et personne haute d'au moins 1 % de la hauteur de l'écran
// (≈ 11 px sur un téléphone de 1 080 px : en deçà, elle ne se distingue plus du décor). Une apparition ou une disparition
// visible : l'opacité varie plus vite qu'un fondu de 0,4 s (RG-03 : 0,5 s, marge de 20 %) ; un saut : plus de 10 m/s d'une
// image à l'autre (marche 1,1 m/s, course 2,9 m/s) ; un détail brutal : niveau de détail changé sans fondu.
struct EtatMesure { float o = 0; vec3 pos{}; int lod = -1; bool vu = false; };
struct Mesure {
    bool actif = false;
    int images = 0, apparitions = 0, disparitions = 0, sauts = 0, detailsBrutaux = 0, vusMax = 0, affichesMax = 0;
    int fondusProches = 0;   // entrées en fondu (permises) d'une personne déjà grande à l'écran (≥ 3 % : à moins de ≈ 35 m)
    double vusSomme = 0;
    std::vector<EtatMesure> prec;
    std::string premier;   // premier évènement (journal) : personne, distance, nature
    int imagePremier = -1;
    void remettre() { const bool a = actif; *this = Mesure{}; actif = a; }
};
Mesure g_mesure;
// part de la hauteur de l'écran occupée par une personne de 1,75 m dont le centre est en p ; < 0 : hors de l'image
float hauteurVue(const GameView& g, const vec3& p) {
    const mat4 pv = g.proj * g.view;
    const vec3 c{p.x, p.y + 0.9f, p.z};
    const float x = pv.at(0, 0) * c.x + pv.at(0, 1) * c.y + pv.at(0, 2) * c.z + pv.at(0, 3);
    const float y = pv.at(1, 0) * c.x + pv.at(1, 1) * c.y + pv.at(1, 2) * c.z + pv.at(1, 3);
    const float w = pv.at(3, 0) * c.x + pv.at(3, 1) * c.y + pv.at(3, 2) * c.z + pv.at(3, 3);
    if (w < 0.3f || std::fabs(x) > w || std::fabs(y) > w) return -1;
    return std::fabs(g.proj.at(1, 1)) * 1.75f / w * 0.5f;
}
constexpr float HAUTEUR_DISTINGUEE = 0.01f;
// banc : début (tronçons) de la fenêtre de 100 tronçons la plus peuplée de la partie en cours (−1 : aucune personne)
int64_t g_zoneDense = -1, g_longueur = 1;

class Pietons : public System {
public:
    const char* name() const override { return "pietons"; }

    void init(Renderer& r, const GameView& g) override {
        gens_.clear(); actif_ = false; omis_ = 0; tirage_ = 0; omisVille_ = 0; for (int& t : tirageVille_) t = 0; etat_.clear(); tPrec_ = 0; premiere_ = true;
        if (!g.map || !g.track) return;
        const char* id = g.map->id;
        tenue::carte() = id;   // WFS-210 : tenue des conducteurs selon la carte
        ville_ = !std::strcmp(id, "ville"); plage_ = !std::strcmp(id, "plage");
        campagne_ = !std::strcmp(id, "campagne"); circuit_ = !std::strcmp(id, "circuit");
        if (!ville_ && !plage_ && !campagne_ && !circuit_) return;
        if (ville_) for (const ville::Promeneur& m : ville::promeneurs(*g.map, *g.track)) { if (!garder()) continue; Personne P; P.s = m.s; P.lat = m.lat; P.v = m.v; P.dir = m.dir; gens_.push_back(P); }   // WFS-176 : promeneurs avec chien (marche reproduite par la Ville) ; WFS-317 : tous gardés (le chien est dessiné par la Ville)
        piste_ = g.track;   // WFS-179
        const size_t nProm = gens_.size();
        peupler(*g.map, (int64_t)g.track->segmentCount());
        const size_t nMarche = gens_.size();
        {   // WFS-71 : clients des terrasses (placeShop), assis sur les serviettes (beachLife), baigneurs debout dans l'eau
            const int64_t L = (int64_t)g.track->segmentCount(); Personne P; P.assis = P.yawFixe = true;
            for (const vie::Client& c : vie::clientsTerrasses(*g.map, L)) { if (!garderVille(VILLE_TERRASSES)) continue; P.s = c.s; P.lat = c.lat; P.pose = 1; P.y = c.y; P.yawRel = c.yaw; gens_.push_back(P); }
            for (const vie::Serviette& v : vie::serviettes(*g.map, L)) { P.s = v.s + 0.4; P.lat = v.lat; P.pose = 2; P.y = vie::hauteurSable(v.lat) + 0.03; P.yawRel = 1.5708f + v.rot; gens_.push_back(P); }
            P.assis = false; for (const vie::Baigneur& b : vie::baigneurs(*g.map, L)) { P.s = b.s; P.lat = b.lat; P.pose = 3; P.y = -2.25; P.yawRel = b.yaw; gens_.push_back(P); }
            for (const vie::Baigneur& b : plage::gens(*g.map, L)) { P.s = b.s; P.lat = b.lat; P.pose = 4; P.y = vie::hauteurSable(b.lat) + 0.02; P.yawRel = b.yaw; gens_.push_back(P); }   // WFS-180 : foule des fêtes, joueurs
        }
        const size_t nDecor = gens_.size();
        if (ville_) for (const ville::Passant& c : ville::passants(*g.map, *g.track)) {   // WFS-137 (RG-02) : piétons des passages zébrés
            if (!garder() || !garderVille(VILLE_PASSANTS)) continue;   // WFS-222 (RG-07) ; WFS-317 (RG-03)
            Personne P; P.s = c.s; P.lat = c.lat; P.v = c.v; P.dir = c.dir; P.feu = c.feu; P.pose = 5; P.yawFixe = true; P.y = 0.15; gens_.push_back(P); }
        if (gens_.empty()) return;
        if (!chargerPersonnage(r, "people/homme.wfp", modeles_[0]) || !chargerPersonnage(r, "people/femme.wfp", modeles_[1])) {
            WFS_ERR("Piétons indisponibles (assets/people) : aucun piéton affiché");   // pas de silhouettes de repli
            gens_.clear(); return;
        }
        for (int k = 0; k < 2; k++) for (PoseSquelette& p : poses_[k]) preparerPose(modeles_[k], p);
        formerFamilles(r, *g.map);   // WFS-233
        if (g_bancFamilles) placerFamillesBanc(g.player ? g.player->position / SEG : 0.0);
        actif_ = true;
        zoneDense();   // WFS-257 (banc)
        WFS_LOG("Piétons : %zu personnes (%s)", gens_.size(), id);
        if (ville_) WFS_LOG("Piétons (WFS-317, RG-03) : Ville : %d personnes omises (3 sur 10 gardées par liste)", omisVille_);
        memoire::compteurs().personnes = (int)gens_.size();   // WFS-297 (RG-06, accroche)
        // WFS-222 (RG-07) : détail (marcheurs, promeneurs et passants : moitié des tirages ; assis et baigneurs : inchangés)
        WFS_LOG("Piétons (WFS-222) : %zu à pied (promeneurs %zu, marcheurs %zu, passants %zu ; %d omis sur ordre de l'utilisateur), %zu assis ou baigneurs",
                nProm + (nMarche - nProm) + (gens_.size() - nDecor), nProm, nMarche - nProm, gens_.size() - nDecor, omis_, nDecor - nMarche);
    }

    void shutdown(Renderer& r) override {
        memoire::compteurs().personnes = 0;   // WFS-297 (RG-06, accroche)
        for (Personnage& m : modeles_) if (m.ok) detruirePersonnage(r, m);
        for (Personnage& m : enfants_) if (m.ok) detruirePersonnage(r, m);   // WFS-233
        detruirePoussettes(r);   // WFS-240
        gens_.clear(); actif_ = false; enfantsOk_ = false;
    }

    // les marcheurs avancent tous, même invisibles (updatePeopleReal) ; temps d'animation de chacun
    void update(double dt, const GameView& g) override {
        if (!actif_ || g.ecran != Ecran::Course) return;
        const double len = (double)g.track->segmentCount();
        for (Personne& P : gens_) {
            if (P.pose == 0) { P.s += P.dir * P.v / SEG_M * dt; if (P.s >= len) P.s -= len; if (P.s < 0) P.s += len; }
            if (P.pose == 5) ville::traverser(P.lat, P.dir, P.marche, P.yawRel, P.y, P.feu, P.v, dt);   // WFS-137 (RG-02) : au rouge des voitures
            double rate = P.pose == 0 ? (P.v > 2.2 ? P.v / PED_JOG_SPEED : P.v / PED_WALK_SPEED) : 1.0;
            if (P.enfant >= 0 && P.pose == 0) rate = familles::cadence(P.v, kBassin_[P.enfant]);   // WFS-233 : foulée de l'enfant
            P.t += dt * rate / (P.look_ ? P.look.scale : 1.f);
        }
    }

    void render(Renderer& r, const GameView& g) override {
        if (!actif_ || (g.sceneDediee && !plage::vueBanc()) || !g.player) return;   // WFS-216 : la foule de la fête dans les vues du banc
        Track& tr = *g.track;
        const double len = (double)tr.segmentCount(), aP = g.player->position / SEG, aMod = std::fmod(std::fmod(aP, len) + len, len);
        const double carLat = g.player->playerX * RW;
        const double dt = std::clamp(g.time - tPrec_, 0.0, 0.1);
        if (etat_.size() != gens_.size()) { etat_.assign(gens_.size(), apparition::Etat{}); candDe_.assign(gens_.size(), -1); pos_.assign(gens_.size(), vec3{}); yaw_.assign(gens_.size(), 0.f); }
        static const bool avant299 = std::getenv("WFS_PIETONS_AVANT") != nullptr;   // WFS-299 : banc, comparaison avant / après
        const float* seuils = r.qualiteBasse ? (avant299 ? PED_LOD_BASSE_AVANT_M : PED_LOD_BASSE_M) : PED_LOD_M;   // WFS-222 (RG-03) : Basse : complet à 8 m, moyen à 20 m, loin à 30 m, puis niveau 3 (WFS-299)
        const mat4 pv = g.proj * g.view;
        // 1. WFS-257 : chaque personne de la portée : place dans le monde, dans le cadre ou non (RG-02)
        cand_.clear();
        for (int i = 0; i < (int)gens_.size(); i++) {
            Personne& P = gens_[(size_t)i];
            double rel = P.s - aMod; if (rel > len / 2) rel -= len; if (rel < -len / 2) rel += len;
            P.rel = rel; candDe_[(size_t)i] = -1;
            if (rel < PED_RANGE0 || rel > PED_RANGE1 || P.poussette) {   // le petit de la poussette suit son parent (plus bas)
                if (!P.poussette) apparition::avancerOpacite(etat_[(size_t)i], false, false, false, 0, dt);
                continue;
            }
            // une voiture arrive sur le trottoir : la personne s'écarte vers les façades (dodge du jeu web) ; WFS-257 (RG-04) :
            // pas de côté vif (≈ 4 m/s) au lieu d'un bond de 2,6 m en 0,25 s
            if (std::fabs(P.rel) < 6 && std::fabs(carLat - P.lat) < 2.4) P.dodge = std::min(1.0, P.dodge + dt * 1.6); else P.dodge = std::max(0.0, P.dodge - dt * 0.5);
            const double side = P.lat >= 0 ? 1 : -1, lat = P.lat + side * P.dodge * 2.6;
            const TrackPoint tp = tr.point((aP + P.rel) * SEG, lat);
            const double gy = P.y != 0 ? P.y : sol(side);
            pos_[(size_t)i] = {(float)tp.x, (float)(tp.y + gy), (float)tp.z};
            // le personnage regarde vers +Z dans son repère : face au sens de la route = demi-tour (repère natif)
            float yaw = (float)(-tp.th) + (P.yawFixe ? P.yawRel : (P.dir > 0 ? 3.14159265f : 0.f));
            if (P.pose == 5 && etat_[(size_t)i].op > 0) {   // WFS-257 (RG-04) : le passant se retourne (≈ 1 s pour un demi-tour), sans pivot instantané
                float e = std::remainder(yaw - yaw_[(size_t)i], 6.2831853f);
                const float pas = (float)dt * 3.2f;
                yaw = yaw_[(size_t)i] + std::clamp(e, -pas, pas);
            }
            yaw_[(size_t)i] = yaw;
            const double dm = rel * SEG_M;
            const float opMax = apparition::opaciteDistance(dm, PED_RANGE1 * SEG_M);
            if (!dansCadre(pv, g.proj, pos_[(size_t)i])) { apparition::avancerOpacite(etat_[(size_t)i], true, false, false, opMax, dt); continue; }
            // 1re image de la partie (carte chargée, scène qui s'ouvre) : chacun est déjà à sa place, sans fondu
            if (premiere_) apparition::avancerOpacite(etat_[(size_t)i], true, false, false, opMax, dt);
            const apparition::Etat& e = etat_[(size_t)i];
            candDe_[(size_t)i] = (int)cand_.size();
            cand_.push_back({i, -1, (float)std::fabs(dm), e.lieu == apparition::Lieu::Affichee && e.op > 0, e.lodPrec >= 0 ? 4 : 2});   // corps + coiffure (× 2 en fondu croisé)
        }
        for (apparition::Candidat& c : cand_) {   // WFS-233 : la famille entre ou sort ensemble
            const int f = gens_[(size_t)c.i].famille;
            if (f >= 0 && !gens_[(size_t)f].poussette) c.lien = candDe_[(size_t)f];
        }
        apparition::choisirPlaces(cand_, ordre_, INST_PIETONS, place_);
        premiere_ = false;
        // 2. opacité et niveau de détail de chacun ; pose de ceux qu'on dessine (sans dessiner)
        Dessin d[DESSINES]; int nd = 0;
        int util[2] = {0, 0}, utilE[familles::NB_ENFANTS] = {};
        for (size_t k = 0; k < cand_.size(); k++) {
            const int i = cand_[k].i;
            Personne& P = gens_[(size_t)i];
            apparition::Etat& e = etat_[(size_t)i];
            // WFS-317 (RG-02) : niveau de détail selon la distance réelle à la caméra (avant : distance le long de la route depuis la
            // voiture, sans l'écart du trottoir ni le recul de la caméra : niveau complet jusqu'à 17 m de la caméra en vue Proche)
            const double dm = length(pos_[(size_t)i] - g.camPos);
            apparition::avancerOpacite(e, true, true, place_[k] != 0, apparition::opaciteDistance(P.rel * SEG_M, PED_RANGE1 * SEG_M), dt);
            apparition::avancerNiveau(e, (float)dm, seuils, dt);
            if (e.op <= 0 || nd >= DESSINES) continue;
            if (!P.look_) { P.look = P.enfant >= 0 ? lookEnfant(P, i) : pedLook(P, i, plage_); P.look_ = true; }
            const Look& L = P.look;
            const Personnage* M; PoseSquelette* pose;
            if (P.enfant >= 0) {
                if (!enfantsOk_ || utilE[P.enfant] >= POSES_ENFANT) continue;
                M = &enfants_[P.enfant]; pose = &posesEnfants_[P.enfant][utilE[P.enfant]++];
            } else {
                const int sx = L.femme ? 1 : 0;
                if (util[sx] >= DESSINES) continue;
                M = &modeles_[sx]; pose = &poses_[sx][util[sx]++];
            }
            const float yaw = yaw_[(size_t)i];
            const char* anim = P.assis ? "Sitting_Idle_Loop" : P.pose == 0 || P.marche ? (P.v * (1 + P.dodge * 0.6) > 2.2 ? "Jog_Fwd_Loop" : L.walk) : P.pose == 4 ? (i % 3 ? "Jog_Fwd_Loop" : "Idle_Talking_Loop") : L.idle;   // WFS-180 : on danse, on saute
            echantillonner(*M, clipParNom(*M, anim), P.t + P.ph / 6.283 * 1.333, *pose);
            Dessin& D = d[nd++];
            D.M = M; D.pose = pose; D.i = i; D.yaw = yaw; D.echelle = L.scale;
            D.pos = pos_[(size_t)i];
            D.monde = translate(D.pos) * rotateY(yaw) * scale({L.scale, L.scale, L.scale});
            D.lod = e.lod; D.lodPrec = e.lodPrec; D.lodT = e.lodT; D.op = e.op;
            D.app = L.app; D.app.corps = M->corps;
            D.coiffure = P.enfant >= 0 ? nullptr : COIFFURES[L.femme ? 1 : 0][L.hair % 3];   // enfants : cheveux peints sur le corps
            // WFS-240 (RG-03) : une fille sur deux a les cheveux longs (coiffure « Hair_Long » de femme.wfp, Quaternius CC0)
            if (cheveuxLongs(P, i)) { D.cheveuxLongs = familles::teteSurAdulte(P.enfant); D.cheveuxDy = familles::cheveuxDecalageY(P.enfant); }
        }
        for (int a = 0; a < nd; a++) {   // WFS-233 : main dans la main (4 et 7 ans)
            const Personne& E = gens_[(size_t)d[a].i];
            if (E.enfant < 0 || E.famille < 0 || !familles::tientLaMain(E.enfant)) continue;
            for (int b = 0; b < nd; b++) if (d[b].i == E.famille) { tenirLaMain(d[b], d[a]); break; }
        }
        // WFS-240 (RG-01) : poussettes : devant chaque parent dessiné dont l'enfant est dans la poussette ; mains sur la poignée
        int nPous = 0;
        for (int a = 0; a < nd && poussettesOk_; a++) {
            const Personne& A = gens_[(size_t)d[a].i];
            if (A.enfant >= 0 || A.famille < 0 || !gens_[(size_t)A.famille].poussette || nPous >= MAX_POUSSETTES) continue;
            const mat4 rot = rotateY(d[a].yaw);
            const vec3 avant = transformer(rot, {0, 0, 1});
            const mat4 monde = translate(d[a].pos + avant * poussette::DEVANT_ADULTE) * rot;
            for (bool gauche : {true, false}) {   // chaque main : le bras dont l'épaule est la plus proche de sa prise
                const vec3 cible = transformer(monde, poussette::prise(gauche));
                const Bras bg = bras(*d[a].M, true), bd = bras(*d[a].M, false);
                auto d2 = [&](const Bras& b) { const vec3 e = epauleMonde(d[a], b); return (e.x - cible.x) * (e.x - cible.x) + (e.z - cible.z) * (e.z - cible.z); };
                const Bras& b = d2(bg) < d2(bd) ? bg : bd;
                atteindre(*d[a].M, *d[a].pose, b.epaule, b.coude, b.main, versModele(d[a], cible));
            }
            poussettes_[nPous++] = {monde, familles::fille(gens_[(size_t)A.famille].enfant), apparition::tramage(d[a].op)};   // WFS-257 : même fondu que le parent
        }
        // 3. dessin : niveau affiché (fondu d'opacité ou fondu croisé, RG-03) et, pendant un fondu croisé, niveau précédent
        char nom[48];
        for (int k = 0; k < nd; k++) {
            const Dessin& D = d[k];
            const bool croise = D.lodPrec >= 0;
            for (int passe = 0; passe < (croise ? 2 : 1); passe++) {
                const int lod = passe ? D.lodPrec : D.lod;
                const float fondu = croise ? (passe ? apparition::tramageComplement(D.lodT) : apparition::tramage(D.lodT)) : apparition::tramage(D.op);
                std::snprintf(nom, sizeof nom, "corps-%d", lod);
                const Plage* corps = plageParNom(*D.M, nom);
                if (!corps && lod == 3) corps = plageParNom(*D.M, "corps-2");   // WFS-299 : niveau 3 absent (modèle sans niveau 2) : niveau 2
                const Plage* coiffure = nullptr;
                if (D.coiffure) {
                    std::snprintf(nom, sizeof nom, "%s-%d", D.coiffure, lod); coiffure = plageParNom(*D.M, nom);
                    if (!coiffure && lod == 3) { std::snprintf(nom, sizeof nom, "%s-2", D.coiffure); coiffure = plageParNom(*D.M, nom); }
                }
                for (const Plage* pl : {corps, coiffure})
                    if (pl) r.drawSkinned(D.M->maillage, D.M->materiau, D.monde, D.pose->palette.data(), (int)D.pose->palette.size(), pl->premier, pl->nombre, &D.app, fondu);
            }
            if (D.cheveuxLongs > 0) {   // WFS-240 (RG-03) : niveaux 1 (complet et moyen) et 2 (loin) de la coiffure
                const int h = D.lod == 0 ? 1 : D.lod >= 3 ? 3 : 2, hp = croise ? (D.lodPrec == 0 ? 1 : D.lodPrec >= 3 ? 3 : 2) : h;   // WFS-299 : niveau 3
                if (!croise || h == hp) dessinerCheveuxLongs(r, D, h, apparition::tramage(croise ? 1.f : D.op));
                else { dessinerCheveuxLongs(r, D, h, apparition::tramage(D.lodT)); dessinerCheveuxLongs(r, D, hp, apparition::tramageComplement(D.lodT)); }
            }
        }
        for (int k = 0; k < nPous; k++) {   // WFS-240 : poussette (tissu, métal, plastique) et petit assis dedans
            for (int p = 0; p < poussette::NB_PARTIES; p++) if (pousMesh_[p].indexCount) r.draw(pousMesh_[p], pousMat_[p], poussettes_[k].monde, poussettes_[k].fondu);
            const Mesh& b = petitMesh_[poussettes_[k].fille ? 1 : 0];
            if (b.indexCount) r.draw(b, petitMat_, poussettes_[k].monde * translate({0, poussette::SIEGE_Y, poussette::SIEGE_Z})
                                                   * scale({poussette::ECHELLE_PETIT, poussette::ECHELLE_PETIT, poussette::ECHELLE_PETIT}), poussettes_[k].fondu);
        }
        if (g_mesure.actif) {   // WFS-257 (banc)
            float op[DESSINES]; bool fo[DESSINES];
            for (int k = 0; k < nd; k++) { op[k] = d[k].op; fo[k] = d[k].lodPrec >= 0; }
            mesurer(g, d, nd, op, fo, dt);
        }
        tPrec_ = g.time;
    }

    // WFS-257 (RG-02) : la personne (sphère de 1,3 m autour du centre du corps) touche le cadre de la caméra
    static bool dansCadre(const mat4& pv, const mat4& proj, const vec3& p) {
        const vec3 c{p.x, p.y + 0.9f, p.z};
        const float x = pv.at(0, 0) * c.x + pv.at(0, 1) * c.y + pv.at(0, 2) * c.z + pv.at(0, 3);
        const float y = pv.at(1, 0) * c.x + pv.at(1, 1) * c.y + pv.at(1, 2) * c.z + pv.at(1, 3);
        const float w = pv.at(3, 0) * c.x + pv.at(3, 1) * c.y + pv.at(3, 2) * c.z + pv.at(3, 3);
        constexpr float R = 1.3f;
        const float kx = std::sqrt(proj.at(0, 0) * proj.at(0, 0) + 1.f), ky = std::sqrt(proj.at(1, 1) * proj.at(1, 1) + 1.f);
        return w > -R && std::fabs(x) - w <= R * kx && std::fabs(y) - w <= R * ky;
    }

private:
    void zoneDense() {
        const int64_t len = (int64_t)piste_->segmentCount();
        std::vector<int> n((size_t)len + 1, 0);
        for (const Personne& P : gens_) n[(size_t)std::clamp<int64_t>((int64_t)P.s, 0, len - 1)]++;
        int meilleur = 0, c = 0; g_zoneDense = -1; g_longueur = std::max<int64_t>(1, len);
        for (int64_t k = 0; k < 100 && k < len; k++) c += n[(size_t)k];
        for (int64_t a = 0; a < len; a++) {   // fenêtre [a, a + 100) sur la boucle
            if (c > meilleur) { meilleur = c; g_zoneDense = a; }
            c += n[(size_t)((a + 100) % len)] - n[(size_t)a];
        }
    }
    // WFS-257 (RG-01, RG-06, banc seulement) : évènements visibles de l'image (voir Mesure)
    void mesurer(const GameView& g, const Dessin* d, int nd, const float* op, const bool* fondu, double dt) {
        Mesure& M = g_mesure;
        if (dt <= 1e-6) return;
        if (M.prec.size() != gens_.size()) M.prec.assign(gens_.size(), EtatMesure{});
        std::vector<EtatMesure> cur(gens_.size());
        std::vector<char> fo(gens_.size(), 0);
        for (int k = 0; k < nd; k++) { cur[(size_t)d[k].i] = {op[k], d[k].pos, d[k].lod, false}; fo[(size_t)d[k].i] = fondu[k]; }
        const double vMax = 1.0 / 0.4;
        int vus = 0, aff = 0;
        auto noter = [&](size_t i, const char* quoi, const vec3& p) {
            if (!M.premier.empty()) return;
            char b[160]; const float dx = p.x - g.camPos.x, dz = p.z - g.camPos.z;
            std::snprintf(b, sizeof b, "%s, personne %zu (pose %d%s), à %.0f m de la caméra", quoi, i, gens_[i].pose, gens_[i].enfant >= 0 ? ", enfant" : "", std::sqrt(dx * dx + dz * dz));
            M.premier = b; M.imagePremier = M.images;
        };
        for (size_t i = 0; i < gens_.size(); i++) {
            const EtatMesure& a = M.prec[i]; EtatMesure& b = cur[i];
            if (a.o <= 0 && b.o <= 0) continue;
            const vec3 pos = b.o > 0 ? b.pos : a.pos;
            const float hv = hauteurVue(g, pos);
            b.vu = hv >= HAUTEUR_DISTINGUEE;
            if (M.images > 0 && a.o <= 0 && b.o > 0 && hv >= 0.03f) M.fondusProches++;
            if (b.o > 0) { aff++; if (b.vu) vus++; }
            if (!b.vu || M.images == 0) continue;   // 1re image mesurée : état de départ seulement
            const double dO = (double)b.o - (double)a.o;
            if (dO > vMax * dt + 1e-4) { M.apparitions++; noter(i, "apparition", pos); }
            if (-dO > vMax * dt + 1e-4) { M.disparitions++; noter(i, "disparition", pos); }
            if (a.o > 0 && b.o > 0) {
                const vec3 dp = b.pos - a.pos;
                if (std::sqrt(dp.x * dp.x + dp.y * dp.y + dp.z * dp.z) > 10.0 * dt) { M.sauts++; noter(i, "saut", pos); }
                if (a.lod != b.lod && !fo[i]) { M.detailsBrutaux++; noter(i, "détail brutal", pos); }
            }
        }
        M.vusSomme += vus; M.vusMax = std::max(M.vusMax, vus); M.affichesMax = std::max(M.affichesMax, aff);
        M.prec.swap(cur); M.images++;
    }

    // WFS-233 : tenue de l'enfant (couleurs tirées comme pedLook, sans maillot ni tenue de soirée), teint et cheveux du parent
    Look lookEnfant(const Personne& P, int i) {
        Personne& par = gens_[(size_t)std::max(P.famille, 0)];
        if (!par.look_) { par.look = pedLook(par, P.famille, plage_); par.look_ = true; }
        Look L = pedLook(P, i, plage_);
        L.swim = false; L.femme = familles::fille(P.enfant); L.walk = "Walk_Loop"; L.idle = "Idle_Loop";
        Mulberry32 R((uint32_t)i * 7919u + 233u);
        L.scale = 0.97f + 0.06f * (float)R.next();   // ± 3 % autour de la médiane CDC (deux enfants du même âge diffèrent)
        L.app.peau = familles::peauEnfant(par.look.app.peau); L.app.cheveux = par.look.app.cheveux;
        if (par.look.app.cheveux.x > 0.45f && std::fabs(par.look.app.cheveux.x - par.look.app.cheveux.z) < 0.05f) L.app.cheveux = hx(HAIR[1]);   // parent aux cheveux gris : châtain foncé
        L.app.vetement = {L.app.vetement.x, L.app.vetement.y, 1.f, (float)tenue::TISSU_COTON};
        return L;
    }

    // hauteur du sol des piétons (GROUND du jeu web) : trottoir de la ville 0,15 m, promenade de la plage 0,1 m
    double sol(double) const { return ville_ ? 0.15 : 0.1; }

    // addPerson (life.js) : direction, vitesse et phase tirées à la graine (le jeu web utilise Math.random)
    void ajouter(double s, double lat, int pose, Mulberry32& A, double v = 0, double latMin = 0, double latMax = 0) {
        if ((int)gens_.size() + omis_ + omisVille_ >= PEOPLE_MAX) return;   // plafond du jeu web compté sur la liste d'origine (WFS-222 ; WFS-317 : omis de la Ville compris)
        Personne P; P.s = s; P.lat = lat; P.pose = pose; P.latMin = latMin; P.latMax = latMax;   // WFS-233 : bande du trottoir
        P.dir = A.next() < 0.5 ? -1 : 1; P.v = v > 0 ? v : 1.1 + A.next() * 0.6; P.ph = A.next() * 6.28;
        if (garder() && garderVille(VILLE_MARCHEURS)) gens_.push_back(P);   // WFS-222 (RG-07) : tirages inchangés (mêmes places pour ceux qui restent) ; WFS-317 (RG-03)
    }
    // WFS-222 (RG-07, ordre de l'utilisateur du 2026-11-07) : au moins 50 % de personnes à pied en moins, sur toutes les cartes :
    // une sur deux des listes d'origine (promeneurs, marcheurs, passants), dans l'ordre des tirages ; les clients attablés,
    // les personnes sur les serviettes et les baigneurs ne sont pas concernés
    bool garder() { const bool g = (tirage_++ % 2) == 1; if (!g) omis_++; return g; }
    // WFS-317 (RG-03) : Ville : 3 sur 10 de chaque liste (apparition::garderVille, foule_apparition.h), compteur propre à chaque liste ; autres cartes : tous
    enum ListeVille { VILLE_MARCHEURS, VILLE_TERRASSES, VILLE_PASSANTS, VILLE_NB };
    bool garderVille(int liste) {
        if (!ville_) return true;
        const bool g = apparition::garderVille(tirageVille_[liste]++);
        if (!g) omisVille_++;
        return g;
    }

    // populatePeople (life.js), même graine mulberry32(seed × 13 + 5) et mêmes tirages
    void peupler(const MapDef& map, int64_t len) {
        Mulberry32 R(map.seed * 13 + 5), A(map.seed * 31 + 11);
        auto rnd = [&]() { return R.next(); };
        for (int64_t n = 0; n < len; n += 3) {
            const char* z = zoneId(map, n, len);
            auto walk = [&](int side, double a, double b, double p) {
                if (rnd() < p) { const double s = n + rnd() * 3; ajouter(s, side * (a + rnd() * (b - a)), 0, A, 0, a, b); }
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
                if (rnd() < 0.35) { const double s2 = rnd(); ajouter((double)n, (A.next() < 0.5 ? -1 : 1) * (RW + 5 + s2 * 6), 3, A, 0, RW + 5, RW + 11); }
            }
        }
    }

    // WFS-233 (RG-01 à RG-03) : 8 enfants MakeHuman chargés pour les cartes qui en ont ; des marcheurs adultes deviennent les
    // enfants d'un autre (total inchangé). Un modèle manquant : aucun enfant, les adultes restent (repli sans perte de piétons)
    void formerFamilles(Renderer& r, const MapDef& map) {
        enfantsOk_ = false;
        const double part = familles::partEnfants(map.id);
        if (part <= 0) return;
        bool ok = true;
        for (int k = 0; k < familles::NB_ENFANTS && ok; k++) ok = chargerPersonnage(r, std::string("people/humains/") + familles::ENFANTS[k] + ".wfp", enfants_[k]);
        if (!ok) {
            WFS_ERR("Enfants (WFS-233) : humains illisibles (assets/people/humains/e*.wfp) : piétons adultes seulement");
            for (Personnage& m : enfants_) if (m.ok) detruirePersonnage(r, m);
            return;
        }
        // rapport des hauteurs de bassin (pose de repos) enfant / adulte du clip (homme.wfp) : foulée du clip de l'enfant
        auto bassin = [](const Personnage& M) {
            PoseSquelette ps; preparerPose(M, ps); echantillonner(M, -1, 0, ps);
            const int o = osParNom(M, "pelvis");
            return o >= 0 ? ps.modele[(size_t)o].at(1, 3) : 0.f;
        };
        const float ref = bassin(modeles_[0]);
        char cad[160]; int nc = 0;
        for (int k = 0; k < familles::NB_ENFANTS; k++) {
            kBassin_[k] = ref > 0 ? bassin(enfants_[k]) / ref : 0.6f;
            for (PoseSquelette& ps : posesEnfants_[k]) preparerPose(enfants_[k], ps);
            // WFS-240 (RG-04) : cadence à 1,0 m/s (pas/min) : multiplicateur du clip × 2 pas par boucle du clip de marche
            const int c = clipParNom(enfants_[k], "Walk_Loop");
            const double duree = c >= 0 ? enfants_[k].clips[(size_t)c].duree : 0;
            if (duree > 0 && nc < (int)sizeof cad - 16) nc += std::snprintf(cad + nc, sizeof cad - (size_t)nc, " %s %.0f", familles::ENFANTS[k], familles::cadence(1.0, kBassin_[k]) * 2 / duree * 60);
        }
        { const int c = clipParNom(modeles_[0], "Walk_Loop"); const double duree = c >= 0 ? modeles_[0].clips[(size_t)c].duree : 0;
          WFS_LOG("Enfants (WFS-240, RG-04) : cadence à 1,0 m/s (pas/min) :%s ; adulte %.0f", cad, duree > 0 ? 1.0 / 1.1 * 2 / duree * 60 : 0.0); }
        std::vector<familles::Marcheur> m(gens_.size());
        for (size_t i = 0; i < gens_.size(); i++) {
            const Personne& P = gens_[i];
            m[i] = {P.s, P.lat, P.v, P.dir, P.latMin, P.latMax, (P.pose == 0 || P.pose == 3) && !P.assis && P.latMax > P.latMin && P.v < 2.2};
        }
        const std::vector<familles::Lien> liens = familles::former(m, (int)gens_.size(), part, (uint32_t)map.seed, SEG_M);
        creerPoussettes(r);   // WFS-240
        int nPoussettes = 0;
        int ages[4] = {0, 0, 0, 0};
        for (const familles::Lien& l : liens) {
            Personne& P = gens_[(size_t)l.parent]; Personne& E = gens_[(size_t)l.enfant];
            P.lat = m[(size_t)l.parent].lat; P.v = m[(size_t)l.parent].v;
            const double ph = P.ph;
            E = P; E.s = m[(size_t)l.enfant].s; E.lat = m[(size_t)l.enfant].lat; E.ph = ph + 2.1; E.look_ = false;
            E.enfant = l.morpho; E.famille = l.parent; P.famille = l.enfant;
            ages[l.morpho / 2]++;
            // WFS-240 (RG-01) : une famille sur 4 environ : l'enfant devient le petit de la poussette (même personne : total inchangé)
            if (poussettesOk_ && poussette::avecPoussette((uint32_t)map.seed, (int)(&l - liens.data()))) { E.poussette = true; nPoussettes++; }
        }
        enfantsOk_ = true;
        WFS_LOG("Enfants (WFS-233) : %zu familles sur %zu piétons (%.0f %% visés ; 4 ans %d, 7 ans %d, 10 ans %d, 14 ans %d) ; "
                "WFS-240 : %d poussette(s)", liens.size(), gens_.size(), part * 100, ages[0], ages[1], ages[2], ages[3], nPoussettes);
    }

    // banc : la première famille de 4 ans et la première de 7 ans, à 7 et 11 m devant la voiture, côté droit, vers la caméra
    void placerFamillesBanc(double s0) {
        int places = 0;
        for (int want : {0, 1, 2}) {   // WFS-240 : 2 = une famille avec poussette, à 9 m
            for (size_t i = 0; i < gens_.size(); i++) {
                Personne& E = gens_[i];
                if (E.enfant < 0 || E.famille < 0 || E.pose != 0 || E.poussette != (want == 2) || (want < 2 && familles::age(E.enfant) != (want ? 7 : 4))) continue;
                Personne& P = gens_[(size_t)E.famille];
                const double s = s0 + (want == 2 ? 9.0 : want ? 11.0 : 7.0) / SEG_M, cote = 1;
                P.dir = E.dir = -1; P.v = E.v = 0.95;
                P.s = s; P.lat = cote * (RW + 1.5);
                E.s = s - familles::AVANCE_M / SEG_M; E.lat = cote * (RW + 1.5 + familles::ECART_LAT_M);
                places++; break;
            }
        }
        if (g_bancCheveux)   // WFS-240 : la première fille aux cheveux longs de 7 ou 10 ans, à 6 m, à la place de la famille de 4 ans
            for (size_t i = 0; i < gens_.size(); i++) {
                Personne& E = gens_[i];
                if (!cheveuxLongs(E, (int)i) || E.famille < 0 || E.pose != 0 || familles::age(E.enfant) < 7 || familles::age(E.enfant) > 10) continue;
                Personne& P = gens_[(size_t)E.famille];
                const double s = s0 + 6.0 / SEG_M;   // de dos (elle s'éloigne) : les cheveux longs tombent dans le dos
                P.dir = E.dir = 1; P.v = E.v = 0.95; P.s = s; P.lat = RW + 1.5;
                E.s = s + familles::AVANCE_M / SEG_M; E.lat = RW + 1.5 + familles::ECART_LAT_M;
                places++; break;
            }
        WFS_LOG("Enfants (WFS-233, banc) : %d famille(s) placée(s) devant la voiture", places);
    }

    // WFS-240 (RG-01) : maillages de la poussette (une fois par partie), du petit (garçon, fille) ; matériaux réels :
    // tissu polyester (sombre, mat), aluminium anodisé (métal), plastique noir des roues et de la poignée
    static constexpr int MAX_POUSSETTES = 8;   // WFS-257 : toutes celles du cadre (4 auparavant, avec 8 personnes affichées)
    struct Poussette { mat4 monde; bool fille; float fondu; };
    Poussette poussettes_[MAX_POUSSETTES];
    Mesh pousMesh_[poussette::NB_PARTIES]{}, petitMesh_[2]{};
    Material pousMat_[poussette::NB_PARTIES]{}, petitMat_{};
    Image petitTex_{};
    bool poussettesOk_ = false;

    void creerPoussettes(Renderer& r) {
        if (poussettesOk_) return;
        veg::Geo g[poussette::NB_PARTIES];
        const int tri = poussette::construire(g);
        auto lin = [](uint32_t h) { return vec3{std::pow(((h >> 16) & 255) / 255.f, 2.2f), std::pow(((h >> 8) & 255) / 255.f, 2.2f), std::pow((h & 255) / 255.f, 2.2f)}; };
        const uint32_t couleur[poussette::NB_PARTIES] = {0x2e3440, 0xb8bcc2, 0x18191b};
        const float rugo[poussette::NB_PARTIES] = {0.92f, 0.35f, 0.6f}, metal[poussette::NB_PARTIES] = {0, 1, 0};
        for (int p = 0; p < poussette::NB_PARTIES; p++) {
            Material m; const vec3 c = lin(couleur[p]); m.color = {c.x, c.y, c.z, 1}; m.rough = rugo[p]; m.metal = metal[p]; m.flags = MAT_DOUBLE;
            pousMat_[p] = r.makeMaterial(m, nullptr, nullptr, nullptr);
            if (!g[p].i.empty()) pousMesh_[p] = r.makeMesh(g[p].v, g[p].i, false);
        }
        // petit : palette de 6 cases (peau, cheveux, haut garçon, haut fille, bas, chaussures), zones de la silhouette
        constexpr int NC = 6;
        const uint32_t pal[NC] = {0xc8927a, 0x4b3527, 0x8fb8d4, 0xe3a3b6, 0x5d6d86, 0xeeeeee};
        uint8_t px[NC * 4];
        for (int k = 0; k < NC; k++) { px[k * 4] = (pal[k] >> 16) & 255; px[k * 4 + 1] = (pal[k] >> 8) & 255; px[k * 4 + 2] = pal[k] & 255; px[k * 4 + 3] = 255; }
        petitTex_ = r.vk().uploadTexture(px, NC, 1, true);
        Material mp; mp.rough = 0.85f; mp.flags = MAT_DOUBLE;
        petitMat_ = r.makeMaterial(mp, &petitTex_, nullptr, nullptr);
        int triPetit = 0;
        for (int f = 0; f < 2; f++) {
            const foule::Silhouette* sil = foule::silhouettePetit(f == 1);
            if (!sil) continue;
            std::vector<Vertex> v; v.reserve(sil->v.size());
            for (const foule::Sommet& q : sil->v) {
                const int c = q.zone == foule::PEAU ? 0 : q.zone == foule::CHEVEUX ? 1 : q.zone == foule::HAUT ? (f ? 3 : 2) : q.zone == foule::BAS ? 4 : 5;
                v.push_back({q.p.x, q.p.y, q.p.z, q.n.x, q.n.y, q.n.z, (c + 0.5f) / NC, 0.5f});
            }
            petitMesh_[f] = r.makeMesh(v, sil->i, false);
            triPetit = std::max(triPetit, (int)sil->i.size() / 3);
        }
        poussettesOk_ = pousMesh_[0].indexCount && petitMesh_[0].indexCount && petitMesh_[1].indexCount;
        WFS_LOG("Poussettes (WFS-240) : %d triangles (tissu %zu, métal %zu, plastique %zu), petit assis %d triangles%s", tri,
                g[0].i.size() / 3, g[1].i.size() / 3, g[2].i.size() / 3, triPetit, poussettesOk_ ? "" : " : indisponibles, aucune poussette");
    }
    // WFS-240 (RG-03) : coiffure « Hair_Long » de femme.wfp (même squelette que les enfants de WFS-233, vérifié par
    // test_fidelite_humains) posée sur la tête de l'enfant : chaque os = décalage × pose de l'enfant × échelle de la tête × inverse
    // de la liaison adulte (la coiffure suit le crâne de l'enfant, à sa taille, 1 cm au-dessus) ; ≤ 1 104 triangles (détail moyen de près)
    static bool cheveuxLongs(const Personne& P, int i) { return P.enfant >= 0 && !P.poussette && familles::fille(P.enfant) && ((uint32_t)i * 2654435761u >> 16) % 2 == 0; }
    void dessinerCheveuxLongs(Renderer& r, const Dessin& D, int niveau, float fondu) {
        const Personnage& F = modeles_[1];
        char nom[32];
        std::snprintf(nom, sizeof nom, "Hair_Long-%d", niveau);
        const Plage* pl = plageParNom(F, nom);
        if (!pl && niveau == 3) pl = plageParNom(F, "Hair_Long-2");   // WFS-299 : niveau 3 absent : niveau 2
        if (!pl || F.palette.size() > cheveuxPal_.size()) return;
        const mat4 k = scale({D.cheveuxLongs, D.cheveuxLongs, D.cheveuxLongs}), dy = translate({0, D.cheveuxDy, 0});
        for (size_t j = 0; j < F.palette.size(); j++) {
            const size_t b = (size_t)F.palette[j];
            cheveuxPal_[j] = b < D.pose->modele.size() && b < F.os.size() ? dy * D.pose->modele[b] * k * F.os[b].bindInverse : mat4::identity();
        }
        ApparencePersonnage app = D.app; app.corps = F.corps;
        r.drawSkinned(F.maillage, F.materiau, D.monde, cheveuxPal_.data(), (int)F.palette.size(), pl->premier, pl->nombre, &app, fondu);
    }
    std::vector<mat4> cheveuxPal_ = std::vector<mat4>(SKIN_MAX_OS, mat4::identity());   // WFS-240 : alloué une fois

    void detruirePoussettes(Renderer& r) {
        if (!pousMat_[0].set && !petitTex_.img) return;
        for (Mesh& m : pousMesh_) if (m.vb.buf) { r.destroy(m); m = Mesh{}; }
        for (Mesh& m : petitMesh_) if (m.vb.buf) { r.destroy(m); m = Mesh{}; }
        for (Material& m : pousMat_) if (m.set) { r.destroy(m); m = Material{}; }
        if (petitMat_.set) { r.destroy(petitMat_); petitMat_ = Material{}; }
        if (petitTex_.img) { r.vk().destroy(petitTex_); petitTex_ = Image{}; }
        poussettesOk_ = false;
    }

    std::vector<Personne> gens_;
    Personnage enfants_[familles::NB_ENFANTS];             // WFS-233
    PoseSquelette posesEnfants_[familles::NB_ENFANTS][POSES_ENFANT];  // WFS-233 ; WFS-257 : POSES_ENFANT par morphotype et par image
    float kBassin_[familles::NB_ENFANTS] = {};
    bool enfantsOk_ = false;
    Personnage modeles_[2];   // 0 homme, 1 femme
    PoseSquelette poses_[2][DESSINES];
    // WFS-257 : état d'affichage (opacité, détail), place dans le monde, orientation affichée de chaque personne ; candidats du
    // cadre et leurs places (tableaux gardés d'une image à l'autre : aucune allocation une fois remplis)
    std::vector<apparition::Etat> etat_;
    std::vector<int> candDe_, ordre_;
    std::vector<vec3> pos_;
    std::vector<float> yaw_;
    std::vector<apparition::Candidat> cand_;
    std::vector<char> place_;
    bool premiere_ = true;   // WFS-257 : 1re image de la partie
    int omis_ = 0, tirage_ = 0;   // WFS-222 (RG-07)
    double tPrec_ = 0;
    int tirageVille_[VILLE_NB] = {}, omisVille_ = 0;   // WFS-317 (RG-03)
    bool actif_ = false, ville_ = false, plage_ = false, campagne_ = false, circuit_ = false;
    const Track* piste_ = nullptr;   // WFS-179
};

} // namespace

WFS_SYSTEM(Pietons, 106)   // décor (100-199) : après la ville (100) et la plage (105)

void benchApparitions(Game& g, Renderer& r, const std::string& out);
void benchPietons(Game& g, Renderer& r, const std::string& out) {
    if (std::getenv("WFS_APPARITIONS_SEUL")) { benchApparitions(g, r, out); return; }   // WFS-257 : essai local
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
    // WFS-233 (RG-04) : familles de jour, voiture à l'arrêt (poursuite proche), enfants de 4 et 7 ans devant elle à droite :
    // enfants-<carte>.png (Ville, Corniche, Campagne) ; deux instants de la marche en Ville
    g_bancFamilles = true;
    for (const char* carte : {"ville", "plage", "campagne"}) {
        if (!g.init(r, "golf", carte)) { WFS_ERR("enfants : chargement %s impossible", carte); continue; }
        g.setAutopilot(false); g.camMode = 0;
        double t = 1100;
        for (int i = 0; i < 30; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 0) g.render(t); }
        g.render(t); g.render(t + 0.01);
        r.capturePng(out + "/enfants-" + carte + ".png");
        WFS_LOG("capture enfants-%s", carte);
        if (!std::strcmp(carte, "ville")) {
            for (int i = 0; i < 20; i++) { g.update(1.0 / 60); t += 1.0 / 60; }
            g.render(t); g.render(t + 0.01);
            r.capturePng(out + "/enfants-ville-1.png");
        }
        g.shutdown();
    }
    g_bancCheveux = true;   // WFS-240 (RG-03) : enfants-cheveux-longs (Ville, de jour)
    if (g.init(r, "golf", "ville")) {
        g.setAutopilot(false); g.camMode = 0;
        double t = 1200;
        for (int i = 0; i < 30; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 0) g.render(t); }
        g.render(t); g.render(t + 0.01);
        r.capturePng(out + "/enfants-cheveux-longs.png");
        WFS_LOG("capture enfants-cheveux-longs");
        g.shutdown();
    }
    g_bancCheveux = false;
    g_bancFamilles = false;
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
    benchApparitions(g, r, out);   // WFS-257
}

// WFS-257 (RG-01, RG-06) : apparitions et disparitions visibles des personnes, 4 cartes, jour et nuit, à 20 images/s :
// course en pilote automatique (vue Proche), départ 150 m avant la partie la plus peuplée de la carte, WFS_APPARITIONS_S
// secondes mesurées par cas après 2 s de mise en route. Sans WFS_APPARITIONS_S (CI, job partie2c ≤ 11 min) : Ville de
// jour seule, 4 s ; avec : les 4 cartes, jour et nuit (essai local : 12 s par cas ≈ 15 min). WFS_APPARITIONS_SUITE=<dossier> (essai local) : rejoue chaque cas et enregistre la suite de 7 images autour
// du premier évènement (apparition-<carte>-<moment>-<n>.png). Total au journal : « apparitions visibles : N ».
void benchApparitions(Game& g, Renderer& r, const std::string& out) {
    const char* e = std::getenv("WFS_APPARITIONS_S");
    const int images = std::clamp(e ? std::atoi(e) : 4, 1, 600) * 20;
    const char* suite = std::getenv("WFS_APPARITIONS_SUITE");
    (void)out;
    int total = 0, sauts = 0, details = 0;
    g_mesure.actif = true;
    // WFS-299 : WFS_APPARITIONS_BASSE=1 : qualité Basse (niveau 3 « très simple » des piétons au-delà de 30 m)
    const bool basse = std::getenv("WFS_APPARITIONS_BASSE") != nullptr;
    ecrans::Reglages& R = ecrans::reglages(); const int q0 = R.qualite;
    if (basse) { R.qualite = ecrans::QUALITE_BASSE; ecrans::appliquerQualiteNiveau(r); }
    for (const char* moment : {"jour", "nuit"}) for (const char* carte : {"ville", "plage", "campagne", "circuit"}) {
        if (!e && (std::strcmp(moment, "jour") || std::strcmp(carte, "ville"))) continue;
        setenv("WFS_MOMENT", moment, 1);
        // départ 150 m avant la partie la plus peuplée de la carte (WFS_DEPART du décor), la voiture partant de l'arrêt
        unsetenv("WFS_DEPART");
        if (!g.init(r, "golf", carte)) { WFS_ERR("apparitions : chargement %s impossible", carte); continue; }
        const int64_t dep = g_zoneDense >= 0 ? ((g_zoneDense - (int64_t)(150 / SEG_M) - (int64_t)(g.player().position / SEG)) % g_longueur + g_longueur) % g_longueur : 0;
        g.shutdown();
        setenv("WFS_DEPART", std::to_string(dep).c_str(), 1);
        auto jouer = [&](int n, int capDe, int capA) {
            if (!g.init(r, "golf", carte)) return false;
            g.setAutopilot(true); g.camMode = 0;
            double t = 2000;
            for (int i = 0; i < 40; i++) { for (int k = 0; k < 3; k++) g.update(1.0 / 60); t += 0.05; g.render(t); }   // 2 s de mise en route
            g_mesure.remettre();
            for (int i = 0; i < n; i++) {
                for (int k = 0; k < 3; k++) g.update(1.0 / 60);
                t += 0.05; g.render(t);
                if (i >= capDe && i <= capA) r.capturePng(std::string(suite) + "/apparition-" + carte + "-" + moment + "-" + std::to_string(i - capDe) + ".png");
            }
            return true;
        };
        if (!jouer(images, -1, -2)) { WFS_ERR("apparitions : chargement %s impossible", carte); continue; }
        const Mesure m = g_mesure;
        WFS_LOG("WFS-257 apparitions %s %s : %d images, %d apparition(s), %d disparition(s), %d saut(s), %d détail(s) brutal(aux) visibles, %d entrée(s) en fondu de près ; "
                "personnes vues %.1f en moyenne, %d au plus ; affichées au plus %d%s%s", carte, moment, m.images, m.apparitions, m.disparitions,
                m.sauts, m.detailsBrutaux, m.fondusProches, m.images ? m.vusSomme / m.images : 0.0, m.vusMax, m.affichesMax, m.premier.empty() ? "" : " ; premier : ", m.premier.c_str());
        total += m.apparitions + m.disparitions; sauts += m.sauts; details += m.detailsBrutaux;
        g.shutdown();
        if (suite && m.imagePremier >= 0) {
            WFS_LOG("WFS-257 suite %s %s : images %d à %d (premier évènement à l'image %d)", carte, moment, m.imagePremier - 3, m.imagePremier + 3, m.imagePremier);
            jouer(m.imagePremier + 4, m.imagePremier - 3, m.imagePremier + 3);
            g.shutdown();
        }
    }
    g_mesure.actif = false;
    unsetenv("WFS_MOMENT"); unsetenv("WFS_DEPART");
    WFS_LOG("WFS-257 : apparitions et disparitions visibles : %d ; sauts : %d ; détails brutaux : %d (objectif : 0)", total, sauts, details);
    if (basse) { R.qualite = q0; ecrans::appliquerQualiteNiveau(r); }
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
    const uint32_t version = magie && std::memcmp(magie, "WFP1", 4) == 0 ? u32() : 0;
    if (version != 2 && version != 3) return false;   // WFS-240 : version 3 (humains de WFS-209 : tangente en plus)
    for (uint32_t t = 0, n = u32(); ok && t < n; t++) prendre(u32());
    f32(); f32(); b.p.hauteur = f32();
    b.p.corps = {f32(), f32(), f32(), f32()};
    const uint32_t nV = u32(), nI = u32(), nP = u32(), nO = u32(), nC = u32();
    if (!ok || nV > 200000 || nI > 600000 || nO > 256) return false;
    b.v.resize(nV); b.idx.resize(nI);
    if (version == 2) { if (const uint8_t* q = prendre((size_t)nV * sizeof(SkinVertex))) std::memcpy(b.v.data(), q, (size_t)nV * sizeof(SkinVertex)); }
    else if (const uint8_t* q = prendre((size_t)nV * 84))   // comme chargerPersonnage : tangente (16 octets après l'uv) mise de côté
        for (uint32_t i = 0; i < nV; i++) {
            std::memcpy(&b.v[i], q + (size_t)i * 84, 32);
            std::memcpy(reinterpret_cast<uint8_t*>(&b.v[i]) + 32, q + (size_t)i * 84 + 48, 36);
        }
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


// WFS-240 (RG-02) : enfants de 7, 10 et 14 ans (garçon, fille) assis, mêmes poses et variantes que les adultes ; fesses en
// y = 0 ; assise = hauteur du siège (0,45 m) : les jambes de l'enfant pendent devant le siège
constexpr const char* ENFANTS_TRIBUNE[6] = {"e07g", "e07f", "e10g", "e10f", "e14g", "e14f"};
int indiceEnfant(int e, int pose, int variante, int proche) { return ((e * 2 + (pose == ASSIS_PARLE)) * VARIANTES + variante) * 2 + proche; }

Cache batirEnfants() {
    Cache c;
    c.s.resize((size_t)indiceEnfant(5, ASSIS_PARLE, VARIANTES - 1, 1) + 1);
    size_t tri = 0;
    for (int e = 0; e < 6; e++) {
        Brut b;
        if (!lire((std::string("people/humains/") + ENFANTS_TRIBUNE[e] + ".wfp").c_str(), b)) {
            WFS_ERR("Spectateurs (WFS-240) : enfant illisible (people/humains/%s.wfp) : aucun enfant en tribune", ENFANTS_TRIBUNE[e]);
            return Cache{};
        }
        for (int p : {ASSIS_REPOS, ASSIS_PARLE}) for (int v = 0; v < VARIANTES; v++) for (int pr = 0; pr < 2; pr++) {
            Silhouette& S = c.s[(size_t)indiceEnfant(e, p, v, pr)];
            S = construire(b, e % 2, 0, (Pose)p, v, pr ? 0.08f : 0.13f);   // grille plus fine : l'enfant est plus petit
            S.assise = 0.45f;
            tri = std::max(tri, S.i.size() / 3);
        }
    }
    c.ok = true;
    float tete[3] = {0, 0, 0};   // hauteur de la tête au-dessus du siège (fesses en y = 0), 7, 10 et 14 ans (garçon, repos, de près)
    for (int a = 0; a < 3; a++) for (const Sommet& v : c.s[(size_t)indiceEnfant(a * 2, ASSIS_REPOS, 0, 1)].v) tete[a] = std::max(tete[a], v.p.y);
    WFS_LOG("Spectateurs (WFS-240) : %zu silhouettes d'enfants assis (7, 10, 14 ans ; garçon, fille), %zu triangles au plus ; "
            "tête au-dessus du siège %.2f / %.2f / %.2f m", c.s.size(), tri, tete[0], tete[1], tete[2]);
    return c;
}

} // namespace

const Silhouette* silhouettePetit(bool fille) {   // WFS-240 (RG-01)
    static const Cache c = [] {
        Cache k; k.s.resize(2);
        for (int f = 0; f < 2; f++) {
            Brut b;
            if (!lire(f ? "people/humains/e04f.wfp" : "people/humains/e04g.wfp", b)) { WFS_ERR("Poussettes (WFS-240) : petit illisible (people/humains/e04*.wfp)"); return Cache{}; }
            k.s[(size_t)f] = construire(b, f, 0, ASSIS_REPOS, 0, 0.07f);
        }
        k.ok = true;
        return k;
    }();
    return c.ok ? &c.s[fille ? 1 : 0] : nullptr;
}

const Silhouette* silhouetteEnfant(uint32_t h, Pose pose, int variante, bool proche) {
    static const Cache c = batirEnfants();
    if (!c.ok) return nullptr;
    const int e = (int)((h >> 9) % 6);
    return &c.s[(size_t)indiceEnfant(e, pose == ASSIS_PARLE ? ASSIS_PARLE : ASSIS_REPOS, ((variante % VARIANTES) + VARIANTES) % VARIANTES, proche ? 1 : 0)];
}


const Silhouette* silhouette(bool femme, int coiffure, Pose pose, int variante, bool proche) {
    static const Cache c = batir();   // une fois, au premier appel (initialisation sûre entre fils : morceaux construits en parallèle)
    if (!c.ok) return nullptr;
    return &c.s[(size_t)indice(femme ? 1 : 0, ((coiffure % COIFFURES) + COIFFURES) % COIFFURES, pose, ((variante % VARIANTES) + VARIANTES) % VARIANTES, proche ? 1 : 0)];
}

} // namespace foule

} // namespace wfs
