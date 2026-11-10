// WaForSpeed — WFS-372 : banc d'essai des gros plans des objets agricoles modélisés de la Campagne (WFS_BANC=campagne-objets).
// Une vue libre (carte::g_vueCarte, comme la vue haute du col de WFS-216) par sorte d'objet, à hauteur d'homme, de jour, et la
// nuit pour la vigne et la ferme : campagne-objets-vigne, -cloture, -bottes, -ferme, -poteau (+ -nuit). Les objets sont les
// premiers de leur sorte posés sur le tour (campagne::objetBanc, rempli au chargement de la carte).
#include <cstdlib>
#include <cstring>
#include <string>
#include "../core/log.h"
#include "../game/game.h"
#include "../game/trace_cartes.h"
#include "../game/track.h"
#include "../render/renderer.h"
#include "decor_campagne_modeles.h"
#include "decor_carte.h"

namespace wfs {

int benchCampagneObjets(Game& g, Renderer& r, const std::string& out) {
    int ecarts = 0;
    Track t; t.build(mapById("campagne"), trace::versionCarte("campagne"));
    campagne::ObjetBanc ob[campagne::NB_OBJETS_BANC];
    if (!g.init(r, "golf", "campagne")) return 1;
    for (int k = 0; k < campagne::NB_OBJETS_BANC; k++) ob[k] = campagne::objetBanc(k);
    g.shutdown();
    const float R = (float)RW;
    // œil et point visé : (lat, le long en m, hauteur) depuis le pied de l'objet ; lat compté vers l'extérieur de la route
    struct Vue { const char* nom; int sorte; float oLat, oLong, oY, cLat, cLong, cY, fov; bool nuit; };
    const Vue vues[] = {
        {"vigne", campagne::OB_VIGNE, -3.2f, -2.f, 1.6f, 1.2f, 7.f, 0.9f, 55, true},
        {"cloture", campagne::OB_CLOTURE, -1.9f, -1.5f, 1.3f, 0.f, 5.f, 0.6f, 50, false},
        {"bottes", campagne::OB_BOTTES, -6.f, -4.f, 1.8f, 0.f, 3.f, 0.7f, 50, false},
        {"ferme", campagne::OB_FERME, -24.f, -10.f, 3.5f, 0.f, 8.f, 3.5f, 55, true},
        {"poteau", campagne::OB_POTEAU, -R - 2.6f, -14.f, 1.6f, 0.f, 0.f, 6.5f, 50, false},
    };
    for (const char* mo : {"jour", "nuit"}) for (const Vue& v : vues) {
        if (!std::strcmp(mo, "nuit") && !v.nuit) continue;
        const campagne::ObjetBanc& o = ob[v.sorte];
        if (o.seg < 0) { ecarts++; WFS_ERR("banc campagne-objets : aucun objet « %s » sur le tour", v.nom); continue; }
        setenv("WFS_MOMENT", mo, 1);
        setenv("WFS_DEPART", std::to_string(o.seg - (int64_t)(25 / SEG_M)).c_str(), 1);
        if (!g.init(r, "golf", "campagne")) { ecarts++; continue; }
        g.setAutopilot(false); g.camMode = 0; g.allerA(Ecran::Course);
        for (int i = 0; i < 120; i++) { g.update(1.0 / 60); if (i % 30 == 0) g.render(400 + i / 60.0); }
        auto P = [&](float lat, float lg, float y) {
            const TrackPoint q = t.point(((double)o.seg + lg / SEG_M) * SEG, o.side * (o.lat + lat));
            return vec3{(float)q.x, (float)q.y + y, (float)q.z};
        };
        carte::g_vueCarte.oeil = P(v.oLat, v.oLong, v.oY); carte::g_vueCarte.cible = P(v.cLat, v.cLong, v.cY);
        carte::g_vueCarte.fov = v.fov; carte::g_vueCarte.actif = true;
        g.render(404); g.render(404.02);
        const std::string nom = std::string("campagne-objets-") + v.nom + (!std::strcmp(mo, "nuit") ? "-nuit" : "");
        r.capturePng(out + "/" + nom + ".png");
        carte::g_vueCarte.actif = false;
        WFS_LOG("capture %s : segment %lld, côté %d, %.1f m de l'axe, %u appels de dessin", nom.c_str(), (long long)o.seg, o.side, o.lat, r.appelsDessin());
        g.shutdown();
    }
    unsetenv("WFS_MOMENT"); unsetenv("WFS_DEPART");
    WFS_LOG("banc campagne-objets : %d écart(s) %s", ecarts, ecarts ? "ÉCHEC" : "OK");
    return ecarts;
}

} // namespace wfs
