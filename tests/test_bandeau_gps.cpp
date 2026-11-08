// WaForSpeed — test WFS-245 (RG-09, ANO-141 ; arbitrage du PO du 2026-11-07 22:05 UTC) : textes du bandeau de la carte GPS.
// Vérifie, sans carte graphique (largeurs mesurées avec la police du jeu, comme ui::width : avances et crénage de stb_truetype) :
//  - tailles au moins 2 fois les anciennes (distance, consigne, nom de rue) ; bandeau dessiné de 60 au plus ; vue de la carte
//    toujours calculée avec 38 (même carte sous le bandeau) ;
//  - les deux rangées tiennent dans le bandeau (flèche et distance, puis consigne au-dessus du liseré) ; le nom tient dans son étiquette ;
//  - toutes les consignes, toutes les distances et tous les noms de rue possibles tiennent dans leur largeur sans « … », à 16, 28
//    et 15 au moins (cadre de 150 de côté, px de référence 720 p).
// Argument : dossier des ressources (défaut : assets), pour assets/fonts/Rajdhani-Bold.ttf.
#define STB_TRUETYPE_IMPLEMENTATION
#include "../third_party/stb/stb_truetype.h"
#include <cstdio>
#include <set>
#include <string>
#include <vector>
#include "../src/ui/minicarte.h"
#include "../src/ui/navigation.h"

using namespace wfs;
using namespace wfs::minicarte;
static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

static stbtt_fontinfo police;
static float echellePolice = 0;   // ui::text : atlas à 48 px (BASE_PX de text.cpp)
static std::vector<int> pointsDeCode(const std::string& s) {
    std::vector<int> r;
    for (size_t i = 0; i < s.size();) {
        const unsigned char c = (unsigned char)s[i];
        int cp = c, n = 1;
        if (c >= 0xF0) { cp = c & 7; n = 4; } else if (c >= 0xE0) { cp = c & 15; n = 3; } else if (c >= 0xC0) { cp = c & 31; n = 2; }
        for (int k = 1; k < n && i + k < s.size(); k++) cp = (cp << 6) | (s[i + k] & 63);
        r.push_back(cp); i += n;
    }
    return r;
}
// largeur à la taille px (même calcul que run() de text.cpp, sans espacement) ; caractère absent : une espace
static float largeur(float px, const std::string& s) {
    const float k = px / 48.f;
    float w = 0; int prec = 0;
    for (int cp : pointsDeCode(s)) {
        if (!stbtt_FindGlyphIndex(&police, cp)) cp = ' ';
        if (prec) w += stbtt_GetCodepointKernAdvance(&police, prec, cp) * echellePolice * k;
        int av, g; stbtt_GetCodepointHMetrics(&police, cp, &av, &g);
        w += av * echellePolice * k; prec = cp;
    }
    return w;
}

int main(int argc, char** argv) {
    const std::string chemin = std::string(argc > 1 ? argv[1] : "assets") + "/fonts/Rajdhani-Bold.ttf";
    std::vector<unsigned char> ttf;
    if (std::FILE* f = std::fopen(chemin.c_str(), "rb")) {
        unsigned char b[65536]; size_t n;
        while ((n = std::fread(b, 1, sizeof b, f)) > 0) ttf.insert(ttf.end(), b, b + n);
        std::fclose(f);
    }
    if (ttf.empty() || !stbtt_InitFont(&police, ttf.data(), 0)) { std::printf("ÉCHEC : police introuvable (%s)\n", chemin.c_str()); return 1; }
    echellePolice = stbtt_ScaleForPixelHeight(&police, 48.f);

    // ---- tailles : au moins 2 fois les anciennes ; bandeau ; vue inchangée
    VERIFIE(DISTANCE_PX >= 2 * AVANT_DISTANCE_PX && CONSIGNE_PX >= 2 * AVANT_CONSIGNE_PX && NOM_PX >= 2 * AVANT_NOM_PX,
            "tailles %.0f / %.0f / %.0f (au moins 2 × %.0f / %.0f / %.0f)", DISTANCE_PX, CONSIGNE_PX, NOM_PX, AVANT_DISTANCE_PX, AVANT_CONSIGNE_PX, AVANT_NOM_PX);
    VERIFIE(BANDEAU_DESSIN <= BANDEAU_MAX && BANDEAU_MAX <= 60, "bandeau dessiné %.0f (60 au plus)", BANDEAU_DESSIN);
    VERIFIE(BANDEAU == 38, "vue de la carte calculée avec %.0f (38 : carte inchangée)", BANDEAU);
    VERIFIE(DISTANCE_MIN <= DISTANCE_PX && CONSIGNE_MIN <= CONSIGNE_PX && NOM_MIN <= NOM_PX, "tailles réduites au plus jusqu'au minimum");

    // ---- les deux rangées dans le bandeau (liseré cyan de 2 en bas)
    const float liseré = BANDEAU_DESSIN - 2;
    const float baseD = DISTANCE_MILIEU + 0.5f * hud::CAPITALES * DISTANCE_PX;
    VERIFIE(baseD - MONTANTES * DISTANCE_PX >= 0 && FLECHE_Y >= 0, "rangée 1 : haut de la distance %.1f (≥ 0)", baseD - MONTANTES * DISTANCE_PX);
    const float basRangee1 = std::max(FLECHE_Y + FLECHE, baseD + JAMBAGES * DISTANCE_PX);
    VERIFIE(CONSIGNE_HAUT >= basRangee1, "rangée 2 sous la rangée 1 : consigne à %.0f, rangée 1 jusqu'à %.1f", CONSIGNE_HAUT, basRangee1);
    const float basConsigne = CONSIGNE_HAUT + (MONTANTES + JAMBAGES) * CONSIGNE_PX;
    VERIFIE(basConsigne <= liseré, "consigne jusqu'à %.1f, liseré à %.0f", basConsigne, liseré);
    VERIFIE(DISTANCE_X >= FLECHE_X + FLECHE, "distance à droite de la flèche");
    VERIFIE((MONTANTES + JAMBAGES) * NOM_PX <= NOM && NOM_BAS >= 0, "nom de rue %.1f de haut dans une étiquette de %.0f", (MONTANTES + JAMBAGES) * NOM_PX, NOM);

    // ---- toutes les consignes, distances et noms de rue possibles, cadre de 150 (largeurs comme bandeau() et nomRue())
    const float lwC = COTE - 2 * CONSIGNE_X, lwD = COTE - DISTANCE_X - CONSIGNE_X, lwN = COTE - 12;
    auto mesurer = [&](const std::set<std::string>& textes, float px, float mini, float lw, const char* quoi) {
        int reduits = 0; float plusPetite = px; std::string pire;
        for (const std::string& t : textes) {
            const float p = tailleTexte([&](float q) { return largeur(q, t); }, px, mini, lw);
            if (p < px) reduits++;
            if (p < plusPetite) { plusPetite = p; pire = t; }
            VERIFIE(largeur(p, t) <= lw, "%s « %s » coupé : %.1f de large à %.1f (place : %.0f)", quoi, t.c_str(), largeur(p, t), p, lw);
        }
        std::printf("%s : %zu textes, %d réduits, plus petite taille %.1f (« %s »)\n", quoi, textes.size(), reduits, plusPetite, pire.c_str());
    };
    std::set<std::string> consignes, distances, noms;
    for (int m = 0; m <= (int)nav::Manoeuvre::Arrivee; m++) {
        nav::Prochaine p{}; p.m = (nav::Manoeuvre)m;
        for (int s = 1; s <= 8; s++) { p.sortie = s; consignes.insert(nav::texteConsigne(p)); }
    }
    for (int d = 0; d <= 20000; d++) distances.insert(nav::texteDistance(d));
    distances.insert("Tout droit");
    for (const char* carte : {"ville", "plage", "campagne", "circuit", "corniche"})
        for (int t = 0; t <= (int)trace::Type::Chicane; t++) for (int a = 0; a < 2; a++)
            for (uint32_t graine = 0; graine < 16; graine++) for (int s = 0; s < 400; s++)
                noms.insert(nav::nomRue(carte, graine, s, (trace::Type)t, a != 0));
    mesurer(consignes, CONSIGNE_PX, CONSIGNE_MIN, lwC, "consignes");
    mesurer(distances, DISTANCE_PX, DISTANCE_MIN, lwD, "distances");
    mesurer(noms, NOM_PX, NOM_MIN, lwN, "noms de rue");
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("bandeau de la carte GPS : OK\n");
    return 0;
}
