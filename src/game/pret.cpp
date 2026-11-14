// WaForSpeed — voiture prêtée par le cadeau du jour (WFS-502) : voir pret.h.
#include "pret.h"
#include <algorithm>
#include <cmath>
#include "../core/configgame.h"
#include "../core/log.h"
#include "boutique.h"
#include "cadeau.h"

namespace wfs::pret {

namespace {
long long g_heureForcee = 0;   // banc d'essai seulement

// durées en secondes, lues dans configgame.json (RC-E19-02), au moins 1 s
long long dureePret() { return std::max(1LL, (long long)std::llround(config().cadeau.pretHeures * 3600.0)); }
long long delaiActivation() { return std::max(1LL, (long long)std::llround(config().cadeau.activationHeures * 3600.0)); }

void vider(EtatCadeau& c) { c.pretVoiture.clear(); c.pretRecu = c.pretFin = c.pretRestantVu = 0; c.pretAvant.clear(); }

// RG-05 : voiture d'avant le prêt, si elle est encore possédée ; sinon la 1re voiture gratuite (jamais une voiture bloquée)
std::string voitureDeRepli(const Sauvegarde& s, const std::string& pretee) {
    if (!s.cadeau.pretAvant.empty() && s.cadeau.pretAvant != pretee && boutique::possedeVoiture(s, s.cadeau.pretAvant)) return s.cadeau.pretAvant;
    for (const char* id : VOITURES_IDS) if (id != pretee && boutique::possedeVoiture(s, id)) return id;
    return s.car;   // aucune autre (impossible : il y a toujours une voiture gratuite) : inchangée
}
} // namespace

Etat etat(const EtatCadeau& c) {
    if (c.pretVoiture.empty()) return Etat::Aucun;
    return c.pretFin > 0 ? Etat::Actif : Etat::EnAttente;
}

bool utilisable(const Sauvegarde& s, const std::string& carId) {
    return !carId.empty() && s.cadeau.pretVoiture == carId && etat(s.cadeau) == Etat::Actif;
}

long long resteActivation(const EtatCadeau& c, long long t) {
    if (etat(c) != Etat::EnAttente) return 0;
    const long long ecoule = std::max(0LL, t - c.pretRecu);   // heure reculée : le délai ne s'allonge pas au-delà de 24 h
    return std::max(0LL, delaiActivation() - ecoule);
}

long long restant(const EtatCadeau& c, long long t) {
    if (etat(c) != Etat::Actif) return 0;
    long long r = c.pretFin - t;
    if (c.pretRestantVu > 0) r = std::min(r, c.pretRestantVu);   // RG-04 : jamais plus que le dernier temps vu
    return std::max(0LL, r);
}

void recevoir(EtatCadeau& c, const std::string& voiture, long long t) {
    vider(c);
    c.pretVoiture = voiture;
    c.pretRecu = t;
    WFS_LOG("prêt (WFS-502) : %s gagnée, à activer dans les %g h", voiture.c_str(), config().cadeau.activationHeures);
}

bool activer(Sauvegarde& s, long long t) {
    mettreAJour(s, t);   // délai dépassé : le prêt est perdu avant tout
    EtatCadeau& c = s.cadeau;
    if (etat(c) != Etat::EnAttente) return false;
    const long long d = dureePret();
    c.pretFin = t + d;
    c.pretRestantVu = d;
    c.pretAvant = s.car != c.pretVoiture ? s.car : std::string();
    WFS_LOG("prêt (WFS-502) : %s activée pour %g h (fin à %lld), voiture d'avant : %s", c.pretVoiture.c_str(), config().cadeau.pretHeures, c.pretFin,
            c.pretAvant.empty() ? "—" : c.pretAvant.c_str());
    return true;
}

Evenement mettreAJour(Sauvegarde& s, long long t, bool enCourse) {
    EtatCadeau& c = s.cadeau;
    switch (etat(c)) {
        case Etat::Aucun: return Evenement::Rien;
        case Etat::EnAttente: {
            if (enCourse || resteActivation(c, t) > 0) return Evenement::Rien;
            WFS_LOG("prêt (WFS-502) : %s pas activée sous %g h : prêt perdu", c.pretVoiture.c_str(), config().cadeau.activationHeures);
            c.pretMessage = "expire;" + c.pretVoiture;
            vider(c);
            return Evenement::Expire;
        }
        case Etat::Actif: break;
    }
    Evenement e = Evenement::Rien;
    // RG-04 : temps restant jamais augmenté ; heure reculée → fin recalée sur maintenant + dernier temps vu
    const long long r = restant(c, t);
    if (c.pretFin - t > r) { c.pretFin = t + r; e = Evenement::Change; WFS_LOG("prêt (WFS-502) : heure du téléphone reculée : il reste %lld s, pas plus", r); }
    if (r != c.pretRestantVu) { c.pretRestantVu = r; if (e == Evenement::Rien) e = Evenement::Change; }
    // voiture d'avant : la dernière voiture possédée choisie pendant le prêt
    if (s.car != c.pretVoiture && s.car != c.pretAvant && boutique::possedeVoiture(s, s.car)) { c.pretAvant = s.car; if (e == Evenement::Rien) e = Evenement::Change; }
    if (r > 0 || enCourse) return e;
    // RG-05 : fin du prêt (hors course)
    const std::string id = c.pretVoiture;
    if (s.car == id) s.car = voitureDeRepli(s, id);
    WFS_LOG("prêt (WFS-502) : fin du prêt de la %s ; voiture choisie : %s ; « Garde-la pour toujours : %d ★ »", id.c_str(), s.car.c_str(), boutique::prixVoiture(id));
    c.pretMessage = "fin;" + id;
    vider(c);
    return Evenement::Fini;
}

Message message(const EtatCadeau& c, std::string* voiture) {
    const size_t p = c.pretMessage.find(';');
    if (p == std::string::npos) return Message::Aucun;
    if (voiture) *voiture = c.pretMessage.substr(p + 1);
    const std::string t = c.pretMessage.substr(0, p);
    return t == "expire" ? Message::Expire : t == "fin" ? Message::Fin : Message::Aucun;
}

void oublierMessage(EtatCadeau& c) { c.pretMessage.clear(); }

long long maintenant() { return g_heureForcee > 0 ? g_heureForcee : cadeau::maintenant().epoch; }
void forcerHeure(long long epoch) { g_heureForcee = std::max(0LL, epoch); }

} // namespace wfs::pret
