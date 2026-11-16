// WaForSpeed — WFS-557 (ordre de l'utilisateur du 2026-11-16 : maquette 05 à 100 %, « tu es un développeur 3D : modélise-le ») :
// coffre en bois du cadeau du jour, posé dans le parking du menu à la place de la voiture pendant l'écran du cadeau ; le couvercle
// s'ouvre (charnière à l'arrière) et une lumière chaude sort de l'intérieur.
//
// Règle de l'utilisateur (2026-11-09 19:41 UTC) : aucune forme géométrique nue. Le coffre est modélisé par le code au niveau du
// vrai objet (coffre de transport en pin, mesures dans coffre_cadeau.cpp) : patins, planches séparées et chanfreinées, montants
// d'angle, feuillards d'acier, couvercle en planches sur traverses, grain du bois en texture générée au chargement.
// Repère : celui de la voiture du menu (mètres, y vers le haut, sol en y = 0, avant de la voiture vers −Z).
#pragma once
#include "../render/renderer.h"

namespace wfs {
struct FrameParams;
namespace ecrans::coffre {

// mesures du coffre (m) : empreinte d'une palette Europe (1,20 × 0,80), corps de 4 planches de 145 mm sur des patins de 95 mm
inline constexpr float LARGEUR = 1.20f, PROFONDEUR = 0.80f, PATIN = 0.095f, PLANCHE = 0.145f, JOINT = 0.005f, EPAISSEUR = 0.021f;
inline constexpr int RANGS = 4;
inline constexpr float HAUT_CORPS = PATIN + RANGS * PLANCHE + (RANGS - 1) * JOINT;   // 0,69 m
inline constexpr float OUVERTURE_MAX = 1.95f;   // couvercle ouvert à 112°, basculé vers l'arrière : on voit son dessous éclairé (maquette 05)

class Coffre {
public:
    bool init(Renderer& r);
    void shutdown(Renderer& r);
    bool pret() const { return pret_; }
    // ouverture : 0 fermé, 1 grand ouvert ; place : matrice du coffre dans le monde (repère de la voiture du menu)
    void dessiner(Renderer& r, const mat4& place, float ouverture);
    // lumière chaude de l'intérieur (lumière ponctuelle), d'autant plus forte que le couvercle est ouvert
    void eclairer(FrameParams& fp, const mat4& place, float ouverture) const;
    size_t triangles() const { return triangles_; }

private:
    enum Mat : int { PIN_A, PIN_B, PIN_C, ACIER, INTERIEUR, NB_MATS };
    Mesh corps_[NB_MATS]{}, couvercle_[NB_MATS]{};
    Material mat_[NB_MATS]{};
    Image grain_{}, grainMR_{};
    bool pret_ = false;
    size_t triangles_ = 0;
};

Coffre& commun();

} // namespace ecrans::coffre
} // namespace wfs
