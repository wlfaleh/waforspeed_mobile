// WaForSpeed — WFS-158 (ST-2, RG-01, RG-02) : corrections d'échelle et de roues des voitures, d'après le référentiel du réel
// (references/reel/voitures.json, WFS-148). Dernière étape de la conversion (tools/convertir_voitures.sh), sur le .wfm lui-même :
//   1. échelle globale (jamais par axe, RG-01) : celle qui met le plus de mesures de la caisse (longueur, largeur, hauteur,
//      empattement) dans leur tolérance, la plus proche des valeurs nominales ; gardée seulement si elle en gagne au moins une ;
//   2. diamètre des roues : chaque roue hors tolérance est mise au diamètre de la fiche (échelle de la roue autour de son
//      centre, avec ses disques et ses étriers) ;
//   3. voies : chaque essieu hors tolérance est remis à la voie de la fiche si chaque roue bouge de 15 mm au plus (au-delà,
//      la roue sortirait de son passage de roue : écart laissé au rapport, RG-05) ; l'empattement ne se corrige pas en
//      déplaçant les roues (elles quitteraient leurs passages de roue) : seule l'échelle globale le change ;
//   4. assise (RG-02) : les 4 pneus posés au même niveau ; la caisse descend ou monte, entre « centre des roues fixe » et
//      « bas des pneus fixe », vers la hauteur nominale de la fiche ; puis la voiture est reposée au sol (bas des pneus à y = 0).
// Les roues sont celles du moteur (roues::decouper, WFS-60) : ce que le jeu fait tourner est exactement ce qui est corrigé.
// Les tangentes (section TAN1 des .wfm version 2) ne changent pas : échelles uniformes et translations gardent leurs directions.
// Rejouable : une voiture déjà corrigée n'est plus modifiée (aucune mesure à gagner).
// usage : corriger_voitures [dossier du dépôt] [id …]   (sans id : les 20 voitures) ; --essai : mesure et rapport sans écrire ;
// WFS_DETAIL=1 : les 8 mesures de chaque voiture avant / après
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "../src/render/bench_realisme.h"
#include "../src/render/vehicule_mesure.h"
using namespace wfs;

#ifndef WFS_SOURCE_DIR
#define WFS_SOURCE_DIR "."
#endif

static const std::vector<std::string> VOITURES = {"golf", "m4", "p911", "gallardo", "p208", "abarth", "tcross", "compass", "s3", "x5", "giulia", "a45",
                                                  "model3", "corsa", "gt86", "elantra", "v60", "rrsport", "mustang", "sf90"};   // CARS (src/game/cars.h)
constexpr double VOIE_MAX_PAR_ROUE = 0.015;   // m : déplacement latéral le plus grand d'une roue dans son passage de roue

// fichier .wfm brut et position des sommets de chaque maillage (pour réécrire les positions sans toucher au reste)
struct Brut {
    std::string octets;
    size_t boite = 0;
    std::vector<size_t> sommets;   // décalage des sommets de chaque maillage
    std::vector<uint32_t> nSommets;
};
static bool lireBrut(const std::string& chemin, Brut& b) {
    std::ifstream f(chemin, std::ios::binary);
    if (!f) return false;
    b.octets.assign(std::istreambuf_iterator<char>(f), {});
    const std::string& s = b.octets;
    auto u32 = [&](size_t o) { uint32_t x; std::memcpy(&x, &s[o], 4); return x; };
    if (s.size() < 44) return false;
    const uint32_t nT = u32(8), nM = u32(12), nMe = u32(16);
    b.boite = 20; size_t o = 44;
    for (uint32_t t = 0; t < nT; t++) o += 4 + u32(o);
    o += (size_t)nM * 56;
    for (uint32_t k = 0; k < nMe; k++) {
        const uint32_t nv = u32(o + 4), ni = u32(o + 8); o += 12;
        b.sommets.push_back(o); b.nSommets.push_back(nv);
        o += (size_t)nv * 32 + (size_t)ni * 4;
    }
    return o <= s.size();
}

static vehicule::Ecart ecart(const reel::Referentiel& R, const std::string& id, double jeu) {
    vehicule::Ecart e; e.jeu = jeu;
    const reel::Json* m = R.mesure(id);
    double bas, haut; std::string reel, tol;
    if (m && reel::bornes(*m, bas, haut, reel, tol)) {
        e.bas = bas / 1000; e.haut = haut / 1000; e.connu = true;
        const reel::Json* v = m->get("valeur");
        e.nominal = v && v->t == reel::Json::Num ? v->n / 1000 : 0.5 * (m->nombre("min", 0) + m->nombre("max", 0)) / 1000;
    }
    return e;
}
static int conformes(const vehicule::Ecart* e, int n, double s = 1) {
    int c = 0;
    for (int i = 0; i < n; i++) { vehicule::Ecart x = e[i]; x.jeu *= s; c += x.connu && vehicule::ecartRelatif(x) == 0; }
    return c;
}
static void ecarts(const reel::Referentiel& R, const std::string& c, const vehicule::Dimensions& d, vehicule::Ecart e[8]) {
    const std::string p = "voiture." + c + ".";
    e[0] = ecart(R, p + "longueur", d.longueur); e[1] = ecart(R, p + "largeur", d.largeur); e[2] = ecart(R, p + "hauteur", d.hauteur);
    e[3] = ecart(R, p + "empattement", d.empattement); e[4] = ecart(R, p + "voie_av", d.voieAv); e[5] = ecart(R, p + "voie_ar", d.voieAr);
    e[6] = ecart(R, p + "roue_av", d.diametreAv); e[7] = ecart(R, p + "roue_ar", d.diametreAr);
}

int main(int argc, char** argv) {
    std::string racine = WFS_SOURCE_DIR;
    std::vector<std::string> ids;
    bool essai = false;
    for (int i = 1; i < argc; i++) {
        const std::string a = argv[i];
        if (a == "--essai") essai = true;
        else if (a.find('/') != std::string::npos || a == ".") racine = a;
        else ids.push_back(a);
    }
    if (ids.empty()) ids = VOITURES;
    reel::Referentiel R;
    if (!R.charger(racine + "/references/reel")) { std::fprintf(stderr, "référentiel illisible : %s\n", R.erreur.c_str()); return 1; }
    int erreurs = 0, gagnees = 0, connuesTotal = 0, avantTotal = 0;
    for (const std::string& c : ids) {
        const std::string chemin = racine + "/assets/cars/" + c + ".wfm";
        Brut brut; vehicule::ModeleWfm m; std::string err;
        if (!lireBrut(chemin, brut) || !vehicule::lireWfm(chemin, m, err)) { std::fprintf(stderr, "%s : illisible (%s)\n", c.c_str(), err.c_str()); erreurs++; continue; }
        const vehicule::Dimensions d0 = vehicule::mesurer(m);
        if (!d0.roues) { std::fprintf(stderr, "%s : 4 roues non trouvées, voiture laissée telle quelle\n", c.c_str()); erreurs++; continue; }
        vehicule::Ecart e0[8]; ecarts(R, c, d0, e0);
        const int avant = conformes(e0, 8);
        int connues = 0; for (const auto& x : e0) connues += x.connu;

        // 1. échelle globale (caisse : longueur, largeur, hauteur, empattement)
        vehicule::Echelle best = vehicule::meilleureEchelle(e0, 4);
        double s = conformes(e0, 4, best.s) > conformes(e0, 4) ? best.s : 1.0;
        for (auto& v : m.sommets) for (size_t i = 0; i < v.size(); i += 8) { v[i] *= (float)s; v[i + 1] *= (float)s; v[i + 2] *= (float)s; }

        // roues du moteur, à cette échelle
        std::vector<roues::Maillage> ms;
        for (size_t k = 0; k < m.sommets.size(); k++)
            ms.push_back({m.sommets[k].data(), 8, (uint32_t)(m.sommets[k].size() / 8), m.indices[k].data(), (uint32_t)m.indices[k].size(), (m.drapeaux[k] & 128) != 0});
        const roues::Decoupe dec = roues::decouper(ms);
        const auto& w = dec.roues;
        const vehicule::Dimensions d1s = vehicule::mesurer(m);   // à cette échelle, avant les roues
        vehicule::Ecart e1[8]; ecarts(R, c, d1s, e1);

        // pièces de chaque roue : triangles du moteur, puis (autres maillages, sauf la peinture) triangles dans le cylindre de
        // la roue, comme roues::decouper : disques, étriers et moyeux suivent la roue (un étrier laissé en place sortirait
        // d'une jante déplacée)
        std::vector<std::vector<int8_t>> coin(m.sommets.size());
        float hx[4] = {0, 0, 0, 0};   // demi-largeur de chaque roue + 1 cm, comme le moteur
        for (size_t mi = 0; mi < m.sommets.size(); mi++)
            for (size_t t = 0; t < m.indices[mi].size() / 3; t++) if (const int k = dec.roueDuTriangle[mi][t]; k >= 0)
                for (int j = 0; j < 3; j++) hx[k] = std::max(hx[k], std::fabs(m.sommets[mi][8 * m.indices[mi][t * 3 + j]] - w[k].centre.x) + 0.01f);
        int partages = 0;
        for (size_t mi = 0; mi < m.sommets.size(); mi++) {
            const auto& I = m.indices[mi]; const float* v = m.sommets[mi].data();
            coin[mi].assign(m.sommets[mi].size() / 8, -1);
            std::vector<int8_t> duTri(I.size() / 3, -1);
            for (size_t t = 0; t < I.size() / 3; t++) {
                int k = dec.roueDuTriangle[mi][t];
                if (k < 0 && !(m.drapeaux[mi] & 1)) for (int q = 0; q < 4 && k < 0; q++) {
                    bool dedans = true;
                    for (int j = 0; j < 3 && dedans; j++) {
                        const float* p = v + 8 * I[t * 3 + j];
                        dedans = std::fabs(p[0] - w[q].centre.x) <= hx[q] && std::hypot(p[1] - w[q].centre.y, p[2] - w[q].centre.z) <= w[q].rayon * 0.97f;
                    }
                    if (dedans) k = q;
                }
                duTri[t] = (int8_t)k;
                if (k >= 0) for (int j = 0; j < 3; j++) coin[mi][I[t * 3 + j]] = (int8_t)k;
            }
            for (size_t t = 0; t < I.size() / 3; t++) {   // triangle fixe qui partage un sommet avec une roue : il sera étiré
                int nr = 0; for (int j = 0; j < 3; j++) nr += coin[mi][I[t * 3 + j]] >= 0;
                partages += duTri[t] < 0 && nr > 0;
            }
        }

        // une correction : échelle des roues et voie ajoutée, par essieu (0 avant, 1 arrière) ; assise (RG-02) : les 4 pneus au
        // même niveau B, choisi entre les bas actuels et les bas après l'échelle des roues autour de leur centre, vers la hauteur
        // nominale ; puis toute la voiture descend de B (bas des pneus à y = 0)
        struct Essai { double S[2] = {1, 1}, dv[2] = {0, 0}, B = 0; vehicule::ModeleWfm m; vehicule::Dimensions d; vehicule::Ecart e[8]; int ok = 0; bool casse = false; int n = 0; };
        const double haut = d1s.hauteur + d1s.sol;   // y du haut de la caisse
        auto essayer = [&](const double S[2], const double dv[2]) {
            Essai x; x.m = m;
            for (int a = 0; a < 2; a++) { x.S[a] = S[a]; x.dv[a] = dv[a]; x.n += (S[a] != 1) + (dv[a] != 0); }
            double lo = 1e9, hi = -1e9;
            for (int k = 0; k < 4; k++) {
                const int a = w[k].avant ? 0 : 1;
                const double b0 = w[k].centre.y - w[k].rayon, bS = w[k].centre.y - w[k].rayon * S[a];
                lo = std::min({lo, b0, bS}); hi = std::max({hi, b0, bS});
            }
            x.B = std::clamp(e1[2].connu ? haut - e1[2].nominal : lo, lo, hi);
            for (size_t mi = 0; mi < x.m.sommets.size(); mi++) {
                float* v = x.m.sommets[mi].data();
                for (size_t i = 0; i < coin[mi].size(); i++) {
                    float* p = v + 8 * i;
                    if (const int k = coin[mi][i]; k >= 0) {
                        const int a = w[k].avant ? 0 : 1;
                        const double sg = w[k].centre.x > 0 ? 1 : -1, dy = x.B - (w[k].centre.y - w[k].rayon * S[a]);
                        const double cq[3] = {w[k].centre.x, w[k].centre.y, w[k].centre.z}, dq[3] = {sg * dv[a] / 2, dy, 0};
                        for (int q = 0; q < 3; q++) p[q] = (float)(cq[q] + (p[q] - cq[q]) * S[a] + dq[q]);
                    }
                    p[1] -= (float)x.B;
                }
            }
            float* bb = x.m.boite;
            for (int q = 0; q < 3; q++) { bb[q] = 1e9f; bb[q + 3] = -1e9f; }
            for (size_t mi = 0; mi < x.m.sommets.size(); mi++) for (uint32_t i : x.m.indices[mi]) for (int q = 0; q < 3; q++) {
                const float y = x.m.sommets[mi][8 * i + q]; bb[q] = std::min(bb[q], y); bb[q + 3] = std::max(bb[q + 3], y);
            }
            x.d = vehicule::mesurer(x.m); ecarts(R, c, x.d, x.e); x.ok = conformes(x.e, 8);
            for (int i = 0; i < 8; i++) x.casse = x.casse || (e1[i].connu && vehicule::ecartRelatif(e1[i]) == 0 && vehicule::ecartRelatif(x.e[i]) != 0);
            return x;
        };
        // corrections possibles (2. diamètres, 3. voies) ; on garde la combinaison qui gagne le plus de mesures sans en perdre
        // aucune (une roue poussée hors de la caisse élargit la voiture : refusée), à égalité celle qui bouge le moins
        double Sc[2] = {1, 1}, dvc[2] = {0, 0};
        std::string refus;
        char b[200];
        for (int a = 0; a < 2; a++) {
            if (e1[6 + a].connu && vehicule::ecartRelatif(e1[6 + a]) != 0) Sc[a] = e1[6 + a].nominal / e1[6 + a].jeu;
            if (e1[4 + a].connu && vehicule::ecartRelatif(e1[4 + a]) != 0) {
                const double delta = e1[4 + a].nominal - e1[4 + a].jeu;
                if (std::fabs(delta) / 2 <= VOIE_MAX_PAR_ROUE) dvc[a] = delta;
                else { std::snprintf(b, sizeof b, " ; voie %s %+.0f mm non corrigée (%.0f mm par roue, RG-05)", a ? "ar" : "av", delta * 1000, std::fabs(delta) * 500); refus += b; }
            }
        }
        Essai meilleur; meilleur.ok = -1;
        for (int masque = 0; masque < 16; masque++) {
            const double S[2] = {masque & 1 ? Sc[0] : 1, masque & 2 ? Sc[1] : 1}, dv[2] = {masque & 4 ? dvc[0] : 0, masque & 8 ? dvc[1] : 0};
            if ((masque & 1 && Sc[0] == 1) || (masque & 2 && Sc[1] == 1) || (masque & 4 && dvc[0] == 0) || (masque & 8 && dvc[1] == 0)) continue;
            Essai x = essayer(S, dv);
            if (x.casse) continue;
            if (x.ok > meilleur.ok || (x.ok == meilleur.ok && x.n < meilleur.n)) meilleur = std::move(x);
        }
        std::string notes;
        for (int a = 0; a < 2; a++) {
            if (meilleur.S[a] != 1) { std::snprintf(b, sizeof b, " ; roues %s %.3f → %.3f m", a ? "ar" : "av", e1[6 + a].jeu, e1[6 + a].nominal); notes += b; }
            else if (Sc[a] != 1) { std::snprintf(b, sizeof b, " ; roues %s %.3f m non corrigées (perdraient une mesure)", a ? "ar" : "av", e1[6 + a].jeu); refus += b; }
            if (meilleur.dv[a] != 0) { std::snprintf(b, sizeof b, " ; voie %s %+.0f mm", a ? "ar" : "av", meilleur.dv[a] * 1000); notes += b; }
            else if (dvc[a] != 0) { std::snprintf(b, sizeof b, " ; voie %s %+.0f mm non corrigée (la roue sortirait de la caisse)", a ? "ar" : "av", dvc[a] * 1000); refus += b; }
        }
        const bool rien = std::fabs(s - 1) < 1e-9 && meilleur.n == 0 && d1s.ecartPneuMax <= vehicule::TOL_PNEU_M;
        const vehicule::Dimensions& d1 = rien ? d0 : meilleur.d;
        const int apres = rien ? avant : meilleur.ok;
        const double B = meilleur.B;
        if (!rien) m = std::move(meilleur.m);
        float* bb = m.boite;
        if (std::getenv("WFS_DETAIL")) for (int i = 0; i < 8; i++)
            std::printf("    %d : %.4f → %.4f [%.4f ; %.4f] %s → %s\n", i, e0[i].jeu, rien ? e0[i].jeu : meilleur.e[i].jeu, e0[i].bas, e0[i].haut,
                        e0[i].connu && vehicule::ecartRelatif(e0[i]) == 0 ? "ok" : "non", (rien ? e0[i] : meilleur.e[i]).connu && vehicule::ecartRelatif(rien ? e0[i] : meilleur.e[i]) == 0 ? "ok" : "non");
        notes += refus;
        connuesTotal += connues; avantTotal += avant; gagnees += apres - avant;
        std::printf("%-9s %d → %d / %d conformes ; échelle × %.4f%s ; pneus %s ; caisse %+.0f mm ; hauteur %.3f → %.3f m ; garde au sol %.3f → %.3f m%s%s\n",
                    c.c_str(), avant, apres, connues, s, notes.c_str(), d1.ecartPneuMax <= vehicule::TOL_PNEU_M ? "posés" : "NON posés",
                    rien ? 0.0 : (d1s.sol - B) * 1000, d0.hauteur, d1.hauteur, d0.gardeAuSol, d1.gardeAuSol,
                    partages ? (" ; " + std::to_string(partages) + " triangles fixes touchent une roue").c_str() : "", rien ? " — inchangée" : "");
        if (apres < avant) { std::fprintf(stderr, "%s : la correction perd des mesures, fichier non écrit\n", c.c_str()); erreurs++; continue; }
        if (essai || rien) continue;
        // réécriture : positions des sommets et boîte, tout le reste (textures, matériaux, normales, uv, indices, TAN1) intact
        std::memcpy(&brut.octets[brut.boite], bb, 6 * sizeof(float));
        for (size_t mi = 0; mi < m.sommets.size(); mi++)
            for (uint32_t i = 0; i < brut.nSommets[mi]; i++) std::memcpy(&brut.octets[brut.sommets[mi] + (size_t)i * 32], &m.sommets[mi][8 * i], 12);
        std::ofstream f(chemin, std::ios::binary);
        f.write(brut.octets.data(), (std::streamsize)brut.octets.size());
        if (!f) { std::fprintf(stderr, "%s : écriture impossible\n", c.c_str()); erreurs++; }
    }
    std::printf("mesures conformes : %d → %d / %d%s\n", avantTotal, avantTotal + gagnees, connuesTotal, essai ? " (essai, rien d'écrit)" : "");
    return erreurs ? 1 : 0;
}
