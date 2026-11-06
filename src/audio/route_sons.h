// WaForSpeed — sons de la route (WFS-174, épic E6) : vent et roulement des pneus selon la vitesse, bande rugueuse au bord
// de la route, herbe / gravier hors de la route, chuintement de l'eau sur chaussée mouillée.
// Calcul seul (aucune dépendance au jeu) : SfxEvenements (sfx.cpp) calcule les cibles à chaque pas de course, SfxMixer les
// lisse et mélange des boucles calculées une fois au démarrage (RG-09). Testé par tests/test_audio.cpp.
#pragma once
#include <algorithm>
#include <cmath>

namespace wfs::route {

// ---- réglages (RG-01 à RG-08, valeurs du PO)
constexpr float VENT_KMH_MIN = 40, VENT_MAX = 0.30f, VENT_NITRO = 1.2f;   // vent nul sous 40 km/h, 0,30 à la pointe, +20 % en nitro
constexpr float OUVERTURE_KMH = 50;                                        // filtre du vent : 400 Hz à 50 km/h → 2 500 Hz à la pointe
constexpr float VENT_GRAVE_HZ = 400, VENT_AIGU_HZ = 2500;
constexpr float BATTEMENT_HZ = 0.3f, BATTEMENT = 0.15f;                    // battement lent du vent : 0,3 Hz, ± 15 %
constexpr float ROULEMENT_MAX = 0.18f;                                     // grondement 80-250 Hz, proportionnel à la vitesse
constexpr float BORD_VOL = 0.30f, BORD_HZ_MIN = 15, BORD_HZ_MAX = 60;       // bande rugueuse : impulsions de 15 à 60 Hz
constexpr float HERBE_VOL = 0.35f;                                         // herbe / gravier : 1-4 kHz, crépitements
constexpr float EAU_MAX = 0.20f;                                           // chuintement d'eau 3-6 kHz, sous la pluie
constexpr float BORD_V_MIN = 0.01f;                                        // bord : au-delà de 1 % de la vitesse maximale
constexpr float SOMME_MAX = 0.45f;                                         // RG-08 : somme des sons de la route
constexpr float LISSAGE_S = 0.15f;                                         // RG-06 : 95 % de la cible en 0,15 s

struct Entree {
    float kmh = 0, kmhMax = 260;      // vitesse (valeur absolue) et vitesse de pointe de la voiture
    float playerX = 0, edge = 1.6f;   // position latérale (−1..1 sur la route) et bord de la carte (map->edge, 1,6 par défaut)
    bool nitro = false, pluie = false;
    bool enCourse = false;            // RG-07 : course en jeu sur l'écran Course (sinon silence)
};
enum class Surface { Asphalte, Bord, HorsRoute };
struct Cibles {
    float vent = 0, ouverture = 0;    // volume du vent ; 0 = filtre à 400 Hz, 1 = filtre à 2 500 Hz
    float roulement = 0, bord = 0, herbe = 0, eau = 0;
    float cadenceBord = BORD_HZ_MIN;  // impulsions par seconde de la bande rugueuse
    Surface surface = Surface::Asphalte;
    float somme() const { return vent + roulement + bord + herbe + eau; }
};

// RG-03 / RG-04 : surface sous les roues (mêmes seuils que la secousse de ressenti.cpp et le freinage offRoad de
// player.cpp). Sur toutes les cartes edge > 1 (ville 1,7, plage 1,6, campagne 1,38, circuit 1,66) : entre 1 et edge,
// herbe / gravier ; à partir de edge (limite où la voiture secoue), bande rugueuse ; le bord est donc testé d'abord.
inline Surface surfaceDe(float playerX, float edge) {
    const float x = std::fabs(playerX);
    if (x >= edge) return Surface::Bord;
    if (x > 1) return Surface::HorsRoute;
    return Surface::Asphalte;
}

// cibles de volume de chaque règle (RG-01 à RG-05), avant le plafond de RG-08 et le lissage
inline Cibles ciblesBrutes(const Entree& e) {
    Cibles c;
    if (!e.enCourse) return c;   // RG-07 : silence hors course
    const float vmax = std::max(e.kmhMax, VENT_KMH_MIN + 1), kmh = std::max(0.f, e.kmh);
    const float v = std::clamp(kmh / vmax, 0.f, 1.f);
    // RG-01 : vent, en carré de la vitesse au-dessus de 40 km/h (continu : nul à 40 km/h, 0,30 à la pointe)
    if (kmh > VENT_KMH_MIN) {
        const float k = std::clamp((kmh * kmh - VENT_KMH_MIN * VENT_KMH_MIN) / (vmax * vmax - VENT_KMH_MIN * VENT_KMH_MIN), 0.f, 1.f);
        c.vent = VENT_MAX * k * (e.nitro ? VENT_NITRO : 1.f);
    }
    c.ouverture = std::clamp((kmh - OUVERTURE_KMH) / std::max(1.f, vmax - OUVERTURE_KMH), 0.f, 1.f);
    c.surface = surfaceDe(e.playerX, e.edge);
    const float roule = std::min(1.f, v * 3);   // bord et herbe : plein volume dès un tiers de la vitesse de pointe
    switch (c.surface) {
        case Surface::Asphalte: c.roulement = ROULEMENT_MAX * v; break;                                  // RG-02
        case Surface::Bord: if (v > BORD_V_MIN) c.bord = BORD_VOL * roule; else c.roulement = ROULEMENT_MAX * v; break;   // RG-03
        case Surface::HorsRoute: c.herbe = HERBE_VOL * roule; break;                                       // RG-04
    }
    c.cadenceBord = BORD_HZ_MIN + (BORD_HZ_MAX - BORD_HZ_MIN) * v;
    if (e.pluie && c.surface != Surface::HorsRoute) c.eau = EAU_MAX * v;                                  // RG-05
    return c;
}
// cibles jouées : RG-08, la somme ne dépasse jamais 0,45. À la pointe sur route sèche, vent 0,30 + roulement 0,18 = 0,48 :
// les deux sont réduits ensemble de 6 % (le plafond l'emporte sur les maxima de RG-01 / RG-02, signalé au PO)
inline Cibles cibles(const Entree& e) {
    Cibles c = ciblesBrutes(e);
    const float s = c.somme();
    if (s > SOMME_MAX) {   // RG-08 : jamais plus de 0,45 à eux tous (le moteur reste le son principal)
        const float k = SOMME_MAX / s;
        c.vent *= k; c.roulement *= k; c.bord *= k; c.herbe *= k; c.eau *= k;
    }
    return c;
}

// RG-06 : coefficient de lissage par échantillon (95 % de la cible en LISSAGE_S : trois constantes de temps)
inline float lissage(float rate) { return 1 - std::exp(-3.f / (LISSAGE_S * rate)); }

} // namespace wfs::route
