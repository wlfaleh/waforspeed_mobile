// WFS-136 (RG-08) : tracé 2 des 4 cartes — sections présentes (RG-06), longueur ±10 % du tracé 1, pente ≤ 9 %,
// rond-point refermé autour de son îlot sans saut de cap, épingles ≥ 150° sur ≤ 80 m, autoroute (longueur, rayon ≥ 600 m) ;
// trafic aux feux : arrêté au rouge, reparti au vert, voitures qui traversent, 0 choc entre voitures du trafic ;
// trafic déterministe ; records mis de côté (*_v1) ; duel refusé si les tracés diffèrent.
#include <cmath>
#include <iterator>
#include <cstdio>
#include <map>
#include <string>
#include "../src/game/duel.h"
#include "../src/game/sauvegarde.h"
#include "../src/game/trace_cartes.h"
#include "../src/game/traffic.h"
using namespace wfs;
using trace::Type;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

static int compte(const Track& t, Type ty) { int n = 0; for (const auto& s : t.sections()) n += s.type == ty; return n; }
static double longueurM(const trace::Section& s) { return (s.fin - s.debut) * SEG_M; }

// cap tourné et longueur d'une plage de segments
static double capTourne(Track& t, int64_t a, int64_t b) { return (t.pose(b).th - t.pose(a).th) * 180 / 3.14159265358979; }

static void verifierCarte(const char* id) {
    Track v1; v1.build(mapById(id));
    Track t; t.build(mapById(id), 2);
    const double L1 = v1.segmentCount() * SEG_M, L2 = t.segmentCount() * SEG_M;
    std::printf("%s : %.0f m (tracé 1 : %.0f m, %+.1f %%), %zu sections\n", id, L2, L1, (L2 / L1 - 1) * 100, t.sections().size());
    VERIFIE(std::abs(L2 / L1 - 1) <= 0.10, "%s : longueur %.0f m hors de ±10 %% de %.0f m", id, L2, L1);
    VERIFIE(t.version() == 2 && !t.sections().empty() && v1.sections().empty(), "%s : versions", id);
    // sections contiguës, du début à la fin du tour
    int64_t fin = 0; bool contigu = true;
    for (const auto& s : t.sections()) { contigu = contigu && s.debut == fin && s.fin > s.debut; fin = s.fin; }
    VERIFIE(contigu && fin == (int64_t)t.segmentCount(), "%s : sections contiguës", id);
    // pente ≤ 9 %, retour à l'altitude 0, cap continu (pas de saut d'un segment à l'autre)
    double pente = 0, saut = 0;   // saut : cap tourné par un segment (rad) ; 0,11 rad = un virage de 11 m de rayon
    for (size_t i = 0; i < t.segmentCount(); i++) {
        const Segment& g = t.segAt((int64_t)i);
        pente = std::max(pente, std::abs(g.y2 - g.y1) * M * ELEV / SEG_M);
        saut = std::max(saut, std::abs(t.pose((int64_t)i + 1).th - t.pose((int64_t)i).th));
    }
    VERIFIE(pente <= 0.09, "%s : pente %.1f %% > 9 %%", id, pente * 100);
    const double bout = std::abs(t.segAt((int64_t)t.segmentCount() - 1).y2) * M * ELEV;
    VERIFIE(bout < 0.01 && std::abs(t.segAt(0).y1) < 1e-9, "%s : altitude %.3f m au bouclage", id, bout);
    VERIFIE(saut < 0.11, "%s : saut de cap de %.3f rad sur un segment", id, saut);
    std::printf("  pente max %.1f %%, cap tourné par segment %.3f rad au plus, altitude au bouclage %.4f m\n", pente * 100, saut, bout);
    // rond-point : l'anneau (¾ de tour) tourne autour d'un îlot de 14 m ; cap de sortie = cap d'entrée ± 90° au plus
    for (const auto& s : t.sections()) {
        if (s.type == Type::RondPoint) {
            const double net = capTourne(t, s.debut, s.fin);
            // segments de l'anneau : courbure de l'anneau (rayon 14 m)
            double cx = 0, cz = 0; int n = 0;
            std::vector<std::pair<double, double>> pts;
            for (int64_t a = s.debut; a < s.fin; a++) if (std::abs(std::abs(t.segAt(a).curve) - SEG_M / (14 * CURVE_K)) < 0.5) {
                const Pose& p = t.pose(a); pts.push_back({p.x, p.z}); cx += p.x; cz += p.z; n++;
            }
            // centre : milieu de l'arc (moyenne des points du cercle complet pondérée, approchée par le centre de courbure)
            double dmin = 1e9, dmax = 0;
            if (n > 2) {
                const Pose& p = t.pose(s.debut + (s.fin - s.debut) / 2);
                const double r = 14, ox = p.x - std::cos(p.th) * r, oz = p.z - std::sin(p.th) * r;   // à gauche de la route
                for (auto [x, z] : pts) { const double d = std::hypot(x - ox, z - oz); dmin = std::min(dmin, d); dmax = std::max(dmax, d); }
            }
            const double tour = n * SEG_M / 14 * 180 / 3.14159265358979;
            std::printf("  rond-point : anneau %.0f°, rayon %.1f à %.1f m, cap de sortie %+.0f°\n", tour, dmin, dmax, net);
            VERIFIE(tour > 240 && dmin > 13 && dmax < 15.5 && std::abs(net) <= 91, "%s : rond-point mal refermé", id);
        }
        if (s.type == Type::Epingle || s.type == Type::Lacets) {
            // chaque épingle : ≥ 150° sur ≤ 80 m
            int64_t a = s.debut;
            while (a < s.fin) {
                if (std::abs(t.segAt(a).curve) < 1e-9) { a++; continue; }
                int64_t b = a; while (b < s.fin && std::abs(t.segAt(b).curve) > 1e-9) b++;
                const double ang = std::abs(capTourne(t, a, b)), lon = (b - a) * SEG_M;
                VERIFIE(ang >= 150 && lon <= 80, "%s : épingle de %.0f° sur %.0f m", id, ang, lon);
                a = b;
            }
        }
        if (s.autoroute && (s.type == Type::Autoroute) && s.rayon > 0) VERIFIE(s.rayon >= 600, "%s : courbe d'autoroute de %.0f m", id, s.rayon);
    }
    double aut = 0; for (const auto& s : t.sections()) if (s.autoroute) aut += longueurM(s);
    const std::string m = id;
    if (m == "ville") {
        int serres = 0; for (const auto& s : t.sections()) serres += s.type == Type::Virage && s.rayon <= 30 && std::abs(s.angle) >= 80;
        VERIFIE(serres == 3 && compte(t, Type::Feux) == 2 && compte(t, Type::RondPoint) == 1 && compte(t, Type::Pont) == 1, "ville : sections");
        VERIFIE(aut >= 1400 && aut <= 1600, "ville : autoroute de %.0f m", aut);
        double tun = 0; for (const auto& s : t.sections()) if (s.type == Type::Tunnel) tun += longueurM(s);
        VERIFIE(std::abs(tun - 300) < 5, "ville : tunnel de %.0f m", tun);
    } else if (m == "plage") {
        const trace::Section* l = nullptr; for (const auto& s : t.sections()) if (s.type == Type::Lacets) l = &s;
        VERIFIE(l && l->branches == 6 && std::abs(l->denivele - 60) < 1, "corniche : 6 lacets, +60 m");
        VERIFIE(compte(t, Type::Tunnel) == 1 && compte(t, Type::RondPoint) == 1 && compte(t, Type::Feux) == 1, "corniche : sections");
    } else if (m == "campagne") {
        const trace::Section* l = nullptr; for (const auto& s : t.sections()) if (s.type == Type::Lacets) l = &s;
        VERIFIE(l && l->branches == 4 && std::abs(l->denivele - 120) < 1, "campagne : col de 120 m à 4 épingles");
        VERIFIE(compte(t, Type::RondPoint) == 1 && compte(t, Type::Feux) == 1 && compte(t, Type::Pont) == 1, "campagne : sections");
        VERIFIE(aut >= 1900 && aut <= 2100, "campagne : autoroute de %.0f m", aut);
    } else {
        int ep = compte(t, Type::Epingle), ch = compte(t, Type::Chicane); bool depart = false, raid = false;
        for (const auto& s : t.sections()) { depart = depart || (s.type == Type::Feux && s.depart); raid = raid || (s.type == Type::Montee && s.rayon > 0 && std::abs(s.denivele - 25) < 1); }
        // ligne droite d'au moins 1 km (segments sans courbure consécutifs)
        double best = 0, cur = 0;
        for (size_t i = 0; i < t.segmentCount(); i++) { cur = std::abs(t.segAt((int64_t)i).curve) < 1e-9 ? cur + SEG_M : 0; best = std::max(best, cur); }
        VERIFIE(ep == 1 && ch == 2 && depart && raid && best >= 999 && compte(t, Type::RondPoint) == 0, "grand prix : sections (épingle %d, chicanes %d, ligne %.0f m)", ep, ch, best);
    }
    // API du décor
    const trace::Section* s0 = trace::sectionA(t, 5 * SEG); const trace::Section* sn = trace::sectionA(t, t.length() - 1);
    VERIFIE(s0 == &t.sections().front() && sn == &t.sections().back() && trace::sectionA(v1, 0) == nullptr, "%s : sectionA", id);
}

// trafic de la Ville aux feux pendant 120 s (mode exact, joueur loin des feux)
static void verifierFeux() {
    Track t; t.build(mapById("ville"), 2);
    Traffic tr; tr.parallel = false;
    TrafficView pv; pv.position = t.length() / 2 + 400 / M; pv.speed = 0; pv.myLen = 4.3 / M; pv.garage = true;
    tr.reset(t, 1234, mapById("ville").trafficN, pv.position);
    int traversent = 0, maxFile = 0, vusTraverser = 0; int chocs = 0, passesRouge = 0; bool repartis = false;
    std::map<const TrafficCar*, double> avant;
    std::vector<const trace::Section*> F; for (const auto& s : t.sections()) if (s.feu >= 0) F.push_back(&s);
    int arretesAuRouge = 0;
    for (int k = 0; k < 120 * 60; k++) {
        tr.advance(T_STEP, pv);
        for (const trace::Section* s : F) {
            double tp = 0; const trace::Feu f = trace::feuCarrefour(s->feu, tr.sim, &tp);
            const double ligne = s->ligneArret * SEG;
            int file = 0;
            for (const auto& c : tr.cars) {
                if (c.cache || c.crashed) continue;
                if (c.traverse == s->feu) { if (std::abs(c.offset) < 1) vusTraverser++; continue; }
                if (c.traverse >= 0) continue;
                const double nez = t.relZ(c.z + c.len / 2 - ligne);   // avant de la voiture par rapport à la ligne
                if (f == trace::Feu::Rouge && nez < 0 && nez > -60 / M && c.speed * M < 0.5) file++;
                const auto it = avant.find(&c);
                if (it != avant.end() && it->second < 0 && nez >= 0 && nez < 5 / M && f == trace::Feu::Rouge && tp > 1.0) passesRouge++;
                avant[&c] = nez;
                if (f == trace::Feu::Vert && tp > 6 && nez < 0 && nez > -40 / M && c.speed * M > 5) repartis = true;
            }
            maxFile = std::max(maxFile, file);
            if (f == trace::Feu::Rouge && file > 0) arretesAuRouge++;
        }
        // chocs entre voitures du trafic près des carrefours (rectangles, voitures qui traversent de profil)
        if (k % 6 == 0) for (size_t i = 0; i < tr.cars.size(); i++) for (size_t j = i + 1; j < tr.cars.size(); j++) {
            const TrafficCar &a = tr.cars[i], &b = tr.cars[j];
            if (a.cache || b.cache || (a.traverse < 0 && b.traverse < 0)) continue;   // chocs aux feux : une voiture qui traverse
            auto ext = [](const TrafficCar& c, double& lx, double& lz) {
                const double L = c.len * M, W = c.w * 2 * RW;
                if (c.traverse >= 0) { lx = L / 2; lz = W / 2; } else { lx = W / 2; lz = L / 2; }
            };
            double ax, az, bx, bz; ext(a, ax, az); ext(b, bx, bz);
            if (std::abs((a.offset - b.offset) * RW) < ax + bx && std::abs(t.relZ(a.z - b.z) * M) < az + bz) chocs++;
        }
    }
    for (const auto& c : tr.cars) traversent += c.traverse >= 0;
    std::printf("feux : %d voiture(s) qui traversent, %d image(s) vues en travers, file de %d au plus, %d passage(s) au rouge, %d choc(s)\n",
                traversent, vusTraverser, maxFile, passesRouge, chocs);
    VERIFIE(traversent >= 2 && traversent <= 4 && vusTraverser > 0, "feux : voitures qui traversent");
    VERIFIE(arretesAuRouge > 0 && maxFile >= 1, "feux : aucune voiture arrêtée au rouge");
    VERIFIE(passesRouge == 0, "feux : %d voiture(s) passée(s) au rouge", passesRouge);
    VERIFIE(repartis, "feux : la file ne repart pas au vert");
    VERIFIE(chocs == 0, "feux : %d choc(s) entre voitures du trafic", chocs);
    // feux : cycle 14 / 3 / 10 s
    VERIFIE(trace::feuCarrefour(0, 0) == trace::Feu::Vert && trace::feuCarrefour(0, 15) == trace::Feu::Orange && trace::feuCarrefour(0, 20) == trace::Feu::Rouge
            && trace::feuCarrefour(0, 27.5) == trace::Feu::Vert && trace::feuCarrefour(1, 20 - 9) == trace::Feu::Rouge, "feux : cycle");
}

static void verifierDeterminisme() {
    auto run = [](uint32_t seed) {
        static Track t; t.build(mapById("campagne"), 2);
        Traffic tr; tr.parallel = false; TrafficView pv; pv.speed = MAX_SPEED * 0.4; pv.myLen = 4.3 / M;
        tr.reset(t, seed, mapById("campagne").trafficN, 0);
        for (int k = 0; k < 30 * 60; k++) tr.advance(T_STEP, pv);
        std::string s; char b[64];
        for (const auto& c : tr.cars) { std::snprintf(b, sizeof b, "%.6f %.6f %.6f;", c.z, c.offset, c.speed); s += b; }
        return s;
    };
    VERIFIE(run(77) == run(77) && run(77) != run(78), "trafic : même graine, même trafic");
    // autoroute : +30 % de voitures au prorata
    Track t; t.build(mapById("campagne"), 2); Traffic tr; tr.reset(t, 5, 100, 0);
    VERIFIE((int)tr.cars.size() > 100, "autoroute : voitures en plus (%zu)", tr.cars.size());
    // limites de vitesse du trafic
    for (const auto& s : t.sections()) if (s.type == Type::RondPoint)
        VERIFIE(std::abs(trace::vitesseTrafic(t, (s.debut + s.fin) / 2 * SEG) * M * 3.6 - 40) < 0.5, "trafic : 40 km/h dans l'anneau");
}

static void verifierRecords() {
    Sauvegarde s;
    VERIFIE(s.lire("version=1\nwaforspeed-best=48210\ncar=m4\nrecord-course5=2;123456\nrecord-poursuite=1;80;10000\n"), "lecture");
    VERIFIE(s.best == 0 && !s.aRecordCourse5() && !s.aRecordPoursuite() && s.nouveauxTraces && s.car == "m4", "records mis de côté");
    VERIFIE(s.anciens["best_v1"] == "48210" && s.anciens["record-course5_v1"] == "2;123456" && s.anciens["record-poursuite_v1"] == "1;80;10000", "clés *_v1");
    VERIFIE(s.cartesAActiver.size() == std::size(trace::CARTES_ACTIVES), "cartes à activer");
    s.activerCartes();
    const std::string tx = s.texte();
    VERIFIE(tx.find("trace=2\n") != std::string::npos && tx.find("best_v1=48210\n") != std::string::npos
            && tx.find("trace-circuit=2\n") != std::string::npos, "écriture trace=2, trace-circuit=2 et *_v1");
    // A7.4 : une carte de plus activée plus tard (sauvegarde sans sa clé) : toast, mais records pas remis à zéro
    Sauvegarde w; w.lire("version=1\ntrace=2\nwaforspeed-best=700\n");
    VERIFIE(w.best == 700 && w.nouveauxTraces && w.cartesAActiver.size() == std::size(trace::CARTES_ACTIVES), "nouvelle carte : records gardés");
    // A7.1 : jeu normal : Ville au tracé 1, Grand Prix au tracé 2 ; banc : forcé partout
    VERIFIE(trace::versionCarte("ville") == (trace::carteActive("ville") ? 2 : 1) && trace::versionCarte("circuit") == 2 && trace::signature() == (trace::carteActive("ville") ? "circuit,ville" : "circuit"), "cartes actives");   // WFS-137 : ville activée
    trace::forcerTrace2(true); VERIFIE(trace::versionCarte("ville") == 2, "tracé 2 forcé au banc"); trace::forcerTrace2(false);
    Sauvegarde r; r.lire(tx); r.best = 900;
    VERIFIE(!r.nouveauxTraces && r.anciens["best_v1"] == "48210" && r.best == 900, "relecture : pas de 2e mise de côté");
    Sauvegarde z; z.lire(r.texte());
    VERIFIE(z.best == 900 && !z.nouveauxTraces, "record du tracé 2 gardé");
}

static void verifierDuel() {
    using namespace duel;
    auto salon = std::make_shared<SalonMemoire>();
    Session a;
    VERIFIE(a.join(std::make_unique<TransportMemoire>(salon, "A"), "k7qx", true, "Alice", "golf", "", "ville"), "join");
    // adversaire d'une ancienne version : présence sans champ « trace »
    salon->injecter("V", encode({{"n", "Vieux"}, {"st", "ready"}, {"r", "0"}, {"car", "golf"}}));
    a.setReady(true);
    bool parti = false; for (int i = 0; i < 20; i++) { salon->now += 100; parti = parti || a.checkStart(); }
    VERIFIE(!parti, "duel : pas de course contre une autre version du tracé");
    salon->injecter("V", encode({{"n", "Vieux"}, {"st", "ready"}, {"r", "0"}, {"car", "golf"}, {"trace", trace::signature()}}));
    for (int i = 0; i < 20 && !parti; i++) { salon->now += 100; parti = a.checkStart(); }
    VERIFIE(parti, "duel : départ avec le même tracé");
}

int main() {
    for (const char* id : {"ville", "plage", "campagne", "circuit"}) verifierCarte(id);
    verifierFeux();
    verifierDeterminisme();
    verifierRecords();
    verifierDuel();
    if (echecs) { std::printf("test_trace : %d échec(s)\n", echecs); return 1; }
    std::printf("test_trace : OK\n");
    return 0;
}
