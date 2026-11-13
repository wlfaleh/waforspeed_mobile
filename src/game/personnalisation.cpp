// WaForSpeed — catalogue de personnalisation (WFS-487) : voir personnalisation.h.
#include "personnalisation.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sstream>
#include "../core/configgame.h"
#include "../core/log.h"
#include "cars.h"

namespace wfs::perso {

static_assert(NB_PEINTURES == BOUTIQUE_COULEURS, "WFS-487 : une entrée boutique.couleurs par peinture (même ordre)");

namespace {
// noms affichés (le français ; l'anglais viendra avec les écrans, WFS-504)
const char* NOMS_FINITIONS[BOUTIQUE_FINITIONS] = {"Brillant", "Métallisé", "Nacré", "Mat", "Satiné", "Candy", "Chromé", "Caméléon"};
const char* NOMS_SKINS[BOUTIQUE_SKINS] = {"Rallye", "Drift Tokyo", "Néon nuit", "Camouflage"};
const char* NOMS_DESSINS[BOUTIQUE_DESSINS] = {"Essentiels", "Numéros et lettres", "Bandes", "Flammes et éclairs", "Étoiles et formes"};

std::vector<Objet> fabriquer() {
    std::vector<Objet> v;
    for (int i = 0; i < NB_PEINTURES; i++) v.push_back({Type::Couleur, PEINTURES[i].id, PEINTURES[i].nom, i});
    v.push_back({Type::Secondaire, ID_SECONDAIRE, "Couleur secondaire", 0});
    for (int i = 0; i < BOUTIQUE_FINITIONS; i++) v.push_back({Type::Finition, BOUTIQUE_FINITIONS_IDS[i], NOMS_FINITIONS[i], i});
    for (int i = 0; i < BOUTIQUE_SKINS; i++) v.push_back({Type::Skin, BOUTIQUE_SKINS_IDS[i], NOMS_SKINS[i], i});
    for (int i = 0; i < BOUTIQUE_DESSINS; i++) v.push_back({Type::Dessins, BOUTIQUE_DESSINS_IDS[i], NOMS_DESSINS[i], i});
    return v;
}

int indexVoiture(const std::string& carId) {
    for (int i = 0; i < VOITURES_NB; i++) if (carId == VOITURES_IDS[i]) return i;
    return -1;
}
bool voitureConnue(const std::string& carId) { return indexVoiture(carId) >= 0; }

// prix de base, gratuit, cadeau de l'objet (config().boutique)
ConfigJeu::Boutique::Objet regle(const Objet& o) {
    const ConfigJeu::Boutique& b = config().boutique;
    switch (o.type) {
        case Type::Couleur: return b.couleur[o.index];
        case Type::Secondaire: return {b.couleurSecondaire, 0, 0};
        case Type::Finition: return b.finition[o.index];
        case Type::Skin: return b.skin[o.index];
        case Type::Dessins: return b.dessins[o.index];
    }
    return {0, 0, 0};
}

// la skin existe pour cette voiture : identifiant présent dans boutique.skins.<id>.voitures (séparés par des espaces)
bool skinSurVoiture(int index, const std::string& carId) {
    std::istringstream in(config().boutique.skinVoitures[index]);
    std::string id;
    while (in >> id) if (id == carId) return true;
    return false;
}

const char* valeurOuVide(const std::map<std::string, std::string>& m, const std::string& k) {
    const auto it = m.find(k);
    return it == m.end() ? "" : it->second.c_str();
}
} // namespace

const char* nomType(Type t) {
    switch (t) {
        case Type::Couleur: return "couleur";
        case Type::Secondaire: return "couleur secondaire";
        case Type::Finition: return "finition";
        case Type::Skin: return "skin";
        case Type::Dessins: return "autocollants";
    }
    return "?";
}

const std::vector<Objet>& catalogue() {
    static const std::vector<Objet> c = fabriquer();
    return c;
}
const Objet* objet(const std::string& id) {
    for (const Objet& o : catalogue()) if (id == o.id) return &o;
    return nullptr;
}
const Objet* couleur(const std::string& id) {
    const Objet* o = objet(id);
    return o && o->type == Type::Couleur ? o : nullptr;
}

int classeVoiture(const std::string& carId) {
    const int i = indexVoiture(carId);
    return i < 0 ? 0 : config().boutique.classe[i];
}
double coefficient(const std::string& carId) { return config().boutique.coefficient[classeVoiture(carId)]; }

bool gratuit(const Objet& o, const std::string& carId) {
    if (regle(o).gratuit) return true;
    // RG-03 : la couleur d'origine d'une voiture est toujours gratuite pour elle
    return o.type == Type::Couleur && voitureConnue(carId) && std::strcmp(o.id, peintureOrigine(carId)) == 0;
}
bool cadeau(const Objet& o) { return regle(o).cadeau != 0; }
bool disponible(const Objet& o, const std::string& carId) {
    if (o.type == Type::Dessins) return true;   // pour toutes les voitures
    if (!voitureConnue(carId) || voitureRetiree(carId.c_str())) return false;
    return o.type != Type::Skin || skinSurVoiture(o.index, carId);
}
int prix(const Objet& o, const std::string& carId) {
    if (gratuit(o, carId)) return 0;
    const int base = regle(o).prix;
    if (o.type == Type::Dessins) return base;   // sans coefficient (RC-E18-11)
    return (int)std::lround(base * coefficient(carId));   // arrondi à l'étoile la plus proche (WFS-498 RG-05)
}

bool possede(const Sauvegarde& s, const Objet& o, const std::string& carId) {
    if (o.type == Type::Dessins) return regle(o).gratuit || s.possedeDessins.count(o.id) > 0;
    if (!disponible(o, carId)) return false;
    if (gratuit(o, carId)) return true;
    const auto it = s.possede.find(carId);
    return it != s.possede.end() && it->second.count(o.id) > 0;
}

const char* texteAchat(Achat a) {
    switch (a) {
        case Achat::Achete: return "acheté";
        case Achat::DejaPossede: return "déjà possédé (rien de débité)";
        case Achat::SoldeInsuffisant: return "étoiles insuffisantes (rien de débité)";
        case Achat::NonVendu: return "pas en vente (rien de débité)";
    }
    return "?";
}

Achat acheter(Sauvegarde& s, long long& solde, const Objet& o, const std::string& carId) {
    if (possede(s, o, carId)) return Achat::DejaPossede;
    if (!disponible(o, carId) || cadeau(o)) return Achat::NonVendu;
    const int p = prix(o, carId);
    if (solde < p) {
        WFS_LOG("boutique : achat refusé (%s %s, %s) : %d ★ demandées, %lld ★ au solde", nomType(o.type), o.id, carId.c_str(), p, solde);
        return Achat::SoldeInsuffisant;
    }
    solde -= p;   // un seul débit (RG-05)
    if (o.type == Type::Dessins) s.possedeDessins.insert(o.id);
    else s.possede[carId].insert(o.id);
    WFS_LOG("boutique : %s %s acheté pour %s : %d ★ (solde %lld ★)", nomType(o.type), o.id, o.type == Type::Dessins ? "toutes les voitures" : carId.c_str(), p, solde);
    return Achat::Achete;
}

Aspect aspectGarage(const Sauvegarde& s, const std::string& carId) {
    Aspect a;
    a.peinture = s.peintureChoisie(carId);
    a.peinture2 = peintureDef(valeurOuVide(s.paint2, carId));
    if (const Objet* f = objet(valeurOuVide(s.finition, carId)); f && f->type == Type::Finition) a.finition = f->id;
    if (const Objet* k = objet(valeurOuVide(s.skin, carId)); k && k->type == Type::Skin && disponible(*k, carId)) a.skin = k->id;
    a.creation = valeurOuVide(s.creation, carId);
    return a;
}

Aspect aspectCourse(const Sauvegarde& s, const std::string& carId) {
    const Aspect g = aspectGarage(s, carId);
    Aspect a;   // défauts : couleur d'origine, même couleur partout, brillant, aucune skin
    auto possedeId = [&](const char* id) { const Objet* o = objet(id); return o && possede(s, *o, carId); };
    if (g.peinture && possedeId(g.peinture->id)) a.peinture = g.peinture;
    const Objet* sec = objet(ID_SECONDAIRE);
    if (g.peinture2 && sec && possede(s, *sec, carId) && possedeId(g.peinture2->id)) a.peinture2 = g.peinture2;
    if (possedeId(g.finition)) a.finition = g.finition;
    if (*g.skin && possedeId(g.skin)) a.skin = g.skin;
    a.creation = g.creation;   // l'atelier (WFS-497) n'a rien à vendre : ses autocollants sont vérifiés à la pose
    return a;
}

void migrer(Sauvegarde& s) {
    int n = 0;
    for (const auto& [car, id] : s.paint) {
        const Objet* o = couleur(id);
        if (!o || possede(s, *o, car)) continue;   // gratuite ou déjà à lui : rien à noter
        s.possede[car].insert(id); n++;
    }
    s.perso = 1;
    WFS_LOG("boutique : sauvegarde d'avant le catalogue : %d couleur(s) déjà choisie(s) gardée(s) gratuitement", n);
}

bool donGarageProvisoire(Sauvegarde& s, const std::string& carId, const std::string& couleurId) {
    const Objet* o = couleur(couleurId);
    if (!o || o->index >= NB_PEINTURES_GARAGE || !disponible(*o, carId) || possede(s, *o, carId)) return false;
    s.possede[carId].insert(o->id);
    WFS_LOG("boutique : PROVISOIRE (jusqu'à WFS-489) : couleur %s donnée à %s par « Choisir » du garage actuel", o->id, carId.c_str());
    return true;
}

std::vector<std::string> bilanCatalogue() {
    std::vector<std::string> v;
    const Type types[] = {Type::Couleur, Type::Secondaire, Type::Finition, Type::Skin, Type::Dessins};
    for (Type t : types) {
        int n = 0, gratuits = 0;
        std::string liste;
        for (const Objet& o : catalogue()) {
            if (o.type != t) continue;
            n++;
            const ConfigJeu::Boutique::Objet r = regle(o);
            gratuits += r.gratuit != 0;
            char b[96];
            std::snprintf(b, sizeof b, "%s%s %s", liste.empty() ? "" : ", ", o.id, r.gratuit ? "gratuit" : r.cadeau ? "cadeau" : (std::to_string(r.prix) + "★").c_str());
            liste += b;
        }
        char t0[96];
        std::snprintf(t0, sizeof t0, "CATALOGUE %s : %d (%d gratuit(s)) : ", nomType(t), n, gratuits);
        v.push_back(t0 + liste);
    }
    char c[160];
    const double* k = config().boutique.coefficient;
    std::snprintf(c, sizeof c, "CATALOGUE coefficients des classes : gratuites ×%g, 1 ×%g, 2 ×%g, 3 ×%g, 4 ×%g, 5 ×%g", k[0], k[1], k[2], k[3], k[4], k[5]);
    v.push_back(c);
    return v;
}

std::vector<std::string> bilanPossession(const Sauvegarde& s) {
    std::vector<std::string> v;
    for (const auto& [car, ids] : s.possede) {
        std::string l = "POSSÉDÉ " + car + " :";
        for (const std::string& id : ids) l += " " + id;
        v.push_back(l);
    }
    std::string d = "POSSÉDÉ autocollants (toutes les voitures) :";
    for (const Objet& o : catalogue()) if (o.type == Type::Dessins && possede(s, o, "")) d += std::string(" ") + o.id;
    v.push_back(d);
    return v;
}

std::vector<std::string> verifierCatalogue() {
    std::vector<std::string> v;
    for (int i = 0; i < BOUTIQUE_SKINS; i++) {
        std::istringstream in(config().boutique.skinVoitures[i]);
        std::string id;
        while (in >> id)
            if (!voitureConnue(id)) v.push_back(std::string("boutique.skins.") + BOUTIQUE_SKINS_IDS[i] + ".voitures : voiture inconnue « " + id + " » (ignorée)");
            else if (voitureRetiree(id.c_str())) v.push_back(std::string("boutique.skins.") + BOUTIQUE_SKINS_IDS[i] + ".voitures : voiture retirée « " + id + " » (ignorée)");
    }
    for (const Objet& o : catalogue())
        if (o.type != Type::Couleur && couleur(o.id)) v.push_back(std::string("identifiant en double : ") + o.id);
    return v;
}

} // namespace wfs::perso
