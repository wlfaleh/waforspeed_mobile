// WaForSpeed — WFS-329 (ANO-178) : fond figé des écrans hors course. Au menu et au choix du terrain, la scène derrière l'interface
// (ville, trafic, foule, voiture du joueur) était redessinée entière à chaque image : 462 appels et 699 k triangles au menu sur le
// téléphone de l'utilisateur, plus qu'une course. Elle est maintenant dessinée normalement le temps de se poser, puis gardée :
// le moteur relit la même image (Renderer::figerScene). Règle seule, sans dépendance (testée par tests/test_fond_fige.cpp).
#pragma once
#include <cstdint>
#include "../core/ecran.h"

namespace wfs::fond_fige {

constexpr int IMAGES_AVANT = 30;   // ≈ 1 s à 30 img/s (cadence hors course) : lumières, expositions et captures posées

// réglages du rendu qui changent l'image : un changement (Réglages ouverts sur le menu) fait redessiner la scène avant de la figer
struct Signature {
    bool basse = false; int ombres = 0; float echelle = 1; int msaa = 1; bool nuit = false;
    bool operator==(const Signature& o) const { return basse == o.basse && ombres == o.ombres && echelle == o.echelle && msaa == o.msaa && nuit == o.nuit; }
};

// écran à fond figé : menu et choix du terrain, sans scène dédiée (le garage a la sienne), hors vue orbite des bancs, hors banc « avant »
inline bool ecranFige(Ecran e, bool sceneDediee, bool orbite, bool fondAnime) {
    return (e == Ecran::Menu || e == Ecran::Terrain) && !sceneDediee && !orbite && !fondAnime;
}

// suivi image par image : images dessinées depuis l'arrivée sur un écran figé (remis à 0 hors de ces écrans ou si la signature
// change) ; figer() dit si le moteur peut garder l'image de la scène
struct Suivi {
    int images = 0; Signature sig{};
    bool avancer(bool fige, const Signature& s) {
        if (!fige || !(s == sig)) images = 0;
        sig = s;
        if (fige) images++;
        return fige && images > IMAGES_AVANT;
    }
};

} // namespace wfs::fond_fige
