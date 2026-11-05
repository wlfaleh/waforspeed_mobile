// WaForSpeed — modes de course et course à 5 voitures (WFS-110) : voir course5.h.
#include "course5.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "../core/math.h"
#include "track.h"

namespace wfs {

ModeCourse& modeCourse() { static ModeCourse m = ModeCourse::Checkpoint; return m; }
int& kmCourse5() { static int km = 4; return km; }
const char* nomMode(ModeCourse m) {
    switch (m) {
        case ModeCourse::Checkpoint: return "Checkpoint";
        case ModeCourse::Course5: return "Course à 5 voitures";
        case ModeCourse::Poursuite: return "Poursuite";
        case ModeCourse::Duel: return "Duel";
    }
    return "?";
}

namespace course5 {

Difficulte& difficulte() { static Difficulte d = NORMALE; return d; }
const char* cleDifficulte(Difficulte d) { return d == FACILE ? "facile" : d == DIFFICILE ? "difficile" : "normale"; }
Difficulte difficulteDeCle(const std::string& v) { return v == "facile" ? FACILE : v == "difficile" ? DIFFICILE : NORMALE; }

// peintures du jeu web (PAINTS), tirées au hasard pour les ordinateurs
static const unsigned PEINTURES[16] = {0xc9ccd2, 0x0f1014, 0x7b808a, 0xb3122b, 0x1d3f8f, 0xff6a00, 0xc8940a, 0x1f5a3c,
                                       0xe6e2d6, 0x141c3a, 0x0f3324, 0x5a0f1e, 0xb39a72, 0x4b2380, 0x0f8f8c, 0xe8b400};

std::vector<Ordinateur> tirer(uint32_t graine, const char* carJoueur, Difficulte d) {
    Mulberry32 R(graine);
    std::vector<int> libres;
    for (int i = 0; i < CAR_COUNT; i++) if (std::strcmp(CARS[i].id, carJoueur)) libres.push_back(i);
    std::vector<Ordinateur> os;
    for (int k = 0; k < ORDINATEURS; k++) {
        if (libres.empty()) for (int i = 0; i < CAR_COUNT; i++) libres.push_back(i);   // moins de 5 voitures : on répète
        const size_t j = (size_t)(R.next() * libres.size()) % libres.size();
        const CarDef& c = CARS[libres[j]];
        libres.erase(libres.begin() + (long)j);
        Ordinateur o;
        o.car = c.id; o.nom = c.model; o.peinture = PEINTURES[(int)(R.next() * 16) % 16];
        o.g.car = c.id; o.g.name = c.model;
        o.varVitesse = 0.96 + R.next() * 0.06;   // petite variation à chaque course (RG-04)
        o.cible = MAX_SPEED * carPerf(c).top * FACTEUR_VITESSE[d] * o.varVitesse;
        o.diff = d; o.nitros = NITROS[d];   // WFS-133 (RG-03 / RG-06)
        os.push_back(o);
    }
    return os;
}

void grille(std::vector<Ordinateur>& os, double sJoueur) {
    // deux rangées de deux devant le joueur (8 m et 16 m), files gauche et droite
    for (size_t k = 0; k < os.size(); k++) {
        Ordinateur& o = os[k];
        o.file = k % 2 ? 2 : 0;
        o.g.rd = sJoueur + (8.0 + 8.0 * (double)(k / 2)) / M;
        o.g.x = o.cibleX = FILES[o.file];
        o.g.v = 0; o.g.done = false; o.g.ft = 0; o.g.hitCd = 0;
        o.nitros = NITROS[o.diff]; o.nitrosPrises = 0; o.nitroT = 0; o.aspiration = false;   // WFS-133
    }
}

namespace {
struct Voiture { double s, x, v, L; };
// une file est libre entre `derriere` et `devant` (m) autour de s, hors de la voiture `moi`
bool fileLibre(const std::vector<Voiture>& vs, size_t moi, double x, double s, double derriere, double devant) {
    for (size_t i = 0; i < vs.size(); i++) {
        if (i == moi || std::abs(vs[i].x - x) > 0.45) continue;
        const double dz = (vs[i].s - s) * M;
        if (dz > -derriere && dz < devant) return false;
    }
    return true;
}
} // namespace

bool dansAspiration(const std::vector<Ordinateur>& os, double s, double x) {
    for (const Ordinateur& o : os) {
        if (std::abs(o.g.x - x) > 0.45) continue;
        const double dz = (o.g.rd - s) * M;
        if (dz > 0 && dz < g_configJeu.course5.aspirationM) return true;
    }
    return false;
}

void avancer(std::vector<Ordinateur>& os, double dt, double finish, double raceTime, double sJoueur, double xJoueur, double vJoueur) {
    const auto& C = g_configJeu.course5;
    std::vector<Voiture> vs;
    for (const Ordinateur& o : os) vs.push_back({o.g.rd, o.g.x, o.g.v, carById(o.car.c_str()).d.L});
    vs.push_back({sJoueur, xJoueur, vJoueur, 4.4});   // le joueur, que les ordinateurs évitent
    for (size_t k = 0; k < os.size(); k++) {
        Ordinateur& o = os[k];
        const CarDef& c = carById(o.car.c_str());
        const double acc = carPerf(c).a0 / M * 0.9;   // a0 en m/s² → unités/s²
        const double pointe = MAX_SPEED * carPerf(c).top, reste = (finish - o.g.rd) * M;
        double cible = o.cible;
        // WFS-133 (RG-04) : rattrapage par rapport au joueur, sauf dans le dernier kilomètre
        const double ecartJ = (o.g.rd - sJoueur) * M;
        if (reste > DERNIER_KM_M && !o.g.done) {
            if (ecartJ < -C.rattrapageDerriereM) cible *= 1 + C.rattrapageBonus;
            else if (ecartJ > C.rattrapageDevantM) cible *= 1 - (o.diff == FACILE ? C.rattrapageMalusFacile : C.rattrapageMalus);
        }
        // voiture la plus proche devant, dans ma file
        int devant = -1; double dzMin = 1e9;
        for (size_t i = 0; i < vs.size(); i++) {
            if (i == k || std::abs(vs[i].x - o.g.x) > 0.45) continue;
            const double dz = (vs[i].s - o.g.rd) * M;
            if (dz > 0 && dz < dzMin) { dzMin = dz; devant = (int)i; }
        }
        // RG-05 : aspiration derrière une voiture (ordinateur ou joueur) de la même file ; jamais au-delà de la pointe
        o.aspiration = devant >= 0 && dzMin < C.aspirationM;
        if (o.aspiration) cible *= 1 + C.aspirationBonus;
        cible = std::min(cible, pointe);
        // RG-06 : nitro dans les 500 derniers mètres, à moins de 40 m d'une voiture devant
        bool prendre = devant >= 0 && dzMin < 40 && reste < C.nitroFinM && reste > 0;
        double plafond = 1e18;   // voiture suivie (sans file libre) : la nitro ne pousse jamais dedans
        if (devant >= 0 && dzMin < 30) {
            const Voiture& f = vs[(size_t)devant];
            if (f.v < cible * 0.98) {   // plus lente : dépasser par une file libre, sinon la suivre
                bool change = false;
                for (int dir : {-1, 1}) {
                    const int nf = o.file + dir;
                    if (nf < 0 || nf > 2 || change) continue;
                    if (fileLibre(vs, k, FILES[nf], o.g.rd, 10, 22)) { o.file = nf; o.cibleX = FILES[nf]; change = true; }
                }
                // RG-06 : dépassement (file libre trouvée, voiture devant à moins de 30 m), lancé (pas dans la mêlée du départ)
                prendre = prendre || (change && o.g.v > 0.8 * o.cible);
                const double marge = (c.d.L + f.L) / 2 + ECART_MIN_M + 6;
                if (!change && dzMin < marge + 12) plafond = f.v * (dzMin < marge ? 0.9 : 1.0);
            }
        }
        if (prendre && plafond > 1e17 && o.nitros > 0 && o.nitroT <= 0 && !o.g.done) {   // jamais bloquée derrière
            o.nitros--; o.nitrosPrises++; o.nitroT = C.nitroDuree;
        }
        if (o.nitroT > 0) { o.nitroT -= dt; cible *= 1 + C.nitroBonus; }
        cible = std::min(cible, plafond);
        if (o.g.v < cible) o.g.v = std::min(cible, o.g.v + acc * (o.nitroT > 0 ? 1.6 : 1.0) * dt);
        else o.g.v = std::max(cible, o.g.v - acc * 1.6 * dt);
        o.g.x += std::clamp(o.cibleX - o.g.x, -0.9 * dt, 0.9 * dt);
        o.g.rd += o.g.v * dt;
        if (!o.g.done && o.g.rd >= finish) { o.g.done = true; o.g.ft = raceTime * 1000; }
        vs[k].s = o.g.rd; vs[k].x = o.g.x; vs[k].v = o.g.v;
    }
    // jamais de choc entre ordinateurs (RG-04) : une voiture qui en rattrape une autre dans sa file reste derrière
    for (size_t a = 0; a < os.size(); a++) for (size_t b = 0; b < os.size(); b++) {
        if (a == b) continue;
        Ordinateur &A = os[a], &B = os[b];
        if (std::abs(A.g.x - B.g.x) > 0.45 || A.g.rd > B.g.rd) continue;   // A derrière B, même file
        const double ecart = (carById(A.car.c_str()).d.L + carById(B.car.c_str()).d.L) / 2 + ECART_MIN_M;
        if ((B.g.rd - A.g.rd) * M < ecart) { A.g.rd = B.g.rd - ecart / M; A.g.v = std::min(A.g.v, B.g.v); }
    }
}

std::vector<Ligne> classement(const std::vector<Ordinateur>& os, const std::string& nomJoueur, double dJoueur, double ftJoueur, bool joueurFini, double finish) {
    std::vector<Ligne> ls;
    ls.push_back({nomJoueur, std::min(dJoueur, finish), ftJoueur, true, joueurFini});
    for (const Ordinateur& o : os) ls.push_back({o.nom, std::min(o.g.rd, finish), o.g.ft / 1000, false, o.g.done});
    std::stable_sort(ls.begin(), ls.end(), [](const Ligne& a, const Ligne& b) {
        if (a.fini != b.fini) return a.fini;
        if (a.fini) return a.ft < b.ft;
        return a.d > b.d;
    });
    return ls;
}
int position(const std::vector<Ligne>& ls) { for (size_t i = 0; i < ls.size(); i++) if (ls[i].moi) return (int)i + 1; return (int)ls.size(); }

} // namespace course5
} // namespace wfs
