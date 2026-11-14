// WaForSpeed — voiture prêtée (WFS-502, zone E3) : système « pret » qui applique pret::mettreAJour hors course (voir pret.h).
//   - RG-01 / RG-05 : délai d'activation dépassé ou fin du prêt → appliqués au menu, au garage, au terrain, au salon… jamais en
//     course ni en pause (une course commencée avant la fin se termine normalement, gains comptés) ;
//   - RG-04 : le temps restant vu est gardé (sauvegarde écrite au plus une fois par minute pendant le prêt) ;
//   - RG-05 : à la fin, si la voiture de la partie est la voiture prêtée, la partie reprend la voiture rendue par pret (voiture
//     d'avant), avec sa peinture de course.
// Sauvegarde inactive (bancs d'essai des autres tickets, parcours de la CI) : rien ne se passe.
#include <cmath>
#include "pret.h"
#include "../core/log.h"
#include "../core/system.h"
#include "cars.h"

namespace wfs {
namespace {

class PretSysteme final : public System {
public:
    const char* name() const override { return "pret"; }
    void update(double, const GameView& g) override {
        Sauvegarde& s = sauvegarde();
        if (!s.actif() || pret::etat(s.cadeau) == pret::Etat::Aucun) return;
        const long long t = pret::maintenant();
        const bool enCourse = g.ecran == Ecran::Course || g.ecran == Ecran::Pause;
        if (t == dernier_ && enCourse == dernierEnCourse_) return;   // une fois par seconde au plus, et à chaque sortie de course
        dernier_ = t; dernierEnCourse_ = enCourse;
        const std::string pretee = s.cadeau.pretVoiture;
        const pret::Evenement e = pret::mettreAJour(s, t, enCourse);
        if (e == pret::Evenement::Rien) return;
        // RG-06 : fin, perte ou recul de l'heure écrits tout de suite ; temps restant vu : une fois par minute (pas d'écriture par seconde)
        if (e != pret::Evenement::Change || t - ecrit_ >= 60) { s.enregistrer(); ecrit_ = t; }
        if (e == pret::Evenement::Fini && g.car && pretee == g.car->id && g.choisirVoiture) {
            const PeintureDef* p = peintureDef(s.peinture(s.car));   // même calcul que la fin d'un essai gratuit (garage_achat_voiture.cpp)
            const unsigned h = p ? p->hex : 0xFFFFFF;
            auto c = [&](int sh) { return std::pow(((h >> sh) & 255) / 255.f, 2.2f); };
            WFS_LOG("prêt (WFS-502) : la partie reprend la %s", s.car.c_str());
            g.choisirVoiture(s.car.c_str(), vec3{c(16), c(8), c(0)});
        }
    }
private:
    long long dernier_ = 0, ecrit_ = 0;
    bool dernierEnCourse_ = false;
};

} // namespace

WFS_SYSTEM(PretSysteme, 206)   // gameplay (200-299) : juste après l'achat de fin d'essai (205), avant la course (210)

} // namespace wfs
