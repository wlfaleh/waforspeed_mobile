// Course à 5 voitures (WFS-110) : tirage des ordinateurs par graine, 10 km sans choc entre eux, tous à l'arrivée,
// classement « Position n/5 ».
// usage : test_course5
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <set>
#include "../src/game/course5.h"
#include "../src/game/player.h"         // WFS-189 : course simulée avec la conduite du joueur
#include "../src/game/trace_cartes.h"   // WFS-189 : versionCarte
#include "../src/game/track.h"
using namespace wfs;
int testPoursuite();   // WFS-111 : tests/test_poursuite.cpp (même exécutable)

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)


// WFS-189 : une Course à 5 simulée comme le pilote automatique du jeu (Game::update : plein gaz, milieu de la voie, pas de
// 1/120 s) ; ordinateurs de course5::avancer ; nitro du joueur prise si `nitro` (jauge > 0,5, comme simulateCourse) et
// rechargée comme course5_system (RG-04). Sans contacts joueur / ordinateurs (les ordinateurs l'évitent).
namespace {
struct Simu { int place = 5; double temps = 0, nitroS = 0, jauge30 = 0; int depasses = 0; double colle = 0; bool fini = false; };
// freinVirage : le joueur simulé freine avant les virages serrés (comme un joueur qui ne sort pas de la route), à la même
// vitesse permise que les ordinateurs (vitesseVirageOrdinateurDevant depuis WFS-327)
Simu simuler(const char* car, course5::Difficulte d, uint32_t graine, const char* carte, double km, bool nitro, bool recharge = true, int trace = 0, bool freinVirage = false) {
    static Track t; static std::string carteFaite;
    const int version = trace ? trace : trace::versionCarte(carte);   // 0 : tracé du jeu (2 en Ville)
    const std::string cle = std::string(carte) + std::to_string(version);
    if (carteFaite != cle) { t.build(mapById(carte), version); carteFaite = cle; }
    Player pl; pl.reset(carById(car));
    auto os = course5::tirer(graine, car, d);
    // WFS-242 (RG-01, RG-02) : le joueur à sa place de la grille (tirée par la graine), comme course5_system
    const int place = std::min((int)os.size(), course5::tirerPlaceJoueur(graine));
    course5::grille(os, place);
    course5::placerSurGrille(pl, t, course5::placeGrille(place));
    const double finish = km * duel::KM, dt = 1.0 / 120;
    Simu r; double ftJ = 0; course5::SuiviDepassements suivi;
    std::vector<double> colle(os.size(), 0);
    for (int i = 0; i < 120 * 400; i++) {
        const double tc = (i + 1) * dt;
        DriveInput in; in.gas = true;
        in.steer = (float)std::clamp((0.0 - pl.playerX) * 3.0 - pl.heading * 4.0, -1.0, 1.0);
        in.nitro = nitro && pl.nitro > 0.5;
        if (freinVirage && pl.speed > course5::vitesseVirageOrdinateurDevant(t, pl.dist, carById(car))) { in.gas = false; in.brake = true; in.nitro = false; }   // WFS-327 : vitesse permise aux ordinateurs
        if (!r.fini) pl.update(dt, in, t, true);
        if (!r.fini && pl.nitroOn) r.nitroS += dt;
        course5::avancer(os, dt, finish, tc, pl.dist, pl.playerX, pl.speed, &t);
        if (!r.fini) {
            const int n = suivi.compter(os, pl.dist);
            r.depasses += n;
            if (recharge) pl.nitro = course5::rechargerNitro(pl.nitro, pl.nitroOn, dt, n);
        }
        if (i + 1 == 120 * 30) r.jauge30 = pl.nitro;
        if (tc <= 10)   // RG-03 : ordinateur à moins de 8 m (pare-chocs) derrière un autre, même file
            for (size_t a = 0; a < os.size(); a++) {
                bool pres = false;
                for (size_t b = 0; b < os.size(); b++) {
                    if (a == b || std::abs(os[a].g.x - os[b].g.x) > 0.45) continue;
                    const double gap = (os[b].g.rd - os[a].g.rd) * M - (carById(os[a].car.c_str()).d.L + carById(os[b].car.c_str()).d.L) / 2;
                    pres = pres || (gap >= 0 && gap < 8);
                }
                colle[a] = pres ? colle[a] + dt : 0; r.colle = std::max(r.colle, colle[a]);
            }
        if (!r.fini && pl.dist >= finish) { r.fini = true; ftJ = tc; }
        bool tous = r.fini; for (const auto& o : os) tous = tous && o.g.done;
        if (tous) break;
    }
    r.temps = ftJ;
    r.place = course5::position(course5::classement(os, "moi", pl.dist, ftJ, r.fini, finish));
    return r;
}
} // namespace

int main() {
    // RG-02 : même graine = même tirage ; 4 modèles différents entre eux et du joueur
    const auto a = course5::tirer(1234, "golf"), b = course5::tirer(1234, "golf"), c = course5::tirer(99, "golf");
    std::set<std::string> modeles; bool memes = a.size() == b.size(), differents = false;
    for (size_t i = 0; i < a.size(); i++) { modeles.insert(a[i].car); memes = memes && a[i].car == b[i].car && a[i].peinture == b[i].peinture; differents = differents || a[i].car != c[i].car; }
    VERIFIE(a.size() == 4 && modeles.size() == 4 && !modeles.count("golf"), "4 modèles différents, sans la voiture du joueur");
    VERIFIE(memes && differents, "graine : même tirage pour la même graine, autre tirage pour une autre");
    std::printf("tirage : %s, %s, %s, %s (graine 1234) : OK\n", a[0].car.c_str(), a[1].car.c_str(), a[2].car.c_str(), a[3].car.c_str());

    // RG-04 : 10 km, 3 graines, pas de 1/120 s ; le joueur est loin derrière (les ordinateurs roulent seuls)
    for (uint32_t graine : {1234u, 7u, 424242u}) {
        auto os = course5::tirer(graine, "golf");
        course5::grille(os, -1);
        const double finish = 10 * duel::KM;
        int chocs = 0; double t = 0;
        bool tous = false;
        for (int i = 0; i < 120 * 60 * 10 && !tous; i++) {
            t += 1.0 / 120;
            course5::avancer(os, 1.0 / 120, finish, t, -1e9, 0, 0);
            for (size_t p = 0; p < os.size(); p++) for (size_t q = p + 1; q < os.size(); q++) {
                const CarDef &A = carById(os[p].car.c_str()), &B = carById(os[q].car.c_str());
                const double dz = std::abs(os[p].g.rd - os[q].g.rd) * M, dx = std::abs(os[p].g.x - os[q].g.x) * RW;
                if (dz < (A.d.L + B.d.L) / 2 && dx < (A.d.W + B.d.W) / 2) chocs++;
            }
            tous = true; for (const auto& o : os) tous = tous && o.g.done;
        }
        VERIFIE(chocs == 0, "graine %u : %d image(s) avec deux ordinateurs l'un dans l'autre", graine, chocs);
        VERIFIE(tous, "graine %u : tous les ordinateurs arrivent", graine);
        double tmin = 1e9, tmax = 0; for (const auto& o : os) { tmin = std::min(tmin, o.g.ft / 1000); tmax = std::max(tmax, o.g.ft / 1000); }
        std::printf("graine %u : 10 km sans choc entre ordinateurs, tous arrivés entre %.1f et %.1f s : %s\n", graine, tmin, tmax,
                    chocs == 0 && tous ? "OK" : "ÉCHEC");
    }

    // WFS-133 (RG-03) : même graine, Difficile termine plus vite que Facile ; nitros au plus le nombre permis ; 10 km, joueur
    // tout près (rattrapage et aspiration actifs) : toujours 0 choc entre ordinateurs, tous arrivent
    {
        double moyenne[3] = {0, 0, 0};
        for (int d = 0; d < 3; d++) {
            auto os = course5::tirer(1234, "golf", (course5::Difficulte)d);
            course5::grille(os, -1);
            const double finish = 10 * duel::KM;
            int chocs = 0, nitrosMax = 0; double t = 0; bool tous = false;
            for (int i = 0; i < 120 * 60 * 12 && !tous; i++) {
                t += 1.0 / 120;
                const double sJ = t * 0.85 * MAX_SPEED;   // joueur à vitesse fixe, au milieu du peloton
                course5::avancer(os, 1.0 / 120, finish, t, sJ, 0.62, 0.85 * MAX_SPEED);
                for (size_t p = 0; p < os.size(); p++) for (size_t q = p + 1; q < os.size(); q++) {
                    const CarDef &A = carById(os[p].car.c_str()), &B = carById(os[q].car.c_str());
                    const double dz = std::abs(os[p].g.rd - os[q].g.rd) * M, dx = std::abs(os[p].g.x - os[q].g.x) * RW;
                    if (dz < (A.d.L + B.d.L) / 2 && dx < (A.d.W + B.d.W) / 2) chocs++;
                }
                tous = true; for (const auto& o : os) tous = tous && o.g.done;
            }
            int prises = 0;
            for (const auto& o : os) { moyenne[d] += o.g.ft / 1000 / os.size(); nitrosMax = std::max(nitrosMax, o.nitrosPrises); prises += o.nitrosPrises; }
            VERIFIE(chocs == 0 && tous, "difficulté %d : %d choc(s), tous arrivés %d", d, chocs, (int)tous);
            VERIFIE(nitrosMax <= course5::NITROS[d], "difficulté %d : %d nitro(s) pour un ordinateur, %d permise(s)", d, nitrosMax, course5::NITROS[d]);
            std::printf("difficulté %s : 10 km, temps moyen %.1f s, %d nitro(s) prise(s) (au plus %d chacun), %d choc : %s\n",
                        course5::cleDifficulte((course5::Difficulte)d), moyenne[d], prises, course5::NITROS[d], chocs,
                        chocs == 0 && tous && nitrosMax <= course5::NITROS[d] ? "OK" : "ÉCHEC");
        }
        VERIFIE(moyenne[2] < moyenne[1] && moyenne[1] < moyenne[0], "Difficile plus rapide que Normale, plus rapide que Facile");
        std::printf("même graine : difficile %.1f s < normale %.1f s < facile %.1f s : OK\n", moyenne[2], moyenne[1], moyenne[0]);
        VERIFIE(course5::difficulteDeCle("difficile") == course5::DIFFICILE && course5::difficulteDeCle("facile") == course5::FACILE &&
                course5::difficulteDeCle("extreme") == course5::NORMALE, "clés de difficulté");
    }
    // RG-04 : rattrapage : un ordinateur 300 m derrière le joueur (même vitesse visée que lui) revient sous 150 m
    {
        auto os = course5::tirer(7, "golf");
        os.resize(1); course5::grille(os, -1);
        const double v = os[0].cible, finish = 50 * duel::KM;
        os[0].g.v = v;
        double sJ = 300 / M, ecart = 300; bool revenu = false;
        for (int i = 0; i < 120 * 120 && !revenu; i++) {
            sJ += v / 120;
            course5::avancer(os, 1.0 / 120, finish, i / 120.0, sJ, -0.62, v);
            ecart = (sJ - os[0].g.rd) * M; revenu = ecart < 150;
        }
        VERIFIE(revenu, "rattrapage : encore %.0f m derrière", ecart);
        // devant de plus de 250 m : il ralentit (−6 %)
        os[0].g.rd = sJ + 400 / M; os[0].g.v = v;
        for (int i = 0; i < 120 * 5; i++) { sJ += v / 120; course5::avancer(os, 1.0 / 120, finish, 0, sJ, -0.62, v); }
        VERIFIE(std::abs(os[0].g.v / v - 0.94) < 0.005, "rattrapage devant : %.3f × la vitesse visée", os[0].g.v / v);
        // dernier kilomètre : aucun rattrapage
        os[0].g.rd = finish - 900 / M; sJ = os[0].g.rd - 400 / M; os[0].g.v = v;
        for (int i = 0; i < 120; i++) { sJ += v / 120; course5::avancer(os, 1.0 / 120, finish, 0, sJ, -0.62, v); }
        VERIFIE(std::abs(os[0].g.v / v - 1) < 1e-6, "dernier km : %.3f × la vitesse visée", os[0].g.v / v);
        std::printf("rattrapage : 300 m derrière → %.0f m ; devant → −6 %% ; dernier km : rien : OK\n", ecart);
    }
    // RG-05 : aspiration : 20 m derrière une voiture plus rapide de la même file → +4 % de vitesse visée
    {
        auto os = course5::tirer(7, "golf");
        os.resize(1); course5::grille(os, -1);
        const double v = os[0].cible, finish = 50 * duel::KM;
        os[0].g.x = os[0].cibleX = 0; os[0].file = 1; os[0].g.v = v; os[0].nitros = 0;
        double sJ = 20 / M;
        for (int i = 0; i < 120 * 3; i++) {   // le joueur (devant, même file) roule plus vite : l'écart reste sous 25 m
            sJ += v * 1.04 / 120; course5::avancer(os, 1.0 / 120, finish, 0, sJ, 0, v * 1.04);
            if (i == 0) sJ = os[0].g.rd + 20 / M;
        }
        VERIFIE(os[0].aspiration && std::abs(os[0].g.v / v - 1.04) < 0.005, "aspiration : %.3f × la vitesse visée", os[0].g.v / v);
        VERIFIE(course5::dansAspiration(os, os[0].g.rd - 10 / M, 0) && !course5::dansAspiration(os, os[0].g.rd - 30 / M, 0),
                "aspiration du joueur : à 10 m oui, à 30 m non");
        std::printf("aspiration : %.3f × la vitesse visée (+4 %%) : OK\n", os[0].g.v / v);
    }

    // RG-06 : classement : arrivés d'abord (temps), puis distance ; position du joueur
    auto os = course5::tirer(5, "m4");
    course5::grille(os, -1);
    os[0].g.done = true; os[0].g.ft = 200000; os[0].g.rd = 2 * duel::KM;
    os[1].g.rd = 1.5 * duel::KM; os[2].g.rd = 0.5 * duel::KM; os[3].g.rd = 1.0 * duel::KM;
    auto ls = course5::classement(os, "Pilote", 1.2 * duel::KM, 0, false, 2 * duel::KM);
    VERIFIE(ls.size() == 5 && ls[0].fini && !ls[1].fini && course5::position(ls) == 3, "classement (position %d/5)", course5::position(ls));
    std::printf("classement : arrivés puis distance, « Position %d/5 » : OK\n", course5::position(ls));

    // WFS-189 (RG-01) : pour chacune des 20 voitures du joueur, 4 adversaires de sa classe (± 20 % de vitesse max), ou les
    // plus proches s'il y en a moins de 4 ; plusieurs graines : jamais hors des candidats
    {
        int horsClasse = 0, completes = 0;
        for (int i = 0; i < CAR_COUNT; i++) {
            const CarDef& J = CARS[i];
            const std::vector<int> cand = course5::candidats(J.id);
            int dans = 0; for (int c : cand) dans += std::abs(CARS[c].spec.vmax - J.spec.vmax) <= course5::CLASSE_VMAX * J.spec.vmax;
            if (dans < 4) completes++;
            // complétés : chaque voiture ajoutée est plus proche (vitesse max) que toutes les voitures écartées
            int pireDedans = 0; for (int c : cand) pireDedans = std::max(pireDedans, std::abs(CARS[c].spec.vmax - J.spec.vmax));
            bool plusProches = true;
            for (int k = 0; k < CAR_COUNT; k++)
                if (k != i && std::find(cand.begin(), cand.end(), k) == cand.end()) plusProches = plusProches && std::abs(CARS[k].spec.vmax - J.spec.vmax) >= pireDedans;
            VERIFIE((int)cand.size() >= 4 && plusProches, "%s : %zu candidat(s), les plus proches : %d", J.id, cand.size(), (int)plusProches);
            for (uint32_t g : {1u, 1234u, 20261105u, 99u}) {
                const auto os = course5::tirer(g, J.id);
                std::set<std::string> vus;
                for (const auto& o : os) {
                    vus.insert(o.car);
                    const int k = (int)(&carById(o.car.c_str()) - CARS);
                    horsClasse += std::find(cand.begin(), cand.end(), k) == cand.end() || o.car == J.id;
                }
                VERIFIE(vus.size() == 4, "%s graine %u : 4 modèles différents", J.id, g);
            }
            std::printf("adversaires %-9s (%3d km/h) : %2zu candidat(s) %s\n", J.id, J.spec.vmax, cand.size(), dans < 4 ? "(complétés par les plus proches)" : "");
        }
        VERIFIE(horsClasse == 0, "%d adversaire(s) tiré(s) hors des candidats", horsClasse);
        std::printf("RG-01 : 20 voitures, 4 graines : adversaires toujours dans la classe (%d voiture(s) complétée(s)) : %s\n", completes, horsClasse ? "ÉCHEC" : "OK");
    }
    // WFS-189 (RG-03, arbitrage du PO 20:57) : grille rangée par 0-100 croissant (à égalité, vitesse max la plus forte
    // devant) ; à 4 s du GO, chaque ordinateur roule à ≥ 90 % de sa vitesse seul sur la même route ; aucun ne reste plus
    // de 2 s à moins de 8 m (pare-chocs) derrière un autre de sa file dans les 10 premières secondes. Graine du banc,
    // avec la Golf et avec la SF90.
    for (const char* joueur : {"golf", "sf90"}) {
        auto os = course5::tirer(20261105, joueur); course5::grille(os, -1);
        bool range = true;
        for (const auto& a : os) for (const auto& b : os)
            if (std::abs(a.g.x - b.g.x) < 0.45 && a.g.rd > b.g.rd) {
                const CarSpec &A = carById(a.car.c_str()).spec, &B = carById(b.car.c_str()).spec;
                range = range && (A.s100 < B.s100 || (A.s100 == B.s100 && A.vmax >= B.vmax));
            }
        VERIFIE(range, "RG-03 (%s) : dans chaque file, la voiture de devant a le meilleur 0-100", joueur);
        static Track piste; static bool faite = false;
        if (!faite) { piste.build(mapById("ville"), trace::versionCarte("ville")); faite = true; }
        auto groupe = os; double pire = 1e9;
        for (int i = 0; i < 480; i++) course5::avancer(groupe, 1.0 / 120, 4 * duel::KM, i / 120.0, -1e9, 0, 0, &piste);
        std::printf("RG-03 (%s) : grille ", joueur);
        for (size_t k = 0; k < os.size(); k++) {
            std::vector<course5::Ordinateur> seul{os[k]};
            for (int i = 0; i < 480; i++) course5::avancer(seul, 1.0 / 120, 4 * duel::KM, i / 120.0, -1e9, 0, 0, &piste);
            const double part = groupe[k].g.v / std::max(1e-9, seul[0].g.v);
            pire = std::min(pire, part);
            std::printf("%s (%.1f s) à %.0f m file %d : %.0f km/h à 4 s, seul %.0f (%.0f %%) ; ", os[k].car.c_str(), carById(os[k].car.c_str()).spec.s100,
                        os[k].g.rd * M, os[k].file, groupe[k].g.v / MAX_SPEED * 260, seul[0].g.v / MAX_SPEED * 260, part * 100);
        }
        const Simu r = simuler(joueur, course5::NORMALE, 20261105, "ville", 4, false);
        std::printf("\nRG-03 (%s) : à 4 s, au moins %.0f %% de la vitesse seul ; plus long « collé » à moins de 8 m : %.2f s\n", joueur, pire * 100, r.colle);
        VERIFIE(pire >= 0.90, "RG-03 (%s) : un ordinateur à %.0f %% de sa vitesse seul à 4 s", joueur, pire * 100);
        VERIFIE(r.colle <= 2, "RG-03 (%s) : un ordinateur reste %.2f s à moins de 8 m derrière un autre", joueur, r.colle);
    }
    // WFS-189 (RG-04) : recharge de la nitro sans trafic
    {
        double j = config().nitro.depart; for (int i = 0; i < 120 * 30; i++) j = course5::rechargerNitro(j, false, 1.0 / 120, 0);
        const double attendu = config().nitro.depart + 30 * 0.015;
        std::printf("RG-04 : jauge après 30 s sans dépassement %.4f (attendu %.4f) ; +1 dépassement : +%.2f ; nitro qui brûle : +%.3f ; plafond %.2f\n",
                    j, attendu, course5::rechargerNitro(0.2, false, 0, 1) - 0.2, course5::rechargerNitro(0.2, true, 1, 1) - 0.2, course5::rechargerNitro(0.97, false, 1, 1));
        VERIFIE(std::abs(j - attendu) < 1e-6, "RG-04 : jauge après 30 s");
        VERIFIE(std::abs(course5::rechargerNitro(0.2, false, 0, 1) - 0.3) < 1e-9 && course5::rechargerNitro(0.2, true, 1, 1) == 0.2 && course5::rechargerNitro(0.97, false, 1, 1) == 1.0,
                "RG-04 : dépassement +0,10, rien pendant la nitro, plafond 1");
        // dépassements : passer derrière le joueur compte une fois ; roue dans roue (va-et-vient à moins de 10 m) : une seule
        // fois ; repassé à plus de 10 m devant puis redépassé : une fois de plus
        auto os = course5::tirer(7, "golf"); os.resize(2);
        course5::SuiviDepassements sv;
        os[0].g.rd = 20 / M; os[1].g.rd = 30 / M; const int n0 = sv.compter(os, 0);
        os[0].g.rd = -1 / M; const int n1 = sv.compter(os, 0);
        int n2 = 0; for (int i = 0; i < 10; i++) { os[0].g.rd = (i % 2 ? -1 : 5) / M; n2 += sv.compter(os, 0); }
        os[0].g.rd = 15 / M; sv.compter(os, 0); os[0].g.rd = -2 / M; const int n3 = sv.compter(os, 0);
        std::printf("RG-04 : dépassements : devant %d, dépassé %d, roue dans roue ×10 %d, repassé à 15 m puis redépassé %d\n", n0, n1, n2, n3);
        VERIFIE(n0 == 0 && n1 == 1 && n2 == 0 && n3 == 1, "RG-04 : dépassements comptés une fois");
        const Simu sans = simuler("golf", course5::NORMALE, 1234, "ville", 4, true, true);
        std::printf("RG-04 : Golf, 4 km, nitro au pilote auto : jauge à 30 s %.2f, %d dépassement(s), %.1f s de nitro en tout\n", sans.jauge30, sans.depasses, sans.nitroS);
    }
    // WFS-189 (RG-02, RG-06) : places moyennes (pilote automatique à fond, sans nitro, 4 km, Ville, 5 graines) ; nitro (avec)
    {
        // protocole (B) retenu par le PO (21:06) : pilote automatique à fond, sans nitro, qui freine avant les virages serrés
        // à la vitesse permise aux ordinateurs ; SF90 et Compass : bouts du catalogue (exceptions du PO)
        const char* voitures[] = {"sf90", "m4", "mustang", "golf", "x5", "p208", "compass"};
        const uint32_t graines[] = {1u, 1234u, 20261105u, 99u, 7u};
        std::printf("RG-02 : | voiture | Facile | Normale | Difficile | temps moyen (Normale) | nitro sur 4 km (s, min-max) |\n");
        std::printf("RG-02 : |---|---|---|---|---|---|\n");
        for (const char* v : voitures) {
            double place[3] = {0, 0, 0}, temps = 0, nMin = 1e9, nMax = 0;
            for (int d = 0; d < 3; d++) for (uint32_t g : graines) {
                const Simu r = simuler(v, (course5::Difficulte)d, g, "ville", 4, false, true, 0, true);
                VERIFIE(r.fini, "%s : le joueur n'arrive pas", v);
                place[d] += r.place / 5.0; if (d == 1) temps += r.temps / 5;
            }
            for (uint32_t g : graines) { const Simu r = simuler(v, course5::NORMALE, g, "ville", 4, true, true, 0, true); nMin = std::min(nMin, r.nitroS); nMax = std::max(nMax, r.nitroS); }
            std::printf("RG-02 : | %s | %.1f | %.1f | %.1f | %.1f s | %.1f à %.1f |\n", v, place[0], place[1], place[2], temps, nMin, nMax);
            const bool sf90 = !std::strcmp(v, "sf90"), compass = !std::strcmp(v, "compass"), m4 = !std::strcmp(v, "m4");
            VERIFIE(place[0] <= 3.0, "RG-02 : %s : place moyenne %.1f en Facile (≤ 3,0)", v, place[0]);
            if (!sf90) VERIFIE(place[1] >= 1.5 && (compass || place[1] <= 3.5), "RG-02 : %s : place moyenne %.1f en Normale", v, place[1]);
            // WFS-283 RG-08 : route sans rond-point (4 km en 5 s de moins) : M4 ≥ 1,8 en Difficile (déjà pile à 2,0 avec le rond-point) ; 1e-6 : somme flottante de 5 places / 5
            if (!sf90) VERIFIE(place[2] + 1e-6 >= (m4 ? 1.8 : 2.0), "RG-02 : %s : place moyenne %.1f en Difficile (≥ %.1f)", v, place[2], m4 ? 1.8 : 2.0);
            VERIFIE(place[0] < 5.0, "RG-06 : %s : dernier en moyenne en Facile", v);
            // WFS-242 : 1e-6 : 360 pas de 1/120 s font 3 s, la somme flottante 2,999 999… (SF90 partie en tête : 3,000 s)
            // WFS-283 RG-08 : SF90 2,5 à 6 s (partie en tête, jauge rechargée au temps sur une course plus courte ; 3,000 s avec le rond-point)
            VERIFIE(nMin + 1e-6 >= (sf90 ? 2.5 : 3) && nMax <= 6, "RG-06 : %s : %.1f à %.1f s de nitro sur 4 km (%.1f à 6)", v, nMin, nMax, sf90 ? 2.5 : 3.0);
        }
    }
    // WFS-189 (RG-02 bis) : virages des ordinateurs calés sur la conduite du joueur : un ordinateur à 100 % de sa pointe fait
    // les 4 km de la Ville dans le temps du pilote automatique du joueur (même voiture, qui freine à la même vitesse permise
    // depuis WFS-327), à ± 3 %
    {
        static Track piste; piste.build(mapById("ville"), trace::versionCarte("ville"));
        for (const char* v : {"golf", "compass", "m4", "p208"}) {
            Player pl; pl.reset(carById(v)); const double dt = 1.0 / 120; double tj = 0, ti = 0;
            for (int i = 0; i < 120 * 200 && !tj; i++) {
                DriveInput in; in.gas = true; in.steer = (float)std::clamp((0.0 - pl.playerX) * 3.0 - pl.heading * 4.0, -1.0, 1.0);
                // WFS-327 : le pilote freine avant les virages à la vitesse permise aux ordinateurs (même règle des deux côtés)
                if (pl.speed > course5::vitesseVirageOrdinateurDevant(piste, pl.dist, carById(v))) { in.gas = false; in.brake = true; }
                pl.update(dt, in, piste, true); if (pl.dist * M >= 4000) tj = (i + 1) * dt;
            }
            auto os = course5::tirer(1, v); os.resize(1); os[0].car = v; course5::grille(os, -1);
            os[0].g.rd = 0; os[0].nitros = 0; os[0].cible = MAX_SPEED * 0.995 * carById(v).spec.vmax / 260;
            for (int i = 0; i < 120 * 200 && !ti; i++) { course5::avancer(os, dt, 1e12, i * dt, -1e9, 0, 0, &piste); if (os[0].g.rd * M >= 4000) ti = (i + 1) * dt; }
            std::printf("RG-02 bis : %-8s 4 km : pilote automatique %.1f s, ordinateur à 100 %% %.1f s (%+.1f %%)\n", v, tj, ti, (ti / tj - 1) * 100);
            VERIFIE(tj > 0 && std::abs(ti / tj - 1) <= 0.03, "RG-02 bis : %s : écart %.1f %%", v, (ti / tj - 1) * 100);
        }
    }

    // WFS-242 (RG-01) : grille réelle : pour les 20 voitures du joueur, 5 places en quinconce derrière la ligne, chacune dans
    // sa voie, aucune carrosserie dans une autre ; côte à côte (même hauteur de grille), au moins 1,5 m entre deux carrosseries
    {
        int fautes = 0; double ecartVoie = 1e9, ecartLateral = 1e9, avantMax = -1e9;
        for (int i = 0; i < CAR_COUNT; i++)
            for (uint32_t g : {1u, 1234u, 20261105u, 99u, 7u}) {
                const char* joueur = CARS[i].id;
                auto os = course5::tirer(g, joueur);
                const int place = std::min((int)os.size(), course5::tirerPlaceJoueur(g));
                course5::grille(os, place);
                const course5::Place pj = course5::placeGrille(place);
                struct B { double s, x, L, W; int file; };
                std::vector<B> bs{{pj.s * M, pj.x * RW, CARS[i].d.L, CARS[i].d.W, pj.file}};
                for (const auto& o : os) { const CarDef& c = carById(o.car.c_str()); bs.push_back({o.g.rd * M, o.g.x * RW, c.d.L, c.d.W, o.file}); }
                for (size_t a = 0; a < bs.size(); a++) {
                    avantMax = std::max(avantMax, bs[a].s + bs[a].L / 2);
                    fautes += bs[a].s + bs[a].L / 2 > -1.0;   // avant de la voiture à plus de 1 m derrière la ligne (s = 0)
                    fautes += std::abs(bs[a].x - course5::FILES[bs[a].file] * RW) > 1e-9 || bs[a].file == 1;   // voie de gauche ou de droite
                    for (size_t b = a + 1; b < bs.size(); b++) {
                        const double dz = std::abs(bs[a].s - bs[b].s) - (bs[a].L + bs[b].L) / 2, dx = std::abs(bs[a].x - bs[b].x) - (bs[a].W + bs[b].W) / 2;
                        fautes += dz <= 0 && dx <= 0;                         // carrosseries l'une dans l'autre
                        if (bs[a].file == bs[b].file) ecartVoie = std::min(ecartVoie, dz);
                        else ecartLateral = std::min(ecartLateral, dx);
                    }
                }
                std::set<int> places; for (const auto& b : bs) places.insert((int)std::lround(-b.s));
                fautes += places.size() != bs.size();   // 5 places différentes
            }
        std::printf("WFS-242 RG-01 : 20 voitures × 5 graines : avant le plus avancé à %.2f m de la ligne, même voie ≥ %.2f m "
                    "de pare-chocs à pare-chocs, côte à côte ≥ %.2f m entre carrosseries : %s\n", -avantMax, ecartVoie, ecartLateral, fautes ? "ÉCHEC" : "OK");
        VERIFIE(fautes == 0, "WFS-242 RG-01 : %d faute(s) sur la grille", fautes);
        VERIFIE(ecartVoie > 8 && ecartLateral >= 1.5, "WFS-242 RG-01 : écarts %.2f m (même voie) et %.2f m (côte à côte)", ecartVoie, ecartLateral);
    }
    // WFS-242 (RG-02) : place du joueur tirée au sort : sur 1 000 graines, chacune des 5 places entre 15 et 25 % du temps
    {
        int n[course5::PLACES] = {};
        for (uint32_t g = 0; g < 1000; g++) n[course5::tirerPlaceJoueur(g * 2654435761u + 17u)]++;
        bool ok = true; for (int k = 0; k < course5::PLACES; k++) ok = ok && n[k] >= 150 && n[k] <= 250;
        std::printf("WFS-242 RG-02 : place du joueur sur 1 000 graines : %d / %d / %d / %d / %d : %s\n", n[0], n[1], n[2], n[3], n[4], ok ? "OK" : "ÉCHEC");
        VERIFIE(ok, "WFS-242 RG-02 : tirage de la place du joueur déséquilibré");
        VERIFIE(course5::tirerPlaceJoueur(1234) == course5::tirerPlaceJoueur(1234), "WFS-242 RG-02 : même graine, même place");
    }
    // WFS-242 (RG-03, RG-06) : départ sur les 4 cartes : 3 s de compte à rebours (personne ne bouge : le joueur appuie sur les gaz,
    // la course le retient comme Course::hold), puis 5 s de course ; le joueur, plein gaz, garde sa voie (ou roule tout droit) et freine
    // derrière une voiture plus lente ; distance
    // minimale entre deux carrosseries (> 0,5 m), aucun changement de voie des ordinateurs pendant les 3 premières secondes
    {
        double pire = 1e9; int changements = 0, bouges = 0, freinages = 0;
        for (const char* carte : {"ville", "campagne", "plage", "circuit"}) {
            static Track t; t.build(mapById(carte), trace::versionCarte(carte));
            double pireCarte = 1e9;
            for (uint32_t g : {1u, 1234u, 20261105u, 99u, 7u})
                for (const char* car : {"golf", "sf90", "p208"})
                    for (bool tenir : {true, false}) {
                        Player pl; pl.reset(carById(car));
                        auto os = course5::tirer(g, car);
                        const int place = std::min((int)os.size(), course5::tirerPlaceJoueur(g));
                        course5::grille(os, place);
                        course5::placerSurGrille(pl, t, course5::placeGrille(place));
                        const double x0 = pl.playerX, s0 = pl.dist;
                        std::vector<int> files; for (const auto& o : os) files.push_back(o.file);
                        const auto rd0 = os; const double dt = 1.0 / 120;
                        for (int i = 0; i < 120 * 8; i++) {
                            const bool course = i >= 360;   // 3 s de décompte, puis la course
                            DriveInput in; in.gas = true;
                            in.steer = tenir ? (float)std::clamp((x0 - pl.playerX) * 3.0 - pl.heading * 4.0, -1.0, 1.0) : 0.f;
                            // joueur prudent : il freine quand la voiture de devant, dans sa voie, est à moins de 2 m + 0,6 s × sa
                            // vitesse (pare-chocs à pare-chocs) : rattraper une voiture plus lente devant soi, c'est à lui de freiner
                            double devant = 1e9;
                            for (const auto& o : os)
                                if (std::abs(o.g.x - pl.playerX) * RW < 2.0 && o.g.rd > pl.dist)
                                    devant = std::min(devant, (o.g.rd - pl.dist) * M - (carById(o.car.c_str()).d.L + pl.car->d.L) / 2);
                            if (course && devant < 2 + 0.6 * pl.speed * M) { in.gas = false; in.brake = true; freinages++; }
                            pl.update(dt, in, t, true);
                            if (!course) { pl.speed = 0; pl.nitroOn = false; continue; }   // Course::hold ; ordinateurs à l'arrêt (course5_system)
                            const double tc = (i - 359) * dt;
                            course5::avancer(os, dt, 4 * duel::KM, tc, pl.dist, pl.playerX, pl.speed, &t);
                            if (tc < course5::GARDE_VOIE_S) for (size_t k = 0; k < os.size(); k++) changements += os[k].file != files[k];
                            struct B { double s, x, L, W; };
                            std::vector<B> bs{{pl.dist * M, pl.playerX * RW, pl.car->d.L, pl.car->d.W}};
                            for (const auto& o : os) { const CarDef& c = carById(o.car.c_str()); bs.push_back({o.g.rd * M, o.g.x * RW, c.d.L, c.d.W}); }
                            for (size_t a = 0; a < bs.size(); a++) for (size_t b = a + 1; b < bs.size(); b++) {
                                const double dz = std::abs(bs[a].s - bs[b].s) - (bs[a].L + bs[b].L) / 2, dx = std::abs(bs[a].x - bs[b].x) - (bs[a].W + bs[b].W) / 2;
                                const double e = std::max(dz, dx);   // écart entre les deux boîtes (négatif : l'une dans l'autre)
                                pireCarte = std::min(pireCarte, e);
                            }
                            if (i == 360) {   // au « GO » : rien n'a bougé pendant le décompte (le joueur a moins de 20 cm de rampe)
                                bouges += std::abs(pl.dist - s0) * M > 0.20 || std::abs(pl.playerX - x0) * RW > 0.05;
                                for (size_t k = 0; k < os.size(); k++) bouges += std::abs(os[k].g.rd - rd0[k].g.rd) * M > 0.20;
                            }
                        }
                    }
            std::printf("WFS-242 RG-03 : %-8s : distance minimale entre deux voitures pendant les 5 premières secondes : %.2f m\n", carte, pireCarte);
            pire = std::min(pire, pireCarte);
        }
        VERIFIE(pire > 0.5, "WFS-242 RG-03 : deux voitures à %.2f m au départ", pire);
        VERIFIE(changements == 0, "WFS-242 RG-03 : %d changement(s) de voie pendant les 3 premières secondes", changements);
        VERIFIE(bouges == 0, "WFS-242 RG-03 : %d voiture(s) qui bougent pendant le décompte", bouges);
        std::printf("WFS-242 RG-03 : 4 cartes × 5 graines × 3 voitures × 2 conduites : écart minimal %.2f m, %d changement de voie, "
                    "%d voiture qui bouge au décompte (joueur qui freine derrière une voiture plus lente : %.1f s en tout) : %s\n",
                    pire, changements, bouges, freinages / 120.0, pire > 0.5 && !changements && !bouges ? "OK" : "ÉCHEC");
    }

    // WFS-243 (RG-ANO-150) : une voiture lente 25 m devant dans la file de gauche, les deux autres files libres : l'ordinateur
    // rapide derrière change de voie UNE fois, garde sa file choisie (cibleX fixe) jusqu'à y être, et dépasse ; puis sur 4 km
    // de la Ville (pilote automatique), changements de voie par ordinateur et par minute (comptés de l'extérieur : file à chaque pas)
    {
        auto os = course5::tirer(7, "golf"); os.resize(2); course5::grille(os, -1);
        os[0].g.rd = 0; os[0].g.x = os[0].cibleX = course5::FILES[0]; os[0].file = 0; os[0].nitros = 0;
        os[1].g.rd = 25 / M; os[1].g.x = os[1].cibleX = course5::FILES[0]; os[1].file = 0; os[1].nitros = 0;
        os[0].g.v = os[0].cible; os[1].cible *= 0.5; os[1].g.v = os[1].cible;
        int changements = 0, ciblesBougees = 0; int fileAvant = os[0].file; bool depasse = false;
        for (int i = 0; i < 120 * 20; i++) {
            const double xc = os[0].cibleX; const bool enRoute = std::abs(os[0].g.x - os[0].cibleX) > 0.05;
            course5::avancer(os, 1.0 / 120, 50 * duel::KM, 10 + i / 120.0, -1e9, 0, 0);
            changements += os[0].file != fileAvant; fileAvant = os[0].file;
            ciblesBougees += enRoute && os[0].cibleX != xc;
            depasse = depasse || os[0].g.rd > os[1].g.rd + 10 / M;
        }
        std::printf("WFS-243 ANO-150 : voiture lente devant, deux files libres : %d changement(s) de voie, cible changée %d fois en route, dépassement %s\n",
                    changements, ciblesBougees, depasse ? "fait" : "PAS FAIT");
        VERIFIE(changements == 1 && ciblesBougees == 0 && depasse, "WFS-243 ANO-150 : %d changement(s), %d cible(s) changée(s) en route, dépassé %d", changements, ciblesBougees, (int)depasse);
        double parMinute = 0, pire = 0;
        for (uint32_t g : {1u, 1234u, 20261105u, 99u, 7u}) {
            static Track t; static bool faite = false;
            if (!faite) { t.build(mapById("ville"), trace::versionCarte("ville")); faite = true; }
            Player pl; pl.reset(carById("golf"));
            auto vs = course5::tirer(g, "golf");
            const int place = std::min((int)vs.size(), course5::tirerPlaceJoueur(g));
            course5::grille(vs, place); course5::placerSurGrille(pl, t, course5::placeGrille(place));
            std::vector<int> files; for (const auto& o : vs) files.push_back(o.file);
            int n = 0; const double dt = 1.0 / 120, duree = 60;
            for (int i = 0; i < 120 * (int)duree; i++) {
                DriveInput in; in.gas = true; in.steer = (float)std::clamp((0.0 - pl.playerX) * 3.0 - pl.heading * 4.0, -1.0, 1.0);
                pl.update(dt, in, t, true);
                course5::avancer(vs, dt, 4 * duel::KM, (i + 1) * dt, pl.dist, pl.playerX, pl.speed, &t);
                for (size_t k = 0; k < vs.size(); k++) { n += vs[k].file != files[k]; files[k] = vs[k].file; }
            }
            const double m = n / (double)vs.size() / (duree / 60);
            parMinute += m / 5; pire = std::max(pire, m);
        }
        std::printf("WFS-243 ANO-150 : Ville, 60 s de course, 5 graines : %.1f changement(s) de voie par ordinateur et par minute en moyenne (au plus %.1f)\n", parMinute, pire);
        VERIFIE(pire <= 30, "WFS-243 ANO-150 : %.1f changements de voie par minute pour un ordinateur", pire);
    }

    // WFS-243 (RG-03) : classement juste : 60 s de course sur la Ville (joueur au pilote automatique, à sa place de la grille), à
    // chaque pas, la position affichée = 1 + nombre d'ordinateurs réellement devant lui sur la route ; un dépassement (dans un sens
    // ou dans l'autre) se voit dans le tableau au pas même où il arrive (moins d'une seconde)
    {
        static Track t; static bool faite = false;
        if (!faite) { t.build(mapById("ville"), trace::versionCarte("ville")); faite = true; }
        int faux = 0, depassements = 0, pas = 0;
        for (uint32_t g : {1u, 7u, 99u}) {
            Player pl; pl.reset(carById("golf"));
            auto os = course5::tirer(g, "golf");
            const int place = std::min((int)os.size(), course5::tirerPlaceJoueur(g));
            course5::grille(os, place); course5::placerSurGrille(pl, t, course5::placeGrille(place));
            int avant = -1; const double dt = 1.0 / 120, fin = 4 * duel::KM;
            for (int i = 0; i < 120 * 60; i++) {
                DriveInput in; in.gas = true; in.nitro = pl.nitro > 0.5;
                in.steer = (float)std::clamp((0.0 - pl.playerX) * 3.0 - pl.heading * 4.0, -1.0, 1.0);
                pl.update(dt, in, t, true);
                course5::avancer(os, dt, fin, (i + 1) * dt, pl.dist, pl.playerX, pl.speed, &t);
                const int affichee = course5::position(course5::classement(os, "Pilote", pl.dist, 0, false, fin));
                int reelle = 1; for (const auto& o : os) reelle += o.g.done || o.g.rd > pl.dist;
                faux += affichee != reelle; pas++;
                if (avant >= 0 && affichee != avant) depassements++;
                avant = affichee;
            }
        }
        std::printf("WFS-243 RG-03 : classement sur 3 × 60 s : %d pas sur %d où la position affichée diffère de l'ordre réel, %d changement(s) de place vus au pas même\n",
                    faux, pas, depassements);
        VERIFIE(faux == 0 && depassements > 0, "WFS-243 RG-03 : %d pas faux, %d dépassement(s)", faux, depassements);
    }

    if (testPoursuite()) echecs++;   // WFS-111
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("course à 5 voitures : tout est OK\n");
    return 0;
}
