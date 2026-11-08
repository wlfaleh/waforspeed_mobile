// WFS-329 (ANO-178) : fond figé des écrans hors course (src/game/fond_fige.h) : écrans concernés, 1 s de scène animée avant
// de figer, retour à zéro hors de ces écrans ou quand un réglage du rendu change (qualité, ombres, échelle, MSAA, nuit)
#include <cstdio>
#include "../src/game/fond_fige.h"
using namespace wfs;
using namespace wfs::fond_fige;

static int echecs = 0;
static void cas(const char* quoi, bool ok) { std::printf("fond figé : %-86s → %s\n", quoi, ok ? "OK" : "ÉCHEC"); if (!ok) echecs++; }

int main() {
    cas("menu et choix du terrain : figés", ecranFige(Ecran::Menu, false, false, false) && ecranFige(Ecran::Terrain, false, false, false));
    cas("course, pause, fin, garage, salon, chargement : jamais figés",
        !ecranFige(Ecran::Course, false, false, false) && !ecranFige(Ecran::Pause, false, false, false) && !ecranFige(Ecran::Fin, false, false, false) &&
        !ecranFige(Ecran::Garage, false, false, false) && !ecranFige(Ecran::Salon, false, false, false) && !ecranFige(Ecran::Chargement, false, false, false));
    cas("scène dédiée, vue orbite des bancs, banc « avant » : pas figé",
        !ecranFige(Ecran::Menu, true, false, false) && !ecranFige(Ecran::Menu, false, true, false) && !ecranFige(Ecran::Menu, false, false, true));
    {
        Suivi s; Signature g; bool avant = false; int premiere = -1;
        for (int i = 0; i < 40; i++) { const bool f = s.avancer(true, g); if (i < IMAGES_AVANT) avant = avant || f; if (f && premiere < 0) premiere = i; }
        cas("1 s de scène animée (30 images), figée à partir de la 31e", !avant && premiere == IMAGES_AVANT);
        cas("hors du menu : animé et compteur remis à zéro", !s.avancer(false, g) && s.images == 0 && !s.avancer(true, g));
    }
    {
        Suivi s; Signature g;
        for (int i = 0; i < 40; i++) s.avancer(true, g);
        Signature h = g; h.basse = true;
        const bool apresChange = s.avancer(true, h);
        int n = 1; while (!s.avancer(true, h) && n < 100) n++;
        cas("qualité changée sur le menu : scène redessinée 30 images, puis figée de nouveau", !apresChange && n == IMAGES_AVANT);
        Signature e = h; e.echelle = 0.6f; Signature m = h; m.msaa = 2; Signature o = h; o.ombres = 3; Signature nu = h; nu.nuit = true;
        cas("échelle, MSAA, ombres ou nuit changés : redessinée", !s.avancer(true, e) && !s.avancer(true, m) && !s.avancer(true, o) && !s.avancer(true, nu));
    }
    std::printf("fond figé : %s (%d échec(s))\n", echecs ? "ÉCHEC" : "OK", echecs);
    return echecs ? 1 : 0;
}
