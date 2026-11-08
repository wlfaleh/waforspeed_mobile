// WFS-257 (ANO-130) : apparition naturelle des personnes, règles pures de src/world/foule_apparition.h, sans GPU.
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
#include "../src/world/foule_apparition.h"
using namespace wfs::apparition;

static int echecs = 0;
static void cas(const std::string& quoi, bool ok) { std::printf("apparition : %-92s → %s\n", quoi.c_str(), ok ? "OK" : "ÉCHEC"); if (!ok) echecs++; }

int main() {
    const double dt = 1.0 / 20;   // 20 images/s (RG-06)
    const float seuils[3] = {16.f, 50.f, 1e9f};   // WFS-299 : 3 seuils (pas de niveau 3 ici)

    {   // RG-02 : hors du cadre, l'état suit sans fondu ; à l'entrée dans le cadre par le bord, la personne est déjà là
        Etat e;
        avancerOpacite(e, true, false, false, 1.f, dt);
        cas("RG-02 : hors du cadre, opacité pleine tout de suite (personne ne la voit)", e.op == 1.f && e.lieu == Lieu::HorsCadre);
        avancerOpacite(e, true, true, true, 1.f, dt);
        cas("RG-02 : entrée dans le cadre par le bord : pleine (elle marchait déjà là)", e.op == 1.f && e.lieu == Lieu::Affichee);
    }
    {   // RG-03 : apparition dans le cadre (bord de la portée) : fondu d'au moins 0,5 s
        Etat e; int images = 0; float pasMax = 0, prec = 0;
        while (e.op < 1.f && images < 100) { avancerOpacite(e, true, true, true, 1.f, dt); pasMax = std::max(pasMax, e.op - prec); prec = e.op; images++; }
        cas("RG-03 : apparition dans le cadre en fondu ≥ 0,5 s (" + std::to_string(images) + " images à 20 img/s)", images >= 10 && pasMax <= (float)(dt / FONDU_S) + 1e-6f);
        // disparition dans le cadre (sortie de la portée vers le loin) : fondu aussi
        images = 0;
        while (e.op > 0.f && images < 100) { avancerOpacite(e, true, true, true, 0.f, dt); images++; }
        cas("RG-03 : disparition dans le cadre en fondu ≥ 0,5 s (" + std::to_string(images) + " images)", images >= 10);
    }
    {   // fondu de distance au bord de la portée
        cas("bord de la portée : opacité 1 à 118 m, 0,5 à 128 m, 0 à 138 m",
            opaciteDistance(118, 138) == 1.f && std::fabs(opaciteDistance(128, 138) - 0.5f) < 1e-6f && opaciteDistance(138, 138) == 0.f);
    }
    {   // sans place dans le cadre : pas encore là ; une place libérée ensuite : fondu, pas d'apparition brutale
        Etat e;
        avancerOpacite(e, true, false, false, 1.f, dt);
        avancerOpacite(e, true, true, false, 1.f, dt);
        cas("RG-02 : dans le cadre sans place : invisible", e.op == 0.f);
        avancerOpacite(e, true, true, true, 1.f, dt);
        cas("RG-03 : place donnée ensuite dans le cadre : fondu (pas d'un coup)", e.op > 0.f && e.op <= (float)(dt / FONDU_S) + 1e-6f);
    }
    {   // RG-04 : hystérésis des niveaux de détail
        Etat e; e.lieu = Lieu::Affichee; e.op = 1;
        avancerNiveau(e, 15.f, seuils, dt);
        cas("détail : complet à 15 m", e.lod == 0);
        int changements = 0, prec = e.lod;
        for (int k = 0; k < 40; k++) { avancerNiveau(e, k % 2 ? 15.5f : 16.5f, seuils, dt); if (e.lod != prec) changements++; prec = e.lod; }
        cas("RG-04 : distance qui oscille de ± 0,5 m autour du seuil : aucun changement de détail", changements == 0);
        avancerNiveau(e, 18.f, seuils, dt);
        cas("RG-03 : changement de détail dans le cadre : fondu croisé (ancien niveau gardé)", e.lod == 1 && e.lodPrec == 0 && e.lodT == 0.f);
        int images = 0;
        while (e.lodPrec >= 0 && images < 100) { avancerNiveau(e, 18.f, seuils, dt); images++; }
        cas("RG-03 : fondu croisé de détail ≥ 0,5 s (" + std::to_string(images) + " images)", images >= 10);
        Etat h; h.lieu = Lieu::HorsCadre; h.op = 1; h.lod = 0;
        avancerNiveau(h, 60.f, seuils, dt);
        cas("détail hors du cadre : changement immédiat (personne ne le voit)", h.lod == 2 && h.lodPrec < 0);
        Etat f; f.lieu = Lieu::Affichee; f.op = 0.5f; f.lod = 2;
        avancerNiveau(f, 40.f, seuils, dt);
        cas("détail pendant un fondu d'opacité : le changement attend", f.lod == 2 && f.lodPrec < 0);
        // WFS-299 : niveau 3 « très simple » au-delà du 3e seuil (Basse : 8 / 20 / 30 m), fondu croisé 2 → 3 dans le cadre
        const float basse[3] = {8.f, 20.f, 30.f};
        Etat b; b.lieu = Lieu::Affichee; b.op = 1; b.lod = 2;
        avancerNiveau(b, 25.f, basse, dt);
        cas("WFS-299 : loin (25 m) : niveau 2", b.lod == 2 && b.lodPrec < 0);
        avancerNiveau(b, 35.f, basse, dt);
        cas("WFS-299 : au-delà de 30 m : niveau 3 en fondu croisé depuis le 2", b.lod == 3 && b.lodPrec == 2);
        cas("WFS-299 : sans 3e seuil (Haute) : jamais de niveau 3", niveauVoulu(500.f, seuils, -1) == 2);
    }
    {   // tramage : complément exact des deux niveaux pendant le fondu croisé
        cas("tramage : 0 sans fondu, −(1 + t) et −(3 + t) pendant un fondu",
            tramage(1.f) == 0.f && std::fabs(tramage(0.25f) + 1.25f) < 1e-6f && std::fabs(tramageComplement(0.25f) + 3.25f) < 1e-6f && tramageComplement(0.f) == 0.f);
    }
    {   // places : les personnes affichées dans le cadre gardent la leur, même au-delà du budget ; famille ensemble
        std::vector<Candidat> c = {{0, -1, 90.f, true, 2}, {1, -1, 5.f, false, 2}, {2, -1, 80.f, true, 2}, {3, 4, 20.f, false, 2}, {4, 3, 21.f, false, 1}, {5, -1, 30.f, false, 2}};
        std::vector<int> ordre; std::vector<char> place;
        choisirPlaces(c, ordre, 6, place);   // 4 instances déjà prises par les affichées, 2 libres
        cas("RG-02 : affichées gardées au-delà du budget (90 m et 80 m)", place[0] && place[2]);
        cas("places : la plus proche des nouvelles d'abord (5 m), dans le budget", place[1]);
        cas("WFS-233 : parent et enfant ensemble ou pas du tout (budget dépassé : aucun des deux)", place[3] == place[4] && !place[3]);
        choisirPlaces(c, ordre, 9, place);
        cas("WFS-233 : budget suffisant : la famille entre ensemble", place[3] && place[4]);
    }
    {   // RG-02 : sortie de la portée (derrière la caméra : hors du cadre) : remise à zéro sans effet visible
        Etat e; e.lieu = Lieu::HorsCadre; e.op = 1; e.lod = 1;
        avancerOpacite(e, false, false, false, 0.f, dt);
        cas("hors de la portée : absente, détail oublié", e.op == 0.f && e.lieu == Lieu::Absente && e.lod < 0);
    }
    {   // WFS-317 (RG-03) : Ville : 3 sur 10 gardés, réguliers (jamais plus de 3 omis de suite), sur toute longueur de liste
        int gardes = 0, trou = 0, trouMax = 0;
        for (int n = 0; n < 1000; n++) { if (garderVille(n)) { gardes++; trou = 0; } else trouMax = std::max(trouMax, ++trou); }
        int g10 = 0; for (int n = 0; n < 10; n++) g10 += garderVille(n);
        cas("WFS-317 : Ville : 300 gardés sur 1 000, 3 sur chaque dizaine, au plus 3 omis de suite", gardes == 300 && g10 == 3 && trouMax <= 3);
        cas("WFS-317 : rang négatif : jamais gardé", !garderVille(-1));
        // marcheurs répartis : candidats serrés au début du tour (comme le plafond de life.js) et clairsemés ailleurs
        std::vector<double> s; for (int i = 0; i < 500; i++) s.push_back(i * 2.0); for (int i = 0; i < 33; i++) s.push_back(1000 + i * 150.0);
        const double L = 6000;
        const std::vector<size_t> a = repartirSurTour(s, L, 16, false), b = repartirSurTour(s, L, 16, true);
        double ecartMax = 0; std::vector<double> p; for (size_t i : a) p.push_back(s[i]); for (size_t i : b) p.push_back(s[i]);
        std::sort(p.begin(), p.end()); for (size_t i = 0; i < p.size(); i++) ecartMax = std::max(ecartMax, (i + 1 < p.size() ? p[i + 1] : p[0] + L) - p[i]);
        bool unique = true; for (size_t i = 0; i < a.size(); i++) for (size_t j = i + 1; j < a.size(); j++) unique = unique && a[i] != a[j];
        cas("WFS-317 : 32 marcheurs sur un tour de 6 000 m : tous placés, uniques, aucun écart > 2 pas (375 m)", a.size() == 16 && b.size() == 16 && unique && ecartMax <= 375);
        cas("WFS-317 : liste plus courte que voulu ou vide : jamais plus que la liste",
            repartirSurTour({1, 2, 3}, 10, 9, true).size() == 3 && repartirSurTour({}, 10, 5, false).empty() && repartirSurTour({1}, 10, 0, false).empty());
    }
    std::printf("apparition : %s (%d échec(s))\n", echecs ? "ÉCHEC" : "OK", echecs);
    return echecs ? 1 : 0;
}
