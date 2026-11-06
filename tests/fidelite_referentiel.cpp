// WFS-148 : référentiel du réel (RG-01) et mesures géométriques de fidélité (RG-05, M-02, M-03).
//  1. Référentiel : chaque fichier de references/reel/ se lit ; chaque mesure a un identifiant unique, une source déclarée
//     (dans son fichier et dans SOURCES.md), un statut connu, une tolérance, et une valeur sauf « à sourcer ». Rouge sinon.
//  2. Calculs de couleur : ΔE2000 (src/render/bench_realisme.h) contre les valeurs de contrôle de colour-science, et
//     conversion L*a*b* D50 → sRGB linéaire contre les albédos dérivés de la charte. Rouge sinon.
//  3. Lecteur JSON borné : textes cassés refusés sans plantage. Rouge sinon.
//  4. Mesures du jeu (voitures chargées, lignes de la route) contre le référentiel : les écarts sont l'état zéro de l'axe
//     Réalisme, marqués « attendu » (RG-08) : ils sont écrits, ils n'échouent pas encore (chaque US de domaine les corrige).
// usage : test_fidelite [dossier du dépôt] [fichier de sortie du rapport]
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include "../src/render/bench_realisme.h"
using namespace wfs::reel;

#ifndef WFS_SOURCE_DIR
#define WFS_SOURCE_DIR "."
#endif

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

static const std::vector<std::string> VOITURES = {"golf", "m4", "p911", "gallardo", "p208", "abarth", "tcross", "compass", "s3", "x5", "giulia", "a45",
                                                  "model3", "corsa", "gt86", "elantra", "v60", "rrsport", "mustang", "sf90"};   // CARS (src/game/cars.h)

int main(int argc, char** argv) {
    const std::string racine = argc > 1 ? argv[1] : WFS_SOURCE_DIR;
    // ---- 1. référentiel
    Referentiel R;
    VERIFIE(R.charger(racine + "/references/reel"), "référentiel illisible : %s", R.erreur.c_str());
    std::string sourcesMd;
    VERIFIE(lireTexte(racine + "/references/reel/SOURCES.md", sourcesMd), "references/reel/SOURCES.md illisible");
    const std::vector<std::string> defauts = controlerReferentiel(R, sourcesMd);
    for (const std::string& d : defauts) VERIFIE(false, "référentiel : %s", d.c_str());
    int n = 0, nSourcer = 0, nConfirmer = 0;
    for (const auto& [f, j] : R.fichiers) if (const Json* ms = j.get("mesures")) for (const Json& m : ms->a) {
        n++; nSourcer += m.texte("statut") == "a_sourcer"; nConfirmer += m.texte("statut") == "a_confirmer";
    }
    std::printf("référentiel : %zu fichiers, %d valeurs (%d « à confirmer », %d « à sourcer »), %zu défaut(s)\n", R.fichiers.size(), n, nConfirmer, nSourcer, defauts.size());
    for (const std::string& c : VOITURES) VERIFIE(R.mesure("voiture." + c + ".longueur"), "voiture %s absente de voitures.json", c.c_str());

    // ---- 2. couleurs
    const auto& cc = R.fichiers["colorchecker.json"];
    if (const Json* ctl = cc.get("controle_deltaE2000")) for (const Json& p : ctl->a) {
        const Json *a = p.get("a"), *b = p.get("b");
        if (!a || !b || a->a.size() != 3 || b->a.size() != 3) { VERIFIE(false, "paire de contrôle ΔE2000 mal formée"); continue; }
        const double d = deltaE2000({a->a[0].n, a->a[1].n, a->a[2].n}, {b->a[0].n, b->a[1].n, b->a[2].n}), ref = p.nombre("deltaE2000", -1);
        VERIFIE(std::fabs(d - ref) < 1e-3, "ΔE2000 %.4f au lieu de %.4f (colour-science)", d, ref);
    } else VERIFIE(false, "colorchecker.json sans valeurs de contrôle ΔE2000");
    VERIFIE(deltaE2000({50, 2.5, 0}, {50, 2.5, 0}) == 0, "ΔE2000 d'une couleur avec elle-même");
    int ncc = 0;
    for (int k = 1; k <= 24; k++) {
        char id[32]; std::snprintf(id, sizeof id, "cc.%02d", k);
        const Json *lab = R.mesure(id), *alb = R.mesure(std::string(id) + ".albedo");
        if (!lab || !alb || !lab->get("valeur") || !alb->get("valeur")) { VERIFIE(false, "%s : case absente", id); continue; }
        const auto& L = lab->get("valeur")->a; const auto& A = alb->get("valeur")->a;
        const double rgb[3] = {A[0].n, A[1].n, A[2].n};
        const double d = deltaE2000(lineaireVersLab(rgb), {L[0].n, L[1].n, L[2].n});
        VERIFIE(d < 0.05, "%s : albédo dérivé → L*a*b* à %.3f ΔE de la charte (conversion C++ ≠ colour-science)", id, d);
        ncc++;
    }
    std::printf("couleurs : ΔE2000 et conversions vérifiés (%d cases)\n", ncc);

    // ---- 3. lecteur JSON borné
    for (const char* t : {"", "{", "{\"a\":}", "[1,2", "{\"a\" 1}", "\"\\u12\"", "1e", "[1] x", "{\"a\":nul}"}) {
        Json j; std::string e; LecteurJson l(t);
        VERIFIE(!l.lire(j, e), "JSON cassé accepté : %s", t);
    }
    { std::string prof(100, '['); prof += std::string(100, ']'); Json j; std::string e; LecteurJson l(prof); VERIFIE(!l.lire(j, e), "JSON trop imbriqué accepté"); }
    { Json j; std::string e; LecteurJson l("{\"é\":[1.5,-2e3,\"\\u00e9\\n\",true,null]}"); VERIFIE(l.lire(j, e) && j.get("é") && j.get("é")->a.size() == 5 && j.get("é")->a[1].n == -2000 && j.get("é")->a[2].s == "\xc3\xa9\n", "JSON valide mal lu"); }

    // ---- 4. mesures du jeu (état zéro, écarts attendus)
    Rapport rap;
    mesuresDonnees(R, rap, racine, VOITURES);
    int nonMes = 0;
    for (const auto& l : rap.lignes) {
        if (l.domaine == "soleil") VERIFIE(l.etat == Etat::Conforme, "soleil : %s %s (réel %s)", l.id.c_str(), l.jeu.c_str(), l.reel.c_str());
        if (l.domaine == "voitures" && l.id.find(".longueur") != std::string::npos) VERIFIE(l.etat != Etat::NonMesurable, "%s non mesurable : %s", l.id.c_str(), l.raison.c_str());
        nonMes += l.etat == Etat::NonMesurable;
    }
    VERIFIE(rap.lignes.size() > 150, "trop peu de mesures (%zu)", rap.lignes.size());
    for (const auto& [d, i] : rap.indices())
        std::printf("état zéro (attendu) — %s : %s (%d / %d conformes, %d non mesurables)\n", d.c_str(), i.enTexte().c_str(), i.conformes, i.conformes + i.nonConformes, i.nonMesurables);
    {   // WFS-149 (RG-B, ANO-86) : domaine sans mesure possible → « non mesurable », jamais « 0.0 % »
        Rapport r; r.nonMesurable("anthropometrie", "humain.*", "tailles", "modèles à venir");
        const std::string t = r.texte("essai");
        VERIFIE(t.find("anthropometrie : non mesurable") != std::string::npos && t.find("0.0 %") == std::string::npos,
                "domaine sans mesure : « non mesurable » attendu dans le rapport");
        VERIFIE(t.find("TOTAL : non mesurable") != std::string::npos, "total sans mesure : « non mesurable » attendu");
    }
    if (argc > 2) {
        std::FILE* f = std::fopen(argv[2], "w");
        if (f) { const std::string t = rap.texte("WaForSpeed — mesures géométriques de fidélité (test_fidelite, WFS-148)"); std::fwrite(t.data(), 1, t.size(), f); std::fclose(f); }
    }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("test_fidelite : OK (%zu mesures, %d non mesurables ; écarts du jeu = état zéro, attendus)\n", rap.lignes.size(), nonMes);
    return 0;
}
