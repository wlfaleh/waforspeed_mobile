// WaForSpeed — WFS-420 : banc de performance automatique sur le téléphone. Voir banc_telephone.h.
#include "banc_telephone.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace wfs::banc_tel {

std::vector<Essai> deroule() {
    std::vector<Essai> d;
    for (bool nuit : {true, false}) {
        d.push_back({nuit, -1, false});
        for (int i = 0; i < N_INTERRUPTEURS; i++) d.push_back({nuit, i, false});
        d.push_back({nuit, -1, true});
    }
    return d;
}

std::string nomEssai(const Essai& e, const char* const libelles[N_INTERRUPTEURS]) {
    if (e.interrupteur < 0) return e.controle ? "Contrôle (tout éteint)" : "Référence (tout éteint)";
    return e.interrupteur < N_INTERRUPTEURS && libelles && libelles[e.interrupteur] ? libelles[e.interrupteur] : "?";
}

// rang « au plus proche » (même règle que qualite::MesureImages, WFS-246) : p dans [0, 1]
static float rang(const std::vector<float>& t, double p) {
    if (t.empty()) return 0;
    const size_t i = (size_t)std::min<double>((double)t.size() - 1, std::floor(p * (double)(t.size() - 1) + 0.5));
    return t[i];
}

Stats stats(std::vector<float>& ms) {
    Stats s;
    if (ms.empty()) return s;
    double somme = 0;
    for (float v : ms) somme += v;
    std::sort(ms.begin(), ms.end());
    s.images = (int)ms.size(); s.secondes = somme / 1000;
    s.mediane = rang(ms, 0.5); s.p95 = rang(ms, 0.95); s.p99 = rang(ms, 0.99);
    s.ips = somme > 0 ? (float)(ms.size() * 1000.0 / somme) : 0;
    return s;
}

double partExpliquee(const Resultat& r) {
    if (!r.gpuMesure || r.gpuTotal <= 0) return -1;
    return (r.gpuOmbre + r.gpuReflet + r.gpuScene + r.gpuPost) / r.gpuTotal;
}

double ecartReference(const std::vector<Resultat>& res, size_t i) {
    if (i >= res.size()) return 0;
    for (const Resultat& r : res)
        if (r.essai.nuit == res[i].essai.nuit && r.essai.interrupteur < 0 && !r.essai.controle && r.images.mediane > 0)
            return ((double)res[i].images.mediane / (double)r.images.mediane - 1) * 100;
    return 0;
}

std::string virgule(double v, int decimales) {
    char b[32]; std::snprintf(b, sizeof b, "%.*f", decimales, v);
    for (char* p = b; *p; p++) if (*p == '.') *p = ',';
    return b;
}

static std::string signe(double v) {   // « +3 % », « −12 % »
    const long n = std::lround(v);
    return (n > 0 ? "+" : n < 0 ? "−" : "") + std::to_string(std::labs(n)) + " %";
}

std::string ligneTableau(const std::vector<Resultat>& res, size_t i) {
    if (i >= res.size()) return "";
    const Resultat& r = res[i];
    if (!r.images.images) return r.interrompu ? "interrompu" : "—";
    std::string l = virgule(r.images.mediane) + " · " + virgule(r.images.p95) + " · " + virgule(r.images.p99) + " · " + virgule(r.images.ips, 0) + " · " +
                    (r.gpuMesure ? virgule(r.gpuTotal) : std::string("—"));
    const bool reference = r.essai.interrupteur < 0 && !r.essai.controle;
    return l + " · " + (reference ? std::string("réf.") : signe(ecartReference(res, i)));
}

std::vector<std::string> remarques(const std::vector<Resultat>& res) {
    std::vector<std::string> m;
    bool sansGpu = false;
    for (size_t i = 0; i < res.size(); i++) {
        const Resultat& r = res[i];
        if (!r.images.images) continue;
        if (!r.gpuMesure) sansGpu = true;
        const double p = partExpliquee(r);
        if (p >= 0 && p < PART_MIN)
            m.push_back(std::string(r.essai.nuit ? "Nuit" : "Jour") + ", " + r.nom + " : les passes n'expliquent que " + virgule(p * 100, 0) +
                        " % du temps GPU total (moins de 90 %).");
        if (r.essai.controle) {
            const double e = ecartReference(res, i);
            if (std::fabs(e) >= 5)
                m.push_back(std::string(r.essai.nuit ? "Nuit" : "Jour") + " : le contrôle final s'écarte de " + signe(e) +
                            " de la référence : le téléphone a chauffé, les écarts de ce moment sont à lire avec prudence.");
        }
    }
    if (sansGpu) m.push_back("Temps GPU : horodatages non disponibles sur ce téléphone (colonnes GPU vides).");
    return m;
}

std::string nomFichier(int annee, int mois, int jour, int heure, int minute) {
    char b[64]; std::snprintf(b, sizeof b, "diagnostic-%04d-%02d-%02d-%02d%02d.txt", annee, mois, jour, heure, minute);
    return b;
}

static std::string temperature(double c) { return c > -999 ? virgule(c) + " °C" : "non disponible"; }
static std::string chauffe(int s) {
    static const char* const N[] = {"aucune", "légère", "modérée", "sévère", "critique", "urgence", "arrêt"};
    return s >= 0 && s <= 6 ? std::to_string(s) + " (" + N[s] + ")" : "inconnue";
}

std::string texteFichier(const Entete& e, const std::vector<Resultat>& res) {
    std::string t = "WaForSpeed : banc de performance automatique du téléphone (WFS-420)\n";
    t += "date : " + e.date + "\n";
    t += "appareil : " + e.appareil + "\n";
    t += "protocole : " + e.protocole + "\n";
    t += "au départ : batterie " + (e.batterie >= 0 ? std::to_string(e.batterie) + " %" : std::string("non disponible")) + ", " +
         (e.enCharge == 1 ? "EN CHARGE (protocole non respecté)" : e.enCharge == 0 ? "pas en charge" : "charge non disponible") + "\n";
    if (!e.interrupteursJson.empty()) t += "ATTENTION : interrupteurs allumés dans configgame.json pendant tout le banc : " + e.interrupteursJson + "\n";
    if (e.interrompu) t += "BANC INTERROMPU : " + e.raison + " (résultats partiels)\n";
    t += "\nTableau (durée d'image en ms ; écart = médiane comparée à la référence du même moment, + = plus lent)\n";
    t += "moment | essai | médiane | 95 % | 99 % | img/s | GPU total | écart\n";
    for (size_t i = 0; i < res.size(); i++)
        t += std::string(res[i].essai.nuit ? "nuit" : "jour") + " | " + res[i].nom + " | " + ligneTableau(res, i) + "\n";
    for (const std::string& r : remarques(res)) t += "remarque : " + r + "\n";
    t += "\nDétail des essais\n";
    for (size_t i = 0; i < res.size(); i++) {
        const Resultat& r = res[i];
        t += "\n[" + std::to_string(i + 1) + "] " + (r.essai.nuit ? "nuit" : "jour") + " — " + r.nom + (r.interrompu ? " (interrompu)" : "") + "\n";
        if (!r.images.images) { t += "  aucune image mesurée\n"; continue; }
        t += "  " + r.rendu + "\n";
        t += "  images : " + std::to_string(r.images.images) + " en " + virgule(r.images.secondes) + " s ; médiane " + virgule(r.images.mediane) + " ms, 95 % " +
             virgule(r.images.p95) + " ms, 99 % " + virgule(r.images.p99) + " ms, " + virgule(r.images.ips) + " img/s\n";
        if (r.gpuMesure) {
            const double p = partExpliquee(r);
            t += "  GPU (ms par image, " + std::to_string(r.gpuImages) + " images) : ombre " + virgule(r.gpuOmbre, 2) + ", reflet " + virgule(r.gpuReflet, 2) +
                 ", scène " + virgule(r.gpuScene, 2) + ", post (bloom, image finale, interface) " + virgule(r.gpuPost, 2) + ", total " + virgule(r.gpuTotal, 2) +
                 " ; passes = " + virgule(p * 100, 0) + " % du total" + (p < PART_MIN ? " (MOINS DE 90 %)" : "") + "\n";
            if (!r.passesEnfichables.empty()) t += "  passes du socle (comprises dans « scène ») : " + r.passesEnfichables + "\n";
        } else t += "  GPU : horodatages non disponibles\n";
        t += "  processeur (ms par image) : logique " + virgule(r.cpuLogique, 2) + ", préparation " + virgule(r.cpuPreparation, 2) + ", envoi " +
             virgule(r.cpuEnvoi, 2) + ", attente du GPU " + virgule(r.cpuAttente, 2) + "\n";
        t += "  dessin (par image) : " + virgule(r.triangles / 1000, 0) + " k triangles, " + virgule(r.appels, 0) + " appels, " + virgule(r.materiaux, 0) + " matériaux distincts\n";
        t += "  mémoire graphique : " + virgule(r.memoireGpuMo, 0) + " Mo : " + r.memoireParType + "\n";
        t += "  chauffe (statut d'Android) : " + chauffe(r.chauffeDebut) + " → " + chauffe(r.chauffeFin) + " ; batterie " + temperature(r.temperatureDebut) +
             " → " + temperature(r.temperatureFin) + "\n";
    }
    return t;
}

// ---------------------------------------------------------------- RG-05 : relevés du téléphone (fichiers du système)
static bool lireLigne(const char* chemin, char* b, size_t n) {
    FILE* f = std::fopen(chemin, "r");
    if (!f) return false;
    const bool ok = std::fgets(b, (int)n, f) != nullptr;
    std::fclose(f);
    return ok;
}
int batteriePourcent() {
    char b[32];
    if (!lireLigne("/sys/class/power_supply/battery/capacity", b, sizeof b)) return -1;
    const int v = std::atoi(b);
    return v >= 0 && v <= 100 ? v : -1;
}
int batterieEnCharge() {
    char b[32];
    if (!lireLigne("/sys/class/power_supply/battery/status", b, sizeof b)) return -1;
    if (!std::strncmp(b, "Charging", 8) || !std::strncmp(b, "Full", 4)) return 1;
    if (!std::strncmp(b, "Discharging", 11) || !std::strncmp(b, "Not charging", 12)) return 0;
    return -1;
}
double temperatureBatterie() {
    char b[32];
    if (!lireLigne("/sys/class/power_supply/battery/temp", b, sizeof b)) return -1000;
    const int v = std::atoi(b);   // dixièmes de degré
    return v > -400 && v < 1200 ? v / 10.0 : -1000;
}

} // namespace wfs::banc_tel
