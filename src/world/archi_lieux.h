#pragma once
// WaForSpeed — WFS-410 (épic E15, file « Ouvertures communes ») : fenêtres et portes réelles des bâtiments HORS du kit
// d'architecture (lieux de la Corniche et de la Ville, église de la Campagne).
//
// Avant ce ticket, ces bâtiments avaient des « fenêtres » peintes à plat (un rectangle sombre ou une fine boîte devant le mur) :
// aucune menuiserie, aucun retrait, forme nue interdite (règle de l'utilisateur du 2026-11-09 19:41 UTC). Ici, une façade de
// lieu est un vrai mur PERCÉ de ses baies, et chaque baie reçoit la menuiserie complète de WFS-402 (archi_ouvertures.h :
// tableau en retrait, dormant, ouvrants, petits bois, porte à panneaux ou vitrée, vitrine à soubassement, appui de pierre,
// volets du style), toujours dans sa version réelle (mode Reelles) : ces lieux sont préparés au chargement (lots de la
// carte), rien n'est construit en course (RG-06), au contraire du kit dont l'interrupteur de WFS-415 coupe les menuiseries.
//
// Aucun Vulkan : testé par tests/test_archi_lieux.cpp. Repère d'une façade : celui de archi_geo.h (x le long de la façade
// depuis son bord gauche vu de la rue, y depuis le sol, z sortant de la façade ; la façade est le plan z = 0).
#include <cstdint>
#include <vector>
#include "archi_geo.h"

namespace wfs::archi::lieux {

// une baie d'une façade de lieu (repère de la façade)
struct Ouv {
    Ouverture type;
    float x, y, l, h;      // axe, bas du tableau, largeur, hauteur (m)
    float plancher = 0;    // plancher de la pièce (poignées 1,00-1,10 m au-dessus)
};

// une façade de lieu : mur [0, largeur] × [y0, y1] percé de ses baies, dans le style choisi (mesures et modèles de WFS-402)
struct Facade {
    Style style = Style::Classique;
    uint32_t graine = 0;   // choix des modèles de fenêtre et de porte (RG-05 de WFS-402)
    float largeur = 0, y0 = 0, y1 = 0;
    float murLibre = 100.f; // plafond du mur plein de chaque côté d'une baie (calculé baie par baie ; volets battants seulement s'ils peuvent s'y rabattre)
    bool commerce = false;   // WFS-410 : toutes les baies sont commerçantes (galerie, centre commercial : éclairées comme des vitrines)
    std::vector<Ouv> baies;
};

// construit la façade au niveau de détail lod : 0 complet, 1 menuiserie simplifiée, 2 loin (mur plein, baies peintes 1 cm
// devant). Le mur va dans la pièce MUR, les tableaux dans ENCADREMENT, les vitres dans VITRE, le reste selon WFS-402.
// vitrines : si donné, les vitrines et les portes (rez-de-chaussée commerçant) y sont construites à part (leur vitre reçoit
// une autre part la nuit : commerces presque tous éclairés, étages 30 à 50 %) ; le mur reste dans out
// Les vitres (pièce VITRE) portent les uv de la pièce de nuit du kit (archi_geo.cpp, uvVitre ; decor.glsl, interieurFenetre) :
// les garder (ajouter(…, garderUv = true)) et les dessiner avec un matériau MAT_INTERIEUR (voir materiauVitre).
void construire(const Facade& f, int lod, Batiment& out, Batiment* vitrines = nullptr);

// arc en plein cintre : baie dont le haut est un demi-cercle (portail, baie d'église). Le mur [0, largeur] × [y0, y1] est
// percé d'un trou dont le haut suit l'arc (n facettes) ; tableau en retrait de d (intrados de l'arc compris), vitrail ou
// porte au fond ; claveaux de pierre en saillie autour de l'arc. Pièces : MUR, ENCADREMENT (tableau et claveaux), VITRE
// (vitrail) ou MENUISERIE (porte à deux vantaux de planches, pentures de fer dans METAL).
struct Arc { float x, y, l, h; bool porte; bool vide = false; };   // axe, bas, largeur, hauteur totale (naissance de l'arc à h − l/2) ; vide : baie ouverte de part en part (baie campanaire), rien au fond
void murArcs(float largeur, float y0, float y1, const std::vector<Arc>& arcs, float d, int lod, Batiment& out);

// passe un maillage du repère de la façade au monde : origine o (coin bas gauche de la façade), x le long de la façade, z
// sortant (unitaires, horizontaux) ; y vers le haut. Repère direct (x × y = z) : faces et normales gardent leur sens.
void versMonde(Maillage& m, vec3 o, vec3 x, vec3 z);
// axe x d'une façade dont la normale sortante est z (repère direct, y vers le haut)
inline vec3 axeFacade(vec3 z) { return cross(vec3{0, 1, 0}, z); }

// recopie un maillage (déjà dans le monde) dans une géométrie de carte (veg::Geo, enseignes::Geo… : v et i), avec une seule
// coordonnée de texture (case de palette) ; garderUv : uv du maillage gardées (vitres : pièce de nuit)
template <class G>
void ajouter(G& g, const Maillage& m, vec2 uv, bool garderUv = false) {
    using V = typename decltype(g.v)::value_type;
    const uint32_t k = (uint32_t)g.v.size();
    for (const Sommet& s : m.v) {
        const vec2 u = garderUv ? s.uv : uv;
        g.v.push_back(V{s.p.x, s.p.y, s.p.z, s.n.x, s.n.y, s.n.z, u.x, u.y});
    }
    for (uint32_t i : m.i) g.i.push_back(k + i);
}

// WFS-410 (avis de l'agent 3D, B1) : réglages du matériau des vitres de lieu, mêmes que les vitres du kit (archi_carte.cpp) :
// pièce de nuit derrière la vitre, lampe de 2 850 K, murs à 31,8 cd/m² (E = 200 lx, ρ = 0,5), teintes de 2 700 à 4 000 K et
// luminances 0,55 à 1,3 tirées par fenêtre (decor.glsl) ; part allumée : ETAGES_ALLUMES des fenêtres d'étage la nuit, VITRINES_ALLUMEES
// des vitrines de commerce (ouvertes le soir) ; de jour, la pièce est éclairée par le ciel seulement
inline constexpr float ETAGES_ALLUMES = 0.40f, VITRINES_ALLUMEES = 0.85f;
inline constexpr float LUMINANCE_PIECE = 31.8f;   // cd/m² (0,5 × 200 lx / π)

// triangles d'un bâtiment (toutes pièces) : preuves du banc et du test
size_t triangles(const Batiment& b);

}  // namespace wfs::archi::lieux
