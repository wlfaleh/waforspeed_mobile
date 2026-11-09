// WaForSpeed — conditions d'utilisation et politique de confidentialité (WFS-308, épic E8 « Écrans et menus »).
// Premier lancement (RG-03) : avant le chargement, écran « Conditions d'utilisation » : texte défilant (store/cgu.md puis
// store/confidentialite.md, copiés dans l'APK sous assets cgu/), case « J'ai lu et j'accepte », bouton « Accepter » actif
// seulement après la case, bouton « Refuser ». Refuser (RG-04) : message « Le jeu ne peut pas démarrer sans votre accord »,
// puis fermeture de l'application. Acceptation gardée dans la sauvegarde avec la version (clé cgu=) : écran remontré seulement
// si VERSION_CGU change. Réglages (RG-05) : même texte en lecture seule, bouton « Fermer ».
//
// Utilisation (plateforme, avant Game::init) :
//   if (ecrans::cguAMontrer(sauvegarde().actif(), sauvegarde().cgu)) ecrans::cgu().ouvrir(renderer, ecrans::Cgu::Mode::Premier);
//   chaque image : ecrans::cgu().dessinerImage(maintenant) ; touchers : ecrans::cgu().toucher(id, x, y, down)
//   ecrans::cgu().etat() == Etat::Accepte → ecrans::cgu().noterAcceptation() puis chargement ; Etat::Quitter → fermer l'appli
#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "../../core/system.h"   // Marges

namespace wfs {
class Renderer;
class Game;
class VkCtx;
namespace ecrans {

// version des CGU et de la confidentialité présentées ; la changer (et la ligne « Version N » de store/cgu.md, vérifiée par
// tests/test_cgu.cpp) fait remontrer l'écran à tous les joueurs (RG-04)
constexpr int VERSION_CGU = 1;
constexpr double CGU_DELAI_FERMETURE = 2.5;   // s : message du refus affiché avant la fermeture

// RG-04 : écran à montrer (sauvegarde active, version acceptée plus ancienne que VERSION_CGU) ; sans sauvegarde (banc) : jamais
inline bool cguAMontrer(bool sauvegardeActive, int versionAcceptee) { return sauvegardeActive && versionAcceptee < VERSION_CGU; }

// texte Markdown simple → paragraphes affichables (titres #, ##, puces -, gras ** et italique * retirés, liens [t](u) → « t (u) »)
struct ParagrapheCgu {
    enum Style { TITRE, SOUS_TITRE, TEXTE, PUCE, VIDE } style = TEXTE;
    std::string texte;
};
std::vector<ParagrapheCgu> paragraphesMarkdown(const std::string& md);
// première ligne « Version N » du texte (0 si absente) : tests/test_cgu.cpp la compare à VERSION_CGU
int versionDansTexte(const std::string& md);

class Cgu {
public:
    enum class Mode { Premier, Lecture };
    enum class Etat { Ferme, Ouvert, Accepte, Refuse, Quitter };
    // lit les deux textes (APK : cgu/cgu.md, cgu/confidentialite.md ; PC : store/), défilement en haut, case décochée
    void ouvrir(Renderer& r, Mode m);
    void fermer() { etat_ = Etat::Ferme; }
    // dessin dans une image en cours (Réglages : drawUi) ; maintenant : horloge (s) pour le délai du refus
    void dessiner(Renderer& r, float W, float H, const Marges& m, double maintenant);
    // image complète (beginFrame / endFrame), premier lancement avant le chargement
    void dessinerImage(double maintenant);
    // toucher (glisser dans le texte = défilement) ; vrai si l'écran est ouvert (le toucher est gardé). WFS-355 : boutons visés
    // à 48 dp (cgu_appui.h) ; premierDoigt : ACTION_DOWN d'Android (aucun autre doigt posé) → un lever perdu avant est oublié
    bool toucher(int id, float x, float y, bool down, bool premierDoigt = false);
    // RG-04 : version acceptée écrite dans la sauvegarde et enregistrée ; l'écran se ferme
    void noterAcceptation();
    // ---- plateforme (android_main.cpp), avant Game::init
    // RG-03 : écran ouvert (police chargée) si la sauvegarde le demande ; vrai : le chargement attend l'acceptation
    bool demarrer(Renderer& r);
    // fenêtre perdue pendant l'écran : police libérée (avant Renderer::shutdown), écran fermé (rouvert par demarrer au retour)
    void perdreFenetre(Renderer& r);
    // une image (rotation de l'écran vérifiée comme en jeu, 8 essais ; taille de la fenêtre en pixels) ; renvoie Accepte (acceptation
    // notée, lancer le chargement) ou Quitter (fermer l'appli) une seule fois, sinon l'état courant. Sans écran : rien, Ferme.
    Etat image(VkCtx& vk, uint32_t largeurFenetre, uint32_t hauteurFenetre, double maintenant);

    Etat etat() const { return etat_; }
    bool ouvert() const { return etat_ == Etat::Ouvert || etat_ == Etat::Refuse; }
    // écran du premier lancement ouvert (avant Game::init) : seul cas que la plateforme traite elle-même (la lecture des Réglages
    // vit dans le jeu et se ferme avec lui)
    bool premierLancement() const { return ouvert() && mode_ == Mode::Premier; }
    // fenêtre tenue par l'écran du premier lancement (ouvert, ou « Accepter » touché mais pas encore traité par image())
    bool tientLaFenetre() const { return mode_ == Mode::Premier && (ouvert() || etat_ == Etat::Accepte); }
    Mode mode() const { return mode_; }
    bool caseCochee() const { return coche_; }
    // banc d'essai : zones des boutons (centre) et défilement
    struct Zone { float x = 0, y = 0, w = 0, h = 0; bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; } };
    const Zone& zoneCase() const { return zCase_; }
    const Zone& zoneAccepter() const { return zAccepter_; }
    const Zone& zoneRefuser() const { return zRefuser_; }
    const Zone& zoneFermer() const { return zFermer_; }
    const Zone& zoneTexte() const { return zTexte_; }
    float defilement() const { return defil_; }
    float defilementMax() const { return defilMax_; }

private:
    struct Ligne { ParagrapheCgu::Style style; std::string texte; float x, y, taille; };
    void mettreEnPage(float largeur, float k);
    Renderer* r_ = nullptr;
    Mode mode_ = Mode::Premier;
    Etat etat_ = Etat::Ferme;
    bool coche_ = false;
    std::vector<ParagrapheCgu> paras_;
    std::vector<Ligne> lignes_;
    float largeurPage_ = -1, kPage_ = -1, hauteurTexte_ = 0;
    float defil_ = 0, defilMax_ = 0;
    double tRefus_ = -1, maintenant_ = 0;
    int essaisRotation_ = 0;
    int doigt_ = -1; float yDoigt_ = 0; int cible_ = 0;   // cible du doigt posé : 0 rien, 1 texte, 2 case, 3 accepter, 4 refuser, 5 fermer
    float dp_ = 1;   // WFS-355 : pixels par dp de la dernière image (cibles de 48 dp)
    int imagesLentes_ = 0;   // WFS-355 : images de plus de 0,5 s notées au journal (5 au plus)
    Zone zCase_, zAccepter_, zRefuser_, zFermer_, zTexte_;
};

Cgu& cgu();   // écran unique (premier lancement et Réglages)

// banc d'essai (WFS_BANC=cgu) : parcours RG-06 simulé (sauvegarde du banc) et captures cgu-* puis Réglages → Conditions
int benchCgu(Game& g, Renderer& r, const std::string& out);

} // namespace ecrans
} // namespace wfs
