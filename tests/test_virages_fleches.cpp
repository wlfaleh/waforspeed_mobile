// WFS-295 (ANO-168) : la voiture du joueur doit tourner vraiment dans les virages et les épingles (ancienne conduite, en jeu
// depuis WFS-258 RG-00 ; flèches, aide désactivée). Mesures sans GPU :
//  - RG-01 : rayon réel (dans le monde) volant à fond à 10 km/h, identique au centre, au bord et sur le bas-côté d'un virage de
//    22 m (± 5 %) et égal à celui de la ligne droite ; rayon comparé à la fiche (diamètre entre trottoirs ≈ 4,15 × empattement) ;
//    WFS-379 : la Corniche n'a plus de lacets ; le virage mesuré est son virage de rue de 22 m (carrefour simple, même sol)
//  - RG-02 : les épingles des lacets (R = 22 m ; WFS-379 : les 4 du col de la Campagne) passées à 20, 25 et 30 km/h aux flèches (−1, 0, +1), sur la
//    file extérieure, aide désactivée puis activée, et en « Glisser » (volant proportionnel) : aucune image sur l'herbe ;
//  - RG-03 : après un accident, la voiture répond à la flèche en 0,5 s au plus, braquage gardé.
// usage : test_virages_fleches [--rapport]
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <vector>
#include "../src/game/cars.h"
#include "../src/game/player.h"
#include "../src/game/trace_cartes.h"
#include "../src/game/track.h"
#include "../src/game/virages_banc.h"
using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)
using namespace wfs::banc_virages;

// une épingle, joueur sur la file extérieure (file −0,6 côté extérieur), vitesse tenue : flèches (−1, 0, +1) ou volant (« Glisser »)
struct Passage { int herbe = 0; double ecartMax = 0; bool fini = false; };
static Passage passer(Track& t, const Epingle& e, double kmh, Aide aide, bool fleches, double depart = 0.6, double avant = 60) {
    const double file = -e.sens * 0.6;   // extérieur du virage (n > 0 à droite ; virage à droite : extérieur à gauche)
    Player p = voiture("golf", aide, t, e.debut - avant / M, -e.sens * depart * RW, kmh);
    Passage r;
    for (int k = 0; k < 120 * 40 && p.dist < e.fin + 40 / M; k++) {
        p.update(DT, tenir(p, kmh, piloter(p, t, file, fleches)), t, true);   // regard du joueur : un point de sa file 9 m devant
        if (std::abs(p.playerX) > 1) r.herbe++;
        r.ecartMax = std::max(r.ecartMax, std::abs(p.playerX));
    }
    r.fini = p.dist >= e.fin + 40 / M;
    return r;
}

int main(int argc, char** argv) {
    const bool rapport = argc > 1 && !std::strcmp(argv[1], "--rapport");
    static Track ville, plage, col;
    ville.build(mapById("ville"), trace::versionCarte("ville"));
    plage.build(mapById("plage"), trace::versionCarte("plage"));
    col.build(mapById("campagne"), trace::versionCarte("campagne"));   // WFS-379 : épingles des lacets du col de la Campagne

    // ---------- RG-01 : rayon réel, volant à fond, 10 km/h ----------
    std::printf("## RG-01 : rayon réel volant à fond à 10 km/h (aide désactivée)\n");
    for (const char* id : {"golf", "m4"}) {
        const double droit = rayon(id, ville, 50 / M, 0, 10, 1.f), fiche = rayonFiche(id);
        std::printf("- %s : ligne droite %.2f m ; fiche (≈ 4,15 × empattement, au centre de la voiture) %.2f m ; écart %+.0f %%\n", id, droit, fiche,
                    100 * (droit / fiche - 1));
    }
    const std::vector<Epingle> ep = epingles(col);
    VERIFIE(ep.size() >= 4, "épingles des lacets introuvables (%zu)", ep.size());
    const trace::Section* rue = nullptr;   // WFS-379 : virage de rue de 22 m de la Corniche (carrefour simple)
    for (const trace::Section& x : plage.sections()) if (x.exRondPoint) { rue = &x; break; }
    VERIFIE(rue, "virage de rue de la Corniche introuvable");
    if (rue) {
        // milieu de la partie la plus serrée (courbure établie), voiture placée au centre, au bord et sur le bas-côté des deux côtés
        int64_t a0 = rue->debut, a1 = rue->debut; double cMax = 0;
        for (int64_t a = rue->debut; a < rue->fin; a++) cMax = std::max(cMax, std::abs(plage.segAt(a).curve));
        for (int64_t a = rue->debut; a < rue->fin; a++) if (std::abs(plage.segAt(a).curve) > 0.99 * cMax) { if (a0 == rue->debut) a0 = a; a1 = a; }
        const Epingle e{a0 * SEG, (a1 + 1) * SEG, courbure(plage, (a0 + a1 + 1) * 0.5 * SEG) > 0 ? 1 : -1};
        const double s = (e.debut + e.fin) / 2, R = 1 / std::abs(courbure(plage, s));
        const double droit = rayon("golf", ville, 50 / M, 0, 10, 1.f);
        std::printf("- virage de rue de la Corniche à %.0f m (R = %.1f m, virage %s), Golf, volant tourné vers l'intérieur :\n", s * M, R, e.sens > 0 ? "à droite" : "à gauche");
        for (double n : {0.0, 7.5, -7.5, 11.5, -11.5}) {
            const double r = rayon("golf", plage, s, n, 10, (float)e.sens), ecart = r / droit - 1;
            const char* ou = n == 0 ? "centre" : std::abs(n) < 8 ? (n * e.sens < 0 ? "bord extérieur" : "bord intérieur") : (n * e.sens < 0 ? "bas-côté extérieur" : "bas-côté intérieur");
            std::printf("  - %s (n = %+.1f m) : rayon %.2f m (%+.1f %% de la ligne droite)\n", ou, n, r, 100 * ecart);
            VERIFIE(std::abs(ecart) <= 0.05, "rayon faux au %s : %.2f m au lieu de %.2f m (%+.1f %%)", ou, r, droit, 100 * ecart);
        }
    }

    // ---------- RG-02 : épingles des lacets de la Campagne (WFS-379), file extérieure ----------
    std::printf("## RG-02 : les %zu épingles des lacets de la Campagne, Golf, file extérieure, vitesse tenue\n", ep.size());
    struct Cas { const char* nom; Aide aide; bool fleches; };
    for (const Cas c : {Cas{"flèches, aide désactivée", Aide::Desactivee, true}, Cas{"flèches, aide activée", Aide::Activee, true},
                        Cas{"Glisser, aide désactivée", Aide::Desactivee, false}, Cas{"Glisser, aide activée", Aide::Activee, false}}) {
        for (double kmh : {20.0, 25.0, 30.0}) {
            int herbe = 0, finies = 0; double ecart = 0;
            for (const Epingle& e : ep) { const Passage r = passer(col, e, kmh, c.aide, c.fleches); herbe += r.herbe; finies += r.fini; ecart = std::max(ecart, r.ecartMax); }
            std::printf("- %s, %2.0f km/h : %d / %zu épingle(s) passée(s), %d image(s) sur l'herbe, écart max %.2f demi-largeur\n", c.nom, kmh, finies,
                        ep.size(), herbe, ecart);
            if (!rapport) VERIFIE(herbe == 0 && finies == (int)ep.size(), "%s à %.0f km/h : %d image(s) sur l'herbe", c.nom, kmh, herbe);
        }
    }

    // ---------- RG-02 (vidéo de l'utilisateur) : voiture au bord extérieur ou sur le bas-côté à l'entrée de l'épingle ----------
    std::printf("## RG-02, cas de la vidéo : voiture au bord extérieur (0,95) ou sur le bas-côté (1,35 demi-largeur) 10 m avant l'épingle, flèches, aide désactivée\n");
    for (double depart : {0.95, 1.35}) for (double kmh : {20.0, 30.0, 50.0}) {
        int herbe = 0, finies = 0; double ecart = 0;
        for (const Epingle& e : ep) { const Passage r = passer(col, e, kmh, Aide::Desactivee, true, depart, 10); herbe += r.herbe; finies += r.fini; ecart = std::max(ecart, r.ecartMax); }
        std::printf("- départ %.2f, %2.0f km/h : %d / %zu passée(s), %d image(s) sur l'herbe (%.1f s par épingle), écart max %.2f demi-largeur\n", depart, kmh, finies, ep.size(),
                    herbe, herbe * DT / std::max<size_t>(1, ep.size()), ecart);
    }

    // ---------- RG-03 : après un accident, la flèche répond en 0,5 s au plus, braquage gardé ----------
    std::printf("## RG-03 : accident à 60 km/h (impact 0,45, comme un choc contre le décor), flèche droite tenue avant et après\n");
    {
        Player p = voiture("golf", Aide::Desactivee, ville, 50 / M, 0, 60);
        for (int k = 0; k < 60; k++) p.update(DT, tenir(p, 60, 1.f), ville, true);
        const double steerAvant = p.steer;
        p.accident(0.45, 1);
        double reponse = -1, steerMin = 1e9;
        double cap0 = pose(p, ville).cap;
        for (int k = 0; k < 240; k++) {
            p.update(DT, tenir(p, 60, 1.f), ville, true);
            steerMin = std::min(steerMin, p.steer);
            const double cap = pose(p, ville).cap;
            if (reponse < 0 && p.controlLock <= 0 && std::remainder(cap - cap0, 2 * M_PI) > 0.002) reponse = (k + 1) * DT;
            cap0 = cap;
        }
        std::printf("- braquage avant l'accident %.2f, au plus bas après %.2f ; la voiture tourne de nouveau à la flèche après %.2f s\n", steerAvant, steerMin, reponse);
        VERIFIE(reponse > 0 && reponse <= 0.5, "réponse à la flèche après l'accident en %.2f s (0,5 s au plus)", reponse);
        VERIFIE(steerMin >= steerAvant - 0.01, "braquage remis à zéro par l'accident (%.2f → %.2f)", steerAvant, steerMin);
    }
    // ---------- interrupteur du banc (captures « avant ») : il rejoue le défaut de main, et il est coupé dans le jeu ----------
    if (!ep.empty()) {
        VERIFIE(!g_avantWfs295, "g_avantWfs295 actif hors du banc");
        const Epingle& e = ep[0];
        const double s = (e.debut + e.fin) / 2, droit = rayon("golf", ville, 50 / M, 0, 10, 1.f);
        g_avantWfs295 = true;
        const double ext = rayon("golf", col, s, -e.sens * 7.5, 10, (float)e.sens);
        g_avantWfs295 = false;
        std::printf("## banc : comportement d'avant rejoué : rayon au bord extérieur %.2f m (%+.0f %% de la ligne droite, %+.0f %% mesuré sur main)\n", ext,
                    100 * (ext / droit - 1), 28.5);
        VERIFIE(ext / droit - 1 > 0.2, "le comportement d'avant n'est pas rejoué par g_avantWfs295 (%.2f m)", ext);
    }
    std::printf("direction dans les virages (WFS-295) : %s\n", echecs ? "ÉCHEC" : "OK");
    return echecs ? 1 : 0;
}
