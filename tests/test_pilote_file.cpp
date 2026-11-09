// WFS-339 : pilote automatique des bancs en conduite d'avant (g_conduiteMonde faux, modèle du jeu) : un changement de file du
// trafic à grande vitesse en ligne droite (commande « (file − écart) × 5 − cap × 4 » de traffic_system.cpp) ne mène plus la
// voiture au bord de la chaussée. Avant WFS-339 : écart −1,05 demi-largeur à 185 km/h, arrêt de 4 s contre une balise du bord
// (banc campagne, 610 m après le départ). Ligne droite de la Campagne (450 → 850 m), file du milieu vers la file de gauche.
// usage : test_pilote_file
#include <cmath>
#include <cstdio>
#include "../src/game/aide_direction_banc.h"
#include "../src/game/conduite_banc.h"
using namespace wfs;

// écart le plus grand (demi-largeurs) et temps pour rejoindre la file (s) ; avecLimite = false : pilote d'avant WFS-339
static void changerDeFile(Track& t, double kmh, bool avecLimite, double& pire, double& rejoint) {
    Player p; p.reset(carById("golf")); p.aide = Aide::Activee;
    p.dist = 450 / M; p.position = t.wrap(p.dist); p.speed = kmh / 3.6 / M;
    const double file = -2.0 / 3;
    pire = 0; rejoint = -1;
    for (int n = 0; n < 120 * 6; n++) {
        DriveInput in; in.gas = true;
        in.steer = (float)std::clamp((file - p.playerX) * 5 - p.heading * 4, -1.0, 1.0);   // traffic_system.cpp, autopilot()
        if (avecLimite) limiterPilote(p, t, in);
        p.update(1.0 / 120, in, t, true);
        pire = std::max(pire, std::abs(p.playerX));
        if (rejoint < 0 && std::abs(p.playerX - file) < 0.05) rejoint = n / 120.0;
    }
}

int main() {
    const bool mondeDefaut = g_conduiteMonde;
    g_conduiteMonde = false;   // conduite d'avant : modèle du jeu (WFS-258, RG-00)
    Track& t = banc_conduite::piste("campagne");
    int ecarts = 0;
    for (int k = 450; k < 850; k++) if (std::abs(t.findSegment(t.wrap(k / M)).curve) > 0.01) { std::printf("Campagne : pas de ligne droite de 450 à 850 m (%d m : courbe %.2f)\n", k, t.findSegment(t.wrap(k / M)).curve); return 1; }
    for (double kmh : {90.0, 130.0, 185.0}) {
        double pireAvant, rjAvant, pire, rj;
        changerDeFile(t, kmh, false, pireAvant, rjAvant);
        changerDeFile(t, kmh, true, pire, rj);
        // file de gauche : 0,667 ; bord de la chaussée : 1 ; marge de 0,8 (1,0 m au-delà du centre de la file)
        const bool bon = pire < 0.8 && rj >= 0 && rj < 4;
        if (!bon) ecarts++;
        std::printf("%3.0f km/h : écart le plus grand %.3f (pilote d'avant : %.3f), file rejointe en %.2f s : %s\n", kmh, pire, pireAvant, rj, bon ? "OK" : "ÉCHEC");
    }
    g_conduiteMonde = mondeDefaut;
    std::printf("pilote des bancs, changement de file (WFS-339) : %s\n", ecarts ? "ÉCHEC" : "OK");
    return ecarts ? 1 : 0;
}
