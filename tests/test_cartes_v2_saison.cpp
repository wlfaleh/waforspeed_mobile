// WFS-346 : saison des cartes de données (src/world/cartes_v2/carte_donnees.*) — neige au sol, flocons et adhérence lus du bloc decor
// (RG-02, RG-03), valeurs par défaut sûres (carte du code : sec, sans neige, mêmes μ qu'avant), refus des valeurs hors limites,
// carte de Noël (neige) et carte d'automne (sans neige) changées par les données seules ; conduite : distance de freinage 100 → 0 avec
// ABS sur la neige tassée de la carte (μ 0,25 : d = v² / (2 μ g) ≈ 157 m, references/reel/cartes_v2/neige.md) contre route sèche ;
// virage sur neige : accélération latérale tenue ≤ μ g ; flocons (meteo.h) : nombre, boucle autour de la caméra, toits.
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include "../src/game/cars.h"
#include "../src/game/conduite_monde.h"
#include "../src/game/player.h"
#include "../src/game/track.h"
#include "../src/render/route_revetements.h"
#include "../src/world/cartes_v2/carte_donnees.h"
#include "../src/world/meteo.h"
using namespace wfs;
using namespace wfs::cartes_v2;

static int g_echecs = 0;
#define VERIFIER(c) do { if (!(c)) { std::printf("ÉCHEC ligne %d : %s\n", __LINE__, #c); g_echecs++; } } while (0)

static std::string lire(const std::string& f) { std::ifstream in(f, std::ios::binary); return std::string(std::istreambuf_iterator<char>(in), {}); }
static const std::string TRACE_MIN = "format 1\nnom Essai\nroute 50 200 50 0 0\n";   // 300 segments
static bool decor(const std::string& t, DonneesCarte& d, std::string& e) { d = {}; d.id = "x"; std::string e0; lireTrace(TRACE_MIN, d, e0); return lireDecor(t, d, e); }

// distance d'arrêt (m) de 100 km/h, frein à fond, ABS, sur un sol de μ donné
static double arret(const physique::Params& p, double mu) {
    using namespace physique;
    Etat e; e.vz = -100 / 3.6; Commande c; c.frein = 1; c.abs = true; Sol s; s.mu = mu;
    double d = 0;
    for (int i = 0; i < 120 * 30 && e.u() > 0.01; i++) { const double z0 = e.z; pas(p, e, c, s, 1.0 / 120); d += z0 - e.z; }
    return d;
}

int main() {
    std::string e;
    const std::string racine = std::string(WFS_SOURCE_DIR) + "/paquets/sources/";

    // ---- valeurs par défaut sûres : sans ligne de saison, carte sèche ; mêmes μ que la conduite d'avant (route 1, bas-côté MU_HERBE)
    const Saison def;
    VERIFIER(def.muChaussee == 1.0f && std::fabs(def.muBasCote - physique::MU_HERBE) < 1e-6 && def.neigeChaussee == 0 && def.flocons == 0);
    VERIFIER(physique::Sol{}.mu == def.muChaussee);
    preparerSaison("ville");   // carte du code : saison par défaut
    VERIFIER(saisonPartie().muChaussee == 1.0f && saisonPartie().neigeChaussee == 0 && saisonPartie().flocons == 0);
    preparerSaison(nullptr);
    VERIFIER(saisonPartie().flocons == 0);

    // ---- lecture des lignes de saison et refus des valeurs hors limites (RG-03)
    DonneesCarte d;
    VERIFIER(decor("format 2\nadherence 0.25 0.2\nneige_chaussee 0.75\nflocons 0.6 1.2 3.5\n", d, e));
    VERIFIER(std::fabs(d.saison.muChaussee - 0.25f) < 1e-6 && std::fabs(d.saison.muBasCote - 0.2f) < 1e-6 && d.saison.neigeChaussee == 0.75f);
    VERIFIER(std::fabs(d.saison.flocons - 0.6f) < 1e-6 && std::fabs(d.saison.chuteMs - 1.2f) < 1e-6 && std::fabs(d.saison.floconMm - 3.5f) < 1e-6);
    VERIFIER(decor("format 2\nherbe 1\n", d, e) && d.saison.muChaussee == 1.0f && d.saison.flocons == 0);   // rien de la carte précédente
    VERIFIER(!decor("format 2\nadherence 0 0.2\n", d, e) && e.find("adherence") != std::string::npos);
    VERIFIER(!decor("format 2\nadherence 2 0.2\n", d, e));
    VERIFIER(!decor("format 2\nadherence 0.3\n", d, e));
    VERIFIER(!decor("format 2\nneige_chaussee 1.5\n", d, e));
    VERIFIER(!decor("format 2\nflocons 0.5 9 3\n", d, e) && e.find("flocons") != std::string::npos);   // chute impossible
    VERIFIER(!decor("format 2\nflocons 0.5 1.2 40\n", d, e));                                            // flocon de 4 cm
    VERIFIER(!decor("format 3\nherbe 1\n", d, e) && e.find("plus récent") != std::string::npos);

    // ---- les deux cartes d'essai : tout change par les données seules (RG-03)
    DonneesCarte n; n.id = "noel-test";
    VERIFIER(lireTrace(lire(racine + "noel-test/trace.txt"), n, e) && lireDecor(lire(racine + "noel-test/decor.txt"), n, e));
    VERIFIER(n.saison.muChaussee >= 0.2f && n.saison.muChaussee <= 0.3f);   // neige tassée (μ 0,2 à 0,3)
    VERIFIER(n.saison.neigeChaussee > 0.9f && n.saison.flocons > 0 && n.saison.chuteMs >= 1.0f && n.saison.chuteMs <= 1.5f);
    VERIFIER(n.saison.floconMm >= 2 && n.saison.floconMm <= 5);
    bool tunnel = false; for (const trace::Section& s : n.sections) tunnel = tunnel || s.type == trace::Type::Tunnel;
    VERIFIER(tunnel);   // abri de la carte d'essai : aucun flocon dessous
    DonneesCarte a; a.id = "automne-test";
    VERIFIER(lireTrace(lire(racine + "automne-test/trace.txt"), a, e) && lireDecor(lire(racine + "automne-test/decor.txt"), a, e));
    VERIFIER(a.saison.neigeChaussee == 0 && a.saison.flocons == 0 && a.saison.muChaussee > 0.8f && a.saison.muBasCote < physique::MU_HERBE);
    VERIFIER(enregistrer(n, e));
    preparerSaison("noel-test");
    VERIFIER(saisonPartie().muChaussee == n.saison.muChaussee && saisonPartie().flocons == n.saison.flocons);
    preparerSaison("ville");
    VERIFIER(saisonPartie().muChaussee == 1.0f && saisonPartie().flocons == 0);   // retour sur une carte du code : sec
    oublier();

    // ---- route : neige au sol transmise au shader par huitièmes, sans toucher au mode ni à la carte (route.glsl)
    for (int q = 0; q <= 3; q++) for (int c = 0; c < 4; c++) for (int k = 0; k <= route::NEIGE_PAS; k++) {
        const float z = route::codeMateriau(q, (route::Carte)c, (float)k / route::NEIGE_PAS);
        const float mode = std::fmod(z, 4.f), carte = std::fmod(std::floor(z / 4.f + 0.01f), 4.f), neige = std::floor(z / 16.f + 0.01f) / 8.f;
        VERIFIER(mode == (q >= 2 ? 2.f : q == 1 ? 1.f : 0.f) && (int)carte == c && std::fabs(neige - (float)k / route::NEIGE_PAS) < 1e-6);
    }
    VERIFIER(route::codeMateriau(2, route::Carte::Campagne) == route::codeMateriau(2, route::Carte::Campagne, 0));   // cartes du code inchangées

    // ---- conduite (RG-02) : freinage 100 → 0 avec ABS, chaque voiture : neige tassée ≈ v² / (2 μ g)
    double pire = 0;
    for (const CarDef& c : CARS) {
        const physique::Params& p = physique::params(c);
        const double sec = arret(p, 1.0), neige = arret(p, n.saison.muChaussee);
        const double attendu = (100 / 3.6) * (100 / 3.6) / (2 * n.saison.muChaussee * p.mu * 9.81);   // μ du sol × μ du pneu
        pire = std::max(pire, std::fabs(neige / attendu - 1));
        if (&c == &CARS[0]) std::printf("  %s : 100-0 sec %.1f m, neige tassée (μ %.2f) %.1f m (théorie %.1f m)\n", c.id, sec, n.saison.muChaussee, neige, attendu);
        VERIFIER(neige > 3.2 * sec && neige < 4.6 * sec);
    }
    std::printf("  écart maxi à la théorie sur neige : %.1f %%\n", pire * 100);
    VERIFIER(pire < 0.25);   // ABS (glissement tenu, pas le pic exact), transfert de charge, traînée : ± 25 %
    {   // virage sur neige : volant à fond à 50 km/h, accélération latérale tenue ≤ μ g (la voiture glisse au lieu de tourner plus)
        using namespace physique;
        const Params& p = params(CARS[0]);
        for (const double mu : {1.0, (double)n.saison.muChaussee}) {
            Etat et; et.vz = -50 / 3.6; Commande cm; cm.braquage = p.braquageMax; Sol s; s.mu = mu; double amax = 0;
            for (int i = 0; i < 120 * 3; i++) { cm.gaz = et.u() < 50 / 3.6 ? 0.3 : 0; pas(p, et, cm, s, 1.0 / 120); if (i > 60) amax = std::max(amax, std::fabs(et.u() * et.r)); }
            std::printf("  virage volant à fond, 50 km/h, μ sol %.2f : %.2f g\n", mu, amax / 9.81);
            VERIFIER(amax / 9.81 <= mu * p.mu * 1.15 + 0.02);
        }
        VERIFIER(braquageMax(p, 20) > 0);
    }

    // ---- conduite du jeu (ancien modèle, g_conduiteMonde faux depuis WFS-258) : la voiture du joueur freine et accélère selon le μ de la
    //      carte ; carte du code : rien ne change ; ordinateurs (Aide::Web) : rien ne change
    {
        DonneesCarte dl; dl.id = "ligne-test"; std::string e1;
        VERIFIER(lireTrace("format 1\nnom Ligne\nroute 50 1900 50 0 0\n", dl, e1) && enregistrer(dl, e1));
        Track t; t.build(mapById("ligne-test"), 1);
        auto freiner = [&](Aide aide) {
            Player p; p.aide = aide; p.reset(CARS[0]); p.dist = p.position = 100 / M; p.speed = (100 / 3.6) / M;
            DriveInput in; in.brake = true;
            const double d0 = p.dist;
            for (int i = 0; i < 120 * 40 && p.speed > 0; i++) p.update(1.0 / 120, in, t, true);
            return (p.dist - d0) * M;
        };
        auto elan = [&]() {   // 0 → 50 km/h plein gaz, temps (s)
            Player p; p.aide = Aide::Desactivee; p.reset(CARS[0]); p.dist = p.position = 100 / M;
            DriveInput in; in.gas = true; int i = 0;
            for (; i < 120 * 60 && p.kmh() < 50; i++) p.update(1.0 / 120, in, t, true);
            return i / 120.0;
        };
        preparerSaison("ville");
        const double sec = freiner(Aide::Desactivee), secWeb = freiner(Aide::Web), t50sec = elan();
        cartes_v2::saisonPartie().muChaussee = n.saison.muChaussee;   // même ligne droite, sol de la carte de Noël
        const double neige = freiner(Aide::Desactivee), neigeWeb = freiner(Aide::Web), t50neige = elan();
        cartes_v2::saisonPartie() = {};
        std::printf("  conduite du jeu, %s : 100-0 sec %.1f m, neige %.1f m ; 0-50 sec %.1f s, neige %.1f s\n", CARS[0].id, sec, neige, t50sec, t50neige);
        VERIFIER(neige > 3.0 * sec && neige < 4.5 * sec);   // ≈ 1 / μ (résistances comprises)
        VERIFIER(neigeWeb == secWeb);                       // ordinateurs : inchangés
        VERIFIER(t50neige > 1.5 * t50sec);                  // antipatinage : l'élan est limité par μ
        oublier();
    }

    // ---- flocons (meteo.h) : nombre, boucle autour de la caméra, chute, toits
    VERIFIER(meteo::nombreFlocons(0, false) == 0 && meteo::nombreFlocons(1, false) == meteo::FLOCONS_MAX && meteo::nombreFlocons(0.6f, true) == 0);
    VERIFIER(meteo::nombreFlocons(0.6f, false) == 1200);
    {
        std::vector<meteo::Flocon> f; meteo::Hasard rnd; const vec3 cam{100, 5, -300};
        meteo::semerFlocons(f, 500, cam, 3.5f, rnd);
        bool dedans = true; for (const auto& k : f) dedans = dedans && std::fabs(k.p.x - cam.x) <= meteo::FLOCONS_DEMI && k.p.y >= cam.y + meteo::FLOCONS_BAS - 1e-3f && k.r >= 0.0035f * 0.3f - 1e-6f && k.r <= 0.0035f * 0.7f + 1e-6f;
        VERIFIER(dedans);
        const float y0 = f[0].p.y;
        meteo::avancerFlocons(f, 0.1f, 1.2f, 0, cam);
        VERIFIER(std::fabs((y0 - f[0].p.y) - 0.12f) < 1e-3f || f[0].p.y > y0);   // chute de 1,2 m/s (ou retour en haut de la boîte)
        const vec3 loin{cam.x, cam.y, cam.z - 200};   // la caméra a roulé de 200 m : tous les flocons reviennent autour d'elle
        meteo::avancerFlocons(f, 1.0f / 60, 1.2f, 0, loin);
        bool suivis = true; for (const auto& k : f) suivis = suivis && std::fabs(k.p.z - loin.z) <= meteo::FLOCONS_DEMI + 1e-3f;
        VERIFIER(suivis);
        const std::vector<std::pair<double, double>> toits = {{1056, 1188}};
        VERIFIER(meteo::sousToit(1100, toits) && !meteo::sousToit(1000, toits) && !meteo::sousToit(1200, toits) && !meteo::sousToit(1056.2, toits));
    }

    if (g_echecs) { std::printf("test_cartes_v2_saison : %d échec(s)\n", g_echecs); return 1; }
    std::printf("test_cartes_v2_saison : OK\n");
    return 0;
}
