// WaForSpeed — WFS-484 (RG-02, RG-03, RG-05) : textes des plaques des autres voitures et place des cases dans l'atlas, sur le
// processeur (sans Vulkan, tests/test_plaques_autres.cpp).
#include <algorithm>
#include <cctype>
#include "../game/profil.h"
#include "vehicule_plaques_autres.h"

namespace wfs::plaques_autres {
namespace {

// lettres sans I, O, U (confondues avec 1, 0, V : les plaques réelles françaises les excluent aussi), chiffres
constexpr char LETTRES[] = "ABCDEFGHJKLMNPQRSTVWXYZ";
constexpr int NB_LETTRES = sizeof(LETTRES) - 1;

char lettre(Mulberry32& h) { return LETTRES[std::min(NB_LETTRES - 1, (int)(h.next() * NB_LETTRES))]; }
char chiffre(Mulberry32& h) { return (char)('0' + std::min(9, (int)(h.next() * 10))); }

std::string majuscules(std::string s) {
    for (char& c : s) c = (char)std::toupper((unsigned char)c);
    return s;
}

} // namespace

std::string tirer(Mulberry32& h, const std::string& plaqueJoueur) {
    const std::string joueur = majuscules(plaqueJoueur);
    for (int essai = 0; essai < 1000; essai++) {
        std::string t;
        if (h.next() < 0.25) {   // « AB-1234 »
            t = {lettre(h), lettre(h), '-', chiffre(h), chiffre(h), chiffre(h), chiffre(h)};
        } else {                 // « AB123CD » (format AB-123-CD sans tirets : 8 caractères au plus)
            t = {lettre(h), lettre(h), chiffre(h), chiffre(h), chiffre(h), lettre(h), lettre(h)};
        }
        if (t == joueur || profil::contientMotInterdit(t)) continue;
        return t;
    }
    return "WF-0000";   // jamais atteint en pratique (≈ 1 tirage sur 10⁴ refusé)
}

std::array<std::string, CASES> tirerAtlas(uint32_t graine, const std::string& plaqueJoueur) {
    std::array<std::string, CASES> t;
    Mulberry32 h(graine ? graine : 0x484u);
    for (int c = 0; c < CASES; c++) {
        if (c == FANTOME) continue;
        std::string s;
        do s = tirer(h, plaqueJoueur);
        while (std::find(t.begin(), t.begin() + c, s) != t.begin() + c);   // jamais deux fois le même texte
        t[(size_t)c] = s;
    }
    t[FANTOME] = plaqueJoueur.empty() ? std::string("WFS") : majuscules(plaqueJoueur);
    return t;
}

uint32_t graineCarte(const char* id) {
    uint32_t x = 2166136261u;
    for (const char* p = id ? id : ""; *p; p++) { x ^= (uint8_t)*p; x *= 16777619u; }
    return x ^ 0x484u;
}

std::string filtrerPlaqueRecue(const std::string& brut) {
    if (brut.size() > 16) return {};
    std::string propre;
    return profil::verifierPlaque(brut, &propre) == profil::Refus::Aucun ? propre : std::string();
}

void uvCase(int cas, float u, float v, float& ua, float& va) {
    cas = std::clamp(cas, 0, CASES - 1);
    const int col = cas % COLONNES, lig = cas / COLONNES;
    ua = (col * CASE_L + u * CASE_L) / (float)ATLAS_L;
    va = (lig * CASE_H + MARGE_PX + v * PLAQUE_PX_H) / (float)ATLAS_H;
}

} // namespace wfs::plaques_autres
