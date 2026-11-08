// WaForSpeed — modes de course et course à 5 voitures (WFS-110) : voir course5.h.
#include "course5.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include "../core/math.h"
#include "player.h"   // WFS-189 : Phys (roulement, traînée)
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

// WFS-189 (RG-02) : vitesse de pointe réelle d'une voiture (unités/s) : celle que le joueur atteint sans nitro, 99,5 % de la
// fiche (étalonnage WFS-188, cars.h). Depuis WFS-188, carPerf().top est le plafond interne de la poussée (≈ 1,07 × la fiche),
// plus la vitesse de pointe : les ordinateurs visaient 0,98 × la fiche en Normale au lieu de 0,90 (joueur toujours battu)
// WFS-327 (RG-01) : adhérence des ordinateurs en virage : celle du jeu (Phys::grip), à l'échelle de la fiche de la voiture
// (μ de la voiture / μ de la Golf, la voiture de référence du joueur) ; RG-02 mesuré sans autre facteur (× 1,2 : le joueur
// moyen avec l'aide n'est plus dans les 3 premiers une fois sur deux). μ lu sur la fiche (physique::adherencePneu), sans
// étalonnage de la voiture : aucun à-coup au premier pas après le « GO » (85,9 ms mesurés avec l'étalonnage des 4 ordinateurs)
static double vitessePointe(const CarDef& c) { return MAX_SPEED * 0.995 * c.spec.vmax / 260.0; }

// peintures du jeu web (PAINTS), tirées au hasard pour les ordinateurs
static const unsigned PEINTURES[16] = {0xc9ccd2, 0x0f1014, 0x7b808a, 0xb3122b, 0x1d3f8f, 0xff6a00, 0xc8940a, 0x1f5a3c,
                                       0xe6e2d6, 0x141c3a, 0x0f3324, 0x5a0f1e, 0xb39a72, 0x4b2380, 0x0f8f8c, 0xe8b400};

std::vector<int> candidats(const char* carJoueur) {
    // WFS-189 (RG-01, ANO-101) : voitures dont la vitesse max de fiche est à ± CLASSE_VMAX de celle du joueur ; moins de
    // ORDINATEURS candidats : on complète avec les plus proches en vitesse max (écart croissant, puis ordre du catalogue)
    const int vJ = carById(carJoueur).spec.vmax;
    std::vector<int> autres, dans;
    for (int i = 0; i < CAR_COUNT; i++) if (std::strcmp(CARS[i].id, carJoueur)) autres.push_back(i);
    for (int i : autres) if (std::abs(CARS[i].spec.vmax - vJ) <= CLASSE_VMAX * vJ) dans.push_back(i);
    if ((int)dans.size() >= ORDINATEURS) return dans;
    std::stable_sort(autres.begin(), autres.end(), [&](int a, int b) { return std::abs(CARS[a].spec.vmax - vJ) < std::abs(CARS[b].spec.vmax - vJ); });
    for (int i : autres) {
        if ((int)dans.size() >= ORDINATEURS) break;
        if (std::find(dans.begin(), dans.end(), i) == dans.end()) dans.push_back(i);
    }
    return dans;
}

std::vector<Ordinateur> tirer(uint32_t graine, const char* carJoueur, Difficulte d) {
    Mulberry32 R(graine);
    std::vector<int> libres = candidats(carJoueur);   // WFS-189 (RG-01) : adversaires de la classe du joueur
    const CarDef& J = carById(carJoueur);
    std::vector<Ordinateur> os;
    for (int k = 0; k < ORDINATEURS; k++) {
        if (libres.empty()) for (int i = 0; i < CAR_COUNT; i++) libres.push_back(i);   // moins de 5 voitures : on répète
        const size_t j = (size_t)(R.next() * libres.size()) % libres.size();
        const CarDef& c = CARS[libres[j]];
        libres.erase(libres.begin() + (long)j);
        Ordinateur o;
        o.car = c.id; o.nom = modele(c);   // WFS-191 : nom de configgame.json
        o.peinture = PEINTURES[(int)(R.next() * 16) % 16];
        o.g.car = c.id; o.g.name = modele(c);
        o.varVitesse = 0.96 + R.next() * 0.06;   // petite variation à chaque course (RG-04)
        // WFS-189 (RG-02 ter) : facteur × min(sa pointe, 1,10 × pointe du joueur) : un adversaire plus rapide du haut de la
        // classe reste battable ; un plus lent garde sa propre pointe. Pointes = vraies vitesses de pointe (vitessePointe)
        o.cible = std::min(vitessePointe(c), PLAFOND_JOUEUR * vitessePointe(J)) * FACTEUR_VITESSE[d] * o.varVitesse;
        o.diff = d; o.nitros = NITROS[d];   // WFS-133 (RG-03 / RG-06)
        os.push_back(o);
    }
    return os;
}

void placerSurGrille(Player& pl, const Track& t, const Place& p) {
    // la conduite (modèle du monde ou d'avant) voit le changement de dist / playerX au pas suivant et y replace la voiture
    pl.dist = p.s; pl.position = t.wrap(p.s); pl.playerX = p.x;
    pl.heading = 0; pl.spinYaw = 0; pl.playerSpin = 0; pl.speed = 0; pl.steer = 0; pl.odo = 0;
}

int tirerPlaceJoueur(uint32_t graine) {
    Mulberry32 R(graine ^ 0x9e3779b9u);   // autre suite que celle du tirage des ordinateurs (tirer)
    return std::min(PLACES - 1, (int)(R.next() * PLACES));
}

void grille(std::vector<Ordinateur>& os, int placeJoueur) {
    // WFS-189 (RG-03, ANO-99, arbitrage du PO 20:57) : rangés par 0-100 de fiche croissant (à 0-100 égal, la plus forte vitesse
    // max devant) : les plus vifs devant ; dans chaque file, la voiture de devant accélère le plus : personne n'est bloqué au départ.
    // WFS-242 (RG-01) : places de la grille derrière la ligne (placeGrille), sauf celle du joueur. Avant : les ordinateurs sur
    // deux rangées 8 m et 21 m DEVANT le joueur, posé sur la ligne : il partait toujours dernier (ANO-114)
    std::stable_sort(os.begin(), os.end(), [](const Ordinateur& a, const Ordinateur& b) {
        const CarSpec &A = carById(a.car.c_str()).spec, &B = carById(b.car.c_str()).spec;
        return A.s100 != B.s100 ? A.s100 < B.s100 : A.vmax > B.vmax;
    });
    int place = 0;
    for (Ordinateur& o : os) {
        if (place == placeJoueur) place++;
        const Place p = placeGrille(place++);   // au plus 4 ordinateurs (configgame.json) : toujours une place libre
        o.file = p.file;
        o.g.rd = p.s; o.g.x = o.cibleX = p.x;
        o.g.rdPrec = o.g.rd; o.g.xPrec = o.g.x;   // WFS-221 : aucun glissement dessiné depuis l'ancienne place
        o.g.v = 0; o.g.done = false; o.g.ft = 0; o.g.hitCd = 0;
        o.nitros = NITROS[o.diff]; o.nitrosPrises = 0; o.nitroT = 0; o.aspiration = false;   // WFS-133
        o.gardeFile = 0; o.fileQuittee = -1; o.changements = 0;   // WFS-243 (ANO-150)
    }
}

// WFS-189 (RG-02 bis) : vitesse permise dans les virages, d'après la conduite du joueur (Player::update). En courbe, la voiture
// dérive vers l'extérieur de 2 × CENTRIFUGAL × (v / MAX_SPEED)² × courbure (largeurs de route par seconde) ; en braquant,
// le cap (au plus maxHeading − 0,0045 × v, borné à 0,16 rad) la ramène de v × sin(cap) / RW. Une courbe se passe à la
// vitesse v si la dérive cumulée, contre-braquage au maximum (× MARGE_VIRAGE), reste sous DERIVE_MAX (bord de la chaussée
// à 1 depuis le milieu de la voie) : une courbe courte se prend plus vite qu'une longue, comme pour le joueur.
namespace {
double derive(const Track& t, int64_t a, int64_t b, double vU) {   // dérive maximale sur les segments [a, b), vitesse vU (unités/s)
    const double vms = vU * M, sp = std::min(vU / MAX_SPEED, 1.1), dtk = SEG / std::max(vU, 1e-6);
    const double maxH = std::clamp((double)Phys::maxHeading - vms * 0.0045, 0.16, (double)Phys::maxHeading);
    const double corr = MARGE_VIRAGE * vms * std::sin(maxH) / RW * dtk;
    double S = 0, pire = 0;
    for (int64_t k = a; k < b; k++) {
        S += 2 * CENTRIFUGAL * sp * sp * t.segAt(k).curve * dtk;
        S -= (S > 0 ? 1 : -1) * std::min(std::abs(S), corr);
        pire = std::max(pire, std::abs(S));
    }
    return pire;
}
// table des vitesses permises par segment (unités/s), freinage anticipé (Phys::brake : le freinage du joueur) : un
// ordinateur ralentit avant le virage, pas dedans. Calculée une fois par piste (préparation, hors du pas de course).
struct TableVirages { const Track* t = nullptr; size_t n = 0; double longueur = 0; int version = 0; std::vector<float> v; };
TableVirages g_virages;
const std::vector<float>& virages(const Track& t) {
    TableVirages& T = g_virages;
    if (T.t == &t && T.n == t.segmentCount() && T.longueur == t.length() && T.version == t.version()) return T.v;
    T.t = &t; T.n = t.segmentCount(); T.longueur = t.length(); T.version = t.version();
    const double vMax = 1.6 * MAX_SPEED;   // vitesse bornée de Player
    std::vector<double> v2(T.n, vMax * vMax);
    // une « courbe » : segments consécutifs dont la courbure dépasse COURBURE_VIRAGE ; vitesse constante cherchée par
    // dichotomie (la plus haute dont la dérive reste ≤ DERIVE_MAX), sur la courbe et les 40 segments qui suivent
    const int64_t n = (int64_t)T.n;
    for (int64_t k = 0; k < n;) {
        if (std::abs(t.segAt(k).curve) < COURBURE_VIRAGE) { k++; continue; }
        int64_t fin = k; while (fin < n && std::abs(t.segAt(fin).curve) >= COURBURE_VIRAGE) fin++;
        double lo = 0.02 * MAX_SPEED, hi = vMax;
        if (derive(t, k, fin + 40, hi) > DERIVE_MAX)
            for (int i = 0; i < 30; i++) { const double mid = (lo + hi) / 2; (derive(t, k, fin + 40, mid) <= DERIVE_MAX ? lo : hi) = mid; }
        else lo = hi;
        for (int64_t j = k; j < fin; j++) v2[(size_t)j] = std::min(v2[(size_t)j], lo * lo);
        k = fin;
    }
    const double b = Phys::brake / M;   // unités/s²
    for (int tour = 0; tour < 2; tour++)   // la piste boucle : deux passes à rebours suffisent
        for (size_t i = T.n; i-- > 0;) { const size_t j = (i + 1) % T.n; v2[i] = std::min(v2[i], v2[j] + 2 * b * SEG); }
    T.v.assign(T.n, 0.f);
    for (size_t k = 0; k < T.n; k++) T.v[k] = (float)std::sqrt(v2[k]);
    return T.v;
}
} // namespace

double vitesseVirage(const Track& t, double s) {
    const std::vector<float>& v = virages(t);
    const int64_t n = (int64_t)v.size(), k = (int64_t)std::floor(s / SEG);
    return n ? v[(size_t)(((k % n) + n) % n)] : 1e18;
}

// WFS-327 (RG-01) : vitesse permise à un ordinateur (unités/s) : jamais au-dessus de la vitesse de virage physiquement possible
// pour sa voiture : même règle que l'aide à la direction du joueur (vitesseVirageAide, player.h : virages des 600 m qui viennent),
// avec l'adhérence du jeu (Phys::grip) mise à l'échelle de la fiche de la voiture (μ de la voiture / μ de la Golf) et le freinage
// du jeu ; et au plus la table « où la voiture reste sur la chaussée » (vitesseVirage)
double vitesseVirageOrdinateur(const Track& t, double s, const CarDef& c) {
    static const double muGolf = physique::adherencePneu(carById("golf"));   // μ de la voiture de référence du joueur
    const double gLat = Phys::grip / 9.81 * physique::adherencePneu(c) / muGolf;
    return std::min(vitesseVirage(t, s), vitesseVirageAide(t, s, gLat, Phys::brake / 9.81) / M);
}
// WFS-327 (RG-01) : la vitesse permise est calculée par points de 4 m (paliers) : l'ordinateur (et le pilote des tests) regarde
// aussi 4 et 8 m plus loin, sinon il a un palier de retard en entrant dans le virage (jusqu'à + 20 % mesuré ; + 1,6 % au plus ainsi)
double vitesseVirageOrdinateurDevant(const Track& t, double s, const CarDef& c) {
    return std::min({vitesseVirageOrdinateur(t, s, c), vitesseVirageOrdinateur(t, s + 4 / M, c), vitesseVirageOrdinateur(t, s + 8 / M, c)});
}
// WFS-258 (accroche) : l'aide à la direction du joueur s'en sert pour ralentir à temps (player.cpp)
static const bool g_vitesseVirageDonnee = (g_vitesseVirageCourse5 = &vitesseVirage, true);

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

void avancer(std::vector<Ordinateur>& os, double dt, double finish, double raceTime, double sJoueur, double xJoueur, double vJoueur, const Track* piste) {
    const auto& C = g_configJeu.course5;
    std::vector<Voiture> vs;
    for (const Ordinateur& o : os) vs.push_back({o.g.rd, o.g.x, o.g.v, carById(o.car.c_str()).d.L});
    vs.push_back({sJoueur, xJoueur, vJoueur, 4.4});   // le joueur, que les ordinateurs évitent
    for (size_t k = 0; k < os.size(); k++) {
        Ordinateur& o = os[k];
        const CarDef& c = carById(o.car.c_str());
        // WFS-189 (RG-02) : même loi que la voiture du joueur (Player::update, plein gaz : poussée a0 × (1 − (v / top)^2,2)
        // moins 0,35 × (roulement + traînée)), pour que l'ordinateur accélère comme sa fiche (0-100 de WFS-188) ; avant :
        // a0 × 0,9 constant (0-100 de la 208 en 6,7 s au lieu de 8,7 s : les petites voitures du joueur toujours distancées)
        const CarPerf pf = carPerf(c);
        const double topU = MAX_SPEED * pf.top, vms = std::max(0.0, o.g.v) * M;
        const double pousse = pf.a0 / M * std::max(0.0, 1 - std::pow(std::max(0.0, o.g.v) / topU, 2.2));
        const double freine = (Phys::roll + Phys::drag * vms * vms) / M * 0.35;
        const double acc = std::max(0.02 / M, pousse - freine);
        const double pointe = vitessePointe(c), reste = (finish - o.g.rd) * M;   // WFS-189 : vraie vitesse de pointe
        double cible = o.cible;
        // WFS-133 (RG-04) : rattrapage par rapport au joueur, sauf dans le dernier kilomètre
        const double ecartJ = (o.g.rd - sJoueur) * M;
        if (reste > DERNIER_KM_M && !o.g.done) {
            if (ecartJ < -C.rattrapageDerriereM) cible *= 1 + C.rattrapageBonus;
            else if (ecartJ > C.rattrapageDevantM) cible *= 1 - (o.diff == FACILE ? C.rattrapageMalusFacile : C.rattrapageMalus);
        }
        if (o.gardeFile > 0) o.gardeFile -= dt;   // WFS-243 (ANO-150)
        const bool enChangement = std::abs(o.g.x - o.cibleX) > 0.05;   // pas encore arrivé dans la file choisie
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
                bool change = enChangement;   // WFS-243 (ANO-150) : dépassement en cours, gardé jusqu'à la nouvelle file
                const bool garde = raceTime < GARDE_VOIE_S || enChangement || o.gardeFile > 0;   // WFS-242 (RG-03) : au départ, chacun garde sa voie et suit
                for (int dir : {-1, 1}) {
                    const int nf = o.file + dir;
                    if (nf < 0 || nf > 2 || change || garde) continue;
                    const double portee = nf == o.fileQuittee ? RETOUR_LIBRE_M : 22;   // WFS-243 (ANO-150) : retour vers la file quittée
                    if (fileLibre(vs, k, FILES[nf], o.g.rd, 10, portee)) {
                        o.fileQuittee = o.file; o.file = nf; o.cibleX = FILES[nf]; change = true;
                        o.gardeFile = GARDE_CHANGEMENT_S; o.changements++;
                    }
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
        bool virageFrein = false;   // WFS-189 (RG-02 bis) : virage serré devant : vitesse permise, freinage du joueur
        if (piste) {
            const double vp = g_avantWfs327 ? vitesseVirage(*piste, o.g.rd) : vitesseVirageOrdinateurDevant(*piste, o.g.rd, c);   // WFS-327 (RG-01)
            if (vp < cible) { cible = vp; virageFrein = o.g.v > vp; }
        }
        if (virageFrein) o.g.v = std::max(cible, o.g.v - (double)Phys::brake / M * dt);
        else if (o.g.v < cible) o.g.v = std::min(cible, o.g.v + (o.nitroT > 0 ? pousse * 1.6 - freine : acc) * dt);
        else o.g.v = std::max(cible, o.g.v - pf.a0 / M * 0.9 * 1.6 * dt);   // ralentir : comme avant WFS-189 (a0 × 0,9 × 1,6)
        o.g.noterPas();   // WFS-221 (accroche) : rendu interpolé
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
int SuiviDepassements::compter(const std::vector<Ordinateur>& os, double sJoueur) {
    if (arme.size() != os.size()) {   // nouvelle grille : seuls les ordinateurs partis devant le joueur pourront être « dépassés »
        arme.resize(os.size());       // WFS-242 (RG-02) : un ordinateur parti derrière ne donne pas de nitro au premier pas
        for (size_t k = 0; k < os.size(); k++) arme[k] = os[k].g.rd >= sJoueur;
    }
    int n = 0;
    for (size_t k = 0; k < os.size(); k++) {
        const double ecart = (os[k].g.rd - sJoueur) * M;   // > 0 : l'ordinateur est devant
        if (arme[k] && ecart < 0) { n++; arme[k] = false; }      // dépassé
        else if (!arme[k] && ecart > REARMEMENT_M) arme[k] = true;   // repassé devant franchement : un nouveau dépassement comptera
    }
    return n;
}
int position(const std::vector<Ligne>& ls) { for (size_t i = 0; i < ls.size(); i++) if (ls[i].moi) return (int)i + 1; return (int)ls.size(); }

} // namespace course5
} // namespace wfs
