// Banc de performance automatique du téléphone (WFS-420) : déroulé des essais (RG-02), statistiques et règle des 90 % (RG-03),
// fichier de résultats (RG-04).
// usage : test_banc_telephone
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>
#include "../src/render/banc_telephone.h"
using namespace wfs::banc_tel;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)
static bool contient(const std::string& t, const char* m) { return t.find(m) != std::string::npos; }

int main() {
    // RG-02 : nuit puis jour ; tout éteint, chaque interrupteur une fois, tout éteint (contrôle)
    const std::vector<Essai> d = deroule();
    VERIFIE(d.size() == 2 * (N_INTERRUPTEURS + 2), "28 essais (%zu)", d.size());
    for (int m = 0; m < 2; m++) {
        const size_t o = (size_t)m * (N_INTERRUPTEURS + 2);
        VERIFIE(d[o].nuit == (m == 0) && d[o].interrupteur == -1 && !d[o].controle, "moment %d : référence en premier", m);
        for (int i = 0; i < N_INTERRUPTEURS; i++) VERIFIE(d[o + 1 + i].interrupteur == i && d[o + 1 + i].nuit == (m == 0), "moment %d : interrupteur %d seul", m, i);
        VERIFIE(d[o + N_INTERRUPTEURS + 1].interrupteur == -1 && d[o + N_INTERRUPTEURS + 1].controle, "moment %d : contrôle en dernier", m);
    }
    VERIFIE(CHAUFFE_S == 30 && MESURE_S == 60, "30 s de chauffe, 60 s de mesure");
    VERIFIE(!actif, "banc inactif par défaut (RG-06)");
    // RG-03 : statistiques (rang au plus proche, comme la mesure de WFS-246)
    std::vector<float> ms;
    for (int i = 0; i < 100; i++) ms.push_back((float)(i + 1));   // 1 … 100 ms
    const Stats s = stats(ms);
    VERIFIE(s.images == 100 && std::fabs(s.secondes - 5.05) < 1e-6, "100 images, 5,05 s");
    VERIFIE(s.mediane == 51 && s.p95 == 95 && s.p99 == 99, "médiane 51, 95 %% 95, 99 %% 99 (%g %g %g)", s.mediane, s.p95, s.p99);
    VERIFIE(std::fabs(s.ips - 100 / 5.05f) < 1e-3f, "img/s = images / durée");
    std::vector<float> vide; VERIFIE(stats(vide).images == 0, "aucune image");
    // RG-03 : part du GPU expliquée par les passes
    Resultat r; r.gpuMesure = true; r.gpuTotal = 10; r.gpuOmbre = 1; r.gpuReflet = 1; r.gpuScene = 6; r.gpuPost = 1.5;
    VERIFIE(std::fabs(partExpliquee(r) - 0.95) < 1e-9, "95 %% expliqués");
    Resultat sans; VERIFIE(partExpliquee(sans) < 0, "sans horodatages : -1");
    // écart à la référence du même moment, remarques (90 %, chauffe du contrôle)
    std::vector<Resultat> res(3);
    res[0].essai = {true, -1, false}; res[0].nom = "Référence (tout éteint)"; res[0].images.mediane = 20; res[0].images.images = 10;
    res[1].essai = {true, 3, false}; res[1].nom = "Sans ombres"; res[1].images.mediane = 15; res[1].images.images = 10;
    res[1].gpuMesure = true; res[1].gpuTotal = 10; res[1].gpuScene = 8;   // 80 % : sous la règle
    res[2].essai = {true, -1, true}; res[2].nom = "Contrôle (tout éteint)"; res[2].images.mediane = 22; res[2].images.images = 10;
    VERIFIE(std::fabs(ecartReference(res, 1) + 25) < 1e-9 && std::fabs(ecartReference(res, 2) - 10) < 1e-9, "écarts −25 %% et +10 %%");
    const std::vector<std::string> rq = remarques(res);
    bool vu90 = false, vuChauffe = false, vuGpu = false;
    for (const std::string& l : rq) { vu90 |= contient(l, "80 %"); vuChauffe |= contient(l, "chauffé"); vuGpu |= contient(l, "Temps GPU : pas de mesure pour 2 essais"); }   // WFS-451 (RG-03) : un essai sur trois a un temps GPU : les deux autres sont nommés
    VERIFIE(vu90 && vuChauffe && vuGpu, "remarques : 90 %%, chauffe, essais sans temps GPU");
    VERIFIE(ligneTableau(res, 0).find("réf.") != std::string::npos && contient(ligneTableau(res, 1), "-25 %"), "ligne du tableau : %s / %s",
            ligneTableau(res, 0).c_str(), ligneTableau(res, 1).c_str());
    // RG-04 : nom et contenu du fichier
    VERIFIE(nomFichier(2026, 11, 11, 9, 5) == "diagnostic-2026-11-11-0905.txt", "nom du fichier");
    Entete e; e.date = "2026-11-11 21:00:00"; e.appareil = "PC"; e.interrompu = true; e.raison = "arrêté";
    const std::string t = texteFichier(e, res);
    VERIFIE(contient(t, "WFS-420") && contient(t, "BANC INTERROMPU") && contient(t, "nuit | Sans ombres") && contient(t, "MOINS DE 90 %"), "texte du fichier");
    VERIFIE(virgule(16.75, 1) == "16,8" || virgule(16.75, 1) == "16,7", "virgule");
    // ---------- WFS-451 ----------
    // RG-01 : mode forcé (Checkpoint = 0) puis rétabli (Poursuite = 2) ; un 2e « forcer » (essai suivant) ne perd pas le mode du joueur
    ModeForce mf;
    VERIFIE(mf.retablir(2) == 2, "sans banc : le mode ne change pas");
    VERIFIE(mf.forcer(2, 0) == 0 && mf.forcer(0, 0) == 0 && mf.force, "mode forcé à chaque essai");
    VERIFIE(mf.retablir(0) == 2 && !mf.force && mf.retablir(1) == 1, "mode du joueur rétabli, une seule fois");
    // RG-01 : mode écrit dans le fichier
    Entete em; em.mode = "Checkpoint"; em.modeJoueur = "Poursuite";
    VERIFIE(contient(texteFichier(em, res), "mode de course : Checkpoint (forcé par le banc) ; mode du joueur : Poursuite, rétabli à la fin"), "mode dans le fichier");
    em.modeJoueur = "Checkpoint";
    VERIFIE(contient(texteFichier(em, res), "mode de course : Checkpoint (forcé par le banc)\n"), "mode dans le fichier (joueur déjà en Checkpoint)");
    // RG-03 : un seul essai sans temps GPU : il est nommé, pas de « horodatages non disponibles »
    std::vector<Resultat> g3 = res;
    g3[0].gpuMesure = true; g3[0].gpuTotal = 10; g3[0].gpuScene = 10; g3[1].gpuScene = 10;   // 100 % expliqués
    g3[2].images.mediane = 20;   // contrôle égal à la référence
    {
        const std::vector<std::string> q = remarques(g3);
        bool nomme = false, indispo = false, chauffe = false;
        for (const std::string& l : q) { nomme |= contient(l, "1 essai (nuit, Contrôle (tout éteint))"); indispo |= contient(l, "non disponibles"); chauffe |= contient(l, "chauffé"); }
        VERIFIE(nomme && !indispo && !chauffe, "un essai sans temps GPU : nommé, sans « non disponibles » ni chauffe (%zu remarques)", q.size());
    }
    // RG-03 : aucun essai avec temps GPU : « horodatages non disponibles »
    for (Resultat& x : g3) x.gpuMesure = false;
    { bool indispo = false; for (const std::string& l : remarques(g3)) indispo |= contient(l, "non disponibles"); VERIFIE(indispo, "aucun temps GPU : horodatages non disponibles"); }
    // RG-03 : contrôle plus rapide (−11 %) : pas de chauffe ; plus lent (+10 %) : chauffe ; +4 % : sous le seuil
    for (const auto& [mediane, attendu] : {std::pair<float, bool>{17.8f, false}, {22.f, true}, {20.8f, false}}) {
        g3[2].images.mediane = mediane;
        bool chauffe = false; for (const std::string& l : remarques(g3)) chauffe |= contient(l, "chauffé");
        VERIFIE(chauffe == attendu, "contrôle à %g ms pour 20 : chauffe %d attendu %d", mediane, chauffe, attendu);
    }
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("banc du téléphone : OK\n");
    return 0;
}
