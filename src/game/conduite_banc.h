// WaForSpeed — banc de conduite (WFS-241, WFS_BANC=conduite ; aussi tests/test_conduite.cpp, sans GPU) :
//  1. fiches : pour les 20 voitures, 0-100, vitesse maximale, freinage 100-0 (ABS), accélération latérale maximale, rayon de
//     braquage, comparés à la fiche (cars.h) et aux repères du skill expert-conduite ;
//  2. virages (RG-01) : épingle du Grand Prix, lacets de la Campagne, ronds-points de la Ville, de la Campagne et de la
//     Corniche, grande courbe à 150 km/h ; joueur simulé (a) volant lâché, (b) ligne idéale, (c) braquage à fond à gauche
//     puis à droite ; aide activée puis désactivée ; ancien modèle (« avant ») et nouveau modèle (« après ») ;
//  3. RG-07 : aide désactivée, aucun appui, 240 km/h avant le rond-point de la Ville : ni direction, ni frein, ni nitro ;
//  4. déterminisme : même entrée, même trajectoire (duel en ligne).
// Seul le modèle « après » compte pour le verdict (écarts) ; « avant » est mesuré pour le tableau du journal.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "aide_direction_banc.h"
#include "player.h"
#include "trace_cartes.h"
#include "track.h"

namespace wfs::banc_conduite {

struct Virage { const char* nom; const char* carte; double debutU, finU; double vitesse; };   // vitesse : 0 = vitesse normale du virage
struct Mesure { double ecartMax = 0, capMax = 0, deriveMax = 0, vMin = 1e9, vMax = 0, dGauche = 0, dDroite = 0; int sorties = 0; bool passe = false; };
enum class Cas { Lache, Ideale, AFond };
inline const char* nomCas(Cas c) { return c == Cas::Lache ? "(a) volant lâché" : c == Cas::Ideale ? "(b) ligne idéale" : "(c) à fond G puis D"; }

inline Track& piste(const char* carte) {   // une piste par carte, construite une fois (même tracé que la partie)
    static Track t[4]; static const char* ids[4] = {"circuit", "campagne", "ville", "plage"}; static bool ok[4] = {};
    for (int i = 0; i < 4; i++) if (!std::strcmp(ids[i], carte)) { if (!ok[i]) { t[i].build(mapById(carte), trace::versionCarte(carte)); ok[i] = true; } return t[i]; }
    return t[0];
}

// les 6 virages du banc (RG-01)
inline std::vector<Virage> virages() {
    std::vector<Virage> v;
    auto section = [&](const char* nom, const char* carte, trace::Type type) {
        Track& t = piste(carte);
        for (const trace::Section& s : trace::sections(t)) {
            if (s.type != type) continue;
            int64_t debut = s.debut, fin = s.fin;
            if (type == trace::Type::Lacets) {   // 1re épingle des lacets : la 1re partie serrée (rayon de moins de 45 m)
                while (debut < s.fin && std::abs(t.segAt(debut).curve) * CURVE_K < SEG_M / 45) debut++;
                fin = debut; while (fin < s.fin && std::abs(t.segAt(fin).curve) * CURVE_K >= SEG_M / 45) fin++;
            }
            v.push_back({nom, carte, debut * SEG, fin * SEG, 0});
            return;
        }
        std::printf("banc conduite : %s sans %s\n", carte, trace::nomType(type));
    };
    section("épingle du Grand Prix", "circuit", trace::Type::Epingle);
    section("lacets de la Campagne", "campagne", trace::Type::Lacets);
    section("rond-point de la Ville", "ville", trace::Type::RondPoint);
    section("rond-point de la Campagne", "campagne", trace::Type::RondPoint);
    section("rond-point de la Corniche", "plage", trace::Type::RondPoint);
    {   // grande courbe : rayon de 250 à 900 m sur 150 m au moins (0,7 g au plus à 150 km/h), Grand Prix
        Track& t = piste("circuit");
        const int64_t n = (int64_t)t.segmentCount(), lmin = (int64_t)(150 / SEG_M);
        for (int64_t a = 0, run = 0; a < n; a++) {
            const double k = std::abs(t.segAt(a).curve) * CURVE_K / SEG_M;
            run = (k > 1.0 / 900 && k < 1.0 / 250) ? run + 1 : 0;
            if (run == lmin) { v.push_back({"grande courbe à 150 km/h", "circuit", (a - lmin + 1) * SEG, (a + 1) * SEG, 150 / 3.6 / M}); break; }
        }
        if (v.size() < 6) std::printf("banc conduite : pas de grande courbe sur le Grand Prix\n");
    }
    return v;
}

// une traversée : départ 80 m avant (à la vitesse visée), arrivée 40 m après ; pas de 1/120 s
inline Mesure rouler(const Virage& vg, Cas cas, Aide aide, bool monde, std::string* trace = nullptr) {
    Track& t = piste(vg.carte);
    const bool avant = g_conduiteMonde; g_conduiteMonde = monde;
    Player p; p.reset(carById("golf")); p.aide = aide;
    const double depart = vg.debutU - 80 / M, fin = vg.finU + 40 / M;
    auto vitesse = [&](double d) { return vg.vitesse > 0 ? vg.vitesse : vitesseVirageNormale(t, d); };
    p.dist = depart; p.position = t.wrap(depart); p.speed = vitesse(depart);
    Mesure m;
    double latDebut = 0;
    for (int n = 0; p.dist < fin && n < 120 * 90; n++) {
        DriveInput in = joueurLigneIdeale(p, t, cas == Cas::Ideale);
        const double cible = vitesse(p.dist);
        in.gas = p.speed < cible * 0.97; in.brake = p.speed > cible * 1.06;
        if (cas == Cas::AFond && p.dist >= vg.debutU) {   // dès l'entrée : 1 s à fond à gauche, 1 s à fond à droite, puis lâché
            static thread_local int k0; if (p.dist - p.speed / 120 < vg.debutU) { k0 = n; latDebut = p.playerX * RW; }
            const int k = n - k0;
            in.steer = k < 120 ? -1.f : k < 240 ? 1.f : 0.f;
            if (k == 120) m.dGauche = p.playerX * RW - latDebut;
            if (k == 240) m.dDroite = p.playerX * RW - latDebut - m.dGauche;
        }
        p.update(1.0 / 120, in, t, true);
        if (p.dist > vg.debutU - 20 / M) {
            m.ecartMax = std::max(m.ecartMax, std::abs(p.playerX * RW));
            m.capMax = std::max(m.capMax, std::abs(p.heading));
            m.deriveMax = std::max(m.deriveMax, std::abs(std::atan2(p.monde.v(), std::max(std::abs(p.monde.u()), 1.0))));   // WFS-253 : angle de dérive
            if (std::abs(p.playerX) > 1) m.sorties++;
        }
        if (p.dist > vg.debutU && p.dist < vg.finU) { m.vMin = std::min(m.vMin, p.kmh()); m.vMax = std::max(m.vMax, p.kmh()); }
        if (trace && n % 60 == 0) {
            char l[200]; std::snprintf(l, sizeof l, "      %+6.1f m du virage : écart %+5.2f m, cap %+5.2f rad, volant %+5.2f, roues %+5.3f rad, %3.0f km/h\n",
                                       (p.dist - vg.debutU) * M, p.playerX * RW, p.heading, p.steer, p.braquageRoues, p.kmh());
            *trace += l;
        }
    }
    m.passe = p.dist >= fin;
    g_conduiteMonde = avant;
    return m;
}

// ---------------------------------------------------------------- WFS-253 : mesures des aides (ABS, antipatinage, ESP)
struct MesureAides {
    double arret = 0, glisFrein = 0;               // ABS : 100 → 0 en ligne droite (m), glissement max des roues avant
    double capFrein = 0;                            // ABS : cap gagné en freinant à fond, volant tourné (rad) : la voiture se dirige-t-elle ?
    double cent = 0, patinage = 0;                  // antipatinage : 0-100 (s), glissement max des roues motrices
    double rPic = 0, r1 = 0, r175 = 0, betaMax = 0; // ESP : sinus avec palier (FMVSS 126) : lacet max, à 1 s et 1,75 s après, dérive max
    bool toupie = false;                            // tête-à-queue (dérive > 90°)
};

inline MesureAides mesurerAides(const CarDef& car, bool abs, bool tc, bool esp) {
    using namespace physique;
    const Params& p = params(car);
    MesureAides m;
    auto cmd = [&]() { Commande c; c.abs = abs; c.antipatinage = tc; c.esp = esp; return c; };
    const double dt = 1.0 / 120;
    {   // 100 → 0 km/h, frein à fond, volant droit
        Etat e; e.vz = -100 / 3.6; Commande c = cmd(); c.frein = 1; Sol s;
        for (int i = 0; i < 120 * 10 && e.u() > 0.01; i++) { const double z0 = e.z; pas(p, e, c, s, dt); m.arret += z0 - e.z; m.glisFrein = std::max(m.glisFrein, e.glissementAv); }
    }
    {   // freinage d'urgence en tournant : 100 km/h, volant tourné pour 0,4 g puis frein à fond pendant 1,5 s : cap gagné
        Etat e; e.vz = -100 / 3.6; Commande c = cmd(); Sol s;
        const double v = 100 / 3.6, d = p.L * 0.4 * G / (v * v) * 1.6;   // angle des roues (sous-virage compris, approché)
        c.braquage = d;
        for (int i = 0; i < 120 * 2; i++) { c.gaz = e.u() < v ? 0.3 : 0; pas(p, e, c, s, dt); }   // virage établi
        const double cap0 = e.cap; c.gaz = 0; c.frein = 1;
        for (int i = 0; i < 180 && e.u() > 1; i++) pas(p, e, c, s, dt);
        m.capFrein = e.cap - cap0;
    }
    {   // 0-100 km/h plein gaz : temps et patinage des roues motrices
        Etat e; Commande c = cmd(); c.gaz = 1; Sol s;
        double t = 0;
        for (; e.u() < 100 / 3.6 && t < 40; t += dt) {
            pas(p, e, c, s, dt);
            m.patinage = std::max(m.patinage, p.transmission == Transmission::Arriere ? e.glissementAr : std::max(e.glissementAv, e.glissementAr));
        }
        m.cent = t;
    }
    {   // ESP : « sinus avec palier » (NHTSA FMVSS 126, S7.9) à 80 km/h, sans gaz : sinus de 0,7 Hz, palier de 0,5 s au 2e sommet,
        // amplitude 6,5 × l'angle qui donne 0,3 g en virage établi ; critères : lacet ≤ 35 % du pic 1 s après la fin, ≤ 20 % à 1,75 s
        const double v = 80 / 3.6;
        double d03 = 0;
        {   // angle de roues pour 0,3 g établi à 80 km/h (volant qui monte lentement)
            Etat e; e.vz = -v; Commande c = cmd(); Sol s;
            for (int i = 0; i < 120 * 40; i++) {
                c.braquage = d03; c.gaz = e.u() < v ? std::clamp((v - e.u()) * 2, 0.0, 1.0) : 0; pas(p, e, c, s, dt);
                if (std::abs(e.u() * e.r) >= 0.3 * G) break;
                d03 += 0.002 / 120;
            }
        }
        const double A = std::min(6.5 * d03, p.braquageMax), f = 0.7, w = 2 * 3.14159265358979 * f;
        Etat e; e.vz = -v; Commande c = cmd(); Sol s;
        const double t1 = 0.75 / f, tFin = t1 + 0.5 + 0.25 / f;   // 3/4 de période, palier de 0,5 s, dernier quart
        double t = 0;
        for (int i = 0; t < tFin + 2.0; i++, t += dt) {
            double dd;
            if (t < 0.75 / f) dd = A * std::sin(w * t);
            else if (t < 0.75 / f + 0.5) dd = -A;
            else if (t < tFin) dd = A * std::sin(w * (t - 0.5));
            else dd = 0;
            c.braquage = dd;
            pas(p, e, c, s, dt);
            const double beta = std::atan2(e.v(), std::max(std::abs(e.u()), 1.0));
            m.betaMax = std::max(m.betaMax, std::abs(beta));
            if (std::abs(beta) > 3.14159265358979 / 2 || e.u() < 0) m.toupie = true;
            if (t > 0.75 / f) m.rPic = std::max(m.rPic, std::abs(e.r));   // pic de lacet après le 1er sommet (comme la norme)
            if (std::abs(t - (tFin + 1.0)) < dt / 2) m.r1 = std::abs(e.r);
            if (std::abs(t - (tFin + 1.75)) < dt / 2) m.r175 = std::abs(e.r);
        }
    }
    return m;
}

inline int mesurerAidesRapport(std::string& rapport) {
    int ecarts = 0; char l[400];
    auto ligne = [&](const char* s) { rapport += s; rapport += "\n"; };
    ligne("## 6. Aides (WFS-253) : ABS, antipatinage, ESP activés (jeu) puis coupés un par un (banc seulement)");
    ligne("| voiture | aides | 100-0 | glissement AV au freinage | cap gagné en freinant en virage | 0-100 | patinage au départ | ESP (FMVSS 126) : lacet 1 s / 1,75 s après | dérive max | verdict |");
    ligne("|---|---|---|---|---|---|---|---|---|---|");
    for (const char* id : {"golf", "mustang", "sf90", "rrsport"}) {
        struct Cfg { const char* nom; bool abs, tc, esp; };
        for (const Cfg& k : {Cfg{"toutes", true, true, true}, Cfg{"sans ABS", false, true, true}, Cfg{"sans antipatinage", true, false, true}, Cfg{"sans ESP", true, true, false}}) {
            const MesureAides m = mesurerAides(carById(id), k.abs, k.tc, k.esp);
            const double q1 = m.rPic > 0 ? m.r1 / m.rPic : 0, q175 = m.rPic > 0 ? m.r175 / m.rPic : 0;
            std::string verdict = "mesure";
            if (k.abs && k.tc && k.esp) {   // configuration du jeu : critères
                const bool bon = m.arret >= 33 && m.arret <= 43 && m.glisFrein <= 0.25 && m.capFrein > 0.05 && q1 <= 0.35 && q175 <= 0.20 && !m.toupie;
                if (!bon) ecarts++;
                verdict = bon ? "OK" : "ÉCART";
            }
            std::snprintf(l, sizeof l, "| %s | %s | %.1f m | %.0f %% | %.2f rad | %.2f s | %.0f %% | %.0f %% / %.0f %% (pic %.2f rad/s) | %.1f°%s | %s |", id, k.nom, m.arret,
                          m.glisFrein * 100, m.capFrein, m.cent, m.patinage * 100, q1 * 100, q175 * 100, m.rPic, m.betaMax * 57.2958, m.toupie ? " TÊTE-À-QUEUE" : "", verdict.c_str());
            ligne(l);
        }
    }
    return ecarts;
}

struct Fiche { const char* id; double cent, fiche100, vmax, ficheVmax, arret, ayMax, braquage; };

inline Fiche mesurerFiche(const CarDef& c) {
    using namespace physique;
    const Params& p = params(c);
    Fiche f{c.id, p.cent, c.spec.s100, p.vmax * 3.6, (double)c.spec.vmax, 0, 0, 0};
    {   // 100 → 0, frein à fond (ABS)
        Etat e; e.vz = -100 / 3.6; Commande cm; cm.frein = 1; Sol s;
        for (int i = 0; i < 120 * 10 && e.u() > 0.01; i++) { const double z0 = e.z; pas(p, e, cm, s, 1.0 / 120); f.arret += z0 - e.z; }
    }
    {   // accélération latérale maximale : vitesse tenue (15 et 30 m/s), volant qui augmente lentement
        for (const double v : {15.0, 30.0}) {
            Etat e; e.vz = -v; double d = 0;
            for (int i = 0; i < 120 * 60; i++) {
                Commande cm; d += 0.02 / 120; cm.braquage = std::min(d, braquageMax(p, v));
                cm.gaz = e.u() < v ? std::clamp((v - e.u()) * 2, 0.0, 1.0) : 0; cm.frein = e.u() > v + 0.3 ? 0.2 : 0;
                Sol s; pas(p, e, cm, s, 1.0 / 120);
                f.ayMax = std::max(f.ayMax, std::abs(e.u() * e.r) / G);
            }
        }
    }
    {   // diamètre de braquage : volant à fond à 10 km/h (cercle décrit par le centre de la voiture + largeur)
        Etat e; e.vz = -10 / 3.6; Commande cm; cm.braquage = p.braquageMax; Sol s;
        for (int i = 0; i < 120 * 4; i++) { cm.gaz = e.u() < 10 / 3.6 ? 0.1 : 0; pas(p, e, cm, s, 1.0 / 120); }
        f.braquage = 2 * std::abs(e.u() / e.r) + c.d.W;
    }
    return f;
}

// tout le banc (sans GPU) ; rapport : tableau lisible (journal, conduite.txt) ; rend le nombre d'écarts du modèle « après »
inline int mesurer(std::string& rapport) {
    int ecarts = 0;
    char l[400];
    auto ligne = [&](const char* s) { rapport += s; rapport += "\n"; };
    ligne("## 1. Fiches (nouveau modèle, plat, route sèche) : 0-100 ±5 %, vitesse maxi ±3 %, 100-0 de 33 à 43 m, latéral 0,80 à 1,05 g");
    ligne("| voiture | 0-100 (fiche) | v maxi (fiche) | 100-0 | latéral maxi | diamètre de braquage |");
    ligne("|---|---|---|---|---|---|");
    for (const CarDef& c : CARS) {
        const Fiche f = mesurerFiche(c);
        const bool bon = std::abs(f.cent / f.fiche100 - 1) <= 0.05 && std::abs(f.vmax / f.ficheVmax - 1) <= 0.03 && f.arret >= 33 && f.arret <= 43 &&
                         f.ayMax >= 0.80 && f.ayMax <= 1.05 && f.braquage > 8.5 && f.braquage < 13.5;
        if (!bon) ecarts++;
        std::snprintf(l, sizeof l, "| %s | %.2f s (%.1f) | %.0f km/h (%.0f) | %.1f m | %.2f g | %.1f m |%s", f.id, f.cent, f.fiche100, f.vmax, f.ficheVmax, f.arret,
                      f.ayMax, f.braquage, bon ? "" : " ÉCART");
        ligne(l);
    }
    ligne("");
    ligne("## 2. Virages (Golf) : écart max au centre (m), images hors de la route, cap max par rapport à la route (rad), vitesse, virage passé");
    const std::vector<Virage> vs = virages();
    if (vs.size() < 6) ecarts++;
    for (const Virage& vg : vs) {
        std::snprintf(l, sizeof l, "### %s (%s)", vg.nom, vg.carte); ligne(l);
        ligne("| cas | aide | AVANT (ancien modèle) | APRÈS (nouveau modèle) | verdict |");
        ligne("|---|---|---|---|---|");
        Mesure apres[3][2];
        for (Cas cas : {Cas::Lache, Cas::Ideale, Cas::AFond}) for (int ia = 0; ia < 2; ia++) {
            const Aide aide = ia == 0 ? Aide::Activee : Aide::Desactivee;
            const Mesure av = rouler(vg, cas, aide, false), ap = rouler(vg, cas, aide, true);
            apres[(int)cas][ia] = ap;
            auto txt = [](const Mesure& m) {
                char b[160]; std::snprintf(b, sizeof b, "écart %.1f m, %d hors route, cap %.2f, %.0f-%.0f km/h, %s%s", m.ecartMax, m.sorties, m.capMax, m.vMin > 1e8 ? 0 : m.vMin, m.vMax,
                                           m.passe ? "passé" : "NON passé", "");
                std::string s = b;
                if (m.dGauche != 0 || m.dDroite != 0) { std::snprintf(b, sizeof b, ", à fond : %+.1f m à gauche puis %+.1f m", m.dGauche, m.dDroite); s += b; }
                return s;
            };
            std::string verdict = "—";
            if (cas == Cas::Lache && aide == Aide::Activee) verdict = ap.sorties == 0 && ap.passe ? "OK (suit la voie)" : "ÉCART (doit suivre la voie sans volant)";
            if (cas == Cas::Lache && aide == Aide::Desactivee) verdict = ap.sorties > 0 ? "OK (va tout droit)" : "ÉCART (doit aller tout droit)";
            if (cas == Cas::Ideale) verdict = ap.sorties == 0 && ap.passe ? "OK" : "ÉCART";
            if (verdict.rfind("ÉCART", 0) == 0) ecarts++;
            std::snprintf(l, sizeof l, "| %s | %s | %s | %s | %s |", nomCas(cas), ia == 0 ? "activée" : "désactivée", txt(av).c_str(), txt(ap).c_str(), verdict.c_str());
            ligne(l);
        }
        // (c) l'aide cède au joueur : à fond à gauche, la voiture se déplace au moins 70 % autant qu'aide désactivée
        const Mesure& ca = apres[2][0]; const Mesure& cd = apres[2][1];
        const bool cede = std::abs(ca.dGauche) >= 0.7 * std::abs(cd.dGauche) && ca.dGauche < -0.5;
        if (!cede) ecarts++;
        std::snprintf(l, sizeof l, "| (c) l'aide cède | — | — | à fond à gauche : %+.1f m aide activée, %+.1f m désactivée | %s |", ca.dGauche, cd.dGauche, cede ? "OK" : "ÉCART");
        ligne(l);
    }
    ligne("");
    {   // 3. RG-07 : aide désactivée, aucun appui, 240 km/h, 150 m avant le rond-point de la Ville
        ligne("## 3. RG-07 : aide désactivée, aucun appui, 240 km/h, 150 m avant le rond-point de la Ville (vidéo 4 de la VABF)");
        for (const bool monde : {false, true}) {
            Track& t = piste("ville");
            Virage rp{}; for (const Virage& v : vs) if (!std::strcmp(v.carte, "ville")) rp = v;
            const bool av0 = g_conduiteMonde; g_conduiteMonde = monde;
            Player p; p.reset(carById("golf")); p.aide = Aide::Desactivee;
            p.dist = rp.debutU - 150 / M; p.position = t.wrap(p.dist); p.speed = 240 / 3.6 / M;
            double volantMax = 0, rouesMax = 0, decelMax = 0, lacetMax = 0, cap0 = 0; bool nitro = false, frein = false;
            int n = 0;
            for (; n < 120 * 3; n++) {
                const double v0 = p.speed;
                p.update(1.0 / 120, DriveInput{}, t, true);
                if (std::abs(p.playerX) > (t.map() ? t.map()->edge : 1.6) - 0.01) break;   // contact avec la glissière : fin de la mesure
                volantMax = std::max(volantMax, std::abs(p.steer)); rouesMax = std::max(rouesMax, std::abs(p.braquageRoues));
                decelMax = std::max(decelMax, (v0 - p.speed) * M * 120 / 9.81); nitro = nitro || p.nitroOn; frein = frein || p.braking;
                const double capMonde = t.pointLisse(p.dist, 0).th + p.heading;
                if (n == 0) cap0 = capMonde;
                lacetMax = std::max(lacetMax, std::abs(std::remainder(capMonde - cap0, 2 * 3.14159265358979)));
            }
            g_conduiteMonde = av0;
            const bool bon = volantMax == 0 && rouesMax == 0 && !nitro && !frein && decelMax < 0.3 && lacetMax < 0.01;
            if (monde && !bon) ecarts++;
            std::snprintf(l, sizeof l, "- %s : %.2f s sans appui, volant max %.2f, roues max %.3f rad, cap du monde tourné de %.3f rad, décélération max %.2f g, frein %s, nitro %s → %s",
                          monde ? "APRÈS" : "AVANT", n / 120.0, volantMax, rouesMax, lacetMax, decelMax, frein ? "oui" : "non", nitro ? "oui" : "non",
                          bon ? "OK (tout droit, frottements seulement)" : monde ? "ÉCART" : "la route fait tourner la voiture");
            ligne(l);
        }
    }
    {   // 4. déterminisme : deux fois la même traversée (ligne idéale, épingle) → même trajectoire au bit près
        if (!vs.empty()) {
            const Mesure a = rouler(vs[0], Cas::Ideale, Aide::Activee, true), b = rouler(vs[0], Cas::Ideale, Aide::Activee, true);
            const bool bon = a.ecartMax == b.ecartMax && a.capMax == b.capMax && a.vMin == b.vMin;
            if (!bon) ecarts++;
            std::snprintf(l, sizeof l, "## 4. Déterminisme : deux traversées identiques de l'épingle : %s", bon ? "OK (au bit près)" : "ÉCART");
            ligne(""); ligne(l);
        }
    }
    {   // 5. pilote automatique (démonstrations, bancs, parcours) : un tour complet de chaque carte sans sortie de route
        ligne(""); ligne("## 5. Pilote automatique (aide activée, vitesse de virage tenue) : un tour complet de chaque carte");
        for (const char* id : {"circuit", "campagne", "ville", "plage"}) for (const char* car : {"golf", "sf90", "rrsport"}) {
            Track& t = piste(id);
            Player p; p.reset(carById(car)); p.aide = Aide::Activee;
            int sorties = 0, n = 0; double ecart = 0;
            for (; n < 120 * 600 && p.dist < t.length(); n++) {
                DriveInput in; in.gas = true;
                in.steer = (float)std::clamp((0.0 - p.playerX) * 3.0 - p.heading * 4.0, -1.0, 1.0);   // pilote de game.cpp
                limiterPilote(p, t, in);
                p.update(1.0 / 120, in, t, true);
                if (std::abs(p.playerX) > 1) sorties++;
                ecart = std::max(ecart, std::abs(p.playerX));
            }
            const bool bon = sorties == 0 && p.dist >= t.length();
            if (!bon) ecarts++;
            std::snprintf(l, sizeof l, "- %s, %s : tour %s en %.0f s, %d image(s) hors de la route, écart max %.2f demi-largeur → %s", id, car,
                          p.dist >= t.length() ? "fini" : "NON fini", n / 120.0, sorties, ecart, bon ? "OK" : "ÉCART");
            ligne(l);
        }
    }
    ligne(""); ecarts += mesurerAidesRapport(rapport);   // WFS-253
    std::snprintf(l, sizeof l, "\nbanc de conduite (WFS-241, WFS-253) : %d écart(s)", ecarts); ligne(l);
    return ecarts;
}

} // namespace wfs::banc_conduite

namespace wfs {
class Game; class Renderer;
// banc dans la partie (conduite_banc.cpp) : conduite.txt + suites de captures ; rend le nombre d'écarts
int benchConduite(Game& g, Renderer& r, const std::string& out);
} // namespace wfs
