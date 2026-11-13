// WaForSpeed — système de langue (WFS-504, épic E20) : voir langue.h et langue_regles.h.
#include "langue.h"
#include <atomic>
#include <cstdlib>
#include <map>
#include <mutex>
#include <set>
#include <string_view>
#include "../core/log.h"
#include "../core/platform.h"
#include "text.h"

namespace wfs {
namespace ui {
namespace {

using Table = std::map<std::string, std::string, std::less<>>;   // std::less<> : find(const char*) sans copie de la clé (revue #372)
Table g_tables[2];   // [Langue::Francais], [Langue::Anglais] ; immuables après charger()
bool g_charge = false;
langue::Choix g_choix = langue::Choix::Automatique;
std::mutex g_mutexTel;   // RG-01 : écrite par le thread de l'interface Android (JNI), lue par le thread du jeu
std::string g_tel;
bool g_telDonne = false;
std::atomic<int> g_telLangue{-1};   // langue déduite du téléphone (RG-01), lue sans verrou à chaque texte ; -1 : pas encore lue
const std::string VIDE;

// journal du banc (RG-07, RG-08)
bool g_journal = false, g_technique = false;
std::string g_ecran;
std::vector<langue::TexteVu> g_vus;
std::vector<langue::Debord> g_debords;

std::string telephone() {   // copie prise sous le verrou (revue #372 : Android peut la réécrire pendant la partie)   // sans valeur donnée par Android : WFS_LANGUE du banc PC, sinon français (RG-09)
    std::lock_guard<std::mutex> v(g_mutexTel);
    if (!g_telDonne) {
        const char* e = std::getenv("WFS_LANGUE");
        g_tel = e && *e ? e : "fr";
        g_telDonne = true;
        g_telLangue = (int)langue::langueDuCode(g_tel);
        WFS_LOG("langue : langue du téléphone « %s » (WFS_LANGUE%s)", g_tel.c_str(), e ? "" : " absente : français");
    }
    return g_tel;
}

bool lireTable(const char* chemin, Table& t) {
    std::vector<uint8_t> d;
    if (!readAsset(chemin, d)) { WFS_ERR("langue : table %s introuvable", chemin); return false; }
    const int illisibles = langue::analyserTable(std::string(d.begin(), d.end()), t);
    if (illisibles) WFS_WARN("langue : %s : %d ligne(s) illisible(s) ou en double", chemin, illisibles);
    WFS_LOG("langue : %s : %zu textes", chemin, t.size());
    return true;
}

} // namespace

namespace langue {

bool charger() {
    if (g_charge) return !g_tables[0].empty() && !g_tables[1].empty();
    g_charge = true;
    const bool fr = lireTable("textes/fr.txt", g_tables[(int)Langue::Francais]);
    const bool en = lireTable("textes/en.txt", g_tables[(int)Langue::Anglais]);
    WFS_LOG("langue : choix %s, téléphone « %s » → %s", cleDeChoix(g_choix), telephone().c_str(), code());
    return fr && en;
}

void definirLangueTelephone(const std::string& c) {
    std::lock_guard<std::mutex> v(g_mutexTel);
    g_tel = c; g_telDonne = true; g_telLangue = (int)langueDuCode(c);
    WFS_LOG("langue : langue du téléphone « %s » (Android)", c.c_str());
}
std::string langueTelephone() { return telephone(); }
void choisir(Choix c) {
    if (c == g_choix) return;
    g_choix = c;
    WFS_LOG("langue : réglage %s → %s", cleDeChoix(c), code());
}
Choix choix() { return g_choix; }
Langue active() {
    if (g_choix == Choix::Francais) return Langue::Francais;
    if (g_choix == Choix::Anglais) return Langue::Anglais;
    const int t = g_telLangue.load(std::memory_order_relaxed);
    return t >= 0 ? (Langue)t : langueDuCode(telephone());   // 1re lecture seulement sous verrou
}
const char* code() { return active() == Langue::Francais ? "fr" : "en"; }

void journalActif(bool oui, const char* ecran) { g_journal = oui; g_ecran = ecran ? ecran : ""; }
bool journalOuvert() { return g_journal; }
void journalTechnique(bool oui) { g_technique = oui; }
void noter(const std::string& t) {
    if (!g_journal) return;
    for (const TexteVu& v : g_vus) if (v.texte == t && v.ecran == g_ecran) return;   // chaque texte une fois par écran
    g_vus.push_back({g_ecran, t, g_technique});
}
void noterDebord(const std::string& t, float largeur, float place) {
    if (!g_journal) return;
    for (const Debord& d : g_debords) if (d.texte == t && d.ecran == g_ecran) return;
    g_debords.push_back({g_ecran, t, cleDuTexte(t), largeur, place});
}
const std::vector<TexteVu>& textesVus() { return g_vus; }
const std::vector<Debord>& debords() { return g_debords; }
void viderJournal() { g_vus.clear(); g_debords.clear(); }

std::string cleDuTexte(const std::string& t) {
    for (const auto& [cle, texte] : g_tables[(int)active()]) if (texte == t || majuscules(texte) == t) return cle;
    return "?";
}

int verifierJournal() {
    int echecs = 0, techniques = 0;
    const bool anglais = active() == Langue::Anglais;
    for (const TexteVu& v : g_vus) {
        if (v.technique) { techniques++; WFS_LOG("textes (WFS-504) : %s : ligne technique non vérifiée : « %s »", v.ecran.c_str(), v.texte.c_str()); continue; }
        const std::string raison = anglais ? phraseFrancaise(v.texte) : "";
        if (raison.empty()) continue;
        echecs++;
        WFS_ERR("textes (WFS-504) : ÉCHEC phrase française (%s) : écran %s, langue %s, clé %s : « %s »", raison.c_str(), v.ecran.c_str(), code(),
                cleDuTexte(v.texte).c_str(), v.texte.c_str());
    }
    for (const Debord& d : g_debords) {
        echecs++;
        WFS_ERR("textes (WFS-504) : ÉCHEC texte qui dépasse : écran %s, langue %s, clé %s : « %s » (%.0f px pour %.0f px, police déjà réduite de 20 %%)",
                d.ecran.c_str(), code(), d.cle.c_str(), d.texte.c_str(), d.largeur, d.place);
    }
    WFS_LOG("textes (WFS-504) : langue %s : %zu texte(s) vus, %d ligne(s) technique(s), %zu dépassement(s), %d échec(s) → %s", code(), g_vus.size(),
            techniques, g_debords.size(), echecs, echecs ? "ROUGE" : "VERT");
    return echecs;
}

} // namespace langue

const std::string& tr(const char* cle) { return trLangue(cle, langue::active()); }

const std::string& trLangue(const char* cle, langue::Langue lg) {
    if (!g_charge) langue::charger();   // filet : les tables sont normalement lues avec la police (écran de chargement)
    const int l = (int)lg;
    const auto it = g_tables[l].find(std::string_view(cle));
    if (it != g_tables[l].end()) return it->second;
    static std::set<std::string, std::less<>> signales;   // trace une fois par clé (recherche sans copie, insertion une fois)
    if (signales.find(std::string_view(cle)) == signales.end() && signales.insert(cle).second) WFS_WARN("langue : clé « %s » absente de la table %s", cle, l ? "en" : "fr");
    // secours (revue #372) : l'autre table (une table absente de l'APK ne vide pas l'écran) ; absente des deux : texte vide
    const Table& autre = g_tables[1 - l];
    const auto se = autre.find(std::string_view(cle));
    return se != autre.end() ? se->second : VIDE;
}

std::string tr(const char* cle, const std::string& v1, const std::string& v2) { return langue::remplir(tr(cle), {v1, v2}); }

} // namespace ui
} // namespace wfs
