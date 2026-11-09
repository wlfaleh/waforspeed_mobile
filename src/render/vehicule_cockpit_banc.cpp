// WaForSpeed — WFS-349 (RG-03) : banc « cockpit-voisine » (voir vehicule_cockpit_banc.h).
// Même méthode que la part cachée de WFS-235 (camera_vue.cpp) : le même instant dessiné plusieurs fois, temps figé :
//   A : l'image normale ; B : la voisine retirée du dessin (TrafficCar::cache) ;
//   K : la voisine retirée, l'habitacle ET le gabarit de ceinture en couleur clé (g_interieurCle + g_gabaritCeinture).
// Pixels de la voisine = A ≠ B ; zone fermée = K ≠ B (habitacle, et tout rayon qui traverse une portière sous la ligne de
// ceinture ou le pied du pare-brise sous la planche) ; écart = pixel de la voisine dans la zone fermée (0 visé) ; pixels de la
// voisine hors zone fermée = vue par les vitres (RG-04 : elle doit rester visible par la vitre latérale).
// WFS_GARDER=1 (essai local) : garde les images B, K et le témoin A2 de chaque mesure ; WFS_BORD=<px> : bord non compté (5 par défaut).
#include "vehicule_cockpit_banc.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "../core/log.h"
#include "../core/ecran.h"
#include "../game/camera_vue.h"
#include "../game/game.h"
#include "../game/player.h"
#include "../game/ressenti.h"
#include "../game/track.h"
#include "../game/traffic_system.h"
#include "../../third_party/stb/stb_image.h"
#include "../../third_party/stb/stb_image_write.h"
#include "eclairage_physique.h"
#include "renderer.h"
#include "vehicule_cockpit.h"

namespace wfs {

namespace {
struct Image3 {
    int w = 0, h = 0; unsigned char* p = nullptr;
    explicit Image3(const std::string& f) { int c; p = stbi_load(f.c_str(), &w, &h, &c, 3); }
    ~Image3() { stbi_image_free(p); }
    bool ok(const Image3& o) const { return p && o.p && w == o.w && h == o.h; }
};
bool differe(const unsigned char* a, const unsigned char* b) {
    return std::max({std::abs(a[0] - b[0]), std::abs(a[1] - b[1]), std::abs(a[2] - b[2])}) > 12;
}
const char* nomRegard(RegardCockpit rg) { return rg == RegardCockpit::Bas ? "bas" : rg == RegardCockpit::Haut ? "haut" : "normal"; }
} // namespace

int benchCockpitVoisine(Game& g, Renderer& r, const std::string& out, std::vector<std::string> voitures,
                        const std::vector<std::string>& captures) {
    if (voitures.empty()) for (int i = 0; i < cockpit::nombreInterieurs(); i++) voitures.push_back(cockpit::interieur(i).car);
    const VueCamera vueAvant = vueCamera();
    const RegardCockpit regardAvant = regardCockpit();
    // la voisine frotte la voiture (1,8 m de centre à centre) : la secousse du choc (WFS-70, tirée au hasard à chaque image)
    // bougerait la caméra entre les images de la mesure : coupée pendant le banc (« Réduire les mouvements » ; tête immobile aussi)
    const bool reduireAvant = reglagesRessenti().reduireMouvements;
    cockpit::g_bancCockpitVoisine = true;   // gabarit et clé douce construits avec l'intérieur (rien en course)
    reglagesRessenti().reduireMouvements = true;
    int ecarts = 0; long pireEcart = 0; double t = 1200;
    auto rouler = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 0) g.render(t); } };
    for (const std::string& car : voitures) for (const bool nuit : {false, true}) {
        const bool garder = std::find(captures.begin(), captures.end(), car) != captures.end() || std::getenv("WFS_GARDER");
        if (nuit && !garder) continue;   // la nuit : images seulement (la lumière des phares voisins sur la planche est permise)
        setenv("WFS_MOMENT", nuit ? "nuit" : "jour", 1);
        if (!g.init(r, car.c_str(), "ville")) { WFS_ERR("cockpit-voisine : chargement impossible (%s)", car.c_str()); ecarts++; continue; }
        choisirVue(VueCamera::Cockpit); g.camMode = 0; g.setAutopilot(true); g.allerA(Ecran::Course);
        rouler(60 * 4);   // décompte et départ
        g.setAutopilot(false);   // plus de commande : la voiture roule droit, la voisine reste à côté (holdLane)
        long pireVoiture = 0, vitreMin = -1;
        for (const double ecartLat : {1.8, 1.9}) for (const int cote : {-1, 1}) for (const double devant : {2.5, -1.0}) {
            // trafficBenchPlace : le joueur est décalé de `lat` (demi-largeurs de route) par rapport à la voisine : lat > 0 = voisine à gauche
            trafficBenchPlace(devant, -cote * ecartLat / RW, 0, true);
            g.update(1.0 / 60); t += 1.0 / 60;
            const int k = trafficBenchCar();
            Traffic* tr = activeTrafficMut();
            if (k < 0 || !tr) { WFS_ERR("cockpit-voisine : aucune voiture placée (%s)", car.c_str()); ecarts++; continue; }
            for (int i = 0; i < 3; i++) { g.update(1.0 / 60); t += 1.0 / 60; }
            TrafficCar& c = tr->cars[(size_t)k];
            const Player& pl = g.player();
            const double dx = (c.offset - pl.playerX) * RW, dz = (c.z - pl.position) * M;
            for (const RegardCockpit rg : {RegardCockpit::Normal, RegardCockpit::Bas, RegardCockpit::Haut}) {
                choisirRegard(rg);
                const std::string base = out + "/cockpit-voisine-" + car + (nuit ? "-nuit" : "") + "-" + (cote < 0 ? "gauche" : "droite") + "-" +
                                         (ecartLat < 1.85 ? "1m8" : "1m9") + "-" + (devant > 0 ? "devant" : "cote") + "-" + nomRegard(rg);
                // lumière figée pendant les trois images (comme WFS-235) ; halo coupé (il déborderait de la voisine)
                eclairage::suspendreCaptures(true);
                const float bloom = r.bloomForce; r.bloomForce = 0;
                // image témoin A2 refaite à la fin : si elle diffère de A (> 0,2 % des pixels), la lumière bougeait : on recommence
                float instable = 100;
                for (int essai = 0; essai < (nuit ? 1 : 6) && instable > 0.2f; essai++) {   // nuit : la pluie change à chaque image (images seulement)
                    for (int i = 0; i < 4; i++) g.render(t);
                    g.render(t); r.capturePng(base + "-A.png");
                    c.cache = true; g.render(t); r.capturePng(base + "-B.png");
                    cockpit::g_interieurCle = cockpit::g_gabaritCeinture = true; g.render(t); r.capturePng(base + "-K.png");
                    cockpit::g_interieurCle = cockpit::g_gabaritCeinture = false; c.cache = false;
                    g.render(t); r.capturePng(base + "-A2.png");
                    Image3 a(base + "-A.png"), a2(base + "-A2.png");
                    long n = 0;
                    if (a.ok(a2)) for (long i = 0; i < (long)a.w * a.h * 3; i += 3) n += differe(a.p + i, a2.p + i);
                    instable = a.ok(a2) ? 100.f * n / ((float)a.w * a.h) : 100.f;
                }
                if (!std::getenv("WFS_GARDER")) std::remove((base + "-A2.png").c_str());
                r.bloomForce = bloom;
                eclairage::suspendreCaptures(false);
                if (!nuit && instable > 0.2f) { WFS_ERR("cockpit-voisine : lumière instable (%.1f %%) : %s", instable, base.c_str()); ecarts++; }
                Image3 A(base + "-A.png"), B(base + "-B.png"), K(base + "-K.png");
                long voisine = 0, ecart = 0, vitre = 0;
                if (A.ok(B) && A.ok(K)) {
                    // zone fermée réduite de R pixels (5 sur une image de 720 lignes) : un pixel du bord de l'habitacle (montant,
                    // appui de vitre, dessus de planche) est en partie habitacle, en partie rue (anticrénelage, filtres de l'image),
                    // et un point très lumineux de la voisine juste au bord (reflet du soleil, feu) s'étale de 3 à 4 pixels sur
                    // l'habitacle (mesuré au banc : même teinte, plus faible ; Compass, Corsa, Range Rover). Seuls comptent les
                    // pixels dont tout le carré (2R + 1)² est dans la zone fermée, sans pixel de la voisine vue par une vitre
                    // (pointe fine d'une vitre entre montant et appui) : une fuite réelle (trou de l'habitacle) en a des milliers
                    // (avant la correction : plus de 10 000) ; les fentes plus fines sont vérifiées par le calcul (tests/test_cockpit_ferme.cpp)
                    const int W = A.w, H = A.h, R = std::max(1, (int)std::lround((std::getenv("WFS_BORD") ? std::atof(std::getenv("WFS_BORD")) : 5.0) * H / 720));   // WFS_BORD : essai local
                    std::vector<unsigned char> ferme((size_t)W * H), parVitre((size_t)W * H);
                    for (long i = 0; i < (long)W * H; i++) {
                        ferme[(size_t)i] = differe(K.p + i * 3, B.p + i * 3);
                        parVitre[(size_t)i] = !ferme[(size_t)i] && differe(A.p + i * 3, B.p + i * 3);
                    }
                    long bord = 0, lueur = 0;
                    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
                        const long i = (long)y * W + x;
                        if (!differe(A.p + i * 3, B.p + i * 3)) continue;
                        voisine++;
                        if (!ferme[(size_t)i]) { vitre++; continue; }
                        bool plein = x >= R && x + R < W && y >= R && y + R < H;
                        for (int dy = -R; plein && dy <= R; dy += 1) for (int dx = -R; plein && dx <= R; dx += 1) {
                            const size_t j = (size_t)(i + (long)dy * W + dx);
                            plein = ferme[j] && !parVitre[j];
                        }
                        if (!plein) { bord++; continue; }
                        // lueur d'un feu de la voisine sur le bord de l'habitacle (anticrénelage d'une lampe très lumineuse) : seule
                        // une couleur monte un peu (mesuré : rouge +15 à +21 niveaux, vert et bleu identiques, 1 à 3 pixels sous
                        // l'appui de vitre, près d'un feu arrière) : pas un morceau de la voiture, ignorée (≤ 25 niveaux, jamais une baisse)
                        {
                            const unsigned char *pa = A.p + i * 3, *pb = B.p + i * 3;
                            const int d0 = pa[0] - pb[0], d1 = pa[1] - pb[1], d2 = pa[2] - pb[2];
                            if (std::min({d0, d1, d2}) >= -3 && std::max({d0, d1, d2}) <= 25) { lueur++; continue; }
                        }
                        ecart++; A.p[i * 3] = 255; A.p[i * 3 + 1] = 0; A.p[i * 3 + 2] = 255;   // écarts en magenta
                    }
                    if (lueur) WFS_LOG("WFS349 %s : %ld pixel(s) de lueur d'un feu (≤ 25 niveaux), non comptés", base.substr(out.size() + 1).c_str(), lueur);
                    if (bord) WFS_LOG("WFS349 %s : %ld pixel(s) de bord (≤ %d px, bords étalés par le rendu), non comptés", base.substr(out.size() + 1).c_str(), bord, R);
                } else { WFS_ERR("cockpit-voisine : captures illisibles (%s)", base.c_str()); ecarts++; }
                if (garder && ecart > 0) stbi_write_png((base + "-ecarts.png").c_str(), A.w, A.h, 3, A.p, A.w * 3);
                // jour : l'écart compte ; nuit : écrit seulement (les phares de la voisine éclairent l'habitacle, comme en vrai)
                if (!nuit) {
                    pireVoiture = std::max(pireVoiture, ecart);
                    if (devant > 0) vitreMin = vitreMin < 0 ? vitre : std::min(vitreMin, vitre);   // RG-04 : voisine devant, vue par les vitres
                }
                WFS_LOG("WFS349 %s : voisine à %.2f m en travers, %+.1f m en long ; pixels de la voisine %ld, sur l'habitacle ou sous la "
                        "ligne de ceinture %ld (0 visé), par les vitres %ld", base.substr(out.size() + 1).c_str(), dx, dz, voisine, ecart, vitre);
                if (!std::getenv("WFS_GARDER")) { std::remove((base + "-B.png").c_str()); std::remove((base + "-K.png").c_str()); }
                const bool image = garder && rg == RegardCockpit::Normal && ecartLat > 1.85;
                if (!image) std::remove((base + "-A.png").c_str());
                else std::rename((base + "-A.png").c_str(), (base + ".png").c_str());
            }
            choisirRegard(RegardCockpit::Normal);
        }
        if (!nuit) {
            WFS_LOG("cockpit-voisine %s : pire image %ld pixel(s) de la voisine sur l'habitacle ou sous la ceinture (0 visé) ; "
                    "voisine devant, vue par les vitres : au moins %ld pixel(s) (> 0 visé)%s", car.c_str(), pireVoiture, std::max(0L, vitreMin), pireVoiture || vitreMin <= 0 ? " ÉCHEC" : " OK");
            if (pireVoiture > 0 || vitreMin <= 0) ecarts++;
            pireEcart = std::max(pireEcart, pireVoiture);
        }
    }
    unsetenv("WFS_MOMENT");
    choisirVue(vueAvant); choisirRegard(regardAvant);
    reglagesRessenti().reduireMouvements = reduireAvant;
    cockpit::g_bancCockpitVoisine = false;
    WFS_LOG("cockpit-voisine (WFS-349 RG-03) : %zu voiture(s), pire %ld pixel(s) interdit(s), %d écart(s)%s", voitures.size(), pireEcart, ecarts,
            ecarts ? " ÉCHEC" : " OK");
    return ecarts;
}

} // namespace wfs
