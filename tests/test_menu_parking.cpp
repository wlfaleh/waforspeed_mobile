// WFS-552 (RG-04) : caméra rapprochée du menu (src/ui/menu_parking_cadrage.h) : pour chaque voiture du jeu, en 2400 × 1080 et en
// 1280 × 720 (et 4:3), la voiture est entière dans l'image (aucune partie coupée), dans la partie droite (centre à droite du milieu,
// à droite de la console du menu), et grande (gros plan : en 2400 × 1080, au moins 45 % de la hauteur de l'image ; maquette 01 : 49 %).
#include <cstdio>
#include "../src/game/cars.h"
#include "../src/ui/menu_parking_cadrage.h"
using namespace wfs;
using namespace wfs::ecrans::parking;

static int echecs = 0;
static void cas(const std::string& quoi, bool ok) { std::printf("menu en 3D : %-96s → %s\n", quoi.c_str(), ok ? "OK" : "ÉCHEC"); if (!ok) echecs++; }

int main() {
    // demi-hauteur minimale de la voiture (NDC) : maquette 01 en 2400 × 1080 (0,49 visée) ; écrans moins larges : la largeur limite
    const struct { const char* nom; float aspect, hauteur; } ECRANS[] = {{"2400x1080", 2400.f / 1080.f, 0.45f}, {"1280x720", 1280.f / 720.f, 0.33f}, {"1024x768", 1024.f / 768.f, 0.24f}};
    for (const auto& e : ECRANS) {
        bool entiere = true, droite = true, grande = true; char pire[160] = "";
        for (int i = 0; i < CAR_COUNT; i++) {
            const CarDef& c = CARS[i];
            const Emprise m = emprise(cadrage(c.d.L, c.d.W, c.d.H, e.aspect), c.d.L, c.d.W, c.d.H, e.aspect);
            const bool ok1 = m.xmin > -0.40f && m.xmax < 0.95f && m.ymin > -0.95f && m.ymax < 0.95f;   // jamais sous la console (tiers gauche)
            const bool ok2 = (m.xmin + m.xmax) / 2 > 0.05f;
            const bool ok3 = (m.ymax - m.ymin) / 2 > e.hauteur;
            if (!(ok1 && ok2 && ok3) && !*pire) std::snprintf(pire, sizeof pire, " (%s : x %.2f..%.2f, y %.2f..%.2f)", c.id, m.xmin, m.xmax, m.ymin, m.ymax);
            entiere = entiere && ok1; droite = droite && ok2; grande = grande && ok3;
        }
        cas(std::string(e.nom) + " : toutes les voitures entières dans l'image" + (entiere ? "" : pire), entiere);
        cas(std::string(e.nom) + " : voiture dans la partie droite de l'image (à droite de la console)", droite);
        cas(std::string(e.nom) + " : gros plan (≥ " + std::to_string((int)(e.hauteur * 100 + 0.5f)) + " % de la hauteur de l'image)" + (grande ? "" : pire), grande);
    }
    {   // SF-X de la maquette, 2400 × 1080 : trois quarts avant gauche (œil devant la voiture et à sa gauche), caméra basse
        const Cadrage c = cadrage(4.85f, 1.97f, 1.19f, 2400.f / 1080.f);
        cas("SF-X : trois quarts avant gauche, œil entre 0,9 et 1,3 m de haut", c.oeil.z < c.cible.z && c.oeil.x < c.cible.x && c.oeil.y >= 0.9f && c.oeil.y <= 1.3f);
    }
    std::printf("menu en 3D : %d échec(s)\n", echecs);
    return echecs ? 1 : 0;
}
