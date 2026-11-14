// WaForSpeed — suivi des courses pour le profil du joueur (WFS-480, épic E17, zone E3).
// Système de gameplay (230, après la course 210, le duel 220, la course à 5 222 et la poursuite 224) : il lit la course
// en cours (activeCourse) et la voiture du joueur, sans rien modifier, et note chaque course dans le profil (RG-07, RG-10) :
//   - départ : la course passe en Play (fin du compte à rebours) ;
//   - pendant la course : distance réelle de la voiture (|Δ odo|, au mètre près), vitesse maximale du compteur
//     (Course::maxKmh) et frôlements (Course::nearCount, le compteur des messages « frôlement » du HUD) ;
//   - fin : course terminée (Over, avec le résultat donné par le mode) ou quittée après le départ (retour au menu,
//     course remise à zéro). Les menus et le compte à rebours ne comptent pas.
// WFS-498 : au départ d'une course solo avec une voiture bloquée, son essai gratuit est noté.
// WFS-536 (épic E22) : à la fin d'une course terminée, les étoiles du score (score_course.h, WFS-535) sont créditées au
// portefeuille et notées dans l'historique ; course quittée : 0 ★ ; résultat du mode jamais reçu : secours boutique.gain_course ;
// un seul crédit par course (boutique::GainCourse). Records du mode lus au départ (le mode écrit le sien à l'arrivée).
// Rien n'est écrit si la sauvegarde est inactive (banc d'essai hors WFS_BANC=sauvegarde : RG-06).
#include "profil_system.h"
#include <cmath>
#include <ctime>
#include <string>
#include <vector>
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "boutique.h"   // WFS-498 : essai gratuit ; WFS-536 : étoiles du score, un seul crédit par course
#include "cars.h"
#include "course.h"
#include "course5.h"
#include "course_system.h"
#include "duel_system.h"
#include "player.h"
#include "score_course.h"   // WFS-536 : score de la course (WFS-535)
#include "score_fin.h"   // WFS-537 (accroche) : bloc « Score » de l'écran de fin
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
        chocs_ = std::max(0, c->bumps + c->accidents - chocsDepart_);   // WFS-536 : chocs de la voiture du joueur (RG-03 de WFS-535)
        // WFS-536 (RC-E22-04) : duel : un adversaire vu pendant la manche qui disparaît sans être arrivé est parti
        if (mode_ == ModeJoue::Duel && adversaireParti_ < 0) {
            const auto& gh = duelSession().ghosts();
            if (gh.size() > adversairesVus_) adversairesVus_ = gh.size();
            else if (gh.size() < adversairesVus_ && c->state == RaceState::Play) adversaireParti_ = c->raceTime;
        }
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
    int chocs_ = 0, chocsDepart_ = 0;     // WFS-536 : chocs de la course (bumps + accidents depuis le départ)
    double adversaireParti_ = -1;         // WFS-536 : duel : secondes de course au départ d'un adversaire (-1 : aucun)
    size_t adversairesVus_ = 0;
    boutique::RecordsDepart records_;     // WFS-536 : records du mode au départ
    boutique::GainCourse gain_;           // WFS-536 (RG-05) : un seul crédit par course
    std::string voiture_;
    ModeJoue mode_ = ModeJoue::Checkpoint;

    void commencer(const Player& pl, const GameView& g) {
        actif_ = true; g_resultatDonne = false;
        odo_ = pl.odo; metres_ = 0; overT_ = 0; vmax_ = 0; frolements_ = 0;
        const Course* c = activeCourse();
        chocs_ = 0; chocsDepart_ = c ? c->bumps + c->accidents : 0;
        adversaireParti_ = -1; adversairesVus_ = 0;
        records_ = boutique::recordsDepart(sauvegarde());
        gain_.depart();
        score_fin::effacer();   // WFS-537 (accroche) : le bloc de la course d'avant disparaît
        voiture_ = g.car ? g.car->id : "";
        if (duelSession().inRoom()) mode_ = ModeJoue::Duel;
        else switch (modeCourse()) {
            case ModeCourse::Course5: mode_ = ModeJoue::Course5; break;
            case ModeCourse::Poursuite: mode_ = ModeJoue::Poursuite; break;
            case ModeCourse::Duel: mode_ = ModeJoue::Duel; break;
            default: mode_ = ModeJoue::Checkpoint; break;
        }
        // WFS-498 (RG-08) : départ d'une course solo avec une voiture bloquée : son essai gratuit est utilisé (une seule fois)
        Sauvegarde& s = sauvegarde();
        if (s.actif() && mode_ != ModeJoue::Duel && !voiture_.empty() && boutique::marquerEssai(s, voiture_)) s.enregistrer();
    }

    void finir(bool quittee) {
        actif_ = false;
        const bool resultat = g_resultatDonne;
        CourseJouee c = g_resultatDonne && !quittee ? g_resultat : CourseJouee{};
        g_resultatDonne = false;
        c.mode = mode_; c.voiture = voiture_; c.quittee = quittee;
        c.date = (long long)std::time(nullptr);
        c.metres = std::llround(metres_); c.vmax = vmax_; c.frolements = frolements_;
        if (quittee) c.victoire = false;
        // WFS-536 (RG-01, RG-02) : étoiles du score ; résultat du mode jamais reçu : -1 = secours boutique.gain_course
        int etoiles = 0;
        score::Score sc;   // WFS-537 (accroche) : gardé pour le bloc « Score » de l'écran de fin
        if (!quittee) {
            if (resultat) {
                sc = score::calculer(boutique::resultatScore(c, records_, chocs_, adversaireParti_));
                etoiles = sc.etoiles;
                WFS_LOG("score : %s, %d point(s) sur %d ligne(s), %d choc(s) : %d ★%s", nomModeJoue(c.mode), sc.total, (int)sc.lignes.size(), chocs_,
                        sc.etoiles, sc.nouveauRecord ? " (nouveau record)" : "");
            } else {
                etoiles = -1;
            }
        }
        Sauvegarde& s = sauvegarde();
        if (quittee) score_fin::effacer();   // WFS-537 (RG-03, accroche) : course quittée : pas de bloc « Score »
        if (!s.actif()) { if (!quittee) score_fin::publier(resultat ? &sc : nullptr, etoiles, -1); gain_ = {}; return; }   // RG-06 : banc d'essai : rien n'est noté ni écrit
        const int credit = gain_.finir(s, quittee, etoiles);   // RG-05 : une seule fois par course ; quittée : 0
        if (!quittee) score_fin::publier(resultat ? &sc : nullptr, credit, boutique::solde(s));   // WFS-537 (accroche) : ★ vraiment reçues, nouveau solde
        c.etoiles = credit;   // RG-03 : ★ gagnées dans l'historique
        s.profil.noterCourse(c);
        WFS_LOG("profil : course notée (%s, %s, %s, %.3f km, %d km/h, %d frôlement(s)) : %lld course(s), %.1f km au total",
                nomModeJoue(c.mode), c.voiture.c_str(), c.resultat().c_str(), c.metres / 1000.0, c.vmax, c.frolements, s.profil.courses, s.profil.km());
        s.enregistrer();
    }
};

WFS_SYSTEM(ProfilSystem, 230)   // gameplay (200-299) : après la course (210), le duel (220), la course à 5 (222) et la poursuite (224)

} // namespace wfs
