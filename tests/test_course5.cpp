// Course à 5 voitures (WFS-110) : tirage des ordinateurs par graine, 10 km sans choc entre eux, tous à l'arrivée,
// classement « Position n/5 ».
// usage : test_course5
#include <cmath>
#include <cstdio>
#include <algorithm>
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
struct Simu { int place = 5; double temps = 0, nitroS = 0, jauge30 = 0; int depasses = 0, sous60a4s = 0; double colle = 0; bool fini = false; };
Simu simuler(const char* car, course5::Difficulte d, uint32_t graine, const char* carte, double km, bool nitro, bool recharge = true, int trace = 0) {
    static Track t; static std::string carteFaite;
    const int version = trace ? trace : trace::versionCarte(carte);   // 0 : tracé du jeu (2 en Ville)
    const std::string cle = std::string(carte) + std::to_string(version);
    if (carteFaite != cle) { t.build(mapById(carte), version); carteFaite = cle; }
    Player pl; pl.reset(carById(car));
    auto os = course5::tirer(graine, car, d);
    course5::grille(os, pl.dist);
    const double finish = km * duel::KM, dt = 1.0 / 120;
    Simu r; double ftJ = 0; course5::SuiviDepassements suivi;
    std::vector<double> colle(os.size(), 0);
    for (int i = 0; i < 120 * 400; i++) {
        const double tc = (i + 1) * dt;
        DriveInput in; in.gas = true;
        in.steer = (float)std::clamp((0.0 - pl.playerX) * 3.0 - pl.heading * 4.0, -1.0, 1.0);
        in.nitro = nitro && pl.nitro > 0.5;
        if (!r.fini) pl.update(dt, in, t, true);
        if (!r.fini && pl.nitroOn) r.nitroS += dt;
        course5::avancer(os, dt, finish, tc, pl.dist, pl.playerX, pl.speed);
        if (!r.fini) {
            const int n = suivi.compter(os, pl.dist);
            r.depasses += n;
            if (recharge) pl.nitro = course5::rechargerNitro(pl.nitro, pl.nitroOn, dt, n);
        }
        if (i + 1 == 120 * 30) r.jauge30 = pl.nitro;
        if (i + 1 == 120 * 4) for (const auto& o : os) r.sous60a4s += o.g.v / MAX_SPEED * 260 < 60;
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
        course5::grille(os, 0);
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
            course5::grille(os, 0);
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
        os.resize(1); course5::grille(os, 0);
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
        os.resize(1); course5::grille(os, 0);
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
    course5::grille(os, 0);
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
    // WFS-189 (RG-03) : grille rangée par vitesse max décroissante (devant = plus rapide, dans chaque file)
    {
        auto os = course5::tirer(20261105, "golf"); course5::grille(os, 0);
        bool range = true;
        for (const auto& a : os) for (const auto& b : os)
            if (std::abs(a.g.x - b.g.x) < 0.45 && a.g.rd > b.g.rd) range = range && carById(a.car.c_str()).spec.vmax >= carById(b.car.c_str()).spec.vmax;
        double rangeeAvant = 0; for (const auto& o : os) rangeeAvant = std::max(rangeeAvant, o.g.rd);
        int vDevant = 1e9, vDerriere = 0;
        for (const auto& o : os) { const int v = carById(o.car.c_str()).spec.vmax; if (o.g.rd == rangeeAvant) vDevant = std::min(vDevant, v); else vDerriere = std::max(vDerriere, v); }
        VERIFIE(range && vDevant >= vDerriere, "grille : rangée de devant plus rapide que celle de derrière");
        const Simu r = simuler("golf", course5::NORMALE, 20261105, "ville", 4, false);
        std::printf("RG-03 : graine 20261105 : ");
        for (const auto& o : os) std::printf("%s %d km/h à %.0f m (file %d) ; ", o.car.c_str(), carById(o.car.c_str()).spec.vmax, o.g.rd * M, o.file);
        std::printf("\nRG-03 : à 4 s du GO : %d ordinateur(s) sous 60 km/h ; plus long « collé » à moins de 8 m derrière un autre : %.2f s\n", r.sous60a4s, r.colle);
        VERIFIE(r.sous60a4s == 0, "RG-03 : %d ordinateur(s) sous 60 km/h à 4 s", r.sous60a4s);
        VERIFIE(r.colle <= 2, "RG-03 : un ordinateur reste %.2f s à moins de 8 m derrière un autre", r.colle);
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
        const char* voitures[] = {"sf90", "m4", "mustang", "golf", "x5", "p208", "compass"};
        const uint32_t graines[] = {1u, 1234u, 20261105u, 99u, 7u};
        for (int trace : {0, 1}) {
            std::printf("RG-02 : tracé %s : | voiture | Facile | Normale | Difficile | temps moyen (Normale) | nitro avec recharge (s, min-max) |\n", trace ? "1" : "du jeu");
            for (const char* v : voitures) {
                double place[3] = {0, 0, 0}, temps = 0, nMin = 1e9, nMax = 0;
                for (int d = 0; d < 3; d++) for (uint32_t g : graines) {
                    const Simu r = simuler(v, (course5::Difficulte)d, g, "ville", 4, false, true, trace);
                    VERIFIE(r.fini, "%s : le joueur n'arrive pas", v);
                    place[d] += r.place / 5.0; if (d == 1) temps += r.temps / 5;
                }
                for (uint32_t g : graines) { const Simu r = simuler(v, course5::NORMALE, g, "ville", 4, true, true, trace); nMin = std::min(nMin, r.nitroS); nMax = std::max(nMax, r.nitroS); }
                std::printf("RG-02 : | %s | %.1f | %.1f | %.1f | %.1f s | %.1f à %.1f |\n", v, place[0], place[1], place[2], temps, nMin, nMax);
                if (trace) continue;
                VERIFIE(place[1] >= 1.5 && place[1] <= 3.5, "RG-02 : %s : place moyenne %.1f en Normale (1,5 à 3,5)", v, place[1]);
                VERIFIE(place[0] <= 3.0, "RG-02 : %s : place moyenne %.1f en Facile (≤ 3,0)", v, place[0]);
                VERIFIE(place[2] >= 2.0, "RG-02 : %s : place moyenne %.1f en Difficile (≥ 2,0)", v, place[2]);
                VERIFIE(place[0] < 5.0, "RG-06 : %s : dernier en moyenne en Facile", v);
                VERIFIE(nMin >= 3 && nMax <= 6, "RG-06 : %s : %.1f à %.1f s de nitro sur 4 km (3 à 6)", v, nMin, nMax);
            }
        }
    }

    if (testPoursuite()) echecs++;   // WFS-111
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("course à 5 voitures : tout est OK\n");
    return 0;
}
