// WaForSpeed — banc de la direction dans les virages (WFS-295, ANO-168, WFS_BANC=virages). Les captures « avant » rejouent le
// comportement de main avant la correction (g_avantWfs295, banc seulement), les « après » le jeu corrigé :
//  1. virages-dessus-avant / -apres : épingle à gauche des lacets de la Corniche vue de dessus ; Golf au bord extérieur, flèche
//     gauche tenue 2 s à 10, 20, 30 et 50 km/h (traces : bleu, vert, orange, rouge), rayon de chaque trace au journal ;
//  2. virages-proche-avant-NN / -apres-NN : la même épingle dans la partie (vue Proche), aux flèches, file extérieure, 20 km/h ;
//  3. virages-choc-avant-NN / -apres-NN : accident à 60 km/h (impact 0,45) flèche droite tenue : 2 s d'images après le choc ;
//  4. virages-rond-point-sans-aide-NN / -aide-NN : rond-point de la Ville aux flèches, aide désactivée puis activée ;
//  5. virages-cercle-10kmh : cercles volant à fond à 10 km/h (Golf en bleu, M4 en rouge) et cercle de la fiche (blanc) ;
//  6. virages-gp-aide-avant-NN / -apres-NN (WFS-258, vidéo de l'utilisateur) : Grand Prix, Gallardo lancée à pleine vitesse,
//     aide activée, gaz tenu, volant lâché, 1er virage (épingle de 20 m) ; « avant » : l'aide ne ralentit pas (g_avantWfs258) ;
//  8. virages-vif-dessus-avant / -apres (WFS-258, point 3) : ligne droite de la Ville vue de dessus, quart de volant 0,5 s à
//     150, 190 et 230 km/h ; 9. virages-vif-avant-NN / -apres-NN : la même chose dans la partie (Grand Prix, Gallardo) ;
// 10. virages-rapide-tenu-avant / -apres : virage rapide du Grand Prix, aide activée, doigt tenu (revue du lead dev, PR #271) ;
//  7. classement (sans capture) : Course à 5 de 2 km sur les 4 cartes (tirage fixe), joueur gaz tenu, volant lâché ou presque (pilote
//     automatique) puis tenu (« Glisser »), aide activée, avec et sans ralentissement de l'aide : place, temps, images hors route (virages.txt ; décision du PO : jamais systématiquement dernier).
// Mesures sans GPU (rayons, épingles, choc) : virages.txt (mêmes calculs que tests/test_virages_fleches.cpp).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include "../../third_party/stb/stb_image_write.h"
#include "../core/log.h"
#include "../render/renderer.h"
#include "../ui/text.h"
#include "conduite_banc.h"
#include "course5.h"
#include "course_system.h"
#include "game.h"
#include "trace_cartes.h"
#include "virages_banc.h"

namespace wfs {
const std::vector<course5::Ordinateur>* course5Ordinateurs();   // course5_system.cpp (accroche de WFS-272)
namespace {
using namespace banc_virages;

// image de la vue de dessus (1280 × 720), cadrée sur une boîte du monde
struct Dessin {
    int w = 1280, h = 720; std::vector<unsigned char> px; double x0 = 0, z0 = 0, ech = 1;
    Dessin() : px((size_t)1280 * 720 * 3, 0) { for (size_t i = 0; i < px.size(); i += 3) { px[i] = 60; px[i + 1] = 110; px[i + 2] = 50; } }   // herbe
    void cadrer(double xmin, double xmax, double zmin, double zmax) {
        ech = std::min((w - 40) / std::max(1.0, xmax - xmin), (h - 40) / std::max(1.0, zmax - zmin));
        x0 = (xmin + xmax) / 2; z0 = (zmin + zmax) / 2;
    }
    void point(double x, double z, unsigned r, unsigned g, unsigned b, int rayonPx = 1) {
        const int cx = (int)std::lround(w / 2 + (x - x0) * ech), cy = (int)std::lround(h / 2 + (z - z0) * ech);
        for (int dy = -rayonPx; dy <= rayonPx; dy++) for (int dx = -rayonPx; dx <= rayonPx; dx++) {
            const int X = cx + dx, Y = cy + dy;
            if (X < 0 || Y < 0 || X >= w || Y >= h) continue;
            unsigned char* q = &px[((size_t)Y * w + X) * 3]; q[0] = (unsigned char)r; q[1] = (unsigned char)g; q[2] = (unsigned char)b;
        }
    }
    void route(Track& t, double s0, double s1) {   // chaussée grise, bords blancs, axe jaune
        for (double s = s0; s < s1; s += 0.25 / M) {
            for (double n = -RW; n <= RW; n += 0.25) { const TrackPoint P = t.pointLisse(s, n); point(P.x, P.z, 95, 95, 100); }
            for (double n : {-RW, RW}) { const TrackPoint P = t.pointLisse(s, n); point(P.x, P.z, 240, 240, 240); }
            const TrackPoint C = t.pointLisse(s, 0); point(C.x, C.z, 230, 200, 40, 0);
        }
    }
    bool ecrire(const std::string& f) { return stbi_write_png(f.c_str(), w, h, 3, px.data(), w * 3) != 0; }
};
constexpr unsigned COULEURS[4][3] = {{60, 140, 255}, {60, 220, 90}, {255, 160, 30}, {240, 50, 50}};

// 1. vue de dessus de l'épingle : flèche tenue depuis le bord extérieur, 4 vitesses
void dessus(Track& plage, const Epingle& e, bool avant, const std::string& out, std::string& rapport) {
    g_avantWfs295 = avant;
    Dessin d;
    std::vector<std::vector<PoseMonde>> traces;
    double xmin = 1e9, xmax = -1e9, zmin = 1e9, zmax = -1e9;
    const double s = e.debut + 8 / M;
    for (double s2 = e.debut - 15 / M; s2 < e.fin + 15 / M; s2 += 1 / M) for (double n : {-RW - 4, RW + 4}) {
        const TrackPoint P = plage.pointLisse(s2, n); xmin = std::min(xmin, P.x); xmax = std::max(xmax, P.x); zmin = std::min(zmin, P.z); zmax = std::max(zmax, P.z);
    }
    d.cadrer(xmin, xmax, zmin, zmax);
    d.route(plage, e.debut - 15 / M, e.fin + 15 / M);
    char l[200];
    const double kmhs[4] = {10, 20, 30, 50};
    for (int i = 0; i < 4; i++) {
        std::vector<PoseMonde> tr;
        const double r = rayon("golf", plage, s, -e.sens * 0.95 * RW, kmhs[i], (float)e.sens, 2.0, &tr);
        for (const PoseMonde& q : tr) d.point(q.x, q.z, COULEURS[i][0], COULEURS[i][1], COULEURS[i][2], 2);
        std::snprintf(l, sizeof l, "- %s, %2.0f km/h, flèche tenue depuis le bord extérieur : rayon %.2f m\n", avant ? "avant" : "après", kmhs[i], r);
        rapport += l;
    }
    d.ecrire(out + (avant ? "/virages-dessus-avant.png" : "/virages-dessus-apres.png"));
    g_avantWfs295 = false;
}

// 5. cercles à 10 km/h (Golf, M4) et cercle de la fiche
void cercles(Track& ville, const std::string& out, std::string& rapport) {
    Dessin d; d.cadrer(-12, 12, -12, 12);
    const char* ids[2] = {"golf", "m4"}; const unsigned col[2][3] = {{60, 140, 255}, {240, 50, 50}};
    char l[200];
    for (int i = 0; i < 2; i++) {
        std::vector<PoseMonde> tr;
        const double r = rayon(ids[i], ville, 50 / M, 0, 10, 1.f, 10.0, &tr), f = rayonFiche(ids[i]);
        const TrackPoint O = ville.pointLisse(50 / M, 0);
        for (const PoseMonde& q : tr) d.point(q.x - O.x, q.z - O.z, col[i][0], col[i][1], col[i][2], 2);
        // cercle de la fiche, même centre que la trace (centre = point de départ + rayon vers la droite de la voiture)
        const double cx = std::cos(O.th) * f, cz = std::sin(O.th) * f;
        for (double a = 0; a < 6.2832; a += 0.004) d.point(cx + f * std::cos(a), cz + f * std::sin(a), 240, 240, 240, 0);
        std::snprintf(l, sizeof l, "- %s, volant à fond à 10 km/h : rayon %.2f m ; fiche %.2f m (%+.0f %%)\n", ids[i], r, f, 100 * (r / f - 1));
        rapport += l;
    }
    d.ecrire(out + "/virages-cercle-10kmh.png");
}

// lance la partie sur `carte`, départ au segment `depart`, aide réglée ; rend faux si le chargement échoue
bool lancer(Game& g, Renderer& r, const char* carte, int64_t depart, bool aide, double& temps, const char* voiture = "golf") {
    reglagesConduite().aideDirection = aide;
    setenv("WFS_DEPART", std::to_string(std::max<int64_t>(0, depart)).c_str(), 1);
    courseBenchRules(true);
    const bool ok = g.init(r, voiture, carte);
    unsetenv("WFS_DEPART");
    if (!ok) return false;
    g.setAutopilot(false); g.modeManette = true; g.manette = DriveInput{}; g.camMode = 0;
    g.allerA(Ecran::Course);
    for (int i = 0; i < 60 * 4; i++) { g.update(1.0 / 60); temps += 1.0 / 60; if (i % 20 == 0) g.render(temps); }   // décompte
    return true;
}
void capturer(Game& g, Renderer& r, double temps, const std::string& out, const char* nom, int i) {
    g.render(temps + 0.005);
    char f[128]; std::snprintf(f, sizeof f, "%s/virages-%s-%02d.png", out.c_str(), nom, i);
    r.capturePng(f);
    const Player& p = g.player();
    WFS_LOG("capture virages-%s-%02d : %3.0f km/h, volant %+5.2f, écart %+5.2f m, cap %+5.2f rad, blocage %.2f s", nom, i, p.kmh(), p.steer,
            p.playerX * RW, p.heading, p.controlLock);
}

// 2 et 4. une épingle ou le rond-point, joueur aux flèches sur la file `file`, vitesse tenue ; 8 images
int sequence(Game& g, Renderer& r, const char* carte, double debutU, double finU, double file, double kmh, bool aide, const char* nom,
             const std::string& out, double& temps) {
    if (!lancer(g, r, carte, (int64_t)((debutU - 70 / M) / SEG), aide, temps)) return 1;
    Track& t = banc_conduite::piste(carte);
    const Player& p = g.player();
    const double d0 = debutU - 10 / M, d1 = finU + 15 / M;
    int pris = 0, herbe = 0;
    for (int n = 0; n < 60 * 120 && pris < 8; n++) {
        DriveInput in = tenir(p, kmh, piloter(p, t, p.dist < debutU - 25 / M ? file * 1.5 : file, true));   // élan : bord extérieur
        g.manette = in; g.update(1.0 / 60); temps += 1.0 / 60;
        if (p.dist > d0 && std::abs(p.playerX) > 1) herbe++;
        const bool prendre = p.dist >= d0 + (d1 - d0) * pris / 7;
        if (n % 10 == 0 || prendre) g.render(temps);
        if (prendre) capturer(g, r, temps, out, nom, pris++);
    }
    WFS_LOG("virages-%s : %d image(s) de jeu sur l'herbe dans le virage", nom, herbe);
    g.manette = DriveInput{}; g.modeManette = false; g.shutdown();
    return pris < 8 ? 1 : 0;
}

// 3. accident à 60 km/h, flèche droite tenue avant et après : 6 images sur les 2 s qui suivent
int choc(Game& g, Renderer& r, bool avant, const std::string& out, double& temps) {
    if (!lancer(g, r, "ville", (int64_t)(40 / M / SEG), false, temps)) return 1;
    // banc seulement : l'accident est déclenché directement sur la voiture du joueur (Player::accident, comme un choc du
    // trafic, impact 0,45 de Course::crash) ; Game n'expose que la lecture du joueur
    Player& p = const_cast<Player&>(g.player());
    int n = 0;
    for (; n < 60 * 20 && p.kmh() < 60; n++) { g.manette = tenir(p, 60, 0.f); g.update(1.0 / 60); temps += 1.0 / 60; if (n % 10 == 0) g.render(temps); }
    for (int k = 0; k < 15; k++) { g.manette = tenir(p, 60, 1.f); g.update(1.0 / 60); temps += 1.0 / 60; }   // flèche droite, 0,25 s
    g_avantWfs295 = avant;
    p.accident(0.45, 1);
    const char* nom = avant ? "choc-avant" : "choc-apres";
    int pris = 0;
    for (int k = 0; k <= 120 && pris < 6; k++) {
        if (k % 24 == 0) capturer(g, r, temps, out, nom, pris++);
        g.manette = tenir(p, 60, 1.f); g.update(1.0 / 60); temps += 1.0 / 60;
        if (k % 6 == 0) g.render(temps);
    }
    g_avantWfs295 = false;
    g.manette = DriveInput{}; g.modeManette = false; g.shutdown();
    return pris < 6 ? 1 : 0;
}

// 6. WFS-258 : 1er virage du Grand Prix, Gallardo lancée gaz tenu, aide activée, volant lâché ; 8 images de 150 m avant à 60 m après
int premierVirageGP(Game& g, Renderer& r, bool avant, const std::string& out, double& temps) {
    Track& t = banc_conduite::piste("circuit");
    double d0 = 0, d1 = 0; bool dans = false;   // 1er virage : rayon < 150 m après 200 m (comme tests/test_aide_ralentit.cpp)
    for (double s = 200 / M; s < t.length(); s += 1 / M) {
        const double R = 1 / std::max(1e-9, std::abs(courbure(t, s)));
        if (R < 150 && !dans) { dans = true; d0 = s; }
        if (dans && R > 300) { d1 = s; break; }
    }
    if (!lancer(g, r, "circuit", 0, true, temps, "gallardo")) return 1;
    g_avantWfs258 = avant;
    const Player& p = g.player();
    const char* nom = avant ? "gp-aide-avant" : "gp-aide-apres";
    const double a = d0 - 150 / M, b = d1 + 60 / M;
    int pris = 0, herbe = 0; double vMax = 0;
    for (int n = 0; n < 60 * 120 && pris < 8; n++) {
        DriveInput in; in.gas = true;
        g.manette = in; g.update(1.0 / 60); temps += 1.0 / 60;
        vMax = std::max(vMax, p.kmh());
        if (p.dist > d0 - 20 / M && p.dist < b) herbe += std::abs(p.playerX) > 1;
        const bool prendre = p.dist >= a + (b - a) * pris / 7 || (n > 60 * 110);
        if (n % 10 == 0 || prendre) g.render(temps);
        if (prendre) { capturer(g, r, temps, out, nom, pris++); WFS_LOG("  %s : %+.0f m du virage, aide qui freine : %s", nom, (p.dist - d0) * M, p.freinAide ? "oui" : "non"); }
    }
    WFS_LOG("virages-%s : %.0f km/h au plus haut, %d image(s) de jeu hors route dans le virage", nom, vMax, herbe);
    g_avantWfs258 = false;
    g.manette = DriveInput{}; g.modeManette = false; g.shutdown();
    return pris < 8 ? 1 : 0;
}

// 7. WFS-258 : Course à 5 de 2 km, joueur au pilote automatique (gaz tenu, suit sa file), aide activée ; place à l'arrivée
int classementCourse5(Game& g, Renderer& r, const char* carte, bool avant, bool volant, std::string& rapport, double& temps, bool avantVif = false) {
    modeCourse() = ModeCourse::Course5; kmCourse5() = 2;
    reglagesConduite().aideDirection = true;
    if (!g.init(r, "golf", carte)) { modeCourse() = ModeCourse::Checkpoint; return 1; }
    (avantVif ? g_avantWfs258Vif : g_avantWfs258) = avant;   // « avant » : main avant le point 1 (aide) ou avant le point 3 (direction)
    g.allerA(Ecran::Menu); for (int i = 0; i < 30; i++) g.update(1.0 / 60);
    g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(!volant); g.modeManette = volant;
    const Player& p = g.player();
    Track& tr = banc_conduite::piste(carte);
    const Course* c = activeCourse();
    int herbe = 0, n = 0;
    for (; n < 60 * 300 && !(c && c->state == RaceState::Over); n++) {
        if (volant) { DriveInput in; in.gas = true; in.steer = piloter(p, tr, 0, false); g.manette = in; }   // « Glisser » : vise l'axe 9 m devant
        g.update(1.0 / 60); temps += 1.0 / 60; c = activeCourse();
        if (c && c->playing()) herbe += std::abs(p.playerX) > 1;
        if (n % 600 == 0) g.render(temps);
    }
    int place = 1;
    if (const auto* os = course5Ordinateurs()) for (const auto& o : *os) place += o.g.done ? 1 : 0;
    char l[220];
    std::snprintf(l, sizeof l, "- %s, %s, %s : %s, place %d/5, %.1f s de course, %d image(s) hors route\n", carte,
                  volant ? "volant tenu (Glisser)" : "volant lâché ou presque (pilote)", avant ? (avantVif ? "direction de main (avant le point 3)" : "sans ralentissement (main)") : (avantVif ? "direction réduite (point 3)" : "aide qui ralentit"), c && c->state == RaceState::Over ? "arrivé" : "PAS arrivé", place,
                  c ? c->raceTime : 0.0, herbe);
    rapport += l; WFS_LOG("classement : %s", l);
    g_avantWfs258 = false; g_avantWfs258Vif = false; g.setAutopilot(false); g.manette = DriveInput{}; g.modeManette = false; g.shutdown();
    modeCourse() = ModeCourse::Checkpoint;
    return 0;
}

// 8. WFS-258 (point 3) : ligne droite de la Ville vue de dessus (120 m, avant le virage qui suit), Golf à 150, 190 et 230 km/h (vert, orange, rouge), un quart
// de volant à droite tenu 0,5 s puis lâché, aide désactivée ; « avant » : angle des roues de main (g_avantWfs258Vif)
void dessusVif(Track& ville, bool avant, const std::string& out, std::string& rapport) {
    g_avantWfs258Vif = avant;
    Dessin d;
    const double s0 = 50 / M;
    double xmin = 1e9, xmax = -1e9, zmin = 1e9, zmax = -1e9;
    for (double s = s0 - 10 / M; s < s0 + 130 / M; s += 1 / M) for (double n : {-RW - 4, RW + 4}) {
        const TrackPoint P = ville.pointLisse(s, n); xmin = std::min(xmin, P.x); xmax = std::max(xmax, P.x); zmin = std::min(zmin, P.z); zmax = std::max(zmax, P.z);
    }
    d.cadrer(xmin, xmax, zmin, zmax);
    d.route(ville, s0 - 10 / M, s0 + 130 / M);
    char l[200];
    const double kmhs[3] = {150, 190, 230};
    for (int i = 0; i < 3; i++) {
        Player p = voiture("golf", Aide::Desactivee, ville, s0, -0.6 * RW, kmhs[i]);
        double ecart = 0;
        for (int k = 0; k < 120 * 3 && p.dist < s0 + 120 / M; k++) {
            p.update(DT, tenir(p, kmhs[i], k < 60 ? 0.25f : 0.f), ville, true);
            const PoseMonde q = pose(p, ville);
            d.point(q.x, q.z, COULEURS[i + 1][0], COULEURS[i + 1][1], COULEURS[i + 1][2], 2);
            if (k == 60) ecart = (p.playerX + 0.6) * RW;
        }
        std::snprintf(l, sizeof l, "- %s, %3.0f km/h, quart de volant 0,5 s : décalage de %.2f m vers la droite\n", avant ? "avant" : "après", kmhs[i], ecart);
        rapport += l;
    }
    d.ecrire(out + (avant ? "/virages-vif-dessus-avant.png" : "/virages-vif-dessus-apres.png"));
    g_avantWfs258Vif = false;
}

// 9. WFS-258 (point 3) : ligne droite du Grand Prix, Gallardo lancée gaz tenu (≈ 200 km/h et plus), aide désactivée, un quart de
// volant à droite tenu 0,5 s ; 6 images toutes les 0,3 s depuis le coup de volant
int changementDeFile(Game& g, Renderer& r, bool avant, const std::string& out, double& temps) {
    if (!lancer(g, r, "circuit", 0, false, temps, "gallardo")) return 1;
    g_avantWfs258Vif = avant;
    const Player& p = g.player();
    const char* nom = avant ? "vif-avant" : "vif-apres";
    int n = 0;
    for (; n < 60 * 30 && p.kmh() < 200; n++) { DriveInput in; in.gas = true; g.manette = in; g.update(1.0 / 60); temps += 1.0 / 60; if (n % 20 == 0) g.render(temps); }
    const double x0 = p.playerX;
    int pris = 0;
    for (int k = 0; k <= 90 && pris < 6; k++) {
        if (k % 18 == 0) { capturer(g, r, temps, out, nom, pris++); WFS_LOG("  %s : décalage %+.2f m depuis le coup de volant", nom, (p.playerX - x0) * RW); }
        DriveInput in; in.gas = true; in.steer = k < 30 ? 0.25f : 0.f;
        g.manette = in; g.update(1.0 / 60); temps += 1.0 / 60;
        if (k % 6 == 0) g.render(temps);
    }
    g_avantWfs258Vif = false;
    g.manette = DriveInput{}; g.modeManette = false; g.shutdown();
    return pris < 6 ? 1 : 0;
}

// 10. WFS-258 (point 3, revue du lead dev) : virage rapide du Grand Prix (rayon 60 m, à 1,9 km du départ), Golf, aide activée,
// doigt tenu (« Glisser », vise l'axe 9 m devant), gaz tenu ; 8 images de 200 m avant à 60 m après ; « avant » : direction de
// main (g_avantWfs258Vif : angle de roues d'avant, limite de l'aide qui cède au doigt à 0,35)
int virageRapideTenu(Game& g, Renderer& r, bool avant, const std::string& out, double& temps) {
    constexpr double D0 = 1880, D1 = 1940;   // m : le virage où la Golf sortait (revue du lead dev : 1 316 images par tour)
    if (!lancer(g, r, "circuit", (int64_t)((D0 - 700) / SEG_M), true, temps)) return 1;
    g_avantWfs258Vif = avant;
    Track& t = banc_conduite::piste("circuit");
    const Player& p = g.player();
    const char* nom = avant ? "rapide-tenu-avant" : "rapide-tenu-apres";
    const double a = (D0 - 200) / M, b = (D1 + 60) / M;
    int pris = 0, herbe = 0;
    for (int n = 0; n < 60 * 120 && pris < 8; n++) {
        DriveInput in; in.gas = true; in.steer = piloter(p, t, 0, false);
        g.manette = in; g.update(1.0 / 60); temps += 1.0 / 60;
        if (p.dist > a && p.dist < b) herbe += std::abs(p.playerX) > 1;
        const bool prendre = p.dist >= a + (b - a) * pris / 7 || n > 60 * 110;
        if (n % 10 == 0 || prendre) g.render(temps);
        if (prendre) { capturer(g, r, temps, out, nom, pris++); WFS_LOG("  %s : %+.0f m du virage, doigt %+.2f, aide qui freine : %s", nom, (p.dist * M - D0), in.steer, p.freinAide ? "oui" : "non"); }
    }
    WFS_LOG("virages-%s : %d image(s) de jeu hors route du virage", nom, herbe);
    g_avantWfs258Vif = false;
    g.manette = DriveInput{}; g.modeManette = false; g.shutdown();
    return pris < 8 ? 1 : 0;
}

} // namespace

int benchVirages(Game& g, Renderer& r, const std::string& out) {
    ui::chargerPolice(r);
    const bool aideAvant = reglagesConduite().aideDirection, reglesAvant = courseBenchRulesOn();
    int ecarts = 0; std::string rapport = "# WFS-295 : direction dans les virages\n";
    Track& plage = banc_conduite::piste("plage");
    Track& ville = banc_conduite::piste("ville");
    const std::vector<Epingle> ep = epingles(plage);
    const Epingle* gauche = nullptr;
    for (const Epingle& e : ep) if (e.sens < 0) { gauche = &e; break; }
    if (!gauche) { WFS_ERR("virages : épingle à gauche des lacets introuvable"); return 1; }
    // WFS_VIRAGES=vif : seulement les sections 8 et 9 (direction à haute vitesse), pour les captures de cette PR
    const char* seul = std::getenv("WFS_VIRAGES");
    const bool tout = !seul || std::strcmp(seul, "vif") != 0;
    double temps = 9000;
    if (tout) {
        rapport += "## 1. Épingle à gauche des lacets de la Corniche, vue de dessus (bleu 10, vert 20, orange 30, rouge 50 km/h)\n";
        dessus(plage, *gauche, true, out, rapport);
        dessus(plage, *gauche, false, out, rapport);
        rapport += "## 5. Cercles volant à fond à 10 km/h\n";
        cercles(ville, out, rapport);
        for (bool avant : {true, false}) {   // 2. épingle en vue Proche, aux flèches, file extérieure
            g_avantWfs295 = avant;
            ecarts += sequence(g, r, "plage", gauche->debut, gauche->fin, 0.6, 20, false, avant ? "proche-avant" : "proche-apres", out, temps);
            g_avantWfs295 = false;
        }
        ecarts += choc(g, r, true, out, temps);    // 3. choc puis flèche
        ecarts += choc(g, r, false, out, temps);
        ecarts += premierVirageGP(g, r, true, out, temps);    // 6. WFS-258 : aide qui ralentit à temps
        ecarts += premierVirageGP(g, r, false, out, temps);
    }
    rapport += "## 8. Direction à haute vitesse (WFS-258, point 3) : ligne droite de la Ville, Golf, quart de volant 0,5 s\n";
    dessusVif(ville, true, out, rapport);
    dessusVif(ville, false, out, rapport);
    ecarts += changementDeFile(g, r, true, out, temps);    // 9. dans la partie, Grand Prix
    ecarts += changementDeFile(g, r, false, out, temps);
    ecarts += virageRapideTenu(g, r, true, out, temps);    // 10. virage rapide, aide activée, doigt tenu
    ecarts += virageRapideTenu(g, r, false, out, temps);
    if (!tout) {   // classement en Course à 5, doigt tenu, direction de main / réduite (point 3)
        rapport += "## 10 bis. Course à 5 de 2 km, aide activée, doigt tenu (« Glisser ») : direction de main / réduite au-dessus de 100 km/h\n";
        for (const char* carte : {"ville", "campagne", "plage", "circuit"})
            for (bool avant : {true, false}) ecarts += classementCourse5(g, r, carte, avant, true, rapport, temps, true);
    }
    if (tout) {
        rapport += "## 7. Course à 5 de 2 km, joueur gaz tenu (pilote automatique), aide activée : avec et sans ralentissement de l'aide\n";
        for (const char* carte : {"ville", "campagne", "plage", "circuit"})   // tirage fixe du banc (graine 1234 sous WFS_BANC)
            for (bool volant : {false, true}) for (bool avant : {true, false}) ecarts += classementCourse5(g, r, carte, avant, volant, rapport, temps);
        const banc_conduite::Virage* rp = nullptr;   // 4. rond-point de la Ville, aide désactivée puis activée
        const std::vector<banc_conduite::Virage> vs = banc_conduite::virages();
        for (const auto& v : vs) if (std::strstr(v.nom, "rond-point de la Ville")) rp = &v;
        if (rp) {
            ecarts += sequence(g, r, "ville", rp->debutU, rp->finU, 0.6, 30, false, "rond-point-sans-aide", out, temps);
            ecarts += sequence(g, r, "ville", rp->debutU, rp->finU, 0.6, 30, true, "rond-point-aide", out, temps);
        } else { WFS_ERR("virages : rond-point de la Ville introuvable"); ecarts++; }
    }
    reglagesConduite().aideDirection = aideAvant; courseBenchRules(reglesAvant);
    { std::ofstream f(out + "/virages.txt"); f << rapport; }
    WFS_LOG("banc des virages (WFS-295) : %d écart(s)%s", ecarts, ecarts ? " ÉCHEC" : " OK");
    return ecarts;
}

} // namespace wfs
