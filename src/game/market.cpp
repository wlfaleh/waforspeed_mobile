// WaForSpeed — market : catalogue, livre des achats, « sans pub », ×2, boutique simulée (WFS-510) : voir market.h.
#include "market.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include "../core/configgame.h"
#include "../core/log.h"
#include "boutique.h"

namespace wfs::market {

namespace {
// noms affichés (FR / EN : RC-E21-08 ; l'écran Market, WFS-513, les prend ici)
const char* const NOMS_FR[MARKET_PRODUITS] = {"Médaille de bronze", "Médaille d'argent", "Médaille d'or", "Coffre de champion", "Pack débutant", "Sans pub"};
const char* const NOMS_EN[MARKET_PRODUITS] = {"Bronze medal", "Silver medal", "Gold medal", "Champion's chest", "Starter pack", "No ads"};
constexpr int DEBUTANT = MARKET_ETOILES, SANS_PUB = MARKET_ETOILES + 1;
} // namespace

// ---------------------------------------------------------------- catalogue (RG-01)
int indexProduit(const std::string& id) {
    for (int i = 0; i < MARKET_PRODUITS; i++) if (id == MARKET_PRODUITS_IDS[i]) return i;
    return -1;
}

long long totalEtoiles(int base, int offert) {
    // entiers seulement : arrondi au plus proche (moitié vers le haut) de base × offert / 100, sans erreur du double
    const long long b = std::max(0, base), o = std::max(0, offert);
    return b + (b * o * 2 + 100) / 200;
}

Pack pack(int i) {
    const auto& c = config().market.pack[i];
    Pack p;
    p.id = MARKET_PRODUITS_IDS[i]; p.nomFr = NOMS_FR[i]; p.nomEn = NOMS_EN[i];
    p.prixSecours = c.prixSecours;
    if (i < MARKET_ETOILES) { p.etoilesBase = c.etoiles; p.offert = c.offert; p.total = totalEtoiles(c.etoiles, c.offert); p.consommable = true; }
    else if (i == DEBUTANT) { p.etoilesBase = c.etoiles; p.total = c.etoiles; p.bonsSkin = config().market.bonsSkinDebutant; }
    return p;
}

std::vector<Pack> catalogue() {
    std::vector<Pack> v;
    for (int i = 0; i < MARKET_PRODUITS; i++) v.push_back(pack(i));
    return v;
}

std::string texteEurosSecours(double euros) { return boutique::texteEuros(std::llround(euros * 100)); }

// ---------------------------------------------------------------- pack débutant (RG-02)
bool packDebutantPropose(const Sauvegarde& s, bool skinsEnVente) {
    return skinsEnVente && !s.market.packDebutant && s.profil.courses >= config().market.debutantApres;
}

// ---------------------------------------------------------------- livre des achats (RG-03, RG-04)
const char* texte(Credit c) {
    switch (c) {
        case Credit::Credite: return "crédité";
        case Credit::DejaCredite: return "déjà crédité";
        case Credit::Inconnu: return "inconnu";
    }
    return "?";
}
const char* texte(Etat e) {
    switch (e) {
        case Etat::Achete: return "acheté";
        case Etat::EnAttente: return "en attente";
        case Etat::Annule: return "annulé";
        case Etat::Erreur: return "erreur";
    }
    return "?";
}

bool dansLivre(const Sauvegarde& s, const std::string& jeton) {
    return std::find(s.market.achats.begin(), s.market.achats.end(), jeton) != s.market.achats.end();
}

Credit crediterAchat(Sauvegarde& s, const std::string& produit, const std::string& jeton) {
    const int i = indexProduit(produit);
    if (i < 0 || !jetonValide(jeton)) {
        WFS_WARN("market : achat refusé (produit « %.40s » %s, jeton %s)", produit.c_str(), i < 0 ? "inconnu" : "connu", jetonValide(jeton) ? "valide" : "mal formé");
        return Credit::Inconnu;
    }
    if (dansLivre(s, jeton)) {
        WFS_LOG("market : %s déjà crédité (jeton déjà dans le livre) : rien de plus", produit.c_str());
        return Credit::DejaCredite;
    }
    Credit r = Credit::Credite;
    const Pack p = pack(i);
    const std::string raison = "achat " + produit;
    if (i < MARKET_ETOILES) boutique::crediter(s, p.total, raison.c_str());
    else if (i == DEBUTANT) {
        if (s.market.packDebutant) r = Credit::DejaCredite;   // une seule fois par joueur, même avec un autre jeton
        else { boutique::crediter(s, p.total, raison.c_str()); s.market.bonsSkin += p.bonsSkin; s.market.packDebutant = true; }
    } else if (i == SANS_PUB) {
        if (s.market.sansPub) r = Credit::DejaCredite;
        s.market.sansPub = true;
    }
    s.market.achats.push_back(jeton);
    if ((int)s.market.achats.size() > EtatMarket::LIVRE_MAX) s.market.achats.erase(s.market.achats.begin(), s.market.achats.end() - EtatMarket::LIVRE_MAX);
    // RC-E21-06 : crédit et jeton dans la même écriture, terminée AVANT que l'appelant consomme / confirme auprès de Google
    if (s.actif()) { s.enregistrer(); s.attendre(); }
    WFS_LOG("market : %s %s (solde %lld ★, %zu jeton(s) au livre)", produit.c_str(), texte(r), s.etoiles, s.market.achats.size());
    return r;
}

Traitement traiterAchat(Sauvegarde& s, const std::string& produit, const std::string& jeton, Etat e) {
    Traitement t;
    switch (e) {
        case Etat::Achete:
            t.credit = crediterAchat(s, produit, jeton);
            t.confirmer = t.credit != Credit::Inconnu;   // déjà crédité : on confirme quand même (sinon Google rembourse sous 3 jours)
            break;
        case Etat::EnAttente:
            t.message = "Achat en attente de paiement";   // rien n'est crédité ; il reviendra « acheté » au rappel
            WFS_LOG("market : %s en attente de paiement : rien de crédité", produit.c_str());
            break;
        case Etat::Annule:
        case Etat::Erreur:
            WFS_LOG("market : %s %s : rien de crédité, retour au market", produit.c_str(), texte(e));
            break;
    }
    return t;
}

// ---------------------------------------------------------------- sans pub (RG-05)
bool sansPub(const Sauvegarde& s) { return s.market.sansPub; }

// ---------------------------------------------------------------- ×2 (RG-06)
int jourLocal(long long secondesUtc) {
    const std::time_t t = (std::time_t)secondesUtc;
    std::tm l{};
#if defined(_WIN32)
    localtime_s(&l, &t);
#else
    localtime_r(&t, &l);
#endif
    return (l.tm_year + 1900) * 10000 + (l.tm_mon + 1) * 100 + l.tm_mday;
}
int aujourdhui() { return jourLocal((long long)std::time(nullptr)); }

int x2Restants(const Sauvegarde& s, int jour) {
    // jour plus récent que le dernier vu : nouveau jour, compteur à zéro ; jour égal ou plus ancien (heure qui recule) : compteur gardé
    const int donnes = jour > s.market.x2Jour ? 0 : s.market.x2Nombre;
    return std::max(0, config().market.x2ParJour - donnes);
}

bool bonusX2Disponible(const Sauvegarde& s, long long course, int jour) {
    return course >= 1 && course != s.market.x2Course && x2Restants(s, jour) > 0;
}

int appliquerX2(Sauvegarde& s, int gainCourse, long long course, int jour) {
    if (gainCourse <= 0 || !bonusX2Disponible(s, course, jour)) {
        WFS_LOG("market : ×2 refusé (course %lld, %d restant(s) aujourd'hui)", course, x2Restants(s, jour));
        return 0;
    }
    if (jour > s.market.x2Jour) { s.market.x2Jour = jour; s.market.x2Nombre = 0; }   // la date la plus récente vue est gardée
    s.market.x2Nombre++;
    s.market.x2Course = course;
    boutique::crediter(s, gainCourse, "×2 de fin de course");
    return gainCourse;
}
int x2SansPub(Sauvegarde& s, int gainCourse, long long course, int jour) { return sansPub(s) ? appliquerX2(s, gainCourse, course, jour) : 0; }
int recompenseVideo(Sauvegarde& s, int gainCourse, long long course, int jour) { return appliquerX2(s, gainCourse, course, jour); }

// ---------------------------------------------------------------- boutique (RG-07)
void brancher(Boutique& b, Sauvegarde& s, std::function<void(const std::string&, const Traitement&)> apres) {
    b.surAchat([&b, &s, apres](const std::string& produit, const std::string& jeton, Etat e) {
        const Traitement t = traiterAchat(s, produit, jeton, e);
        if (t.confirmer) b.confirmer(produit, jeton);   // toujours APRÈS l'écriture de la sauvegarde (crediterAchat)
        if (apres) apres(produit, t);
    });
}

void BoutiqueSimulee::chargerProduits() {
    const auto copie = enCours_;   // le rappel peut confirmer (et retirer) pendant la boucle
    if (!copie.empty()) WFS_LOG("boutique simulée : %zu achat(s) pas encore confirmé(s) renvoyé(s)", copie.size());
    for (const auto& [id, jeton] : copie) signaler(id, jeton, Etat::Achete);
}

std::string BoutiqueSimulee::prix(const std::string& id) const {
    const int i = indexProduit(id);
    return i < 0 ? std::string() : texteEurosSecours(config().market.pack[i].prixSecours);
}

void BoutiqueSimulee::acheter(const std::string& id) {
    if (indexProduit(id) < 0) { signaler(id, "", Etat::Erreur); return; }
    if (reponse_ == Reponse::Annule) { signaler(id, "", Etat::Annule); return; }
    char j[48]; std::snprintf(j, sizeof j, "sim-%s-%d", id.c_str(), ++numero_);
    if (reponse_ == Reponse::EnAttente) { signaler(id, j, Etat::EnAttente); return; }
    enCours_.push_back({id, j});
    coupure_ = reponse_ == Reponse::Interrompu;
    signaler(id, j, Etat::Achete);
    if (doubler_) { doubler_ = false; signaler(id, j, Etat::Achete); }   // Google qui renvoie le même achat deux fois
}

void BoutiqueSimulee::restaurer() {
    for (const auto& [id, jeton] : possedes_) signaler(id, jeton, Etat::Achete);
    chargerProduits();
}

void BoutiqueSimulee::confirmer(const std::string& id, const std::string& jeton) {
    if (coupure_) {   // application coupée entre l'écriture de la sauvegarde et la confirmation
        coupure_ = false;
        WFS_LOG("boutique simulée : application coupée avant la confirmation de %s (%s)", id.c_str(), jeton.c_str());
        return;
    }
    const auto it = std::find(enCours_.begin(), enCours_.end(), std::make_pair(id, jeton));
    if (it == enCours_.end()) return;   // déjà confirmé (même jeton reçu deux fois)
    enCours_.erase(it);
    confirmes_++;
    const int i = indexProduit(id);
    if (i >= MARKET_ETOILES) possedes_[id] = jeton;   // non consommable : rendu par restaurer()
}

std::unique_ptr<Boutique> creerBoutique() {
    const char* e = std::getenv("WFS_BOUTIQUE");
    if (e && std::string(e) == "simulee") return std::make_unique<BoutiqueSimulee>();
    return nullptr;
}

} // namespace wfs::market
