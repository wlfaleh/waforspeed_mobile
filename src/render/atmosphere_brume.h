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
// Sources des mesures (RG-02, corrigée par le PO le 2026-11-10, option A : brume d'été de chaleur et de sel) :
//   - visibilité météorologique V = 3,912 / β (formule de Koschmieder, seuil de contraste 2 %, OMM n° 8, 2018, § 9.1.2) ;
//   - Côte d'Azur en été, l'après-midi : brume de chaleur et de sel, visibilité de 10 à 20 km fréquente (relevés METAR de
//     Nice-Côte d'Azur LFMN, été : visibilités de 10 à 20 km les après-midi de brume sèche) ; RG-02 : Corniche ≈ 15 km,
//     Ville ≈ 18 km, Grand Prix ≈ 20 km, Campagne ≈ 22 km (± 10 %) ;
//   - hauteur d'échelle des aérosols : ≈ 1 à 1,5 km sur terre (Seinfeld et Pandis, « Atmospheric Chemistry and Physics »,
//     3e éd., 2016, § 2.4 ; même H_MIE = 1 200 m que atmosphere_physique.h), plus basse au-dessus de la mer (couche limite
//     marine de 500 à 1 000 m, brume de sel) ; air sec de l'intérieur moins chargé.
// Tenue de RG-02 à la Corniche (perte de contraste, objet et œil au même niveau) : 4,97 % à 200 m (≤ 5 %), 22,5 % à 1 km
// (≤ 25 %), 40,0 % à 2 km (35 à 45 %), 72,1 % à 5 km (60 à 75 %).
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
//   plage (Corniche) : brume de mer, V ≈ 15,3 km (β0 2,55·10⁻⁴ : 2,6·10⁻⁴ donnerait 5,07 % à 200 m, au-dessus de la borne),
//   couche marine basse (800 m) ;
//   ville : voile urbain, V ≈ 18 km ;
//   circuit (Grand Prix) et autres : V ≈ 20 km ;
//   campagne : air plus sec de l'arrière-pays, V ≈ 22 km, couche plus haute (1 200 m).
inline Reglage reglageCarte(const char* id) {
    if (!id) id = "";
    if (!std::strcmp(id, "plage")) return {2.55e-4, 800, 0};
    if (!std::strcmp(id, "ville")) return {2.17e-4, 1000, 0};
    if (!std::strcmp(id, "campagne")) return {1.78e-4, 1200, 0};
    return {1.96e-4, 1200, 0};
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
