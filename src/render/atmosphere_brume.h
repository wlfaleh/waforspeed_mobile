// WaForSpeed — WFS-381 (épic E13, ANO-221) : brume de l'air au loin (perspective atmosphérique), réglage par carte.
//
// Le jour, les objets lointains se fondent dans la couleur de l'horizon : leur contraste restant vaut exp(−τ), τ = épaisseur
// optique entre l'œil et l'objet. τ dépend de la distance ET de la hauteur : l'air près du sol (et de la mer) porte plus
// d'aérosols (couche limite) ; le coefficient d'extinction décroît en exp(−(y − yRef) / H) (hauteur d'échelle H).
//
//   β(y) = β0 × exp(−max(y − yRef, 0) / H)                               (m⁻¹)
//   τ    = β(yŒil) × d × (1 − exp(−Δy / H)) / (Δy / H),  Δy = yPoint − yŒil   (intégrale exacte le long du segment droit)
//   part de la brume = 1 − exp(−τ)
//
// Le même calcul est fait dans shaders/eclairage.glsl (fogAt) : ce fichier en est la référence testée (tests/test_brume.cpp).
// β0 inclut la diffusion des molécules (Rayleigh, ≈ 1,2·10⁻⁵ m⁻¹ pondéré en luminance) et celle des aérosols.
//
// Sources des mesures (RG-02, 2e décision du PO du 2026-11-10 07:30:48, option C : « jour de brume de chaleur » d'été) :
//   - visibilité météorologique V = 3,912 / β (formule de Koschmieder, seuil de contraste 2 %, OMM n° 8, 2018, § 9.1.2) ;
//   - Côte d'Azur en été, l'après-midi : brume sèche de chaleur et de sel, visibilité de 5 à 10 km fréquente (relevés METAR de
//     Nice-Côte d'Azur LFMN, été) ; RG-02 : Corniche 8 km, Ville 10 km, Grand Prix 11 km, Campagne 12 km, autres cartes 10 km ;
//   - hauteur d'échelle des aérosols : ≈ 1 à 1,5 km sur terre (Seinfeld et Pandis, « Atmospheric Chemistry and Physics »,
//     3e éd., 2016, § 2.4 ; même H_MIE = 1 200 m que atmosphere_physique.h), plus basse au-dessus de la mer (couche limite
//     marine de 500 à 1 000 m, brume de sel) ; air sec de l'intérieur moins chargé.
// Tenue de RG-02 à la Corniche (perte de contraste à l'horizontale, œil à 2 m) : 4,8 % à 100 m (≤ 5 %), 9,3 % à 200 m,
// 38,7 % à 1 km, 62,4 % à 2 km, 91,3 % à 5 km.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstring>

namespace wfs::brume {

struct Reglage {
    double beta0;     // extinction au niveau de référence (m⁻¹)
    double hauteur;   // hauteur d'échelle H (m)
    double yRef;      // niveau de référence (m, repère du monde : niveau de la mer et du sol des cartes)
};

// RG-04 : un réglage par carte (paramètres, pas de shader séparé)
//   plage (Corniche) : brume de mer, V 8 km, couche marine basse (800 m) ;
//   ville : voile urbain, V 10 km ;
//   circuit (Grand Prix) : V 11 km ;
//   campagne : air plus sec de l'arrière-pays, V 12 km, couche plus haute (1 200 m) ;
//   autres cartes (Neige, Désert…) : V 10 km.
inline Reglage reglageCarte(const char* id) {
    if (!id) id = "";
    if (!std::strcmp(id, "plage")) return {4.89e-4, 800, 0};
    if (!std::strcmp(id, "ville")) return {3.91e-4, 1000, 0};
    if (!std::strcmp(id, "circuit")) return {3.56e-4, 1200, 0};
    if (!std::strcmp(id, "campagne")) return {3.26e-4, 1200, 0};
    return {3.91e-4, 1000, 0};
}

// visibilité météorologique (m) d'un réglage (Koschmieder)
inline double visibilite(const Reglage& r) { return r.beta0 > 0 ? 3.912 / r.beta0 : 1e30; }

// épaisseur optique entre l'œil (hauteur yOeil) et un point à la distance d (m) et à la hauteur yPoint
inline double epaisseur(const Reglage& r, double d, double yOeil, double yPoint) {
    if (!(d > 0) || !(r.beta0 > 0)) return 0;
    if (!(r.hauteur > 0)) return r.beta0 * d;
    const double invH = 1.0 / r.hauteur;
    const double bOeil = r.beta0 * std::exp(-std::max(yOeil - r.yRef, 0.0) * invH);
    const double x = (std::max(yPoint, r.yRef) - std::max(yOeil, r.yRef)) * invH;   // même borne que le shader : sous yRef, densité de yRef
    const double f = std::fabs(x) < 1e-4 ? 1.0 - 0.5 * x : (1.0 - std::exp(-x)) / x;
    return bOeil * d * f;
}

// part de la couleur de la brume dans le pixel (0 : objet net, 1 : seulement la brume)
inline double part(const Reglage& r, double d, double yOeil, double yPoint) { return 1.0 - std::exp(-epaisseur(r, d, yOeil, yPoint)); }
// contraste restant d'un objet par rapport à la brume (1 = intact)
inline double contraste(const Reglage& r, double d, double yOeil, double yPoint) { return std::exp(-epaisseur(r, d, yOeil, yPoint)); }

} // namespace wfs::brume
