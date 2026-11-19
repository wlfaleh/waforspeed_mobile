// WaForSpeed — WFS-567 (ANO-315, ANO-290) : messages de course lisibles, sans cyan, jamais sur la route.
// Vérifie le calcul seul de src/ui/toasts.h (namespace messages) et la longueur des titres dans les tables de textes :
//   RG-01 : aucun cyan, contraste texte / fond ≥ 4,5:1 (fond translucide posé sur du blanc ou du noir) ;
//   RG-02 : messages de rang 0 et 1 hors de la zone centrale de la route, en haut, sous le HUD (et sous l'encart de la Poursuite),
//           en 1280x720, 1600x720, 2400x1080, avec ou sans encoche ; 2 messages au plus ;
//   RG-03 : lettres ≥ 3,5 % de la hauteur, vie 1,5 à 2,5 s, fondus ≤ 0,2 s, titres ≤ 18 caractères (FR et EN).
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <map>
#include "../src/ui/langue_regles.h"
#include "../src/ui/toasts.h"

using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

// luminance relative (WCAG 2) d'une couleur sRGB 0..255
static double lum(double r, double g, double b) {
    auto c = [](double v) { v /= 255; return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4); };
    return 0.2126 * c(r) + 0.7152 * c(g) + 0.0722 * c(b);
}
static double contraste(double l1, double l2) { if (l1 < l2) std::swap(l1, l2); return (l1 + 0.05) / (l2 + 0.05); }
static void rgb(unsigned h, double& r, double& g, double& b) { r = (h >> 16) & 255; g = (h >> 8) & 255; b = h & 255; }

// nombre de caractères (UTF-8) d'un titre, marqueurs {n} comptés comme un chiffre
static int caracteres(const std::string& s) {
    int n = 0;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '{') { const size_t f = s.find('}', i); if (f != std::string::npos) { n++; i = f; continue; } }
        if (((unsigned char)s[i] & 0xC0) != 0x80) n++;
    }
    return n;
}

int main(int argc, char** argv) {
    using namespace messages;
    // RG-01 : aucun cyan
    for (Repere r : {Repere::Gain, Repere::Danger, Repere::Info}) VERIFIE(couleurRepere(r) != 0x00E5FF, "repère en cyan");
    VERIFIE(TEXTE != 0x00E5FF && SOMBRE != 0x00E5FF, "texte ou fond en cyan");
    // RG-01 : contraste du texte sur le fond translucide, posé sur le pire fond de la scène (blanc : ciel, façade claire)
    {
        double tr, tg, tb, sr, sg, sb; rgb(TEXTE, tr, tg, tb); rgb(SOMBRE, sr, sg, sb);
        for (double derriere : {255.0, 128.0, 0.0}) {
            const double a = FOND_OPACITE, fr = a * sr + (1 - a) * derriere, fg = a * sg + (1 - a) * derriere, fb = a * sb + (1 - a) * derriere;
            const double c = contraste(lum(tr, tg, tb), lum(fr, fg, fb));
            VERIFIE(c >= 4.5, "contraste %.2f:1 < 4,5:1 sur un fond de %g", c, derriere);
        }
    }
    // RG-03 : durées
    VERIFIE(VIE >= 1.5 && VIE <= 2.5, "vie %.2f s hors de 1,5 à 2,5 s", VIE);
    VERIFIE(ENTREE <= 0.2 && SORTIE <= 0.2, "fondus %.2f / %.2f s > 0,2 s", ENTREE, SORTIE);
    VERIFIE(MAX_VISIBLES == 2, "RG-02 : %d messages à la fois (2 au plus)", MAX_VISIBLES);
    // RG-02, RG-03 : place et taille sur les écrans du jeu
    struct Ecran { float W, H; Marges m; const char* nom; };
    const Ecran ecrans[] = {{1280, 720, {}, "1280x720"}, {1600, 720, {}, "1600x720"}, {2400, 1080, {}, "2400x1080"},
                            {2400, 1080, {90, 0, 0, 0}, "2400x1080 encoche 90 px"}, {2400, 1080, {0, 0, 90, 0}, "2400x1080 encoche à droite"}};
    for (const Ecran& e : ecrans) {
        VERIFIE(taillePolice(e.H) >= 0.035f * e.H - 1e-3f, "%s : lettres de %.1f px < 3,5 %% de H", e.nom, taillePolice(e.H));
        const Rect route = zoneRoute(e.W, e.H);
        for (int poursuite = 0; poursuite < 2; poursuite++) {
            const float bas = poursuite ? poursuiteEncartBas(e.H, e.m.haut) : 0, demi = poursuite ? poursuiteEncartDemi(e.H) : 0;
            const float wmax = largeurMax(e.W, e.H, e.m, demi);
            VERIFIE(wmax >= 0.25f * e.H, "%s%s : largeur permise %.0f px trop petite", e.nom, poursuite ? " (Poursuite)" : "", wmax);
            const float xc = (e.m.gauche + e.W - e.m.droite) / 2;
            const Rect encart{xc - demi, 0, 2 * demi, bas};
            for (int rang = 0; rang < MAX_VISIBLES; rang++) {
                for (float w : {0.2f * e.H, wmax}) {   // court, puis le plus large permis
                    const Rect q = place(e.W, e.H, e.m, bas, demi, rang, w);
                    VERIFIE(!croise(q, route), "%s%s : message %d (%.0f px) dans la zone de la route (y %.0f à %.0f)", e.nom,
                            poursuite ? " (Poursuite)" : "", rang, w, q.y, q.y + q.h);
                    VERIFIE(q.y >= bandeauxHaut(e.H, e.m) - 0.5f, "%s : message %d sur la rangée du haut du HUD", e.nom, rang);
                    VERIFIE(q.x >= e.m.gauche && q.x + q.w <= e.W - e.m.droite, "%s : message %d hors de l'écran", e.nom, rang);
                    if (poursuite) VERIFIE(!croise(q, encart), "%s (Poursuite) : message %d sous l'encart SCORE / POLICE", e.nom, rang);
                    else VERIFIE(q.x + q.w <= bandeauxBordDroit(e.W, e.H, e.m) + 0.5f, "%s : message %d sur la mini-carte", e.nom, rang);
                }
            }
        }
    }
    // RG-03 : titres ≤ 18 caractères (valeur à part), en français et en anglais
    const std::string actifs = argc > 1 ? argv[1] : "assets";
    for (const char* table : {"fr", "en"}) {
        std::ifstream f(actifs + "/textes/" + table + ".txt", std::ios::binary);
        VERIFIE((bool)f, "table %s introuvable", table);
        if (!f) continue;
        std::stringstream ss; ss << f.rdbuf();
        std::map<std::string, std::string> t;
        ui::langue::analyserTable(ss.str(), t);
        for (const char* cle : {"toast.checkpoint", "toast.quasi", "toast.quasi_combo", "toast.combo", "toast.police_approche", "toast.accident",
                                "toast.combo_perdu", "toast.feu_grille", "toast.nouveaux_traces", "police.renfort", "police.embouteillage",
                                "police.barrage", "police.deboite", "police.travaux", "police.accident", "police.camion", "police.ralentisseur",
                                "police.arrestation"}) {
            VERIFIE(t.count(cle), "%s : clé %s absente", table, cle);
            if (t.count(cle)) VERIFIE(caracteres(t[cle]) <= 18, "%s : « %s » = « %s » : %d caractères (18 au plus)", table, cle, t[cle].c_str(), caracteres(t[cle]));
        }
    }
    std::printf("test_messages : %s (%d échec(s))\n", echecs ? "ÉCHEC" : "OK", echecs);
    return echecs ? 1 : 0;
}
