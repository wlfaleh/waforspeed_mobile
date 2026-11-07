// WFS-210 : tenues des personnages selon le lieu (RG-01), albédo de la peau (RG-02, phototype VI mesuré).
// Vérifie sur le CPU : tenue déterministe par graine ; tissus du lieu (Ville : laine / coton / jean, Corniche : lin / coton,
// Campagne : jean / coton, Grand Prix : combinaisons synthétiques et maillots) ; coupes d'été à la Corniche ; couleurs de
// tissus réels (luminance linéaire 0,03-0,75 : aucun noir ni blanc purs, avis expert-realisme-3d) ; peau des 12 morphotypes
// dans la plage de la charte 0,05-0,65 avec la peau VI publiée (PB-MAT, 0,09 ; 0,05 ; 0,02).
// Compilé dans test_fidelite_humains (tests/fidelite_humains.cpp : #include) ; seul : g++ … -DWFS_TENUES_SEUL
#include <cmath>
#include <cstdio>
#include <cstring>
#include "../src/world/conducteurs.h"
#include "../src/world/pietons.h"
namespace tenues_test {
using namespace wfs;

static int echecs = 0;
static void cas(const char* quoi, bool ok) { std::printf("tenues : %-92s → %s\n", quoi, ok ? "OK" : "ÉCHEC"); echecs += !ok; }
static float lin(float c) { return std::pow(c, 2.2f); }
static float lumi(vec3 c) { return 0.2126f * lin(c.x) + 0.7152f * lin(c.y) + 0.0722f * lin(c.z); }

int executer() {
    const char* cartes[4] = {"ville", "plage", "campagne", "circuit"};
    int tissus[4][tenue::NB_TISSUS] = {}, courtes[4] = {}, combis[4] = {};
    bool couleurs = true, deterministe = true, peauGardee = true;
    for (int c = 0; c < 4; c++)
        for (uint32_t g = 0; g < 400; g++) {
            ApparencePersonnage a; a.peau = {0.4f, 0.3f, 0.2f}; a.cheveux = {0.1f, 0.1f, 0.1f};
            tenue::tenueLieu(cartes[c], g, g % 2, a);
            ApparencePersonnage b; tenue::tenueLieu(cartes[c], g, g % 2, b);
            deterministe = deterministe && a.haut.x == b.haut.x && a.bas.y == b.bas.y && a.vetement.w == b.vetement.w;
            peauGardee = peauGardee && a.peau.x == 0.4f && a.cheveux.x == 0.1f && a.vetement.z == 1.f;
            const int t = (int)a.vetement.w;
            if (t > 0 && t < tenue::NB_TISSUS) tissus[c][t]++;
            courtes[c] += a.vetement.x < 1.f || a.vetement.y < 1.f;
            combis[c] += a.haut.x == a.bas.x && a.haut.y == a.bas.y && a.haut.z == a.bas.z;
            for (vec3 v : {a.haut, a.bas, a.chaussures}) { const float l = lumi(v); couleurs = couleurs && l >= 0.03f && l <= 0.75f; }
        }
    std::printf("  tissus (coton, laine, jean, lin, synthétique) : ville %d/%d/%d/%d/%d, corniche %d/%d/%d/%d/%d, campagne %d/%d/%d/%d/%d, Grand Prix %d/%d/%d/%d/%d\n",
                tissus[0][1], tissus[0][2], tissus[0][3], tissus[0][4], tissus[0][5], tissus[1][1], tissus[1][2], tissus[1][3], tissus[1][4], tissus[1][5],
                tissus[2][1], tissus[2][2], tissus[2][3], tissus[2][4], tissus[2][5], tissus[3][1], tissus[3][2], tissus[3][3], tissus[3][4], tissus[3][5]);
    std::printf("  manches ou jambes courtes : ville %d, corniche %d, campagne %d, Grand Prix %d (sur 400) ; une pièce (robe, costume, combinaison) : %d, %d, %d, %d\n",
                courtes[0], courtes[1], courtes[2], courtes[3], combis[0], combis[1], combis[2], combis[3]);
    cas("tenue déterministe pour une graine ; peau, cheveux gardés ; habillé", deterministe && peauGardee);
    cas("Ville : vestes et costumes en laine (≥ 25 %), robes et costumes d'une pièce", tissus[0][tenue::TISSU_LAINE] >= 100 && combis[0] >= 80 && tissus[0][tenue::TISSU_SYNTHETIQUE] == 0);
    cas("Corniche : lin et coton seulement, tenues d'été (≥ 60 % de manches ou jambes courtes)", tissus[1][tenue::TISSU_LIN] >= 120 && tissus[1][tenue::TISSU_LAINE] == 0 && tissus[1][tenue::TISSU_JEAN] == 0 && courtes[1] >= 240);
    cas("Campagne : tenues de travail (jean ou coton, bleus de travail d'une pièce)", tissus[2][tenue::TISSU_JEAN] >= 120 && tissus[2][tenue::TISSU_LAINE] == 0 && tissus[2][tenue::TISSU_LIN] == 0 && combis[2] >= 60);
    cas("Grand Prix : combinaisons synthétiques (≥ 35 %) et maillots de supporters", tissus[3][tenue::TISSU_SYNTHETIQUE] >= 140 && tissus[3][tenue::TISSU_COTON] >= 140);
    cas("couleurs de tissus réels : luminance linéaire 0,03-0,75 (ni noir ni blanc purs)", couleurs);
    {   // RG-02 : peau des 12 morphotypes dans la charte (0,05-0,65), peau VI publiée pour le morphotype d'origine africaine
        float lmin = 1, lmax = 0; bool ok = true;
        for (int i = 0; i < conducteurs::NB_HUMAINS; i++) {
            const vec3 a = conducteurs::albedoPeau(i); const float l = conducteurs::luminance(a);
            lmin = std::min(lmin, l); lmax = std::max(lmax, l); ok = ok && l >= 0.05f && l <= 0.65f && a.x > a.y && a.y > a.z;
        }
        const vec3 vi = conducteurs::albedoPeau(1);   // h02 : origine africaine (ORIGINES)
        std::printf("  peau : luminance %.3f à %.3f ; morphotype h02 = (%.3f ; %.3f ; %.3f)\n", lmin, lmax, vi.x, vi.y, vi.z);
        cas("peau des 12 morphotypes dans 0,05-0,65, teinte chaude", ok);
        cas("peau VI mesurée (PB-MAT mat.peau_vi.albedo 0,09 ; 0,05 ; 0,02, ± 10 %)", std::fabs(vi.x - 0.09f) <= 0.009f && std::fabs(vi.y - 0.05f) <= 0.005f && std::fabs(vi.z - 0.02f) <= 0.002f);
    }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("tenues : tout est OK\n");
    return 0;
}
} // namespace tenues_test

#ifdef WFS_TENUES_SEUL
int main() { return tenues_test::executer(); }
#endif
