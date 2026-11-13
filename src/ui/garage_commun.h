// WaForSpeed — garage : outils communs aux panneaux (WFS-534, découpage de garage.cpp ; voir le guide en tête de garage.h).
// Jetons de couleur de l'interface, fiche de chaque voiture, zones d'appui de la dernière image, mise en page partagée et
// petite interface commune des panneaux (dessiner, zones d'appui, appui). Interne au garage : seuls les fichiers
// src/ui/garage*.cpp l'incluent.
#pragma once
#include <cmath>
#include <string>
#include "../core/math.h"
#include "../game/cars.h"
#include "../game/sauvegarde.h"   // WFS-487 : PEINTURES

namespace wfs {
class Renderer;
struct FrameParams;
struct SceneDediee;
struct GameView;
namespace ecrans::garage {

// jetons :root du jeu web (codes sRGB) : css/style.css
inline vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
constexpr unsigned TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, SURFACE2 = 0x1E2130,
                   ON_ACCENT = 0x0A0B10, TAG = 0xC9CDDA, BG = 0x0A0B10;
inline const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.72f}, BORD{1, 1, 1, 0.08f};
// couleur 0xRRGGBB → linéaire (même conversion que la peinture des voitures, game.cpp)
inline vec3 lin(unsigned h) {
    auto c = [&](int sh) { return std::pow(((h >> sh) & 255) / 255.f, 2.2f); };
    return {c(16), c(8), c(0)};
}

// WFS-487 (RG-01) : une seule liste de couleurs, PEINTURES (src/game/sauvegarde.h) ; WFS-489 : le garage les montre toutes (28)
inline const PeintureDef* const PAINTS = PEINTURES;
constexpr int NB_PEINTURES = wfs::NB_PEINTURES;
int peintureParId(const char* id);

// champs de CARS (catalog.js) utilisés seulement par le garage : type (body), accroche (tag), néon (glow), peinture par défaut
struct Fiche { const char* id; const char* body; const char* tag; unsigned glow; const char* paint; };
const Fiche& ficheDe(const char* id);

int indexDe(const CarDef* c);   // indice dans CARS[] de la voiture de la partie (1re voiture permise sinon)
// WFS-404 (RG-01, RG-02) : le garage ne propose que les voitures permises (voituresPermises(), cars.h) ; l'index reste un indice
// de CARS[], la place affichée (« 05 / 17 ») et les points comptent seulement les voitures permises
int placeDe(int i);
std::string deuxChiffres(int n);

// zones touchables posées à la dernière image
struct Zone {
    float x = 0, y = 0, w = 0, h = 0;
    bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; }
};
// Toutes les zones du garage, une seule variable globale comme avant le découpage : elles survivent à la partie (le banc
// d'essai et les parcours lisent garageBouton juste après un Game::init) et sont vidées en quittant l'écran Garage.
// Chaque panneau n'écrit que ses propres champs.
struct Zones { Zone prev, next, choisir, retour, peintures[NB_PEINTURES], points[CAR_COUNT];
               Zone onglets[2], chevrons[2], toit[NB_PEINTURES + 1]; };   // WFS-489 : onglets, chevrons et pastilles « Toit et rétros »
extern Zones g_z;

// parallélogramme d'un seul tenant (WFS-99), haut décalé à droite de `biais`
void biseau(Renderer& r, float x, float y, float w, float h, float biais, const vec4& c);
// dégradé linéaire d'un seul uiQuad (WFS-99) : c0 → c1 de gauche à droite (horizontal) ou de haut en bas
void degrade(Renderer& r, float x0, float y0, float x1, float y1, const vec4& c0, const vec4& c1, bool horizontal);

// mise en page de l'image, calculée par garage.cpp et lue par les panneaux (pixels de l'interface)
struct MiseEnPage {
    float W = 0, H = 0, k = 1, op = 1;          // taille de l'écran, échelle (H / 720), opacité de l'entrée
    float gl = 0, gr = 0, gt = 0, gb = 0;       // marges gauche, droite, haut, bas (zone sûre comprise)
    float padX = 0, padY = 0, statH = 0, paintH = 0, panelH = 0;   // .show-panel : marges internes, hauteurs des rangées
    float px0 = 0, px1 = 0, py0 = 0, y1 = 0;    // .show-panel : bords, haut, haut du contenu
    float bx = 0, bw = 0;                       // colonne des boutons (Choisir, Retour) à droite du panneau
};
// état du garage lu par les panneaux
struct EtatGarage {
    int index = 0;            // voiture affichée (indice dans CARS[])
    int peinture = 0;         // peinture de cette voiture (indice dans PAINTS)
    bool voiturePrete = false;   // modèle 3D chargé (sinon « Chargement… »)
    int peinture2 = -1;          // WFS-489 : couleur secondaire montrée (indice dans PAINTS), −1 = comme la carrosserie
    bool operator==(const EtatGarage&) const = default;
};

// interface commune d'un panneau du garage (WFS-534, RG-01)
class PanneauGarage {
public:
    virtual ~PanneauGarage() = default;
    virtual void dessiner(Renderer& r, const MiseEnPage& mp, const EtatGarage& e) = 0;   // dans drawUi, pose ses zones
    virtual int appui(float x, float y) const = 0;   // numéro de la zone touchée (propre au panneau), -1 si aucune
    virtual void viderZones() = 0;                   // en quittant l'écran Garage
};

// accès du banc d'essai (garage_bancs.cpp) au système garage de la partie en cours
class AccesBancGarage {
public:
    virtual int index() const = 0;                     // voiture affichée (indice dans CARS[])
    virtual void choisirPeinture(int i) = 0;           // peinture de la voiture affichée
    virtual bool reglerScene(FrameParams& fp, SceneDediee& s, const GameView& g) = 0;   // scène dédiée du garage
    virtual void dessinerPlanche(Renderer& r) = 0;     // planche des 16 peintures
protected:
    ~AccesBancGarage() = default;
};
AccesBancGarage* garageEnCours();   // nullptr si aucune partie n'a de garage

} // namespace ecrans::garage
} // namespace wfs
