// WaForSpeed — intérieurs par modèle de la vue cockpit (WFS-197, lot 1 de WFS-195 : Golf, M4, 911, SF90).
// Une voiture qui a son intérieur ici le voit dessiné à la place du cockpit commun (camera_vue.cpp, buildCockpit du jeu web) ;
// toutes les autres gardent exactement le cockpit commun (RG-01). L'intérieur n'est construit que pour la voiture du joueur
// en vue Cockpit et libéré au changement de voiture et au shutdown.
// - Définitions chiffrées par modèle (InterieurDef) : volant, combiné, écran central, planche, essuie-glaces (sources et
//   statut de chaque valeur : commentaires de vehicule_cockpit.cpp, journal de WFS-197).
// - Fonctions pures (tests/test_cockpit.cpp) : choix de l'intérieur, essuie-glaces, chiffres du combiné, projection de la carte.
// - Interieur : géométrie construite par code (vehicule_cockpit_geo.h), écrans dynamiques (combiné, carte), dessin.
// Repère de la caméra : x à droite, y en haut, −z devant ; œil du conducteur à l'origine.
#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
#include "../core/math.h"
#include "renderer.h"

namespace wfs {
class Track;
}

namespace wfs::cockpit {

// WFS-201 : styles du lot 2 ; Aucun = pas de combiné derrière le volant (Model 3 : vitesse, rapport et carte sur l'écran central)
enum class Combine { VwDigital, BmwLive, Porsche5, FerrariIncurve, AudiVirtual, AlfaCadrans, PeugeotICockpit, LamboCompteTours,
                     FordDigital, Aucun };
enum class Essuie { Paralleles, Opposition, Unique };

struct InterieurDef {
    const char* car;                 // id de CarDef
    // volant (RG-03) : diamètre extérieur et épaisseur de la jante (m), branches, méplat (part du rayon coupée en bas)
    float volantD, janteEp; int branches; float meplat;
    bool palettes, manettino, demarreur;
    unsigned cuir, couture[3];       // couleurs sRGB ; couture : 0 = sans (M4 : bleu, violet, rouge)
    unsigned branchesCol, repereCol; // branches (alu, noir laqué…) ; repère du point milieu en haut (0 = sans)
    vec3 volantPos; float volantIncl;   // centre du volant (repère de l'œil), inclinaison de la colonne (rad autour de x)
    // combiné (RG-04) : type, taille (m), centre
    Combine combine; float combineW, combineH; vec3 combinePos;
    unsigned combineActif, combineFond, combineAlerte;   // couleurs du style (sRGB)
    // écran central (RG-05) : taille (m), centre, lacet vers le conducteur (rad)
    float ecranW, ecranH; vec3 ecranPos; float ecranLacet;
    // écran central (RG-05) : ecranW = 0 → pas d'écran central dans la vraie voiture (SF90) : la carte va dans le tiers droit
    // du combiné
    // planche : haut de planche (y, m), bord de la casquette côté conducteur (z) ; pied du pare-brise (y, z)
    float plancheY, plancheZ, pareBriseY, pareBriseZ;
    unsigned plancheHaut, plancheBas, accent; float accentMetal, accentRugo;   // accent : alu brossé, carbone, noir laqué…
    bool carbone;                    // inserts en carbone (M4, SF90)
    // essuie-glaces (RG-06) : disposition, longueur des balais côté conducteur et passager (m), arc de balayage (rad)
    Essuie essuie; float balaiG, balaiD, arc;
    float assise;                    // WFS-201 (RG-07) : hauteur du coussin du siège au-dessus du sol (m) ; œil : hauteurOeil()
    float meplatHaut = 0;            // WFS-201 : méplat en haut de la jante (208 : volant i-Cockpit, plat en haut et en bas)
    bool molettes = false;           // WFS-201 : deux molettes sur les branches (Model 3)
    bool palettesColonne = false;    // WFS-201 : palettes fixées sur la colonne, elles ne tournent pas avec le volant (Giulia, Gallardo)
    bool repereHaut = false;         // WFS-201 : repère de couleur à midi sur la jante (A 45)
    bool boutonsBranches = false;    // WFS-202 : boutons de la couleur du repère sur les branches (Elantra N : boutons N bleus)
};

// WFS-201 (RG-07) : hauteur des yeux assis d'ANSUR-II (references/reel/anthropometrie.json, humain.*.eyeheightsitting) :
// moyenne des hommes (804,5 mm, n = 4082) et des femmes (747,7 mm, n = 1986) pondérée par l'effectif = 785,9 mm ; mesurée
// dos droit, ramenée au dossier incliné de 25° des voitures de tourisme (SAE J1100, angle A40 de 20 à 30° : estimé)
constexpr float OEIL_ASSIS_M = 0.7859f, DOSSIER_RAD = 25.f * PI / 180.f;
inline float hauteurOeil(const InterieurDef& d) { return d.assise + OEIL_ASSIS_M * std::cos(DOSSIER_RAD); }

// WFS-315 (RG-01) : largeur de l'habitacle du modèle : largeur intérieure aux coudes à l'avant (m) et écart latéral entre l'œil
// du conducteur et l'axe de la voiture (m, le conducteur est à gauche), écart des montants ; cotes et sources : vehicule_cockpit_defs.cpp, journal.
// Avant WFS-315, toutes les voitures avaient 1,56 m et 0,36 m (valeurs gardées pour un modèle sans cote)
struct Largeur { float coudes = 1.56f, oeil = 0.36f, montants = 1.44f; };   // montants : écart entre les pieds des deux montants du pare-brise (m)
Largeur largeurPour(const InterieurDef& d);

// RG-01 : intérieur du modèle, nullptr pour une voiture qui garde le cockpit commun
const InterieurDef* interieurPour(const char* carId);
int nombreInterieurs();
const InterieurDef& interieur(int i);

// ---------- essuie-glaces (RG-06) ----------
// cadence (balayages aller-retour par minute) : 0 sans pluie, 45 en vitesse 1, 65 au-delà de 100 km/h (vitesse 2)
inline float cadenceEssuie(double kmh, bool pluie) { return !pluie ? 0.f : std::fabs(kmh) > 100 ? 65.f : 45.f; }
// phase d'un cycle (0..1 : repos → arc → repos) ; sans pluie, le cycle en cours finit puis les balais restent au repos
struct EtatEssuie {
    float phase = 0; bool enMarche = false;
    void avancer(float dt, double kmh, bool pluie) {
        const float c = cadenceEssuie(kmh, pluie);
        if (pluie) enMarche = true;
        if (!enMarche) { phase = 0; return; }
        phase += dt * (c > 0 ? c : 45.f) / 60.f;
        if (phase >= 1) { phase -= std::floor(phase); if (!pluie) { enMarche = false; phase = 0; } }
    }
    // angle du balai (rad) : 0 au repos, `arc` en haut du balayage, mouvement doux (vitesse nulle aux deux bouts)
    float angle(float arc) const { return arc * 0.5f * (1 - std::cos(2 * PI * phase)); }
};

// ---------- combiné (RG-04) : chiffres à 7 segments (graphisme inventé, aucune police copiée) ----------
// segments a (haut), b (haut droit), c (bas droit), d (bas), e (bas gauche), f (haut gauche), g (milieu) : bit 0 = a … bit 6 = g
inline uint8_t segments7(int chiffre) {
    static const uint8_t T[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
    return chiffre >= 0 && chiffre <= 9 ? T[chiffre] : 0;
}
// lettres du rapport : « N » (point mort à l'arrêt), « R » (marche arrière) : segments approchés
constexpr uint8_t SEG_N = 0x37, SEG_R = 0x50;
// valeurs affichées : vitesse (km/h entiers, 0 à 999), régime (part du maximum, 0..1), rapport (1..5, 0 = N, −1 = R), nitro
struct Valeurs { int kmh = 0; float regime = 0; int rapport = 0; float nitro = 0; bool nuit = false; };
inline int vitesseAffichee(double kmh) { return (int)std::clamp(std::lround(std::fabs(kmh)), 0L, 999L); }

// ---------- écran central : carte (RG-05) ----------
// point du monde (x, z) dans le repère de la voiture : u à droite, v devant (m) ; cap = direction de la voiture
// (th de la route + cap du joueur), même convention que la caméra (avant = (sin cap, −cos cap))
inline vec2 versVoiture(vec2 p, vec2 joueur, double cap) {
    const float s = (float)std::sin(cap), c = (float)std::cos(cap);
    const vec2 d{p.x - joueur.x, p.y - joueur.y};
    return {d.x * c + d.y * s, d.x * s - d.y * c};
}
constexpr double CARTE_ARRIERE_M = 150, CARTE_AVANT_M = 350, CARTE_PAS_M = 5;   // fenêtre de la mini-carte GPS (WFS-173 RG-06)
constexpr float CARTE_JOUEUR_Y = 0.30f;   // flèche du joueur à 30 % de la hauteur depuis le bas (route qui arrive au-dessus)

// ---------- état de l'image ----------
struct EtatInterieur {
    float volant = 0;      // angle du volant (rad, volantCible lissé)
    Valeurs v;
    float essuie = 0;      // angle des balais (rad)
};

// banc « cockpit » (WFS-235, RG-05) : vrai = l'intérieur est dessiné d'une couleur clé (mesure de la part de rue visible)
extern bool g_interieurCle;
// WFS-349 (RG-03) : banc « cockpit-voisine » : avec g_interieurCle, dessine aussi en couleur clé le gabarit de ce qui doit rester
// fermé (dehors, sous la ligne de ceinture, sur les côtés) : un pixel de la voisine dans la clé est un pixel interdit
extern bool g_gabaritCeinture;
// WFS-349 : vrai pendant le banc « cockpit-voisine » (mis avant Game::init) : l'intérieur construit aussi le gabarit et la clé
// douce ; faux en course : rien de plus n'est construit
extern bool g_bancCockpitVoisine;
// WFS-349 (RG-02) : ligne de ceinture (bas des vitres latérales, haut des portières), repère de l'œil (m) : haut de la planche
// moins 1 cm, la hauteur de WFS-315 (portières de WFS-197), inchangée
inline float ligneCeinture(const InterieurDef& d) { return d.plancheY - 0.01f; }
// WFS-349 (RG-02) : portière : face intérieure 1 cm en dedans de la largeur aux coudes (WFS-315, inchangée), vitre latérale
// EPAISSEUR_PORTE plus loin (appui de vitre de 12 cm : portières réelles de 10 à 15 cm, DA-realisme) ; plancher sous l'œil
// (talon → point H ≈ 0,27 m, cote SAE H30 d'une berline, plus 0,71 m du point H à l'œil : approximation, ±3 cm)
constexpr float EPAISSEUR_PORTE = 0.12f, PLANCHER_Y = -0.98f;
inline float faceVitre(float demi) { return demi - 0.01f + EPAISSEUR_PORTE; }   // distance de la vitre latérale à l'axe (m)
// WFS-349 (RG-02) : boîtes qui ferment l'habitacle (repère de l'œil, centre et taille en m) : portière gauche, portière droite,
// plancher, tablier. cx, demi : centre de la voiture et demi-largeur aux coudes (largeurPour). Portière : de 6 cm devant le pied
// du pare-brise (sous le pied du montant) à 0,6 m derrière l'œil, du plancher à la ligne de ceinture ; dessinées par
// Interieur::construire, vérifiées par tests/test_cockpit_ferme.cpp (rayons de l'œil)
struct BoiteHab { vec3 centre, taille; };
inline std::array<BoiteHab, 4> boitesFermeture(const InterieurDef& d, float cx, float demi) {
    const float yc = ligneCeinture(d), zAv = d.pareBriseZ - 0.06f, zAr = 0.60f;
    std::array<BoiteHab, 4> b{};
    for (int k = 0; k < 2; k++) {
        const float sx = k == 0 ? -1.f : 1.f, xi = cx + sx * (demi - 0.01f), xv = cx + sx * faceVitre(demi);
        b[(size_t)k] = {{(xi + xv) * 0.5f, (yc + PLANCHER_Y) * 0.5f, (zAv + zAr) * 0.5f}, {EPAISSEUR_PORTE, yc - PLANCHER_Y, zAr - zAv}};
    }
    b[2] = {{cx, PLANCHER_Y - 0.01f, (zAv + zAr) * 0.5f}, {2 * demi, 0.02f, zAr - zAv}};   // plancher (moquette)
    // tablier, sous le bord avant de la planche, monté 1 cm dans la planche (sans ce centimètre, une fente de quelques mm restait
    // sous le bord avant ; vu de l'œil, le haut du tablier reste caché derrière le dessus de la planche, qui remonte vers lui)
    const float haut = d.pareBriseY + 0.01f;
    b[3] = {{cx, (haut + PLANCHER_Y) * 0.5f, d.pareBriseZ}, {2 * demi, haut - PLANCHER_Y, 0.02f}};
    return b;
}
// WFS-349 : couleur linéaire des portières, du plancher et du tablier : celle du bas de planche, luminance ramenée à 0,045 au
// moins (jamais noir pur : plastique noir réel 0,04-0,05, DA-realisme) ; même règle que le cuir du volant
inline vec3 couleurPorte(const InterieurDef& d) {
    auto l = [](unsigned c) { return std::pow((float)(c & 255) / 255.f, 2.2f); };
    vec3 c{l(d.plancheBas >> 16), l(d.plancheBas >> 8), l(d.plancheBas)};
    const float y = 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z;
    if (y < 0.045f) c = y > 1e-5f ? c * (0.045f / y) : vec3{0.045f, 0.045f, 0.045f};
    return c;
}

// WFS-334 (ANO-182, ANO-186) : pare-brise et balais de la dernière image où l'intérieur a été dessiné, lus par la pluie
// (src/world/meteo.cpp) pour ne dessiner les gouttes que derrière la vitre et sur la vitre, chassées par les balais
struct PareBriseVu {
    bool vu = false;                 // vrai : écrit par Interieur::dessiner depuis la dernière lecture (la pluie le remet à faux)
    mat4 repere = mat4::identity();  // repère de l'intérieur (repère de l'œil → monde)
    vec3 coins[4]{};                 // axes des montants : bas gauche, bas droit, haut droit, haut gauche (repère de l'œil, m ; WFS-354)
    int balais = 0;                  // nombre de balais dessinés
    vec3 pivot[2]{}; float sens[2] = {1, 1}, longueur[2] = {0, 0};
    vec3 ex{1, 0, 0}, eu{0, 1, 0};   // plan du pare-brise : axe x et axe « haut » le long des montants
    float angle = 0;                 // angle des balais (rad)
};
PareBriseVu& pareBriseVu();
// WFS-354 (RG-01) : axes des montants dessinés (repère de l'œil) : pieds à ±montant, haut rentré de RETRAIT_MONTANT_HAUT ;
// barres de LARGEUR_MONTANT × EPAISSEUR_MONTANT (mêmes cotes que barre(mont…) d'Interieur::construire). PareBriseVu::coins les
// porte ; la vitre visible de l'œil (sans ce que la barre cache) est calculée par la pluie (meteo::vitreVue)
constexpr float LARGEUR_MONTANT = 0.075f, EPAISSEUR_MONTANT = 0.05f, RETRAIT_MONTANT_HAUT = 0.10f;
// WFS-323 (RG-ANO-207) : haut du pare-brise (traverse avant du pavillon), repère de l'œil (m). Avant : 0,40 m au-dessus de l'œil et
// 0,42 m devant (43° au-dessus du regard : hors de l'image, on voyait le ciel au-dessus du pare-brise, ANO-207). Vision vers le
// haut d'une berline (angle SAE J1050 « upward vision », 14 à 20° : estimé) : traverse à 0,21 m au-dessus de l'œil et 0,52 m devant
// (22° ; 18° de l'œil de la caméra, 4 cm plus haut) ; avec le pied du pare-brise (0,22 m sous l'œil, 1,10 m devant), le pare-brise
// est incliné de ≈ 36° sur l'horizontale (berlines 25 à 40°). Le ciel de toit commence à cette traverse et passe au-dessus de la tête
// (garde au toit ≈ 0,10 m, E)
constexpr float HAUT_PARE_BRISE_Y = 0.21f, HAUT_PARE_BRISE_Z = -0.52f, CIEL_DE_TOIT_Y = 0.27f;
inline void coinsVitre(float cx, float montant, float yb, float zb, float yh, float zh, vec3 c[4]) {
    const float xh = montant - RETRAIT_MONTANT_HAUT;
    c[0] = {cx - montant, yb, zb}; c[1] = {cx + montant, yb, zb}; c[2] = {cx + xh, yh, zh}; c[3] = {cx - xh, yh, zh};
}

class Interieur {
public:
    bool construire(Renderer& r, const InterieurDef& d);
    void detruire(Renderer& r);
    bool pret() const { return def_ != nullptr; }
    const InterieurDef* def() const { return def_; }
    // cam : matrice de la caméra (monde ← œil) ; carte : tracé autour du joueur (points du monde x, z déjà échantillonnés),
    // position et cap du joueur
    void dessiner(Renderer& r, const mat4& cam, const EtatInterieur& e, const std::vector<vec2>& route, vec2 joueur, double cap);
    void tailleCarte(float& w, float& h) const;   // taille de la carte (m) : écran central, ou tiers droit du combiné (SF90)
    int appels() const { return appels_; }   // RG-08 : appels de dessin de la dernière image
    size_t octets() const { return octets_; }   // RG-08 : mémoire des maillages et textures de l'intérieur
private:
    // attache : 0 = habitacle (caméra), 1 = volant (tourne avec lui), 2 = colonne de direction (inclinée, ne tourne pas : WFS-201)
    struct Piece { Mesh m; int mat; int attache; };
    void ajouter(Renderer& r, std::vector<Vertex>& v, std::vector<uint32_t>& i, int mat, int attache);
    void dessinerCombine(Renderer& r, const mat4& cam, const Valeurs& v);
    void dessinerCarte(Renderer& r, const mat4& cam, const std::vector<vec2>& route, vec2 joueur, double cap, bool nuit);
    const InterieurDef* def_ = nullptr;
    std::vector<Piece> pieces_;
    std::vector<Material> mats_;
    // WFS-282 (RG-04) : copies de nuit (même jeu de descripteurs, détruites avec mats_) : lumière d'ambiance de l'habitacle
    std::vector<Material> matsNuit_;
    Image grain_{};                          // normal map du cuir (grain), calculée
    Mesh bras_[2]{}; Material brasMat_{}, brasNuit_{};    // essuie-glaces (dessinés selon l'angle de l'image)
    Material cleMat_{};                      // banc : toutes les pièces d'une couleur clé (g_interieurCle, WFS-235)
    Mesh gabarit_{};                         // WFS-349 (RG-03) : banc seulement, jamais dessiné en course (g_gabaritCeinture)
    Material cleDouce_{};                    // WFS-349 (RG-03) : clé du banc « cockpit-voisine » (bords nets)
    const Material& mat(const Material& m) const;
    // écrans dynamiques : un maillage par couleur (actif, fond, alerte ; route, flèche)
    Mesh dyn_[5]{}; Material dynMat_[5]{}, dynNuit_[5]{};
    std::vector<Vertex> dv_[5]; std::vector<uint32_t> di_[5];
    int appels_ = 0; size_t octets_ = 0;
    float cx_ = 0.36f, demi_ = 0.78f, montant_ = 0.72f;   // WFS-315 : centre de la voiture (repère de l'œil), demi-largeur aux coudes, demi-écart des montants (m)
};

} // namespace wfs::cockpit
