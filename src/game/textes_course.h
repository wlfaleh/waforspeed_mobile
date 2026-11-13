// WaForSpeed — textes de la course en français ou en anglais (WFS-506, épic E20) : HUD, bandeaux, consignes de la carte de
// navigation, fin de course des 4 modes. Tous passent par ui::tr (RC-E20-01) ; ce fichier ne garde que les petites aides
// communes aux systèmes de course :
//   - remplir() : texte d'une clé avec ses valeurs {1} {2}, écrit dans un tampon réutilisé (RG-04 : aucune allocation
//     nouvelle par image une fois la capacité atteinte ; ui::tr avec valeurs crée une chaîne et un tableau à chaque appel) ;
//   - ordinal() : « 1re », « 2e » / « 1st », « 2nd », « 3rd » (places, sorties de rond-point) ;
//   - NomPropre : un nom inventé (rue, pilote, adversaire) noté à part au journal du banc « textes » (RG-02, RC-E20-03 :
//     il ne se traduit pas, ce n'est pas une phrase française).
#pragma once
#include <cstdio>
#include <string>
#include <string_view>
#include "../ui/langue.h"
#include "../ui/text.h"
#include "../ui/ecrans/textes_banc.h"   // WFS-505 : NomPropre

namespace wfs::textes {

// texte de `cle` dans la langue active, {1} et {2} remplacés par v1 et v2, dans `tampon` (vidé, capacité gardée)
inline const std::string& remplir(std::string& tampon, const char* cle, std::string_view v1 = {}, std::string_view v2 = {}) {
    const std::string& t = ui::tr(cle);
    tampon.clear();
    for (size_t i = 0; i < t.size(); i++) {
        if (t[i] == '{' && i + 2 < t.size() && (t[i + 1] == '1' || t[i + 1] == '2') && t[i + 2] == '}') {
            tampon += t[i + 1] == '1' ? v1 : v2;
            i += 2;
        } else tampon += t[i];
    }
    return tampon;
}

// nombre ordinal court : clés ordinal.1 à ordinal.6 (« 1re » / « 1st »…), puis ordinal.n (« {1}e » / « {1}th »)
inline const std::string& ordinal(std::string& tampon, int n) {
    static const char* const CLES[] = {"ordinal.1", "ordinal.2", "ordinal.3", "ordinal.4", "ordinal.5", "ordinal.6"};
    if (n >= 1 && n <= 6) { tampon = ui::tr(CLES[n - 1]); return tampon; }
    char b[16]; std::snprintf(b, sizeof b, "%d", n);
    return remplir(tampon, "ordinal.n", b);
}

// titre de la fin de course (Course à 5, Duel) : « VICTOIRE » / « VICTORY », « 2e PLACE » / « 2nd PLACE » (WFS-146 : jamais « 2E »)
inline std::string titrePlace(int rang) {
    if (rang <= 1) return ui::tr("fin.victoire");
    std::string o, t;
    return remplir(t, "fin.place", ordinal(o, rang));
}

// texte d'une clé en capitales, gardé tant que la langue ne change pas (RG-04 : une seule conversion, pas une par image) ;
// une variable statique par endroit du code : static textes::Capitales gaz{"hud.gaz"}; … gaz()
struct Capitales {
    const char* cle;
    int langue = -1;
    std::string texte;
    const std::string& operator()() {
        const int l = (int)ui::langue::active();
        if (l != langue) { texte = ui::majuscules(ui::tr(cle)); langue = l; }
        return texte;
    }
};

// nom propre inventé (rue, pseudo, adversaire) : noté à part au journal du banc « textes », jamais vérifié comme une phrase
// (struct de WFS-505, banc des menus, réutilisée)
using ecrans::textes::NomPropre;

} // namespace wfs::textes

namespace wfs {
class Game; class Renderer;
// banc WFS_BANC=textes (après les Réglages de WFS-504) et WFS_BANC=textes-course (seul) : une course de chaque mode dans la
// langue de WFS_LANGUE, captures textes-<fr|en>-course-* et -fin-*, parcours fin → Rejouer → fin → Menu ; rend le nombre d'échecs
int benchCourseTextes(Game& g, Renderer& r, const std::string& out);
} // namespace wfs
