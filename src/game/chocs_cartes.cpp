// WaForSpeed — chocs de la voiture du joueur avec les obstacles des cartes (WFS-256, ANO-129) : voir chocs_decor.h.
// Système « chocs-decor » (ordre 99, avant les cartes 100-199) :
//  - init : registre préparé pour la piste ; Ville et îlots des ronds-points déclarés (chocs_declaration.cpp) ; la Plage, la
//    Campagne et le Grand Prix déclarent leurs objets à leur init (accroches de plage.cpp et decor_carte.cpp) ;
//  - collide (après le pas de conduite) : chocs du pas → règles de la course (frottement, petit choc, accident), comme les
//    chocs du trafic (Course::applyHits) ; les sons et vibrations suivent (WFS-28, WFS-70, WFS-255).
#include <cstring>
#include "../core/log.h"
#include "../core/system.h"
#include "chocs_decor.h"
#include "course_system.h"
#include "player.h"

namespace wfs {

class ChocsDecor : public System {
public:
    const char* name() const override { return "chocs-decor"; }
    void init(Renderer&, const GameView& g) override {
        chocs::registre().vider();
        if (!g.track || !g.map) return;
        const chocs::Declares d = chocs::declarerCarte(*g.track, *g.map);
        WFS_LOG("chocs (WFS-256) : %s : %d obstacle(s) de la Ville, %d îlot(s) de rond-point (les autres cartes déclarent leur décor à leur chargement)",
                g.map->id, d.ville, d.ilots);
    }
    void shutdown(Renderer&) override { chocs::registre().vider(); }
    // après le pas de conduite (les systèmes ne sont appelés qu'après Player::update) : règles de la course
    void collide(double dt, Player& pl, const GameView&) override {
        cdFrottement_ -= dt;
        if (pl.chocsDecor.n == 0) return;
        const Player::ChocsDuPas c = pl.chocsDecor;
        pl.chocsDecor = {};
        Course* course = activeCourseMut();
        if (!course || !course->playing()) return;
        HitEvents e;
        const double v = c.impact;   // m/s, vitesse d'approche de l'obstacle
        if (v < 2.0) { if (cdFrottement_ <= 0) { e.scrapes++; cdFrottement_ = 0.18; } }   // frottement (même cadence que le trafic)
        else if (v < 14.0) e.push('b', v / (MAX_SPEED * M));                                 // petit choc (impact comme le trafic : vitesse / vitesse max)
        else e.push('a', v / (MAX_SPEED * M));                                                // accident : 50 km/h et plus contre un obstacle fixe
        course->applyHits(e);
        if (v >= 2.0) WFS_LOG("choc contre %s à %.0f km/h%s", chocs::nomGenre(c.genre), v * 3.6, v >= 14.0 ? " (accident)" : "");
    }
private:
    double cdFrottement_ = 0;
};

} // namespace wfs

WFS_SYSTEM(wfs::ChocsDecor, 99)   // avant les cartes (100-199), qui déclarent leurs objets à leur init
