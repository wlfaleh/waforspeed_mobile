// WaForSpeed — banc de profilage (WFS-222, RG-01) : voir bench_perf.h.
// Chaque système enregistré (WFS_SYSTEM) est enveloppé, le temps du banc seulement, par un « chronomètre » qui appelle le
// vrai système et mesure update / render / drawUi : aucune ligne de game.cpp n'est touchée, le registre est rétabli à la fin.
#include "bench_perf.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "../core/log.h"
#include "../core/memoire.h"   // WFS-297 : mémoire du processus et de la carte graphique
#include "../core/platform.h"
#include "../core/system.h"
#include "../game/game.h"
#include "../game/camera_vue.h"   // WFS-299 : vue Proche au banc des parties
#include "../game/course_system.h"   // WFS-297 : règles de la course au banc des parties
#include "../game/course5.h"   // WFS-335 : Course à 5 au banc des parties
#include "../game/traffic_system.h"   // WFS-297 : voitures du trafic vivantes
#include "eclairage.h"                // WFS-297 : lampadaires déclarés
#include "../ui/ecrans/reglages.h"
#include "../ui/ecrans/chargement.h"   // WFS-297 (RG-06) : écran de chargement et ses chiffres
#include "passes.h"
#include "qualite_auto.h"
#include "renderer.h"

namespace wfs {
namespace {

struct Cumul { double update = 0, render = 0, ui = 0, autre = 0; double appels = 0, triangles = 0, pire = 0, avant = 0; int rendus = 0; };   // WFS-329 : rendus = appels de render()
std::map<std::string, Cumul>& cumuls() { static std::map<std::string, Cumul> c; return c; }

// mesure d'un appel (ms) ajoutée au cumul `champ` du système
struct Mesure {
    double& champ; double t0;
    explicit Mesure(double& c) : champ(c), t0(nowSeconds()) {}
    ~Mesure() { champ += (nowSeconds() - t0) * 1000; }
};

class Chrono : public System {
public:
    Chrono(std::unique_ptr<System> s, const char* nom) : s_(std::move(s)), c_(cumuls()[nom]) {}
    const char* name() const override { return s_->name(); }
    void init(Renderer& r, const GameView& g) override { s_->init(r, g); }
    void update(double dt, const GameView& g) override { Mesure m(c_.update); s_->update(dt, g); }
    void collide(double dt, Player& p, const GameView& g) override { Mesure m(c_.update); s_->collide(dt, p, g); }
    void autopilot(double dt, DriveInput& in, const GameView& g) override { Mesure m(c_.autre); s_->autopilot(dt, in, g); }
    void ambiance(FrameParams& fp, const GameView& g) override { Mesure m(c_.autre); s_->ambiance(fp, g); }
    bool remplaceSol(const GameView& g) const override { return s_->remplaceSol(g); }
    bool remplaceTrottoir(const GameView& g) const override { return s_->remplaceTrottoir(g); }
    bool scene(FrameParams& fp, SceneDediee& sd, const GameView& g) override { Mesure m(c_.autre); return s_->scene(fp, sd, g); }
    void render(Renderer& r, const GameView& g) override {
        uint32_t a0, t0, a1, t1; r.fileScene(a0, t0);
        { Mesure m(c_.render); s_->render(r, g); }
        c_.rendus++;
        r.fileScene(a1, t1); c_.appels += a1 - a0; c_.triangles += t1 - t0;   // ce que le système a mis en file (vu par la caméra)
    }
    void drawUi(Renderer& r, const GameView& g) override { Mesure m(c_.ui); s_->drawUi(r, g); }
    bool onTouch(const Touch& t, const GameView& g) override { return s_->onTouch(t, g); }
    void onEcran(Ecran a, Ecran n, const GameView& g) override { s_->onEcran(a, n, g); }
    void shutdown(Renderer& r) override { s_->shutdown(r); }
private:
    std::unique_ptr<System> s_;
    Cumul& c_;
};

struct Resultat {
    std::string carte, qualite;
    double image = 0, pire = 0, cpu = 0, cpuPire = 0, update = 0, render = 0, attente = 0, fin = 0;
    int plus33 = 0, cpuPlus33 = 0; uint32_t appels = 0, triangles = 0;
    std::vector<std::pair<double, std::string>> postes;   // ms par image, nom (du plus cher au moins cher)
    std::string gpu;
    std::vector<std::pair<double, std::string>> dessins;   // triangles par image, système (passe de scène)
};

} // namespace

// ---------------------------------------------------------------- WFS-297 (RG-01) : parties de suite sans quitter le jeu
// WFS_PERF_PARTIES=1 (ou une liste « plage:jour,ville:nuit,ville:jour ») : la 1re partie par init, les suivantes par le même chemin
// que « Rouler » de l'écran Terrain (Game::demandes, changerCarte). À chaque partie : fenêtres de mesure au départ, après 1 et 2 min
// de course (pilote automatique, une image sur 10 dessinée entre les fenêtres) : image, appels et triangles par système, coût GPU
// des passes, trafic, lampadaires, mémoire du processus (RSS). Une ligne « parties : … » par fenêtre, tableau à la fin.
namespace {
struct Fenetre { std::string partie, quand; double image = 0, gpu = 0, rss = 0, gpuMo = 0; uint32_t appels = 0, triangles = 0; int trafic = 0, lampes = 0; std::string couts, systemes; };
}

static void benchParties(Game& g, Renderer& r, const std::string& out, const std::string& liste) {
    std::vector<std::pair<std::string, bool>> parties;
    std::vector<bool> modes;   // WFS-335 : vrai = Course à 5
    {   // « carte:moment,… » ; défaut : la suite de l'essai de l'utilisateur (Corniche, Ville de nuit, Ville), puis Ville de nuit encore
        const std::string l = liste == "1" ? "plage:jour,ville:nuit,ville:jour,ville:nuit" : liste;
        size_t p = 0;
        while (p < l.size()) {
            size_t v = l.find(',', p); if (v == std::string::npos) v = l.size();
            const std::string e = l.substr(p, v - p); const size_t d = e.find(':');
            // WFS-335 (RG-01) : 3e champ facultatif « course5 » (Course à 5 : adversaires, sans trafic)
            const size_t d2 = d == std::string::npos ? d : e.find(':', d + 1);
            parties.push_back({e.substr(0, d), d != std::string::npos && e.substr(d + 1, d2 == std::string::npos ? d2 : d2 - d - 1) == "nuit"});
            modes.push_back(d2 != std::string::npos && e.substr(d2 + 1) == "course5");
            p = v + 1;
        }
    }
    const int MESURE = std::getenv("WFS_PERF_IMAGES") ? std::atoi(std::getenv("WFS_PERF_IMAGES")) : 120;
    const int MINUTE = std::getenv("WFS_PERF_MINUTE") ? std::atoi(std::getenv("WFS_PERF_MINUTE")) : 60 * 60;   // images de course entre deux fenêtres
    std::vector<Fenetre> res;
    double t = 3000;
    const VueCamera vue0 = vueCamera();
    courseBenchRules(true);   // décompte et « GO » comme dans le jeu (le numéro de la partie avance au départ)
    for (size_t k = 0; k < parties.size(); k++) {
        const auto& [carte, nuit] = parties[k];
        const std::string nom = std::to_string(k + 1) + ". " + carte + (nuit ? " nuit" : " jour");
        modeCourse() = modes[k] ? ModeCourse::Course5 : ModeCourse::Checkpoint;   // WFS-335 : appliqué au départ de la course
        if (k == 0) {
            setenv("WFS_MOMENT", nuit ? "nuit" : "jour", 1);
            if (!g.init(r, "golf", carte.c_str())) { WFS_ERR("parties : chargement %s impossible", carte.c_str()); return; }
        } else {   // comme « Rouler » de l'écran Terrain : la demande est appliquée au début de la mise à jour suivante
            Game::Demandes& d = Game::demandes(); d.carte = carte; d.nuit = nuit;
            g.update(1.0 / 60); g.render(t); t += 1.0 / 60;
        }
        {   // RG-06 : écran de chargement à 100 % (chiffres du chargement et mémoire), capturé à chaque partie
            FrameParams fp; fp.view = mat4::identity(); fp.proj = mat4::identity(); fp.camPos = {0, 0, 0};
            if (r.beginFrame(fp)) { ecrans::dessinerChargement(r, (float)r.width(), (float)r.height(), 1, "Prêt", false, 1); r.endFrame(); }
            r.capturePng(out + "/parties-" + std::to_string(k + 1) + "-chargement.png");
        }
        g.allerA(Ecran::Course); g.setAutopilot(true); g.camMode = 0;
        choisirVue(VueCamera::Proche);   // WFS-299 (remarque du lead dev sur WFS-297) : vue Proche, celle de l'utilisateur
        for (int i = 0; i < 4 * 60; i++) { g.update(1.0 / 60); if (i % 10 == 0) g.render(t); t += 1.0 / 60; }   // décompte et départ (« GO » : partie suivante, RG-06)
        courseBenchSetTime(1e5);   // règles de la course comme dans le jeu, sans fin de chrono pendant les mesures (WFS-299 : sous 1e6, le HUD reste affiché)
        {   // RG-06 : panneau Réglages depuis la pause (rendu, mémoire, numéro de la partie)
            g.allerA(Ecran::Pause); g.update(1.0 / 60);
            ecrans::ouvrirReglagesBanc(true);
            g.render(t); g.render(t + 0.02); r.capturePng(out + "/parties-" + std::to_string(k + 1) + "-reglages.png");
            ecrans::ouvrirReglagesBanc(false); g.allerA(Ecran::Course); g.update(1.0 / 60);
        }
        for (int f = 0; f < 3; f++) {
            const bool villeNuit = carte == "ville" && nuit;   // WFS-299 : aussi en 1re partie   // demande n° 3 du ticket : Ville de nuit en 2e partie, vue Proche, 3 captures à 30 s
            if (f > 0) for (int i = 0; i < MINUTE; i++) {
                g.update(1.0 / 60); if (i % 10 == 0) g.render(t); t += 1.0 / 60;
                if (f == 1 && villeNuit && i >= 10 * 60 && i < 13 * 60 && i % 30 == 0) {   // WFS-299 : suite à 0,5 s d'écart (piétons qui se rapprochent : niveaux de détail)
                    g.render(t); g.render(t + 0.02);
                    r.capturePng(out + "/parties-" + std::to_string(k + 1) + "-suite-" + std::to_string((i - 10 * 60) / 30) + ".png");
                }
                // 0, 30 et 60 s ; WFS-317 (RG-03) : Ville de jour aussi (rue au-delà du 1er km : marcheurs répartis sur le tour)
                if (f == 1 && carte == "ville" && (i == 0 || (i + 1) % (30 * 60) == 0) && i < 61 * 60) {
                    g.render(t); g.render(t + 0.02);
                    r.capturePng(out + "/parties-" + std::to_string(k + 1) + "-ville-" + (nuit ? "nuit-" : "jour-") + std::to_string((i + 1) / 60) + "s.png");
                }
            }
            for (int i = 0; i < 30; i++) { g.update(1.0 / 60); g.render(t); t += 1.0 / 60; }   // images complètes avant la mesure
            for (auto& [n, c] : cumuls()) c = Cumul{};
            passes::remettreAZero();
            Fenetre w; w.partie = nom; w.quand = f == 0 ? "départ" : f == 1 ? "1 min" : "2 min";
            double appels = 0, tri = 0;
            for (int i = 0; i < MESURE; i++) {
                const double t0 = nowSeconds();
                g.update(1.0 / 60); g.render(t); t += 1.0 / 60;
                w.image += (nowSeconds() - t0) * 1000;
                appels += r.appelsDessin(); tri += r.chronoImage().triangles;
            }
            w.image /= MESURE; w.appels = (uint32_t)(appels / MESURE); w.triangles = (uint32_t)(tri / MESURE);
            for (const passes::Cout& c : passes::coutsCumules()) if (c.mesurable) w.gpu += c.ms;
            w.couts = passes::texteCouts(passes::coutsCumules());
            std::vector<std::pair<double, std::string>> d;
            for (auto& [n, c] : cumuls()) if (c.triangles > 0) d.push_back({c.triangles / MESURE, n + " " + std::to_string((int)(c.appels / MESURE)) + " appels / " + std::to_string((int)(c.triangles / MESURE / 1000)) + " k tri."});
            std::sort(d.begin(), d.end(), [](auto& a, auto& b) { return a.first > b.first; });
            for (size_t i = 0; i < d.size() && i < 6; i++) w.systemes += (i ? " ; " : "") + d[i].second;
            const Traffic* tr = activeTraffic(); w.trafic = tr ? (int)tr->cars.size() : 0;
            w.lampes = (int)lampadaires().size(); w.rss = memoire::processusMo(); w.gpuMo = memoire::gpuMo();
            WFS_LOG("parties : %s, %s : image %.1f ms, GPU %.2f ms (%s), %u appels, %u triangles, trafic %d, lampadaires %d, RSS %.0f Mo, carte graphique %.0f Mo, %.2f km",
                    w.partie.c_str(), w.quand.c_str(), w.image, w.gpu, w.couts.c_str(), w.appels, w.triangles, w.trafic, w.lampes, w.rss, w.gpuMo, g.player().dist * M / 1000);
            WFS_LOG("parties : %s, %s : par système : %s", w.partie.c_str(), w.quand.c_str(), w.systemes.c_str());
            {   // WFS-335 (RG-01) : relevé voiture par voiture (une image), distance réelle à la caméra ; triangles par famille
                r.releverVoitures = true; g.update(1.0 / 60); g.render(t); t += 1.0 / 60; r.releverVoitures = false;
                auto rv = r.releveVoitures();
                std::sort(rv.begin(), rv.end(), [](const auto& a, const auto& b) { return a.distance < b.distance; });
                uint32_t somme = 0;
                for (const auto& v : rv) {
                    somme += v.triangles;
                    WFS_LOG("voitures : %s, %s : %s %6.1f m, %5.0f px, niveau %d, %6u triangles, %3u appels", w.partie.c_str(), w.quand.c_str(),
                            v.joueur ? "joueur    " : "autre     ", v.distance, v.px, v.niveau, v.triangles, v.appels);
                }
                const Renderer::ComptesImage& ce = r.comptesImage();
                WFS_LOG("voitures : %s, %s : %zu voitures, %u triangles en file (scène) ; toutes passes : voitures %u, décor %u, personnes %u triangles ; appels voitures %u",
                        w.partie.c_str(), w.quand.c_str(), rv.size(), somme, ce.triangles[Renderer::FAM_VOITURES], ce.triangles[Renderer::FAM_DECOR],
                        ce.triangles[Renderer::FAM_PERSONNES], ce.appels[Renderer::FAM_VOITURES]);
            }
            res.push_back(w);
            if (f == 0) r.capturePng(out + "/parties-" + std::to_string(k + 1) + "-" + carte + (nuit ? "-nuit" : "-jour") + ".png");
        }
    }
    g.shutdown();
    courseBenchRules(false);
    modeCourse() = ModeCourse::Checkpoint;   // WFS-335
    choisirVue(vue0);
    WFS_LOG("parties : | partie | moment | image (ms) | GPU (ms) | appels | triangles | trafic | lampadaires | RSS (Mo) | carte graphique (Mo) |");
    for (const Fenetre& w : res)
        WFS_LOG("parties : | %s | %s | %.1f | %.2f | %u | %u | %d | %d | %.0f | %.0f |", w.partie.c_str(), w.quand.c_str(), w.image, w.gpu, w.appels, w.triangles, w.trafic, w.lampes, w.rss, w.gpuMo);
}

// WFS-329 (ANO-178) : écrans hors course, qualité Basse : WFS_PERF_MENU=ville:nuit,ville:jour. Menu : 30 images au fond animé
// (avant le fond figé), puis 60 images ; moyennes d'appels, de triangles et de temps processeur par image ; captures du menu
// (animé à 1 s, puis figé à 3 s), des Réglages ouverts sur le menu, puis de l'écran Terrain. WFS_FOND_ANIME=1 : fond d'avant
// (scène redessinée à chaque image), même binaire, pour les captures « avant »
static void benchMenu(Game& g, Renderer& r, const std::string& out, const std::string& liste) {
    double t = 5000;
    size_t p = 0;
    while (p < liste.size()) {
        size_t v = liste.find(',', p); if (v == std::string::npos) v = liste.size();
        const std::string e = liste.substr(p, v - p); const size_t d = e.find(':');
        const std::string carte = e.substr(0, d); const bool nuit = d != std::string::npos && e.substr(d + 1) == "nuit";
        p = v + 1;
        setenv("WFS_MOMENT", nuit ? "nuit" : "jour", 1);
        if (!g.init(r, "golf", carte.c_str())) { WFS_ERR("menu : chargement %s impossible", carte.c_str()); continue; }
        const std::string nom = "menu-" + carte + (nuit ? "-nuit" : "-jour");
        g.allerA(Ecran::Menu);
        double appels[2] = {}, tri[2] = {}, ms[2] = {}; int n[2] = {};
        for (int i = 0; i < 90; i++) {
            const double t0 = nowSeconds();
            g.update(1.0 / 30); g.render(t); t += 1.0 / 30;
            const int k = i < 30 ? 0 : 1;
            if (i >= 2 && i != 30) { ms[k] += (nowSeconds() - t0) * 1000; appels[k] += r.appelsDessin(); tri[k] += r.chronoImage().triangles; n[k]++; }
            if (i == 29) r.capturePng(out + "/" + nom + "-1s.png");
            if (i == 30) for (auto& [nm, c] : cumuls()) c.rendus = 0;   // WFS-329 (revue) : systèmes qui tournent encore, fond figé
        }
        {   // revue du lead dev : son (AudioSystem) et mesure des img/s (QualiteSystem) continuent quand le fond est figé
            int sys = 0, tous = 0; std::string audio = "?", qual = "?";
            for (auto& [nm, c] : cumuls()) { tous++; if (c.rendus > 0) sys++; if (nm.find("Audio") != std::string::npos) audio = std::to_string(c.rendus); if (nm.find("Qualite") != std::string::npos) qual = std::to_string(c.rendus); }
            WFS_LOG("menu (WFS-329) : %s : images 31 à 90 : %d système(s) sur %d appelés à chaque image ; render() du son : %s, de la qualité : %s ; %s",
                    nom.c_str(), sys, tous, audio.c_str(), qual.c_str(), qualite::ligneRendu().c_str());
        }
        r.capturePng(out + "/" + nom + "-3s.png");
        for (int k = 0; k < 2; k++) if (n[k]) { appels[k] /= n[k]; tri[k] /= n[k]; ms[k] /= n[k]; }
        WFS_LOG("menu (WFS-329) : %s : 1re seconde : %.0f appels, %.0f k triangles, %.1f ms ; ensuite : %.0f appels, %.0f k triangles, %.1f ms%s",
                nom.c_str(), appels[0], tri[0] / 1000, ms[0], appels[1], tri[1] / 1000, ms[1], r.sceneFigee() ? " (fond figé)" : " (fond animé)");
        ecrans::ouvrirReglagesBanc(true);
        g.render(t); g.render(t + 0.02); r.capturePng(out + "/" + nom + "-reglages.png");
        ecrans::ouvrirReglagesBanc(false);
        g.allerA(Ecran::Terrain);
        for (int i = 0; i < 40; i++) { g.update(1.0 / 30); g.render(t); t += 1.0 / 30; }
        r.capturePng(out + "/" + nom + "-terrain.png");
        WFS_LOG("menu (WFS-329) : %s : écran Terrain : %u appels%s", nom.c_str(), r.appelsDessin(), r.sceneFigee() ? " (fond figé)" : " (fond animé)");
        g.shutdown();
    }
    unsetenv("WFS_MOMENT");
}

void benchPerf(Game& g, Renderer& r, const std::string& out) {
    // registre enveloppé (rétabli à la fin du banc)
    std::vector<SystemEntry>& reg = systemRegistry();
    const std::vector<SystemEntry> origine = reg;
    for (SystemEntry& e : reg) {
        auto make = e.make; const char* nom = e.name;
        e.make = [make, nom]() -> std::unique_ptr<System> { return std::make_unique<Chrono>(make(), nom); };
    }
    if (const char* menu = std::getenv("WFS_PERF_MENU")) {   // WFS-329 : écrans hors course, qualité Basse
        ecrans::Reglages& R0 = ecrans::reglages(); const int q = R0.qualite;
        R0.qualite = qualite::BASSE; ecrans::appliquerQualiteNiveau(r);
        benchMenu(g, r, out, menu);
        R0.qualite = q; ecrans::appliquerQualiteNiveau(r);
        reg = origine;
        return;
    }
    if (const char* parties = std::getenv("WFS_PERF_PARTIES")) {   // WFS-297 : qualité Basse (celle du téléphone de l'utilisateur)
        ecrans::Reglages& R0 = ecrans::reglages(); const int q = R0.qualite;
        R0.qualite = std::getenv("WFS_PERF_HAUTE") ? qualite::HAUTE : qualite::BASSE; ecrans::appliquerQualiteNiveau(r);
        benchParties(g, r, out, parties);
        R0.qualite = q; ecrans::appliquerQualiteNiveau(r);
        reg = origine;
        return;
    }
    const int CHAUFFE = std::getenv("WFS_PERF_CHAUFFE") ? std::atoi(std::getenv("WFS_PERF_CHAUFFE")) : 180;
    const int IMAGES = std::getenv("WFS_PERF_IMAGES") ? std::atoi(std::getenv("WFS_PERF_IMAGES")) : 300;
    const char* seule = std::getenv("WFS_PERF_CARTE");   // essai local : une seule carte
    // WFS-246 (RG-07) : WFS_PERF_ECHELLE=0.6 : scène dessinée à 60 % de ses cibles ; =alterne : 100 % / 60 % toutes les 10 images
    // (la résolution dynamique ne doit recréer aucune cible ni attendre le GPU : recréations comptées, 0 attendu)
    const char* ech = std::getenv("WFS_PERF_ECHELLE");
    const bool alterne = ech && std::string(ech) == "alterne";
    const float echFixe = ech && !alterne ? (float)std::atof(ech) : 1.f;
    ecrans::Reglages& R = ecrans::reglages();
    const int q0 = R.qualite;
    std::vector<Resultat> res;
    struct Cas { const char* carte; int q; const char* nom; };
    for (const char* carte : {"ville", "plage", "campagne", "circuit"}) {
        if (seule && std::string(seule) != carte) continue;
        for (const Cas& c : {Cas{carte, qualite::HAUTE, "Haute"}, Cas{carte, qualite::BASSE, "Basse"}}) {
            R.qualite = c.q; ecrans::appliquerQualiteNiveau(r);
            if (!g.init(r, "golf", c.carte)) { WFS_ERR("perf : chargement %s impossible", c.carte); continue; }
            g.allerA(Ecran::Course); g.setAutopilot(true); g.camMode = 1;
            double t = 2000;
            // essai local : WFS_PERF_AVANCE=n images de course sans mesure (une image dessinée sur 30) avant la chauffe
            const int AVANCE = std::getenv("WFS_PERF_AVANCE") ? std::atoi(std::getenv("WFS_PERF_AVANCE")) : 0;
            for (int i = 0; i < AVANCE; i++) { g.update(1.0 / 60); if (i % 30 == 0) g.render(t); t += 1.0 / 60; }
            r.echelleDynamique(echFixe);   // WFS-246
            for (int i = 0; i < CHAUFFE; i++) { g.update(1.0 / 60); g.render(t); t += 1.0 / 60; }
            const uint32_t recre0 = r.recreations();
            for (auto& [nom, k] : cumuls()) k = Cumul{};   // remise à zéro (les chronomètres gardent une référence sur leur cumul)
            passes::remettreAZero();
            Resultat s; s.carte = c.carte; s.qualite = c.nom;
            double appels = 0, tri = 0;
            double pireHors = 0; std::string pireNom;   // pire image : système le plus cher de cette image
            for (int i = 0; i < IMAGES; i++) {
                if (alterne && i % 10 == 0) r.echelleDynamique((i / 10) % 2 ? 0.6f : 1.f);   // WFS-246 (RG-07)
                for (auto& [nom, k] : cumuls()) k.avant = k.update + k.render + k.ui + k.autre;
                const double t0 = nowSeconds();
                g.update(1.0 / 60);
                const double t1 = nowSeconds();
                g.render(t); t += 1.0 / 60;
                const double t2 = nowSeconds();
                const Renderer::ChronoImage& ci = r.chronoImage();   // image précédente (attente : celle-ci, voir renderer.cpp)
                const double img = (t2 - t0) * 1000, up = (t1 - t0) * 1000, re = (t2 - t1) * 1000;
                // processeur seul : l'image sans l'attente du GPU (lavapipe : le « GPU » est le processeur du banc)
                const double cpu = img - ci.attenteGpuMs;
                s.image += img; s.pire = std::max(s.pire, img); s.plus33 += img > 33.3;
                s.cpu += cpu; s.cpuPire = std::max(s.cpuPire, cpu); s.cpuPlus33 += cpu > 33.3;
                s.update += up; s.render += re; s.attente += ci.attenteGpuMs; s.fin += ci.finImageMs;
                appels += r.appelsDessin(); tri += ci.triangles;
                double sys = 0;
                for (auto& [nom, k] : cumuls()) { const double d = k.update + k.render + k.ui + k.autre - k.avant; sys += d; k.pire = std::max(k.pire, d); }
                if (cpu - sys > pireHors) pireHors = cpu - sys;
            }
            {   // pires images : par système, et moteur hors systèmes (Game, Renderer)
                std::vector<std::pair<double, std::string>> p;
                for (auto& [nom, k] : cumuls()) p.push_back({k.pire, nom});
                p.push_back({pireHors, "moteur hors systèmes"});
                std::sort(p.begin(), p.end(), [](auto& a, auto& b) { return a.first > b.first; });
                for (size_t k = 0; k < p.size() && k < 4; k++) WFS_LOG("perf %s %s :   pire image %-40s %6.2f ms", c.carte, c.nom, p[k].second.c_str(), p[k].first);
            }
            const double n = IMAGES;
            s.image /= n; s.cpu /= n; s.update /= n; s.render /= n; s.attente /= n; s.fin /= n;
            s.appels = (uint32_t)(appels / n); s.triangles = (uint32_t)(tri / n);
            double sysRender = 0, sysUpdate = 0;
            for (auto& [nom, k] : cumuls()) {
                sysRender += k.render + k.ui + k.autre; sysUpdate += k.update;
                s.postes.push_back({k.update / n, nom + ".update"});
                s.postes.push_back({k.render / n, nom + ".render"});
                if (k.triangles > 0) s.dessins.push_back({k.triangles / n, nom + " : " + std::to_string((int)(k.appels / n)) + " appels"});
                if (k.ui > 0.01 * n) s.postes.push_back({k.ui / n, nom + ".drawUi"});
                if (k.autre > 0.01 * n) s.postes.push_back({k.autre / n, nom + ".ambiance/scene/autopilot"});
            }
            s.postes.push_back({s.fin, "Renderer::endFrame (passes, envoi)"});
            s.postes.push_back({std::max(0.0, s.render - s.attente - s.fin - sysRender / n), "Game::render hors systèmes (route, voiture, caméra, beginFrame)"});
            s.postes.push_back({std::max(0.0, s.update - sysUpdate / n), "Game::update hors systèmes (conduite)"});
            std::sort(s.postes.begin(), s.postes.end(), [](auto& a, auto& b) { return a.first > b.first; });
            s.gpu = passes::texteCouts(passes::coutsCumules());
            WFS_LOG("perf %s %s : image %.1f ms (pire %.1f, %d > 33 ms) ; processeur %.1f ms (pire %.1f, %d > 33 ms) : update %.2f, render %.2f "
                    "dont attente GPU %.2f, endFrame %.2f ; %u appels de dessin, %u triangles ; %s",
                    s.carte.c_str(), s.qualite.c_str(), s.image, s.pire, s.plus33, s.cpu, s.cpuPire, s.cpuPlus33, s.update, s.render, s.attente, s.fin,
                    s.appels, s.triangles, s.gpu.c_str());
            for (size_t k = 0; k < s.postes.size() && k < 8; k++)
                WFS_LOG("perf %s %s :   %zu. %-48s %6.2f ms", s.carte.c_str(), s.qualite.c_str(), k + 1, s.postes[k].second.c_str(), s.postes[k].first);
            std::sort(s.dessins.begin(), s.dessins.end(), [](auto& a, auto& b) { return a.first > b.first; });
            for (size_t k = 0; k < s.dessins.size() && k < 8; k++)
                WFS_LOG("perf %s %s :   triangles %-40s %8.0f", s.carte.c_str(), s.qualite.c_str(), s.dessins[k].second.c_str(), s.dessins[k].first);
            res.push_back(s);
            if (ech) {   // WFS-246 (RG-07)
                const uint32_t n = r.recreations() - recre0;
                if (n) WFS_ERR("perf %s %s : échelle %s : %u recréation(s) des cibles pendant la mesure (RG-07 : 0 attendu)", s.carte.c_str(), s.qualite.c_str(), ech, n);
                else WFS_LOG("perf %s %s : échelle %s : aucune recréation des cibles ni attente du GPU pendant la mesure (WFS-246 RG-07 : OK)", s.carte.c_str(), s.qualite.c_str(), ech);
                if (alterne) r.echelleDynamique(0.6f);   // capture à 60 % (agrandie par la passe finale)
            }
            r.capturePng(out + "/perf-" + s.carte + "-" + (c.q == qualite::BASSE ? "basse" : "haute") + (ech ? "-echelle" : "") + ".png");   // fin de la mesure
            r.echelleDynamique(1.f);
            g.shutdown();
        }
    }
    WFS_LOG("perf : tableau (ms par image, banc PC lavapipe ; processeur = image sans l'attente du GPU)");
    WFS_LOG("perf : | carte | qualité | image | pire | > 33 ms | processeur | pire proc. | appels | triangles | 1er poste | 2e poste | 3e poste |");
    for (const Resultat& s : res) {
        auto poste = [&](size_t k) { char b[96]; if (k < s.postes.size()) std::snprintf(b, sizeof b, "%s %.2f", s.postes[k].second.c_str(), s.postes[k].first); else b[0] = 0; return std::string(b); };
        WFS_LOG("perf : | %s | %s | %.1f | %.1f | %d | %.1f | %.1f | %u | %u | %s | %s | %s |", s.carte.c_str(), s.qualite.c_str(), s.image, s.pire, s.plus33,
                s.cpu, s.cpuPire, s.appels, s.triangles, poste(0).c_str(), poste(1).c_str(), poste(2).c_str());
    }
    R.qualite = q0; ecrans::appliquerQualiteNiveau(r);
    reg = origine;
}

} // namespace wfs
