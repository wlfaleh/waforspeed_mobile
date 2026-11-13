// WaForSpeed — WFS-535 : score d'une course et conversion en étoiles (voir score_course.h).
#include "score_course.h"
#include <algorithm>
#include <cmath>

namespace wfs::score {

namespace {
int arrondi(double v) { return (int)std::floor(v + 0.5); }   // demi vers le haut, sans dépendre du mode d'arrondi
}

Score calculer(const ResultatCourse& r, const ConfigJeu::ScoreEtoiles& b) {
    Score s;
    if (!r.terminee) return s;   // RG-05 : course quittée : pas de score, 0 ★
    // RC-E22-04 : duel dont l'adversaire part avant le délai : aucun gain (pas de partie arrangée)
    if (r.mode == ModeJoue::Duel && r.adversaireParti >= 0 && r.adversaireParti < b.duelDepartMinS) return s;
    auto ajouter = [&](const char* cle, int points) { if (points > 0) { s.lignes.push_back({cle, points}); s.total += points; } };

    // RG-01 : place ou issue
    switch (r.mode) {
        case ModeJoue::Course5:
            if (r.place >= 1 && r.place <= 5) ajouter("score.place", b.place[r.place - 1]);
            break;
        case ModeJoue::Duel:
            ajouter(r.victoire ? "score.duel_gagne" : "score.duel_perdu", r.victoire ? b.duelGagne : b.duelPerdu);
            break;
        case ModeJoue::Poursuite:
            if (r.victoire) ajouter("score.poursuite_victoire", b.poursuiteVictoire);
            else {
                ajouter("score.poursuite_defaite", b.poursuiteDefaite);
                if (r.scorePoursuite > 0) ajouter("score.poursuite_score_restant", arrondi(b.poursuitePartScore * r.scorePoursuite));
            }
            break;
        case ModeJoue::Checkpoint:
            ajouter("score.solo_terminee", b.soloTerminee);
            // plafond appliqué en réel, avant la conversion en entier : un compteur énorme ne déborde pas
            if (r.pointsDistance > 0) ajouter("score.solo_distance", arrondi(std::min<double>(b.soloPlafondDistance, b.soloPartDistance * (double)r.pointsDistance)));
            break;
    }

    // RG-02 : temps contre le record du mode et de la carte
    if (r.record <= 0) ajouter("score.premiere_course", b.premiereCourse);
    else if (r.temps > 0 && r.temps < r.record) { ajouter("score.record_battu", b.recordBattu); s.nouveauRecord = true; }
    else if (r.temps > 0 && r.temps <= r.record * (1.0 + b.procheRecordPart)) ajouter("score.proche_record", b.procheRecord);

    // RG-03 : conduite propre
    if (r.chocs == 0) ajouter("score.zero_choc", b.propreZero);
    else if (r.chocs > 0 && r.chocs <= b.proprePeuMax) ajouter("score.peu_chocs", b.proprePeu);

    // RG-04 : 2 défis par mode (compteur inconnu : 0, sans erreur)
    switch (r.mode) {
        case ModeJoue::Course5:
            if (r.depassements > 0) ajouter("score.defi_depassements", (int)std::min<long long>(b.c5DepassementMax, (long long)r.depassements * b.c5Depassement));
            if (r.enTeteDernierTour == Defi::Reussi) ajouter("score.defi_en_tete", b.c5EnTete);
            break;
        case ModeJoue::Poursuite:
            if (r.contactsPolice == 0) ajouter("score.defi_sans_police", b.poSansPolice);
            if (r.victoire && r.scorePoursuite >= 0 && r.scorePoursuiteDepart > 0 && r.scorePoursuite >= b.poScoreGardePart * r.scorePoursuiteDepart)
                ajouter("score.defi_score_garde", b.poScoreGarde);
            break;
        case ModeJoue::Duel:
            if (r.jamaisDerriere == Defi::Reussi) ajouter("score.defi_jamais_derriere", b.duJamaisDerriere);
            if (r.victoire && r.ecartArrivee > b.duEcartS) ajouter("score.defi_ecart", b.duEcart);
            break;
        case ModeJoue::Checkpoint:
            if (r.checkpointsSansRetard == Defi::Reussi) ajouter("score.defi_sans_retard", b.soSansRetard);
            if (r.nitroJamaisVide == Defi::Reussi) ajouter("score.defi_nitro", b.soNitro);
            break;
    }

    // RG-05 : conversion en étoiles, bornée pour une course terminée
    const int ppe = std::max(1, b.pointsParEtoile);
    s.etoiles = std::clamp(arrondi((double)s.total / ppe), b.etoilesMin, std::max(b.etoilesMin, b.etoilesMax));
    return s;
}

} // namespace wfs::score
