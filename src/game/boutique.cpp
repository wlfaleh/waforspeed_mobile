// WaForSpeed — portefeuille d'étoiles, voitures à acheter, essai gratuit, équivalents et équilibrage (WFS-498) : voir boutique.h.
#include "boutique.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include "../core/configgame.h"
#include "../core/log.h"
#include "cars.h"

namespace wfs::boutique {

namespace {
int indexConfig(const std::string& carId) {   // place dans VOITURES_IDS (configgame) ; −1 si inconnue
    for (int i = 0; i < VOITURES_NB; i++) if (carId == VOITURES_IDS[i]) return i;
    return -1;
}
bool dansCars(const std::string& carId) {
    for (const auto& c : CARS) if (carId == c.id) return true;
    return false;
}
// « 21 780 » : séparateur des milliers en espace (texte du journal)
std::string milliers(long long n) {
    std::string t = std::to_string(n < 0 ? -n : n), r;
    for (size_t i = 0; i < t.size(); i++) { if (i && (t.size() - i) % 3 == 0) r += ' '; r += t[i]; }
    return (n < 0 ? "-" : "") + r;
}
std::string heures(double h) { char b[32]; std::snprintf(b, sizeof b, "%.1f h", h); return b; }
} // namespace

// ---------------------------------------------------------------- portefeuille (RG-01)
long long solde(const Sauvegarde& s) { return s.etoiles; }

void crediter(Sauvegarde& s, long long n, const char* raison) {
    if (n <= 0) return;
    s.etoiles = std::min(SOLDE_MAX, s.etoiles + std::min(n, SOLDE_MAX));   // borné : jamais de débordement
    WFS_LOG("étoiles : +%lld ★ (%s) : solde %lld ★", n, raison ? raison : "?", s.etoiles);
}

bool debiter(Sauvegarde& s, long long n, const char* raison) {
    if (n < 0 || n > s.etoiles) {
        WFS_LOG("étoiles : débit refusé (%s) : %lld ★ demandées, %lld ★ au solde", raison ? raison : "?", n, s.etoiles);
        return false;
    }
    s.etoiles -= n;
    if (n) WFS_LOG("étoiles : −%lld ★ (%s) : solde %lld ★", n, raison ? raison : "?", s.etoiles);
    return true;
}

int crediterCourse(Sauvegarde& s) {
    const int n = config().boutique.gainCourse;
    crediter(s, n, "course terminée");
    return n > 0 ? n : 0;
}

// ---------------------------------------------------------------- équivalents (RG-02)
std::string equivalentMedailles(long long prix) {
    if (prix <= 0) return "gratuit";
    const auto& m = config().boutique.medaille;   // 0 bronze, 1 argent, 2 or
    long long reste = (prix + 99) / 100 * 100;    // à la centaine supérieure
    long long n[BOUTIQUE_MEDAILLES] = {};
    for (int i = BOUTIQUE_MEDAILLES - 1; i >= 0; i--) {   // or, argent, bronze
        const long long e = m[i].etoiles > 0 ? m[i].etoiles : 1;
        n[i] = reste / e; reste -= n[i] * e;
    }
    if (reste > 0) n[0]++;   // packs réglés autrement dans configgame.json : un bronze de plus couvre le reste
    std::string t;
    for (int i = BOUTIQUE_MEDAILLES - 1; i >= 0; i--)
        if (n[i]) t += (t.empty() ? "" : " + ") + std::to_string(n[i]) + " " + BOUTIQUE_MEDAILLES_IDS[i];
    return t;
}

long long equivalentCentimes(long long prix) {
    if (prix <= 0) return 0;
    const auto& b = config().boutique.medaille[0];
    const long long centimesPack = std::llround(b.euros * 100);   // 0,99 € → 99 (exact, sans erreur d'arrondi du double)
    const long long etoiles = b.etoiles > 0 ? b.etoiles : 1;
    return (prix * centimesPack * 2 + etoiles) / (etoiles * 2);   // au centime le plus proche
}

std::string texteEuros(long long c) {
    char b[48]; std::snprintf(b, sizeof b, ",%02lld €", c % 100);
    return milliers(c / 100) + b;
}

// ---------------------------------------------------------------- voitures (RG-03, RG-04, RG-07)
bool voitureVendue(const std::string& carId) { return indexConfig(carId) >= 0 && dansCars(carId) && !voitureRetiree(carId.c_str()); }
int prixVoiture(const std::string& carId) { return voitureVendue(carId) ? config().boutique.prixVoiture[indexConfig(carId)] : -1; }
bool voitureGratuite(const std::string& carId) { return prixVoiture(carId) == 0; }
bool possedeVoiture(const Sauvegarde& s, const std::string& carId) {
    return voitureVendue(carId) && (voitureGratuite(carId) || s.voituresPossedees.count(carId) > 0);
}

const char* texteAchat(AchatVoiture a) {
    switch (a) {
        case AchatVoiture::Achetee: return "achetée";
        case AchatVoiture::DejaPossedee: return "déjà possédée (rien de débité)";
        case AchatVoiture::SoldeInsuffisant: return "étoiles insuffisantes (rien de débité)";
        case AchatVoiture::NonVendue: return "pas en vente (rien de débité)";
    }
    return "?";
}

AchatVoiture acheterVoiture(Sauvegarde& s, const std::string& carId) {
    if (!voitureVendue(carId)) return AchatVoiture::NonVendue;
    if (possedeVoiture(s, carId)) return AchatVoiture::DejaPossedee;
    const std::string raison = "voiture " + carId;
    if (!debiter(s, prixVoiture(carId), raison.c_str())) return AchatVoiture::SoldeInsuffisant;   // un seul débit
    s.voituresPossedees.insert(carId);
    WFS_LOG("boutique : voiture %s achetée : %d ★ (solde %lld ★)", carId.c_str(), prixVoiture(carId), s.etoiles);
    return AchatVoiture::Achetee;
}

perso::Achat acheterObjet(Sauvegarde& s, const perso::Objet& o, const std::string& carId) {
    return perso::acheter(s, s.etoiles, o, carId);   // débit une seule fois, refus sans rien changer (WFS-487)
}

// ---------------------------------------------------------------- essai (RG-08)
bool essaiDisponible(const Sauvegarde& s, const std::string& carId) {
    return voitureVendue(carId) && !possedeVoiture(s, carId) && !s.essais.count(carId);
}
bool marquerEssai(Sauvegarde& s, const std::string& carId) {
    if (!essaiDisponible(s, carId)) return false;
    s.essais.insert(carId);
    WFS_LOG("boutique : essai gratuit de la %s utilisé (une seule course)", carId.c_str());
    return true;
}

// ---------------------------------------------------------------- sauvegardes d'avant (RG-09)
void migrer(Sauvegarde& s) {
    const bool gardee = voitureVendue(s.car) && !possedeVoiture(s, s.car);
    if (gardee) s.voituresPossedees.insert(s.car);
    WFS_LOG("boutique : sauvegarde d'avant le portefeuille : voiture %s %s, solde 0 ★", s.car.c_str(), gardee ? "gardée (possédée)" : "déjà gratuite ou hors vente");
}

// ---------------------------------------------------------------- équilibrage (RG-11)
Equilibrage equilibrage() {
    const auto& b = config().boutique;
    Equilibrage e;
    e.gainCourse = b.gainCourse; e.minutesParCourse = b.minutesParCourse;
    const long long gain = b.gainCourse > 0 ? b.gainCourse : 1;   // gain 0 : compté comme 1 (le banc signale l'écart)
    auto courses = [&](long long etoiles) { return (etoiles + gain - 1) / gain; };
    for (int i = 0; i < VOITURES_NB; i++) {
        const std::string id = VOITURES_IDS[i];
        const int p = prixVoiture(id);
        if (p <= 0) continue;   // gratuite ou pas vendue
        e.nbVoitures++; e.totalVoitures += p;
        if (e.premiereVoiture.empty() || p < e.prixPremiere) { e.premiereVoiture = id; e.prixPremiere = p; }
    }
    e.coursesPremiere = courses(e.prixPremiere);
    e.coursesVoitures = courses(e.totalVoitures);
    e.heuresVoitures = e.coursesVoitures * e.minutesParCourse / 60.0;
    // pour information : tout ce qui se vend, par voiture vendue (gratuites comprises), et les packs de dessins une fois
    long long perso = 0;
    for (const perso::Objet& o : perso::catalogue()) {
        if (perso::cadeau(o)) continue;
        if (o.type == perso::Type::Dessins) { perso += perso::prix(o, "golf"); continue; }
        for (int i = 0; i < VOITURES_NB; i++)
            if (voitureVendue(VOITURES_IDS[i]) && perso::disponible(o, VOITURES_IDS[i])) perso += perso::prix(o, VOITURES_IDS[i]);
    }
    e.totalPersonnalisation = perso;
    e.coursesTout = courses(e.totalVoitures + perso);
    e.heuresTout = e.coursesTout * e.minutesParCourse / 60.0;
    return e;
}

std::vector<std::string> bilanEquilibrage(const Equilibrage& e) {
    std::vector<std::string> v;
    v.push_back("ÉQUILIBRAGE : gain " + std::to_string(e.gainCourse) + " ★ par course, " + [&] { char t[24]; std::snprintf(t, sizeof t, "%.2f", e.minutesParCourse); return std::string(t); }() + " min par course");
    v.push_back("ÉQUILIBRAGE : 1re voiture (" + e.premiereVoiture + ", " + milliers(e.prixPremiere) + " ★) : " + std::to_string(e.coursesPremiere) + " courses (cible 22 à 28)");
    v.push_back("ÉQUILIBRAGE : les " + std::to_string(e.nbVoitures) + " voitures : " + milliers(e.totalVoitures) + " ★, " + milliers(e.coursesVoitures) + " courses, " +
                heures(e.heuresVoitures) + " (cible 108 à 132 h)");
    v.push_back("ÉQUILIBRAGE (information) : peintures, finitions, skins et dessins : " + milliers(e.totalPersonnalisation) + " ★ ; tout : " +
                milliers(e.totalVoitures + e.totalPersonnalisation) + " ★, " + milliers(e.coursesTout) + " courses, " + heures(e.heuresTout));
    v.push_back("ÉQUILIBRAGE : équivalents : " + milliers(e.prixPremiere) + " ★ = " + equivalentMedailles(e.prixPremiere) + " = " + texteEuros(equivalentCentimes(e.prixPremiere)) +
                " ; " + milliers(e.totalVoitures) + " ★ = " + equivalentMedailles(e.totalVoitures) + " = " + texteEuros(equivalentCentimes(e.totalVoitures)));
    return v;
}

std::vector<std::string> verifierEquilibrage(const Equilibrage& e) {
    std::vector<std::string> v;
    if (e.nbVoitures == 0) v.push_back("aucune voiture payante");
    if (e.gainCourse <= 0) v.push_back("gain par course nul");
    if (e.coursesPremiere < 22 || e.coursesPremiere > 28) v.push_back("1re voiture : " + std::to_string(e.coursesPremiere) + " courses, hors de 22 à 28");
    if (e.heuresVoitures < 108 || e.heuresVoitures > 132) v.push_back("toutes les voitures : " + heures(e.heuresVoitures) + ", hors de 108 à 132 h");
    return v;
}

std::vector<std::string> bilanPortefeuille(const Sauvegarde& s) {
    std::string possedees, essais;
    for (int i = 0; i < VOITURES_NB; i++) if (possedeVoiture(s, VOITURES_IDS[i])) possedees += (possedees.empty() ? "" : " ") + std::string(VOITURES_IDS[i]);
    for (const std::string& id : s.essais) essais += (essais.empty() ? "" : " ") + id;
    return {"PORTEFEUILLE : " + milliers(s.etoiles) + " ★",
            "PORTEFEUILLE : voitures possédées : " + (possedees.empty() ? std::string("aucune") : possedees),
            "PORTEFEUILLE : essais gratuits faits : " + (essais.empty() ? std::string("aucun") : essais)};
}

} // namespace wfs::boutique
