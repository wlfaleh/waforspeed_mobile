// WFS-158 (ST-1, audit) : les 20 voitures mesurées comme une fiche constructeur (src/render/vehicule_mesure.h) contre le
// référentiel du réel (references/reel/voitures.json, WFS-148) : longueur, largeur sans rétroviseurs, hauteur hors antenne,
// empattement, voies, diamètres des roues (M-01, M-02), pneus posés sur le sol (RG-02). Pour chaque voiture, le diagnostic
// dit ce que permet RG-01 (échelle globale, position des roues) et ce qui ne se corrige pas sans changer le modèle (RG-05).
// Rouge si un modèle ne se lit pas, si ses 4 roues ne sont pas trouvées ou si une mesure manque au référentiel. Les écarts
// au réel sont l'état actuel, écrits et comptés : ils n'échouent pas encore ; ST-5 rendra le test strict (critère « vert
// pour les 20 voitures » de WFS-158).
// usage : test_fidelite_voitures [dossier du dépôt] [fichier de sortie du rapport]
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include "../src/render/bench_realisme.h"
#include "../src/render/vehicule_mesure.h"
using namespace wfs;

#ifndef WFS_SOURCE_DIR
#define WFS_SOURCE_DIR "."
#endif

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

static const std::vector<std::string> VOITURES = {"golf", "m4", "p911", "gallardo", "p208", "abarth", "tcross", "compass", "s3", "x5", "giulia", "a45",
                                                  "model3", "corsa", "gt86", "elantra", "v60", "rrsport", "mustang", "sf90"};   // CARS (src/game/cars.h)

// valeur du jeu (m) contre la mesure du référentiel (mm), bornes tolérance comprise
static vehicule::Ecart ecart(const reel::Referentiel& R, const std::string& id, double jeu) {
    vehicule::Ecart e; e.jeu = jeu;
    const reel::Json* m = R.mesure(id);
    VERIFIE(m != nullptr, "%s absente du référentiel", id.c_str());
    double bas, haut; std::string reel, tol;
    if (m && reel::bornes(*m, bas, haut, reel, tol)) {
        e.bas = bas / 1000; e.haut = haut / 1000; e.connu = true;
        const reel::Json* v = m->get("valeur");
        e.nominal = v && v->t == reel::Json::Num ? v->n / 1000 : 0.5 * (m->nombre("min", 0) + m->nombre("max", 0)) / 1000;
    }
    return e;
}
static std::string pct(const vehicule::Ecart& e) {
    if (!e.connu) return "  à sourcer";
    const double r = vehicule::ecartRelatif(e);
    char b[32]; std::snprintf(b, sizeof b, r == 0 ? "   conforme" : "%+10.1f %%", r * 100);
    return b;
}

int main(int argc, char** argv) {
    const std::string racine = argc > 1 ? argv[1] : WFS_SOURCE_DIR;
    reel::Referentiel R;
    VERIFIE(R.charger(racine + "/references/reel"), "référentiel illisible : %s", R.erreur.c_str());
    std::string rapport;
    auto ecrire = [&](const char* fmt, auto... a) {
        char b[512];
        if constexpr (sizeof...(a) == 0) std::snprintf(b, sizeof b, "%s", fmt); else std::snprintf(b, sizeof b, fmt, a...);
        std::printf("%s", b); rapport += b;
    };
    ecrire("WaForSpeed — audit des 20 voitures (WFS-158, ST-1) : mesures « fiche constructeur » (vehicule_mesure.h)\n");
    ecrire("écarts au réel, tolérance ± 1 % comprise (conforme = dans la tolérance) ; « antenne » : hauteur brute − hauteur fiche\n\n");
    ecrire("%-9s %11s %11s %11s %11s %11s %11s %11s %11s | %8s %8s | %s\n", "voiture", "longueur", "largeur", "hauteur", "empatt.", "voie av", "voie ar",
           "roue av", "roue ar", "antenne", "pneus", "diagnostic (RG-01 / RG-05)");
    int conformesBoite = 0, conformes = 0, connues = 0, posees = 0, aRemplacer = 0;
    for (const std::string& c : VOITURES) {
        vehicule::ModeleWfm m; std::string err;
        if (!vehicule::lireWfm(racine + "/assets/cars/" + c + ".wfm", m, err)) { VERIFIE(false, "%s.wfm : %s", c.c_str(), err.c_str()); continue; }
        const vehicule::Dimensions d = vehicule::mesurer(m);
        VERIFIE(d.roues, "%s : 4 roues non trouvées (roues::decouper)", c.c_str());
        const std::string p = "voiture." + c + ".";
        const vehicule::Ecart e[8] = {ecart(R, p + "longueur", d.longueur), ecart(R, p + "largeur", d.largeur), ecart(R, p + "hauteur", d.hauteur),
                                      ecart(R, p + "empattement", d.empattement), ecart(R, p + "voie_av", d.voieAv), ecart(R, p + "voie_ar", d.voieAr),
                                      ecart(R, p + "roue_av", d.diametreAv), ecart(R, p + "roue_ar", d.diametreAr)};
        // état avant : mesure de WFS-148 (reel::mesurerWfm : boîte, largeur sous 55 % de la hauteur de la boîte), pour l'indice
        const reel::MesureVoiture w = reel::mesurerWfm(racine + "/assets/cars/" + c + ".wfm");
        const vehicule::Ecart eb[3] = {ecart(R, p + "longueur", w.longueur), ecart(R, p + "largeur", w.largeurSansRetro), ecart(R, p + "hauteur", w.hauteur)};
        for (const auto& x : eb) conformesBoite += x.connu && vehicule::ecartRelatif(x) == 0;
        for (int i = 3; i < 8; i++) conformesBoite += e[i].connu && vehicule::ecartRelatif(e[i]) == 0;
        for (const auto& x : e) { connues += x.connu; conformes += x.connu && vehicule::ecartRelatif(x) == 0; }
        const bool pose = d.ecartPneuMax <= vehicule::TOL_PNEU_M && std::fabs(d.ecartSolBoite) <= vehicule::TOL_PNEU_M;
        posees += pose;
        // diagnostic : meilleure échelle globale (carrosserie), puis roues à déplacer à cette échelle
        const vehicule::Echelle s = vehicule::meilleureEchelle(e, 3);
        std::string diag;
        char b[160];
        // RG-05 : au-delà de 1 % d'écart restant après la meilleure échelle, le modèle n'a pas les proportions de la voiture
        if (s.residu > 0.01) { std::snprintf(b, sizeof b, "proportions fausses (résidu %.1f %% après × %.3f) → RG-05", s.residu * 100, s.s); diag = b; aRemplacer++; }
        else if (s.residu > 0) { std::snprintf(b, sizeof b, "× %.3f, résidu %.1f %% (à affiner)", s.s, s.residu * 100); diag = b; }
        else if (std::fabs(s.s - 1) > 1e-9) { std::snprintf(b, sizeof b, "échelle globale × %.3f", s.s); diag = b; }
        else diag = "carrosserie conforme";
        const char* noms[5] = {"empattement", "voie av", "voie ar", "roue av", "roue ar"};
        for (int i = 3; i < 8; i++) {
            vehicule::Ecart x = e[i]; x.jeu *= s.s;
            const double r = vehicule::ecartRelatif(x);
            if (x.connu && r != 0) { std::snprintf(b, sizeof b, " ; %s %+.0f mm", noms[i - 3], (r < 0 ? x.bas - x.jeu : x.haut - x.jeu) * -1000); diag += b; }
        }
        if (!pose) { std::snprintf(b, sizeof b, " ; pneus : écart %.0f mm, sous les pneus %.0f mm", d.ecartPneuMax * 1000, d.ecartSolBoite * 1000); diag += b; }
        char antenne[16]; std::snprintf(antenne, sizeof antenne, "%+.0f mm", (d.hauteurBrute - d.hauteur) * 1000);
        ecrire("%-9s %s %s %s %s %s %s %s %s | %8s %8s | %s\n", c.c_str(), pct(e[0]).c_str(), pct(e[1]).c_str(), pct(e[2]).c_str(), pct(e[3]).c_str(),
               pct(e[4]).c_str(), pct(e[5]).c_str(), pct(e[6]).c_str(), pct(e[7]).c_str(), antenne, pose ? "posés" : "NON", diag.c_str());
        ecrire("%-9s   mesures : L %.3f · l %.3f (WFS-148 %.3f, boîte %.3f) · h %.3f (boîte %.3f) · empatt. %.3f · voies %.3f / %.3f · roues %.3f / %.3f · garde au sol %.3f m\n",
               "", d.longueur, d.largeur, w.largeurSansRetro, d.largeurBoite, d.hauteur, d.hauteurBoite, d.empattement, d.voieAv, d.voieAr,
               d.diametreAv, d.diametreAr, d.gardeAuSol);
    }
    const int connuesBoite = connues;   // mêmes grandeurs connues avant et après
    ecrire("\nindice voitures : avant (boîte, WFS-148) %d / %d = %.1f %% ; après (mesure fiche) %d / %d = %.1f %% ; pneus posés : %d / 20 ; "
           "modèles aux proportions fausses (RG-05) : %d\n",
           conformesBoite, connuesBoite, connuesBoite ? 100.0 * conformesBoite / connuesBoite : 0, conformes, connues, connues ? 100.0 * conformes / connues : 0,
           posees, aRemplacer);
    if (argc > 2) { if (std::FILE* f = std::fopen(argv[2], "w")) { std::fwrite(rapport.data(), 1, rapport.size(), f); std::fclose(f); } }
    // la mesure elle-même : une antenne fine ne compte pas dans la hauteur, un toit oui (modèle synthétique)
    {
        vehicule::ModeleWfm m;
        m.boite[0] = -0.9f; m.boite[1] = 0; m.boite[2] = -2; m.boite[3] = 0.9f; m.boite[4] = 2.0f; m.boite[5] = 2;
        auto quad = [&](std::vector<float> pts) {   // 4 coins x y z → 2 triangles (sommets x y z nx ny nz u v)
            m.sommets.emplace_back(); m.indices.push_back({0, 1, 2, 0, 2, 3});
            for (size_t i = 0; i < 4; i++) { for (int k = 0; k < 3; k++) m.sommets.back().push_back(pts[i * 3 + k]); for (int k = 0; k < 5; k++) m.sommets.back().push_back(0); }
            m.drapeaux.push_back(0);
        };
        quad({-0.8f, 1.5f, -1.5f, 0.8f, 1.5f, -1.5f, 0.8f, 1.5f, 1.5f, -0.8f, 1.5f, 1.5f});        // toit à 1,5 m (4,8 m²)
        quad({-0.9f, 0.3f, -2, 0.9f, 0.3f, -2, 0.9f, 0.3f, 2, -0.9f, 0.3f, 2});                    // plancher (largeur 1,8 m)
        quad({0.9f, 0.2f, -2, 0.9f, 0.7f, -2, 0.9f, 0.7f, 2, 0.9f, 0.2f, 2});                      // flanc droit
        quad({-0.9f, 0.2f, -2, -0.9f, 0.7f, -2, -0.9f, 0.7f, 2, -0.9f, 0.2f, 2});                  // flanc gauche
        quad({0.0f, 1.5f, 1.0f, 0.004f, 1.5f, 1.0f, 0.004f, 2.0f, 1.004f, 0.0f, 2.0f, 1.004f});    // antenne de 50 cm
        quad({1.0f, 1.0f, -0.5f, 1.0f, 1.2f, -0.5f, 1.0f, 1.2f, -0.3f, 1.0f, 1.0f, -0.3f});        // rétroviseur au-dessus de 55 %
        const vehicule::Dimensions d = vehicule::mesurer(m);
        std::printf("modèle synthétique : hauteur %.3f (brute %.3f), largeur %.3f\n", d.hauteur, d.hauteurBrute, d.largeur);
        VERIFIE(std::fabs(d.hauteur - 1.5) < 1e-6 && std::fabs(d.hauteurBrute - 2.0) < 1e-6, "mesure : l'antenne compte dans la hauteur");
        VERIFIE(std::fabs(d.largeur - 1.8) < 1e-6, "mesure : le rétroviseur compte dans la largeur (%.3f)", d.largeur);
    }
    {   // meilleure échelle : 2 % trop grand partout → × 0,98 ; proportions fausses → résidu
        const vehicule::Ecart ok[3] = {{4.08, 3.96, 4.04, 4.0, true}, {1.836, 1.782, 1.818, 1.8, true}, {1.479, 1.4355, 1.4645, 1.45, true}};
        const vehicule::Echelle s = vehicule::meilleureEchelle(ok, 3);
        VERIFIE(s.residu == 0 && std::fabs(s.s - 0.98) < 0.0006, "échelle : × %.4f, résidu %.3f (attendu × 0,98, 0)", s.s, s.residu);
        const vehicule::Ecart faux[3] = {{4.0, 3.96, 4.04, 4.0, true}, {1.8, 1.782, 1.818, 1.8, true}, {1.8, 1.4355, 1.4645, 1.45, true}};
        VERIFIE(vehicule::meilleureEchelle(faux, 3).residu > 0.1, "échelle : une hauteur fausse de 23 %% doit rester hors tolérance");
    }
    {   // lecture bornée : fichier absent ou d'un autre format → erreur, pas de plantage
        vehicule::ModeleWfm m; std::string e;
        VERIFIE(!vehicule::lireWfm(racine + "/assets/cars/absente.wfm", m, e) && !e.empty(), "fichier absent accepté");
        VERIFIE(!vehicule::lireWfm(racine + "/CMakeLists.txt", m, e), "fichier d'un autre format accepté");
    }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("test_fidelite_voitures : OK (audit ST-1 : écarts au réel = état actuel, écrits ; ST-5 rend le test strict)\n");
    return 0;
}
