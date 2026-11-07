// WaForSpeed — piétons animés (WFS-19, épic E2). Port de js/world/life.js (populatePeople, addPerson) et de
// js/world/people.js (pedLook, updatePeopleReal) du jeu web : vrais personnages `homme` et `femme` (Quaternius, CC0,
// assets/people/*.wfp) animés par squelette (WFS-63), habillés par l'apparence de WFS-72. Aucune silhouette géométrique.
// Système enfichable « Pietons » (ordre 106, après les décors ville 100 et plage 105).
#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#include "../core/math.h"
#include "../render/renderer.h"   // WFS-210 : ApparencePersonnage

namespace wfs {
class Game;
class Renderer;
// banc d'essai : captures pietons-ville-0 / -1 (deux instants, poses de marche différentes) et pietons-plage, puis g.shutdown()
void benchPietons(Game& g, Renderer& r, const std::string& outDir);

// WFS-210 (RG-01) : tenue selon le lieu (carte en cours, lue par le système des piétons à chaque partie) : Ville (vestes,
// costumes, robes), Corniche (chemises légères, tenues d'été), Campagne (tenues de travail), Grand Prix (supporters,
// combinaisons). Remplace haut, bas, chaussures et la coupe (vetement : manches, jambes ; vetement.w = tissu, TISSU_*) ;
// garde peau, cheveux et corps. Déterministe pour une graine donnée.
namespace tenue {
enum Tissu { TISSU_AUCUN = 0, TISSU_COTON, TISSU_LAINE, TISSU_JEAN, TISSU_LIN, TISSU_SYNTHETIQUE, NB_TISSUS };
inline std::string& carte() { static std::string c = "ville"; return c; }   // carte en cours (écrite par les piétons)
inline const char* carteCourante() { return carte().c_str(); }   // « ville » avant la première partie

inline void tenueLieu(const char* carte, uint32_t graine, bool femme, ApparencePersonnage& app) {
    Mulberry32 R(graine * 2246822519u + 0x210u);
    auto r = [&]() { return (float)R.next(); };
    auto hx = [](unsigned h) { return vec3{((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f}; };
    auto pick = [&](const unsigned* a, int n) { return hx(a[std::min((int)(r() * (float)n), n - 1)]); };
    const std::string c = carte ? carte : "ville";
    float manches = 1.95f, jambes = 1.9f;   // longueurs du jeu web (pedLook) : 1,95 manches longues, 0,45 courtes ; 1,9 pantalon, 0,72 genou
    int tissu = TISSU_COTON;
    if (c == "plage") {   // Corniche : chemises légères et tenues d'été (lin, coton), shorts, robes d'été
        static const unsigned HAUT[] = {0xe2ddd0, 0x8db3d1, 0xd98c74, 0x9cc3b5, 0xc9b48e, 0x2e3f5c, 0xd8d6cf};   // avis expert-realisme-3d
        static const unsigned BAS[] = {0xe2ddd0, 0xc9b48e, 0x2e3f5c, 0x9cc3b5, 0x8e9196};
        static const unsigned PIEDS[] = {0xc9a27a, 0x8a5a36, 0xdcdcd8, 0x40352e};
        app.haut = pick(HAUT, 7); app.chaussures = pick(PIEDS, 4);
        const bool robe = femme && r() < 0.4f;
        app.bas = robe ? app.haut : pick(BAS, 5);
        manches = r() < 0.7f ? 0.45f : (femme ? 0.12f : 1.95f);
        jambes = robe ? 0.72f : (r() < 0.65f ? 0.72f : 1.9f);
        tissu = r() < 0.5f ? TISSU_LIN : TISSU_COTON;
    } else if (c == "campagne") {   // Campagne : tenues de travail (bleu de travail, toile kaki, chemise à carreaux unie, jean)
        static const unsigned HAUT[] = {0x6b6a4a, 0x5a4636, 0x2f4a3a, 0x8a3b2e, 0x5e7391, 0xcfc6b0};   // avis expert-realisme-3d
        static const unsigned BAS[] = {0x5e7391, 0x2f3b52, 0x6b6a4a, 0x5a4636};
        static const unsigned PIEDS[] = {0x4a3020, 0x40352e, 0x5a3a22};
        app.haut = pick(HAUT, 6); app.chaussures = pick(PIEDS, 3);
        const bool combinaison = r() < 0.3f;   // bleu de travail d'une pièce
        app.bas = combinaison ? app.haut : pick(BAS, 4);
        manches = r() < 0.75f ? 1.95f : 0.45f;
        tissu = combinaison ? TISSU_COTON : (r() < 0.6f ? TISSU_JEAN : TISSU_COTON);
    } else if (c == "circuit") {   // Grand Prix : maillots de supporters aux couleurs des équipes, combinaisons de course
        static const unsigned EQUIPES[] = {0xb3201c, 0xdcdcd8, 0x38383b, 0x1f4fa0, 0xe8c21a, 0xe8731e};   // avis expert-realisme-3d (aucun noir sous #303030)
        static const unsigned BAS[] = {0x2b3a55, 0x363638, 0x3a4a66, 0x6b6f75};
        app.haut = pick(EQUIPES, 6);
        if (r() < 0.5f) { app.bas = app.haut; app.chaussures = hx(0x38383b); manches = 1.95f; jambes = 1.9f; tissu = TISSU_SYNTHETIQUE; }   // combinaison
        else { app.bas = pick(BAS, 4); app.chaussures = hx(r() < 0.5f ? 0xdcdcd8 : 0x38383b); manches = 0.45f; tissu = TISSU_COTON; }
    } else {   // Ville : vestes et costumes (laine), robes, tenues de ville
        static const unsigned VESTE[] = {0x3a3d42, 0x2b3a55, 0x8e9196, 0xa8946f, 0x3a3d42};   // avis expert-realisme-3d
        static const unsigned HAUT[] = {0xd8d6cf, 0x8e9196, 0x2b3a55, 0xa8946f, 0x3a3d42, 0x8db3d1};
        static const unsigned ROBE[] = {0x3a3d42, 0x2b3a55, 0x8a3b2e, 0xa8946f, 0x2f4a3a};
        static const unsigned BAS[] = {0x2f3b52, 0x3a3d42, 0x2b3a55, 0x8e9196};
        const float t = r();
        if (femme && t < 0.35f) { app.haut = app.bas = pick(ROBE, 5); jambes = 0.72f; manches = r() < 0.5f ? 1.95f : 0.45f; tissu = TISSU_COTON; }   // robe
        else if (t < 0.7f) { app.haut = pick(VESTE, 5); app.bas = r() < 0.7f ? app.haut : pick(BAS, 4); manches = 1.95f; tissu = TISSU_LAINE; }   // veste, costume
        else { app.haut = pick(HAUT, 6); app.bas = pick(BAS, 4); manches = r() < 0.5f ? 1.95f : 0.45f; tissu = r() < 0.4f ? TISSU_JEAN : TISSU_COTON; }
        app.chaussures = hx(r() < 0.7f ? 0x38383b : 0x4a3020);
    }
    app.maillot = app.haut;
    app.vetement = {manches, jambes, 1.f, (float)tissu};
}
} // namespace tenue

// WFS-199 : spectateurs des tribunes. Les mêmes personnages (homme.wfp, femme.wfp), allégés (sommets regroupés sur une grille,
// en pose de liaison) et posés une fois sur le processeur (Sitting_* / Idle_*, plusieurs instants du clip) : maillage fixe que
// le décor ajoute à ses morceaux (aucun appel de dessin en plus, aucune limite d'instances animées). Zone de chaque sommet,
// calculée comme le pedMaterial du jeu web (scene_skin.frag, WFS-72) : le décor y met ses couleurs.
namespace foule {
enum Zone : uint8_t { PEAU, CHEVEUX, HAUT, BAS, CHAUSSURES };
enum Pose { ASSIS_REPOS, ASSIS_PARLE, DEBOUT_REPOS, DEBOUT_PARLE, NB_POSES };
constexpr int COIFFURES = 3, VARIANTES = 3;   // variante : instant du clip (et manches courtes / longues)
struct Sommet { vec3 p, n; Zone zone; };
// repère local : face vers +z, centre en x = z = 0 ; assis : fesses en y = 0 (posées sur le siège) ; debout : pieds en y = 0
struct Silhouette { std::vector<Sommet> v; std::vector<uint32_t> i; float assise = 0.45f; };   // assise : fesses − pieds (m), assis seulement
// proche : grille de 10 cm, sinon 16 cm (de loin) ; nullptr si les personnages sont illisibles
const Silhouette* silhouette(bool femme, int coiffure, Pose pose, int variante, bool proche);
// WFS-240 (RG-02) : enfants en tribune. ≈ 10 % des spectateurs assis, à côté d'un adulte assis du même rang, deviennent un enfant
// de 7, 10 ou 14 ans (morphotypes de WFS-233) : la place reste occupée, le nombre de spectateurs ne change pas. Tirage par le
// hachage h de la place (le même que le reste du spectateur) ; silhouette assise de l'enfant (fesses en y = 0, assise 0,45 m
// comme le siège : pieds dans le vide), garçon ou fille selon h
// tirage réglé sur les deux tribunes du Grand Prix (même hachage que circuit.cpp) : 180 ‰ des places éligibles → 10,0 %
// des spectateurs (38 enfants sur 381 en Haute : 22 / 250 à la grille, 16 / 131 à l'épingle)
constexpr uint32_t ENFANTS_TRIBUNE_POUR_MILLE = 180;
inline bool enfantEnTribune(uint32_t h, bool assis, bool voisinAdulte) {
    return assis && voisinAdulte && ((h * 0x9E3779B1u) >> 12) % 1000 < ENFANTS_TRIBUNE_POUR_MILLE;
}
const Silhouette* silhouetteEnfant(uint32_t h, Pose pose, int variante, bool proche);
// WFS-240 (RG-01) : petit assis dans une poussette (morphotype de 4 ans, garçon ou fille, assis au repos, de près) ; mise à
// l'échelle de la poussette (poussette::ECHELLE_PETIT) par l'appelant
const Silhouette* silhouettePetit(bool fille);
} // namespace foule
} // namespace wfs
