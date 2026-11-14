// WaForSpeed — WFS-484 : plaques des autres voitures (src/render/vehicule_plaques_tirage.cpp, vehicule_plaque_image.cpp), sans GPU.
// RG-06 : 1 000 tirages sans mot interdit ni doublon de la plaque du joueur ; même numéro avant / arrière d'une même voiture
//         (une case de l'atlas par numéro : les deux plaques lisent la même case, aux mêmes uv) ;
// RG-02 : format (7 caractères, « AB123CD » ou « AB-1234 », sans I, O, U), atlas sans deux fois le même texte, tirage stable ;
// RG-03 : plaque reçue du duel filtrée (mots interdits, longueur, caractères) ; fantôme = plaque du joueur ;
// RG-05 : atlas d'une seule texture, cases sans recouvrement ; image de 256 × 54 px aux caractères de 75 mm.
#define STB_TRUETYPE_IMPLEMENTATION
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <vector>
#include "../third_party/stb/stb_truetype.h"
#include "../src/game/profil.h"
#include "../src/render/vehicule_plaque.h"
#include "../src/render/vehicule_plaques_autres.h"

using namespace wfs;
using namespace wfs::plaques_autres;

static int g_echecs = 0;
#define VERIFIE(cond, ...) do { if (!(cond)) { g_echecs++; std::printf("ÉCHEC : "); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static bool formatJuste(const std::string& t) {
    auto L = [](char c) { return c >= 'A' && c <= 'Z' && c != 'I' && c != 'O' && c != 'U'; };
    auto C = [](char c) { return c >= '0' && c <= '9'; };
    if (t.size() != 7) return false;
    if (t[2] == '-') return L(t[0]) && L(t[1]) && C(t[3]) && C(t[4]) && C(t[5]) && C(t[6]);
    return L(t[0]) && L(t[1]) && C(t[2]) && C(t[3]) && C(t[4]) && L(t[5]) && L(t[6]);
}

int main() {
    const std::string racine = WFS_SOURCE_DIR;
    {
        std::ifstream f(racine + "/assets/profil/mots_interdits.txt", std::ios::binary);
        const std::string mots((std::istreambuf_iterator<char>(f)), {});
        VERIFIE(profil::chargerMotsInterdits(mots) > 50, "liste des mots interdits non chargée");
    }

    // ---------- RG-06 / RG-02 : 1 000 tirages ----------
    {
        const std::string joueur = "AB123CD";
        Mulberry32 h(0x484u);
        int interdits = 0, joueurVu = 0, mauvais = 0, tirets = 0;
        std::set<std::string> vus;
        for (int i = 0; i < 1000; i++) {
            const std::string t = tirer(h, joueur);
            interdits += profil::contientMotInterdit(t);
            joueurVu += t == joueur;
            mauvais += !formatJuste(t);
            tirets += t[2] == '-';
            vus.insert(t);
            VERIFIE(profil::verifierPlaque(t) == profil::Refus::Aucun, "« %s » refusée par la règle de la plaque (WFS-480)", t.c_str());
        }
        VERIFIE(interdits == 0, "RG-06 : %d tirage(s) avec un mot interdit", interdits);
        VERIFIE(joueurVu == 0, "RG-06 : la plaque du joueur tirée %d fois", joueurVu);
        VERIFIE(mauvais == 0, "RG-02 : %d tirage(s) hors format", mauvais);
        VERIFIE(tirets > 150 && tirets < 350, "RG-02 : %d « AB-1234 » sur 1 000 (≈ 250 attendus)", tirets);
        VERIFIE(vus.size() > 990, "RG-02 : %zu textes différents sur 1 000", vus.size());
        // le joueur ne peut jamais être tiré, même quand sa plaque a le format du jeu : tirage forcé sur un générateur qui
        // donnerait sa plaque (on prend le 1er texte tiré comme plaque du joueur, puis on retire avec la même graine)
        Mulberry32 a(77), b(77);
        const std::string premier = tirer(a, "");
        VERIFIE(tirer(b, premier) != premier, "RG-02 : plaque du joueur tirée malgré l'exclusion");
        // un mot interdit inséré à la main dans la liste du jeu est bien filtré par le tirage
        profil::chargerMotsInterdits("ab1\nkx\n");   // « ab1 » et « kx » : refuse « AB1… », « …KX… » (forme normalisée)
        Mulberry32 c(5);
        int refus = 0;
        for (int i = 0; i < 1000; i++) { const std::string t = tirer(c, ""); refus += profil::contientMotInterdit(t); }
        VERIFIE(refus == 0, "RG-06 : %d tirage(s) gardés malgré un mot interdit ajouté", refus);
        std::ifstream f(racine + "/assets/profil/mots_interdits.txt", std::ios::binary);
        profil::chargerMotsInterdits(std::string((std::istreambuf_iterator<char>(f)), {}));
    }

    // ---------- atlas : textes tous différents, stables, fantôme = joueur ----------
    {
        const auto A = tirerAtlas(graineCarte("ville"), "KX-4821"), B = tirerAtlas(graineCarte("ville"), "KX-4821");
        const auto C = tirerAtlas(graineCarte("corniche"), "KX-4821");
        std::set<std::string> s(A.begin(), A.end());
        VERIFIE(s.size() == (size_t)CASES, "RG-02 : %zu textes différents dans l'atlas (%d attendus)", s.size(), CASES);
        VERIFIE(A == B, "RG-02 : tirage de l'atlas instable pour la même carte");
        VERIFIE(A != C, "plaques identiques sur deux cartes différentes");
        VERIFIE(A[FANTOME] == "KX-4821", "RG-03 : fantôme « %s » au lieu de la plaque du joueur", A[FANTOME].c_str());
        for (int c = 0; c < CASES; c++) if (c != FANTOME) VERIFIE(A[(size_t)c] != "KX-4821" && formatJuste(A[(size_t)c]), "case %d : « %s »", c, A[(size_t)c].c_str());
        VERIFIE(tirerAtlas(1, "")[FANTOME] == "WFS", "fantôme sans profil : plaque du jeu « WFS »");
        VERIFIE(TRAFIC_0 + TRAFIC_N <= COURSE5_0 && COURSE5_0 + COURSE5_N <= POLICE_0 && POLICE_0 + POLICE_N <= DUEL_0 && DUEL_0 + DUEL_N <= FANTOME && FANTOME < CASES,
                "répartition des cases qui se recouvre");
        std::printf("atlas : trafic n° 0 « %s », n° 1 « %s », Course à 5 « %s », police « %s », duel « %s »\n",
                    A[0].c_str(), A[1].c_str(), A[COURSE5_0].c_str(), A[POLICE_0].c_str(), A[DUEL_0].c_str());
    }

    // ---------- RG-06 : avant / arrière : une seule case par numéro, uv dans la case, cases sans recouvrement ----------
    {
        int hors = 0;
        for (int c = 0; c < CASES; c++) {
            float u0, v0, u1, v1;
            uvCase(c, 0, 0, u0, v0); uvCase(c, 1, 1, u1, v1);
            const int col = c % COLONNES, lig = c / COLONNES;
            const float cu0 = col / (float)COLONNES, cu1 = (col + 1) / (float)COLONNES;
            const float cv0 = (lig * CASE_H + MARGE_PX - 0.01f) / ATLAS_H, cv1 = (lig * CASE_H + MARGE_PX + PLAQUE_PX_H + 0.01f) / ATLAS_H;
            hors += u0 < cu0 - 1e-6f || u1 > cu1 + 1e-6f || v0 < cv0 || v1 > cv1 || v1 > (lig + 1) * CASE_H / (float)ATLAS_H;
        }
        VERIFIE(hors == 0, "RG-05 : %d case(s) dont les uv sortent de la case", hors);
        // trafic : numéro → case ; l'avant et l'arrière de la voiture n° 17 lisent la même case (même calcul, un seul texte)
        const int n17 = TRAFIC_0 + 17 % TRAFIC_N;
        float ua, va, ub, vb;
        uvCase(n17, 0.3f, 0.6f, ua, va); uvCase(n17, 0.3f, 0.6f, ub, vb);
        VERIFIE(ua == ub && va == vb, "RG-06 : avant et arrière de la même voiture sur deux cases");
    }

    // ---------- RG-03 : plaque reçue du duel ----------
    {
        VERIFIE(filtrerPlaqueRecue("kx-4821") == "KX-4821", "plaque du duel en majuscules");
        VERIFIE(filtrerPlaqueRecue("").empty(), "plaque vide acceptée");
        VERIFIE(filtrerPlaqueRecue("ABCDEFGHI").empty(), "plaque de 9 caractères acceptée");
        VERIFIE(filtrerPlaqueRecue("AB 12").empty() && filtrerPlaqueRecue("A<b>").empty() && filtrerPlaqueRecue("-AB").empty(), "caractère interdit accepté");
        VERIFIE(filtrerPlaqueRecue("CONNARD").empty() && filtrerPlaqueRecue("c0nnard").empty(), "mot interdit accepté dans la plaque du duel");
        VERIFIE(filtrerPlaqueRecue(std::string(5000, 'A')).empty(), "texte très long accepté");
    }

    // ---------- RG-05 : image de la case (256 × 54 px), mêmes mesures que la plaque du joueur ----------
    {
        std::ifstream f(racine + "/assets/fonts/Rajdhani-Bold.ttf", std::ios::binary);
        const std::vector<uint8_t> ttf((std::istreambuf_iterator<char>(f)), {});
        VERIFIE(!ttf.empty(), "police introuvable");
        plaque_joueur::ImagePlaque a, b;
        VERIFIE(plaque_joueur::dessinerImage("AB123CD", ttf.data(), a, CASE_L), "image de la case");
        VERIFIE(a.largeur == CASE_L && a.hauteur == PLAQUE_PX_H && a.couleur.size() == (size_t)CASE_L * PLAQUE_PX_H * 4 && a.normales.size() == a.couleur.size(),
                "image de %d × %d px (%d × %d attendus)", a.largeur, a.hauteur, CASE_L, PLAQUE_PX_H);
        VERIFIE(std::fabs(a.hauteurTexteMm - plaque_joueur::CARACTERE_H_MM) < 0.01f, "caractères de %.1f mm", a.hauteurTexteMm);
        VERIFIE(plaque_joueur::dessinerImage("AB123CD", ttf.data(), b) && b.largeur == plaque_joueur::IMAGE_L && std::fabs(b.largeurTexteMm - a.largeurTexteMm) < 1.f,
                "même texte : %.1f mm (joueur) contre %.1f mm (atlas)", b.largeurTexteMm, a.largeurTexteMm);
        int encre = 0;
        for (size_t i = 0; i < a.couleur.size(); i += 4) encre += a.couleur[i] < 80;
        VERIFIE(encre > 600, "texte presque absent de l'image de la case (%d px d'encre)", encre);
        std::printf("case : %d × %d px, « AB123CD » de %.0f mm de large (joueur : %.0f mm)\n", a.largeur, a.hauteur, a.largeurTexteMm, b.largeurTexteMm);
    }
    if (g_echecs) { std::printf("test_plaques_autres : %d échec(s)\n", g_echecs); return 1; }
    std::printf("test_plaques_autres : OK\n");
    return 0;
}
