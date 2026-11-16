// WaForSpeed — WFS-496 : logique de l'atelier d'autocollants (voir atelier_calques.h).
#include "atelier_calques.h"
#include <algorithm>
#include <cstring>
#include "../../core/configgame.h"
#include "../../core/log.h"
#include "../../game/personnalisation.h"
#include "../../game/sauvegarde.h"

namespace wfs::ecrans::atelier {

// ---------------------------------------------------------------- motifs et packs
namespace {
// RG-02 : les 10 motifs gratuits (pack « essentiels ») : 2 bandes, 3 numéros, 1 flamme, 1 éclair, 1 étoile, 2 lettres
constexpr const char* GRATUITS[] = {"bande-large", "bande-double", "chiffre-1", "chiffre-2", "chiffre-3",
                                    "flamme-1", "eclair-1", "etoile-1", "lettre-a", "lettre-b"};
bool commencePar(const std::string& s, const char* p) { return s.rfind(p, 0) == 0; }
}

const char* packDuMotif(const std::string& motif) {
    if (!va::motif(motif)) return nullptr;
    for (const char* g : GRATUITS) if (motif == g) return "essentiels";
    // packs de configgame.h (BOUTIQUE_DESSINS_IDS) : « Numéros et lettres », « Bandes », « Flammes et éclairs », « Étoiles et formes »
    if (commencePar(motif, "chiffre-") || commencePar(motif, "lettre-")) return "numeros";
    if (commencePar(motif, "bande-")) return "bandes";
    if (commencePar(motif, "flamme-") || commencePar(motif, "eclair-")) return "flammes";
    return "etoiles";
}

bool motifGratuit(const std::string& motif) { const char* p = packDuMotif(motif); return p && std::strcmp(p, "essentiels") == 0; }
int motifsGratuits() { return (int)(sizeof(GRATUITS) / sizeof(GRATUITS[0])); }

bool motifPermis(const std::string& motif) {
    const char* p = packDuMotif(motif);
    if (!p) return false;
    if (std::strcmp(p, "essentiels") == 0) return true;
    const Sauvegarde& s = sauvegarde();
    if (!s.actif()) return false;
    const perso::Objet* o = perso::objet(p);
    return o && perso::possede(s, *o, "");
}

std::vector<const va::Motif*> motifsDe(va::Famille f) {
    std::vector<const va::Motif*> v;
    for (int gratuit = 1; gratuit >= 0; gratuit--)
        for (const va::Motif& m : va::bibliotheque())
            if (m.famille == f && motifGratuit(m.id) == (gratuit == 1)) v.push_back(&m);
    return v;
}

// ---------------------------------------------------------------- état
bool CalqueAtelier::operator==(const CalqueAtelier& o) const {
    return masque == o.masque && c.motif == o.c.motif && c.dir == o.c.dir && c.x == o.c.x && c.y == o.c.y && c.rotation == o.c.rotation &&
           c.echelle == o.c.echelle && c.couleur == o.c.couleur && c.miroir == o.c.miroir;
}

int ramener(int d) { return ((d % 360) + 540) % 360 - 180; }   // même règle que vehicule_autocollants::lire

void Etat::charger(const std::string& texte, int* rejetes) {
    calques_.clear(); avant_.clear(); apres_.clear(); geste_ = false; modifie_ = false;
    int rej = 0;
    size_t i = 0;
    while (i < texte.size()) {
        size_t f = texte.find(';', i);
        if (f == std::string::npos) f = texte.size();
        std::string item = texte.substr(i, f - i);
        i = f + 1;
        if (item.empty()) continue;
        CalqueAtelier ca;
        if (item[0] == '!') { ca.masque = true; item.erase(0, 1); }
        std::vector<va::Calque> un;
        if ((int)calques_.size() >= MAX_CALQUES || !va::lire(item, un) || un.size() != 1) { rej++; continue; }
        ca.c = un[0];
        calques_.push_back(ca);
    }
    choisi_ = calques_.empty() ? -1 : (int)calques_.size() - 1;
    if (rejetes) *rejetes = rej;
}

std::string Etat::texteGarde() const {
    std::string t;
    for (const CalqueAtelier& ca : calques_) {
        if (!t.empty()) t += ';';
        if (ca.masque) t += '!';
        t += va::texte({ca.c});
    }
    return t;
}

std::string Etat::texteCuisson() const {
    std::vector<va::Calque> v;
    for (const CalqueAtelier& ca : calques_) if (!ca.masque) v.push_back(ca.c);
    return v.empty() ? std::string() : va::texte(v);
}

void Etat::garder() {
    if (geste_) return;   // geste en cours : l'état d'avant est déjà gardé (debutGeste)
    avant_.push_back({calques_, choisi_});
    if ((int)avant_.size() > MAX_HISTORIQUE) avant_.erase(avant_.begin());
    apres_.clear();
    modifie_ = true;
}

bool Etat::ajouter(const std::string& motif, va::Direction d) {
    if ((int)calques_.size() >= MAX_CALQUES || !va::motif(motif)) return false;
    garder();
    CalqueAtelier ca;
    ca.c.motif = motif; ca.c.dir = d;
    calques_.push_back(ca);
    choisi_ = (int)calques_.size() - 1;
    return true;
}

bool Etat::supprimer() {
    if (!calqueChoisi()) return false;
    garder();
    calques_.erase(calques_.begin() + choisi_);
    if (choisi_ >= (int)calques_.size()) choisi_ = (int)calques_.size() - 1;
    return true;
}

bool Etat::monter() {
    if (!calqueChoisi() || choisi_ + 1 >= (int)calques_.size()) return false;
    garder();
    std::swap(calques_[choisi_], calques_[choisi_ + 1]);
    choisi_++;
    return true;
}

bool Etat::descendre() {
    if (!calqueChoisi() || choisi_ == 0) return false;
    garder();
    std::swap(calques_[choisi_], calques_[choisi_ - 1]);
    choisi_--;
    return true;
}

bool Etat::masquer() {
    if (!calqueChoisi()) return false;
    garder();
    calques_[choisi_].masque = !calques_[choisi_].masque;
    return true;
}

bool Etat::tourner(int degres) {
    if (!calqueChoisi() || degres % 360 == 0) return false;
    garder();
    calques_[choisi_].c.rotation = ramener(calques_[choisi_].c.rotation + degres);
    return true;
}

bool Etat::agrandir(int pourcent) {
    if (!calqueChoisi()) return false;
    const int e = std::clamp(calques_[choisi_].c.echelle + pourcent, va::ECHELLE_MIN, va::ECHELLE_MAX);
    if (e == calques_[choisi_].c.echelle) return false;
    garder();
    calques_[choisi_].c.echelle = e;
    return true;
}

bool Etat::miroir() {
    if (!calqueChoisi()) return false;
    garder();
    calques_[choisi_].c.miroir = !calques_[choisi_].c.miroir;
    return true;
}

bool Etat::colorer(const std::string& couleur) {
    if (!calqueChoisi() || !peintureDef(couleur) || calques_[choisi_].c.couleur == couleur) return false;
    garder();
    calques_[choisi_].c.couleur = couleur;
    return true;
}

void Etat::choisir(int i) { if (i >= -1 && i < (int)calques_.size()) choisi_ = i; }

void Etat::debutGeste() {
    if (geste_ || !calqueChoisi()) return;
    debut_ = {calques_, choisi_};
    geste_ = true;
}

void Etat::placer(int xCm, int yCm) {
    if (!calqueChoisi()) return;
    calques_[choisi_].c.x = std::clamp(xCm, -600, 600);
    calques_[choisi_].c.y = std::clamp(yCm, -300, 300);
}

void Etat::transformer(int rotation, int echelle) {
    if (!calqueChoisi()) return;
    calques_[choisi_].c.rotation = ramener(rotation);
    calques_[choisi_].c.echelle = std::clamp(echelle, va::ECHELLE_MIN, va::ECHELLE_MAX);
}

void Etat::finGeste() {
    if (!geste_) return;
    geste_ = false;
    if (debut_.calques == calques_) return;   // rien n'a bougé : pas de pas d'annulation
    avant_.push_back(debut_);
    if ((int)avant_.size() > MAX_HISTORIQUE) avant_.erase(avant_.begin());
    apres_.clear();
    modifie_ = true;
}

bool Etat::annuler() {
    if (avant_.empty() || geste_) return false;
    apres_.push_back({calques_, choisi_});
    calques_ = avant_.back().calques; choisi_ = avant_.back().choisi;
    avant_.pop_back();
    modifie_ = true;
    return true;
}

bool Etat::refaire() {
    if (apres_.empty() || geste_) return false;
    avant_.push_back({calques_, choisi_});
    calques_ = apres_.back().calques; choisi_ = apres_.back().choisi;
    apres_.pop_back();
    modifie_ = true;
    return true;
}

// ---------------------------------------------------------------- sauvegarde
std::string texteGarde(const std::string& carId) {
    const Sauvegarde& s = sauvegarde();
    if (!s.actif()) return {};
    const auto it = s.autocollants.find(carId);
    return it == s.autocollants.end() ? std::string() : it->second;
}

void garder(const std::string& carId, const std::string& texte) {
    Sauvegarde& s = sauvegarde();
    if (!s.actif()) return;
    if (texte.empty()) s.autocollants.erase(carId); else s.autocollants[carId] = texte;
    s.enregistrer();
    WFS_LOG("atelier : %s : habillage gardé (%zu caractères) (WFS-496)", carId.c_str(), texte.size());
}

std::string calquesCourse(const std::string& carId) {
    const std::string t = texteGarde(carId);
    if (t.empty()) return {};
    Etat e;
    e.charger(t);
    std::vector<va::Calque> v;
    for (const CalqueAtelier& ca : e.calques()) if (!ca.masque && motifPermis(ca.c.motif)) v.push_back(ca.c);
    return v.empty() ? std::string() : va::texte(v);
}

} // namespace wfs::ecrans::atelier
