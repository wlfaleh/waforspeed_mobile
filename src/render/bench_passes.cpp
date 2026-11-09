// WaForSpeed — banc du socle des passes (WFS-150) : voir bench_passes.h.
#include "bench_passes.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cstdio>
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "../game/game.h"
#include "../game/track.h"
#include "../platform/manette.h"
#include "../ui/ecrans/reglages.h"
#include "../ui/text.h"
#include "environnement.h"
#include "passes.h"
#include "qualite_auto.h"
#include "renderer.h"
#include "../../third_party/stb/stb_image_write.h"   // implémentation : renderer.cpp

namespace wfs {
namespace {

// caméra du banc : face f de la capture (même point, même direction que la face, 90° de haut) ou vue fixe
struct EtatBanc {
    bool actif = false;
    int face = -1;                 // ≥ 0 : caméra sur la face f (M-01)
    vec3 point{}; bool pointPose = false;
    float exposition = 1;
};
EtatBanc g_b;

// point du repère de la route au joueur : a m devant, l m à droite, h m de haut (comme bench_realisme.cpp)
vec3 pointRoute(Track& t, double s, float a, float l, float h) {
    const TrackPoint q = t.point(s + a / M, l);
    return {(float)q.x, (float)q.y + h, (float)q.z};
}

class PassesBanc : public System {
public:
    const char* name() const override { return "passes-banc"; }
    void ambiance(FrameParams& fp, const GameView& g) override {
        if (!g_b.actif || !g.track || !g.player) return;
        if (!g_b.pointPose) { g_b.point = pointRoute(*g.track, g.player->dist, 6, 0, 1.7f); g_b.pointPose = true; }
        g_b.exposition = fp.exposure;
        const float aspect = fp.proj.at(0, 0) != 0 ? -fp.proj.at(1, 1) / fp.proj.at(0, 0) : 16.f / 9.f;
        if (g_b.face >= 0) {   // même direction que la face (image retournée : mêmes directions, donc même luminance moyenne)
            vec3 a, h; environnement::axesFace(g_b.face, a, h);
            fp.view = lookAt(g_b.point, g_b.point + a, h);
            fp.proj = perspectiveVk(PI / 2, aspect, 0.1f);
        } else {               // vue fixe : le long de la route, 1,7 m de haut, vers 60 m devant
            fp.view = lookAt(pointRoute(*g.track, g.player->dist, -6, 0, 1.7f), pointRoute(*g.track, g.player->dist, 60, 0, 1.f), {0, 1, 0});
            fp.proj = perspectiveVk(60 * PI / 180, aspect, 0.1f);
        }
        const mat4 iv = inverseAffine(fp.view); fp.camPos = {iv.at(0, 3), iv.at(1, 3), iv.at(2, 3)};
    }
};
WFS_SYSTEM(PassesBanc, 975)   // banc (900-999) : après l'ambiance du banc du réalisme (970)

std::string fmt(double v, int d) { char b[48]; std::snprintf(b, sizeof b, "%.*f", d, v); return b; }
double luminance(const float* p) { return 0.2126 * p[0] + 0.7152 * p[1] + 0.0722 * p[2]; }   // Rec. 709 / sRGB linéaire (Y relatif)
// M-01 (WFS-161, accroche autorisée par le PO) : luminance bornée des deux côtés (cubemap et scène). Le ciel vaut ~0,5 ; le disque
// solaire, des milliers : quand une arête (immeuble du kit) le coupe, la part couverte dépend de la résolution (256 / 720 px)
// et domine la moyenne. La borne ne touche que le disque solaire ; tolérance ± 1 % inchangée sur les 6 faces.
constexpr double Y_MAX_M01 = 20;
double luminanceM01(const float* p) { return std::min(luminance(p), Y_MAX_M01); }

// ACES de post.frag (sans vignette ni bloom) puis gamma 1 / 2,2 : affichage de la planche seulement
uint8_t versOctet(float x, float expo) {
    x *= expo;
    const float a = std::clamp((x * (2.51f * x + 0.03f)) / (x * (2.43f * x + 0.59f) + 0.14f), 0.f, 1.f);
    return (uint8_t)std::lround(std::pow(a, 1.f / 2.2f) * 255.f);
}

void scene(Game& g, double t, int pas = 90) {
    for (int i = 0; i < pas; i++) { g.update(1.0 / 60); if (i % 10 == 0) g.render(t); }
}

std::string ligneCouts(const std::vector<passes::Cout>& c) {
    std::string s;
    for (const passes::Cout& x : c) {
        if (!x.mesurable) { s += " | non mesurable"; continue; }
        s += " | " + (x.images ? fmt(x.ms, 3) : std::string("—"));
    }
    return s;
}

} // namespace

int benchPasses(Game& g, Renderer& r, const std::string& out, std::ostream& mes, std::ostream& infos) {
    int ecarts = 0;
    ecrans::Reglages& R = ecrans::reglages();
    const int q0 = R.qualite; const bool u0 = qualite::ultraChoisi;
    const double t = 2000;
    auto niveau = [&](int n) {   // n = qualite::Niveau (ULTRA : Haute + ultraChoisi)
        qualite::ultraChoisi = n == qualite::ULTRA;
        R.qualite = std::min(n, (int)qualite::HAUTE); ecrans::appliquerQualiteNiveau(r);
    };
    const char* NOMS[4] = {"Basse", "Moyenne", "Haute", "Ultra"};
    setenv("WFS_MOMENT", "jour", 1); unsetenv("WFS_DEPART");

    // ---------------- RG-01 / RG-02 / RG-05 : passes enregistrées, passe d'essai et pré-passe, 0 écart d'image
    mes << "\n== passes de rendu enfichables (WFS-150, RG-01) ==\n  ordre fixe : ombres du soleil → reflets → étape ombres → pré-passe → passes d'écran"
        << " → scène → étape scène → transparents → étape transparents → bloom → post-traitement → passe finale\n"
        << "  étape | passe | niveau minimal | demande la pré-passe | cibles\n";
    for (const PasseRendu* p : passes::liste())
        mes << "  " << nomEtape(p->etape()) << " | " << p->nom() << " | " << NOMS[std::clamp(p->qualiteMin(), 0, 3)] << " | "
            << (p->demandePrePasse() ? "oui" : "non") << " | " << p->cibles() << "\n";
    mes << "  horodatages GPU : " << (passes::horodatagesDisponibles() ? "oui (vkCmdWriteTimestamp, 2 par passe et par image)" : "non mesurable (le pilote n'en a pas)") << "\n";
    g_b = EtatBanc{}; g_b.actif = true;
    niveau(qualite::HAUTE);
    if (g.init(r, "golf", "ville")) {
        g.setAutopilot(false); g.camMode = 0; g.allerA(Ecran::Chargement);
        scene(g, t);
        for (int i = 0; i < 3; i++) g.render(t);
        r.capturePng(out + "/passes-sans-essai.png");
        passes::activerEssai(true);
        passes::remettreAZero();
        for (int i = 0; i < 130; i++) g.render(t);
        const std::vector<passes::Cout> c = passes::coutsCumules();
        r.capturePng(out + "/passes-avec-essai.png");
        std::vector<uint8_t> px; uint32_t w = 0, h = 0;
        if (passes::lireEssai(r, px, w, h)) {
            stbi_write_png((out + "/realisme-prepasse-essai.png").c_str(), (int)w, (int)h, 4, px.data(), (int)w * 4);
            size_t objets = 0; for (size_t i = 0; i < px.size() / 4; i++) if (i % w < w / 2 && (px[i * 4] | px[i * 4 + 1] | px[i * 4 + 2])) objets++;
            WFS_LOG("capture realisme-prepasse-essai : %ux%u, %zu pixels d'objets dans la moitié des normales", w, h, objets);
            if (!objets) { WFS_ERR("passes (WFS-150) : pré-passe vide"); ecarts++; }
        } else { WFS_ERR("passes (WFS-150) : image de la passe d'essai illisible"); ecarts++; }
        passes::activerEssai(false);
        mes << "\n== coût GPU de chaque passe, Ville, jour, Haute, passe d'essai active (130 images, scène figée) ==\n  passe | étape | images | ms GPU par image\n";
        for (const passes::Cout& x : c)
            mes << "  " << x.nom << (x.renderer ? " (Renderer)" : "") << " | " << nomEtape(x.etape) << " | " << x.images << " | "
                << (x.mesurable ? (x.images ? fmt(x.ms, 3) : std::string("n'a pas tourné")) : std::string("non mesurable")) << "\n";
        g.shutdown();
        // RG-06 : la passe d'essai et la pré-passe ne changent pas l'image (comparaison octet par octet des deux PNG)
        auto lire = [](const std::string& f) {
            std::vector<char> v; std::FILE* fp = std::fopen(f.c_str(), "rb");
            if (!fp) return v;
            std::fseek(fp, 0, SEEK_END); v.resize((size_t)std::ftell(fp)); std::fseek(fp, 0, SEEK_SET);
            if (std::fread(v.data(), 1, v.size(), fp) != v.size()) v.clear();
            std::fclose(fp);
            return v;
        };
        const std::vector<char> A = lire(out + "/passes-sans-essai.png"), B = lire(out + "/passes-avec-essai.png");
        const bool pareil = !A.empty() && A == B;
        mes << "  image du jeu avec / sans la passe d'essai (et la pré-passe) : " << (pareil ? "identique (0 écart, RG-06)" : "DIFFÉRENTE") << "\n";
        WFS_LOG("passes (WFS-150) : image avec / sans passe d'essai : %s", pareil ? "identique" : "différente");
        if (!pareil) { WFS_ERR("passes (WFS-150) : image du jeu différente avec la passe d'essai"); ecarts++; }
        std::remove((out + "/passes-sans-essai.png").c_str()); std::remove((out + "/passes-avec-essai.png").c_str());
    } else { WFS_ERR("passes (WFS-150) : images avec / sans passe d'essai illisibles"); ecarts++; }

    // ---------------- RG-05 / P-02 : coût des passes du Renderer par niveau (Ville, jour, même vue, 130 images)
    mes << "\n== coût GPU des passes par niveau (Ville, jour, vue fixe, scène figée, 130 images ; aucune passe enfichable active) ==\n  niveau | ms par image (mur)";
    for (const passes::Cout& x : passes::coutsCumules()) mes << " | " << x.nom;
    mes << "\n";
    for (int n = 0; n < 4; n++) {
        niveau(n);
        if (!g.init(r, "golf", "ville")) { WFS_ERR("passes (WFS-150) : niveau %d : chargement de la Ville impossible", n); ecarts++; continue; }
        g.allerA(Ecran::Chargement); scene(g, t);
        passes::remettreAZero();
        const double t0 = nowSeconds();
        for (int i = 0; i < 130; i++) g.render(t);
        const double ms = (nowSeconds() - t0) / 130 * 1000;
        mes << "  " << NOMS[n] << " | " << fmt(ms, 1) << ligneCouts(passes::coutsCumules()) << "\n";
        WFS_LOG("passes (WFS-150) : coût en %s : %.1f ms par image ; %s", NOMS[n], ms, passes::texteCouts(passes::coutsCumules()).c_str());
        g.shutdown();
    }

    // ---------------- RG-03 / M-01 : capture d'environnement (Ultra : 256 px ; puis Basse : 64 px), une face par image
    for (const char* moment : {"jour", "nuit"}) {
        setenv("WFS_MOMENT", moment, 1);
        const bool jour = !std::strcmp(moment, "jour");
        // le reflet de la chaussée mouillée (WFS-16) est la vue miroir de la caméra du joueur : jamais lu par la capture ;
        // M-01 compare donc la scène sans ce reflet (WFS_REFLETS=0, interrupteur de mesure de WFS-142), jour comme nuit
        setenv("WFS_REFLETS", "0", 1);
        niveau(qualite::ULTRA);
        if (!g.init(r, "golf", "ville")) { WFS_ERR("environnement (WFS-150) : M-01 (%s) : chargement de la Ville impossible", moment); ecarts++; continue; }
        g.allerA(Ecran::Chargement); g_b.pointPose = false; g_b.face = 0;
        scene(g, t);
        const int avant = environnement::capturesFaites();
        passes::remettreAZero();
        environnement::demander(g_b.point);
        double lumScene[6] = {};
        for (int f = 0; f < 6; f++) {
            g_b.face = f;
            if (environnement::faceSuivante() != f) { WFS_ERR("environnement : face %d attendue, %d en cours", f, environnement::faceSuivante()); ecarts++; }
            g.render(t);
            std::vector<float> hdr; uint32_t w = 0, h = 0;
            if (!passes::lireHdr(r, hdr, w, h)) { WFS_ERR("environnement (WFS-150) : M-01 (%s) face %d : cible HDR illisible", moment, f); ecarts++; continue; }
            const uint32_t x0 = (w - std::min(w, h)) / 2, cote = std::min(w, h);   // carré central = les 90° × 90° de la face
            double s = 0;
            for (uint32_t y = 0; y < cote; y++) for (uint32_t x = x0; x < x0 + cote; x++) s += luminanceM01(&hdr[((size_t)y * w + x) * 4]);
            lumScene[f] = s / ((double)cote * cote);
        }
        const bool unePar = environnement::capturesFaites() == avant + 1 && environnement::prete();
        std::vector<float> faces[6]; int cote = 0;
        const bool lu = environnement::lireFaces(r, faces, cote);
        // RG-03 : rien de plus tant qu'aucune capture n'est demandée (60 images : aucune nouvelle capture)
        g_b.face = -1;
        for (int i = 0; i < 60; i++) g.render(t);
        const bool rienDePlus = environnement::capturesFaites() == avant + 1;
        unsetenv("WFS_REFLETS");
        std::string coutCapture = "non mesurable";
        for (const passes::Cout& x : passes::coutsCumules())
            if (x.nom.rfind("capture", 0) == 0 && x.mesurable) coutCapture = fmt(x.ms, 3) + " ms GPU par face (" + std::to_string(x.images) + " faces)";
        mes << "\n== capture d'environnement (WFS-150, RG-03), Ville, " << moment << ", Ultra ==\n"
            << "  cubemap " << cote << " × " << cote << " × 6, R16G16B16A16 (HDR avant exposition) ; point (" << fmt(g_b.point.x, 1) << ", " << fmt(g_b.point.y, 1)
            << ", " << fmt(g_b.point.z, 1) << ") ; une face par image : " << (unePar ? "oui (6 images, 1 capture)" : "NON") << " ; 60 images de plus sans demande : "
            << (rienDePlus ? "aucune capture" : "CAPTURE EN TROP") << " ; coût : " << coutCapture << "\n"
            << "  M-01 : luminance moyenne Y (sRGB linéaire) de chaque face contre la scène vue dans la même direction (carré central de la cible HDR, 90°), ± 1 %\n"
            << "  face | Y face | Y scène | écart | verdict\n";
        if (!unePar || !rienDePlus || !lu || cote != 256) { WFS_ERR("environnement (WFS-150) : capture (%s) : une face par image %s, capture en trop %s, relue %s, côté %d (256 voulu)", moment, unePar ? "oui" : "NON", rienDePlus ? "non" : "OUI", lu ? "oui" : "NON", (int)cote); ecarts++; }
        static const char* NF[6] = {"+X", "-X", "+Y", "-Y", "+Z", "-Z"};
        int conformes = 0;
        for (int f = 0; f < 6 && lu; f++) {
            double s = 0; for (size_t i = 0; i < faces[f].size(); i += 4) s += luminanceM01(&faces[f][i]);
            const double lf = s / ((double)cote * cote), e = lumScene[f] > 0 ? (lf - lumScene[f]) / lumScene[f] : 0;
            const bool ok = std::fabs(e) <= 0.01;
            conformes += ok;
            mes << "  " << NF[f] << " | " << fmt(lf, 4) << " | " << fmt(lumScene[f], 4) << " | " << fmt(e * 100, 2) << " % | " << (ok ? "conforme" : "NON CONFORME") << "\n";
            infos << "M-01 (WFS-150) " << moment << " face " << NF[f] << " : Y " << fmt(lf, 4) << " contre " << fmt(lumScene[f], 4) << " (" << fmt(e * 100, 2) << " %, ± 1 %) : "
                  << (ok ? "conforme" : "non conforme") << "\n";
            WFS_LOG("environnement (WFS-150) : M-01 %s face %s : %.4f contre %.4f (%.2f %%)", moment, NF[f], lf, lumScene[f], e * 100);
        }
        mes << "  M-01 " << moment << " : " << conformes << " / 6 faces conformes\n";
        if (conformes < 6) { WFS_ERR("environnement (WFS-150) : M-01 (%s) : %d / 6 faces conformes", moment, conformes); ecarts++; }
        if (jour && lu) {   // planche en croix (+Y en haut ; −X, +Z, +X, −Z au milieu ; −Y en bas), exposition et ACES du jeu
            const int W = 4 * cote, H = 3 * cote;
            std::vector<uint8_t> px((size_t)W * H * 4, 40);
            const int pos[6][2] = {{2, 1}, {0, 1}, {1, 0}, {1, 2}, {1, 1}, {3, 1}};
            for (int f = 0; f < 6; f++) for (int y = 0; y < cote; y++) for (int x = 0; x < cote; x++) {
                const float* s = &faces[f][((size_t)y * cote + x) * 4];
                uint8_t* d = &px[((size_t)(pos[f][1] * cote + y) * W + pos[f][0] * cote + x) * 4];
                for (int c = 0; c < 3; c++) d[c] = versOctet(s[c], g_b.exposition);
                d[3] = 255;
            }
            stbi_write_png((out + "/realisme-cubemap-essai.png").c_str(), W, H, 4, px.data(), W * 4);
            WFS_LOG("capture realisme-cubemap-essai : 6 faces de %d px de la Ville, jour, Ultra (planche en croix)", cote);
        }
        g.shutdown();
    }
    {   // taille selon le niveau : Haute 128, Basse 64 (demande seule, sans image)
        for (int n : {qualite::HAUTE, qualite::BASSE}) {
            niveau(n);
            if (!g.init(r, "golf", "ville")) { WFS_ERR("environnement (WFS-150) : taille de la capture (niveau %d) : chargement de la Ville impossible", n); ecarts++; continue; }
            g.allerA(Ecran::Chargement); scene(g, t, 20);
            environnement::demander(g_b.point);
            for (int i = 0; i < 6; i++) g.render(t);
            mes << "  capture en " << NOMS[n] << " : " << environnement::taille() << " px (" << (environnement::prete() ? "prête" : "PAS PRÊTE") << ")\n";
            if (environnement::taille() != qualite::tailleCubemap(n) || !environnement::prete()) { WFS_ERR("environnement (WFS-150) : capture en %s : %d px (%d voulus), %s", NOMS[n], environnement::taille(), qualite::tailleCubemap(n), environnement::prete() ? "prête" : "pas prête"); ecarts++; }
            g.shutdown();
        }
    }
    const vec3 pointVille = g_b.point;
    g_b = EtatBanc{};
    niveau(q0 == qualite::HAUTE && u0 ? qualite::ULTRA : q0);
    setenv("WFS_MOMENT", "jour", 1);

    // ---------------- RG-04 / P-05 : Ultra dans les Réglages, parcours accueil → Réglages (Ultra) → course → accueil
    ui::chargerPolice(r);
    if (g.init(r, "golf", "ville")) {
        g.setAutopilot(true); g.camMode = 0;
        double tt = 3000; int n = 0;
        auto image = [&]() { g.update(1.0 / 60); tt += 1.0 / 60; if (++n % 10 == 0) g.render(tt); };
        auto capture = [&](const char* nom) { g.render(tt); g.render(tt + 0.01); r.capturePng(out + "/" + nom + ".png"); WFS_LOG("capture %s : écran %s", nom, nomEcran(g.ecran())); };
        auto toucher = [&](int id, const manette::Zone& z) {
            const float x = z.x + z.w / 2, y = z.y + z.h / 2;
            g.onTouch({id, x, y, true}); image(); g.onTouch({id, x, y, false}); image();
        };
        auto zones = [&]() { g.render(tt); return manette::zonesPrecedentes(); };
        for (int i = 0; i < 30; i++) image();
        g.allerA(Ecran::Menu);
        for (int i = 0; i < 60; i++) image();
        int etapes = 0;
        // 1) accueil : bouton « Réglages » = dernière zone déclarée ; panneau sans Ultra tant que la Haute n'a pas tenu
        qualite::tenueUltra = {};
        std::vector<manette::Zone> z = zones();
        if (!z.empty()) { toucher(1, z.back()); etapes++; }
        z = zones();
        const size_t sansUltra = z.size();
        // 2) la Haute tient (banc de chauffe simulé : 5 mesures de 2 s à 60 images/s, chauffe 0) → Ultra proposé
        for (int i = 0; i < qualite::MESURES_TENUE; i++) qualite::tenueUltra.mesure(60, qualite::HAUTE, 0);
        z = zones();
        const bool propose = z.size() == sansUltra + 1;
        WFS_LOG("réglages (WFS-150) : %zu zones sans Ultra, %zu avec (Haute tenue : %s)", sansUltra, z.size(), qualite::tenueUltra.tenue ? "oui" : "non");
        // zones : Caméra (4 choix), puis Graphismes (Basse, Moyenne, Haute, Ultra) → Ultra = 8e zone ; même rangée que Basse
        if (propose && z.size() > 7 && std::fabs(z[7].y - z[4].y) < 0.5f && std::fabs(z[7].h - z[4].h) < 0.5f && std::fabs(z[7].w - z[4].w) < 0.5f) {
            toucher(2, z[7]); etapes++;
        } else { WFS_ERR("réglages (WFS-150) : choix « Ultra » introuvable (%zu zones)", z.size()); ecarts++; }
        const bool ultra = qualite::niveauAffiche(R.qualite) == qualite::ULTRA;
        capture("reglages-qualite-ultra");
        WFS_LOG("réglages (WFS-150) : %s ; %s", ultra ? "Ultra choisi (coûts de la Haute)" : "ULTRA NON CHOISI", qualite::ligneRendu().c_str());
        qualite::diagnosticForce = true;
        capture("reglages-diagnostic");
        WFS_LOG("réglages (WFS-150) : mode diagnostic : %s", qualite::ligneRendu().c_str());
        qualite::diagnosticForce = false;
        // 3) « Fermer », course 5 s en Ultra, une capture d'environnement demandée au départ (256 px, une seule)
        z = zones();
        if (!z.empty()) { toucher(3, z.back()); etapes++; }
        g.allerA(Ecran::Course);
        const int c0 = environnement::capturesFaites();
        environnement::demander(pointVille);   // point de départ de la Ville (6 m devant, 1,7 m de haut)
        for (int i = 0; i < 60 * 5; i++) image();
        capture("parcours-ultra-course");
        const bool enCourse = g.ecran() == Ecran::Course && g.player().dist > 1;
        const int captures = environnement::capturesFaites() - c0;
        if (enCourse && captures == 1 && environnement::taille() == 256) etapes++;
        WFS_LOG("parcours (WFS-150) : course en Ultra : %s, %.0f m, %d capture(s) d'environnement de %d px", enCourse ? "oui" : "NON", g.player().dist * M, captures, environnement::taille());
        {   // WFS-214 (accroche, RG-03, ANO-106) : chaussée en Ultra puis en Haute, même instant, même vue (niveaux posés directement : le banc ne
            // trouve plus « Ultra » dans le panneau à onglets) ; capture d'environnement refaite à chaque niveau ; niveau d'avant rendu ensuite
            const int qAv = R.qualite; const bool uAv = qualite::ultraChoisi;
            for (const int n : {(int)qualite::ULTRA, (int)qualite::HAUTE}) {
                niveau(n); environnement::demander(pointVille); for (int i = 0; i < 8; i++) g.render(tt);
                capture(n == qualite::ULTRA ? "chaussee-ultra-course" : "chaussee-haute-course");
                WFS_LOG("chaussée (WFS-214) : %s, capture d'environnement de %d px", NOMS[n], environnement::taille());
            }
            R.qualite = qAv; qualite::ultraChoisi = uAv; ecrans::appliquerQualiteNiveau(r);
        }
        // 4) retour à l'accueil, Ultra gardé
        g.allerA(Ecran::Menu);
        for (int i = 0; i < 60; i++) image();
        capture("parcours-ultra-retour");
        if (g.ecran() == Ecran::Menu && qualite::niveauAffiche(R.qualite) == qualite::ULTRA) etapes++;
        mes << "\n== niveau Ultra (WFS-150, RG-04, P-05) ==\n  Ultra proposé seulement si la Haute tient (" << qualite::MESURES_TENUE << " mesures de 2 s à " << fmt(qualite::IPS_TENUE, 0)
            << " images/s ou plus, en course, chauffe 0) : avant " << sansUltra << " zones, après " << z.size() + 0 << " ; Ultra = " << (ultra ? "choisi" : "NON") << "\n"
            << "  parcours accueil → Réglages (Ultra) → course → accueil : " << etapes << " / 5 étapes"
            << " (bouton Réglages, choix Ultra, Fermer, course avec 1 capture de 256 px, retour à l'accueil en Ultra)\n"
            << "  ligne « Rendu » en mode diagnostic : " << "voir reglages-diagnostic.png\n";
        WFS_LOG("parcours (WFS-150) : accueil → Réglages (Ultra) → course → accueil : %d / 5 étapes%s", etapes, etapes == 5 ? " OK" : " ÉCHEC");
        if (etapes != 5 || !propose) { WFS_ERR("parcours (WFS-150) : %d / 5 étapes, Ultra %s", etapes, propose ? "proposé" : "non proposé"); ecarts++; }
        qualite::tenueUltra = {};
        g.shutdown();
    } else { WFS_ERR("parcours (WFS-150) : chargement de la Ville impossible"); ecarts++; }
    niveau(q0 == qualite::HAUTE && u0 ? qualite::ULTRA : q0);
    unsetenv("WFS_MOMENT");
    WFS_LOG("passes (WFS-150) : banc terminé, %d écart(s)", ecarts);
    return ecarts;
}

} // namespace wfs
