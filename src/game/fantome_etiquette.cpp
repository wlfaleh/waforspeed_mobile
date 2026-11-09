// WaForSpeed — étiquette « Votre record · 1:12,4 » au-dessus du fantôme du record (WFS-318, RG-03, épic E3) et banc « fantome ».
// Même dessin que les noms des adversaires du duel (duel_system.cpp, nameSprite : fond sombre, liseré cyan, plus petite au
// loin), tailles en dp pour rester lisible sur téléphone (12 à 16 dp) ; opacité selon la place du fantôme
// (fantome::opaciteEtiquette : en fondu au loin et derrière le joueur). Rien n'est dessiné quand le fantôme est caché (RG-02).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include "../core/log.h"
#include "../core/system.h"
#include "../render/renderer.h"
#include "../ui/petit_ecran.h"
#include "../ui/text.h"
#include "course5.h"
#include "course_system.h"
#include "fantome.h"
#include "game.h"
#include "player.h"
#include "sauvegarde.h"

namespace wfs {

namespace {

const vec4 FOND{10 / 255.f, 11 / 255.f, 16 / 255.f, 0.78f};   // nameSprite du duel : rgba(10,11,16,.78)
const vec4 CYAN{0.f, 229 / 255.f, 1.f, 1.f}, TEXTE{242 / 255.f, 244 / 255.f, 248 / 255.f, 1.f}, ETIQ{166 / 255.f, 173 / 255.f, 194 / 255.f, 1.f};

class FantomeEtiquette : public System {
public:
    const char* name() const override { return "fantôme : étiquette"; }
    void drawUi(Renderer& r, const GameView& g) override {
        if (!ui::fontReady() || g.ecran != Ecran::Course || g.sceneDediee || !g.player || !g.track || !fantome::affiche()) return;
        double d = 0, x = 0, duree = 0;
        if (!fantomeCourant(d, x, duree)) return;
        const float a = fantome::opaciteEtiquette((d - g.player->dist) * M);
        if (a <= 0.02f) return;
        const TrackPoint p = g.track->point(d, x * RW);
        const mat4 vp = g.proj * g.view;
        const float wx = (float)p.x, wy = (float)p.y + 1.9f, wz = (float)p.z;   // au-dessus du toit
        const float cx = vp.at(0, 0) * wx + vp.at(0, 1) * wy + vp.at(0, 2) * wz + vp.at(0, 3);
        const float cy = vp.at(1, 0) * wx + vp.at(1, 1) * wy + vp.at(1, 2) * wz + vp.at(1, 3);
        const float cw = vp.at(3, 0) * wx + vp.at(3, 1) * wy + vp.at(3, 2) * wz + vp.at(3, 3);
        if (cw < 0.5f) return;   // derrière la caméra ou trop près
        const float sx = (cx / cw * 0.5f + 0.5f) * g.uiWidth, sy = (cy / cw * 0.5f + 0.5f) * g.uiHeight;
        const float dp = ui::pixelsParDp(g.uiWidth), px = std::clamp(16 * dp * 10.f / cw, 12 * dp, 16 * dp);   // plus petite au loin
        const std::string lib = ui::majuscules("Votre record"), tps = fantome::tempsRecord(duree), sep = "  ·  ";
        const float wl = ui::width(px * 0.8f, lib, 0.12f), wt = ui::width(px, tps, 0.f), ws = ui::width(px, sep, 0.f);
        const float w = wl + ws + wt + px * 1.4f, h = px * 1.6f, x0 = sx - w / 2, y0 = sy - h;
        auto alpha = [&](vec4 c) { c.w *= a; return c; };
        r.uiRect(x0, y0, w, h, alpha(FOND));
        r.uiRect(x0, y0, px * 0.16f, h, alpha(CYAN));   // liseré cyan (comme les noms du duel)
        const float tx = x0 + px * 0.7f;
        ui::text(r, tx, y0 + (h - px * 0.8f) / 2, px * 0.8f, lib, alpha(ETIQ), ui::GAUCHE, 0.12f, false);
        ui::text(r, tx + wl, y0 + (h - px) / 2, px, sep, alpha(ETIQ), ui::GAUCHE, 0.f, false);
        ui::text(r, tx + wl + ws, y0 + (h - px) / 2, px, tps, alpha(CYAN), ui::GAUCHE, 0.f, false, true);
    }
};

} // namespace

WFS_SYSTEM(FantomeEtiquette, 419)   // interface : sous le HUD (420), au-dessus de la scène

// WFS-318 : banc « fantome » : Checkpoint, Ville de nuit, fantôme d'un record de 72,4 s (trace écrite par le banc, à 18 m/s
// sur la file du joueur, parti 20 m devant) : capture avec l'étiquette, puis la même course avec le fantôme caché
void benchFantome(Game& g, Renderer& r, const std::string& out) {
    Sauvegarde& s = sauvegarde();
    const ModeCourse modeAvant = modeCourse();
    modeCourse() = ModeCourse::Checkpoint;
    setenv("WFS_MOMENT", "nuit", 1);
    courseBenchRules(true);
    // 1re partie (sauvegarde vide) : place de départ du joueur
    const std::string dirA = out + "/fantome-a", dirB = out + "/fantome-b";
    std::system(("mkdir -p '" + dirA + "' '" + dirB + "'").c_str());
    std::remove((dirA + "/sauvegarde.txt").c_str()); std::remove((dirB + "/sauvegarde.txt").c_str());
    s.charger(dirA + "/sauvegarde.txt");
    if (!g.init(r, "golf", "ville")) { WFS_ERR("fantôme (WFS-318) : chargement impossible"); s.desactiver(); courseBenchRules(false); return; }
    const double d0 = g.player().dist, x0 = g.player().playerX;
    // trace du record (2e sauvegarde : autre dossier, le fantôme est relu)
    fantome::Trace tr; tr.terrain = "ville"; tr.car = "golf"; tr.peinture = 0x1f5fbf; tr.score = 12450;
    for (int i = 0; i <= 724; i++) tr.e.push_back({d0 + (20.0 + 18.0 * i * fantome::PAS_S) / M, x0, 0.0, 18.0});
    s.charger(dirB + "/sauvegarde.txt");
    {
        const std::string octets = fantome::encoder(tr), f = s.cheminVoisin(fantome::nomFichier("ville"));
        std::FILE* fp = std::fopen(f.c_str(), "wb");
        if (fp) { std::fwrite(octets.data(), 1, octets.size(), fp); std::fclose(fp); }
        WFS_LOG("fantôme (WFS-318) : trace du banc écrite (%s, %zu échantillons, %.1f s)", f.c_str(), tr.e.size(), tr.duree());
    }
    if (!g.init(r, "golf", "ville")) { WFS_ERR("fantôme (WFS-318) : chargement impossible"); s.desactiver(); courseBenchRules(false); return; }
    g.setAutopilot(true); g.camMode = 0;
    double t = 3000;
    for (int i = 0; i < 60 * 3 + 60 * 3; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 30 == 29) g.render(t); }   // décompte puis 3 s
    auto capture = [&](const char* nom, const char* quoi) {
        g.render(t += 0.02); g.render(t += 0.02); r.capturePng(out + "/" + nom + ".png");
        double d = 0, x = 0, du = 0;
        const bool vu = fantomeCourant(d, x, du);
        const double rel = (d - g.player().dist) * M;
        WFS_LOG("capture %s : %s ; fantôme %s, %.0f m devant, opacité de l'étiquette %.2f, « Votre record · %s »", nom, quoi,
                vu && fantome::affiche() ? "affiché" : "caché", rel, fantome::affiche() ? fantome::opaciteEtiquette(rel) : 0.f,
                fantome::tempsRecord(du).c_str());
    };
    capture("fantome-etiquette-nuit", "Checkpoint, Ville de nuit, fantôme du record devant, étiquette au-dessus");
    fantome::affiche() = false;
    capture("fantome-cache-nuit", "même course, réglage « Fantôme du record : Caché »");
    fantome::affiche() = true;
    g.shutdown();
    s.desactiver();
    unsetenv("WFS_MOMENT");
    modeCourse() = modeAvant;
    courseBenchRules(false);
}

} // namespace wfs
