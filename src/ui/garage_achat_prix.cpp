// WaForSpeed — garage : prix des pastilles et ce qui manque pour partir en course (WFS-489, RG-03) : voir garage_achat.h.
// Calcul seul (catalogue et possession de WFS-487), sans rendu : testé par tests/test_garage_achat.cpp.
#include "garage_achat.h"

namespace wfs::ecrans::garage {

long long prixPastille(const Sauvegarde& s, const char* carId, int peinture, bool secondaire) {
    if (peinture < 0 || peinture >= NB_PEINTURES) return 0;
    long long total = 0;
    if (const perso::Objet* c = perso::couleur(PAINTS[peinture].id); c && !perso::possede(s, *c, carId)) total += perso::prix(*c, carId);
    if (secondaire)   // « Toit et rétros » : le droit de poser une couleur secondaire sur cette voiture (WFS-487, 50 ★ × coefficient)
        if (const perso::Objet* d = perso::objet(perso::ID_SECONDAIRE); d && !perso::possede(s, *d, carId)) total += perso::prix(*d, carId);
    return total;
}

Manque manque(const Sauvegarde& s, const EtatGarage& e) {
    Manque m;
    const char* car = CARS[e.index].id;
    auto ajouter = [&](const perso::Objet* o) {
        if (!o || perso::possede(s, *o, car) || m.n >= 3) return;
        for (int i = 0; i < m.n; i++) if (m.objets[i] == o) return;   // même couleur en principale et en secondaire : une fois
        m.objets[m.n++] = o; m.total += perso::prix(*o, car);
    };
    if (e.peinture >= 0 && e.peinture < NB_PEINTURES) ajouter(perso::couleur(PAINTS[e.peinture].id));
    if (e.peinture2 >= 0 && e.peinture2 < NB_PEINTURES) { ajouter(perso::objet(perso::ID_SECONDAIRE)); ajouter(perso::couleur(PAINTS[e.peinture2].id)); }
    return m;
}

} // namespace wfs::ecrans::garage
