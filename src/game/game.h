// WaForSpeed — la partie : piste, voiture du joueur, route générée par morceaux, caméra, commandes tactiles.
#pragma once
#include <functional>
#include <map>
#include <string>
#include <vector>
#include "player.h"
#include "track.h"
#include "../render/renderer.h"
#include "../core/system.h"

namespace wfs {

struct Touch { int id; float x, y; bool down; };

class Game {
public:
    bool init(Renderer& r, const char* carId, const char* mapId, void (*progression)(float, const char*) = nullptr);   // WFS-35 : écran de chargement
    void shutdown();
    void onTouch(const Touch& t);                 // coordonnées logiques (paysage, pixels)
    void setAutopilot(bool on) { autopilot_ = on; }
    void update(double dt);
    void render(double time);
    const Player& player() const { return player_; }
    void setCar(const char* carId);
    // écrans (WFS-36, voir src/core/ecran.h) : demande appliquée entre deux images, systèmes prévenus par onEcran
    void allerA(Ecran e) { ecranDemande_ = e; demandeEcran_ = true; }
    Ecran ecran() const { return ecran_; }
    // marges de sécurité (WFS-51) : données par la plateforme (Android : encoche ; banc : WFS_ENCOCHE) ; les commandes
    // tactiles et leurs zones de toucher sont décalées d'autant ; montrerMarges : bandes visibles (captures du banc)
    void setMarges(const Marges& m) { marges_ = m; view_.marges = m; }
    const Marges& marges() const { return marges_; }
    bool montrerMarges = false;
    // WFS-22 : manette (src/platform/manette_jeu.cpp) : commandes de conduite données par la manette ; vrai = commandes tactiles masquées
    DriveInput manette; bool modeManette = false;
    // caméra : 0 poursuite proche, 1 poursuite haute, 2 orbite (vitrine)
    int camMode = 0; float orbit = 0;
    // WFS-41 : demande de l'écran Terrain, appliquée au début de la mise à jour suivante : carte non vide = course neuve sur
    // cette carte (moment : WFS_MOMENT) ; même carte et même moment : pas de reconstruction (WFS-74, RG-02).
    // WFS-74 : écran de chargement branché par la plateforme (Game ne dépend pas de src/ui/ecrans) : debutChargement avant la
    // reconstruction, progression passée à init (étapes réelles), finChargement(réussite) après ; rien de branché = sans écran.
    struct Demandes {
        std::string carte; bool nuit = true;
        std::function<void()> debutChargement;
        void (*progression)(float, const char*) = nullptr;
        std::function<void(bool ok)> finChargement;
    };
    static Demandes& demandes();
private:
    struct Chunk { Mesh road, walk, ground; bool ready = false; };
    struct ChunkCpu { std::vector<Vertex> v[3]; std::vector<uint32_t> i[3]; };
    void buildChunkCpu(int64_t c, ChunkCpu& out);
    void streamChunks();
    void retire(Mesh& m);
    void drawControls();
    mat4 carMatrix(double s, double lat, double yaw);

    Renderer* r_ = nullptr;
    Track track_;
    Player player_;
    Model car_;
    const CarDef* carDef_ = nullptr;
    vec3 paint_{0.1f, 0.3f, 0.85f};
    std::map<int64_t, Chunk> chunks_;
    std::vector<std::pair<uint64_t, Mesh>> graveyard_;
    uint64_t frameNo_ = 0;
    Material roadMat_, walkMat_, groundMat_, shadowMat_;
    Mesh shadowQuad_;
    double lastTime_ = 0;
    vec3 camEye_{}, camLook_{}; bool camInit_ = false;
    // commandes
    DriveInput input_;
    int steerId_ = -1, gasId_ = -1, brakeId_ = -1, nitroId_ = -1;
    float steerX0_ = 0, steerX_ = 0;
    bool autopilot_ = false;
    bool latchGas_ = false, latchBrake_ = false, latchNitro_ = false;
    double acc_ = 0;
    Marges marges_;
    // écran affiché ; par défaut Course tant que le menu (WFS-37) n'existe pas
    Ecran ecran_ = Ecran::Course, ecranDemande_ = Ecran::Course;
    bool demandeEcran_ = false;
    std::string voitureDemandee_; vec3 peintureDemandee_{};   // WFS-11 : choix du garage (GameView::choisirVoiture)
    void appliquerEcran();
    void changerCarte();   // WFS-41 / WFS-74 : demande de l'écran Terrain (Game::demandes)
    void lacherCommandes();
    // systèmes enfichables (WFS-30, voir src/core/system.h)
    std::vector<std::unique_ptr<System>> systems_;
    GameView view_;
    void systemsInit();
    void systemsShutdown();
    void systemsUpdate(double dt);
    void systemsRender(const vec3& eye, const vec3& look, const mat4& view, const mat4& proj, double time);
    void systemsUi();
    bool systemsTouch(const Touch& t);
};

} // namespace wfs
