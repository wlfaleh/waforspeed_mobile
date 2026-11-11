// WaForSpeed — nuit complète (ticket WFS-09, épic E4 ; absorbe WFS-16 « route mouillée avec reflets »).
// Port de l'éclairage de nuit du jeu web (wlfaleh/waaps, waforspeed/) :
//   - ambiance (js/world/maps.js, envFor nuit) : ciel, hémisphère #46327a / #08080e, lune #8a96ff, brouillard linéaire 40-540 m ;
//   - phares du joueur : deux feux de croisement réels (WFS-218, phares.h ; avant : projecteur unique du web, 0xfff1dd,
//     portée 170 m, angle 0,7) ; feu de recul (PointLight 0xe8eeff, 2,2 la nuit, 14 m) ;
//   - tache des phares du jeu web (buildBeam / updateBeam) : retirée par WFS-382 (doublait la lumière des deux feux) ;
//   - lampadaires (js/world/scene.js, « éclairage des lampadaires proches ») : 4 lumières au plus sur les plus proches,
//     halo de la tête et flaque de lumière au sol (poolQuad de js/world/decor.js) ;
//   - route mouillée (WFS-16) : nuit en ville et sur la corniche ; flaques et reflets temps réel en qualité haute.
// Les feux des voitures (arrière, stop, recul, phares) sont allumés par le moteur (Renderer::feuxVoiture) dès que
// FrameParams::feux est vrai ; ce système donne seulement l'état du joueur (frein, marche arrière).
// Moment : WFS_MOMENT=nuit (écran Terrain, WFS-41, ou banc d'essai), comme la plage (WFS-14).
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "../core/log.h"
#include "../core/system.h"
#include "retroviseurs.h"   // WFS-324 (accroche) : lampadaires derrière, pour les rétroviseurs
#include "../game/cars.h"
#include "../game/player.h"
#include "../game/track.h"
#include "../ui/ecrans/reglages.h"
#include "eclairage.h"
#include "renderer.h"
#include "feux_modele.h"   // WFS-109
#include "phares.h"        // WFS-218 : feux de croisement du joueur
#include "nuit_qualite.h"  // WFS-421 : halos au sol des lampadaires coupés en Basse
#include "../world/meteo.h"   // WFS-142 (RG-01) : route mouillée seulement quand il pleut (lecture seule)

namespace wfs {
namespace {

vec3 linHex(uint32_t h) {
    auto c = [&](int sh) { return std::pow(((h >> sh) & 255) / 255.f, 2.2f); };
    return {c(16), c(8), c(0)};
}

constexpr float HALO_TETE_M = 0.6f;   // WFS-356 (RG-05) : rayon du halo des têtes de lampe de la Ville (m), ≤ 3 fois la tête (0,36 m de profondeur)
constexpr int LAMPES_MAX = 4;   // RG-05 : 4 lumières ponctuelles au plus

class Nuit : public System {
public:
    const char* name() const override { return "nuit"; }

    void init(Renderer&, const GameView& g) override {
        const char* m = std::getenv("WFS_MOMENT");
        nuit_ = m && std::strcmp(m, "nuit") == 0 && g.map;
        if (!nuit_) return;
        const char* id = g.map->id;
        ville_ = !std::strcmp(id, "ville"); plage_ = !std::strcmp(id, "plage");
        campagne_ = !std::strcmp(id, "campagne"); circuit_ = !std::strcmp(id, "circuit");
        // RG-06 : la ville et la campagne (WFS-225) inscrivent leurs lampadaires (src/world/ville.cpp, campagne.cpp) ;
        // la corniche et le circuit n'ont pas encore de lampadaires en décor natif : têtes de lampe placées ici aux pas du
        // jeu web (decor.js : « beach » tous les 30 segments côté mer, « flood » tous les 34 en alternance)
        auto& L = lampadaires();
        const int64_t len = (int64_t)g.track->segmentCount();
        // WFS-225 (RG-01) : la Campagne inscrit ses lampadaires (village, rond-point, feu, péage) : ils ne sont plus effacés ici
        if (L.empty() && plage_) for (int64_t n = 15; n < len; n += 30) L.push_back({n, (float)RW + 0.6f, 5.6f, StyleLampadaire::Plage});
        else if (L.empty() && circuit_)
            for (int64_t n = 10; n < len; n += 34) L.push_back({n, ((n / 34) % 2 ? 1.f : -1.f) * ((float)RW + 4.f), 16.f, StyleLampadaire::Stade});
        std::sort(L.begin(), L.end(), [](const Lampadaire& a, const Lampadaire& b) { return a.seg < b.seg; });
        WFS_LOG("Nuit : carte %s, %zu lampadaires, phares de croisement %s (%.0f cd chacun)%s", id, L.size(), phares::led(g.car ? g.car->id : nullptr) ? "LED" : "halogènes",
                phares::creteCd(phares::led(g.car ? g.car->id : nullptr)), mouille() ? ", route mouillée" : "");
    }
    // WFS-142 (RG-01) : la chaussée n'est mouillée que s'il pleut (meteo::pluieActive : la ville la nuit), ou si le banc
    // force la météo (WFS_METEO=pluie / sec). Avant : mouillée à chaque nuit en ville et sur la corniche, même sans pluie.
    static int meteoForcee() {   // 1 pluie, 0 sec, -1 rien de forcé
        const char* m = std::getenv("WFS_METEO");
        if (!m) return -1;
        return !std::strcmp(m, "pluie") ? 1 : !std::strcmp(m, "sec") ? 0 : -1;
    }
    bool mouille() const { const int f = meteoForcee(); return f >= 0 ? f == 1 : meteo::pluieActive(ville_ ? "ville" : "", nuit_); }

    bool actif(const GameView& g) const { return nuit_ && !g.sceneDediee && g.ecran != Ecran::Garage; }

    // matrice de la voiture du joueur (Game::carMatrix)
    static mat4 matriceJoueur(const GameView& g) {
        const Player& p = *g.player;
        const TrackPoint q = g.track->point(p.dist, p.playerX * RW);
        return translate({(float)q.x, (float)q.y, (float)q.z}) * rotateY((float)-(q.th + p.heading + p.spinYaw)) * rotateX((float)q.slope);
    }

    // distance (segments) du joueur au lampadaire, sur la piste qui se répète ; < 0 : derrière
    static int64_t ecart(const Lampadaire& l, int64_t aP, int64_t len) {
        int64_t d = ((l.seg - aP) % len + len) % len;
        return d > len - 40 ? d - len : d;
    }

    void ambiance(FrameParams& fp, const GameView& g) override {
        if (!actif(g)) {   // WFS-142 (RG-09) : le jour, route mouillée seulement si le banc force la pluie (aucune pluie de jour en jeu)
            if (!nuit_ && meteoForcee() == 1 && g.player && !g.sceneDediee && g.ecran != Ecran::Garage) {
                fp.mouille = true; fp.flaques = fp.reflets = ecrans::reglages().qualite == ecrans::QUALITE_HAUTE;
                fp.refletY = matriceJoueur(g).transformPoint({0, 0, 0}).y;
            }
            return;
        }
        const bool haute = ecrans::reglages().qualite == ecrans::QUALITE_HAUTE;
        // ---------- RG-01 : envFor nuit (couleurs éclaircies comme la plage, WFS-14 : l'étalonnage du web n'est pas porté) ----------
        // WFS-97 (RG-04, ANO-37) : en ville et sur la corniche, on aurait dit le crépuscule (façades lavande) → ciel et
        // brouillard moins éclaircis, hémisphère × 0,45 et lune × 0,4 : façades sombres, la voiture, les néons, les flaques
        // et les reflets ressortent. Exposition (1,1) gardée : la baisser éteindrait aussi néons et flaques ; ambiance (0,7)
        // gardée : elle ne règle que les reflets du ciel. Circuit (nuit bleutée voulue) et campagne inchangés.
        const bool rue = ville_ || plage_;
        const float k = rue ? 1.4f : 2.2f;
        uint32_t hor = 0x3a1a36, fog = 0x120d1d, hemiS = 0x46327a; float hemiI = 0.75f, fogFar = 540;
        if (plage_) { hor = 0x2b2240; fog = 0x141428; fogFar = 640; }
        // WFS-225 (RG-02) : ciel de pleine lune et voile lointain bleutés (×2) : silhouettes des arbres et des collines lisibles
        if (campagne_) { hor = 0x2a3656; fog = 0x1c2638; hemiS = 0x3a4670; hemiI = 0.6f; }
        if (circuit_) { hor = 0x1f2440; fog = 0x0e1020; hemiS = 0x5a5c90; hemiI = 0.95f; fogFar = 640; }
        if (ecrans::reglages().qualite == ecrans::QUALITE_BASSE) fogFar = std::min(fogFar, 420.f);
        fp.skyTop = linHex(0x04040a) * k; fp.skyHorizon = linHex(hor) * k; fp.ground = linHex(0x0a0a10) * k;
        fp.fogColor = linHex(fog) * k; fp.fogNear = 40; fp.fogFar = fogFar; fp.fogDensity = 0;
        fp.hemiSky = linHex(hemiS); fp.hemiGround = linHex(0x08080e); fp.hemiIntensity = hemiI * (rue ? 0.45f : 1.f);
        // lune : 0,35 du jeu web (soleil 1,2 → 3,2 ici)
        fp.sunDir = normalize(vec3{-0.4f, 0.8f, 0.3f}); fp.sunColor = linHex(0x8a96ff); fp.sunIntensity = 0.35f * 3.2f / 1.2f * (rue ? 0.4f : 1.f);
        fp.ambient = 0.7f; fp.exposure = 1.1f; fp.night = true; fp.shadows = false;
        // ---------- feux et route mouillée ----------
        fp.feux = true;
        fp.mouille = mouille(); fp.flaques = haute; fp.reflets = haute;
        const mat4 W = matriceJoueur(g);
        fp.refletY = W.transformPoint({0, 0, 0}).y;   // plan du miroir : la chaussée sous la voiture
        // ---------- lumières : phares, recul, 4 lampadaires les plus proches ----------
        fp.spotCount = 0;
        const CarDims& d = g.car->d;
        // WFS-218 (RG-00 à RG-02) : deux feux de croisement réels, un à chaque optique avant lue sur le modèle (WFS-413 : centre
        // de chaque glace ; sans modèle : proportions du jeu web) ;
        // axe = avant de la voiture, à l'horizontale ; coupure, intensité (cd, flux sourcé) et teinte : phares.h. Remplace
        // le projecteur unique au centre (cône de 40°, portée 170 m, sans coupure : murs éclairés jusqu'au toit).
        const bool estLed = phares::led(g.car->id);
        const FeuxModele* fm = g.modele && g.modele->ok ? &g.modele->feux : nullptr;
        const float ox = fm ? fm->avantX : d.W / 2 - 0.3f, oy = fm ? fm->avantY : std::max(0.6f, d.H * 0.41f), oz = fm ? fm->avant - 0.03f : -d.L / 2 - 0.03f;
        // WFS-413 (RG-02, ANO-240) : chaque feu part du centre de SA glace mesurée sur le modèle, 1 cm derrière elle (dans l'optique),
        // et plus d'un point calculé devant le pare-chocs ; axe, coupure et intensité inchangés (WFS-218, WFS-382)
        auto source = [&](int cote) {
            if (fm && fm->phares) { const int s = cote > 0 ? 1 : 0; return fm->phare[s] - fm->phareNormale[s] * feux_modele::DERRIERE_GLACE; }
            return vec3{cote * ox, oy, oz};
        };
        // WFS-382 (RG-01) : caméra derrière les optiques (vues Poursuite, Proche, Capot, Cockpit ; pas la vue de face) : pénombre 1 =
        // marque lue par eclairage.glsl (retroChaussee : rétroréflexion de l'asphalte vers le conducteur) ; ignorée par le feu de croisement
        const vec3 avantJ = normalize(W.transformDir({0, 0, -1}));
        const bool derriere = dot(g.camPos - W.transformPoint({0, oy, oz}), avantJ) < 0.5f && length(g.camPos - W.transformPoint({0, 0, 0})) < 15.f;
        for (int cote : {-1, 1}) {
            FrameParams::Spot& h = fp.spots[fp.spotCount++];
            const vec3 src = source(cote);
            h.pos = W.transformPoint(src); h.target = W.transformPoint(src + vec3{0, 0, -10.f});
            h.color = estLed ? phares::TEINTE_LED : phares::TEINTE_HALOGENE; h.intensity = phares::creteCd(estLed) / feux_modele::LUX_PAR_UNITE;
            h.distance = phares::PORTEE_M; h.angle = 1.45f; h.penumbra = derriere ? 1.f : 0.f; h.decay = 0; h.croisement = true;
        }
        if (g.player->reversing || g.player->speed < -1) {   // feu de recul (revLight)
            // WFS-171 (RG-01, ANO-70) : lumière posée sur les feux (hauteur ≈ 0,6 H, ras du pare-chocs ; elle était 0,8 m
            // derrière et 0,6 m de haut : disque blanc décollé), tournée vers l'arrière et 31° vers le bas, cône de 83°
            // (la caisse n'est pas éclairée), bord doux. Intensité tirée de R23 (feux_modele.h, CD_RECUL_SOL = 40 cd par
            // feu vers la chaussée) : 2 × 40 cd donnent ≈ 30 lux à 1 m derrière, 5 lux à 3 m, < 1 lux à 6 m ; avec
            // l'étalon de 35 lux par unité, portée 8 m et décroissance 2 donnent 28, 6,6 et 0,5 lux (écart ≤ 30 % de 1 à 6 m).
            // Couleur : ampoule à incandescence (≈ 3000 K), même blanc halogène que les phares (DA-realisme § 3).
            FrameParams::Spot& r = fp.spots[fp.spotCount++];
            r.pos = W.transformPoint({0, std::max(0.5f, d.H * 0.6f), d.L / 2 - 0.05f});
            r.target = W.transformPoint({0, std::max(0.5f, d.H * 0.6f) - 0.6f, d.L / 2 + 0.95f});
            r.color = linHex(0xfff1dd); r.intensity = 1.85f; r.distance = 8; r.angle = 1.45f; r.penumbra = 1; r.decay = 2;
        }
        const auto& L = lampadaires();
        const int64_t len = (int64_t)g.track->segmentCount(), aP = (int64_t)std::floor(g.player->dist / SEG);
        proches_.clear();
        // WFS-337 : une tête seule (2e lanterne d'un candélabre double) ne prend pas l'une des 4 lumières : même coût de rendu
        for (size_t i = 0; i < L.size(); i++) { if (L[i].style == StyleLampadaire::Tete) continue; const int64_t e = ecart(L[i], aP, len); if (e >= -4 && e < 160) proches_.push_back({e, i}); }
        std::sort(proches_.begin(), proches_.end());
        for (size_t j = 0; j < proches_.size() && j < (size_t)LAMPES_MAX; j++) {
            const Lampadaire& l = L[proches_[j].second];
            const TrackPoint q = g.track->point((double)(aP + proches_[j].first) * SEG, l.lat);
            FrameParams::Spot& s = fp.spots[fp.spotCount++];
            // decay 2 comme PointLight(0xffb46b, 1.1, 26, 2) du jeu web (renderer.js) : lumière moins étalée (WFS-97, RG-10)
            s.pos = {(float)q.x, (float)q.y + l.y, (float)q.z}; s.target = s.pos + vec3{0, -1, 0}; s.angle = PI; s.decay = 2;
            const bool stade = l.style == StyleLampadaire::Stade;
            s.color = linHex(stade ? 0xeaf2ff : 0xffb46b); s.intensity = stade ? 1.8f : 1.1f; s.distance = stade ? 60.f : 26.f;
        }
        // WFS-324 (accroche) : vue Cockpit avec rétroviseurs : les 2 lampadaires les plus proches derrière la voiture (5 à 60 m) éclairent aussi
        // (sans eux, la rue derrière restait noire dans les glaces la nuit, refus du lead dev, PR #341) ; avant les phares des autres voitures
        if (retro::actif()) {
            derriere_.clear();
            for (size_t i = 0; i < L.size(); i++) {
                if (L[i].style == StyleLampadaire::Tete) continue;
                const int64_t e = ecart(L[i], aP, len);
                if (e < -4 && e >= -50) derriere_.push_back({-e, i});
            }
            std::sort(derriere_.begin(), derriere_.end());   // le plus proche derrière d'abord
            for (size_t j = 0; j < derriere_.size() && j < 2 && fp.spotCount < FrameParams::SPOTS_MAX - 2; j++) {
                const Lampadaire& l = L[derriere_[j].second];
                const TrackPoint q = g.track->point((double)(aP - derriere_[j].first) * SEG, l.lat);
                FrameParams::Spot& s = fp.spots[fp.spotCount++];
                s.pos = {(float)q.x, (float)q.y + l.y, (float)q.z}; s.target = s.pos + vec3{0, -1, 0}; s.angle = PI; s.decay = 2;
                const bool stade = l.style == StyleLampadaire::Stade;
                s.color = linHex(stade ? 0xeaf2ff : 0xffb46b); s.intensity = stade ? 1.8f : 1.1f; s.distance = stade ? 60.f : 26.f;
            }
        }
        pharesAutres(fp, W.transformPoint({0, 0, 0}));
    }

    // ---------- WFS-226 (RG-01, RG-02) : vraies lumières des autres voitures les plus proches ----------
    // Voitures notées à l'image précédente par Renderer::feuxVoiture (trafic, adversaires, police, camion) : les 2 (Basse) ou
    // 4 plus proches du joueur, à moins de 70 m, reçoivent un feu de croisement réel (même photométrie que le joueur, paire
    // LED en un projecteur au milieu des optiques, 10 cm devant, position prolongée d'une image). Les autres : lumière au sol calculée (lumiereSolAutres).
    void pharesAutres(FrameParams& fp, const vec3& joueur) {
        eclairants_.clear();
        const int nMax = ecrans::reglages().qualite == ecrans::QUALITE_BASSE ? phares::AUTRES_ECLAIRANTS_BASSE : phares::AUTRES_ECLAIRANTS;
        tri_.clear();
        for (size_t i = 0; i < prec_.size(); i++) {
            const float dist = length(prec_[i].centre - joueur);
            if (dist < phares::AUTRES_PORTEE_SPOT) tri_.push_back({dist, i});
        }
        std::sort(tri_.begin(), tri_.end());
        for (size_t k = 0; k < tri_.size() && (int)k < nMax && fp.spotCount < FrameParams::SPOTS_MAX; k++) {
            const phares::PhareAutre& a = prec_[tri_[k].second];
            // position de cette image : celle de l'image précédente + son déplacement depuis l'avant-dernière (voiture la
            // plus proche à moins de 3 m) : sans cela, à 110 km/h, la lumière partait 0,5 m en arrière (flanc des voisines)
            vec3 depl{}; float dMin = 3.f;
            for (const phares::PhareAutre& b : prec2_) { const float dd = length(b.centre - a.centre); if (dd < dMin) { dMin = dd; depl = a.centre - b.centre; } }
            FrameParams::Spot& h = fp.spots[fp.spotCount++];
            // WFS-413 (RG-02) : milieu des deux glaces mesurées, 1 cm derrière elles (il était 10 cm devant un point du pare-chocs)
            h.pos = a.centre + depl - a.avant * feux_modele::DERRIERE_GLACE; h.target = h.pos + a.avant * 10.f;
            h.color = phares::TEINTE_LED; h.intensity = 2 * phares::creteCd(true) / feux_modele::LUX_PAR_UNITE;
            h.distance = 90; h.angle = 1.45f; h.penumbra = 0; h.decay = 0; h.croisement = true;
            eclairants_.push_back(a.centre + depl);
        }
    }
    // RG-02 : lumière des phares au sol pour les voitures sans vraie lumière (éclairement de phares::eclairementSol changé en
    // luminance d'asphalte, ρ E / LUX_PAR_UNITE comme le rendu des projecteurs), grille couchée devant la voiture, bords fondus
    void lumiereSolAutres(Renderer& r, const GameView& g) {
        static constexpr float RANGS[] = {0.3f, 1.5f, 3.5f, 6.5f, 10.5f, 15.5f, 22.f, 30.f};
        constexpr int NZ = 8, NX = 7; constexpr float DEMI = 7.f;
        auto& A = phares::autres();
        for (const phares::PhareAutre& a : A) {
            bool eclaire = false;
            for (const vec3& e : eclairants_) eclaire = eclaire || length(e - a.centre) < 2.5f;
            if (eclaire || length(a.centre - g.camPos) > 160.f) continue;
            Vertex v[(NZ - 1) * (NX - 1) * 4]; vec3 p[NZ][NX], c[NZ][NX];
            const vec3 haut = normalize(cross(a.droite, a.avant));   // droite × avant = haut (avant vers −Z)
            for (int k = 0; k < NZ; k++) for (int i = 0; i < NX; i++) {
                const float x = -DEMI + 2 * DEMI * i / (NX - 1), z = RANGS[k];
                const float fen = (1 - phares::lisse(DEMI * 0.5f, DEMI, std::abs(x))) * (1 - phares::lisse(RANGS[NZ - 3], RANGS[NZ - 1], z));
                p[k][i] = a.sol + a.droite * x + a.avant * z + haut * 0.04f;
                c[k][i] = phares::TEINTE_LED * (feux_modele::ALBEDO_ASPHALTE * phares::eclairementSol(x, z, a.demiEcart, a.hauteur) / feux_modele::LUX_PAR_UNITE * fen);
            }
            int n = 0;
            auto sommet = [&](int k, int i) { const vec3& q = p[k][i]; const vec3& cc = c[k][i]; v[n++] = {q.x, q.y, q.z, cc.x, cc.y, cc.z, 0.5f, 0.5f}; };
            for (int k = 0; k < NZ - 1; k++) for (int i = 0; i < NX - 1; i++) { sommet(k, i); sommet(k, i + 1); sommet(k + 1, i + 1); sommet(k + 1, i); }
            r.lumiereSol(v, n / 4);
        }
        prec2_.swap(prec_); prec_.assign(A.begin(), A.end()); A.clear();
    }

    void render(Renderer& r, const GameView& g) override {
        if (!actif(g)) { if (!g.sceneDediee && g.ecran != Ecran::Garage) feuxJoueur(r, g); return; }   // WFS-109 : feux du joueur le jour
        Track& t = *g.track;
        const Player& p = *g.player;
        // WFS-382 (RG-01, RG-04, ANO-222) : plus de « tache des phares » du jeu web (grille de 110 × 26 m en mélange multiplicatif,
        // dessinée deux fois en Campagne et sans éclairage public) : elle s'ajoutait aux deux vrais feux de croisement (WFS-218) et
        // faisait le cône blanc brûlé devant la voiture. La chaussée n'est plus éclairée que par les deux feux (phares.h).
        // ---------- RG-05 : têtes de lampe (WFS-421 : plus de flaque de lumière au sol, poolQuad) ----------
        const auto& L = lampadaires();
        const int qualite = ecrans::reglages().qualite;   // WFS-421 : lue une fois par image
        const bool haloT = nuit_qualite::haloTeteLampadaire(qualite);
        const int64_t len = (int64_t)t.segmentCount(), aP = (int64_t)std::floor(p.dist / SEG);
        // WFS-421 (suite, ordre de l'utilisateur du 2026-11-11) : plus aucun disque additif au sol (« flaque » poolQuad du jeu web,
        // r.haloSol) au pied des lampadaires, à AUCUNE qualité : c'était un grand cercle orange à bord net posé sur la chaussée
        // (forme nue interdite, règle du 2026-11-09). La rue n'est plus éclairée que par la vraie lumière des lampadaires
        // (projecteurs du shader, update) ; il ne reste ici que le petit halo de la tête de lampe (RG-02).
        for (const Lampadaire& l : L) {
            const int64_t e = ecart(l, aP, len);
            if (e < -20 || e > 420) continue;   // ≈ 500 m : au-delà, le brouillard les efface
            // halo de la tête : lampadaires du décor de la Ville et « têtes seules » (la corniche et le circuit n'ont pas encore de mât natif)
            // WFS-337 (RG-01, ANO-189) : le candélabre double de la voie rapide (ville.cpp, terre-plein central) porte 2 lanternes
            // à 5 m l'une de l'autre ; la 2e lanterne est inscrite en « tête seule » : son halo, rien d'autre
            if (!haloT || (l.style != StyleLampadaire::Tete && l.style != StyleLampadaire::Ville)) continue;
            haloTete(r, t, (double)(aP + e) * SEG, l, linHex(0xffd9a8));
        }
        lumiereSolAutres(r, g);   // WFS-226 : après le trafic, les adversaires et la police (systèmes d'ordre < 950)
        feuxJoueur(r, g);
    }

    // halo d'une tête de lampe, centré sur sa lanterne (même point que la lumière ponctuelle de ambiance)
    // WFS-356 (RG-05, ANO-202) : rayon 0,6 m (avant 1,1 m : disque de 2,2 m, une « lune » au-dessus de chaque lampe) ; la tête
    // de lampe de la Ville mesure 0,90 × 0,36 m (ville.cpp, LAMPE) : halo ≤ 3 fois la tête
    // WFS-337 (RG-02) : + 0,25 m sur la hauteur inscrite : vasque de la rue à + 0,34 m, lanternes de la voie rapide à + 0,03 à
    // + 0,15 m, du pont à + 0,45 m (géométrie de ville.cpp et mobilier) : écart ≤ 0,2 m, 3 fois moins que le rayon du halo
    static void haloTete(Renderer& r, Track& t, double s, const Lampadaire& l, const vec3& tete) {
        const TrackPoint hq = t.point(s, l.lat);
        r.halo({(float)hq.x, (float)hq.y + l.y + 0.25f, (float)hq.z}, HALO_TETE_M, tete * 1.6f);
    }

    // ---------- RG-04 : feux du joueur (dessiné juste après les systèmes par Game) ----------
    // WFS-109 (RG-07, RG-11, RG-15) : de jour comme de nuit ; feu stop allumé et éteint en 80 ms (RG-02)
    void feuxJoueur(Renderer& r, const GameView& g) {
        const Player& p = *g.player;
        stopK_ = feux_modele::transition(stopK_, p.braking, tFeux_ < 0 ? -1.f : (float)(g.time - tFeux_)); tFeux_ = g.time;
        FeuxVoiture fx; fx.nuit = actif(g); fx.joueur = true;   // WFS-226 : ses phares sont les deux projecteurs ci-dessus
        fx.stop = p.braking; fx.recul = p.reversing || p.speed < -1; fx.frein = stopK_;   // WFS-382 : ligne recollée au commentaire (feux stop et recul du joueur)
        r.feuxProchaineVoiture(fx);
    }
    float stopK_ = 0; double tFeux_ = -1;

    void shutdown(Renderer&) override { lampadaires().clear(); nuit_ = false; prec_.clear(); prec2_.clear(); phares::autres().clear(); }

private:
    bool nuit_ = false, ville_ = false, plage_ = false, campagne_ = false, circuit_ = false;
    std::vector<std::pair<int64_t, size_t>> proches_, derriere_;   // WFS-324 : derriere_ (lampadaires derrière, rétroviseurs)
    std::vector<phares::PhareAutre> prec_, prec2_;   // WFS-226 : voitures notées à l'image précédente et à l'avant-dernière
    std::vector<vec3> eclairants_;                   // WFS-226 : centres des voitures à vraie lumière cette image
    std::vector<std::pair<float, size_t>> tri_;
};

} // namespace

// après tous les décors et l'interface : la tache et les halos se trient avec les transparents ; feuxProchaineVoiture
// doit être le dernier appel avant le dessin de la voiture du joueur par Game
WFS_SYSTEM(Nuit, 950)

} // namespace wfs
