// WaForSpeed — garage : outils communs aux panneaux (WFS-534) : voir garage_commun.h.
#include "garage_commun.h"
#include <cstring>
#include "../game/commandes_tactiles.h"   // WFS-99 : dessin2d (biseaux d'un seul tenant, traits lissés)
#include "../render/renderer.h"

namespace wfs::ecrans::garage {

Zones g_z;

int peintureParId(const char* id) { for (int i = 0; i < NB_PEINTURES; i++) if (std::strcmp(PAINTS[i].id, id) == 0) return i; return 0; }

namespace {
const Fiche FICHES[] = {
    {"golf", "Compacte", "La compacte sportive de référence : équilibrée et précise.", 0x00E5FF, "bleu"},
    {"p208", "Citadine", "Citadine vive et légère : idéale pour se faufiler dans le trafic.", 0xFF6A00, "jaune"},
    {"abarth", "Mini sportive", "Petite bombe italienne : minuscule, nerveuse, très agile.", 0xFF6A00, "rouge"},
    {"tcross", "SUV compact", "SUV compact facile à conduire, stable dans les chocs.", 0xB6FF00, "orange"},
    {"compass", "SUV", "SUV robuste : haut, lourd, solide quand ça frotte.", 0xB6FF00, "vert"},
    {"s3", "Berline", "Berline compacte 4 roues motrices : rapide et sûre.", 0x00E5FF, "gris"},
    {"x5", "Grand SUV", "Grand SUV premium : puissant mais lourd dans les virages.", 0x00E5FF, "blanc"},
    {"giulia", "Berline sport", "Berline italienne de 510 ch : vitesse de pointe énorme.", 0xFF1F4B, "rouge"},
    {"m4", "Coupé", "Coupé de course pour la route : brutal en accélération.", 0xFF6A00, "noir"},
    {"p911", "Sportive", "La sportive absolue : 4 roues motrices, freins et virages parfaits.", 0x00E5FF, "gris"},
    {"a45", "Compacte sportive", "Compacte de 381 ch à 4 roues motrices : une fusée de poche.", 0x00E5FF, "gris"},
    {"model3", "Berline électrique", "Électrique : couple instantané, silence et grosse accélération.", 0x00E5FF, "blanc"},
    {"corsa", "Citadine", "Citadine légère et maniable : parfaite pour les petites rues.", 0xB6FF00, "jaune"},
    {"gt86", "Coupé", "Petit coupé propulsion : léger, joueur et facile à faire glisser.", 0xFF1F4B, "rouge"},
    {"elantra", "Berline sport", "Berline coréenne affûtée sur circuit : vive et précise.", 0x00E5FF, "bleu"},
    {"v60", "Break sport", "Break suédois de 350 ch : rapide, sûr et stable.", 0x00E5FF, "bleu"},
    {"mustang", "Sportive", "V8 de 466 ch en propulsion : vive et précise, à doser en sortie de virage.", 0xFF6A00, "bleu"},   // WFS-359 : Corvette C7
    {"rrsport", "Grand SUV", "SUV de luxe massif : imposant dans les chocs, lourd en virage.", 0xB6FF00, "noir"},
    {"gallardo", "Supercar cabriolet", "V10 à ciel ouvert : 4 roues motrices et plus de 320 km/h.", 0xB6FF00, "jaune"},
    {"sf90", "Hypercar hybride", "Hybride de 1030 ch : la plus rapide du garage, à manier avec soin.", 0xFF1F4B, "rouge"},
};
} // namespace
const Fiche& ficheDe(const char* id) { for (const auto& f : FICHES) if (std::strcmp(f.id, id) == 0) return f; return FICHES[0]; }

int indexDe(const CarDef* c) { for (int i = 0; i < CAR_COUNT; i++) if (c && &CARS[i] == c && !voitureRetiree(c->id)) return i; return voituresPermises().idx[0]; }
int placeDe(int i) { const VoituresPermises& v = voituresPermises(); for (int p = 0; p < v.n; p++) if (v.idx[p] == i) return p; return 0; }
std::string deuxChiffres(int n) { return (n < 10 ? "0" : "") + std::to_string(n); }

// parallélogramme (clip-path / skewX du web) d'un seul tenant, haut décalé à droite de `biais` (WFS-99, ANO-20 / ANO-33 :
// plus de bandes qui se chevauchent, donc ni rayures ni escalier)
void biseau(Renderer& r, float x, float y, float w, float h, float biais, const vec4& c) {
    dessin2d::biseau(r, x, y, w, h, biais, dessin2d::uni(c));
}
// dégradé linéaire d'un seul uiQuad sur le rectangle (x0, y0)-(x1, y1) : c0 → c1 de gauche à droite (horizontal)
// ou de haut en bas (WFS-99 : voiles du garage sans bandes)
void degrade(Renderer& r, float x0, float y0, float x1, float y1, const vec4& c0, const vec4& c1, bool horizontal) {
    const vec4& hg = c0; const vec4& hd = horizontal ? c1 : c0; const vec4& bd = c1; const vec4& bg = horizontal ? c0 : c1;
    const UiVertex q[4] = {{x0, y0, -1, -1, hg.x, hg.y, hg.z, hg.w}, {x1, y0, -1, -1, hd.x, hd.y, hd.z, hd.w},
                           {x1, y1, -1, -1, bd.x, bd.y, bd.z, bd.w}, {x0, y1, -1, -1, bg.x, bg.y, bg.z, bg.w}};
    r.uiQuad(q);
}

} // namespace wfs::ecrans::garage
