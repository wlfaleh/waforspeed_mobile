// WaForSpeed — WFS-324 : banc « retroviseurs » (voir retroviseurs_banc.h).
#include "retroviseurs_banc.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include "../core/ecran.h"
#include "../core/log.h"
#include "../core/platform.h"
#include "../game/camera_vue.h"
#include "../game/cars.h"
#include "../game/game.h"
#include "../game/player.h"
#include "../game/ressenti.h"
#include "../game/track.h"
#include "../game/traffic_system.h"
#include "../ui/ecrans/reglages.h"
#include "../../third_party/stb/stb_image.h"
#include "../../third_party/stb/stb_image_write.h"
#include "eclairage_physique.h"
#include "environnement.h"
#include "model_loader.h"
#include "passes.h"
#include "qualite_auto.h"
#include "renderer.h"
#include "retroviseurs.h"

namespace wfs {

namespace {
struct Img {
    int w = 0, h = 0; unsigned char* p = nullptr;
    explicit Img(const std::string& f) { int c; p = stbi_load(f.c_str(), &w, &h, &c, 3); }
    ~Img() { stbi_image_free(p); }
};
const char* NOMS_GLACES[retro::NB_GLACES] = {"interieur", "gauche", "droite"};

// zoom ×3 du rectangle (marge de 25 %) d'une glace dans l'image `f`, écrit dans `sortie` (lissage bilinéaire)
bool zoomer(const std::string& f, const std::string& sortie, float x0, float y0, float x1, float y1) {
    Img a(f);
    if (!a.p) return false;
    const float mx = (x1 - x0) * 0.25f, my = (y1 - y0) * 0.25f;
    const int X0 = std::max(0, (int)(x0 - mx)), Y0 = std::max(0, (int)(y0 - my)), X1 = std::min(a.w, (int)(x1 + mx)), Y1 = std::min(a.h, (int)(y1 + my));
    if (X1 - X0 < 4 || Y1 - Y0 < 4) return false;
    const int K = 3, W = (X1 - X0) * K, H = (Y1 - Y0) * K;
    std::vector<unsigned char> o((size_t)W * H * 3);
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        const float sx = X0 + (x + 0.5f) / K - 0.5f, sy = Y0 + (y + 0.5f) / K - 0.5f;
        const int ix = std::clamp((int)std::floor(sx), 0, a.w - 2), iy = std::clamp((int)std::floor(sy), 0, a.h - 2);
        const float fx = std::clamp(sx - ix, 0.f, 1.f), fy = std::clamp(sy - iy, 0.f, 1.f);
        for (int c = 0; c < 3; c++) {
            auto px = [&](int u, int v) { return (float)a.p[((size_t)v * a.w + u) * 3 + c]; };
            const float v = (px(ix, iy) * (1 - fx) + px(ix + 1, iy) * fx) * (1 - fy) + (px(ix, iy + 1) * (1 - fx) + px(ix + 1, iy + 1) * fx) * fy;
            o[((size_t)y * W + x) * 3 + c] = (unsigned char)std::lround(v);
        }
    }
    return stbi_write_png(sortie.c_str(), W, H, 3, o.data(), W * 3) != 0;
}
// pixels différents (> 12 niveaux) entre deux images dans un rectangle
long pixelsDifferents(const Img& a, const Img& b, float x0, float y0, float x1, float y1) {
    if (!a.p || !b.p || a.w != b.w || a.h != b.h) return -1;
    long n = 0;
    for (int y = std::max(0, (int)y0); y < std::min(a.h, (int)y1); y++) for (int x = std::max(0, (int)x0); x < std::min(a.w, (int)x1); x++) {
        const unsigned char *p = a.p + ((size_t)y * a.w + x) * 3, *q = b.p + ((size_t)y * a.w + x) * 3;
        n += std::max({std::abs(p[0] - q[0]), std::abs(p[1] - q[1]), std::abs(p[2] - q[2])}) > 12;
    }
    return n;
}
std::string virgule(double v, int d = 1) { char b[32]; std::snprintf(b, sizeof b, "%.*f", d, v); for (char* c = b; *c; c++) if (*c == '.') *c = ','; return b; }
} // namespace

int benchRetroviseurs(Game& g, Renderer& r, const std::string& out, std::vector<std::string> voitures) {
    if (voitures.empty()) voitures = {"golf"};
    const VueCamera vueAvant = vueCamera();
    const bool reduireAvant = reglagesRessenti().reduireMouvements;
    reglagesRessenti().reduireMouvements = true;   // tête immobile : mêmes images d'une prise à l'autre
    int ecarts = 0; double t = 1500;
    std::ostringstream mes;
    auto rouler = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 0) g.render(t); } };
    const float W = (float)r.width(), H = (float)r.height();
    const bool rapide = std::getenv("WFS_RETRO_RAPIDE") != nullptr;   // essai local : jour, voiture derrière, sans mesure du coût
    // rétroviseurs mesurés sur les modèles 3D de toutes les voitures (mesurerRetroviseurs, au chargement)
    mes << "== rétroviseurs des modèles 3D (repère du modèle, m) ==\n";
    for (const CarDef& c : CARS) {
        Model mo;
        if (!loadCarModel(r, c.file, mo)) { mes << "  " << c.id << " : modèle illisible\n"; continue; }
        mes << "  " << c.id;
        for (int k = 0; k < 2; k++) {
            if (!mo.retroMesure[k]) { mes << (k ? " ; droit : non trouvé (place estimée)" : " : gauche : non trouvé (place estimée)"); continue; }
            const vec3 a = mo.retroMin[k], b = mo.retroMax[k];
            char t[160]; std::snprintf(t, sizeof t, "%s : x %.2f à %.2f, y %.2f à %.2f, z %.2f à %.2f", k ? " ; droit" : " : gauche", a.x, b.x, a.y, b.y, a.z, b.z);
            mes << t;
        }
        mes << "\n";
        r.destroy(mo);
    }
    for (const std::string& car : voitures) for (const bool nuit : {false, true}) {
        if (rapide && nuit) continue;
        setenv("WFS_MOMENT", nuit ? "nuit" : "jour", 1);
        if (!g.init(r, car.c_str(), "ville")) { WFS_ERR("retroviseurs : chargement impossible (%s)", car.c_str()); ecarts++; continue; }
        choisirVue(VueCamera::Cockpit); g.camMode = 0; g.setAutopilot(true); g.allerA(Ecran::Course);
        rouler(60 * 4);   // décompte et départ
        g.setAutopilot(false);
        // scènes : voiture 10 m derrière dans la même voie (glace intérieure), puis 9 m derrière, 3,3 m à gauche (glace gauche)
        struct Scene { const char* nom; double devant, gauche; };
        for (const Scene sc : {Scene{"derriere", -10.0, 0.0}, Scene{"gauche", -9.0, 3.3}}) {
            if (rapide && sc.gauche > 0) continue;
            trafficBenchPlace(sc.devant, sc.gauche / RW, 0, true);   // lat > 0 : voiture à gauche du joueur
            for (int i = 0; i < 4; i++) { g.update(1.0 / 60); t += 1.0 / 60; }
            const int k = trafficBenchCar();
            Traffic* tr = activeTrafficMut();
            if (k < 0 || !tr) { WFS_ERR("retroviseurs : aucune voiture placée (%s)", car.c_str()); ecarts++; continue; }
            TrafficCar& c = tr->cars[(size_t)k];
            const std::string base = out + "/retro-" + car + "-" + sc.nom + (nuit ? "-nuit" : "");
            // lumière figée pendant les prises (comme les bancs du cockpit) ; 8 images : chaque glace redessinée deux fois
            for (int i = 0; i < 12 && environnement::enCours(); i++) g.render(t);
            eclairage::suspendreCaptures(true);
            for (int i = 0; i < 8; i++) g.render(t);
            r.capturePng(base + ".png");
            c.cache = true;
            for (int i = 0; i < 8; i++) g.render(t);
            r.capturePng(base + "-sans.png");
            c.cache = false;
            for (int i = 0; i < 8; i++) g.render(t);
            eclairage::suspendreCaptures(false);
            Img A(base + ".png"), B(base + "-sans.png");
            const Player& pl = g.player();
            mes << "\n" << car << " " << sc.nom << (nuit ? " nuit" : " jour") << " : voiture " << tr->cars[(size_t)k].z - pl.position << " (unités) derrière, "
                << virgule((c.offset - pl.playerX) * RW, 2) << " m en travers\n";
            long vues[retro::NB_GLACES] = {0, 0, 0};
            for (int gl = 0; gl < retro::NB_GLACES; gl++) {
                float x0, y0, x1, y1;
                if (!retro::rectEcran(gl, W, H, x0, y0, x1, y1)) { mes << "  glace " << NOMS_GLACES[gl] << " : hors de l'image\n"; continue; }
                vues[gl] = pixelsDifferents(A, B, x0, y0, x1, y1);
                const retro::Comptes cg = retro::comptesGlace(gl);
                mes << "  glace " << NOMS_GLACES[gl] << " : à l'écran " << (int)(x1 - x0) << " × " << (int)(y1 - y0) << " px, pixels de la voiture dans le rectangle : " << vues[gl]
                    << " ; reflet : " << cg.appels << " appels, " << cg.triangles / 1000 << " k triangles (voiture du joueur " << cg.trianglesJoueur / 1000 << " k)\n";
                zoomer(base + ".png", base + "-zoom-" + NOMS_GLACES[gl] + ".png", x0, y0, x1, y1);
            }
            std::remove((base + "-sans.png").c_str());
            if (std::getenv("WFS_GARDER")) for (int gl = 0; gl < retro::NB_GLACES; gl++) {   // essai local : images des reflets (Reinhard, γ 2,2)
                std::vector<float> f; uint32_t w = 0, h = 0;
                if (!retro::lireImage(r, gl, f, w, h)) continue;
                std::vector<unsigned char> o((size_t)w * h * 3);
                for (size_t i = 0; i < (size_t)w * h; i++) for (int c = 0; c < 3; c++) {
                    const float v = f[i * 4 + c] * 0.6f;
                    o[i * 3 + c] = (unsigned char)std::lround(255 * std::pow(std::clamp(v / (1 + v), 0.f, 1.f), 1 / 2.2f));
                }
                stbi_write_png((base + "-texture-" + NOMS_GLACES[gl] + ".png").c_str(), (int)w, (int)h, 3, o.data(), (int)w * 3);
            }
            WFS_LOG("retroviseurs %s %s %s : voiture vue dans la glace intérieure %ld px, gauche %ld px, droite %ld px", car.c_str(), sc.nom,
                    nuit ? "nuit" : "jour", vues[0], vues[1], vues[2]);
            // critère du banc : la voiture derrière, dans la même voie, se voit dans la glace intérieure ; dans la voie de gauche (≈ 25° derrière
            // à gauche, hors du champ de la glace intérieure, comme en vrai), dans la glace gauche
            if ((sc.gauche == 0 && vues[0] <= 0) || (sc.gauche > 0 && vues[1] <= 0)) {
                WFS_ERR("retroviseurs %s %s : voiture derrière invisible dans une glace (intérieure %ld, gauche %ld)", car.c_str(), sc.nom, vues[0], vues[1]);
                ecarts++;
            }
        }
        g.shutdown();
    }
    // ---- RG-03 : coût en Basse et en Haute (Golf, Ville, jour, même instant figé) : 120 images avec, puis sans rétroviseurs
    if (!rapide) {
        setenv("WFS_MOMENT", "jour", 1);
        ecrans::Reglages& R = ecrans::reglages();
        const int qualiteAvant = R.qualite;
        mes << "\n== coût (RG-03), Golf, Ville, jour, vue Cockpit, image figée, 4 × 40 images avec et sans en alternance (médianes) : ms par image (mur, rendu logiciel lavapipe) ==\n";
        for (const int n : {(int)qualite::BASSE, (int)qualite::HAUTE}) {
            qualite::ultraChoisi = false; R.qualite = n; ecrans::appliquerQualiteNiveau(r);
            if (!g.init(r, voitures[0].c_str(), "ville")) { ecarts++; continue; }
            choisirVue(VueCamera::Cockpit); g.camMode = 0; g.setAutopilot(true); g.allerA(Ecran::Course);
            rouler(60 * 4);
            trafficBenchPlace(-10, 0, 0, true);
            for (int i = 0; i < 4; i++) { g.update(1.0 / 60); t += 1.0 / 60; }
            eclairage::suspendreCaptures(true);
            // avec / sans en alternance, 4 fois 40 images : médiane de chaque côté (le rendu logiciel varie de ± 5 % d'une série à l'autre)
            std::vector<double> serie[2]; double passe = 0; uint32_t appels[2] = {0, 0};
            for (int tour = 0; tour < 8; tour++) {
                const int sans = tour & 1;
                retro::desactiver(sans == 1);
                for (int i = 0; i < 6; i++) g.render(t);
                passes::remettreAZero();
                const double t0 = nowSeconds();
                uint64_t a = 0;
                for (int i = 0; i < 40; i++) { g.render(t); a += r.appelsDessin(); }
                serie[sans].push_back((nowSeconds() - t0) / 40 * 1000); appels[sans] = (uint32_t)(a / 40);
                if (!sans) for (const passes::Cout& x : passes::coutsCumules()) if (x.nom.rfind("rétroviseurs", 0) == 0) passe = x.ms;
            }
            double ms[2];
            for (int k = 0; k < 2; k++) { std::sort(serie[k].begin(), serie[k].end()); ms[k] = (serie[k][1] + serie[k][2]) * 0.5; }
            retro::desactiver(false);
            if (n == qualite::BASSE) {   // capture 3 du ticket : affichage « Rendu / Images » des Réglages, vue Cockpit, Basse, sans puis avec
                // pause appliquée (images rendues) avant d'ouvrir le panneau, qui reste ouvert pour les deux images (1re version : le panneau
                // s'ouvrait avant la pause et l'image « sans » montrait l'écran Pause, refus du lead dev, PR #341)
                g.allerA(Ecran::Pause);
                for (int i = 0; i < 10; i++) g.render(t);
                ecrans::ouvrirReglagesBanc(true);
                for (int sans = 1; sans >= 0; sans--) {
                    retro::desactiver(sans == 1);
                    for (int i = 0; i < 30; i++) g.render(t);
                    r.capturePng(out + "/retro-reglages-rendu-basse-" + (sans ? "sans" : "avec") + ".png");
                    WFS_LOG("capture retro-reglages-rendu-basse-%s : écran %s, %s", sans ? "sans" : "avec", nomEcran(g.ecran()), qualite::ligneRendu().c_str());
                }
                ecrans::ouvrirReglagesBanc(false);
                retro::desactiver(false);
                g.allerA(Ecran::Course);
            }
            eclairage::suspendreCaptures(false);
            const double part = ms[1] > 0 ? (ms[0] - ms[1]) / ms[1] : 0;
            // téléphone à 21 img/s (WFS-317) : même part du temps de l'image → images/s perdues (estimation, le banc n'est pas le téléphone)
            const double perte = 21.0 - 21.0 / (1.0 + std::max(0.0, part));
            mes << "  " << (n == qualite::BASSE ? "Basse" : "Haute") << " : avec " << virgule(ms[0]) << " ms (" << appels[0] << " appels), sans " << virgule(ms[1]) << " ms ("
                << appels[1] << " appels), passe « rétroviseurs » " << virgule(passe, 2) << " ms GPU ; écart " << virgule(part * 100) << " % → à 21 img/s : "
                << virgule(perte, 2) << " img/s perdues (estimé, ≤ 2 visé)\n";
            WFS_LOG("retroviseurs (RG-03) : %s : avec %.1f ms, sans %.1f ms, passe %.2f ms GPU, écart %.1f %% → %.2f img/s perdues à 21 img/s (estimé)",
                    n == qualite::BASSE ? "Basse" : "Haute", ms[0], ms[1], passe, part * 100, perte);
            g.shutdown();
        }
        R.qualite = qualiteAvant; ecrans::appliquerQualiteNiveau(r);
    }
    unsetenv("WFS_MOMENT");
    choisirVue(vueAvant);
    reglagesRessenti().reduireMouvements = reduireAvant;
    if (FILE* f = std::fopen((out + "/retroviseurs-mesures.txt").c_str(), "w")) { std::fputs(mes.str().c_str(), f); std::fclose(f); }
    WFS_LOG("retroviseurs (WFS-324) : %zu voiture(s), %d écart(s)%s", voitures.size(), ecarts, ecarts ? " ÉCHEC" : " OK");
    return ecarts;
}

} // namespace wfs
