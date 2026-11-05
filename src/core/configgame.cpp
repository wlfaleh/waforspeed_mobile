// WaForSpeed — configuration du jeu par configgame.json (WFS-128) : voir configgame.h.
#include "configgame.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include "log.h"
#include "platform.h"

namespace wfs {

namespace {

// ---------------------------------------------------------------- table des clés (une ligne par réglage, RG-08)
enum class Type { Entier, Reel, Flottant };
struct Champ {
    const char* cle; Type type; void* ptr; double min, max; const char* unite; const char* aide;
};

std::vector<Champ> champs(ConfigJeu& c) {
    using T = Type;
    auto& s = c.score; auto& k = c.checkpoint; auto& n = c.nitro; auto& c5 = c.course5; auto& p = c.poursuite;
    auto& cam = c.camera; auto& ph = c.conduite;
    return {
        {"score.points_par_metre", T::Entier, &s.pointsParMetre, 0, 100, "points", "points gagnés par mètre parcouru"},
        {"score.quasi_contact", T::Entier, &s.quasiContact, 0, 10000, "points", "points d'un quasi-contact, multipliés par le combo"},
        {"score.combo_duree", T::Reel, &s.comboDuree, 0.5, 10, "s", "le combo continue si le quasi-contact suivant arrive avant ce délai"},
        {"score.choc_perd_combo", T::Reel, &s.chocPerdCombo, 0, 1, "impact (0 à 1)", "un petit choc plus fort que ça fait perdre le combo"},
        {"checkpoint.depart", T::Reel, &k.depart, 10, 600, "s", "chrono au départ de la course Checkpoint"},
        {"checkpoint.bonus", T::Reel, &k.bonus, 0, 300, "s", "secondes ajoutées à chaque checkpoint"},
        {"checkpoint.distance", T::Reel, &k.distanceM, 100, 5000, "m", "distance entre deux checkpoints"},
        {"nitro.depart", T::Reel, &n.depart, 0, 1, "réserve (0 à 1)", "réserve de nitro au départ"},
        {"nitro.poussee", T::Reel, &n.poussee, 1, 4, "×", "accélération multipliée pendant la nitro"},
        {"course5.ordinateurs", T::Entier, &c5.ordinateurs, 1, 4, "voitures", "voitures pilotées par l'ordinateur (course à 5 : 4)"},
        {"course5.vitesse_facile", T::Reel, &c5.facteur[0], 0.3, 1.2, "× vitesse de pointe", "vitesse visée par les ordinateurs, difficulté facile"},
        {"course5.vitesse_normale", T::Reel, &c5.facteur[1], 0.3, 1.2, "× vitesse de pointe", "vitesse visée par les ordinateurs, difficulté normale"},
        {"course5.vitesse_difficile", T::Reel, &c5.facteur[2], 0.3, 1.2, "× vitesse de pointe", "vitesse visée par les ordinateurs, difficulté difficile"},
        {"course5.ecart_min", T::Reel, &c5.ecartMinM, 0.2, 10, "m", "les ordinateurs ne s'approchent jamais plus de la voiture de devant"},
        {"course5.rattrapage_derriere", T::Reel, &c5.rattrapageDerriereM, 0, 5000, "m", "ordinateur plus loin que ça derrière le joueur : il accélère (WFS-133)"},
        {"course5.rattrapage_devant", T::Reel, &c5.rattrapageDevantM, 0, 5000, "m", "ordinateur plus loin que ça devant le joueur : il ralentit"},
        {"course5.rattrapage_bonus", T::Reel, &c5.rattrapageBonus, 0, 0.5, "part de la vitesse", "vitesse visée en plus d'un ordinateur trop loin derrière"},
        {"course5.rattrapage_malus", T::Reel, &c5.rattrapageMalus, 0, 0.5, "part de la vitesse", "vitesse visée en moins d'un ordinateur trop loin devant"},
        {"course5.rattrapage_malus_facile", T::Reel, &c5.rattrapageMalusFacile, 0, 0.5, "part de la vitesse", "même chose en difficulté facile"},
        {"course5.aspiration_distance", T::Reel, &c5.aspirationM, 0, 100, "m", "aspiration à moins de cette distance derrière une voiture"},
        {"course5.aspiration_bonus", T::Reel, &c5.aspirationBonus, 0, 0.3, "part de la vitesse", "vitesse visée en plus dans l'aspiration"},
        {"course5.nitro_bonus", T::Reel, &c5.nitroBonus, 0, 0.5, "part de la vitesse", "vitesse en plus pendant la nitro d'un ordinateur"},
        {"course5.nitro_duree", T::Reel, &c5.nitroDuree, 0, 10, "s", "durée de la nitro d'un ordinateur"},
        {"course5.nitro_fin", T::Reel, &c5.nitroFinM, 0, 5000, "m", "nitro permise avant l'arrivée à moins de cette distance"},
        {"poursuite.score_depart", T::Entier, &p.scoreDepart, 1, 1000, "points", "score au départ de la poursuite"},
        {"poursuite.distance", T::Reel, &p.kmArrivee, 1, 50, "km", "distance à parcourir pour gagner"},
        {"poursuite.choc_trafic", T::Entier, &p.chocTrafic, 0, 100, "points", "points perdus sur un choc avec le trafic"},
        {"poursuite.choc_glissiere", T::Entier, &p.chocGlissiere, 0, 100, "points", "points perdus sur un choc contre la glissière"},
        {"poursuite.choc_police", T::Entier, &p.chocPolice, 0, 100, "points", "points perdus sur un choc avec la police"},
        {"poursuite.colle_distance", T::Reel, &p.colleM, 1, 50, "m", "police plus près que ça : un point perdu par période"},
        {"poursuite.colle_periode", T::Reel, &p.colleS, 0.2, 10, "s", "période de la perte d'un point quand la police colle"},
        {"poursuite.police_depart", T::Entier, &p.policeDepart, 1, 6, "voitures", "voitures de police au départ"},
        {"poursuite.police_max", T::Entier, &p.policeMax, 1, 6, "voitures", "voitures de police au plus"},
        {"poursuite.renfort", T::Reel, &p.renfortKm, 0.5, 10, "km", "une voiture de police de plus tous les … km"},
        {"poursuite.densite_trafic", T::Reel, &p.densite, 1, 4, "×", "trafic multiplié pendant la poursuite"},
        {"camera.poursuite.recul", T::Flottant, &cam.loin.dist, 2, 30, "m", "vue Poursuite : distance derrière la voiture"},
        {"camera.poursuite.hauteur", T::Flottant, &cam.loin.h, 0.5, 20, "m", "vue Poursuite : hauteur de la caméra"},
        {"camera.poursuite.regard_devant", T::Flottant, &cam.loin.ahead, 0, 40, "m", "vue Poursuite : point visé devant la voiture"},
        {"camera.poursuite.regard_hauteur", T::Flottant, &cam.loin.lookH, -20, 10, "m", "vue Poursuite : hauteur du point visé (négatif : sous la route, caméra inclinée vers le bas)"},
        {"camera.poursuite.zoom", T::Flottant, &cam.fov[0], 30, 110, "degrés", "vue Poursuite : champ de vision (plus grand = plus large)"},
        {"camera.proche.recul", T::Flottant, &cam.proche.dist, 2, 30, "m", "vue Proche : distance derrière la voiture"},
        {"camera.proche.hauteur", T::Flottant, &cam.proche.h, 0.5, 20, "m", "vue Proche : hauteur de la caméra"},
        {"camera.proche.regard_devant", T::Flottant, &cam.proche.ahead, 0, 40, "m", "vue Proche : point visé devant la voiture"},
        {"camera.proche.regard_hauteur", T::Flottant, &cam.proche.lookH, -20, 10, "m", "vue Proche : hauteur du point visé"},
        {"camera.proche.zoom", T::Flottant, &cam.fov[3], 30, 110, "degrés", "vue Proche : champ de vision"},
        {"camera.capot.zoom", T::Flottant, &cam.fov[1], 30, 110, "degrés", "vue Capot : champ de vision"},
        {"camera.cockpit.zoom", T::Flottant, &cam.fov[2], 30, 110, "degrés", "vue Cockpit : champ de vision"},
        {"camera.zoom_vitesse", T::Flottant, &cam.elargVitesse, 0, 30, "degrés", "champ de vision ajouté à pleine vitesse"},
        {"camera.zoom_nitro", T::Flottant, &cam.elargNitro, 0, 30, "degrés", "champ de vision ajouté pendant la nitro"},
        {"camera.suivi_oeil", T::Flottant, &cam.suiviOeil, 1, 60, "/s", "vitesse à laquelle la caméra suit la voiture (plus grand = plus raide)"},
        {"camera.suivi_regard", T::Flottant, &cam.suiviRegard, 1, 60, "/s", "vitesse à laquelle le regard suit la route"},
        {"trafic.densite", T::Reel, &c.trafic.densite, 0.2, 3, "×", "nombre de voitures du trafic de chaque carte multiplié par ce facteur"},
        {"conduite.frein", T::Reel, &ph.brake, 1, 40, "m/s²", "freinage"},
        {"conduite.roulement", T::Reel, &ph.roll, 0, 5, "m/s²", "ralentissement au lever de pied (roulement)"},
        {"conduite.trainee", T::Reel, &ph.drag, 0, 0.01, "", "freinage de l'air (croît avec la vitesse au carré)"},
        {"conduite.frein_moteur", T::Reel, &ph.engineBrake, 0, 10, "m/s²", "frein moteur"},
        {"conduite.adherence", T::Reel, &ph.grip, 5, 60, "m/s²", "adhérence en virage (au-delà, la voiture glisse)"},
        {"conduite.braquage_max", T::Reel, &ph.maxSteer, 0.1, 1.5, "", "braquage le plus fort"},
        {"conduite.braquage_vitesse", T::Reel, &ph.steerV, 1, 60, "m/s", "vitesse au-delà de laquelle le braquage diminue"},
        {"conduite.braquage_entree", T::Reel, &ph.steerIn, 0.5, 30, "/s", "rapidité du volant quand on braque"},
        {"conduite.braquage_sortie", T::Reel, &ph.steerOut, 0.5, 30, "/s", "rapidité du volant quand on relâche"},
        {"conduite.realignement", T::Reel, &ph.align, 0, 20, "/s", "la voiture se remet dans l'axe de la route"},
        {"conduite.cap_max", T::Reel, &ph.maxHeading, 0.1, 1.6, "rad", "angle le plus grand avec la route en course"},
        {"conduite.hors_route", T::Reel, &ph.offRoad, 0, 40, "m/s²", "ralentissement hors de la route"},
        {"conduite.vitesse_libre", T::Reel, &ph.vLibre, 0, 40, "m/s", "conduite libre (demi-tour) sous cette vitesse"},
        {"conduite.vitesse_recul", T::Reel, &ph.vRecul, 0, 20, "m/s", "marche arrière enchaînée sous cette vitesse"},
        {"qualite.plancher_moyenne", T::Flottant, &c.qualite.plancherMoyenne, 0.5, 1, "× résolution", "résolution la plus basse en qualité Moyenne"},
    };
}

double lireChamp(const Champ& f) {
    switch (f.type) {
        case Type::Entier: return *(int*)f.ptr;
        case Type::Reel: return *(double*)f.ptr;
        default: return *(float*)f.ptr;
    }
}
void ecrireChamp(const Champ& f, double v) {
    switch (f.type) {
        case Type::Entier: *(int*)f.ptr = (int)v; break;
        case Type::Reel: *(double*)f.ptr = v; break;
        default: *(float*)f.ptr = (float)v; break;
    }
}
// valeurs calculées à partir des clés (unités du jeu)
void deduire(ConfigJeu& c) { c.checkpoint.distanceU = c.checkpoint.distanceM / 0.006; }

// ---------------------------------------------------------------- lecteur JSON maison (RG-05)
// Objets, nombres, booléens, chaînes, tableaux (ignorés), null. Les nombres sont lus à la main, sans strtod / atof :
// la langue du téléphone (virgule décimale) ne change rien (RG-16). Un nombre décimal « court » (au plus 15 chiffres,
// exposant ≤ 22) est lu exactement comme le littéral du code (mantisse entière / puissance de 10 exacte).
struct Valeur { enum { Nombre, Booleen, Autre } genre = Autre; double nombre = 0; };
struct Lecteur {
    const char* p; const char* fin; std::string erreur;
    std::map<std::string, Valeur> plat;   // « section.cle » → valeur
    void blancs() { while (p < fin && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++; }
    bool echec(const char* quoi) { if (erreur.empty()) erreur = quoi; return false; }
    bool chaine(std::string& out) {
        if (p >= fin || *p != '"') return echec("chaîne attendue");
        for (p++; p < fin && *p != '"'; p++) {
            if (*p == '\\') { if (++p >= fin) break; out += *p == 'n' ? '\n' : *p; } else out += *p;
        }
        if (p >= fin) return echec("chaîne non terminée");
        p++; return true;
    }
    bool nombre(double& v) {
        const char* d = p; bool neg = false;
        if (p < fin && (*p == '-' || *p == '+')) { neg = *p == '-'; p++; }
        unsigned long long m = 0; int chiffres = 0, exp10 = 0; bool vu = false;
        while (p < fin && *p >= '0' && *p <= '9') { if (chiffres < 18) { m = m * 10 + (*p - '0'); if (m) chiffres++; } else exp10++; p++; vu = true; }
        if (p < fin && *p == '.') { p++; while (p < fin && *p >= '0' && *p <= '9') { if (chiffres < 18) { m = m * 10 + (*p - '0'); if (m) chiffres++; exp10--; } p++; vu = true; } }
        if (!vu) { p = d; return echec("nombre attendu"); }
        if (p < fin && (*p == 'e' || *p == 'E')) {
            p++; bool eneg = false; if (p < fin && (*p == '-' || *p == '+')) { eneg = *p == '-'; p++; }
            int e = 0; bool ev = false; while (p < fin && *p >= '0' && *p <= '9') { e = std::min(e * 10 + (*p - '0'), 400); p++; ev = true; }
            if (!ev) return echec("exposant attendu");
            exp10 += eneg ? -e : e;
        }
        static const double P10[23] = {1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15,
                                       1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22};
        double x = (double)m;
        if (m < (1ULL << 53) && exp10 >= -22 && exp10 <= 22) x = exp10 < 0 ? x / P10[-exp10] : x * P10[exp10];   // exact
        else x = x * std::pow(10.0, exp10);
        v = neg ? -x : x;
        return true;
    }
    bool valeur(const std::string& chemin, int profondeur) {
        if (profondeur > 16) return echec("trop d'objets imbriqués");
        blancs();
        if (p >= fin) return echec("fin du fichier inattendue");
        if (*p == '{') {
            p++; blancs();
            if (p < fin && *p == '}') { p++; return true; }
            while (true) {
                blancs(); std::string cle;
                if (!chaine(cle)) return false;
                blancs(); if (p >= fin || *p != ':') return echec("« : » attendu");
                p++;
                if (!valeur(chemin.empty() ? cle : chemin + "." + cle, profondeur + 1)) return false;
                blancs();
                if (p < fin && *p == ',') { p++; continue; }
                if (p < fin && *p == '}') { p++; return true; }
                return echec("« , » ou « } » attendu");
            }
        }
        if (*p == '[') {   // tableaux : lus, ignorés (aucune clé n'en utilise)
            p++; blancs();
            if (p < fin && *p == ']') { p++; plat[chemin] = {}; return true; }
            while (true) {
                if (!valeur("", profondeur + 1)) return false;
                blancs();
                if (p < fin && *p == ',') { p++; continue; }
                if (p < fin && *p == ']') { p++; plat[chemin] = {}; return true; }
                return echec("« , » ou « ] » attendu");
            }
        }
        if (*p == '"') { std::string s; if (!chaine(s)) return false; if (!chemin.empty() && chemin[0] != '_') plat[chemin] = {}; return true; }
        if (fin - p >= 4 && !std::strncmp(p, "true", 4)) { p += 4; plat[chemin] = {Valeur::Booleen, 1}; return true; }
        if (fin - p >= 5 && !std::strncmp(p, "false", 5)) { p += 5; plat[chemin] = {Valeur::Booleen, 0}; return true; }
        if (fin - p >= 4 && !std::strncmp(p, "null", 4)) { p += 4; plat[chemin] = {}; return true; }
        double v; if (!nombre(v)) return false;
        plat[chemin] = {Valeur::Nombre, v};
        return true;
    }
};

std::string g_dossierTelephone;

// nombre écrit le plus court qui se relit à l'identique (fichier et README)
std::string ecrireNombre(double v, Type t) {
    if (t == Type::Entier) return std::to_string((long long)v);
    char b[64];
    const bool fixe = v == 0 || (std::fabs(v) >= 1e-6 && std::fabs(v) < 1e15);   // écriture décimale simple (60.0, 0.00042)
    for (int prec = fixe ? 0 : 1; prec <= (fixe ? 20 : 17); prec++) {
        std::snprintf(b, sizeof b, fixe ? "%.*f" : "%.*g", prec, v);
        for (char* q = b; *q; q++) if (*q == ',') *q = '.';   // jamais de virgule décimale (langue du PC)
        Lecteur l{b, b + std::strlen(b), {}, {}}; double r = 0;
        if (l.nombre(r) && (t == Type::Flottant ? (float)r == (float)v : r == v)) break;
    }
    std::string s = b;
    if (s.find_first_of(".e") == std::string::npos) s += ".0";
    return s;
}

} // namespace

BilanConfig lireConfig(const std::string& texte, const std::string& nom, ConfigJeu& c) {
    BilanConfig b; b.fichier = nom;
    std::vector<Champ> fs = champs(c);
    Lecteur l{texte.data(), texte.data() + texte.size(), {}, {}};
    l.blancs();
    const bool ok = l.p < l.fin && *l.p == '{' && l.valeur("", 0) && (l.blancs(), l.p == l.fin || l.echec("texte après la fin du JSON"));
    if (!ok) {   // JSON cassé : toutes les valeurs du code (RG-02)
        const size_t pos = (size_t)(l.p - texte.data());
        size_t ligne = 1; for (size_t i = 0; i < pos && i < texte.size(); i++) ligne += texte[i] == '\n';
        b.messages.push_back(nom + " : JSON invalide ligne " + std::to_string(ligne) + " (" + (l.erreur.empty() ? "objet attendu" : l.erreur) + ") : valeurs du jeu gardées");
        b.defauts = (int)fs.size(); b.erreurs = 1;
        return b;
    }
    b.jsonValide = true;
    for (const Champ& f : fs) {
        auto it = l.plat.find(f.cle);
        if (it == l.plat.end()) { b.defauts++; b.messages.push_back(std::string(f.cle) + " absente : valeur du jeu (" + ecrireNombre(lireChamp(f), f.type) + ")"); continue; }
        const Valeur v = it->second; l.plat.erase(it);
        const bool entier = f.type == Type::Entier;
        if (v.genre != Valeur::Nombre || (entier && v.nombre != std::floor(v.nombre))) {
            b.defauts++; b.erreurs++; b.messages.push_back(std::string(f.cle) + (entier ? " : nombre entier attendu" : " : nombre attendu") + ", valeur du jeu gardée");
            continue;
        }
        if (!(v.nombre >= f.min && v.nombre <= f.max)) {   // RG-04 : hors bornes
            b.defauts++; b.erreurs++;
            b.messages.push_back(std::string(f.cle) + " = " + ecrireNombre(v.nombre, Type::Reel) + " hors bornes [" + ecrireNombre(f.min, Type::Reel) + ", " +
                                 ecrireNombre(f.max, Type::Reel) + "] : valeur du jeu (" + ecrireNombre(lireChamp(f), f.type) + ")");
            continue;
        }
        ecrireChamp(f, v.nombre); b.lues++;
    }
    for (const auto& [cle, v] : l.plat) {   // clés inconnues (faute de frappe) : signalées, sans effet
        if (cle.empty() || cle[0] == '_' || cle.find("._") != std::string::npos) continue;   // « _aide » : commentaires du fichier
        b.erreurs++; b.messages.push_back(cle + " : clé inconnue, sans effet");
    }
    if (c.poursuite.policeDepart > c.poursuite.policeMax) {
        b.messages.push_back("poursuite.police_depart > police_max : police_depart ramené à police_max");
        c.poursuite.policeDepart = c.poursuite.policeMax;
    }
    deduire(c);
    return b;
}

void configCheminTelephone(const std::string& dossier) { g_dossierTelephone = dossier; }

static bool lireFichier(const std::string& chemin, std::string& out) {
    FILE* f = std::fopen(chemin.c_str(), "rb");
    if (!f) return false;
    char buf[4096]; size_t n; out.clear();
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) out.append(buf, n);
    std::fclose(f);
    return true;
}

BilanConfig chargerConfig() {
    std::string texte, nom;
    bool trouve = false;
#if defined(__ANDROID__)
    if (!g_dossierTelephone.empty() && lireFichier(g_dossierTelephone + "/configgame.json", texte)) { nom = g_dossierTelephone + "/configgame.json (téléphone)"; trouve = true; }
    if (!trouve) { std::vector<uint8_t> a; if (readAsset("configgame.json", a)) { texte.assign(a.begin(), a.end()); nom = "configgame.json (APK)"; trouve = true; } }
#else
    if (const char* e = std::getenv("WFS_CONFIG"); e && *e) { if (lireFichier(e, texte)) { nom = e; trouve = true; } else WFS_WARN("configuration : WFS_CONFIG=%s introuvable", e); }
    if (!trouve && lireFichier("configgame.json", texte)) { nom = "configgame.json"; trouve = true; }
#endif
    ConfigJeu c;   // toujours repartir des valeurs du code (un fichier retiré ne laisse aucune trace)
    BilanConfig b;
    if (trouve) b = lireConfig(texte, nom, c);
    else { b.defauts = configNombreCles(); deduire(c); }
    config() = c;
    for (const std::string& m : b.messages) WFS_WARN("configuration : %s", m.c_str());
    if (!trouve) WFS_LOG("configuration : aucun configgame.json, valeurs du jeu (%d clés)", b.defauts);
    else WFS_LOG("configuration : %s, %d clé(s) lue(s), %d valeur(s) du jeu, %d erreur(s)", nom.c_str(), b.lues, b.defauts, b.erreurs);
    return b;
}

int configNombreCles() { ConfigJeu c; return (int)champs(c).size(); }

std::string configJson(const ConfigJeu& c0) {
    ConfigJeu c = c0;
    const std::vector<Champ> fs = champs(c);
    std::string s = "{\n  \"_aide\": \"Configuration de WaForSpeed (WFS-128). Une ligne par clé : configgame.README.md. Valeurs = jeu actuel.\"";
    // « a.b.c » → objets a { b { c } } ; les clés d'une même section se suivent dans la table
    std::vector<std::string> ouverts;
    bool premier = false;   // premier élément de l'objet courant (pas de virgule avant)
    auto retrait = [&](size_t n) { return std::string(2 + 2 * n, ' '); };
    for (const Champ& f : fs) {
        std::vector<std::string> parts; std::string cur;
        for (const char* q = f.cle; *q; q++) { if (*q == '.') { parts.push_back(cur); cur.clear(); } else cur += *q; }
        size_t commun = 0;
        while (commun < ouverts.size() && commun < parts.size() && ouverts[commun] == parts[commun]) commun++;
        while (ouverts.size() > commun) { ouverts.pop_back(); s += "\n" + retrait(ouverts.size()) + "}"; premier = false; }
        while (ouverts.size() < parts.size()) {
            if (!premier) s += ",";
            s += "\n" + retrait(ouverts.size()) + "\"" + parts[ouverts.size()] + "\": {";
            ouverts.push_back(parts[ouverts.size()]); premier = true;
        }
        if (!premier) s += ",";
        s += "\n" + retrait(ouverts.size()) + "\"" + cur + "\": " + ecrireNombre(lireChamp(f), f.type);
        premier = false;
    }
    while (!ouverts.empty()) { ouverts.pop_back(); s += "\n" + retrait(ouverts.size()) + "}"; }
    return s + "\n}\n";
}

std::string configReadme() {
    ConfigJeu c;
    const std::vector<Champ> fs = champs(c);
    std::string s =
        "# configgame.json — réglages du jeu (WFS-128)\n\n"
        "Le fichier `configgame.json` (racine du dépôt) règle le jeu **sans toucher au code**. Il est copié dans l'APK.\n\n"
        "- **Sur le téléphone, sans reconstruire** : poser un `configgame.json` modifié dans "
        "`Android/data/com.waforspeed.game/files/` (câble USB ou gestionnaire de fichiers), puis relancer le jeu : il remplace "
        "celui de l'APK. Le retirer = retour aux valeurs de l'APK.\n"
        "- **Sur PC (banc d'essai)** : `./configgame.json`, ou le fichier donné par la variable `WFS_CONFIG` ; relu à chaque partie.\n"
        "- Les nombres s'écrivent avec un **point** (`0.5`, jamais `0,5`), comme en JSON.\n"
        "- Une clé absente, un mauvais type ou une valeur hors bornes : le jeu garde sa valeur et l'écrit au journal "
        "(`configuration : …` dans logcat). Un fichier cassé : toutes les valeurs du jeu. Le jeu ne plante jamais à cause du fichier.\n"
        "- Les clés qui commencent par `_` (comme `_aide`) sont des commentaires.\n"
        "- Ce README et le fichier sont fabriqués par `./build/test_configgame --ecrire` : ajouter une clé = une ligne dans "
        "`src/core/configgame.cpp` (table `champs`), puis relancer cette commande.\n\n"
        "| Clé | Sens | Unité | Min | Max | Défaut |\n|---|---|---|---|---|---|\n";
    for (const Champ& f : fs)
        s += std::string("| `") + f.cle + "` | " + f.aide + " | " + f.unite + " | " + ecrireNombre(f.min, f.type) + " | " +
             ecrireNombre(f.max, f.type) + " | " + ecrireNombre(lireChamp(f), f.type) + " |\n";
    s += "\nPas encore dans le fichier (tickets en cours sur ces zones) : taille et place des commandes tactiles (WFS-102), "
         "nombre de joueurs du duel et distances proposées (écrans de WFS-102). À ajouter quand ces tickets seront fusionnés.\n";
    return s;
}

} // namespace wfs
