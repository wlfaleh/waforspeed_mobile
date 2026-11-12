#pragma once
// WaForSpeed — WFS-161 (ST-1) : kit d'architecture, règles d'assemblage (aucun Vulkan : testé par tests/archi_kit.cpp).
//
// Un bâtiment = une façade découpée en travées (colonnes de fenêtres) et en niveaux : un rez-de-chaussée plus haut que les
// étages, des étages courants, une corniche, une toiture. Le style fixe les plages (hauteurs, ouvertures, toitures) ; la
// graine (Mulberry32) choisit dans ces plages, toujours de la même façon pour la même graine.
//
// RG-01 : hauteur d'étage 2,7 à 3,2 m, rez-de-chaussée 3,5 à 4,5 m (règles de gestion de WFS-161), pour les 6 styles.
// Dimensions des ouvertures : plages courantes, à confirmer au référentiel references/reel/ (M-02 ; demande au PO du
// 2026-11-07). RG-02 : styles sans nommer de ville.
#include <cstdint>
#include <vector>
#include "../core/math.h"

namespace wfs::archi {

enum class Style : uint8_t { Classique, Moderne, Mediterraneen, Village, Industriel, Villa, Nombre };
enum class Rdc : uint8_t { Commerce, Porche, Garage, Logement };   // RG-01 : modules de rez-de-chaussée
enum class Etage : uint8_t { Fenetres, Balcons, Loggias };          // RG-01 : modules d'étage
enum class Toit : uint8_t { Zinc, Tuiles, Terrasse };               // RG-01 : toitures

struct Plage {
    float min = 0, max = 0;
    bool contient(float v) const { return v >= min - 1e-4f && v <= max + 1e-4f; }
    float tirer(Mulberry32& R) const { return min + (max - min) * (float)R.next(); }
};

// RG-01 : plages communes à tous les styles
inline constexpr Plage H_RDC{3.5f, 4.5f}, H_ETAGE{2.7f, 3.2f};

// règles d'un style (RG-02) : plages dans lesquelles l'assemblage choisit
struct ReglesStyle {
    const char* nom;
    Plage travee;               // largeur d'une travée (axe à axe des fenêtres), m
    Plage fenL, fenH;           // fenêtre d'étage : largeur, hauteur (tableau), m
    Plage allege;               // hauteur de l'appui au-dessus du plancher, m
    Plage porteL, porteH;       // porte d'entrée, m
    Plage vitrineH;             // vitrine de commerce (hauteur), m
    float retrait;              // profondeur du tableau (fenêtre en retrait du nu de la façade, RG-03), m
    float corniche;             // hauteur de la corniche (0 : aucune), m
    float saillieCorniche;      // débord de la corniche, m
    int etagesMin, etagesMax;   // étages au-dessus du rez-de-chaussée
    uint8_t rdcPossibles;       // bits de Rdc
    uint8_t etagesPossibles;    // bits de Etage
    uint8_t toitsPossibles;     // bits de Toit
};

const ReglesStyle& regles(Style s);

// une ouverture de la façade (repère de la façade : x le long de la façade depuis son bord gauche, y depuis le sol)
enum class Ouverture : uint8_t { Fenetre, PorteFenetre, Porte, Vitrine, PorteGarage };
struct Baie { Ouverture type; int niveau; int travee; float x, y, l, h; bool balcon; };

// plan d'une façade assemblée
struct Plan {
    Style style = Style::Classique;
    uint32_t graine = 0;        // WFS-402 : graine du bâtiment (choix des modèles de fenêtre et de porte, RG-05)
    float largeur = 0;          // largeur de la façade, m
    int travees = 0;            // nombre de travées
    float travee = 0;           // largeur d'une travée, m
    float rive = 0;             // mur plein à chaque bout de la façade (largeur non divisible en travées de la plage), m
    float hRdc = 0, hEtage = 0; // hauteurs choisies (RG-01)
    int etages = 0;             // étages au-dessus du rez-de-chaussée
    float corniche = 0;
    Rdc rdc = Rdc::Logement;
    Etage etage = Etage::Fenetres;
    Toit toit = Toit::Terrasse;
    bool edicules = false;      // toiture-terrasse : édicules techniques (cage d'escalier, machinerie)
    bool secondaire = false;    // WFS-455 : façade arrière ou latérale (facadeSecondaire) : ni balcon filant ni traits de la rue
    std::vector<Baie> baies;
    float hauteur() const { return hRdc + etages * hEtage + corniche; }   // hauteur jusqu'au haut de la corniche
    float niveauY(int n) const { return n == 0 ? 0.f : hRdc + (n - 1) * hEtage; }   // plancher du niveau n
    float niveauH(int n) const { return n == 0 ? hRdc : hEtage; }
    float axe(int t) const { return rive + (t + 0.5f) * travee; }   // axe de la travée t, depuis le bord gauche
};

// albédos des matériaux (sRGB linéaire), recopiés de references/reel/materiaux.json (le dossier references/ n'est pas
// embarqué dans l'APK) ; test_archi_kit vérifie qu'ils restent égaux au référentiel. Pierre, crépi, zinc : absents du
// référentiel, béton à la place (PROVISOIRE, ST-4).
struct Albedo { const char* id; float r, g, b; };
inline constexpr Albedo ALB_BETON{"mat.beton.albedo", 0.30f, 0.30f, 0.30f};   // WFS-206 : 0,51 → 0,30 au référentiel
inline constexpr Albedo ALB_BRIQUE{"mat.brique.albedo", 0.262f, 0.095f, 0.061f};
inline constexpr Albedo ALB_TERRE_CUITE{"mat.terre_cuite.albedo", 0.555f, 0.212f, 0.11f};
inline constexpr Albedo ALB_ALUMINIUM{"mat.aluminium.albedo", 0.916f, 0.923f, 0.924f};
inline constexpr Albedo ALB_INOX{"mat.acier_inox.albedo", 0.669f, 0.639f, 0.598f};
// appareil de la façade de chaque style (decor.glsl, surfaceFacade) : 0 crépi, 1 pierre de taille, 2 brique, 3 moellons, 4 béton
inline constexpr int APPAREIL[(int)Style::Nombre] = {1, 4, 0, 3, 2, 0};

// assemble une façade de largeur donnée, la plus proche possible de la hauteur voulue, dans les règles du style
Plan assembler(Style s, float largeur, float hauteurVoulue, uint32_t graine);

// WFS-455 (ANO-254) : façade secondaire d'un bâtiment (arrière sur cour, ou côté exposé), de largeur donnée : mêmes niveaux,
// même style et mêmes fenêtres d'étage que la façade sur rue p (une façade réelle garde ses hauteurs d'allège et de linteau
// sur tout le bâtiment), mais seulement des fenêtres (aucun balcon, loggia ni vitrine) ; arrière : une porte de service au
// rez-de-chaussée. Les travées sont tirées dans la plage du style avec une graine propre (face 1 arrière, 2 gauche, 3 droite).
// Plan sans aucune baie si la largeur est trop petite pour une fenêtre (mur aveugle : vrai mur mitoyen étroit).
Plan facadeSecondaire(const Plan& p, float largeur, int face);

// contrôles d'un plan (utilisés par le test et par l'assemblage en mode vérification) : renvoie le nombre d'erreurs
// et écrit la première dans erreur (si non nul)
int verifier(const Plan& p, const char** erreur = nullptr);

} // namespace wfs::archi
