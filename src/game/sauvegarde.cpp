// WaForSpeed — sauvegarde et progression (WFS-24) : voir sauvegarde.h.
#include "sauvegarde.h"
#include <cstdio>
#include <cstring>
#include <sstream>
#include "../core/log.h"
#include "cars.h"
#include "track.h"
#ifndef _WIN32
#include <unistd.h>
#endif

namespace wfs {

const PeintureDef* peintureDef(const std::string& id) {
    for (const auto& p : PEINTURES) if (id == p.id) return &p;
    return nullptr;
}

const char* peintureOrigine(const std::string& carId) {
    // paint de CARS[] du jeu web (js/cars/catalog.js) ; les 4 premières voitures du catalogue natif ont une teinte propre
    // (cars.h), les 16 de WFS-13 la teinte exacte de leur peinture
    if (carId == "m4" || carId == "rrsport") return "noir";
    if (carId == "p911") return "gris";
    if (carId == "gallardo") return "jaune";
    for (const auto& c : CARS)
        if (carId == c.id) for (const auto& p : PEINTURES) if (p.hex == c.paint) return p.id;
    return "bleu";   // golf
}

static bool voitureExiste(const std::string& id) {
    for (const auto& c : CARS) if (id == c.id) return true;
    return false;
}
static bool carteExiste(const std::string& id) { return id == mapById(id.c_str()).id; }   // mapById : MAPS[0] si inconnue
static const char* CAMS[] = {"poursuite", "capot", "cockpit", "proche"};   // VUES de src/game/camera_vue.h (WFS-66, WFS-85)
static const char* QUALITES[] = {"low", "medium", "high"};         // quality du web (low / high) + niveau moyen (WFS-68)

// cleanName du jeu web (js/net/duel.js) : caractères de contrôle retirés, espaces autour retirés, 14 caractères au plus
static std::string nettoyerPseudo(const std::string& s) {
    std::string t;
    for (unsigned char c : s) if (c >= 0x20 && c != 0x7f) t += (char)c;
    const size_t a = t.find_first_not_of(' '), b = t.find_last_not_of(' ');
    if (a == std::string::npos) return "";
    t = t.substr(a, b - a + 1);
    size_t n = 0, i = 0;   // coupe à 14 caractères (UTF-8 : octets de suite 10xxxxxx non comptés)
    for (; i < t.size(); i++) if (((unsigned char)t[i] & 0xC0) != 0x80 && n++ == 14) break;
    return t.substr(0, i);
}

static bool entier(const std::string& s, long long& v) {
    if (s.empty() || s.size() > 18) return false;
    v = 0;
    for (char c : s) { if (c < '0' || c > '9') return false; v = v * 10 + (c - '0'); }
    return true;
}
// WFS-125 : « a;b;c » → n entiers positifs ; faux si mal formé (la clé est alors ignorée, RG-03)
static bool entiers(const std::string& s, long long* v, int n) {
    size_t a = 0;
    for (int i = 0; i < n; i++) {
        const size_t b = i + 1 < n ? s.find(';', a) : s.size();
        if (b == std::string::npos || !entier(s.substr(a, b - a), v[i])) return false;
        a = b + 1;
    }
    return true;
}

namespace records {
bool course5Bat(int place, long long ms, int ancPlace, long long ancMs) {
    if (place < 1) return false;
    if (ancPlace < 1) return true;
    return place < ancPlace || (place == ancPlace && ms < ancMs);
}
bool poursuiteBat(bool victoire, int score, long long distM, bool ancVictoire, int ancScore, long long ancDistM) {
    if (ancDistM < 0) return true;                       // record vide
    if (victoire != ancVictoire) return victoire;        // une victoire bat toujours un « ARRÊTÉ »
    return victoire ? score > ancScore : distM > ancDistM;
}
} // namespace records

void Sauvegarde::defauts() {
    best = 0; car = "golf"; paint.clear(); map = "ville"; tod = "nuit"; quality = "high"; qualityChoisie = false; cam = "poursuite";
    muted = false; music = true; nick = "Pilote"; km = 4; steer = "glisser"; gaucher = false; vibrations = true;
    grandsBoutons = false; gyro = 0; gyroZone = 1;   // WFS-108
    mode = "checkpoint"; km5 = 4; diff5 = "normale";   // WFS-110 ; WFS-133
    rec5Place = 0; rec5Ms = 0; recPVictoire = false; recPScore = 0; recPDistM = -1;   // WFS-125 : records vides
}

std::string Sauvegarde::texte() const {
    std::ostringstream o;
    o << "version=" << VERSION << "\n";
    o << "waforspeed-best=" << best << "\n";
    o << "car=" << car << "\n";
    for (const auto& [id, p] : paint) o << "paint-" << id << "=" << p << "\n";
    o << "map=" << map << "\n" << "tod=" << tod << "\n";
    if (qualityChoisie) o << "quality=" << quality << "\n";   // absente : qualité automatique (WFS-15)
    o << "cam=" << cam << "\n";
    o << "muted=" << (muted ? 1 : 0) << "\n" << "music=" << (music ? 1 : 0) << "\n" << "nick=" << nick << "\n" << "km=" << km << "\n" << "steer=" << steer << "\n";
    o << "commandes=" << (gaucher ? "gaucher" : "droitier") << "\n" << "vibrations=" << (vibrations ? 1 : 0) << "\n";   // WFS-75
    o << "boutons=" << (grandsBoutons ? "grands" : "normaux") << "\n" << "gyro=" << gyro << "\n" << "gyroZone=" << gyroZone << "\n";   // WFS-108
    o << "mode=" << mode << "\n" << "km5=" << km5 << "\n" << "diff5=" << diff5 << "\n";   // WFS-110 ; WFS-133
    if (aRecordCourse5()) o << "record-course5=" << rec5Place << ";" << rec5Ms << "\n";   // WFS-125 : absents tant que vides
    if (aRecordPoursuite()) o << "record-poursuite=" << (recPVictoire ? 1 : 0) << ";" << recPScore << ";" << recPDistM << "\n";
    return o.str();
}

bool Sauvegarde::lire(const std::string& contenu) {
    defauts();
    std::istringstream in(contenu);
    std::string ligne;
    bool premiere = true;
    while (std::getline(in, ligne)) {
        if (!ligne.empty() && ligne.back() == '\r') ligne.pop_back();
        if (premiere) {   // RG-05 : version de format ; autre chose = fichier corrompu → défauts
            premiere = false;
            if (ligne != "version=1") { WFS_LOG("sauvegarde : format inconnu (« %.40s ») : valeurs par défaut", ligne.c_str()); return false; }
            continue;
        }
        const size_t eq = ligne.find('=');
        if (eq == std::string::npos) continue;   // ligne illisible : ignorée
        std::string k = ligne.substr(0, eq);
        const std::string v = ligne.substr(eq + 1);
        if (k.rfind("waforspeed-", 0) == 0) k = k.substr(11);   // clés du web acceptées avec ou sans préfixe
        long long n = 0;
        // RG-04 : chaque valeur est validée, une valeur inconnue garde le défaut de sa clé
        if (k == "best") { if (entier(v, n)) best = n; }
        else if (k == "car") { if (voitureExiste(v)) car = v; }
        else if (k.rfind("paint-", 0) == 0) { const std::string id = k.substr(6); if (voitureExiste(id) && peintureDef(v)) paint[id] = v; }
        else if (k == "map") { if (carteExiste(v)) map = v; }
        else if (k == "tod") { tod = v == "jour" ? "jour" : "nuit"; }
        else if (k == "quality") { for (const char* q : QUALITES) if (v == q) { quality = v; qualityChoisie = true; } }
        else if (k == "cam") { for (const char* c : CAMS) if (v == c) cam = v; if (v == "loin") cam = "poursuite"; }   // « loin » du web = poursuite (WFS-85)
        else if (k == "muted") { muted = v == "1"; }
        else if (k == "music") { music = v != "0"; }
        else if (k == "nick") { const std::string p = nettoyerPseudo(v); if (!p.empty()) nick = p; }
        else if (k == "km") { if (entier(v, n) && (n == 2 || n == 4 || n == 6 || n == 10)) km = (int)n; }
        else if (k == "steer") { steer = v == "fleches" ? "fleches" : "glisser"; }
        else if (k == "commandes") { gaucher = v == "gaucher"; } else if (k == "vibrations") { vibrations = v != "0"; }   // WFS-75
        else if (k == "boutons") { grandsBoutons = v == "grands"; }   // WFS-108
        else if (k == "gyro") { if (entier(v, n) && n >= 0 && n <= 3) gyro = (int)n; }
        else if (k == "gyroZone") { if (entier(v, n) && n >= 0 && n <= 2) gyroZone = (int)n; }
        else if (k == "mode") { mode = v == "course5" || v == "poursuite" ? v : "checkpoint"; }   // WFS-110 ; WFS-119 : poursuite
        else if (k == "km5") { if (entier(v, n) && (n == 2 || n == 4 || n == 6 || n == 10)) km5 = (int)n; }
        else if (k == "diff5") { diff5 = v == "facile" || v == "difficile" ? v : "normale"; }   // WFS-133 : inconnue → normale
        else if (k == "record-course5") {   // WFS-125 : place 1..5, temps en ms
            long long w[2];
            if (entiers(v, w, 2) && w[0] >= 1 && w[0] <= 5) { rec5Place = (int)w[0]; rec5Ms = w[1]; }
        } else if (k == "record-poursuite") {   // WFS-125 : victoire 0/1, score 0..100, distance en m
            long long w[3];
            if (entiers(v, w, 3) && w[0] <= 1 && w[1] <= 100) { recPVictoire = w[0] == 1; recPScore = (int)w[1]; recPDistM = w[2]; }
        }
    }
    if (premiere) return false;   // fichier vide
    return true;
}

bool Sauvegarde::battreRecord(long long score) {
    if (score <= best) return false;
    best = score;
    return true;
}

bool Sauvegarde::battreRecordCourse5(int place, long long tempsMs) {
    if (!records::course5Bat(place, tempsMs, rec5Place, rec5Ms)) return false;
    rec5Place = place; rec5Ms = tempsMs;
    return true;
}

bool Sauvegarde::battreRecordPoursuite(bool victoire, int score, long long distanceM) {
    if (!records::poursuiteBat(victoire, score, distanceM, recPVictoire, recPScore, recPDistM)) return false;
    recPVictoire = victoire; recPScore = score; recPDistM = distanceM;
    return true;
}

std::string Sauvegarde::peinture(const std::string& carId) const {
    const auto it = paint.find(carId);
    return it != paint.end() && peintureDef(it->second) ? it->second : peintureOrigine(carId);
}

const PeintureDef* Sauvegarde::peintureChoisie(const std::string& carId) const {
    const auto it = paint.find(carId);
    return it != paint.end() ? peintureDef(it->second) : nullptr;
}

int Sauvegarde::camIndex() const {
    for (int i = 0; i < 4; i++) if (cam == CAMS[i]) return i;
    return 0;
}
void Sauvegarde::setCamIndex(int i) { cam = CAMS[i >= 0 && i < 4 ? i : 0]; }
int Sauvegarde::qualiteIndex() const {
    for (int i = 0; i < 3; i++) if (quality == QUALITES[i]) return i;
    return 2;
}
void Sauvegarde::setQualiteIndex(int i) { quality = QUALITES[i >= 0 && i < 3 ? i : 2]; qualityChoisie = true; }

bool Sauvegarde::charger(const std::string& chemin) {
    attendre();
    chemin_ = chemin; actif_ = true;
    defauts();
    std::FILE* f = std::fopen(chemin.c_str(), "rb");
    if (!f) { WFS_LOG("sauvegarde : aucun fichier (%s) : valeurs par défaut", chemin.c_str()); return false; }
    std::string c; char buf[4096]; size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0 && c.size() < (1u << 20)) c.append(buf, n);   // 1 Mo au plus
    std::fclose(f);
    const bool ok = lire(c);
    WFS_LOG("sauvegarde : %s lue (%s) : car=%s best=%lld", chemin.c_str(), ok ? "valide" : "corrompue, défauts", car.c_str(), best);
    return ok;
}

bool ecrireAtomique(const std::string& chemin, const std::string& contenu) {
    const std::string tmp = chemin + ".tmp";
    std::FILE* f = std::fopen(tmp.c_str(), "wb");
    if (!f) return false;
    bool ok = std::fwrite(contenu.data(), 1, contenu.size(), f) == contenu.size() && std::fflush(f) == 0;
#ifndef _WIN32
    ok = ok && fsync(fileno(f)) == 0;   // données sur le disque avant le renommage
#endif
    ok = std::fclose(f) == 0 && ok;
#ifdef _WIN32
    if (ok) std::remove(chemin.c_str());   // rename ne remplace pas un fichier existant sous Windows
#endif
    ok = ok && std::rename(tmp.c_str(), chemin.c_str()) == 0;
    if (!ok) std::remove(tmp.c_str());
    return ok;
}

void Sauvegarde::enregistrer() {
    if (!actif_ || chemin_.empty()) return;
    std::lock_guard<std::mutex> l(m_);
    aEcrire_ = texte(); cheminAEcrire_ = chemin_; enAttente_ = true;   // copie faite ici : le thread ne lit que sa copie
    if (!ecrivain_.joinable()) ecrivain_ = std::thread([this] { boucleEcriture(); });
    cv_.notify_all();
}

void Sauvegarde::enregistrerFichier(const std::string& chemin, std::string contenu) {
    if (!actif_ || chemin.empty()) return;
    std::lock_guard<std::mutex> l(m_);
    for (auto& f : fichiers_) if (f.first == chemin) { f.second = std::move(contenu); cv_.notify_all(); return; }   // le plus récent
    fichiers_.emplace_back(chemin, std::move(contenu));
    if (!ecrivain_.joinable()) ecrivain_ = std::thread([this] { boucleEcriture(); });
    cv_.notify_all();
}

std::string Sauvegarde::cheminVoisin(const std::string& nom) const {
    const size_t k = chemin_.find_last_of("/\\");
    return (k == std::string::npos ? std::string() : chemin_.substr(0, k + 1)) + nom;
}

void Sauvegarde::desactiver() {
    attendre();
    actif_ = false; chemin_.clear();
    defauts();
}

void Sauvegarde::boucleEcriture() {
    std::unique_lock<std::mutex> l(m_);
    while (true) {
        cv_.wait(l, [this] { return enAttente_ || !fichiers_.empty() || quitter_; });
        if (!enAttente_ && fichiers_.empty()) return;   // quitter_ sans écriture en attente
        std::string c, p;
        if (enAttente_) { c = std::move(aEcrire_); p = cheminAEcrire_; enAttente_ = false; }
        else { c = std::move(fichiers_.front().second); p = fichiers_.front().first; fichiers_.pop_front(); }   // WFS-125
        enCours_ = true;
        l.unlock();
        const bool ok = ecrireAtomique(p, c);
        if (!ok) WFS_ERR("sauvegarde : écriture impossible (%s)", p.c_str());
        l.lock();
        enCours_ = false;
        cv_.notify_all();
    }
}

void Sauvegarde::attendre() {
    std::unique_lock<std::mutex> l(m_);
    cv_.wait(l, [this] { return !enAttente_ && fichiers_.empty() && !enCours_; });
}

Sauvegarde::~Sauvegarde() {
    { std::lock_guard<std::mutex> l(m_); quitter_ = true; cv_.notify_all(); }
    if (ecrivain_.joinable()) ecrivain_.join();   // l'écriture en attente est faite avant de quitter
}

Sauvegarde& sauvegarde() {
    static Sauvegarde s;
    return s;
}

} // namespace wfs
