// WaForSpeed — systèmes enfichables (point d'extension du moteur, ticket WFS-30).
//
// Un module du jeu (ville, sons, interface, météo…) est une classe dérivée de `System`, dans SON fichier,
// enregistrée avec la macro WFS_SYSTEM. La partie (`Game`) crée tous les systèmes enregistrés et les appelle
// dans la boucle : un nouveau module = un nouveau fichier, sans toucher au noyau.
//
//   // src/world/ville.cpp
//   #include "../core/system.h"
//   class Ville : public wfs::System {
//   public:
//       const char* name() const override { return "ville"; }
//       void init(wfs::Renderer& r, const wfs::GameView& g) override { ... }
//       void render(wfs::Renderer& r, const wfs::GameView& g) override { ... r.draw(...) ... }
//   };
//   WFS_SYSTEM(Ville, 100)   // ordre d'appel : du plus petit au plus grand
//
// Règles : un système lit l'état de la partie via GameView (lecture seule, ne pas modifier le joueur ni la piste) ;
// il possède et libère ses propres ressources GPU (shutdown) ; il ne dessine qu'entre beginFrame et endFrame
// (méthodes render / drawUi). Les fichiers .cpp de src/world, src/ui, src/audio, src/game sont compilés automatiquement.
#pragma once
#include <functional>
#include <memory>
#include <vector>
#include "math.h"

namespace wfs {

class Renderer;
class Track;
struct Player;
struct MapDef;
struct CarDef;
struct Touch;

// état de la partie, en lecture pour les systèmes (rempli par Game à chaque appel)
struct GameView {
    Track* track = nullptr;            // piste (lecture : point(), pose(), findSegment() ; ne pas reconstruire)
    const Player* player = nullptr;    // voiture du joueur (position, vitesse, cap…)
    const CarDef* car = nullptr;       // fiche de la voiture du joueur
    const MapDef* map = nullptr;       // carte en cours
    vec3 camPos{}, camTarget{};        // caméra de l'image en cours (render / drawUi)
    mat4 view = mat4::identity(), proj = mat4::identity();
    double time = 0;                   // temps depuis le lancement (s)
    float uiWidth = 0, uiHeight = 0;   // taille de l'interface (pixels logiques, paysage)
    bool night = false;                // ambiance de nuit
};

class System {
public:
    virtual ~System() = default;
    virtual const char* name() const = 0;
    // création des ressources (après le chargement de la piste et de la voiture)
    virtual void init(Renderer&, const GameView&) {}
    // simulation à pas fixe (même pas que la conduite, 1/120 s)
    virtual void update(double /*dt*/, const GameView&) {}
    // dessin 3D dans la scène (après la route, avant la voiture du joueur)
    virtual void render(Renderer&, const GameView&) {}
    // interface 2D, par-dessus l'image (pixels logiques)
    virtual void drawUi(Renderer&, const GameView&) {}
    // tactile : renvoyer true pour garder le toucher (les commandes de conduite ne le verront pas)
    virtual bool onTouch(const Touch&, const GameView&) { return false; }
    // libération des ressources (GPU au repos)
    virtual void shutdown(Renderer&) {}
};

// registre des systèmes : chaque WFS_SYSTEM ajoute une fabrique au démarrage du programme
struct SystemEntry { int order; const char* name; std::function<std::unique_ptr<System>()> make; };
inline std::vector<SystemEntry>& systemRegistry() { static std::vector<SystemEntry> r; return r; }
struct SystemRegistrar {
    SystemRegistrar(int order, const char* name, std::function<std::unique_ptr<System>()> make) {
        systemRegistry().push_back({order, name, std::move(make)});
    }
};

} // namespace wfs

#define WFS_SYSTEM_CAT2(a, b) a##b
#define WFS_SYSTEM_CAT(a, b) WFS_SYSTEM_CAT2(a, b)
// enregistre la classe T (constructeur sans argument) ; order : ordre d'appel croissant
#define WFS_SYSTEM(T, order) \
    static ::wfs::SystemRegistrar WFS_SYSTEM_CAT(wfsSystemReg_, __LINE__)((order), #T, [] { return std::unique_ptr<::wfs::System>(new T()); });
