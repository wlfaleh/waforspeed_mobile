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
#include "../core/platform.h"
#include "../core/system.h"
#include "../game/game.h"
#include "../game/traffic_system.h"   // WFS-281 (accroche) : voitures du trafic
#include "../ui/ecrans/reglages.h"
#include "passes.h"
#include "qualite_auto.h"
#include "renderer.h"

namespace wfs {
namespace {

struct Cumul { double update = 0, render = 0, ui = 0, autre = 0; double appels = 0, triangles = 0, pire = 0, avant = 0; };
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

void benchPerf(Game& g, Renderer& r, const std::string& out) {
    // registre enveloppé (rétabli à la fin du banc)
    std::vector<SystemEntry>& reg = systemRegistry();
    const std::vector<SystemEntry> origine = reg;
    for (SystemEntry& e : reg) {
        auto make = e.make; const char* nom = e.name;
        e.make = [make, nom]() -> std::unique_ptr<System> { return std::make_unique<Chrono>(make(), nom); };
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
            const int AR = std::getenv("WFS_PERF_AVANCE_RENDU") ? std::atoi(std::getenv("WFS_PERF_AVANCE_RENDU")) : 30;   // WFS-281 (accroche) : 0 = rien dessiné
            for (int i = 0; i < AVANCE; i++) { g.update(1.0 / 60); if (AR > 0 && i % AR == 0) g.render(t); t += 1.0 / 60; }
            if (const Traffic* tr = activeTraffic()) WFS_LOG("perf %s %s : trafic : %zu voitures au total après %d images (WFS-281)", c.carte, c.nom, tr->cars.size(), AVANCE);
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
