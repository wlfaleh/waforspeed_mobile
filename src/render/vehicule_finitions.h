// WaForSpeed — WFS-490 (épic E18, domaine E10 Véhicules) : finitions de la peinture (brillant, métallisé, nacré, mat, satiné,
// chromé, caméléon, candy). Le rendu est dans shaders/vehicule.glsl (finitionSurface, vernisFinition) ; valeurs sourcées dans
// references/reel/voitures/peintures.md.
//
// Transport sans constante de poussée en plus (128 octets déjà pris) : la finition voyage dans le métal des pièces peintes,
// pc.params.x = métal + 2 × finition (métal borné à [0, 1] ; lireFinition du shader fait l'inverse). Brillant (0) : valeur
// d'origine, inchangée au bit près (RG-01). Seule la voiture du joueur reçoit une finition (RG-11 : trafic, adversaires, police
// en brillant) : Renderer::finitionJoueur, posée par le garage (WFS-491) ; bancs : variable WFS_FINITION=<id>.
#pragma once
#include <cstring>

namespace wfs::vehicule_finitions {

enum Finition : int { BRILLANT = 0, METALLISE, NACRE, MAT, SATINE, CHROME, CAMELEON, CANDY, NOMBRE };

// identifiants (sauvegarde, configgame, bancs) et noms affichés en français (le garage passe par ui::tr, WFS-491)
inline constexpr const char* IDS[NOMBRE] = {"brillant", "metallise", "nacre", "mat", "satine", "chrome", "cameleon", "candy"};
inline constexpr const char* NOMS[NOMBRE] = {"Brillant", "Métallisé", "Nacré", "Mat", "Satiné", "Chromé", "Caméléon", "Candy"};

// finition d'après son identifiant ; −1 si inconnu (une donnée externe inconnue donne le brillant chez l'appelant)
inline int depuisId(const char* id) {
    if (!id) return -1;
    if (std::strcmp(id, "nacree") == 0) return NACRE;   // WFS-491 (accroche) : identifiant du catalogue (configgame.json, boutique.finitions)
    for (int i = 0; i < NOMBRE; i++) if (std::strcmp(id, IDS[i]) == 0) return i;
    return -1;
}

// métal codé pour pc.params.x ; brillant ou finition hors bornes : métal d'origine, intact
inline float coder(float metal, int finition) {
    if (finition <= BRILLANT || finition >= NOMBRE) return metal;
    const float m = metal < 0.f ? 0.f : (metal > 1.f ? 1.f : metal);
    return m + 2.f * (float)finition;
}
// même calcul que lireFinition (shaders/vehicule.glsl) : rend la finition et remet le métal d'origine dans « metal »
inline int decoder(float& metal) {
    float f = (float)(int)(metal * 0.5f);   // metal ≥ 0 : troncature = floor
    if (metal < 0.f) f = 0.f;
    metal -= 2.f * f;
    return f < 0.f ? 0 : (f > 7.f ? 7 : (int)f);
}

} // namespace wfs::vehicule_finitions
