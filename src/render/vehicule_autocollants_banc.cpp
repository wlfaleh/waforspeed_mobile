// WaForSpeed — WFS-495 : banc des autocollants (WFS_BANC=autocollants). Cinq autocollants sur trois voitures (208, GT86,
// Corvette C7 = id mustang), voiture du joueur en vitrine (Ville), placés d'après les roues du modèle :
//   1. double bande du capot au coffre (direction toit, 180 % : 4,5 m) ;
//   2. flamme sur l'aile avant gauche, qui part du passage de roue (miroir : aile avant droite) ;
//   3-4. numéro « 27 » sur la portière avant gauche, et sur la droite (deux calques de plus : un miroir garderait chaque chiffre
//        lisible mais inverserait l'ordre) ;
//   5. étoile sur le côté droit du capot (miroir : côté gauche).
// Captures :
//   - autocollants-<id>-jour : planche 2 × 2 (3/4 avant gauche, profil gauche, 3/4 arrière droit, 3/4 avant droit vu de haut) ;
//   - autocollants-<id>-roue : gros plan du passage de roue avant gauche (la flamme s'arrête au bord, rien sur le pneu ni la jante) ;
//   - autocollants-<id>-porte : gros plan de la portière avant gauche (numéro, jonction porte / aile) ;
//   - autocollants-<id>-dessus : vue haute de 3/4 avant (bande qui suit le capot, le pare-brise sans rien, le toit, le coffre) ;
//   - autocollants-<id>-nuit : planche de nuit ;
//   - autocollants-p208-skin : flamme, éclair et étoiles posés sur la skin Rallye (RG-05 ; sans numéro : la skin a sa plaque) ;
//   - autocollants-<id>-course-jour, -nuit : en course, caméra de poursuite ;
//   - autocollants-bibliotheque : les 50 motifs de la bibliothèque (RG-06), dessinés par le moteur (blanc sur gris).
// Journal : texte des calques de chaque voiture, cuisson (durée, nombre de cœurs), mémoire des textures du GPU, nombre de textures
// d'habillage en mémoire (RC-E18-02 : une seule), et contrôle « sans autocollant » : sans calque (variable absente ou texte vide),
// aucune cuisson, la skin seule de WFS-492 est posée telle quelle. WFS_AUTOCOLLANTS_VOITURES="id …" : seulement ces voitures.
// Domaine : E10 Véhicules ; appelé par desktop_main.cpp (accroche).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>
#include "../../third_party/stb/stb_image.h"
#include "../../third_party/stb/stb_image_write.h"
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "../game/game.h"
#include "../game/sauvegarde.h"
#include "renderer.h"
#include "vehicule_autocollants.h"
#include "vehicule_skin.h"

namespace wfs {
namespace {

bool g_actif = false, g_repeindre = false, g_texteSkin = false;
vec3 g_peinture{};
const Model* g_modele = nullptr;

vec3 lin(unsigned h) { auto c = [&](int sh) { return std::pow(((h >> sh) & 255) / 255.f, 2.2f); }; return {c(16), c(8), c(0)}; }

class AutocollantsBanc final : public System {
public:
    const char* name() const override { return "autocollants-banc"; }
    void update(double, const GameView& g) override {
        if (!g_actif) return;
        if (g_repeindre && g.car && g.choisirVoiture) { g.choisirVoiture(g.car->id, g_peinture); g_repeindre = false; }
        g_modele = g.modele;
    }
};

// moitié centrale d'une capture (moyenne 2 × 2) dans la case (cx, cy) d'une planche
void coller(const std::string& png, std::vector<uint8_t>& planche, int COL, int CW, int CH, int cx, int cy, float zoom) {
    int w = 0, h = 0, n = 0;
    stbi_uc* px = stbi_load(png.c_str(), &w, &h, &n, 3);
    if (!px) { WFS_ERR("autocollants : capture illisible (%s)", png.c_str()); return; }
    const int x0 = cx * CW, y0 = cy * CH, stride = COL * CW * 3;
    const float fw = w * zoom, fh = h * zoom;
    for (int y = 0; y < CH; y++) for (int x = 0; x < CW; x++) {
        const int sx = (int)((w - fw) * 0.5f + x * fw / CW), sy = (int)((h - fh) * 0.5f + y * fh / CH);
        uint8_t* d = &planche[(size_t)(y0 + y) * stride + (size_t)(x0 + x) * 3];
        for (int c = 0; c < 3; c++) {
            int s = 0;
            for (int dy = 0; dy < 2; dy++) for (int dx = 0; dx < 2; dx++) s += px[((size_t)std::min(h - 1, sy + dy) * w + std::min(w - 1, sx + dx)) * 3 + c];
            d[c] = (uint8_t)(s / 4);
        }
    }
    stbi_image_free(px);
}

// les 5 autocollants d'une voiture, placés d'après la boîte et les roues du modèle (repère de la direction : vehicule_autocollants.h)
struct Couleurs { const char* peinture; const char* bande; const char* flamme; const char* numero; const char* etoile; };
std::string calques(const Model& m, const Couleurs& c) {
    namespace va = vehicule_autocollants;
    const float bmin[3] = {m.bmin.x, m.bmin.y, m.bmin.z}, bmax[3] = {m.bmax.x, m.bmax.y, m.bmax.z};
    float an[3];
    va::ancrage(va::Direction::Gauche, bmin, bmax, an);
    float zav = m.bmin.z + 0.2f * (m.bmax.z - m.bmin.z), zar = m.bmax.z - 0.2f * (m.bmax.z - m.bmin.z), yr = 0.32f, rayon = 0.32f;
    for (int k = 0; k < m.nRoues; k++) {   // roues avant (z < 0) et arrière
        if (m.roues[k].centre.z < 0) zav = m.roues[k].centre.z; else zar = m.roues[k].centre.z;
        yr = m.roues[k].centre.y; rayon = m.roues[k].rayon;
    }
    auto cm = [](float v) { return (int)std::lround(v * 100.f); };
    std::ostringstream o;
    float aD[3];
    va::ancrage(va::Direction::Toit, bmin, bmax, aD);
    // 1. double bande au milieu de la longueur (repère du toit : x vers l'arrière)
    o << "bande-double,t," << cm((m.bmin.z + m.bmax.z) * 0.5f - aD[2]) << ",0,0,180," << c.bande << ",0";
    // 2. flamme : base sur l'aile avant, au-dessus de la roue ; les langues passent sur le passage de roue (coupées par l'ouverture)
    //    et finissent sur la portière
    o << ";flamme-1,g," << cm(zav + 0.18f - an[2]) << "," << cm(yr + 0.17f - an[1]) << ",0,95," << c.flamme << ",1";
    // 3-4. numéro 27 sur la portière avant (42 % de l'empattement), entre le bas de caisse et la vitre ; posé de chaque côté (le
    //      miroir garde chaque chiffre lisible mais inverserait leur ordre : « 72 »). Flanc droit : x vers l'avant
    //      chiffres de 24 cm (80 %), centrés 2 cm au-dessus du haut des roues : sous la vitre des trois voitures (ceinture de la
    //      Corvette à 0,80 m, tools/skins/voitures.json)
    const float zp = zav + 0.42f * (zar - zav), yp = yr + rayon + 0.02f;
    o << ";chiffre-2,g," << cm(zp - 0.09f - an[2]) << "," << cm(yp - an[1]) << ",0,80," << c.numero << ",0";
    o << ";chiffre-7,g," << cm(zp + 0.09f - an[2]) << "," << cm(yp - an[1]) << ",0,80," << c.numero << ",0";
    o << ";chiffre-2,d," << cm(-(zp + 0.09f - an[2])) << "," << cm(yp - an[1]) << ",0,80," << c.numero << ",0";
    o << ";chiffre-7,d," << cm(-(zp - 0.09f - an[2])) << "," << cm(yp - an[1]) << ",0,80," << c.numero << ",0";
    // 5. étoile sur le côté droit du capot (repère du capot : y vers la droite de la voiture), un peu tournée
    o << ";etoile-1,c,10,46,12,70," << c.etoile << ",1";
    if (g_texteSkin) {   // variante sur la skin Rallye : pas de numéro (la skin a sa plaque), éclair sur l'aile arrière, étoiles du toit
        std::ostringstream k;
        k << "flamme-1,g," << cm(zav + 0.18f - an[2]) << "," << cm(yr + 0.17f - an[1]) << ",0,95," << c.flamme << ",1";
        k << ";eclair-1,g," << cm(zar - 0.05f - an[2]) << "," << cm(yr + rayon + 0.12f - an[1]) << ",-8,60," << c.etoile << ",1";
        k << ";etoile-3,t,0,40,0,80," << c.etoile << ",1;etoile-1,c,10,46,12,70," << c.etoile << ",1";
        return k.str();
    }
    return o.str();
}

} // namespace

WFS_SYSTEM(AutocollantsBanc, 495)

void benchAutocollants(Game& g, Renderer& r, const std::string& out) {
    namespace vs = vehicule_skin;
    namespace va = vehicule_autocollants;
    struct Voiture { const char* id; Couleurs c; };
    const Voiture VOITURES[] = {
        {"p208", {"bleu", "blanc", "orange", "blanc", "jaunecitron"}},
        {"gt86", {"blanc", "rougevif", "orangefluo", "noir", "rougevif"}},
        {"mustang", {"noir", "jaunecitron", "orangefluo", "jaunecitron", "blanc"}},
    };
    std::vector<std::string> ids{"p208", "gt86", "mustang"};
    if (const char* l = std::getenv("WFS_AUTOCOLLANTS_VOITURES")) { ids.clear(); std::istringstream is(l); std::string id; while (is >> id) ids.push_back(id); }
    auto avec = [&](const std::string& id) { return std::find(ids.begin(), ids.end(), id) != ids.end(); };
    double t = 3000;
    auto images = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; g.render(t); } };
    auto rendre = [&](const std::string& png) { g.render(t + 0.001); g.render(t + 0.002); r.capturePng(png); };
    auto capture = [&](const std::string& nom) { rendre(out + "/" + nom + ".png"); WFS_LOG("capture %s (WFS-495)", nom.c_str()); };
    const size_t texAvant = r.vk().octetsTextures;
    auto preparer = [&](const Voiture& v, const char* skin, bool autocollants) {
        setenv("WFS_SKIN", skin, 1);
        unsetenv("WFS_AUTOCOLLANTS");
        // 1er chargement : le modèle donne la place des roues ; les calques sont posés au chargement suivant (Game::setCar)
        if (!g.init(r, v.id, "ville")) { WFS_ERR("autocollants : %s impossible à charger", v.id); return false; }
        g.setAutopilot(false); g.camMode = 2;
        g_actif = true; g_repeindre = true;
        const PeintureDef* p = peintureDef(v.c.peinture);
        g_peinture = lin(p ? p->hex : 0x1d3f8fu);
        images(3);
        if (autocollants && g_modele) {
            const std::string texte = calques(*g_modele, v.c);
            setenv("WFS_AUTOCOLLANTS", texte.c_str(), 1);
            WFS_LOG("autocollants : %s : calques « %s »", v.id, texte.c_str());
            g_repeindre = true;   // « Choisir » recharge le modèle : Game::setCar recuit l'habillage avec les calques
            images(3);
        }
        const vs::Etat e = vs::etat();
        WFS_LOG("autocollants : %s, skin « %s », %s : %d texture(s) d'habillage en mémoire, %.1f Ko, %d pièces habillées, %d calques posés, "
                "cuisson %.0f ms%s ; textures du GPU %.1f Mo (avant le banc %.1f Mo) (WFS-495, RC-E18-02)",
                v.id, *skin ? skin : "aucune", autocollants ? "avec autocollants" : "sans autocollant", e.textures, e.octetsGpu / 1024.0, e.materiaux,
                e.calques, e.msCuisson, e.decodeCpu ? " (ETC2 décodé sur le processeur)" : "", r.vk().octetsTextures / 1048576.0, texAvant / 1048576.0);
        return true;
    };
    auto viser = [&](double avantM, double droiteM, float hauteur) {
        const Game::Vue v = g.calculerVue(t);
        const vec3 f = normalize(vec3{v.fwd.x, 0, v.fwd.z}), d = cross(f, {0, 1, 0});
        const vec3 o = f * (float)avantM + d * (float)droiteM;
        g.orbit = std::atan2(o.x, o.z); g.orbitRecul = length(o); g.orbitHauteur = hauteur;
    };
    const float h0 = g.orbitHauteur, r0 = g.orbitRecul;
    auto finir = [&]() { g.orbitHauteur = h0; g.orbitRecul = r0; g_actif = false; g.shutdown(); unsetenv("WFS_SKIN"); unsetenv("WFS_AUTOCOLLANTS"); };
    const std::string tmp = out + "/.autocollants-tmp.png";
    constexpr int COL = 2, LIG = 2, CW = 960, CH = 540;
    const double VUES[4][3] = {{4.4, -3.4, 1.9}, {0.2, -6.2, 1.3}, {-4.4, 3.4, 2.0}, {3.0, 2.6, 4.4}};
    auto planche = [&](const std::string& nom) {
        std::vector<uint8_t> p((size_t)COL * CW * LIG * CH * 3, 18);
        for (int k = 0; k < 4; k++) { viser(VUES[k][0], VUES[k][1], (float)VUES[k][2]); rendre(tmp); coller(tmp, p, COL, CW, CH, k % COL, k / COL, 0.62f); }
        stbi_write_png((out + "/" + nom + ".png").c_str(), COL * CW, LIG * CH, 3, p.data(), COL * CW * 3);
        WFS_LOG("capture %s : 3/4 avant gauche, profil gauche, 3/4 arrière droit, dessus (WFS-495)", nom.c_str());
    };

    // bibliothèque (RG-06) : chaque motif rastérisé par le moteur, dans une planche de 10 colonnes
    {
        std::vector<uint8_t> police;
        readAsset("fonts/Rajdhani-Bold.ttf", police);
        const int C = 10, W = 220, H = 140;
        const auto& bib = va::bibliotheque();
        const int L = (int)(bib.size() + C - 1) / C;
        std::vector<uint8_t> img((size_t)C * W * L * H * 3, 48);
        int k = 0;
        for (const va::Motif& m : bib) {
            const auto cs = va::contours(m.id, police);
            const float ech = std::min((W - 24) / std::max(m.largeur, 0.3f), (H - 24) / m.hauteur);
            const int ox = (k % C) * W, oy = (k / C) * H;
            // couverture par sur-échantillonnage 4 × 4 (règle du nombre d'enroulements non nul, comme la cuisson)
            for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
                int cov = 0;
                for (int sy = 0; sy < 4; sy++) for (int sx = 0; sx < 4; sx++) {
                    const float px = (x + (sx + 0.5f) / 4 - W / 2.f) / ech, py = -(y + (sy + 0.5f) / 4 - H / 2.f) / ech;
                    int w = 0;
                    for (const auto& c : cs) for (size_t i = 0; i < c.size(); i++) {
                        const auto& a = c[i]; const auto& b = c[(i + 1) % c.size()];
                        if ((a.second > py) != (b.second > py) && a.first + (py - a.second) / (b.second - a.second) * (b.first - a.first) > px)
                            w += b.second > a.second ? 1 : -1;
                    }
                    cov += w != 0;
                }
                uint8_t* o = &img[((size_t)(oy + y) * C * W + ox + x) * 3];
                for (int c = 0; c < 3; c++) o[c] = (uint8_t)(48 + (236 - 48) * cov / 16);
            }
            k++;
        }
        stbi_write_png((out + "/autocollants-bibliotheque.png").c_str(), C * W, L * H, 3, img.data(), C * W * 3);
        WFS_LOG("capture autocollants-bibliotheque : %zu motifs (WFS-495, RG-06)", bib.size());
    }

    setenv("WFS_MOMENT", "jour", 1);
    for (const Voiture& v : VOITURES) {
        if (!avec(v.id) || !preparer(v, "", true)) continue;
        const std::string id = v.id;
        planche("autocollants-" + id + "-jour");
        viser(1.9, -2.3, 0.75); capture("autocollants-" + id + "-roue");
        viser(0.1, -3.0, 1.05); capture("autocollants-" + id + "-porte");
        viser(3.6, -1.6, 3.2); capture("autocollants-" + id + "-dessus");
        finir();
    }
    // RG-05 : autocollants par-dessus la skin Rallye (sans le numéro : la skin a déjà sa plaque « 07 ») ; flamme, éclair sur
    // l'aile arrière, étoiles évidées sur le toit, par-dessus les bandes de la skin
    if (avec("p208")) {
        g_texteSkin = true;
        if (preparer(VOITURES[0], "rallye", true)) { planche("autocollants-p208-skin"); finir(); }
        g_texteSkin = false;
    }
    // sans autocollant : aucune cuisson, l'habillage est la skin seule de WFS-492 (même texture EAC, même chemin du shader) ;
    // l'image se compare à celle de main (banc skins, captures avant / après de la PR)
    if (avec("p208")) {
        int ecarts = 0;
        for (int k = 0; k < 2; k++) {
            if (!preparer(VOITURES[0], "rallye", false)) { ecarts++; break; }
            if (k == 1) { setenv("WFS_AUTOCOLLANTS", "", 1); g_repeindre = true; images(3); }   // texte vide : même chemin que sans la variable
            const vs::Etat e = vs::etat();
            const bool ok = e.skin == 1 && e.calques == 0 && e.textures == 1 && e.msCuisson == 0;
            if (!ok) ecarts++;
            WFS_LOG("autocollants : %s : skin %d (1 = Rallye seule, %d = cuit), %d calque, cuisson %.0f ms : %s", k ? "texte vide" : "sans la variable",
                    e.skin, vs::CUIT, e.calques, e.msCuisson, ok ? "OK" : "ÉCART");
            if (k == 0) { viser(VUES[0][0], VUES[0][1], (float)VUES[0][2]); capture("autocollants-p208-sans"); }
            finir();
        }
        WFS_LOG("autocollants : sans autocollant : %d écart(s) (0 attendu)", ecarts);
    }
    setenv("WFS_MOMENT", "nuit", 1);
    for (const Voiture& v : VOITURES) {
        if (!avec(v.id) || !preparer(v, "", true)) continue;
        planche(std::string("autocollants-") + v.id + "-nuit");
        finir();
    }
    for (const char* moment : {"jour", "nuit"}) {
        setenv("WFS_MOMENT", moment, 1);
        for (const Voiture& v : VOITURES) {
            if (!avec(v.id) || !preparer(v, "", true)) continue;
            g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(true);
            images(4);
            for (int i = 0; i < 300; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 6 == 0) g.render(t); }
            const vs::Etat e = vs::etat();
            WFS_LOG("autocollants : en course (%s, %s) : %d texture(s) d'habillage en mémoire, %d calques (RC-E18-02 : rien de cuit en course)",
                    v.id, moment, e.textures, e.calques);
            capture(std::string("autocollants-") + v.id + "-course-" + moment);
            g.setAutopilot(false);
            finir();
        }
    }
    unsetenv("WFS_MOMENT");
    std::remove(tmp.c_str());
    const vs::Etat e = vs::etat();
    WFS_LOG("autocollants : fin du banc : %d texture(s) d'habillage en mémoire (0 attendu), textures du GPU %.1f Mo (avant le banc %.1f Mo)", e.textures,
            r.vk().octetsTextures / 1048576.0, texAvant / 1048576.0);
}

} // namespace wfs
