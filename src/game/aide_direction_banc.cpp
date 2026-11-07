// WaForSpeed — banc de conduite de l'aide à la direction (WFS-217) : voir aide_direction_banc.h. Le joueur simulé conduit
// la vraie partie (Game, voiture du joueur, trafic, décor) par la manette simulée (Game::manette), sur le tracé 2 des cartes.
#include "aide_direction_banc.h"
#include <cstdio>
#include <cstdlib>
#include "../core/log.h"
#include "../render/renderer.h"
#include "course_system.h"
#include "game.h"

namespace wfs {

namespace {
// virage serré : courbe de moins de 45 m de rayon (comme trace::vitesseTrafic)
bool serre(const Track& t, int64_t a) { return std::abs(t.segAt(a).curve) * CURVE_K > SEG_M / 45; }
}

int benchAideDirection(Game& g, Renderer& r, const std::string& out) {
    int ecarts = 0;
    double temps = 5000;
    const bool aideAvant = reglagesConduite().aideDirection, reglesAvant = courseBenchRulesOn();
    struct Cas { const char* carte; trace::Type type; const char* capture; };
    const Cas CAS[] = {{"circuit", trace::Type::Epingle, "conduite-epingle"}, {"campagne", trace::Type::Lacets, nullptr},
                       {"ville", trace::Type::RondPoint, "conduite-rond-point"}, {"plage", trace::Type::RondPoint, nullptr}};
    for (const Cas& c : CAS) {
        static Track t; t.build(mapById(c.carte), trace::versionCarte(c.carte));   // même tracé que la partie (déterministe)
        // première partie serrée de la section (lacets : 1re épingle) : de 80 m avant à 40 m après
        int64_t debut = -1, fin = -1;
        for (const trace::Section& s : trace::sections(t)) {
            if (s.type != c.type) continue;
            for (int64_t a = s.debut; a < s.fin; a++) if (serre(t, a)) { debut = a; break; }
            if (c.type == trace::Type::RondPoint) debut = s.debut;   // entrée du rond-point (courbe de 25 m) comprise
            fin = debut; while (fin < s.fin && (serre(t, fin) || c.type == trace::Type::RondPoint)) fin++;
            break;
        }
        if (debut < 0) { WFS_ERR("aide à la direction : %s sans %s", c.carte, trace::nomType(c.type)); ecarts++; continue; }
        const int64_t depart = std::max<int64_t>(0, debut - (int64_t)(80 / SEG_M));
        const double finU = (fin + (int64_t)(40 / SEG_M)) * SEG, milieu = (debut + fin) / 2 * SEG;
        for (const bool aide : {true, false}) {
            // dans la partie (Game, trafic, décor, rendu) pour les deux captures, aide activée ; sinon voiture seule sur le même
            // tracé (Player, pas de 1/120 s, mêmes commandes toutes les 1/60 s) : le banc tient dans le temps du job de CI
            const bool partie = aide && c.capture;
            reglagesConduite().aideDirection = aide;
            Player seule; seule.reset(carById("golf")); seule.aide = aide ? Aide::Activee : Aide::Desactivee;
            if (partie) {
                setenv("WFS_DEPART", std::to_string(depart).c_str(), 1);
                courseBenchRules(true);
                const bool ok = g.init(r, "golf", c.carte);
                unsetenv("WFS_DEPART");
                if (!ok) { WFS_ERR("aide à la direction : chargement de %s impossible", c.carte); ecarts++; continue; }
                g.setAutopilot(false); g.modeManette = true; g.manette = DriveInput{}; g.camMode = 0;
                g.allerA(Ecran::Course);
                for (int i = 0; i < 60 * 4; i++) { g.update(1.0 / 60); temps += 1.0 / 60; }   // décompte
            } else { seule.dist = depart * SEG; seule.position = t.wrap(seule.dist); }
            const Player& p = partie ? g.player() : seule;
            double psi0 = t.point(p.dist, p.playerX * RW).th + p.heading, sautMax = 0, ecartMax = 0;
            int sorties = 0, chocs = 0, n = 0;
            bool capture = !partie;
            WFS_LOG("aide à la direction %s, %s : %s, segments %lld à %lld (départ %lld)%s", aide ? "activée" : "désactivée", c.carte,
                    trace::nomType(c.type), (long long)debut, (long long)fin, (long long)depart, partie ? ", dans la partie" : ", voiture seule");
            for (; p.dist < finU && n < 60 * 60; n++) {
                const DriveInput in = joueurLigneIdeale(p, t);
                const double lock0 = p.controlLock;
                if (partie) {
                    g.manette = in; g.update(1.0 / 60); temps += 1.0 / 60;
                    if (!capture && p.dist > milieu - 30 / M && n % 4 == 0) g.render(temps);   // route et décor chargés avant la capture
                } else for (int k = 0; k < 2; k++) seule.update(1.0 / 120, in, t, true);
                if (p.controlLock > lock0 + 0.1) chocs++;   // choc avec le trafic : commandes bloquées (hors mesure de l'aide)
                const double psi = t.point(p.dist, p.playerX * RW).th + p.heading, saut = std::abs(std::atan2(std::sin(psi - psi0), std::cos(psi - psi0)));
                psi0 = psi;
                if (p.dist > depart * SEG + 60 / M && p.controlLock <= 0) {   // après la mise en vitesse, hors chocs
                    sautMax = std::max(sautMax, saut); ecartMax = std::max(ecartMax, std::abs(p.playerX));
                    if (std::abs(p.playerX) > 1) sorties++;
                }
                if (n % 30 == 0)
                    WFS_LOG("  trajectoire : %+6.1f m du virage, écart %+5.2f, cap %+5.2f rad, volant %+5.2f, %3.0f km/h", (p.dist - debut * SEG) * M,
                            p.playerX, p.heading, p.steer, p.kmh());
                if (!capture && p.dist >= milieu) {
                    capture = true;
                    g.render(temps); g.render(temps + 0.01); r.capturePng(out + "/" + c.capture + ".png");
                    WFS_LOG("capture %s : aide activée, milieu du virage, écart %+.2f, %.0f km/h", c.capture, p.playerX, p.kmh());
                }
            }
            const bool bon = sorties == 0 && sautMax <= 0.05 && p.dist >= finU;
            if (!bon) ecarts++;
            char ligne[256];
            std::snprintf(ligne, sizeof ligne, "aide à la direction %s, %s %s : écart max %.2f, %d image(s) hors de la route, saut de cap max %.3f rad/image, %d choc(s), %s",
                          aide ? "activée" : "désactivée", c.carte, trace::nomType(c.type), ecartMax, sorties, sautMax, chocs,
                          p.dist >= finU ? "virage passé" : "virage NON passé");
            if (bon) WFS_LOG("%s", ligne); else WFS_ERR("%s", ligne);
            if (partie) { g.manette = DriveInput{}; g.modeManette = false; g.shutdown(); courseBenchRules(reglesAvant); }
        }
    }
    reglagesConduite().aideDirection = aideAvant;
    WFS_LOG("aide à la direction (WFS-217) : banc de conduite, %d écart(s)", ecarts);
    return ecarts;
}

} // namespace wfs
