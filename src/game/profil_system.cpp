// WaForSpeed — suivi des courses pour le profil du joueur (WFS-480, épic E17, zone E3).
// Système de gameplay (230, après la course 210, le duel 220, la course à 5 222 et la poursuite 224) : il lit la course
// en cours (activeCourse) et la voiture du joueur, sans rien modifier, et note chaque course dans le profil (RG-07, RG-10) :
//   - départ : la course passe en Play (fin du compte à rebours) ;
//   - pendant la course : distance réelle de la voiture (|Δ odo|, au mètre près), vitesse maximale du compteur
//     (Course::maxKmh) et frôlements (Course::nearCount, le compteur des messages « frôlement » du HUD) ;
//   - fin : course terminée (Over, avec le résultat donné par le mode) ou quittée après le départ (retour au menu,
//     course remise à zéro). Les menus et le compte à rebours ne comptent pas.
// Rien n'est écrit si la sauvegarde est inactive (banc d'essai hors WFS_BANC=sauvegarde : RG-06).
#include "profil_system.h"
#include <cmath>
#include <ctime>
#include <string>
#include <vector>
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "cars.h"
#include "course.h"
#include "course5.h"
#include "course_system.h"
#include "duel_system.h"
#include "player.h"
#include "sauvegarde.h"
#include "track.h"

namespace wfs {

namespace {
CourseJouee g_resultat;          // résultat donné par le mode pour la course en cours
bool g_resultatDonne = false;
}

void profilNoterResultat(const CourseJouee& r) { g_resultat = r; g_resultatDonne = true; }

class ProfilSystem final : public System {
public:
    const char* name() const override { return "profil"; }

    void init(Renderer&, const GameView&) override {
        static bool charge = false;   // RG-04 : liste des mots interdits, lue une fois par lancement
        if (charge) return;
        charge = true;
        std::vector<uint8_t> octets;
        if (readAsset("profil/mots_interdits.txt", octets)) {
            const int n = profil::chargerMotsInterdits(std::string(octets.begin(), octets.end()));
            WFS_LOG("profil : %d mots interdits chargés", n);
        } else {
            WFS_ERR("profil : assets/profil/mots_interdits.txt introuvable : aucun mot refusé");
        }
    }

    void collide(double dt, Player& pl, const GameView& g) override {
        const Course* c = g.ecran == Ecran::Course ? activeCourse() : nullptr;
        if (!c) return;
        if (actif_ && c->state == RaceState::Count) { finir(true); return; }   // course remise à zéro avant la fin : quittée
        if (!actif_ && c->state == RaceState::Play) commencer(pl, g);
        if (!actif_) return;
        const double d = std::fabs(pl.odo - odo_) * M;   // distance réelle de la voiture (sens indifférent)
        if (d < 5.0) metres_ += d;                         // > 5 m en un pas (600 m/s) : voiture replacée, pas un trajet
        odo_ = pl.odo;
        vmax_ = std::max(vmax_, c->maxKmh);
        frolements_ = c->nearCount;
        if (c->state == RaceState::Over) {
            overT_ += dt;
            if (g_resultatDonne || overT_ > 0.5) finir(false);   // résultat du mode (donné à l'arrivée), au plus 0,5 s après
        }
    }

    void onEcran(Ecran, Ecran nouveau, const GameView&) override {
        if (actif_ && nouveau != Ecran::Course && nouveau != Ecran::Pause) finir(true);   // quittée après le départ
    }

private:
    bool actif_ = false;
    double odo_ = 0, metres_ = 0, overT_ = 0;
    int vmax_ = 0, frolements_ = 0;
    std::string voiture_;
    ModeJoue mode_ = ModeJoue::Checkpoint;

    void commencer(const Player& pl, const GameView& g) {
        actif_ = true; g_resultatDonne = false;
        odo_ = pl.odo; metres_ = 0; overT_ = 0; vmax_ = 0; frolements_ = 0;
        voiture_ = g.car ? g.car->id : "";
        if (duelSession().inRoom()) mode_ = ModeJoue::Duel;
        else switch (modeCourse()) {
            case ModeCourse::Course5: mode_ = ModeJoue::Course5; break;
            case ModeCourse::Poursuite: mode_ = ModeJoue::Poursuite; break;
            case ModeCourse::Duel: mode_ = ModeJoue::Duel; break;
            default: mode_ = ModeJoue::Checkpoint; break;
        }
    }

    void finir(bool quittee) {
        actif_ = false;
        CourseJouee c = g_resultatDonne && !quittee ? g_resultat : CourseJouee{};
        g_resultatDonne = false;
        c.mode = mode_; c.voiture = voiture_; c.quittee = quittee;
        c.date = (long long)std::time(nullptr);
        c.metres = std::llround(metres_); c.vmax = vmax_; c.frolements = frolements_;
        if (quittee) c.victoire = false;
        Sauvegarde& s = sauvegarde();
        if (!s.actif()) return;   // RG-06 : banc d'essai : rien n'est noté ni écrit
        s.profil.noterCourse(c);
        WFS_LOG("profil : course notée (%s, %s, %s, %.3f km, %d km/h, %d frôlement(s)) : %lld course(s), %.1f km au total",
                nomModeJoue(c.mode), c.voiture.c_str(), c.resultat().c_str(), c.metres / 1000.0, c.vmax, c.frolements, s.profil.courses, s.profil.km());
        s.enregistrer();
    }
};

WFS_SYSTEM(ProfilSystem, 230)   // gameplay (200-299) : après la course (210), le duel (220), la course à 5 (222) et la poursuite (224)

} // namespace wfs
