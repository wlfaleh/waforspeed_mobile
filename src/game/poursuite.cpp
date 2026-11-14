// WaForSpeed — mode « Poursuite police » (WFS-111) : règles, police et programme de la course. Voir poursuite.h.
#include "poursuite.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "../core/math.h"
#include "cars.h"     // WFS-461 : fiche de la voiture de police
#include "roues.h"    // WFS-540 : courbure de la route (volant du policier)
#include "player.h"   // WFS-461 : Phys (freinage, résistance), vitesseVirageAide

namespace wfs {
namespace poursuite {

void perdre(Etat& e, int points, const char* raison) {
    if (e.arrete || e.gagne) return;
    const int avant = e.score;
    e.score = std::max(0, e.score - points);
    if (e.score < avant) { e.pertes++; e.dernierePerte = avant - e.score; e.raison = raison ? raison : ""; }   // WFS-338 (RG-03)
    if (e.score == 0) e.arrete = true;   // RG-01 : défaite immédiate, écran « ARRÊTÉ »
}
void chocTrafic(Etat& e, const char* raison) { if (e.cdTrafic > 0) return; e.cdTrafic = DELAI_CHOC; e.chocsTrafic++; perdre(e, CHOC_TRAFIC, raison); }
void chocGlissiere(Etat& e, const char* raison) { if (e.cdGlissiere > 0) return; e.cdGlissiere = DELAI_CHOC; e.chocsGlissiere++; perdre(e, CHOC_GLISSIERE, raison); }
void chocPolice(Etat& e) { e.chocsPolice++; perdre(e, CHOC_POLICE, RAISON_POLICE); }
std::string textePerte(int points, const char* raison) { return "-" + std::to_string(points) + " " + (raison ? raison : ""); }   // « - » (la police de l’interface n’a pas le signe U+2212)
void quasiContact(Etat& e) { if (e.arrete || e.gagne) return; e.quasi++; e.score = std::min(SCORE_DEPART, e.score + 1); }

void pas(Etat& e, double dt, double dPoliceM, double ecartM, double kmhJ) {
    e.cdTrafic = std::max(0.0, e.cdTrafic - dt); e.cdGlissiere = std::max(0.0, e.cdGlissiere - dt);
    if (e.arrete || e.gagne) return;
    // WFS-540 (RG-03, ANO-268) : arrestation : joueur (presque) arrêté près de la police
    const double v = std::abs(kmhJ);
    if (v < ARRET_KMH && dPoliceM < ARRET_M) e.arrestation += dt;
    else if (v > REPRISE_KMH || dPoliceM >= ARRET_M) e.arrestation = 0;
    if (e.arrestation >= ARRET_S) {
        e.arrete = e.parPolice = true; e.raison = RAISON_ARRESTATION;   // fin de la course : écran « ARRÊTÉ PAR LA POLICE »
        return;
    }
    // WFS-540 (RG-04, ANO-268) : −1 par seconde seulement au contact réel (avant : police à moins de 12 m, centres), jamais pendant
    // le compte à rebours de l'arrestation
    if (e.arrestation > 0 || ecartM > CONTACT_REEL_M) { e.colle = 0; return; }
    e.colle += dt;
    while (e.colle >= COLLE_S && !e.arrete) { e.colle -= COLLE_S; perdre(e, 1, RAISON_COLLE); }   // −1 par seconde collée
}
int compteArrestation(const Etat& e) {
    if (e.arrestation <= 0 || e.arrete) return 0;
    return std::clamp((int)std::ceil(ARRET_S - e.arrestation), 1, (int)ARRET_S);
}
void arrivee(Etat& e, double distKm) { if (!e.arrete && !e.gagne && distKm >= KM_ARRIVEE && e.score >= 1) e.gagne = true; }
int ralentisseur(Etat& e, double kmh) {
    if (e.arrete || e.gagne || kmh <= RALENTISSEUR_KMH) return 0;
    e.ralentisseurs++; const int avant = e.score; perdre(e, CHOC_RALENTISSEUR, RAISON_RALENTISSEUR); return avant - e.score;
}
int policeVoulue(double distKm) { return std::min(POLICE_MAX, POLICE_DEPART + (int)std::floor(std::max(0.0, distKm) / RENFORT_KM)); }

// ---------------------------------------------------------------- police
bool attaque(const Police& p, int indice, double t) {
    if (p.fixe || t < 4) return false;
    return std::fmod(t + indice * (CYCLE_S / 2.7), CYCLE_S) < ATTAQUE_S;
}

FichePolice fichePolice(const char* police, const char* joueur) {
    const CarDef& c = carById(police ? police : "p208");
    FichePolice f;
    f.P = &physique::params(c);   // WFS-540 (RG-00) : même moteur de conduite et même étalonnage que la voiture du joueur
    f.vMax = c.spec.vmax / 3.6 / M;
    f.frein = 0.45 * physique::G;  // s'arrêter derrière un obstacle : freinage franc mais pas d'urgence (≈ 4,4 m/s²)
    f.longueur = c.d.L; f.largeur = c.d.W;
    if (joueur) {
        const CarDef& j = carById(joueur);
        f.contactL = (c.d.L + j.d.L) / 2; f.contactW = (c.d.W + j.d.W) / 2;
    }
    return f;
}

void poser(Police& p, double rd, double x, double v) {
    p.rd = p.rdPrec = rd; p.x = p.xPrec = x; p.v = v; p.h = p.hPrec = 0; p.braquage = 0;
    p.mondeOk = false; p.evite = false; p.bloque = p.manoeuvre = 0;
}

namespace {
constexpr double PI_P = 3.14159265358979323846;
constexpr double VITESSE_VOLANT = 1.0;     // rad/s aux roues : un conducteur tourne le volant, il ne le jette pas (à confirmer)
constexpr double RESTITUTION = 0.2;        // choc mou entre voitures (0,1 à 0,3 à basse vitesse ; comme duel::contacter)
constexpr double SEPARATION_MAX = 1.0;     // m/s : vitesse à laquelle deux carrosseries qui se recouvrent se séparent au plus
constexpr double OBSTACLE_L = 4.4, OBSTACLE_W = 1.9;   // m : voiture du trafic, cône, camion (une file) : gabarit de contact

// monde construit depuis la projection (naissance, banc) : dans l'axe de la route + h, vitesse v le long de la caisse
void entrer(Track& t, Police& p) {
    const TrackPoint C = t.pointLisse(p.rd, p.x * RW);
    p.monde = physique::Etat{};
    p.monde.x = C.x; p.monde.z = C.z; p.monde.cap = C.th + p.h;
    const double u = p.v * M;
    p.monde.vx = u * std::sin(p.monde.cap); p.monde.vz = -u * std::cos(p.monde.cap);
    p.monde.rapport = 1; p.mondeOk = true;
}
// projection de l'état du monde sur la route (même calcul que Player::projeterPose : point de l'axe le plus proche, Newton)
void projeter(Track& t, Police& p) {
    double s = p.rd;
    TrackPoint C{};
    for (int i = 0; i < 6; i++) {
        C = t.pointLisse(s, 0);
        const double e = (p.monde.x - C.x) * std::sin(C.th) - (p.monde.z - C.z) * std::cos(C.th);
        s += e / M;
        if (std::abs(e) < 1e-6) break;
    }
    C = t.pointLisse(s, 0);
    p.rd = s;
    p.x = ((p.monde.x - C.x) * std::cos(C.th) + (p.monde.z - C.z) * std::sin(C.th)) / RW;
    p.h = std::remainder(p.monde.cap - C.th, 2 * PI_P);
    p.v = p.monde.u() / M;
}
// contact (repère de la route) entre la police p et un corps (ds, dx : position du corps − police, m ; vitesse du corps vxO, vzO
// dans le monde ; demi-longueur et demi-largeur cumulées ; part de l'impulsion prise par la police : 1 = corps immobile infini).
// Comme duel::contacter : côte à côte si le recouvrement latéral est petit devant le longitudinal. Effet : impulsion sur la
// vitesse de la police seulement (choc mou), plus une vitesse de séparation bornée si les carrosseries se recouvrent. Contact
// « anticipé » (méthode usuelle des moteurs physiques) : à moins de MARGE_M, la police n'approche pas plus vite que ce qui ferme le
// jour en un pas ; elle ne rentre donc pas dans le corps, et le joueur n'est pas repoussé d'un coup au pas suivant.
constexpr double MARGE_M = 0.25;
bool contact(Police& p, double th, double ds, double dx, double vxO, double vzO, double demiL, double demiW, double part, double dt) {
    const double ovZ = demiL - std::abs(ds), ovX = demiW - std::abs(dx);
    if (ovZ <= -MARGE_M || ovX <= -MARGE_M || (ovZ <= 0 && ovX <= 0)) return false;
    const double fx = std::sin(th), fz = -std::cos(th), rx = std::cos(th), rz = std::sin(th);   // avant / droite de la route
    double nx, nz, pen;   // normale du corps vers la police ; pen < 0 : jour qui reste
    const bool cote = ovZ > 0 && (ovX <= 0 || ovX < ovZ * 0.7);
    if (cote) { const double sg = dx > 0 ? -1.0 : 1.0; nx = rx * sg; nz = rz * sg; pen = ovX; }
    else { const double sg = ds > 0 ? -1.0 : 1.0; nx = fx * sg; nz = fz * sg; pen = ovZ; }
    if (pen <= -MARGE_M) return false;
    const double vn = (p.monde.vx - vxO) * nx + (p.monde.vz - vzO) * nz;   // < 0 : la police va vers le corps
    double dv = 0;
    if (pen > 0) {   // carrosseries en contact : choc mou, puis séparation douce si elles se recouvrent
        if (vn < 0) dv = -(1 + RESTITUTION) * vn * part;
        const double sep = std::min(SEPARATION_MAX, std::max(0.0, pen - 0.02) * 5) * part;
        if (vn + dv < sep) dv = sep - vn;
    } else {         // jour de -pen : approche permise jusqu'à le fermer en un pas, pas plus
        const double vMin = pen / dt;   // < 0
        if (vn < vMin) dv = (vMin - vn) * part;
    }
    p.monde.vx += dv * nx; p.monde.vz += dv * nz;
    return pen > 0;
}
} // namespace

void avancer(std::vector<Police>& ps, double dt, double t, double sJ, double xJ, double vJ, const std::vector<Obstacle>& obs,
             const FichePolice& f, Track& piste, double muRoute, double hJ) {
    const physique::Params& P = f.P ? *f.P : physique::params(carById("p208"));
    const double vJm = vJ * M;
    const bool joueurLent = std::abs(vJm) < REPRISE_KMH / 3.6;   // RG-02 : sous 10 km/h, la police laisse toujours un passage
    // ---------- contacts : impulsions sur les vitesses, AVANT le pas de conduite (la police cède dans le pas même où elle touche) ;
    // aucune position écrite ----------
    // voiture du joueur : la police prend TOUTE l'impulsion du contact (elle cède : RG-02, le joueur n'est jamais coincé ni poussé
    // de côté par elle ; l'effet du contact sur le joueur reste celui de duel::contacter, borné par l'adhérence de ses pneus)
    const double edge = piste.map() ? piste.map()->edge : 1.6;
    auto contacts = [&](Police& p, size_t i) {
        const double th = piste.pointLisse(p.rd, 0).th, fx = std::sin(th), fz = -std::cos(th);
        const double capJ = th + hJ;   // vitesse réelle du joueur : dans l'axe de SA caisse (elle peut braquer vers la police)
        contact(p, th, (sJ - p.rd) * M, (xJ - p.x) * RW, std::sin(capJ) * vJm, -std::cos(capJ) * vJm, f.contactL, f.contactW, 1.0, dt);
        // trafic, cônes, camion : la police ne les traverse pas (corps « infinis » ici : leur réaction est celle du trafic, WFS-541)
        for (const Obstacle& o : obs) {
            const double ds = (o.s - p.rd) * M; if (std::abs(ds) > 8) continue;
            contact(p, th, ds, (o.x - p.x) * RW, fx * o.v * M, fz * o.v * M, (f.longueur + OBSTACLE_L) / 2, (f.largeur + OBSTACLE_W) / 2, 1.0, dt);
        }
        // autre voiture de police : la moitié de l'impulsion chacune (même voiture) ; barrage garé : corps fixe
        for (size_t k = 0; k < ps.size(); k++) {
            if (k == i) continue;
            const Police& q = ps[k];
            const double ds = (q.rd - p.rd) * M; if (std::abs(ds) > 8) continue;
            const bool fixe = q.fixe || !q.mondeOk;
            contact(p, th, ds, (q.x - p.x) * RW, fixe ? 0.0 : q.monde.vx, fixe ? 0.0 : q.monde.vz, f.longueur, f.largeur, fixe ? 1.0 : 0.5, dt);
        }
        // glissière (bord de la carte) : la vitesse vers l'extérieur est annulée, comme pour la voiture du joueur
        if (std::abs(p.x) > edge) {
            const double sg = p.x > 0 ? 1.0 : -1.0, nx = std::cos(th) * sg, nz = std::sin(th) * sg;
            const double vn = p.monde.vx * nx + p.monde.vz * nz;
            if (vn > -0.3) { p.monde.vx -= (vn + 0.3) * nx; p.monde.vz -= (vn + 0.3) * nz; }
        }
    };
    for (size_t i = 0; i < ps.size(); i++) {
        Police& p = ps[i];
        p.hitCd = std::max(0.0, p.hitCd - dt);
        p.rdPrec = p.rd; p.xPrec = p.x; p.hPrec = p.h;   // WFS-221 (accroche) : rendu interpolé
        if (p.fixe) { p.v = 0; continue; }   // barrage : voiture garée
        double rel = (sJ - p.rd) * M;        // > 0 : le joueur est devant (m)
        // la poursuite ne se perd pas : une voiture distancée (hors de vue, > PERDUE_M) passe le relais à une autre, née à
        // APPARITION_M derrière le joueur dans sa file, à une vitesse ≤ sa fiche (WFS-338 RG-05 : hors de toute vue)
        if (rel > PERDUE_M) {
            const double fx = FILES[p.x < -1.0 / 3 ? 0 : p.x > 1.0 / 3 ? 2 : 1];
            poser(p, sJ - APPARITION_M / M, fx, std::min(f.vMax, std::max(p.v, vJ)));
            p.cibleX = fx; rel = APPARITION_M;
        }
        if (!p.mondeOk) { p.v = std::min(p.v, f.vMax); entrer(piste, p); }   // WFS-461 (RG-03) : née à une vitesse ≤ sa fiche
        contacts(p, i);
        physique::Etat& m = p.monde;
        const double u = m.u(), au = std::abs(u);

        // ---------- où aller : place visée (vitesse) et file visée (volant) ----------
        const bool att = attaque(p, (int)i, t) && !joueurLent;
        // WFS-338 (RG-05) : en attaque derrière, pare-chocs contre pare-chocs (le contact se fait par les vitesses, sans téléportation)
        double viseM = att ? (p.role == 0 ? f.contactL - 0.2 : 0.0) : 15.0 + p.role * 3.0;
        // RG-02 / RG-03 : joueur (presque) arrêté : elle vient s'arrêter derrière lui, GARDE_M de jour entre les pare-chocs (assez près
        // pour l'arrêter, jamais collée : il peut toujours repartir) ; les voitures « à côté » s'arrêtent derrière elle
        if (joueurLent) viseM = f.contactL + GARDE_M + p.role * (f.longueur + 1.5);
        const bool joueurDerriere = rel < -1.0;
        const bool memeFile = std::abs(xJ - p.x) * RW < f.contactW + 0.5;
        double cxLaisse = -9;   // file où elle se range pour laisser passer le joueur (-9 : aucune)
        double vCible;   // m/s, < 0 : marche arrière
        if (joueurDerriere) {
            const double d = -rel;
            if (vJm > 1.0) {
                // le joueur roule vers l'avant derrière elle : elle le laisse passer (dans sa file : elle garde de l'avance et se
                // range sur le côté ; rangée : elle ralentit pour qu'il passe)
                if (memeFile) { vCible = vJm + 2.0; cxLaisse = placeCote(xJ, p.role == 2 ? 2 : 1); }
                else vCible = std::max(0.0, vJm - 3.0);
            } else if (joueurLent && memeFile && d < f.contactL + GARDE_M + 0.5) {
                // RG-02 : joueur arrêté juste derrière elle, dans sa file (elle lui barre la route) : elle avance au pas en se rangeant
                vCible = 2.0; cxLaisse = placeCote(xJ, p.role == 2 ? 2 : 1);
            } else {
                // RG-10 : le joueur recule (ou attend) derrière elle : elle ne recule pas avec lui. Elle freine jusqu'à l'arrêt ;
                // arrêtée, elle recule doucement vers lui (≤ V_RECUL_POLICE, en braquant) s'il est entre 8 et 60 m, sinon elle attend
                vCible = u > 0.3 || d < ARRET_M || d > 60 ? 0.0 : -std::min(V_RECUL_POLICE, 0.4 * (d - ARRET_M));
            }
        } else {
            // rattrape (WFS-338 : ≤ approcheMax, de 45 m/s au loin à 4 m/s au contact : elle se voit arriver) ; trop près, elle se
            // laisse glisser en arrière doucement (WFS-461 RG-04 : √(2 × 1,5 m/s² × écart), sans à-coup près de sa place)
            const double ecart = rel - viseM;
            const double laisse = std::min({12.0, std::sqrt(2 * 1.5 * std::max(0.0, -ecart)), 0.8 * std::max(0.0, -ecart)});
            vCible = std::max(0.0, vJm + (ecart >= 0 ? std::min(ecart * 1.6, approcheMax(rel)) : -laisse));
            vCible = std::min(vCible, f.vMax * M);
            // WFS-461 (RG-04) : pas plus vite que ses pneus ne le permettent dans les virages qui viennent
            // (limite du policier : 0,65 μ, soit ≈ 0,7 g pour la 208 : mesuré sur le moteur de conduite, la 208 tient 0,73 g dans un
            // virage de 180 m à 130 km/h et 0,90 g dans un virage de 60 m ; freinage avant le virage : 0,8 μ)
            if (au > 1) vCible = std::min(vCible, vitesseVirageAide(piste, p.rd, 0.65 * P.mu * muRoute, 0.8 * muRoute));
        }
        double cx = cxLaisse > -9 ? cxLaisse : joueurDerriere ? p.cibleX : p.role == 0 ? xJ : placeCote(xJ, p.role);   // WFS-338 : jamais par-dessus le joueur
        if (rel > 40 && !att) cx = p.cibleX;   // loin : reste dans sa file (moins de zigzags)
        // obstacles devant elle dans sa trajectoire (trafic, cônes, camion, barrage) : contourne par une file libre, sinon s'arrête ;
        // une autre voiture de police qui roule devant dans sa file : elle la suit à 7 m (convoi, comme avant WFS-540)
        // il regarde d'autant plus loin qu'il roule vite (3 s devant lui, 30 m au moins, 120 m au plus)
        const double portee = std::clamp(3.0 * au, 30.0, 120.0);
        // revue du lead dev (PR #406) : la gêne se juge là où sera SA voiture en arrivant à l'obstacle (cap actuel ; en changement de
        // file, bornée à la file visée) et avec la largeur réelle des deux voitures : avant, un véhicule arrêté dans la file qu'elle
        // quittait la faisait freiner jusqu'à l'arrêt, en biais, à moitié sortie de la file (camion en travers, RG-09)
        const double demiLargeurs = ((f.largeur + OBSTACLE_W) / 2 + 0.4) / RW;
        auto xEnArrivant = [&](double d) {
            if (!p.evite) return p.x;
            const double xa = p.x + std::sin(p.h) * d / RW;
            return p.eviteX > p.x ? std::clamp(xa, p.x, p.eviteX) : std::clamp(xa, p.eviteX, p.x);
        };
        const Obstacle* gene = nullptr; double geneD = 1e9;
        for (const Obstacle& o : obs) {
            const double d = (o.s - p.rd) * M;
            if (d > 0 && d < portee && std::abs(o.x - xEnArrivant(d)) < demiLargeurs && d < geneD) { geneD = d; gene = &o; }
        }
        Obstacle barrage;   // voiture de police garée devant (copie : obs reste en lecture seule)
        double suivreD = 1e9, suivreV = 0;
        for (size_t k = 0; k < ps.size(); k++) {
            if (k == i) continue;
            const double d = (ps[k].rd - p.rd) * M;
            if (d <= 0 || d >= portee || std::abs(ps[k].x - xEnArrivant(d)) >= demiLargeurs) continue;
            if (ps[k].fixe) { if (d < geneD) { geneD = d; barrage = {ps[k].rd, ps[k].x, 0}; gene = &barrage; } }
            else if (d < suivreD) { suivreD = d; suivreV = ps[k].monde.u(); }
        }
        // RG-09 (ANO-272) : une file choisie pour contourner est gardée jusqu'à 10 m après l'obstacle (ou 3 s sans obstacle devant) :
        // plus de changement de cible à chaque image (aller-retour à côté du camion)
        if (p.evite) {
            p.eviteT += dt;
            if ((p.rd - p.eviteFinS) * M > 10 || (p.eviteT > 3 && !gene)) p.evite = false;
        }
        if (gene && !p.evite && !joueurDerriere) {
            double mieux = p.x, mieuxD = -1;
            for (double fl : FILES) {
                double libre = 200;
                for (const Obstacle& o : obs) { const double d = (o.s - p.rd) * M; if (d > -8 && d < libre && std::abs(o.x - fl) < 0.36) libre = d; }
                for (size_t k = 0; k < ps.size(); k++) {
                    if (k == i) continue;
                    const double d = (ps[k].rd - p.rd) * M; if (d > -8 && d < libre && std::abs(ps[k].x - fl) < 0.36) libre = d;
                }
                if (libre > mieuxD + 1) { mieuxD = libre; mieux = fl; }
            }
            if (mieuxD > geneD + 5 && std::abs(mieux - p.x) > 0.2) { p.evite = true; p.eviteX = mieux; p.eviteFinS = gene->s; p.eviteT = 0; }
        }
        if (p.evite) cx = p.eviteX;
        // à hauteur du joueur, elle ne braque jamais vers lui : elle garde son écart (sa file ou plus loin de lui)
        if (std::abs(rel) < f.contactL + 2 && (cx - p.x) * (xJ - p.x) > 0) cx = p.x;
        cx = std::clamp(cx, -CHAUSSEE_X, CHAUSSEE_X);   // RG-10 : elle reste sur la chaussée
        // comme un conducteur qui regarde dans son rétroviseur : une autre voiture de police à sa hauteur, du côté où elle veut aller :
        // elle garde sa file (elle ne se rabat pas sur elle) jusqu'à ce que le côté soit libre
        if (std::abs(cx - p.x) > 0.05)
            for (size_t k = 0; k < ps.size(); k++) {
                if (k == i) continue;
                const double d = (ps[k].rd - p.rd) * M, cote = (ps[k].x - p.x) * (cx - p.x);
                if (std::abs(d) < f.longueur + 2 && cote > 0 && std::abs(ps[k].x - p.x) < std::abs(cx - p.x) + 0.3) { cx = p.cibleX = p.x; break; }
            }
        if (rel <= 40 || gene || joueurDerriere) p.cibleX = cx;
        // obstacle encore devant dans sa file : elle s'arrête derrière lui, dans l'axe de sa file (RG-09 : sans glisser de côté)
        // RG-02 : le joueur est à sa hauteur et se rabat sur elle (ou elle est déjà au contact de son flanc) : elle freine pour le
        // laisser passer devant, au lieu de se laisser pousser de côté (jusque contre la glissière)
        if (std::abs(rel) < f.contactL + 1.0) {
            const double dxJ = (xJ - p.x) * RW, vLatJ = vJm * std::sin(hJ);   // vitesse latérale du joueur (m/s, + à droite)
            if (std::abs(dxJ) < f.contactW + 0.6 && (dxJ * vLatJ < -0.3 || std::abs(dxJ) < f.contactW + 0.1))
                vCible = std::min(vCible, std::max(0.0, vJm * std::cos(hJ) - 3.0));
        }
        // comme un conducteur : la voiture s'écarte de sa file (sous-virage) : il lève le pied, puis freine
        const double ecartFile = std::abs(p.x - p.cibleX) * RW;
        if (ecartFile > 1.5 && vCible > 0 && au > 8) vCible = std::min(vCible, au - 0.2);   // lève le pied (sans freiner en virage)
        if (gene && vCible > 0) {
            vCible = std::min(vCible, gene->v * M + std::sqrt(2 * f.frein * std::max(0.0, geneD - (f.longueur + OBSTACLE_L) / 2 - 2.5)));
            // en changement de file pour le contourner : elle avance au pas en braquant (rayon de braquage de 5 m) tant que les
            // carrosseries ne se touchent pas (le contact anticipé l'empêche d'entrer dedans) ; sinon elle resterait arrêtée derrière
            if (p.evite && geneD > (f.longueur + OBSTACLE_L) / 2 + 0.3) vCible = std::max(vCible, std::min(1.5, vCible + 1.5));
        }
        if (suivreD < portee && vCible > 0) vCible = std::min(vCible, std::max(0.0, suivreV) + std::sqrt(2 * f.frein * std::max(0.0, suivreD - 7)));

        // revue du lead dev (PR #406) : coincée l'avant contre un obstacle (coin du camion pris en biais) alors qu'elle veut avancer :
        // comme un conducteur, elle recule au pas de 4 m (5 s au plus), l'arrière vers le côté opposé à sa file visée (le nez se tourne
        // vers elle), puis repart braquage serré
        if (p.manoeuvre > 0) { p.manoeuvre -= dt; vCible = -1.5; if ((p.recul0 - p.rd) * M > 4.0) p.manoeuvre = 0; }
        else if (vCible > 1 && au < 0.3 && !joueurDerriere) { p.bloque += dt; if (p.bloque > 1.0) { p.manoeuvre = 5.0; p.recul0 = p.rd; p.bloque = 0; } }
        else p.bloque = 0;
        // ---------- volant : comme un conducteur qui suit sa file : courbure de la file (la route qui tourne), plus la correction de
        // l'écart de position et de cap mesuré à une distance de visée devant lui (poursuite pure rapportée à la file : elle ne coupe
        // pas les virages) ; braquage anticipé selon le sous-virage de la voiture (gradient du modèle bicyclette, Rajamani § 3.3) et
        // corrigé par la rotation ressentie (vitesse de lacet) : sans cela, la voiture « pompe » dans les grandes courbes rapides.
        // En marche arrière (lente) : poursuite pure d'un point de la file derrière elle ----------
        const double mg = P.masse * physique::G, Caf = P.cyAvant * mg * P.b / P.L, Car = P.cyArriere * mg * P.a / P.L;
        const double Kus = P.masse / P.L * (P.b / Caf - P.a / Car);       // gradient de sous-virage (s²/m)
        double kappa;   // courbure voulue de la trajectoire (1/m, + à droite)
        if (vCible < 0) {
            const double ld = 6.0;
            const TrackPoint T = piste.pointLisse(p.rd - ld / M, p.cibleX * RW);
            const double sn = std::sin(m.cap), cs = std::cos(m.cap), dxT = T.x - m.x, dzT = T.z - m.z;
            const double av = dxT * sn - dzT * cs, dr = dxT * cs + dzT * sn;
            kappa = 2 * dr / std::max(1.0, av * av + dr * dr);
            // manœuvre de dégagement : braquage à fond du côté opposé à la file visée (en marche arrière, l'avant pivote vers elle),
            // jusqu'à ≈ 60° par rapport à la route
            if (p.manoeuvre > 0) {
                const double sens = p.cibleX > p.x ? 1.0 : -1.0;   // + : file visée à droite, l'avant doit pivoter à droite
                kappa = p.h * sens < 1.05 ? -sens / 5.0 : 0.0;
            }
        } else {
            const double ld = std::clamp(0.9 * au + 4, 6.0, 40.0);
            const double kRoute = roues::courbureRoute(piste, p.rd), n = p.cibleX * RW;
            const double kFile = kRoute / std::max(0.2, 1 - kRoute * n);   // file décalée de n : rayon R − n
            const double ey = (p.cibleX - p.x) * RW;                       // + : la file visée est à droite
            // correction bornée : un changement de file se fait à 0,35 g au plus (conduite ferme mais pas brutale)
            const double kMax = 0.35 * P.mu * muRoute * physique::G / std::max(1.0, u * u);
            kappa = kFile + std::clamp(2 * (ey - std::sin(p.h) * ld) / (ld * ld), -kMax, kMax);
        }
        const double rVoulu = u * kappa;
        double roue = std::atan((P.L + Kus * u * u) * kappa) + (au > physique::V_DYNAMIQUE ? 0.08 * (rVoulu - m.r) : 0.0);
        const double roueMax = physique::braquageMax(P, au);
        roue = std::clamp(roue, -roueMax, roueMax);
        p.braquage = std::clamp(p.braquage + std::clamp(roue - p.braquage, -VITESSE_VOLANT * dt, VITESSE_VOLANT * dt), -roueMax, roueMax);

        // ---------- pédales : gaz, frein, marche arrière (rien d'autre) ----------
        physique::Commande c;
        c.braquage = p.braquage;
        if (vCible < -0.05) {
            if (u > 0.3) c.frein = 1;   // s'arrêter avant de passer la marche arrière
            else {
                const double err = u - vCible;   // > 0 : recule trop lentement
                if (err > 0) { c.recul = true; c.gaz = std::clamp(err * 0.4, 0.0, 1.0); }
                else if (err < -0.3) c.frein = std::clamp(-err * 0.3, 0.0, 1.0);
            }
        } else {
            const double err = vCible - u;
            if (u < -0.3) c.frein = 1;   // en arrière : d'abord s'arrêter
            else if (err > 0.1) c.gaz = std::clamp(err * 0.35, 0.0, 1.0);
            else if (err < -0.3) c.frein = std::clamp(-err * 0.25, 0.0, 1.0);
        }
        physique::Sol sol;
        const bool basCote = std::abs(p.x) > 1;
        sol.mu = muRoute * (basCote ? physique::MU_HERBE : 1.0); if (basCote) sol.crr = physique::CRR_HERBE;
        sol.pente = piste.pointLisse(p.rd, p.x * RW).slope * std::cos(p.h);
        physique::pas(P, m, c, sol, dt);
        projeter(piste, p);
    }

}

Police renfort(int indice, double sJ, double vJ, uint32_t graine) {
    Mulberry32 R(graine * 2654435761u + (uint32_t)indice * 40503u + 11u);
    Police p;
    p.role = indice % 3;
    p.x = p.cibleX = FILES[(int)(R.next() * 3) % 3];
    // WFS-338 (RG-05) : hors de la vue (avant : 90 m, dans le rétroviseur) ; WFS-540 : naît dans l'axe de sa file, monde construit
    // au premier pas
    poser(p, sJ - (APPARITION_M + indice * 9) / M, p.x, vJ);
    p.peinture = 0xe6e8ea;   // WFS-338 (RG-07) : police nationale : caisse blanche unie (albédo ≈ 0,8), avant : une sur deux bleu marine
    return p;
}

double approcheMax(double relM) { return std::clamp(APPROCHE_MIN + APPROCHE_PENTE * std::max(0.0, relM), APPROCHE_MIN, APPROCHE_MAX); }

double placeCote(double xJ, int role, bool* cote) {
    const double voulu = role == 1 ? -COTE_X : COTE_X;
    if (cote) *cote = true;
    if (std::abs(xJ + voulu) <= BORD_X) return xJ + voulu;
    if (std::abs(xJ - voulu) <= BORD_X) return xJ - voulu;   // le côté voulu est hors de la chaussée : l'autre côté
    if (cote) *cote = false;
    return xJ;                                               // aucun côté libre : derrière le joueur
}

double ecartCarrosseriesM(const std::vector<Police>& ps, double sJ, double xJ, const FichePolice& f) {
    double best = 1e9;
    for (const Police& p : ps) {
        if (p.fixe) continue;
        const double gz = std::abs(p.rd - sJ) * M - f.contactL, gx = std::abs(p.x - xJ) * RW - f.contactW;
        best = std::min(best, std::max(0.0, std::max(gz, gx)));
    }
    return best;
}

std::string texteDistancePolice(double m) {
    if (!(m < 1e8)) return "\xE2\x80\x94";   // « — » : aucune voiture de police mobile
    if (m < 1000) return std::to_string((int)std::lround(std::max(0.0, m))) + " M";
    const long c = std::lround(m / 10);   // centièmes de km, virgule française
    char b[32]; std::snprintf(b, sizeof b, "%ld,%02ld KM", c / 100, c % 100); return b;
}

bool sePlaceAvant(Surprise s) { return s != Surprise::Deboite; }

std::vector<Cone> chantier(int file) {
    // file fermée : bord extérieur (côté glissière) xe et limite avec la file voisine xi ; file du milieu : biseau de gauche
    // à droite, puis cônes sur ses deux limites
    std::vector<Cone> c;
    const double fx = FILES[std::clamp(file, 0, 2)], demi = 1.0 / 3.0;
    const double xe = file == 1 ? fx - demi + 0.04 : fx + (file == 0 ? -demi : demi) * 0.88;
    const double xi = file == 1 ? fx + demi - 0.04 : fx + (file == 0 ? demi : -demi) * 0.92;
    constexpr int BISEAU = 7; constexpr double LONG_BISEAU = 36;   // 7 cônes sur 36 m (écart latéral < largeur d'une voiture)
    for (int i = 0; i < BISEAU; i++) c.push_back({i * LONG_BISEAU / (BISEAU - 1), xe + (xi - xe) * i / (BISEAU - 1)});
    for (double d = LONG_BISEAU + 10; d <= 120; d += 10) {
        c.push_back({d, xi});
        if (file == 1) c.push_back({d, fx - demi + 0.04});
    }
    return c;
}

double plusProcheM(const std::vector<Police>& ps, double sJ, double xJ) {
    double best = 1e9;
    for (const Police& p : ps) {
        if (p.fixe) continue;
        const double dz = (p.rd - sJ) * M, dx = (p.x - xJ) * RW;
        best = std::min(best, std::sqrt(dz * dz + dx * dx));
    }
    return best;
}

// ---------------------------------------------------------------- programme (graine)
const char* bandeau(Surprise s) {
    switch (s) {
        case Surprise::Barrage: return "BARRAGE DE POLICE";
        case Surprise::Deboite: return "ATTENTION : VOITURE QUI DÉBOÎTE";
        case Surprise::Travaux: return "TRAVAUX : FILE FERMÉE";
        case Surprise::Accident: return "ACCIDENT DEVANT";
        case Surprise::Camion: return "CAMION EN TRAVERS";
        case Surprise::Ralentisseur: return "RALENTISSEUR";
        default: return "";
    }
}

Programme programme(uint32_t graine, bool banc) {
    Mulberry32 R(graine ? graine : 1);
    Programme p;
    // 3 embouteillages : vers 2,5, 5,5 et 8 km (± 0,6 km), 300 à 600 m, une file libre (passage étroit)
    for (int i = 0; i < 3; i++) {
        Bouchon b;
        b.km = 2.5 + i * 2.8 + (R.next() - 0.5) * 1.2;
        b.longueurM = 300 + R.next() * 300;
        b.passage = (int)(R.next() * 3) % 3;
        b.kmhMin = 20; b.kmhMax = 40;
        p.bouchons.push_back(b);
    }
    if (banc) { p.bouchons[0].km = 0.9; p.bouchons[0].longueurM = 400; p.bouchons[0].passage = 2; }   // captures : embouteillage à 0,9 km
    // 6 surprises : les 6 types mélangés (une fois chacun, WFS-119) ; une tous les 0,9 à 1,3 km dès 0,9 km
    std::vector<Surprise> types;
    for (int i = 0; i < TYPES_SURPRISE; i++) types.push_back((Surprise)i);
    for (int i = TYPES_SURPRISE - 1; i > 0; i--) std::swap(types[(size_t)i], types[(size_t)((int)(R.next() * (i + 1)) % (i + 1))]);
    double km = 0.9;
    for (Surprise t : types) {
        Evenement e; e.km = km; e.type = t; e.file = (int)(R.next() * 3) % 3;
        if (t == Surprise::Camion) e.file = fileLibreCamion(e.file);
        // pas de surprise dans un embouteillage
        for (int passe = 0; passe < 2; passe++)
            for (const Bouchon& b : p.bouchons) if (e.km > b.km - 0.25 && e.km < b.km + b.longueurM / 1000 + 0.1) e.km = b.km + b.longueurM / 1000 + 0.2;
        if (banc && e.km < 2.4) e.km = 2.4;   // banc : la place des captures (barrage, ralentisseur, camion) reste libre
        if (e.km < KM_ARRIVEE - 0.3) p.surprises.push_back(e);
        km = e.km + 0.9 + R.next() * 0.4;
    }
    if (banc) {
        p.surprises.insert(p.surprises.begin(), Evenement{BANC_CAMION_KM, Surprise::Camion, 2, false});
        p.surprises.insert(p.surprises.begin(), Evenement{BANC_RALENTISSEUR_KM, Surprise::Ralentisseur, 0, false});
    }
    if (banc) p.surprises.insert(p.surprises.begin(), Evenement{BANC_TRAVAUX_KM, Surprise::Travaux, 0, false});   // WFS-338 : chantier
    if (banc) p.surprises.insert(p.surprises.begin(), Evenement{0.45, Surprise::Barrage, 0, false});   // captures : barrage à 0,45 km
    return p;
}

// ---------------------------------------------------------------- camion en travers (WFS-119)
// WFS-404 (RG-03) : X5 retiré (licence NC) → Range Rover Sport ; joueur en Range Rover Sport → Jeep Compass (autre grand modèle libre)
const char* camionCar(const char* carJoueur) { return carJoueur && !std::strcmp(carJoueur, "rrsport") ? "compass" : "rrsport"; }
int fileLibreCamion(int file) { return file == 1 ? 0 : file; }
Travers camionTravers(int fileLibre, double longueurM, double largeurM) {
    // à cheval sur la limite des 2 files couvertes (±RW/3) : la file libre (0 : gauche, 2 : droite) n'est pas touchée
    Travers tv;
    tv.xM = (fileLibre == 0 ? 1 : -1) * RW / 3;
    tv.demiX = longueurM * CAMION_ECHELLE / 2;
    tv.demiZ = largeurM * CAMION_ECHELLE / 2;
    return tv;
}

// ---------------------------------------------------------------- mode gardé (WFS-119 RG-03)
std::string cleMode(ModeCourse m) { return m == ModeCourse::Course5 ? "course5" : m == ModeCourse::Poursuite ? "poursuite" : "checkpoint"; }
ModeCourse modeDeCle(const std::string& cle) { return cle == "course5" ? ModeCourse::Course5 : cle == "poursuite" ? ModeCourse::Poursuite : ModeCourse::Checkpoint; }

// WFS-141 (RG-01) : 2 Hz = période de 0,5 s : rouge la 1re moitié, bleu la 2e (une voiture sur deux : l'inverse)
Gyrophare gyrophare(double t, int voiture) {
    const double ph = t - std::floor(t / 0.5) * 0.5;   // 0 ≤ ph < 0,5 (t négatif compris)
    const bool premier = (ph < 0.25) != (voiture % 2 == 1);
    Gyrophare g; g.rouge = premier; g.bleu = !premier;
    return g;
}
// WFS-141 (RG-03) : « 2e PLACE » : le « e » reste minuscule (ui::majuscules le mettait en capitale, comme ANO-35)
std::string titrePlace(int rang) { return rang <= 1 ? "VICTOIRE" : std::to_string(rang) + "e PLACE"; }

} // namespace poursuite
} // namespace wfs
