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
    Image grain_{};                          // normal map du cuir (grain), calculée
    Mesh bras_[2]{}; Material brasMat_{};    // essuie-glaces (dessinés selon l'angle de l'image)
    // écrans dynamiques : un maillage par couleur (actif, fond, alerte ; route, flèche)
    Mesh dyn_[5]{}; Material dynMat_[5]{}, dynNuit_[5]{};
    std::vector<Vertex> dv_[5]; std::vector<uint32_t> di_[5];
    int appels_ = 0; size_t octets_ = 0;
};

} // namespace wfs::cockpit
