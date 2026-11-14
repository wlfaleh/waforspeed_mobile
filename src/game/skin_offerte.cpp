// WaForSpeed — skins : skin offerte, skins données, bons de skin (WFS-494) : voir skin_offerte.h.
#include "skin_offerte.h"
#include "../core/log.h"
#include "boutique.h"

namespace wfs::skins {
namespace {
// skin du catalogue, existante pour cette voiture ; nullptr sinon
const perso::Objet* skinDe(const std::string& carId, const std::string& skinId) {
    const perso::Objet* o = perso::objet(skinId);
    return o && o->type == perso::Type::Skin && perso::disponible(*o, carId) ? o : nullptr;
}
// une skin en vente, pas encore possédée par cette voiture (offerte et bon)
const perso::Objet* aChoisir(const Sauvegarde& s, const std::string& carId, const std::string& skinId) {
    const perso::Objet* o = skinDe(carId, skinId);
    return o && !perso::cadeau(*o) && !perso::possede(s, *o, carId) ? o : nullptr;
}
} // namespace

bool aDesSkins(const std::string& carId) {
    for (const perso::Objet& o : perso::catalogue()) if (o.type == perso::Type::Skin && perso::disponible(o, carId)) return true;
    return false;
}

std::vector<const perso::Objet*> enVente(const std::string& carId) {
    std::vector<const perso::Objet*> v;
    for (const perso::Objet& o : perso::catalogue())
        if (o.type == perso::Type::Skin && perso::disponible(o, carId) && !perso::cadeau(o)) v.push_back(&o);
    return v;
}

bool offerteDue(const Sauvegarde& s, const std::string& carId) {
    if (!s.actif() || !boutique::possedeVoiture(s, carId) || s.skinsOffertes.count(carId)) return false;
    for (const perso::Objet* o : enVente(carId)) if (!perso::possede(s, *o, carId)) return true;
    return false;
}

bool offrir(Sauvegarde& s, const std::string& carId, const std::string& skinId) {
    const perso::Objet* o = aChoisir(s, carId, skinId);
    if (!o || !offerteDue(s, carId)) return false;
    s.possede[carId].insert(o->id);
    s.skinsOffertes.insert(carId);
    s.skin[carId] = o->id;
    WFS_LOG("skins : skin offerte %s choisie pour %s (une seule par voiture)", o->id, carId.c_str());
    return true;
}

bool donnerSkin(Sauvegarde& s, const std::string& carId, const std::string& skinId) {
    const perso::Objet* o = skinDe(carId, skinId);
    if (!o || perso::possede(s, *o, carId)) return false;
    s.possede[carId].insert(o->id);
    WFS_LOG("skins : skin %s donnée à %s (cadeau du jour ou succès)", o->id, carId.c_str());
    return true;
}

bool donnerSkin(const std::string& carId, const std::string& skinId) {
    Sauvegarde& s = sauvegarde();
    if (!donnerSkin(s, carId, skinId)) return false;
    if (s.actif()) s.enregistrer();
    return true;
}

bool bonUtilisable(const Sauvegarde& s, const std::string& carId, const std::string& skinId) {
    return s.market.bonsSkin > 0 && boutique::possedeVoiture(s, carId) && aChoisir(s, carId, skinId);
}

bool utiliserBon(Sauvegarde& s, const std::string& carId, const std::string& skinId) {
    if (!bonUtilisable(s, carId, skinId)) return false;
    const perso::Objet* o = aChoisir(s, carId, skinId);
    s.market.bonsSkin--;
    s.possede[carId].insert(o->id);
    s.skin[carId] = o->id;
    WFS_LOG("skins : bon de skin utilisé : %s pour %s (bons restants : %d)", o->id, carId.c_str(), s.market.bonsSkin);
    return true;
}

} // namespace wfs::skins
