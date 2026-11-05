// WaForSpeed — écran de chargement de l'application (ticket WFS-35, épic E8 « Écrans et menus »).
// Affiché dès la création du contexte Vulkan, avant tout chargement lourd : fond nuit, sur-titre cyan, logo
// « WAFOR / SPEED » du menu web, barre de progression dégradée cyan → orange et étape en cours ; à 100 %, la scène
// apparaît en fondu de 300 ms (système FonduChargement) ; en cas d'échec, message rouge du jeu web (main.js boot()).
//
// Utilisation (plateformes) :
//   auto& ch = ecrans::chargement();
//   ch.debut(renderer, tVulkan);                                  // première image (police + 0 %)
//   if (!game.init(renderer, "golf", "ville", ecrans::Chargement::progression)) ch.erreur(); else ch.fin();
#pragma once
#include <functional>
#include <string>

namespace wfs {
class Renderer;
class Game;
namespace ecrans {

class Chargement {
public:
    // charge la police (WFS-26) et dessine la première image à 0 % ; depuis : instant de fin de l'initialisation Vulkan
    void debut(Renderer& r, double depuis);
    // nouvelle étape (p de 0 à 1, jamais en recul) : journal de la durée de l'étape précédente, image redessinée
    void etape(float p, const char* libelle);
    // fonction à passer à Game::init (appelée entre les étapes, sur l'écran de chargement de l'application)
    static void progression(float p, const char* libelle);
    void fin();          // 100 % (dernière image), total au journal, puis fondu de 300 ms par-dessus la scène
    void erreur(bool quitter = false);   // échec : message rouge + bouton « Réessayer » (« Quitter » : le moteur ne peut pas repartir)
    // WFS-102 (RG-10) : toucher de la plateforme pendant l'écran d'erreur (vrai : gardé) ; reessaiDemande() : bouton
    // relâché dessus, à traiter une fois (« Réessayer » : l'erreur est effacée, la plateforme relance le chargement)
    bool toucher(int id, float x, float y, bool down);
    bool reessaiDemande();
    void dessiner();     // redessine l'écran tel quel (ex. : écran d'erreur à chaque image)
    // banc d'essai : appelé après chaque image dessinée (n = numéro de l'étape, 0 = début)
    std::function<void(int n)> apresImage;

    float pourcent() const { return p_; }
    const std::string& libelle() const { return libelle_; }
    bool enErreur() const { return erreur_; }
private:
    Renderer* r_ = nullptr;
    float p_ = 0; int n_ = 0; bool erreur_ = false;
    std::string libelle_;
    double t0_ = 0, tEtape_ = 0;
};

Chargement& chargement();   // écran de chargement de l'application (un seul)

// mise en page commune des écrans (.screen-inner du web) et bloc sur-titre + logo, partagés par le chargement et le menu (WFS-37)
struct MiseEnPage { float k, logo, x, largeur, sur, gap, ligne, hauteurBloc; };
MiseEnPage miseEnPage(float W, float H);
// dessine le sur-titre et le logo à partir de y (haut du bloc) ; renvoie y sous le logo + l'espace (gap) suivant
float dessinerBlocLogo(Renderer& r, const MiseEnPage& m, float y, float opacite);
// dessin de l'écran (sans beginFrame / endFrame) : opacite de 0 à 1 (fondu)
void dessinerChargement(Renderer& r, float W, float H, float p, const std::string& libelle, bool erreur, float opacite);
// banc d'essai (desktop_main.cpp) : captures chargement-0/1/2, -fondu et -erreur ; racine(false) fait échouer le
// chargement des fichiers (voiture introuvable), racine(true) le rétablit
void benchChargement(Game& g, Renderer& r, const std::string& outDir, double depuis, void (*racine)(bool ok));
// fondu en cours : opacité de l'écran de chargement à l'instant `temps` de la scène (0 = fini)
float opaciteFondu(double temps);

} // namespace ecrans
} // namespace wfs
