// WaForSpeed — banc « modes-photos » (WFS-562, RG-01, RG-03) : fabrique UNE fois les 3 photos de l'écran « Choisis ton mode »
// dans le jeu lui-même (vrai rendu du moteur, carte Ville, voitures du jeu, scènes posées pour la photo), sans interface :
//   - modes-photo-checkpoint : la voiture du joueur arrive sous le portique de checkpoint, trois quarts arrière, jour ;
//   - modes-photo-course5    : la voiture du joueur au milieu de 4 concurrents, côte à côte dans la même rue, jour ;
//   - modes-photo-poursuite  : la voiture du joueur poursuivie par la police aux gyrophares allumés, nuit.
// Les PNG sont réduits en assets/ui/modes/*.jpg par tools/ui/photos_modes.sh (taille de la carte en 2400 × 1080).
// Pas de rendu hors écran en course : ce banc ne sert qu'à refaire les photos.
#include <cmath>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>
#include "../core/log.h"
#include "../render/renderer.h"
#include "camera_vue.h"
#include "course5.h"
#include "course_system.h"
#include "game.h"
#include "roues_banc.h"   // poursuiteBancPolice (accroche WFS-452)

namespace wfs {

bool portiqueCheckpointVisible();   // src/world/portique.cpp

int benchModesPhotos(Game& g, Renderer& r, const std::string& out) {
    int ecarts = 0;
    double t = 0;
    constexpr double PAS = 1.0 / 60;
    auto image = [&](int n) { for (int i = 0; i < n; i++) { g.update(PAS); t += PAS; if (i % 10 == 9) g.render(t); } };
    auto photo = [&](const std::string& nom) {
        g.bancSansInterface = true;
        g.render(t); g.render(t + 0.01); r.capturePng(out + "/" + nom + ".png");
        g.bancSansInterface = false;
        WFS_LOG("capture %s : joueur %.0f km/h", nom.c_str(), g.player().kmh());
    };
    // caméra de vitrine posée par rapport à la voiture : avantM devant, droiteM à droite, à hauteur m (comme le banc police-540)
    // visee : point visé (devant, droite, hauteur) par rapport à la voiture ; visé sous les sujets → sujets dans la moitié haute
    // de la photo, au-dessus du voile sombre du nom du mode (RG-04)
    auto viser = [&](double avantM, double droiteM, float hauteur, vec3 visee) {
        const Game::Vue v = g.calculerVue(t);
        const vec3 f = normalize(vec3{v.fwd.x, 0, v.fwd.z}), d = cross(f, {0, 1, 0});
        const vec3 o = f * (float)avantM + d * (float)droiteM;
        g.camMode = 2; g.orbit = std::atan2(o.x, o.z); g.orbitRecul = length(o); g.orbitHauteur = hauteur;
        g.orbitVise = f * visee.x + d * visee.y + vec3{0, visee.z - 0.55f, 0};
    };
    auto demarrer = [&](ModeCourse mode, bool nuit) {
        modeCourse() = mode;
        g.allerA(Ecran::Menu); image(2);
        Game::demandes().carte = "ville"; Game::demandes().nuit = nuit;
        image(1);
        g.allerA(Ecran::Course); image(1);
        g.camMode = 1; g.setAutopilot(true); g.modeManette = false;
        image(60 * 4);   // décompte
    };
    const bool regles = courseBenchRulesOn();
    courseBenchRules(true);
    const ModeCourse modeAvant = modeCourse();
    const VueCamera vueAvant = vueCamera();
    choisirVue(VueCamera::Poursuite);
    if (!g.init(r, "golf", "ville")) { courseBenchRules(regles); WFS_ERR("modes-photos : Ville impossible à charger"); return 1; }
    Player& joueur = const_cast<Player&>(g.player());   // banc seulement : voiture remise au milieu de la chaussée

    // 1) Checkpoint : 1er checkpoint où le portique est dressé (pas en courbe serrée ni en tunnel), voiture à 9 m de la ligne
    demarrer(ModeCourse::Checkpoint, false);
    {
        const Course* c = activeCourse();
        auto avancer = [&](double resteM) {
            for (int i = 0; i < 60 * 90 && c && (c->nextCp - joueur.dist) * M > resteM; i++) {
                g.update(PAS); t += PAS; if (i % 30 == 0) g.render(t);
                courseBenchSetTime(30); c = activeCourse();
            }
        };
        avancer(60); g.render(t);
        for (int k = 0; k < 3 && c && !portiqueCheckpointVisible(); k++) {   // passer ce checkpoint (niveau suivant), 60 m avant le prochain
            const int niveau = c->level;
            for (int i = 0; i < 60 * 30 && c && c->level == niveau; i++) { g.update(PAS); t += PAS; courseBenchSetTime(30); c = activeCourse(); }
            avancer(60); g.render(t);
        }
        if (!portiqueCheckpointVisible()) { ecarts++; WFS_ERR("modes-photos : aucun portique de checkpoint dressé"); }
        avancer(9);
        g.setAutopilot(false); g.modeManette = true; g.manette = DriveInput{};
        viser(-14.0, -2.6, 2.8f, {-7.5f, -0.5f, 0.5f});   // voiture à ≈ 34 % de la hauteur, haut du portique à ≈ 6 %
        photo("modes-photo-checkpoint");
        g.modeManette = false;
    }
    // 2) Course à 5 : 4 concurrents tenus autour du joueur (écart en m, file), joueur dans la file du milieu
    demarrer(ModeCourse::Course5, false);
    {
        image(60 * 3);
        joueur.playerX = 0;
        course5BancSuiveurs({{3.0, 0}, {-2.5, 2}, {-8.5, 1}, {9.0, 2}});
        image(3);
        g.setAutopilot(false); g.modeManette = true; g.manette = DriveInput{}; g.manette.gas = true;
        image(2);
        viser(-11.0, -2.0, 3.2f, {-5.0f, -0.5f, 0.6f});   // voiture du joueur à ≈ 35 %, concurrents de 20 à 50 %
        photo("modes-photo-course5");
        course5BancSuiveurs({});
        g.modeManette = false;
    }
    // 3) Poursuite, nuit : la police tenue derrière le joueur (elle le chasse, gyrophares allumés)
    demarrer(ModeCourse::Poursuite, true);
    {
        image(60 * 3);
        g.setAutopilot(false); g.modeManette = true; g.manette = DriveInput{}; g.manette.gas = true;
        // tenue 5,5 m derrière le joueur, un peu sur sa gauche (on voit les deux voitures depuis l'avant), gyrophares allumés
        poursuiteBancPolice(true, -5.5, joueur.playerX - 0.24);
        image(6);
        viser(7.5, -2.6, 1.7f, {3.5f, -0.5f, 0.3f});   // joueur à ≈ 35 %, police derrière lui à ≈ 30 %
        photo("modes-photo-poursuite");
        poursuiteBancPolice(false, 0, 0);
        g.modeManette = false;
    }
    g.setAutopilot(false); g.camMode = 0; g.orbitVise = {};
    g.shutdown();
    modeCourse() = modeAvant; choisirVue(vueAvant);
    courseBenchRules(regles);
    return ecarts;
}

} // namespace wfs
