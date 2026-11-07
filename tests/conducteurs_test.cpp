// WFS-73 : conducteurs dans les voitures : mesure du pavillon (RG-01) et placement (RG-03, RG-07), sans GPU.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include "../src/world/conducteurs.h"
using namespace wfs;

static int echecs = 0;
static void cas(const std::string& quoi, bool ok) { std::printf("conducteurs : %-92s → %s\n", quoi.c_str(), ok ? "OK" : "ÉCHEC"); if (!ok) echecs++; }

// carrosserie simplifiée : boîte de 4,3 × 1,8 m, pavillon plat à « toit » au-dessus de l'habitacle, capot plus bas devant
static Model voiture(float toit) {
    Model m; m.ok = true; m.bmin = {-0.9f, 0, -2.15f}; m.bmax = {0.9f, toit, 2.15f};
    return m;
}
static std::vector<float> sommets(float toit) {
    std::vector<float> v;
    for (float x = -0.9f; x <= 0.9f; x += 0.05f)
        for (float z = -2.15f; z <= 2.15f; z += 0.05f) {
            const float y = std::fabs(z) < 0.9f ? toit : 0.9f;   // pavillon au-dessus de l'habitacle, capot et coffre à 0,9 m
            v.insert(v.end(), {x, y, z, x, 0.2f, z});           // dessus et plancher
        }
    v.insert(v.end(), {-0.36f, toit + 0.5f, 1.8f});   // antenne loin derrière la tête : hors de la colonne mesurée
    return v;
}

int main() {
    using namespace conducteurs;
    {   // RG-01 : pavillon mesuré au-dessus de la place (x −0,36, z −L × 0,02), pas le point le plus haut de la voiture
        Model m = voiture(1.45f); const std::vector<float> v = sommets(1.45f);
        mesurerSiege(m, v.data(), 3, v.size() / 3);
        cas("pavillon mesuré au-dessus du conducteur : 1,45 m (antenne arrière ignorée)", m.siege.w > 0.5f && std::fabs(m.siege.x - 1.45f) < 1e-4f);
        cas("place du conducteur : x = −0,36 ; z = −0,02 × L", std::fabs(m.siege.y + 0.36f) < 1e-4f && std::fabs(m.siege.z + 0.02f * 4.3f) < 1e-4f);
        Model vide = voiture(1.2f); const float loin[3] = {0.8f, 1.0f, 2.0f};
        mesurerSiege(vide, loin, 3, 1);
        cas("aucun sommet au-dessus de la place : non mesuré (pas de limite de pavillon)", vide.siege.w < 0.5f);
    }
    // RG-07 : échelle 1 (stature réelle) quand la tête tient sous le pavillon : assise respectée
    const float oeilRel = 0.589f, crane = 0.114f, k0 = 1.7562f / 1.5998f, cible = k0 * oeilRel;
    {
        const Placement P = placer(0.442f, cible, oeilRel, crane, 1.417f, true);   // Golf : assise 0,442, pavillon 1,417
        cas("Golf : échelle 1 gardée (" + std::to_string(P.echelle).substr(0, 5) + "), œil = assise + œil assis réel", std::fabs(P.echelle - k0) < 1e-4f
            && std::fabs(P.oeilY - (0.442f + cible)) < 1e-4f);
        cas("Golf : tête sous le pavillon (crâne + 3,5 cm ≤ pavillon)", P.oeilY + P.echelle * crane + GARDE_TOIT_M <= 1.417f + 1e-4f);
    }
    {   // RG-03 : voiture très basse : échelle réduite, jamais sous 0,86 ×, tête toujours sous le pavillon
        const Placement P = placer(0.25f, cible, oeilRel, crane, 0.95f, true);
        cas("voiture très basse : échelle réduite jusqu'à 0,86 × au plus", P.echelle < k0 && P.echelle >= ECHELLE_MIN * k0 - 1e-4f);
        cas("voiture très basse : tête sous le pavillon (l'œil descend si l'échelle ne suffit pas)", P.oeilY + P.echelle * crane + GARDE_TOIT_M <= 0.95f + 1e-4f);
        const Placement S = placer(0.25f, cible, oeilRel, crane, 0.95f, false);
        cas("pavillon non mesuré : échelle 1, rien n'est réduit", std::fabs(S.echelle - k0) < 1e-4f);
    }
    {   // deux hauteurs de voitures : le SUV assoit plus haut que la berline (assise du modèle)
        const Placement golf = placer(0.442f, cible, oeilRel, crane, 1.417f, true), suv = placer(0.794f, cible, oeilRel, crane, 1.80f, true);
        cas("SUV (assise 0,794) : œil plus haut que la Golf de la différence d'assise", std::fabs((suv.oeilY - golf.oeilY) - (0.794f - 0.442f)) < 1e-4f);
    }
    cas("coût (RG-05) : 50 m au plus, file de 24 voitures", DISTANCE_MAX_M == 50.f && FILE_MAX == 24);
    cas("RG-06 : 48 instances à squelette (8 piétons × 2 + 16 conducteurs × 2)", SKIN_MAX_INSTANCES >= 16 + 2 * 16);
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("conducteurs : tout est OK\n");
    return 0;
}
