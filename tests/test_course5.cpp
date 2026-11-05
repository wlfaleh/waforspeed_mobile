// Course à 5 voitures (WFS-110) : tirage des ordinateurs par graine, 10 km sans choc entre eux, tous à l'arrivée,
// classement « Position n/5 ».
// usage : test_course5
#include <cmath>
#include <cstdio>
#include <set>
#include "../src/game/course5.h"
#include "../src/game/track.h"
using namespace wfs;
int testPoursuite();   // WFS-111 : tests/test_poursuite.cpp (même exécutable)

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

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

    if (testPoursuite()) echecs++;   // WFS-111
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("course à 5 voitures : tout est OK\n");
    return 0;
}
