// WFS-234 (RG-05) : calculs de la carte de navigation (src/ui/navigation.h), sur le vrai tracé 2 des 4 cartes.
//  - prochaine manœuvre : la plus proche devant le joueur, distance = début de la section − joueur, 0 tant qu'on y est,
//    tour suivant pris en compte, arrivée quand elle est plus proche ; rond-point du tracé 2 → 3e sortie ;
//  - distance arrondie comme un GPS (« Maintenant », 10 m, 50 m, 0,1 km) ;
//  - zoom croissant avec la vitesse, borné ; lissage sans saut ;
//  - noms de rues inventés : stables, non vides, aucun nom de ville réelle ;
//  - signalements fixes sur des lignes droites ; bouchons : 3 voitures lentes rapprochées.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include "../src/game/track.h"
#include "../src/ui/navigation.h"
using namespace wfs;
using namespace wfs::nav;

static int echecs = 0, cas = 0;
#define VERIFIE(c, ...) do { cas++; if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int main() {
    // ---- distances arrondies
    VERIFIE(texteDistance(0) == "Maintenant" && texteDistance(14.9) == "Maintenant", "maintenant");
    VERIFIE(texteDistance(15) == "20 m" && texteDistance(44) == "40 m" && texteDistance(96) == "100 m", "10 m : %s %s %s",
            texteDistance(15).c_str(), texteDistance(44).c_str(), texteDistance(96).c_str());
    VERIFIE(texteDistance(249) == "250 m" && texteDistance(260) == "250 m" && texteDistance(940) == "950 m", "50 m : %s", texteDistance(260).c_str());
    VERIFIE(texteDistance(980) == "1 km" && texteDistance(1240) == "1,2 km" && texteDistance(12345) == "12,3 km", "km : %s %s",
            texteDistance(1240).c_str(), texteDistance(12345).c_str());
    VERIFIE(texteDistance(NAN) == "Maintenant", "NaN");

    // ---- zoom
    VERIFIE(avantM(0) == AVANT_MIN_M && avantM(-5) == AVANT_MIN_M && avantM(1000) == AVANT_MAX_M, "zoom borné");
    double prec = 0; bool croissant = true;
    for (int v = 0; v <= 300; v += 10) { const double a = avantM(v); if (a < prec) croissant = false; prec = a; }
    VERIFIE(croissant, "zoom croissant avec la vitesse");
    VERIFIE(std::fabs(avantM(150) - 305) < 1e-9, "150 km/h : %.0f m", avantM(150));
    {
        double z = 200; double maxSaut = 0;
        for (int i = 0; i < 120; i++) { const double n = lisser(z, 400, 1.0 / 60, TAU_ZOOM); maxSaut = std::max(maxSaut, n - z); z = n; }
        VERIFIE(maxSaut < 4 && z > 300 && z < 400, "lissage : saut max %.2f m par image, %.0f m après 2 s", maxSaut, z);
        VERIFIE(lisser(NAN, 250, 0.1, 1) == 250 && lisser(100, 250, 0, 1) == 250, "lissage : départ");
    }

    // ---- prochaine manœuvre sur le vrai tracé 2 des 4 cartes
    for (const char* id : {"ville", "plage", "campagne", "circuit"}) {
        Track t; t.build(mapById(id), 2);
        const auto& S = trace::sections(t);
        const double L = t.length();
        int nManoeuvres = 0, nRp = 0;
        for (const auto& s : S) { if (manoeuvreDe(s) != Manoeuvre::Aucune) nManoeuvres++; if (s.type == trace::Type::RondPoint) nRp++; }
        VERIFIE(nManoeuvres >= 3, "%s : %d manœuvres seulement", id, nManoeuvres);
        int erreurs = 0, ecartsDist = 0;
        double maxSaut = 0;
        for (double s = 0; s < 2.2 * L; s += 3 / M) {   // tous les 3 m sur plus de deux tours
            const Prochaine p = prochaine(S, s, L, 0, M);
            if (p.m == Manoeuvre::Aucune) { erreurs++; continue; }
            // contrôle par force brute : la section de manœuvre dont le début (ce tour ou le suivant) est le plus proche devant,
            // ou celle qui contient s
            const double sT = s - std::floor(s / L) * L;
            double best = 1e18;
            for (const auto& q : S) {
                if (manoeuvreDe(q) == Manoeuvre::Aucune) continue;
                for (int k = 0; k < 2; k++) {
                    const double a = q.debutU() + k * L, b = q.finU() + k * L;
                    if (b <= sT) continue;
                    best = std::min(best, std::max(0.0, (a - sT) * M));
                    break;
                }
            }
            if (std::fabs(best - p.distanceM) > 1e-6) ecartsDist++;
            if (p.distanceM > 0 && std::fabs((p.sAbs - s) * M - p.distanceM) > 1e-6) erreurs++;
            if (p.m == Manoeuvre::RondPoint && p.sortie != 3) erreurs++;
            const Prochaine p2 = prochaine(S, s + 3 / M, L, 0, M);   // en avançant de 3 m, la distance baisse de 3 m (ou change de manœuvre)
            if (p2.section == p.section && p.distanceM > 3) maxSaut = std::max(maxSaut, std::fabs(p.distanceM - p2.distanceM - 3));
        }
        VERIFIE(erreurs == 0 && ecartsDist == 0, "%s : %d erreur(s), %d écart(s) de distance", id, erreurs, ecartsDist);
        VERIFIE(maxSaut < 1e-6, "%s : distance non continue (%.3f m)", id, maxSaut);
        std::printf("navigation : %s : %zu sections, %d manœuvres (%d rond(s)-point(s)), tour %.2f km : OK\n", id, S.size(), nManoeuvres, nRp, L * M / 1000);
        // dans un rond-point : distance 0, consigne de sortie
        for (const auto& q : S) if (q.type == trace::Type::RondPoint) {
            const Prochaine p = prochaine(S, (q.debutU() + q.finU()) / 2 + L, L, 0, M);
            VERIFIE(p.m == Manoeuvre::RondPoint && p.distanceM == 0 && texteConsigne(p) == "Au rond-point, 3e sortie", "%s : dans le rond-point : %s",
                    id, texteConsigne(p).c_str());
            const Prochaine av = prochaine(S, q.debutU() - 150 / M, L, 0, M);   // Ville : virage serré 250 m avant le rond-point
            VERIFIE(av.m == Manoeuvre::RondPoint && std::fabs(av.distanceM - 150) < 1e-6 && texteDistance(av.distanceM) == "150 m",
                    "%s : 150 m avant le rond-point : %s à %.1f m", id, texteConsigne(av).c_str(), av.distanceM);
        }
        // WFS-283 (RG-03) : plus aucune consigne de rond-point en jeu ; le carrefour simple s'annonce comme un virage serré à gauche
        for (const auto& q : S) if (q.exRondPoint) {
            const Prochaine av = prochaine(S, q.debutU() - 150 / M, L, 0, M);
            VERIFIE(nRp == 0 && manoeuvreDe(q) == Manoeuvre::SerreGauche && av.m == Manoeuvre::SerreGauche && std::fabs(av.distanceM - 150) < 1e-6 &&
                    texteConsigne(av).find("rond-point") == std::string::npos, "%s : carrefour simple : %s à %.1f m", id, texteConsigne(av).c_str(), av.distanceM);
            std::printf("navigation : %s : carrefour simple annoncé « %s » à %s\n", id, texteConsigne(av).c_str(), texteDistance(av.distanceM).c_str());
        }
        // arrivée plus proche que la manœuvre suivante
        const Prochaine p0 = prochaine(S, 0, L, 0, M);
        const double arr = (p0.distanceM - 50) / M;
        if (arr > 0) {
            const Prochaine pa = prochaine(S, 0, L, arr, M);
            VERIFIE(pa.m == Manoeuvre::Arrivee && std::fabs(pa.distanceM - (p0.distanceM - 50)) < 1e-6, "%s : arrivée", id);
        }
        // noms : stables et non vides, aucune ville réelle (liste de contrôle des grandes villes françaises)
        const char* villes[] = {"Paris", "Marseille", "Lyon", "Toulouse", "Nice", "Nantes", "Bordeaux", "Lille", "Monaco", "Cannes"};
        for (int i = 0; i < (int)S.size(); i++) {
            const std::string n = nomRue(id, t.map()->seed, i, S[i].type, S[i].autoroute);
            VERIFIE(!n.empty() && n == nomRue(id, t.map()->seed, i, S[i].type, S[i].autoroute), "%s : nom %d instable", id, i);
            for (const char* v : villes) VERIFIE(n.find(v) == std::string::npos, "%s : nom réel « %s »", id, n.c_str());
        }
        // signalements fixes : sur des lignes droites du tour
        for (const SignalFixe& f : signauxFixes(S, M)) {
            const trace::Section& q = S[f.id];
            VERIFIE(f.sTour >= q.debutU() && f.sTour < q.finU() && q.rayon <= 0, "%s : signalement hors d'une ligne droite", id);
        }
    }
    // ---- sortie de rond-point selon le virage net, et sens des flèches
    { trace::Section s; s.type = trace::Type::RondPoint; s.angle = -90; VERIFIE(sortieRondPoint(s) == 3, "−90° : 3e sortie");
      s.angle = 0; VERIFIE(sortieRondPoint(s) == 2, "0° : 2e sortie"); s.angle = 90; VERIFIE(sortieRondPoint(s) == 1, "90° : 1re sortie"); }
    { trace::Section s; s.type = trace::Type::Virage; s.angle = 88; s.rayon = 22; s.cote = 1;
      VERIFIE(manoeuvreDe(s) == Manoeuvre::SerreDroite && sensDe(manoeuvreDe(s)) == 1, "virage serré à droite");
      s.angle = -25; s.rayon = 700; s.cote = -1; VERIFIE(manoeuvreDe(s) == Manoeuvre::Aucune, "courbe d'autoroute : pas de manœuvre");
      s.angle = -50; s.rayon = 150; VERIFIE(manoeuvreDe(s) == Manoeuvre::LegerGauche, "serrer à gauche");
      s.angle = -90; s.rayon = 60; VERIFIE(manoeuvreDe(s) == Manoeuvre::Gauche && sensDe(Manoeuvre::Gauche) == -1, "tourner à gauche"); }
    // ---- bouchons
    {
        const double sM[] = {0, 15, 30, 50, 200, 210, 300, 310, 320, 330};
        const double kmh[] = {10, 12, 8, 90, 5, 5, 20, 10, 15, 24};
        int q[8];
        const int n = bouchons(sM, kmh, 10, q, 8);
        VERIFIE(n == 2 && q[0] == 0 && q[1] == 6, "bouchons : %d (%d, %d)", n, n > 0 ? q[0] : -1, n > 1 ? q[1] : -1);
        const double ecartes[] = {0, 40, 80}; const double lents[] = {5, 5, 5};
        VERIFIE(bouchons(ecartes, lents, 3, q, 8) == 0, "voitures lentes trop écartées : pas de bouchon");
        VERIFIE(bouchons(sM, kmh, 0, q, 8) == 0, "aucune voiture");
    }
    std::printf("navigation : %d vérifications, %s\n", cas, echecs ? "ÉCHEC" : "TOUT EST VERT");
    return echecs ? 1 : 0;
}
