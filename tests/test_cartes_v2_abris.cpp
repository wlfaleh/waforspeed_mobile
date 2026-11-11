// WFS-392 : tunnels des cartes de données — (RG-01) chaussée sèche sous CHAQUE tunnel : abris rangés dans les cases du matériau de la
// route (route::coderAbris) et relus comme route.glsl (route::abriRoute) : sec au milieu de chaque tunnel, enneigé dehors, fondu aux
// bouches, tours suivants, long tunnel sans bande de neige, tunnels en trop comptés ; code du matériau inchangé sans abri ;
// (RG-02, RG-03) éclairage lu du bloc decor (tunnel_eclairage), valeurs par défaut, refus des valeurs hors limites, place des
// luminaires (pas courant, pas du seuil, tout dans le tunnel) ; carte d'essai de Noël : 2 tunnels.
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include "../src/game/track.h"
#include "../src/render/route_revetements.h"
#include "../src/world/cartes_v2/carte_donnees.h"
using namespace wfs;
using namespace wfs::cartes_v2;

static int g_echecs = 0;
#define VERIFIER(c) do { if (!(c)) { std::printf("ÉCHEC ligne %d : %s\n", __LINE__, #c); g_echecs++; } } while (0)

static std::string lire(const std::string& f) { std::ifstream in(f, std::ios::binary); return std::string(std::istreambuf_iterator<char>(in), {}); }
static const std::string TRACE_MIN = "format 1\nnom Essai\nroute 50 200 50 0 0\n";   // 300 segments
static bool decor(const std::string& t, DonneesCarte& d, std::string& e) { d = {}; d.id = "x"; std::string e0; lireTrace(TRACE_MIN, d, e0); return lireDecor(t, d, e); }
static bool entier24(float c) { return c >= 0 && c < 16777216.f && c == std::floor(c); }

int main() {
    std::string e;
    const std::string depot = WFS_SOURCE_DIR;

    // ---- carte d'essai de Noël : 2 tunnels lus du tracé
    DonneesCarte n; n.id = "noel-test";
    VERIFIER(lireTrace(lire(depot + "/paquets/sources/noel-test/trace.txt"), n, e) && lireDecor(lire(depot + "/paquets/sources/noel-test/decor.txt"), n, e));
    std::vector<std::pair<int64_t, int64_t>> t;
    for (const trace::Section& s : n.sections) if (s.type == trace::Type::Tunnel) t.push_back({s.debut, s.fin});
    VERIFIER(t.size() == 2);

    // ---- RG-01 : sec sous chaque tunnel, enneigé dehors, fondu de 9 m à chaque bouche (−1 à +8 m), même chose aux tours suivants
    const route::Abris a = route::coderAbris(t, n.segments());
    VERIFIER(a.tunnels == 2 && a.perdus == 0 && a.tourSeg == n.segments());
    for (const float c : a.code) VERIFIER(entier24(c));
    const double tour = n.segments() * SEG_M;
    for (int tr = 0; tr < 3; tr++) for (const auto& [d0, d1] : t) {
        const double y0 = d0 * SEG_M + tr * tour, y1 = d1 * SEG_M + tr * tour;
        VERIFIER(route::abriRoute(a, (y0 + y1) / 2) == 0.f);                                  // milieu : sec
        VERIFIER(route::abriRoute(a, y0 + 8.5) == 0.f && route::abriRoute(a, y1 - 8.5) == 0.f);
        VERIFIER(route::abriRoute(a, y0 - 1.5) == 1.f && route::abriRoute(a, y1 + 1.5) == 1.f);   // dehors : neige entière
        const float f3 = route::abriRoute(a, y0 + 3), f6 = route::abriRoute(a, y0 + 6);
        VERIFIER(f3 > 0 && f3 < 1 && f6 > 0 && f6 < f3);                                       // neige amenée par les roues, de moins en moins
    }
    VERIFIER(route::abriRoute(a, 30) == 1.f && route::abriRoute(a, (t[0].second + t[1].first) / 2.0 * SEG_M) == 1.f);   // entre les deux : neige
    VERIFIER(route::abriRoute(route::Abris{}, 100) == 1.f);                                    // aucun abri : neige partout

    // ---- constante du shader = constante C++ = SEG_M ; le shader relit le tour dans le code du matériau
    VERIFIER(route::SEG_ROUTE_M == SEG_M);
    { const std::string glsl = lire(depot + "/shaders/route.glsl");
      const size_t i = glsl.find("const float SEG_ROUTE_M = ");
      VERIFIER(i != std::string::npos && std::fabs(std::atof(glsl.c_str() + i + 26) - SEG_M) < 1e-9); }
    for (int q = 0; q <= 3; q++) for (int c = 0; c < 4; c++) for (int k = 0; k <= route::NEIGE_PAS; k++) for (const int ts : {0, 300, 1220, route::TOUR_SEG_MAX}) {
        const float z = route::codeMateriau(q, (route::Carte)c, (float)k / route::NEIGE_PAS, ts);
        const float mode = std::fmod(z, 4.f), carte = std::fmod(std::floor(z / 4.f + 0.01f), 4.f), neige = std::fmod(std::floor(z / 16.f + 0.01f), 16.f) / 8.f;
        VERIFIER(entier24(z) && mode == (q >= 2 ? 2.f : q == 1 ? 1.f : 0.f) && (int)carte == c && std::fabs(neige - (float)k / route::NEIGE_PAS) < 1e-6);
        VERIFIER((int)std::floor(z / 256.f + 0.001f) == ts);
        if (!ts) VERIFIER(z == route::codeMateriau(q, (route::Carte)c, (float)k / route::NEIGE_PAS));   // sans abri : valeur d'avant
    }

    // ---- tunnels qui se touchent : réunis (aucune bande de neige au raccord)
    { const route::Abris b = route::coderAbris({{100, 200}, {200, 260}}, 1000);
      VERIFIER(b.tunnels == 1 && b.perdus == 0 && b.code[1] == 0 && route::abriRoute(b, 200 * SEG_M) == 0.f); }
    // ---- long tunnel (2 500 segments = 3 km) : 3 cases qui se recouvrent, sec d'un bout à l'autre au-delà du fondu
    { const route::Abris b = route::coderAbris({{500, 3000}}, 6000);
      VERIFIER(b.tunnels == 1 && b.perdus == 0 && b.code[2] > 0);
      bool sec = true; for (double y = 500 * SEG_M + 8.5; y < 3000 * SEG_M - 8.5; y += 0.7) sec = sec && route::abriRoute(b, y) == 0.f;
      VERIFIER(sec && route::abriRoute(b, 3000 * SEG_M + 2) == 1.f); }
    // ---- plus de tunnels que de cases, tour trop long : les tunnels en trop sont comptés, les premiers restent secs, jamais de plantage
    { const route::Abris b = route::coderAbris({{10, 50}, {100, 150}, {200, 250}, {300, 350}}, 1000);
      VERIFIER(b.tunnels == 4 && b.perdus == 1 && route::abriRoute(b, 225 * SEG_M) == 0.f && route::abriRoute(b, 325 * SEG_M) == 1.f);
      const route::Abris c = route::coderAbris({{10, 50}}, 70000);
      VERIFIER(c.perdus == 1 && c.tourSeg == 0 && route::abriRoute(c, 30 * SEG_M) == 1.f);
      const route::Abris v = route::coderAbris({}, 1000);
      VERIFIER(v.tunnels == 0 && v.tourSeg == 0); }

    // ---- RG-03 : éclairage des tunnels : valeurs par défaut sûres, lecture, refus
    DonneesCarte d;
    VERIFIER(decor("format 2\nherbe 1\n", d, e));
    VERIFIER(d.eclairageTunnel.type == EclairageTunnel::Sodium && d.eclairageTunnel.pasM == 12 && d.eclairageTunnel.pasSeuilM == 6 && d.eclairageTunnel.seuilM == 60);
    VERIFIER(decor("format 3\ntunnel_eclairage led 18 9 40\n", d, e));
    VERIFIER(d.eclairageTunnel.type == EclairageTunnel::Led && d.eclairageTunnel.pasM == 18 && d.eclairageTunnel.pasSeuilM == 9 && d.eclairageTunnel.seuilM == 40);
    VERIFIER(decor("format 3\nherbe 1\n", d, e) && d.eclairageTunnel.type == EclairageTunnel::Sodium);   // rien de la carte précédente
    VERIFIER(decor("format 3\ntunnel_eclairage aucun 12 6 60\n", d, e) && d.eclairageTunnel.type == EclairageTunnel::Aucun);
    VERIFIER(!decor("format 3\ntunnel_eclairage neon 12 6 60\n", d, e) && e.find("tunnel_eclairage") != std::string::npos);
    VERIFIER(!decor("format 3\ntunnel_eclairage sodium 40 6 60\n", d, e));    // pas trop grand (25 m au plus)
    VERIFIER(!decor("format 3\ntunnel_eclairage sodium 12 14 60\n", d, e));   // seuil plus espacé que la section courante
    VERIFIER(!decor("format 3\ntunnel_eclairage sodium 12 6 900\n", d, e));
    VERIFIER(!decor("format 3\ntunnel_eclairage sodium 12 6\n", d, e));
    VERIFIER(!decor("format 4\nherbe 1\n", d, e) && e.find("plus récent") != std::string::npos);
    VERIFIER(n.eclairageTunnel.type == EclairageTunnel::Sodium);   // carte de Noël : ligne lue du paquet

    // ---- RG-02 : place des luminaires : tous dans le tunnel, écart jamais plus grand que le pas courant, resserré aux bouches
    for (const auto& [d0, d1] : std::vector<std::pair<int64_t, int64_t>>{{880, 990}, {1060, 1150}, {0, 12}, {100, 1100}}) {
        const EclairageTunnel def;
        const std::vector<int64_t> l = luminairesTunnel(def, d0, d1);
        const int64_t pas = std::llround(def.pasM / SEG_M), pasSeuil = std::llround(def.pasSeuilM / SEG_M);
        VERIFIER(!l.empty() && l.front() >= d0 && l.front() - d0 <= pasSeuil && l.back() < d1 && d1 - l.back() <= pas);
        bool ok = true; for (size_t i = 1; i < l.size(); i++) ok = ok && l[i] - l[i - 1] >= pasSeuil && l[i] - l[i - 1] <= pas;
        VERIFIER(ok);
        if (l.size() > 2) VERIFIER(l[1] - l[0] == pasSeuil && l[l.size() - 1] - l[l.size() - 2] == pasSeuil);   // zones de seuil
        if (d1 - d0 > 400) { bool courant = false; for (size_t i = 1; i < l.size(); i++) courant = courant || l[i] - l[i - 1] == pas; VERIFIER(courant); }
    }
    { EclairageTunnel sans; sans.type = EclairageTunnel::Aucun; VERIFIER(luminairesTunnel(sans, 100, 300).empty()); }

    if (g_echecs) { std::printf("test_cartes_v2_abris : %d échec(s)\n", g_echecs); return 1; }
    std::printf("test_cartes_v2_abris : OK\n");
    return 0;
}
