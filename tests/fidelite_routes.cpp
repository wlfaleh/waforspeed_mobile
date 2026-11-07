// WFS-205 (RG-01, RG-02, RG-04 ; M-01, M-02) : bibliothèque des revêtements de route mesurée contre le réel.
//   - chaque revêtement : 3 cartes KTX2 (ASTC pour le téléphone, RGBA8 de repli) présentes et lisibles ;
//   - M-01 : albédo moyen (luminance Rec. 709, sRGB décodé en linéaire) de la carte de couleur dans la plage du réel ;
//   - M-02 : rugosité sèche moyenne (canal G de l'ORM) dans la plage ; jamais métallique (canal B nul) ;
//   - tailles (décision du PO 2026-11-07 01:40:18) : ASTC couleur et normale ≤ 1024, ORM ≤ 512, RGBA8 ≤ 512 ; ≤ 25 Mo d'ASTC en tout ;
//   - qualité : 1K en Haute / Ultra, 512 en Moyenne, 256 en Basse (niveaux sautés) ;
//   - repli procédural de route.glsl (RG-02 appliquée aux constantes) : albédo moyen dans la plage de l'enrobé usé.
// Écrit le rapport (une ligne par mesure, indice de fidélité = part des mesures dans leur plage).
// usage : test_fidelite_routes [dossier du dépôt] [fichier du rapport]
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>
#include "../src/render/ktx2.h"
#include "../src/render/bench_realisme.h"   // lecteur JSON du référentiel (reel::lireJson)
#include "../src/render/route_mesures.h"
#include "../src/render/route_revetements.h"
#include <cstring>
#include <cstdlib>
using namespace wfs;

#ifndef WFS_SOURCE_DIR
#define WFS_SOURCE_DIR "."
#endif

static int echecs = 0, mesures = 0, conformes = 0;
static std::ostringstream rapport;
static void cas(const std::string& quoi, bool ok) {
    std::printf("routes : %-92s → %s\n", quoi.c_str(), ok ? "OK" : "ÉCHEC");
    if (!ok) echecs++;
}
static void mesure(const std::string& id, const char* m, double v, double mn, double mx) {
    const bool ok = v >= mn - 1e-9 && v <= mx + 1e-9;
    mesures++; if (ok) conformes++;
    char l[256]; std::snprintf(l, sizeof l, "%-16s | %s | %.4f | plage %.2f-%.2f | %s\n", id.c_str(), m, v, mn, mx, ok ? "dans la plage" : "HORS PLAGE");
    rapport << l;
    cas(id + " : " + m + " " + std::to_string(v).substr(0, 6), ok);
}

static bool charger(const std::string& chemin, std::vector<uint8_t>& o) {
    std::ifstream f(chemin, std::ios::binary);
    if (!f) return false;
    o.assign(std::istreambuf_iterator<char>(f), {});
    return true;
}

// --- copie C++ du repli procédural de shaders/route.glsl (hash, noise, fbm, constantes de l'asphalte) pour RG-02 / RG-04
static double fract(double x) { return x - std::floor(x); }
static double hashG(double x, double y) { return fract(std::sin(x * 127.1 + y * 311.7) * 43758.5453); }
static double noiseG(double x, double y) {
    const double ix = std::floor(x), iy = std::floor(y), fx = x - ix, fy = y - iy;
    const double ux = fx * fx * (3 - 2 * fx), uy = fy * fy * (3 - 2 * fy);
    const double a = hashG(ix, iy), b = hashG(ix + 1, iy), c = hashG(ix, iy + 1), d = hashG(ix + 1, iy + 1);
    return (a + (b - a) * ux) * (1 - uy) + (c + (d - c) * ux) * uy;
}
static double fbmG(double x, double y) { double s = 0, a = 0.5; for (int i = 0; i < 4; i++) { s += a * noiseG(x, y); x *= 2.03; y *= 2.03; a *= 0.5; } return s; }

int main(int argc, char** argv) {
    const std::string depot = argc > 1 ? argv[1] : WFS_SOURCE_DIR;
    const std::string sortie = argc > 2 ? argv[2] : "";
    double astcTotal = 0;
    for (int i = 0; i < route::NB_REVETEMENTS; i++) {
        const route::InfoRevetement& in = route::info((route::Revetement)i);
        const std::string base = depot + "/assets/textures/route/" + in.id + "/route_" + in.id + "-";
        bool present = true;
        for (const char* carte : {"couleur", "normale", "orm"}) {
            for (const char* v : {"astc", "rgba8"}) {
                std::vector<uint8_t> o; ktx2::Fichier f; std::string err;
                const std::string chemin = base + carte + "." + v + ".ktx2";
                if (!charger(chemin, o) || !ktx2::lire(o.data(), o.size(), f, err)) { cas(chemin + " lisible", false); present = false; continue; }
                const bool orm = std::string(carte) == "orm";
                const uint32_t maxi = std::string(v) == "astc" ? (orm ? 512u : 1024u) : (orm ? 256u : 512u);
                cas(std::string(in.id) + "-" + carte + "." + v + " : " + std::to_string(f.w) + " px ≤ " + std::to_string(maxi), f.w <= maxi && f.w == f.h);
                if (std::string(v) == "astc") astcTotal += (double)o.size();
            }
        }
        if (!present) std::printf("routes : %s incomplet (repli procédural en jeu)\n", in.id);
    }
    // M-01, M-02 : mesures communes au banc « realisme » (WFS-206, RG-05 : src/render/route_mesures.cpp)
    for (const route::MesureRevetement& m : route::mesurerBibliotheque(depot)) {
        mesure(m.id, m.grandeur.c_str(), m.valeur, m.min, m.max);
        if (m.grandeur.rfind("M-02", 0) == 0) cas(m.id + " : jamais métallique (canal B de l'ORM nul)", !m.metal);
    }
    std::printf("routes : ASTC de la bibliothèque : %.1f Mo (téléphone, dans l'APK)\n", astcTotal / 1048576.0);
    cas("ASTC de la bibliothèque ≤ 25 Mo (décision du PO, budget de l'APK ≤ 200 Mo)", astcTotal <= 25.0 * 1048576.0);
    // qualité : côté maximal chargé et niveaux sautés
    cas("qualité : 1K Ultra / Haute, 512 Moyenne, 256 Basse ; ORM moitié",
        route::coteMax(3, false) == 1024 && route::coteMax(2, false) == 1024 && route::coteMax(1, false) == 512 && route::coteMax(0, false) == 256
        && route::coteMax(2, true) == 512);
    cas("niveaux sautés : 1024 → 256 = 2 ; 512 → 1024 = 0 ; jamais sous 4 texels",
        route::niveauxSautes(1024, 256) == 2 && route::niveauxSautes(512, 1024) == 0 && route::niveauxSautes(8, 1) == 1);
    // repli procédural (RG-02, RG-04) : albédo moyen de l'asphalte sans peinture, sur 400 m × 20 m (pas de 0,25 m)
    {
        double s = 0; int n = 0;
        for (double y = 0; y < 400; y += 0.25)
            for (double x = -10; x < 10; x += 0.25) {
                const double grain = fbmG(x * 3.1, y * 3.1) * 0.6 + fbmG(x * 23.0, y * 23.0) * 0.4;
                double a = 0.085 * 0.2126 + 0.088 * 0.7152 + 0.095 * 0.0722, b = 0.19 * 0.2126 + 0.19 * 0.7152 + 0.198 * 0.0722;
                const double v = (a + (b - a) * grain) * (0.85 + 0.3 * fbmG(x * 0.08, y * 0.05));
                s += v; n++;
            }
        const route::InfoRevetement& use = route::info(route::Revetement::EnrobeUse);
        mesure("repli-procedural", "M-01 albédo moyen (route.glsl)", s / n, use.albedoMin, use.albedoMax);
    }
    // ---------------------------------------------------------------- WFS-206
    {   // RG-02 : revêtement propre à chaque carte ; carte et mode de lecture transmis au shader (pc.params.z = mode + 4 × carte)
        using route::Carte; using route::Revetement;
        cas("RG-02 : Ville et Corniche enrobé usé, Campagne enrobé grenu, Grand Prix béton bitumineux de course",
            route::chaussee(route::carteDe("ville")) == Revetement::EnrobeUse && route::chaussee(route::carteDe("plage")) == Revetement::EnrobeUse
            && route::chaussee(route::carteDe("campagne")) == Revetement::EnrobeCampagne && route::chaussee(route::carteDe("circuit")) == Revetement::BetonCircuit
            && route::carteDe("plage") == Carte::Corniche && route::carteDe(nullptr) == Carte::Ville);
        bool code = true;
        for (int q = 0; q < 4; q++) for (int c = 0; c < 4; c++) {
            const float z = route::codeMateriau(q, (Carte)c), mode = std::fmod(z, 4.f);
            code = code && (int)std::floor(z / 4.f + 0.01f) == c && mode == (q >= 2 ? 2.f : q == 1 ? 1.f : 0.f);
        }
        cas("code du matériau : carte et mode relus comme route.glsl (modeLecture, carteRoute)", code);
    }
    {   // M-03, M-04 : constantes du shader = constantes C++ ; écartement = voie moyenne des voitures (voitures.json) ± 5 %
        std::vector<uint8_t> g; charger(depot + "/shaders/route.glsl", g);
        const std::string glsl(g.begin(), g.end());
        auto constante = [&](const char* nom) {
            const size_t i = glsl.find(std::string("const float ") + nom + " = ");
            return i == std::string::npos ? -1.0 : std::atof(glsl.c_str() + i + std::strlen("const float ") + std::strlen(nom) + 3);
        };
        cas("route.glsl : BANDE_ROULEMENT_M, ECARTEMENT_ROUES_M, BORDURE_LARGEUR_M identiques au C++, DEMI_CHAUSSEE_M = RW (7,5)",
            std::fabs(constante("BANDE_ROULEMENT_M") - route::BANDE_ROULEMENT_M) < 1e-4 && std::fabs(constante("ECARTEMENT_ROUES_M") - route::ECARTEMENT_ROUES_M) < 1e-4
            && std::fabs(constante("BORDURE_LARGEUR_M") - route::BORDURE_LARGEUR_M) < 1e-4 && std::fabs(constante("DEMI_CHAUSSEE_M") - 7.5) < 1e-4);
        double somme = 0; int n = 0;   // voies avant et arrière relevées (mesures voiture.*.voie_av / voie_ar de voitures.json)
        wfs::reel::Json vj; std::string err;
        if (wfs::reel::lireJson(depot + "/references/reel/voitures.json", vj, err))
            if (const wfs::reel::Json* ms = vj.get("mesures"))
                for (const wfs::reel::Json& m : ms->a) {
                    const std::string id = m.texte("id");
                    if (id.find(".voie_a") == std::string::npos) continue;
                    const double v = m.nombre("valeur", 0);
                    if (v > 500) { somme += v; n++; }
                }
        const double voie = n ? somme / n / 1000.0 : 0;
        char t[160]; std::snprintf(t, sizeof t, "M-03 : écartement des bandes %.3f m = voie moyenne %.3f m (%d voies) ± 5 %%", route::ECARTEMENT_ROUES_M, voie, n);
        mesure("bandes-roulement", "M-03 écartement / voie", route::ECARTEMENT_ROUES_M / std::max(voie, 1e-6), 0.95, 1.05);
        cas(t, n >= 10 && std::fabs(route::ECARTEMENT_ROUES_M / voie - 1) <= 0.05);
        mesure("bandes-roulement", "M-03 largeur (m)", route::BANDE_ROULEMENT_M, 0.8, 1.0);
        // M-04 : bordure : 12-15 cm ± 1 % (CURB_H de la chaussée de Game et de la Ville)
        for (const char* f : {"/src/game/game.cpp", "/src/world/ville.cpp"}) {
            std::vector<uint8_t> b; charger(depot + f, b);
            const std::string src(b.begin(), b.end());
            const size_t i = src.find("CURB_H = ");
            const double h = i == std::string::npos ? 0 : std::atof(src.c_str() + i + 9);
            mesure(std::string("bordure") + (std::strstr(f, "ville") ? "-ville" : "-jeu"), "M-04 hauteur (m)", h, 0.12 * 0.99, 0.15 * 1.01);
        }
        mesure("bordure-granit", "M-04 hauteur du shader (m)", route::BORDURE_H_M, 0.12 * 0.99, 0.15 * 1.01);
    }
    const double indice = mesures ? 100.0 * conformes / mesures : 0;
    std::printf("routes : indice de fidélité des revêtements : %.1f %% (%d / %d mesures dans leur plage)\n", indice, conformes, mesures);
    rapport << "indice de fidélité des revêtements : " << indice << " % (" << conformes << " / " << mesures << ")\n";
    cas("indice de fidélité ≥ 99 %", indice >= 99.0);
    if (!sortie.empty()) { std::ofstream f(sortie); f << rapport.str(); }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("routes : tout est OK\n");
    return 0;
}
