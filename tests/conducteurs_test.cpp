// WFS-73 : conducteurs dans les voitures : mesure du pavillon (RG-01) et placement (RG-03, RG-07), sans GPU.
#include <algorithm>
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
    // ---------------------------------------------------------------- WFS-211
    {   // RG-03 (M-03) : albédo de la peau dans 0,05-0,65 (charte du 2026-11-07, WFS-210 : peau VI mesurée) pour les 12 morphotypes
        bool ok = true; float lmin = 1, lmax = 0;
        for (int i = 0; i < NB_HUMAINS; i++) {
            const vec3 a = albedoPeau(i); const float l = luminance(a);
            lmin = std::min(lmin, l); lmax = std::max(lmax, l);
            ok = ok && l >= 0.05f && l <= 0.65f && a.x > a.y && a.y > a.z;   // teinte chaude (rouge > vert > bleu)
        }
        cas("M-03 : albédo de la peau des 12 morphotypes dans 0,05-0,65 (" + std::to_string(lmin).substr(0, 5) + " à " + std::to_string(lmax).substr(0, 5) + ")", ok);
        cas("M-03 : teints variés (écart de luminance ≥ 0,25)", lmax - lmin >= 0.25f);
    }
    {   // RG-02 : voitures voisines jamais avec le même modèle, modèle stable par voiture
        Attribution A; int m[FILE_MAX]; bool distincts = true;
        for (int i = 0; i < NB_HUMAINS; i++) m[i] = A.choisir((uint32_t)i + 1);
        for (int i = 0; i < NB_HUMAINS; i++) for (int j = 0; j < i; j++) distincts = distincts && m[i] != m[j];
        cas("12 voitures dans la file : 12 modèles différents", distincts);
        A.finImage();
        bool stable = true;
        for (int i = NB_HUMAINS - 1; i >= 0; i--) stable = stable && A.choisir((uint32_t)i + 1) == m[i];   // autre ordre : même modèle
        cas("image suivante, autre ordre : chaque voiture garde son modèle", stable);
        A.finImage();
        for (int i = 1; i < NB_HUMAINS; i++) A.choisir((uint32_t)i + 1);   // la voiture 1 sort de la file
        A.finImage();
        const int nouveau = A.choisir(1000);
        bool libre = true; for (int i = 1; i < NB_HUMAINS; i++) libre = libre && nouveau != m[i];
        cas("une voiture qui arrive prend le modèle laissé libre (pas celui d'une voisine)", libre);
        int memes = 0;   // trafic : graines consécutives (indice de voiture + 2) : modèles préférés déjà variés
        for (uint32_t g = 2; g < 26; g++) memes += Attribution::prefere(g) == Attribution::prefere(g + 1);
        cas("modèles préférés de deux graines voisines rarement égaux (" + std::to_string(memes) + " / 24)", memes <= 4);
    }
    {   // RG-01 : hystérésis : autour de 8 m, le niveau ne bascule qu'à ± 0,5 m du seuil
        Attribution A; A.choisir(7);
        const int a = A.niveau(7, 7.9f * 7.9f), b = A.niveau(7, 8.2f * 8.2f), c = A.niveau(7, 7.8f * 7.8f), d = A.niveau(7, 8.6f * 8.6f), e = A.niveau(7, 8.3f * 8.3f);
        cas("hystérésis : 7,9 → 8,2 → 7,8 m gardent corps-0 ; 8,6 m passe en corps-1 ; 8,3 m le garde", a == 0 && b == 0 && c == 0 && d == 1 && e == 1);
        cas("hystérésis : saut direct de 5 à 30 m : corps-2 sans attendre", A.niveau(7, 5.f * 5.f) == 0 && A.niveau(7, 30.f * 30.f) == 2);
    }
    cas("RG-01 : niveaux de détail : corps-0 en vue Proche (6 m), corps-1 à 12 m, corps-2 au-delà de 20 m",
        niveauDetail(36.f) == 0 && niveauDetail(144.f) == 1 && niveauDetail(30.f * 30.f) == 2);
    // WFS-317 (RG-02) : Basse : 3e seuil à 30 m (corps-3 « très simple ») ; Moyenne / Haute : corps-2 jusqu'au bout de la file
    cas("WFS-317 : Basse : corps-0 à 6 m, corps-1 à 12 m, corps-2 à 25 m, corps-3 à 35 et 50 m",
        niveauDetail(36.f, -1, true) == 0 && niveauDetail(144.f, -1, true) == 1 && niveauDetail(25.f * 25.f, -1, true) == 2 &&
        niveauDetail(35.f * 35.f, -1, true) == 3 && niveauDetail(50.f * 50.f, -1, true) == 3);
    cas("WFS-317 : Haute : pas de corps-3, même à 50 m", niveauDetail(35.f * 35.f) == 2 && niveauDetail(50.f * 50.f) == 2);
    {   // hystérésis autour de 30 m en Basse ; retour en Haute depuis corps-3 : corps-2 tout de suite
        Attribution A; A.choisir(9);
        const int a = A.niveau(9, 29.8f * 29.8f, true), b = A.niveau(9, 30.3f * 30.3f, true), c = A.niveau(9, 30.6f * 30.6f, true), d = A.niveau(9, 29.7f * 29.7f, true);
        cas("WFS-317 : hystérésis à 30 m : 29,8 → 30,3 m gardent corps-2 ; 30,6 m passe en corps-3 ; 29,7 m le garde", a == 2 && b == 2 && c == 3 && d == 3);
        cas("WFS-317 : passage en Haute depuis corps-3 : corps-2", A.niveau(9, 31.f * 31.f, false) == 2);
    }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("conducteurs : tout est OK\n");
    return 0;
}
