// WaForSpeed — WFS-420 : banc de performance automatique sur le téléphone (mesure seule, aucune correction).
// Ce fichier : le déroulé des essais, les statistiques d'un essai et le texte des résultats. Calcul seul (aucun appel Vulkan ni
// Android) : testé par tests/test_banc_telephone.cpp. La conduite du banc (chargement de la Ville, rejeu du trajet, mesure image
// par image, écrans du protocole et des résultats) est dans src/ui/ecrans/reglages_banc.cpp.
//
// RG-01 : Ville de nuit (pluie), puis de jour ; caméra Arrière, graine du trafic GRAINE, pilote automatique : aucune conduite à la main.
// RG-02 : chaque essai = CHAUFFE_S de chauffe puis MESURE_S de mesure ; pour chaque moment : tout éteint (référence), puis un seul
//         interrupteur de WFS-419 à la fois (12), puis de nouveau tout éteint (contrôle de la chauffe) : 14 essais × 2 moments.
// RG-03 : médiane, 95 %, 99 %, img/s, temps GPU par passe et total, temps processeur, triangles, appels, matériaux, mémoire graphique
//         par type, chauffe et température ; les passes doivent expliquer au moins PART_MIN du temps GPU total (sinon c'est écrit).
// RG-06 : hors banc, rien n'est mesuré ni écrit (actif = faux).
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace wfs::banc_tel {

constexpr double CHAUFFE_S = 30, MESURE_S = 60;   // RG-02
constexpr uint32_t GRAINE = 0x5eed1u;              // RG-01 : graine du trafic (celle de la course solo)
constexpr double PART_MIN = 0.90;                  // RG-03 : part du temps GPU total expliquée par les passes
constexpr int N_INTERRUPTEURS = 12;                // diag::N_INTERRUPTEURS (vérifié par reglages_banc.cpp)

inline bool actif = false;   // RG-06 : vrai seulement pendant le banc (qualité automatique figée, mesures)
inline double dureeChauffe = CHAUFFE_S, dureeMesure = MESURE_S;   // banc PC seulement : durées raccourcies (WFS_BANC_TEL_DUREES)

struct Essai {
    bool nuit = true;
    int interrupteur = -1;   // −1 : tout éteint
    bool controle = false;   // tout éteint, dernier essai du moment (contrôle de la chauffe)
};
// RG-02 : nuit (14 essais) puis jour (14 essais)
std::vector<Essai> deroule();
// « Référence (tout éteint) », « MSAA ×1 », « Contrôle (tout éteint) » ; libelles = diag::INFOS[i].libelle
std::string nomEssai(const Essai& e, const char* const libelles[N_INTERRUPTEURS]);

// durées d'image (ms) d'un essai ; le tableau est trié sur place
struct Stats { float mediane = 0, p95 = 0, p99 = 0, ips = 0; int images = 0; double secondes = 0; };
Stats stats(std::vector<float>& ms);

struct Resultat {
    Essai essai; std::string nom;
    Stats images;
    // GPU (ms par image, moyenne sur l'essai) : horodatages du Renderer ; mesure = faux sans horodatages
    bool gpuMesure = false; int gpuImages = 0;
    double gpuOmbre = 0, gpuReflet = 0, gpuScene = 0, gpuPost = 0, gpuTotal = 0;
    std::string passesEnfichables;   // passes du socle WFS-150 (comprises dans « scène »), texte de passes::texteCouts
    // processeur (ms par image) : logique (Game::update), préparation (Game::render hors attente et envoi), envoi (endFrame),
    // attente du GPU (beginFrame)
    double cpuLogique = 0, cpuPreparation = 0, cpuEnvoi = 0, cpuAttente = 0;
    double triangles = 0, appels = 0, materiaux = 0;
    double memoireGpuMo = 0; std::string memoireParType;
    int chauffeDebut = -1, chauffeFin = -1;           // statut thermique d'Android (0 aucun … 6 arrêt ; −1 inconnu)
    double temperatureDebut = -1000, temperatureFin = -1000;   // °C de la batterie (−1000 : non disponible)
    std::string rendu;   // ligne « Rendu » (niveau, échelle, MSAA) au moment de la mesure
    bool interrompu = false;
};
// RG-03 : part du temps GPU total expliquée par les passes (ombre + reflet + scène + post) ; −1 sans mesure
double partExpliquee(const Resultat& r);
// écart de la médiane d'un essai à la référence du même moment (%, + = plus lent) ; 0 sans référence
double ecartReference(const std::vector<Resultat>& res, size_t i);

struct Entete {
    std::string date, appareil, interrupteursJson, protocole;
    int batterie = -1, enCharge = -1;   // relevés au départ (−1 : non disponible)
    bool interrompu = false; std::string raison;
};
// RG-04 : texte complet du fichier diagnostic-AAAA-MM-JJ-hhmm.txt
std::string texteFichier(const Entete& e, const std::vector<Resultat>& res);
// RG-04 : nom du fichier pour une date locale
std::string nomFichier(int annee, int mois, int jour, int heure, int minute);
// ligne courte du tableau à l'écran : « 16,8 · 18,1 · 25,0 · 58,9 · 12,4 · −3 % »
std::string ligneTableau(const std::vector<Resultat>& res, size_t i);
// remarques pour l'écran (passes < 90 %, contrôle de la chauffe, sans horodatages) ; vide si rien à signaler
std::vector<std::string> remarques(const std::vector<Resultat>& res);

// RG-05 : relevés du téléphone pour le protocole (fichiers du système, lisibles sans permission sur la plupart des téléphones) ;
// −1 / −1000 si la lecture est impossible (banc PC, téléphone qui la refuse)
int batteriePourcent();
int batterieEnCharge();          // 1 en charge, 0 non, −1 inconnu
double temperatureBatterie();     // °C

// mise en forme des nombres à la française (« 16,8 »)
std::string virgule(double v, int decimales = 1);

} // namespace wfs::banc_tel
