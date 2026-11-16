// WFS-561 : tableau LED des résultats (src/ui/tableau_led.h) et placement de la scène (src/ui/resultats_parking.h), calcul seul.
//   - police : chaque caractère des textes du tableau (FR / EN, noms des pilotes, temps, écarts) a son glyphe ; accents en haut ;
//   - mise en page : tout point dans la matrice 186 × 88, colonnes sans chevauchement, 6 lignes au plus, lignes vides cachées,
//     texte trop long coupé ;
//   - RG-02 : taille du panneau ≈ 3 × 1,5 m (±5 %), pas de 16 mm ;
//   - cadrage : le panneau entier dans l'image, dans la partie gauche-centre, la voiture à sa droite (maquette 11), en 2400 × 1080
//     et en 1280 × 720 ;
//   - reflet : l'image miroir d'un point est sur le sol, entre le mur et l'œil, et alignée avec l'œil et le point miroir.
#include <cmath>
#include <cstdio>
#include <string>
#include "../src/ui/resultats_parking.h"
#include "../src/ui/tableau_led.h"
using namespace wfs;

static int echecs = 0;
static void cas(const std::string& quoi, bool ok) { std::printf("tableau LED : %-100s → %s\n", quoi.c_str(), ok ? "OK" : "ÉCHEC"); if (!ok) echecs++; }

// projection d'un point du repère du parking dans l'image (NDC, y vers le haut) par la caméra de l'écran
static bool projeter(const resultats3d::Camera& c, float aspect, vec3 p, float& x, float& y) {
    const vec3 f = normalize(c.cible - c.oeil), r = normalize(cross(f, vec3{0, 1, 0})), u = cross(r, f);
    const vec3 d = p - c.oeil;
    const float z = dot(d, f);
    if (z <= 0.1f) return false;
    const float t = std::tan(c.fovY / 2);
    x = dot(d, r) / (z * t * aspect);
    y = dot(d, u) / (z * t) + c.decalageY;
    return true;
}

int main() {
    // ---- police
    {
        uint8_t g[9];
        const std::string textes = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 :,.+-—·/'!?()#%&_ÉÈÊËÀÂÄÎÏÔÖÙÛÜŸÑÇ";
        bool tous = true; std::string manque;
        for (char32_t c : led::pointsDeCode(textes)) if (!led::glyphe(c, g)) { tous = false; manque += " U+" + std::to_string((unsigned)c); }
        cas("glyphe de chaque capitale, chiffre, signe et lettre accentuée française" + manque, tous);
        led::glyphe(U'É', g);
        cas("É : accent sur les 2 rangées du haut, E dessous", g[0] != 0 && g[2] == 0x1F && g[8] == 0x1F);
        led::glyphe(U'E', g);
        cas("E : rien sur les 2 rangées d'accent", g[0] == 0 && g[1] == 0);
        cas("minuscules et accents mis en capitales (é → É, ç → Ç, œ → Œ)", led::majuscule(U'é') == U'É' && led::majuscule(U'ç') == U'Ç' && led::majuscule(U'a') == U'A' && led::majuscule(0x153) == 0x152);
        cas("UTF-8 invalide sauté sans planter", led::pointsDeCode(std::string("A\xFF" "B\xC3")).size() == 2);
    }
    // ---- RG-02 : panneau réel
    {
        const float aL = led::COLONNES * led::PAS, aH = led::RANGEES * led::PAS;
        cas("surface active " + std::to_string(aL) + " × " + std::to_string(aH) + " m, caisson ≈ 3 × 1,5 m (±5 %)",
            std::fabs(resultats3d::CAISSON_L - 3.0f) <= 0.15f && std::fabs(resultats3d::CAISSON_H - 1.5f) <= 0.075f && aL < resultats3d::CAISSON_L - 0.1f && aH < resultats3d::CAISSON_H - 0.1f);
        cas("pas de 16 mm (P16), 31 caractères de 6 points par ligne, 6 lignes", led::PAS == 0.016f && led::NB_CAR == 31 && led::NB_LIGNES * led::LIGNE_PAS <= led::RANGEES + led::LIGNE_PAS - led::CAR_H);
    }
    // ---- mise en page
    {
        const std::string fr[4] = {"POS", "PILOTE", "TEMPS", "ÉCART"}, en[4] = {"POS", "DRIVER", "TIME", "GAP"};
        std::vector<led::Ligne> l = {{"1", "TOI · LUME", "2:05,4", "—", true}, {"2", "NADIA", "2:06,9", "+1,5"}, {"3", "VANTOR GT", "2:08,2", "+2,8"},
                                     {"4", "SELENE TS", "2:11,0", "+5,6"}, {"5", "SF-X", "2:13,7", "+8,3"}, {"6", "EN TROP", "9:99,9", "+9,9"}};
        std::vector<led::Point> pts;
        led::composer(fr, l, pts);
        bool dedans = true;
        for (const led::Point& p : pts) dedans = dedans && p.c >= 0 && p.c < led::COLONNES && p.r >= 0 && p.r < led::RANGEES;
        cas("maquette 11 (FR) : " + std::to_string(pts.size()) + " points allumés, tous dans la matrice 186 × 88", dedans && pts.size() > 500);
        int lignesVues = 0;
        for (int k = 0; k < led::NB_LIGNES + 1; k++) {
            bool vu = false;
            for (const led::Point& p : pts) vu = vu || (p.r >= led::rangeeLigne(k) && p.r < led::rangeeLigne(k) + led::CAR_H);
            lignesVues += vu;
        }
        cas("en-tête + 5 lignes, la 6e ligne de pilote ignorée", lignesVues == 6);
        cas("colonnes sans chevauchement (POS, PILOTE, TEMPS, ÉCART)",
            led::COL_POS.debut + led::COL_POS.largeur < led::COL_PILOTE.debut && led::COL_PILOTE.debut + led::COL_PILOTE.largeur < led::COL_TEMPS.debut + 1 &&
            led::COL_TEMPS.debut + led::COL_TEMPS.largeur < led::COL_ECART.debut + 1 && led::COL_ECART.debut + led::COL_ECART.largeur <= led::NB_CAR);
        std::vector<led::Point> p2;
        cas("pseudo trop long coupé à 13 caractères", led::poserTexte("TOI · ABCDEFGHIJKLMNOP", led::COL_PILOTE.debut, led::COL_PILOTE.largeur, 1, p2) == 13);
        led::composer(en, l, pts);
        cas("en-tête anglais posé (DRIVER, TIME, GAP)", !pts.empty());
        // duel : 2 lignes ; Poursuite : lignes vides cachées
        std::vector<led::Ligne> d = {{"1", "TOI · LUME", "1:12,0", "—", true}, {"", "", "", ""}, {"2", "MARCO", "1:13,1", "+1,1"}};
        led::composer(fr, d, pts);
        int maxR = 0; for (const led::Point& p : pts) maxR = std::max<int>(maxR, p.r);
        cas("duel : ligne vide cachée, 2 lignes de pilotes juste sous l'en-tête", maxR < led::rangeeLigne(3));
        std::string sansGlyphe = "ÃÕ€";   // Ã, Õ → A, O ; € sauté
        cas("lettre sans glyphe : sa lettre sans accent ; signe inconnu sauté", led::poserTexte(sansGlyphe, 0, 5, 1, p2) == 2);
    }
    // ---- cadrage (maquette 11)
    for (const auto& e : {std::pair<const char*, float>{"2400x1080", 2400.f / 1080.f}, {"1280x720", 1280.f / 720.f}}) {
        const resultats3d::Camera c = resultats3d::camera(e.second);
        const vec3 ce{resultats3d::CENTRE_TABLEAU.x, resultats3d::CENTRE_TABLEAU.y, resultats3d::faceZ()};
        bool entier = true; float xmin = 9, xmax = -9, ymin = 9, ymax = -9;
        for (float sx : {-1.f, 1.f}) for (float sy : {-1.f, 1.f}) {
            float x, y;
            const bool ok = projeter(c, e.second, ce + vec3{sx * resultats3d::CAISSON_L / 2, sy * resultats3d::CAISSON_H / 2, 0}, x, y);
            entier = entier && ok && std::fabs(x) < 0.97f && std::fabs(y) < 0.97f;
            xmin = std::min(xmin, x); xmax = std::max(xmax, x); ymin = std::min(ymin, y); ymax = std::max(ymax, y);
        }
        char t[160]; std::snprintf(t, sizeof t, "%s : tableau entier dans l'image (x %.2f..%.2f, y %.2f..%.2f)", e.first, xmin, xmax, ymin, ymax);
        cas(t, entier);
        cas(std::string(e.first) + " : tableau à gauche du centre, assez grand pour être lu (≥ 22 % de la largeur de l'image)", (xmin + xmax) / 2 < 0.f && (xmax - xmin) / 2 >= 0.22f);
        float vx, vy;
        cas(std::string(e.first) + " : voiture (centre de la place) à droite du tableau", projeter(c, e.second, resultats3d::POSE_VOITURE + vec3{0, 0.7f, 0}, vx, vy) && vx > xmax - 0.15f && vx < 0.9f);
        cas(std::string(e.first) + " : tableau au-dessus du milieu de l'image (sol mouillé dessous)", (ymin + ymax) / 2 > 0.f);
    }
    // ---- reflet
    {
        const resultats3d::Camera c = resultats3d::camera(2400.f / 1080.f);
        const vec3 P = resultats3d::positionPoint(0, 0), Q = resultats3d::refletSol(c.oeil, P);
        const vec3 m{P.x, -P.y, P.z}, a = normalize(Q - c.oeil), b = normalize(m - c.oeil);
        cas("reflet sur le sol (y = 0), entre l'œil et le mur, aligné avec l'image miroir", Q.y == 0.f && Q.z < resultats3d::Z_FOND && Q.z > c.oeil.z && dot(a, b) > 0.9999f);
    }
    std::printf("tableau LED : %d échec(s)\n", echecs);
    return echecs ? 1 : 0;
}
