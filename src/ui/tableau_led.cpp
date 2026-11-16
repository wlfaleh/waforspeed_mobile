// WaForSpeed — WFS-561 : tableau LED des résultats, partie calcul (voir tableau_led.h).
// Police : matrice 5 × 7 des afficheurs à points (dessin des caractères ASCII du contrôleur HD44780 de Hitachi, la référence
// des afficheurs à matrice de points), redessinée ici point par point ; accents sur les 2 rangées du dessus.
#include "tableau_led.h"
#include <algorithm>
#include <cstring>

namespace wfs::led {
namespace {

// '#' = point allumé ; 7 rangées de 5 colonnes (capitales, chiffres, signes)
struct Dessin { char32_t c; const char* r[7]; };
constexpr Dessin POLICE[] = {
    {U'A', {" ### ", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"}},
    {U'B', {"#### ", "#   #", "#   #", "#### ", "#   #", "#   #", "#### "}},
    {U'C', {" ### ", "#   #", "#    ", "#    ", "#    ", "#   #", " ### "}},
    {U'D', {"###  ", "#  # ", "#   #", "#   #", "#   #", "#  # ", "###  "}},
    {U'E', {"#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#####"}},
    {U'F', {"#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#    "}},
    {U'G', {" ### ", "#   #", "#    ", "# ###", "#   #", "#   #", " ####"}},
    {U'H', {"#   #", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"}},
    {U'I', {" ### ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### "}},
    {U'J', {"  ###", "   # ", "   # ", "   # ", "   # ", "#  # ", " ##  "}},
    {U'K', {"#   #", "#  # ", "# #  ", "##   ", "# #  ", "#  # ", "#   #"}},
    {U'L', {"#    ", "#    ", "#    ", "#    ", "#    ", "#    ", "#####"}},
    {U'M', {"#   #", "## ##", "# # #", "# # #", "#   #", "#   #", "#   #"}},
    {U'N', {"#   #", "#   #", "##  #", "# # #", "#  ##", "#   #", "#   #"}},
    {U'O', {" ### ", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "}},
    {U'P', {"#### ", "#   #", "#   #", "#### ", "#    ", "#    ", "#    "}},
    {U'Q', {" ### ", "#   #", "#   #", "#   #", "# # #", "#  # ", " ## #"}},
    {U'R', {"#### ", "#   #", "#   #", "#### ", "# #  ", "#  # ", "#   #"}},
    {U'S', {" ####", "#    ", "#    ", " ### ", "    #", "    #", "#### "}},
    {U'T', {"#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  "}},
    {U'U', {"#   #", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "}},
    {U'V', {"#   #", "#   #", "#   #", "#   #", "#   #", " # # ", "  #  "}},
    {U'W', {"#   #", "#   #", "#   #", "# # #", "# # #", "# # #", " # # "}},
    {U'X', {"#   #", "#   #", " # # ", "  #  ", " # # ", "#   #", "#   #"}},
    {U'Y', {"#   #", "#   #", "#   #", " # # ", "  #  ", "  #  ", "  #  "}},
    {U'Z', {"#####", "    #", "   # ", "  #  ", " #   ", "#    ", "#####"}},
    {U'0', {" ### ", "#   #", "#  ##", "# # #", "##  #", "#   #", " ### "}},
    {U'1', {"  #  ", " ##  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### "}},
    {U'2', {" ### ", "#   #", "    #", "   # ", "  #  ", " #   ", "#####"}},
    {U'3', {"#####", "   # ", "  #  ", "   # ", "    #", "#   #", " ### "}},
    {U'4', {"   # ", "  ## ", " # # ", "#  # ", "#####", "   # ", "   # "}},
    {U'5', {"#####", "#    ", "#### ", "    #", "    #", "#   #", " ### "}},
    {U'6', {"  ## ", " #   ", "#    ", "#### ", "#   #", "#   #", " ### "}},
    {U'7', {"#####", "    #", "   # ", "  #  ", " #   ", " #   ", " #   "}},
    {U'8', {" ### ", "#   #", "#   #", " ### ", "#   #", "#   #", " ### "}},
    {U'9', {" ### ", "#   #", "#   #", " ####", "    #", "   # ", " ##  "}},
    {U' ', {"     ", "     ", "     ", "     ", "     ", "     ", "     "}},
    {U':', {"     ", " ##  ", " ##  ", "     ", " ##  ", " ##  ", "     "}},
    {U',', {"     ", "     ", "     ", "     ", "     ", " ##  ", " #   "}},   // virgule décimale (2:05,4) : plus basse qu'un point-virgule
    {U'.', {"     ", "     ", "     ", "     ", "     ", " ##  ", " ##  "}},
    {U'+', {"     ", "  #  ", "  #  ", "#####", "  #  ", "  #  ", "     "}},
    {U'-', {"     ", "     ", "     ", "#####", "     ", "     ", "     "}},
    {U'—', {"     ", "     ", "     ", "#####", "     ", "     ", "     "}},   // tiret cadratin : celui du 1er (pas d'écart)
    {U'·', {"     ", "     ", "     ", "  #  ", "     ", "     ", "     "}},   // point médian : « TOI · LUME »
    {U'/', {"     ", "    #", "   # ", "  #  ", " #   ", "#    ", "     "}},
    {U'\'', {" ##  ", "  #  ", " #   ", "     ", "     ", "     ", "     "}},
    {U'!', {"  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "     ", "  #  "}},
    {U'?', {" ### ", "#   #", "    #", "   # ", "  #  ", "     ", "  #  "}},
    {U'(', {"   # ", "  #  ", " #   ", " #   ", " #   ", "  #  ", "   # "}},
    {U')', {" #   ", "  #  ", "   # ", "   # ", "   # ", "  #  ", " #   "}},
    {U'#', {" # # ", " # # ", "#####", " # # ", "#####", " # # ", " # # "}},
    {U'%', {"##   ", "##  #", "   # ", "  #  ", " #   ", "#  ##", "   ##"}},
    {U'&', {" ##  ", "#  # ", "# #  ", " #   ", "# # #", "#  # ", " ## #"}},
    {U'_', {"     ", "     ", "     ", "     ", "     ", "     ", "#####"}},
    {U'★', {"  #  ", "  #  ", "#####", " ### ", " ### ", "## ##", "#   #"}},
};

// accents (2 rangées au-dessus de la capitale) : lettre de base et dessin
struct Accent { char32_t c, base; const char* r[2]; };
constexpr Accent ACCENTS[] = {
    {U'É', U'E', {"   # ", "  #  "}}, {U'È', U'E', {" #   ", "  #  "}}, {U'Ê', U'E', {"  #  ", " # # "}}, {U'Ë', U'E', {" # # ", "     "}},
    {U'À', U'A', {" #   ", "  #  "}}, {U'Â', U'A', {"  #  ", " # # "}}, {U'Ä', U'A', {" # # ", "     "}},
    {U'Î', U'I', {"  #  ", " # # "}}, {U'Ï', U'I', {" # # ", "     "}},
    {U'Ô', U'O', {"  #  ", " # # "}}, {U'Ö', U'O', {" # # ", "     "}},
    {U'Ù', U'U', {" #   ", "  #  "}}, {U'Û', U'U', {"  #  ", " # # "}}, {U'Ü', U'U', {" # # ", "     "}},
    {U'Ÿ', U'Y', {" # # ", "     "}}, {U'Ñ', U'N', {" ## #", "#  # "}},
};

uint8_t bits(const char* s) {
    uint8_t b = 0;
    for (int k = 0; k < 5 && s[k]; k++) if (s[k] == '#') b |= (uint8_t)(1u << (4 - k));
    return b;
}

const Dessin* dessin(char32_t c) {
    for (const Dessin& d : POLICE) if (d.c == c) return &d;
    return nullptr;
}

} // namespace

char32_t majuscule(char32_t c) {
    if (c >= U'a' && c <= U'z') return c - 32;
    if ((c >= 0xE0 && c <= 0xFE && c != 0xF7)) return c - 32;   // à → À … þ → Þ (Latin-1), sauf ÷
    if (c == 0xFF) return 0x178;                                // ÿ → Ÿ
    if (c == 0x153) return 0x152;                               // œ → Œ
    return c;
}

bool glyphe(char32_t c, uint8_t r[9]) {
    std::memset(r, 0, 9);
    if (c == U'Ç') {   // C à cédille : la cédille sous la 7e rangée ne tient pas, elle prend la place de l'espace du bas (rangée 9)
        const Dessin* d = dessin(U'C');
        for (int k = 0; k < 7; k++) r[2 + k] = bits(d->r[k]);
        return true;
    }
    for (const Accent& a : ACCENTS)
        if (a.c == c) {
            const Dessin* d = dessin(a.base);
            r[0] = bits(a.r[0]); r[1] = bits(a.r[1]);
            for (int k = 0; k < 7; k++) r[2 + k] = bits(d->r[k]);
            return true;
        }
    const Dessin* d = dessin(c);
    if (!d) return false;
    for (int k = 0; k < 7; k++) r[2 + k] = bits(d->r[k]);
    return true;
}

std::vector<char32_t> pointsDeCode(std::string_view s) {
    std::vector<char32_t> out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size();) {
        const unsigned char b = (unsigned char)s[i];
        int n = b < 0x80 ? 1 : (b >> 5) == 6 ? 2 : (b >> 4) == 14 ? 3 : (b >> 3) == 30 ? 4 : 0;
        if (n == 0 || i + (size_t)n > s.size()) { i++; continue; }   // octet invalide : sauté
        char32_t c = n == 1 ? b : n == 2 ? (b & 0x1F) : n == 3 ? (b & 0x0F) : (b & 0x07);
        bool ok = true;
        for (int k = 1; k < n; k++) {
            const unsigned char s2 = (unsigned char)s[i + (size_t)k];
            if ((s2 >> 6) != 2) { ok = false; break; }
            c = (c << 6) | (s2 & 0x3F);
        }
        if (ok) out.push_back(c);
        i += ok ? (size_t)n : 1;
    }
    return out;
}

int poserTexte(std::string_view utf8, int debut, int largeur, int ligne, std::vector<Point>& sortie) {
    if (ligne < 0 || ligne >= NB_LIGNES || debut < 0 || largeur <= 0) return 0;
    const int fin = std::min(NB_CAR, debut + largeur);
    const int r0 = rangeeLigne(ligne);
    int col = debut, poses = 0;
    for (char32_t c : pointsDeCode(utf8)) {
        if (col >= fin) break;
        uint8_t g[9];
        const char32_t m = majuscule(c);
        if (!glyphe(m, g)) {
            // sans glyphe : lettre sans accent si c'en est une (ã → A…), sinon le caractère est sauté
            char32_t base = 0;
            if (m >= 0xC0 && m <= 0xC5) base = U'A'; else if (m >= 0xC8 && m <= 0xCB) base = U'E';
            else if (m >= 0xCC && m <= 0xCF) base = U'I'; else if (m >= 0xD2 && m <= 0xD6) base = U'O';
            else if (m >= 0xD9 && m <= 0xDC) base = U'U';
            if (!base || !glyphe(base, g)) continue;
        }
        for (int rr = 0; rr < 9; rr++)
            for (int k = 0; k < 5; k++)
                if (g[rr] & (1u << (4 - k))) sortie.push_back({(int16_t)(MARGE_G + col * CAR_L + k), (int16_t)(r0 + rr)});
        col++; poses++;
    }
    return poses;
}

void composer(const std::string (&entete)[4], const std::vector<Ligne>& lignes, std::vector<Point>& sortie) {
    sortie.clear();
    const Colonne cols[4] = {COL_POS, COL_PILOTE, COL_TEMPS, COL_ECART};
    for (int k = 0; k < 4; k++) poserTexte(entete[k], cols[k].debut, cols[k].largeur, 0, sortie);
    int l = 1;
    for (const Ligne& li : lignes) {
        if (l >= NB_LIGNES) break;
        if (li.pos.empty() && li.pilote.empty() && li.temps.empty() && li.ecart.empty()) continue;   // ligne vide cachée
        poserTexte(li.pos, COL_POS.debut, COL_POS.largeur, l, sortie);
        poserTexte(li.pilote, COL_PILOTE.debut, COL_PILOTE.largeur, l, sortie);
        poserTexte(li.temps, COL_TEMPS.debut, COL_TEMPS.largeur, l, sortie);
        poserTexte(li.ecart, COL_ECART.debut, COL_ECART.largeur, l, sortie);
        l++;
    }
}

} // namespace wfs::led
