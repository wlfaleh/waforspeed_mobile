#pragma once
// WaForSpeed — WFS-161 (ST-1) : géométrie du kit d'architecture (aucun Vulkan : testée par tests/archi_geo.cpp).
//
// Repère local d'un bâtiment : x le long de la façade (0 → largeur), y vers le haut (0 = sol), z sortant de la façade
// (la façade est le plan z = 0, le bâtiment s'étend vers z < 0 sur sa profondeur). La carte place ce repère dans le
// monde (archi::versMonde). Les ouvertures sont de vrais trous dans le mur (RG-03) : tableaux en retrait, vitre ou
// menuiserie au fond, appui en saillie. uv en mètres pour les murs (textures en répétition), 0..1 par baie pour les vitres
// (intérieurs de nuit, ST-3).
#include <cstdint>
#include <vector>
#include "archi_kit.h"

namespace wfs::archi {

struct Sommet { vec3 p, n; vec2 uv; };
struct Maillage {
    std::vector<Sommet> v;
    std::vector<uint32_t> i;
    size_t triangles() const { return i.size() / 3; }
};

// un maillage par matériau (un appel de dessin par matériau)
enum Part : int { MUR, ENCADREMENT, VITRE, MENUISERIE, TOIT, METAL, VOLET, FIN, CHASSIS, IMAGE, NB_PARTS };   // VOLET : ST-2, volets peints (méditerranéen) ; FIN : WFS-370, barreaux (fils de 16 mm, dessinés en couverture partielle)

// niveaux de détail (RG-05) : 0 complet ; 1 tableaux et vitres gardés, sans appuis, balcons, garde-corps ni édicules ;
// 2 volume seul (murs pleins, vitres peintes à plat sur la façade, toiture), pour le loin
struct Batiment { Maillage part[NB_PARTS]; };

// WFS-370 : graine (0..99) d'une baie, codée dans ses uv : fixe pour une même baie, différente d'une fenêtre et d'un bâtiment à l'autre
inline int graineBaie(const Plan& p, int niveau, int travee) {
    uint32_t x = (uint32_t)(p.hRdc * 1e4f) * 2654435761u ^ (uint32_t)(p.hEtage * 1e4f) * 40503u ^ (uint32_t)(niveau * 131 + travee * 7919 + (int)p.style * 31);
    x ^= x >> 15; x *= 2246822519u; x ^= x >> 13;
    return (int)(x % 100u);
}

// WFS-416 (RG-04) : façon de faire les menuiseries. Interrupteur : selon ouverturesReelles() (jeu, kit de secours) ;
// Reelles : menuiseries 3D de WFS-402 (cuisson, niveau 0) ; Image : au niveau 1, la menuiserie de chaque baie est un seul
// rectangle (pièce IMAGE) posé au plan du dormant, qui porte l'image avec relief de son modèle (uv : u = numéro de tuile
// + 0 à 0,999 sur la largeur de la baie, v = 0 à 1 sur sa hauteur) ; les volets simplifiés du niveau 1 restent en volume ;
// Aucune : murs percés, toiture et traits des planchers seulement, sans le contenu des baies (la cuisson le pose à part, baie
// par baie, pour ne garder qu'un exemplaire de chaque modèle : archi_cuit.h).
enum class Baies : uint8_t { Interrupteur, Reelles, Image, Aucune };
// numéro de la tuile d'image d'une baie (0 à NB_TUILES − 1) : style, type d'ouverture et modèle (4 fenêtres, 4 portes par style)
inline constexpr int TUILES_PAR_STYLE = 16, NB_TUILES = TUILES_PAR_STYLE * (int)Style::Nombre;
int tuileImage(const Plan& p, const Baie& b);

// construit le bâtiment d'un plan, de profondeur donnée, au niveau de détail lod (0, 1 ou 2)
void construire(const Plan& p, float profondeur, int lod, Batiment& out, Baies mode = Baies::Interrupteur);

// WFS-416 (RG-03) : nombre de bâtiments construits (appels de construire) depuis le lancement : le jeu avec un paquet cuit n'en
// fait aucun pendant la course (test et banc « batiments »)
size_t constructions();
// profondeur du tableau d'une baie (retrait de la menuiserie derrière le nu de la façade), m
float profondeurTableau(const Plan& p, const Baie& b);

// WFS-415 (RG-02) : interrupteur des ouvertures réelles de WFS-402 (menuiseries, volets, appuis, porte vitrée). Faux par
// défaut (jeu, APK Android) : chaque baie est un trou en retrait avec sa vitre ou sa porte au fond, comme avant WFS-402, pour
// ne plus rien construire de plus pendant la course. Vrai : rendu de WFS-402 à l'identique (WFS_OUVERTURES=1 au banc,
// tests de WFS-402). À choisir avant toute construction (lu sur les fils de travail, jamais changé pendant une course).
bool ouverturesReelles();
void choisirOuverturesReelles(bool actif);

// WFS-402 : une seule baie du plan (tableau, vitre ou fond, menuiserie, volets, appui, traits du style), sans les murs
void baie(const Plan& p, const Baie& b, int lod, Batiment& out, Baies mode = Baies::Interrupteur);

// aire des murs de la façade avant (z = 0, normale +z) entre le sol et yMax (sans le socle ni l'acrotère), pour les
// contrôles : façade − ouvertures
double aireFacade(const Maillage& mur, float yMax);

} // namespace wfs::archi
