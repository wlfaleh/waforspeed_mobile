// WaForSpeed — WFS-373 : banc d'essai des gros plans des objets modélisés du Grand Prix (WFS_BANC=grandprix-objets).
// Une vue libre (carte::g_vueCarte) par sorte d'objet, à hauteur d'homme, de jour, et la nuit pour les objets éclairés ou
// très présents : grandprix-objets-<sorte>(-nuit). Les objets sont les premiers de leur sorte posés sur le tour
// (grandprix::objetBanc, rempli au chargement de la carte). Puis la vue de course au départ, en qualité Basse et en Haute :
// temps d'une image (rendu logiciel, à comparer entre deux versions) et appels de dessin, écrits au journal.
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <string>
#include "../core/log.h"
#include "../game/game.h"
#include "../game/trace_cartes.h"
#include "../game/track.h"
#include "../render/renderer.h"
#include "decor_carte.h"
#include "decor_grandprix_modeles.h"

namespace wfs {

int benchGrandPrixObjets(Game& g, Renderer& r, const std::string& out) {
    using namespace grandprix;
    int ecarts = 0;
    Track t; t.build(mapById("circuit"), trace::versionCarte("circuit"));
    ObjetBanc ob[NB_OBJETS_BANC];
    if (!g.init(r, "golf", "circuit")) return 1;
    for (int k = 0; k < NB_OBJETS_BANC; k++) ob[k] = objetBanc(k);
    g.shutdown();
    // œil et point visé : (lat, le long en m, hauteur) depuis le pied de l'objet ; lat compté vers l'extérieur de la route
    struct Vue { const char* nom; int sorte; float oLat, oLong, oY, cLat, cLong, cY, fov; bool nuit; };
    const Vue vues[] = {
        {"pneus", OB_PNEUS, 3.2f, -1.5f, 1.4f, 0.3f, 2.4f, 0.4f, 50, true},
        {"publicite", OB_PUB, -6.f, -1.f, 1.6f, 0.f, 6.f, 0.8f, 55, false},
        {"tribune", OB_TRIBUNE, -12.f, -4.f, 3.f, 3.f, 14.f, 3.f, 60, true},
        {"mur-stands", OB_MUR_STANDS, -4.f, -4.f, 1.7f, 0.f, 3.f, 2.f, 55, false},
        {"stands", OB_STAND, -13.f, -4.f, 3.f, 2.f, 10.f, 4.f, 60, true},
        {"drapeau", OB_DRAPEAU, -3.5f, -9.f, 1.8f, 0.f, 1.2f, 6.f, 55, false},
        {"commissaire", OB_COMMISSAIRE, -5.f, -5.f, 2.f, 0.f, 0.f, 1.6f, 50, false},
        {"freinage", OB_FREIN, -4.f, -4.f, 1.6f, 0.f, 0.f, 1.5f, 50, false},
        {"portique", OB_PORTIQUE, 0.f, -25.f, 3.f, 0.f, 0.f, 7.f, 55, true},
        {"passerelle", OB_PASSERELLE, 2.f, -26.f, 3.f, 0.f, 0.f, 5.5f, 55, false},
        {"camion", OB_CAMION, 6.f, -12.f, 2.5f, 0.f, 2.f, 2.f, 55, false},
        {"accueil", OB_ACCUEIL, 28.f, -12.f, 7.f, 7.f, 15.f, 5.f, 60, true},
        {"tour", OB_TOUR, -16.f, -14.f, 8.f, 3.f, 0.f, 13.f, 60, true},
        {"helicoptere", OB_HELICO, -4.f, -6.f, 3.f, 7.f, 1.5f, 1.5f, 55, false},
        {"fans", OB_FANS, -4.5f, -5.f, 2.f, 2.f, 0.f, 1.6f, 55, false},
        {"grande-roue", OB_ROUE, -22.f, -24.f, 6.f, 0.f, 13.f, 12.f, 62, true},
        {"tentes", OB_TENTE, -4.f, -5.f, 2.f, 0.f, 2.f, 0.6f, 55, false},
        {"grillage", OB_GRILLAGE, -3.f, -3.f, 1.5f, 0.f, 2.f, 1.f, 55, false},
        {"projecteur", OB_PROJ, -5.f, -9.f, 14.f, 0.f, 0.f, 18.2f, 55, true},
    };
    for (const char* mo : {"jour", "nuit"}) for (const Vue& v : vues) {
        if (!std::strcmp(mo, "nuit") && !v.nuit) continue;
        const ObjetBanc& o = ob[v.sorte];
        if (o.seg < 0) { ecarts++; WFS_ERR("banc grandprix-objets : aucun objet « %s » sur le tour", v.nom); continue; }
        setenv("WFS_MOMENT", mo, 1);
        setenv("WFS_DEPART", std::to_string(o.seg - (int64_t)(25 / SEG_M)).c_str(), 1);
        if (!g.init(r, "golf", "circuit")) { ecarts++; continue; }
        g.setAutopilot(false); g.camMode = 0; g.allerA(Ecran::Course);
        for (int i = 0; i < 120; i++) { g.update(1.0 / 60); if (i % 30 == 0) g.render(400 + i / 60.0); }
        auto P = [&](float lat, float lg, float y) {
            const TrackPoint q = t.point(((double)o.seg + lg / SEG_M) * SEG, o.side * (o.lat + lat));
            return vec3{(float)q.x, (float)q.y + y, (float)q.z};
        };
        carte::g_vueCarte.oeil = P(v.oLat, v.oLong, v.oY); carte::g_vueCarte.cible = P(v.cLat, v.cLong, v.cY);
        carte::g_vueCarte.fov = v.fov; carte::g_vueCarte.actif = true;
        g.render(404); g.render(404.02);
        const std::string nom = std::string("grandprix-objets-") + v.nom + (!std::strcmp(mo, "nuit") ? "-nuit" : "");
        r.capturePng(out + "/" + nom + ".png");
        carte::g_vueCarte.actif = false;
        WFS_LOG("capture %s : segment %lld, côté %d, %.1f m de l'axe, %u appels de dessin", nom.c_str(), (long long)o.seg, o.side, o.lat, r.appelsDessin());
        g.shutdown();
    }
    // RG-04 : vue de course au départ (stands, tribune, portique), Basse puis Haute : temps d'une image et appels de dessin
    unsetenv("WFS_MOMENT"); unsetenv("WFS_DEPART");
    for (int basse = 1; basse >= 0; basse--) {
        r.qualiteBasse = basse != 0;
        if (!g.init(r, "golf", "circuit")) { ecarts++; continue; }
        g.setAutopilot(true); g.camMode = 0;
        for (int i = 0; i < 300; i++) { g.update(1.0 / 60); if (i % 60 == 0) g.render(400 + i / 60.0); }
        g.render(405);
        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < 4; i++) g.render(405.02 + i * 0.016);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / 4;
        const std::string nom = std::string("grandprix-perf-") + (basse ? "basse" : "haute");
        r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s (WFS-373 RG-04) : %.0f km/h, %.1f ms par image (rendu logiciel lavapipe), %u appels de dessin", nom.c_str(), g.player().kmh(), ms, r.appelsDessin());
        g.shutdown();
    }
    r.qualiteBasse = false;
    WFS_LOG("banc grandprix-objets : %d écart(s) %s", ecarts, ecarts ? "ÉCHEC" : "OK");
    return ecarts;
}

} // namespace wfs
