// WaForSpeed — la partie : piste, voiture du joueur, route générée par morceaux, caméra, commandes tactiles.
#pragma once
#include <map>
#include <vector>
#include "player.h"
#include "track.h"
#include "traffic.h"
#include "../render/renderer.h"

namespace wfs {

struct Touch { int id; float x, y; bool down; };

class Game {
public:
    bool init(Renderer& r, const char* carId, const char* mapId);
    void shutdown();
    void onTouch(const Touch& t);                 // coordonnées logiques (paysage, pixels)
    void setAutopilot(bool on) { autopilot_ = on; }
    void update(double dt);
    void render(double time);
    const Player& player() const { return player_; }
    const Traffic& traffic() const { return traffic_; }
    // compteurs de la course (quasi-contacts, chocs) : affichés par le HUD de WFS-12
    int nearMisses = 0, accidents = 0;
    // banc d'essai : place le joueur `metres` derrière la voiture du trafic la plus entourée,
    // décalé de `lat` (demi-largeurs de route), à sa vitesse + `extraKmh` ; renvoie l'indice de la voiture
    int benchPlaceBehind(double metres, double lat, double extraKmh);
    void setCar(const char* carId);
    // caméra : 0 poursuite proche, 1 poursuite haute, 2 orbite (vitrine)
    int camMode = 0; float orbit = 0;
private:
    struct Chunk { Mesh road, walk, ground; bool ready = false; };
    struct ChunkCpu { std::vector<Vertex> v[3]; std::vector<uint32_t> i[3]; };
    void buildChunkCpu(int64_t c, ChunkCpu& out);
    void streamChunks();
    void retire(Mesh& m);
    void drawControls();
    mat4 carMatrix(double s, double lat, double yaw);
    void autopilotInput(DriveInput& in, double dt);
    int nativeModelFor(int webType) const;
    void drawTraffic();

    Renderer* r_ = nullptr;
    Track track_;
    Player player_;
    // modèles 3D des voitures natives (joueur + trafic), chargés une fois ; carIdx_ = voiture du joueur
    Model models_[CAR_COUNT];
    int carIdx_ = 0;
    const CarDef* carDef_ = nullptr;
    Traffic traffic_;
    float shake_ = 0;
    double autoTimer_ = 0, autoTarget_ = 0;
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
    double acc_ = 0;
};

} // namespace wfs
