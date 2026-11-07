// WFS-233 (RG-02, RG-03) : règles des familles (src/world/foule_familles.h), sans rendu. Inclus par tests/fidelite_humains.cpp.
//   - total des piétons inchangé (les enfants remplacent des adultes) ; part d'enfants proche de la part visée par lieu ;
//   - chaque enfant à côté d'un adulte : même sens, même vitesse, écart latéral 0,65 m, 0,15 m devant, côté façades ;
//   - personne sur la chaussée (|lat| ≥ bord du trottoir) ni hors de sa bande de trottoir ; vitesse de famille 0,9 à 1,15 m/s ;
//   - aucun enfant dans un lieu sans part (carte inconnue) ; déterministe pour une graine.
#include <cstdio>
#include <string>
#include <vector>
#include "../src/world/foule_familles.h"

namespace familles_test {
using namespace wfs::familles;

inline int executer() {
    int echecs = 0;
    auto cas = [&](const std::string& quoi, bool ok) { std::printf("familles : %-94s → %s\n", quoi.c_str(), ok ? "OK" : "ÉCHEC"); if (!ok) echecs++; };
    const double RW_ = 7.5, SEG_M_ = 1.2;
    // une rue de 2 000 tronçons : marcheurs des deux trottoirs (bande RW + 1,4 … RW + 3,2) et 30 % d'autres piétons (assis…)
    auto rue = [&](uint32_t graine, std::vector<Marcheur>& m, int& total) {
        wfs::Mulberry32 R(graine);
        m.clear();
        for (int n = 0; n < 2000; n += 3) {
            if (R.next() < 0.4) { Marcheur a; a.s = n + R.next() * 3; a.latMin = RW_ + 1.4; a.latMax = RW_ + 3.2; a.lat = (R.next() < 0.5 ? -1 : 1) * (a.latMin + R.next() * 1.8); a.v = 1.1 + R.next() * 0.6; a.dir = R.next() < 0.5 ? -1 : 1; a.candidat = true; m.push_back(a); }
        }
        total = (int)(m.size() / 0.7);
    };
    for (const char* carte : {"ville", "plage", "campagne", "circuit"}) {
        std::vector<Marcheur> m; int total = 0;
        rue(17, m, total);
        const std::vector<Marcheur> avant = m;
        const double part = partEnfants(carte);
        const std::vector<Lien> liens = former(m, total, part, 1234, SEG_M_);
        const std::string c = carte;
        cas(c + " : total inchangé (" + std::to_string(m.size()) + " marcheurs)", m.size() == avant.size());
        const double obtenu = (double)liens.size() / total;
        char t[128]; std::snprintf(t, sizeof t, " : %.1f %% d'enfants pour %.0f %% visés (± 3 points)", obtenu * 100, part * 100);
        cas(c + t, std::fabs(obtenu - part) <= 0.03 && part >= 0.10 && part <= 0.15);
        bool cote = true, ensemble = true, trottoir = true, vitesse = true, uniques = true;
        std::vector<int> vu(m.size(), 0);
        for (const Lien& l : liens) {
            const Marcheur& P = m[(size_t)l.parent]; const Marcheur& E = m[(size_t)l.enfant];
            uniques = uniques && !vu[(size_t)l.parent]++ && !vu[(size_t)l.enfant]++ && l.morpho >= 0 && l.morpho < NB_ENFANTS;
            ensemble = ensemble && P.dir == E.dir && P.v == E.v && std::fabs(std::fabs(E.lat - P.lat) - ECART_LAT_M) < 1e-9
                       && std::fabs((E.s - P.s) * SEG_M_ - P.dir * AVANCE_M) < 1e-9;
            cote = cote && std::fabs(E.lat) > std::fabs(P.lat) && (E.lat > 0) == (P.lat > 0);   // enfant côté façades
            for (const Marcheur* x : {&P, &E}) trottoir = trottoir && std::fabs(x->lat) >= x->latMin - 1e-9 && std::fabs(x->lat) <= x->latMax + 1e-9 && std::fabs(x->lat) > RW_;
            vitesse = vitesse && P.v >= VITESSE_MIN - 1e-9 && P.v <= vitesseMax(l.morpho) + 1e-9 && vitesseMax(l.morpho) <= 1.15;
        }
        cas(c + " : chaque enfant avec un adulte (même sens et vitesse, 0,65 m à côté, 0,15 m devant)", ensemble && uniques && !liens.empty());
        cas(c + " : l'enfant côté façades, personne sur la chaussée ni hors du trottoir", cote && trottoir);
        cas(c + " : vitesse de la famille entre 0,9 m/s et la vitesse de l'enfant (≤ 1,15 m/s)", vitesse);
        int ages[4] = {};
        for (const Lien& l : liens) ages[l.morpho / 2]++;
        cas(c + " : les quatre âges présents", liens.size() < 8 || (ages[0] && ages[1] && ages[2] && ages[3]));
        std::vector<Marcheur> m2 = avant;
        const std::vector<Lien> l2 = former(m2, total, part, 1234, SEG_M_);
        bool memes = l2.size() == liens.size();
        for (size_t k = 0; memes && k < l2.size(); k++) memes = l2[k].parent == liens[k].parent && l2[k].enfant == liens[k].enfant && l2[k].morpho == liens[k].morpho;
        cas(c + " : déterministe pour une graine", memes);
    }
    {   // bande trop étroite pour deux (< 0,65 m), lieu sans part, aucun candidat : aucune famille
        std::vector<Marcheur> m(40); for (Marcheur& a : m) { a.candidat = true; a.latMin = RW_ + 0.3; a.latMax = RW_ + 0.8; a.lat = RW_ + 0.5; }
        cas("bande de 0,5 m : aucune famille", former(m, 40, 0.15, 1, SEG_M_).empty());
        std::vector<Marcheur> m2(40); for (Marcheur& a : m2) { a.candidat = true; a.latMin = RW_ + 1; a.latMax = RW_ + 3; a.lat = RW_ + 2; }
        cas("carte sans part d'enfants : aucune famille", former(m2, 40, partEnfants("garage"), 1, SEG_M_).empty());
        for (Marcheur& a : m2) a.candidat = false;
        cas("aucun marcheur candidat (assis, joggeurs, passages) : aucune famille", former(m2, 40, 0.15, 1, SEG_M_).empty());
    }
    // main dans la main à 4 et 7 ans seulement ; cadence sans glissement (v / (1,1 × rapport des bassins))
    cas("main dans la main : 4 et 7 ans oui, 10 et 14 ans non", tientLaMain(0) && tientLaMain(3) && !tientLaMain(4) && !tientLaMain(7));
    cas("cadence : 1,0 m/s, bassin 0,55 → 1,65 × le clip", std::fabs(cadence(1.0, 0.55) - 1.0 / (1.1 * 0.55)) < 1e-12);
    {   // teint : l'enfant (teinte unie MakeHuman) rend la même couleur sRGB que son parent (atlas Quaternius × peau)
        const wfs::vec3 pe{0.6f, 0.45f, 0.36f}, e = peauEnfant(pe);
        const bool ok = std::fabs(e.x * 222 - pe.x * 169) < 1e-3 && std::fabs(e.y * 178 - pe.y * 114) < 1e-3 && std::fabs(e.z * 150 - pe.z * 80) < 1e-3;
        cas("teint de l'enfant = teint du parent (sRGB, au millième)", ok);
    }
    return echecs;
}
} // namespace familles_test
