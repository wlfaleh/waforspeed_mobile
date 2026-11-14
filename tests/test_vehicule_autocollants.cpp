// WaForSpeed — WFS-495 : test des autocollants (src/render/vehicule_autocollants.h) et de la compression ETC2 RGBA8
// (src/render/vehicule_etc2.h), sans GPU :
//   - ETC2 : blocs unis (alpha exact), image de motifs nets codée puis décodée (PSNR ≥ 32 dB sur la couleur, ≥ 38 dB sur l'alpha) ;
//   - texte des calques : aller-retour, bornes, rejets (motif, direction, couleur inconnus ; plus de 20 calques) ;
//   - bibliothèque : 50 motifs (5 bandes, 10 chiffres, 3 flammes, 3 éclairs, 3 étoiles, 26 lettres), chacun dessiné ;
//   - cuisson : bande sur le capot (centre couvert, de la bonne couleur ; rien hors de la zone du dessus), miroir (même place de
//     l'autre côté), lettres jamais retournées, skin seule identique aux masques, durée (journal) ;
//   - couleurs des skins : copie C++ (vehicule_skin::couleurs) = SKIN_COULEURS de shaders/vehicule.glsl.
// Lancement depuis la racine du dépôt (lit assets/fonts/Rajdhani-Bold.ttf et shaders/vehicule.glsl). WFS_APERCU=<dossier> :
// écrit aussi la planche de la bibliothèque (autocollants-bibliotheque.png) et l'atlas cuit de l'essai (autocollants-atlas.png).
#define STB_TRUETYPE_IMPLEMENTATION
#include "../third_party/stb/stb_truetype.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../third_party/stb/stb_image_write.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include "../src/render/vehicule_autocollants.h"
#include "../src/render/vehicule_etc2.h"
#include "../src/render/vehicule_skin.h"

using namespace wfs;
namespace va = wfs::vehicule_autocollants;
namespace ve = wfs::vehicule_etc2;
namespace vs = wfs::vehicule_skin;

static int echecs = 0;
#define VERIFIER(c, ...) do { if (!(c)) { echecs++; std::printf("ÉCHEC %s:%d : ", __FILE__, __LINE__); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

static double psnr(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b, int canal0, int canaux) {
    double e = 0; size_t n = 0;
    for (size_t i = 0; i < a.size() / 4; i++) for (int c = canal0; c < canal0 + canaux; c++) { const double d = (double)a[i * 4 + c] - b[i * 4 + c]; e += d * d; n++; }
    e /= (double)n;
    return e == 0 ? 99.0 : 10 * std::log10(255.0 * 255.0 / e);
}

static std::vector<uint8_t> lireFichier(const char* p) {
    std::ifstream f(p, std::ios::binary);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

int main() {
    // ---------------------------------------------------------------- ETC2 : blocs unis
    {
        uint8_t t[16][4], b[16], d[16][4];
        const uint8_t cas[][4] = {{0, 0, 0, 0}, {255, 255, 255, 255}, {200, 30, 40, 128}, {12, 200, 90, 7}};
        for (const auto& c : cas) {
            for (auto& p : t) for (int k = 0; k < 4; k++) p[k] = c[k];
            ve::encoderBloc(t, b); ve::decoderBloc(b, d);
            int pire = 0;
            for (int i = 0; i < 16; i++) { VERIFIER(d[i][3] == c[3], "alpha uni %d décodé %d", c[3], d[i][3]); for (int k = 0; k < 3; k++) pire = std::max(pire, std::abs(d[i][k] - c[k])); }
            VERIFIER(pire <= 6, "couleur unie (%d,%d,%d) : écart %d > 6", c[0], c[1], c[2], pire);
        }
    }
    // ---------------------------------------------------------------- bibliothèque
    const std::vector<uint8_t> police = lireFichier("assets/fonts/Rajdhani-Bold.ttf");
    VERIFIER(!police.empty(), "police Rajdhani absente (lancer depuis la racine du dépôt)");
    {
        int n[6] = {};
        for (const va::Motif& m : va::bibliotheque()) {
            n[(int)m.famille]++;
            const auto cs = va::contours(m.id, police);
            size_t pts = 0; for (const auto& c : cs) pts += c.size();
            VERIFIER(!cs.empty() && pts >= 3, "motif %s : aucun contour", m.id);
            float xmin = 1e9f, xmax = -1e9f, ymin = 1e9f, ymax = -1e9f;
            for (const auto& c : cs) for (const auto& p : c) { xmin = std::min(xmin, p.first); xmax = std::max(xmax, p.first); ymin = std::min(ymin, p.second); ymax = std::max(ymax, p.second); }
            // le dessin tient dans sa taille annoncée (±3 cm ; lettres et chiffres : hauteur des capitales, Q et J dépassent un peu)
            if (!va::lisible(m.famille)) VERIFIER(xmax - xmin <= m.largeur + 0.03f && ymax - ymin <= m.hauteur + 0.03f, "motif %s : %.2f × %.2f m > %.2f × %.2f", m.id, xmax - xmin, ymax - ymin, m.largeur, m.hauteur);
            else VERIFIER(ymax - ymin >= 0.25f && ymax - ymin <= 0.40f, "glyphe %s : hauteur %.3f m", m.id, ymax - ymin);
        }
        VERIFIER(va::bibliotheque().size() == 50, "bibliothèque : %zu motifs (50 attendus)", va::bibliotheque().size());
        VERIFIER(n[0] == 5 && n[1] == 10 && n[2] == 3 && n[3] == 3 && n[4] == 3 && n[5] == 26, "familles %d %d %d %d %d %d", n[0], n[1], n[2], n[3], n[4], n[5]);
    }
    // ---------------------------------------------------------------- texte des calques
    {
        std::vector<va::Calque> c;
        int rej = -1;
        const std::string t = "flamme-1,g,-110,-5,0,100,orange,1;bande-double,c,0,0,0,220,blanc,0;chiffre-7,g,10,10,15,120,noir,1";
        VERIFIER(va::lire(t, c, &rej) && rej == 0 && c.size() == 3, "lecture : %zu calques, %d rejets", c.size(), rej);
        VERIFIER(va::texte(c) == t, "aller-retour : « %s »", va::texte(c).c_str());
        VERIFIER(c.size() == 3 && c[0].dir == va::Direction::Gauche && c[0].miroir && c[1].dir == va::Direction::Capot && c[1].echelle == 220 && c[2].rotation == 15, "champs lus");
        va::lire("etoile-1,t,0,0,190,999,rouge,0;inconnu,g,0,0,0,100,rouge,0;etoile-2,x,0,0,0,100,rouge,0;etoile-3,g,0,0,0,100,mauve,0;etoile-1,g,0,0", c, &rej);
        VERIFIER(c.size() == 1 && rej == 4, "rejets : %zu calques, %d rejets", c.size(), rej);
        VERIFIER(c.size() == 1 && c[0].echelle == va::ECHELLE_MAX && c[0].rotation == -170, "bornes : échelle %d, rotation %d", c.empty() ? 0 : c[0].echelle, c.empty() ? 0 : c[0].rotation);
        std::string long21;
        for (int i = 0; i < 21; i++) long21 += (i ? ";" : "") + std::string("etoile-1,g,0,0,0,100,rouge,0");
        va::lire(long21, c, &rej);
        VERIFIER(c.size() == 20 && rej == 1, "21 calques : %zu gardés, %d rejet(s)", c.size(), rej);
        for (char ch : va::texte(c)) VERIFIER(ch != '=' && ch != '\n' && ch != '|', "caractère interdit dans le texte (%c)", ch);
    }
    // ---------------------------------------------------------------- cuisson
    {
        std::vector<va::Calque> c;
        va::lire("bande-large,c,0,0,0,100,rouge,0;lettre-r,g,60,0,0,200,blanc,1;flamme-1,a,0,0,0,100,orange,0", c);
        va::Entree e;
        e.calques = &c; e.police = &police; e.garderRgba = true;
        va::Resultat r;
        VERIFIER(va::cuire(e, r) && r.calques == 4 && r.motifsVides == 0, "cuisson : %d calques posés, %d vides", r.calques, r.motifsVides);
        VERIFIER(r.niveaux.size() == 11 && r.niveaux[0].size() == ve::octets(1024, 1024), "niveaux : %zu", r.niveaux.size());
        std::printf("test_vehicule_autocollants : cuisson de %d calques : dessin %.1f ms, mipmaps %.1f ms, compression %.1f ms (1 cœur)\n",
                    r.calques, r.msDessin, r.msMip, r.msCompression);
        const auto px = [&](float u, float v) { const int x = (int)u, y = (int)v; return &r.rgba[((size_t)y * 1024 + x) * 4]; };
        // bande rouge sur le capot : au centre (a = 0, b = 0), couverte, rouge ; à 40 cm de côté, rien
        float an[3], u, v;
        va::ancrage(va::Direction::Capot, e.bmin, e.bmax, an);
        va::versAtlas(va::Direction::Capot, an, 0, 0, u, v);
        const uint8_t* p = px(u, v);
        VERIFIER(p[3] == 255 && p[0] > 150 && p[1] < 60 && p[2] < 80, "bande du capot : (%d,%d,%d,%d) au centre", p[0], p[1], p[2], p[3]);
        va::versAtlas(va::Direction::Capot, an, 0, 0.40f, u, v);
        VERIFIER(px(u, v)[3] == 0, "bande du capot : couverte à 40 cm de l'axe (%d)", px(u, v)[3]);
        // la bande est sur le dessus seulement, la flamme sur l'avant seulement : rien sur la moitié « arrière » des faces
        int avant = 0, arriere = 0;
        for (int y = vs::atlas::FACES0; y < vs::atlas::FACES1; y++) for (int x = 0; x < 1024; x++) (x < 512 ? avant : arriere) += r.rgba[((size_t)y * 1024 + x) * 4 + 3] > 0;
        VERIFIER(avant > 1000 && arriere == 0, "faces : %d texels couverts à l'avant (flamme), %d à l'arrière (0 attendu)", avant, arriere);
        // miroir : la lettre R à 60 cm derrière l'ancrage du flanc gauche se retrouve au même endroit sur le flanc droit
        int g = 0, d = 0;
        for (int y = vs::atlas::GAUCHE0; y < vs::atlas::GAUCHE1; y++) for (int x = 0; x < 1024; x++) g += r.rgba[((size_t)y * 1024 + x) * 4 + 3] > 128;
        for (int y = vs::atlas::DROITE0; y < vs::atlas::DROITE1; y++) for (int x = 0; x < 1024; x++) d += r.rgba[((size_t)y * 1024 + x) * 4 + 3] > 128;
        VERIFIER(g > 500 && std::abs(g - d) <= g / 50, "flancs : %d texels à gauche, %d à droite (même R)", g, d);
        va::ancrage(va::Direction::Gauche, e.bmin, e.bmax, an);
        float ug, vg, ud, vd;
        va::versAtlas(va::Direction::Gauche, an, 0.60f, 0, ug, vg);
        va::versAtlas(va::Direction::Droite, an, -0.60f, 0, ud, vd);
        VERIFIER(std::abs(vd - vg - (vs::atlas::DROITE0 - vs::atlas::GAUCHE0)) < 0.01f && std::abs(ud - (1024 - ug)) < 0.01f, "repères des flancs symétriques");
        // la lettre n'est pas retournée : la jambe du R (en bas à droite, vue de dehors) est à droite du centre sur les deux flancs.
        // Flanc gauche vu de dehors : droite = arrière (u croissant) ; flanc droit : droite = avant (u décroissant… vu de dehors aussi
        // à droite de l'image) : dans les deux cas la couverture de la moitié droite de la lettre (vue de dehors) est la même
        auto moitie = [&](int l0, float uc, float vc, int sens) {   // texels couverts dans le quart bas, côté « droite vue de dehors »
            int n = 0;
            for (int y = (int)vc; y < (int)vc + 20; y++) for (int k = 4; k < 40; k++) n += r.rgba[((size_t)y * 1024 + (int)uc + sens * k) * 4 + 3] > 128;
            (void)l0; return n;
        };
        const int jambeG = moitie(vs::atlas::GAUCHE0, ug, vg, +1), jambeD = moitie(vs::atlas::DROITE0, ud, vd, +1);
        VERIFIER(jambeG > 0 && std::abs(jambeG - jambeD) <= jambeG / 4 + 4, "R lisible des deux côtés : %d / %d texels", jambeG, jambeD);
        // compression : décodé contre l'original
        std::vector<uint8_t> dec(r.rgba.size());
        ve::decoderImage(r.niveaux[0].data(), 1024, 1024, dec.data());
        const double pc = psnr(r.rgba, dec, 0, 3), pa = psnr(r.rgba, dec, 3, 1);
        std::printf("test_vehicule_autocollants : ETC2 RGBA8 niveau 0 : PSNR couleur %.1f dB, alpha %.1f dB\n", pc, pa);
        VERIFIER(pc >= 32 && pa >= 38, "PSNR couleur %.1f dB (≥ 32), alpha %.1f dB (≥ 38)", pc, pa);
        if (const char* dossier = std::getenv("WFS_APERCU")) {
            // atlas cuit sur fond de peinture bleue (aperçu)
            std::vector<uint8_t> img(1024 * 1024 * 3);
            for (size_t i = 0; i < 1024 * 1024; i++) {
                const float a = dec[i * 4 + 3] / 255.f;
                const uint8_t bleu[3] = {29, 63, 143};
                for (int k = 0; k < 3; k++) img[i * 3 + k] = (uint8_t)(bleu[k] * (1 - a) + dec[i * 4 + k] * a);
            }
            stbi_write_png((std::string(dossier) + "/autocollants-atlas.png").c_str(), 1024, 1024, 3, img.data(), 1024 * 3);
        }
        // skin seule : la cuisson reproduit les masques (couverture = 1 − (1 − R)(1 − G))
        std::vector<uint8_t> masques(1024 * 1024 * 2, 0);
        for (int y = 100; y < 140; y++) for (int x = 0; x < 1024; x++) { masques[(y * 1024 + x) * 2] = 255; if (y >= 120) masques[(y * 1024 + x) * 2 + 1] = 255; }
        va::Entree s;
        s.masques = masques.data(); s.garderRgba = true;
        VERIFIER(vs::couleurs(1, s.couleurA, s.couleurB), "couleurs du Rallye");
        va::Resultat rs;
        VERIFIER(va::cuire(s, rs), "cuisson d'une skin seule");
        const uint8_t* a = &rs.rgba[(110 * 1024 + 500) * 4]; const uint8_t* b = &rs.rgba[(130 * 1024 + 500) * 4]; const uint8_t* z = &rs.rgba[(160 * 1024 + 500) * 4];
        VERIFIER(a[3] == 255 && std::abs(a[0] - 232) <= 1 && b[3] == 255 && std::abs(b[0] - 26) <= 1 && z[3] == 0, "skin seule : A (%d,%d) B (%d,%d) vide %d", a[0], a[3], b[0], b[3], z[3]);
        va::Entree vide;
        va::Resultat rv;
        VERIFIER(!va::cuire(vide, rv), "rien à cuire : faux");
    }
    // ---------------------------------------------------------------- planche de la bibliothèque (aperçu)
    if (const char* dossier = std::getenv("WFS_APERCU")) {
        const int C = 10, W = 200, H = 120;   // 10 colonnes, case de 200 × 120 px
        const int L = (int)(va::bibliotheque().size() + C - 1) / C;
        std::vector<uint8_t> img((size_t)C * W * L * H * 3, 30);
        int k = 0;
        for (const va::Motif& m : va::bibliotheque()) {
            const auto cs = va::contours(m.id, police);
            const float ech = std::min((W - 20) / std::max(m.largeur, 0.3f), (H - 20) / m.hauteur);
            const int ox = (k % C) * W + W / 2, oy = (k / C) * H + H / 2;
            for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
                // pair-impair par lancer de rayon horizontal (aperçu seulement)
                const float px = (x - W / 2 + 0.5f) / ech, py = -(y - H / 2 + 0.5f) / ech;
                int w = 0;
                for (const auto& c : cs) for (size_t i = 0; i < c.size(); i++) {
                    const auto& p0 = c[i]; const auto& p1 = c[(i + 1) % c.size()];
                    if ((p0.second > py) != (p1.second > py)) {
                        const float xi = p0.first + (py - p0.second) / (p1.second - p0.second) * (p1.first - p0.first);
                        if (xi > px) w += p1.second > p0.second ? 1 : -1;
                    }
                }
                if (w) { uint8_t* o = &img[((size_t)(oy - H / 2 + y) * C * W + (ox - W / 2 + x)) * 3]; o[0] = 240; o[1] = 240; o[2] = 235; }
            }
            k++;
        }
        stbi_write_png((std::string(dossier) + "/autocollants-bibliotheque.png").c_str(), C * W, L * H, 3, img.data(), C * W * 3);
    }
    // ---------------------------------------------------------------- couleurs des skins : C++ = shader
    {
        std::ifstream f("shaders/vehicule.glsl");
        std::stringstream ss; ss << f.rdbuf();
        const std::string s = ss.str();
        const size_t p = s.find("const vec3 SKIN_COULEURS[");
        VERIFIER(p != std::string::npos, "SKIN_COULEURS absent de shaders/vehicule.glsl");
        if (p != std::string::npos) {
            const std::string bloc = s.substr(p, s.find(';', p) - p);
            std::vector<float> v;   // valeurs de chaque vec3, vec3(0.0) = 3 zéros
            const std::regex re(R"(vec3\(([^()]*)\))");
            for (auto it = std::sregex_iterator(bloc.begin(), bloc.end(), re); it != std::sregex_iterator(); ++it) {
                std::vector<float> c; std::stringstream cs((*it)[1].str()); std::string x;
                while (std::getline(cs, x, ',')) c.push_back(std::stof(x));
                if (c.size() == 1) c = {c[0], c[0], c[0]};
                v.insert(v.end(), c.begin(), c.end());
            }
            VERIFIER(v.size() == (size_t)(vs::NOMBRE + 1) * 6, "SKIN_COULEURS : %zu valeurs", v.size());
            for (int k = 1; k <= vs::NOMBRE && v.size() == (size_t)(vs::NOMBRE + 1) * 6; k++) {
                float a[3] = {}, b[3] = {};
                vs::couleurs(k, a, b);
                for (int c = 0; c < 3; c++)
                    VERIFIER(std::abs(a[c] - v[k * 6 + c]) < 1e-4f && std::abs(b[c] - v[k * 6 + 3 + c]) < 1e-4f,
                             "skin %d : couleurs C++ (vehicule_skin::couleurs) ≠ SKIN_COULEURS du shader", k);
            }
        }
    }
    if (echecs) { std::printf("test_vehicule_autocollants : %d échec(s)\n", echecs); return 1; }
    std::printf("test_vehicule_autocollants : OK (ETC2 RGBA8, texte des calques, 50 motifs, cuisson, miroir, couleurs des skins)\n");
    return 0;
}
