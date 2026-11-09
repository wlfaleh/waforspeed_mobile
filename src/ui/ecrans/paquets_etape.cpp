// WaForSpeed — « Mise à jour et installation des paquets » (WFS-306) : machine d'états sans rendu (voir paquets.h). Ne dépend que de
// src/core : testée seule par tests/test_paquets.cpp avec une source factice. Le dessin, le toucher des zones, le bandeau du menu et le
// banc sont dans paquets.cpp.
#include "paquets.h"
#include <algorithm>
#include <cstdio>
#include "../../core/log.h"

namespace wfs {
namespace ecrans {
namespace {
constexpr double FIN_AFFICHEE_S = 0.6;   // « Ville installée » avant la suite du chargement
std::string hexa(uint64_t v) { char b[17]; std::snprintf(b, sizeof b, "%016llx", (unsigned long long)v); return b; }
} // namespace

std::string texteFormats(const std::vector<carte::FormatTexture>& f) {
    std::string s;
    for (carte::FormatTexture x : f) s += std::string(s.empty() ? "" : ",") + carte::nomFormat(x);
    return s;
}
int versionApk(const char* v) {
    const std::string s = v ? v : "";
    const size_t p = s.rfind('.');
    if (p == std::string::npos) return 0;
    int n = 0;
    for (size_t i = p + 1; i < s.size() && s[i] >= '0' && s[i] <= '9' && n < 100000000; i++) n = n * 10 + (s[i] - '0');
    return n;
}
std::string texteMo(long long o) { return std::to_string(std::max(1LL, (o + 524288) / 1048576)) + " Mo"; }

bool EtapePaquets::demarrerAvec(SourcePaquets& s, const std::string& c, const std::vector<carte::FormatTexture>& formats, uint64_t e, int apk, double t) {
    source_ = &s; carte_ = c; fenetre_ = true; installe_ = false; secours_ = false; menu_ = false;
    empreinte_ = hexa(e); formats_ = texteFormats(formats); apk_ = apk;
    for (carte::FormatTexture f : formats)   // RG-01 : paquet juste et à jour déjà là → rien de plus
        if (carte::verifier_carte(carte::dossierPaquets() + "/" + carte::nomFichier(c, f), {c, f, e}) == carte::Verdict::Bon) {
            WFS_LOG("paquets (WFS-306) : %s présent et vérifié, pas de téléchargement", carte::nomFichier(c, f).c_str());
            etat_ = Etat::Ferme;
            return false;
        }
    etat_ = Etat::Recherche; t0_ = maintenant_ = t; recus_ = taille_ = 0; tDebutTelechargement_ = tFini_ = -1; message_.clear(); doigt_ = -1; essaisRotation_ = 0;
    s.chercher(c, formats_, empreinte_, apk);
    WFS_LOG("paquets (WFS-306) : %s absent ou ancien, recherche de la liste (formats %s, empreinte %s, APK %d)", c.c_str(), formats_.c_str(), empreinte_.c_str(), apk);
    return true;
}

void EtapePaquets::finir(bool avecPaquet, const char* raison) {
    if (!avecPaquet) secours_ = true;
    tFini_ = maintenant_;
    etat_ = menu_ ? Etat::Ferme : Etat::Fini;   // au menu : la proposition se referme seulement
    menu_ = false;
    WFS_LOG("paquets (WFS-306) : fin de l'étape, %s (%s)", avecPaquet ? "paquet installé" : "carte du code (secours)", raison);
}

void EtapePaquets::avancer(double t) {
    maintenant_ = t;
    if (!source_ || !ouvert()) return;
    long long recus = 0, taille = 0;
    const SourcePaquets::Phase ph = source_->phase(recus, taille);
    switch (etat_) {
    case Etat::Recherche:
        if (ph == SourcePaquets::ABSENTE) { finir(false, "pas de liste ou rien pour ce téléphone"); break; }
        if (ph == SourcePaquets::PRETE) {
            source_->proposition(nom_, formatPropose_);
            taille_ = taille;
            reseau_ = source_->reseau();
            const long long libre = source_->placeLibre(), besoin = taille + taille / 10;   // RG-07 (le reste exact est revu par Java)
            if (libre >= 0 && libre < besoin) {
                message_ = "Place insuffisante : " + texteMo(besoin) + " nécessaires, " + texteMo(libre) + " libres";
                etat_ = Etat::Place;
                WFS_LOG("paquets (WFS-306) : %s", message_.c_str());
                break;
            }
            etat_ = Etat::Proposition;
            WFS_LOG("paquets (WFS-306) : proposition %s (%s, %s), réseau %s", nom_.c_str(), formatPropose_.c_str(), texteMo(taille_).c_str(),
                    reseau_ == 1 ? "Wi-Fi" : reseau_ == 2 ? "données mobiles" : "aucun");
        }
        break;
    case Etat::Proposition:
        reseau_ = source_->reseau();
        break;
    case Etat::Telechargement:
        recus_ = recus; if (taille > 0) taille_ = taille;
        coupure_ = ph == SourcePaquets::COUPURE;
        if (ph == SourcePaquets::FINI) {
            if (tFini_ < 0) { tFini_ = t; installe_ = true; WFS_LOG("paquets (WFS-306) : %s installée (%s)", nom_.c_str(), texteMo(taille_).c_str()); }
            if (t - tFini_ >= FIN_AFFICHEE_S) finir(true, "téléchargé et vérifié");
        } else if (ph == SourcePaquets::SHA_FAUX || ph == SourcePaquets::ERREUR) {
            message_ = ph == SourcePaquets::SHA_FAUX ? "Le paquet téléchargé est abîmé (deux essais)." : "Le téléchargement a échoué.";
            etat_ = Etat::Echec;
            WFS_LOG("paquets (WFS-306) : échec : %s", message_.c_str());
        } else if (ph == SourcePaquets::PLACE) {
            message_ = "Place insuffisante : " + texteMo(taille_ + taille_ / 10) + " nécessaires, " + texteMo(source_->placeLibre()) + " libres";
            etat_ = Etat::Place;
        }
        break;
    default: break;
    }
}

void EtapePaquets::installer() {
    if (!source_ || (etat_ != Etat::Proposition && etat_ != Etat::Echec)) return;
    WFS_LOG("paquets (WFS-306) : « %s » (%s)%s", etat_ == Etat::Echec ? "Réessayer" : "Installer", reseau_ == 1 ? "Wi-Fi" : reseau_ == 2 ? "données mobiles" : "sans réseau",
            menu_ ? ", depuis le menu" : "");
    source_->installer();
    if (menu_) { etat_ = Etat::Ferme; menu_ = false; return; }   // au menu : le bandeau montre la progression ; la carte sert au prochain lancement
    etat_ = Etat::Telechargement; tDebutTelechargement_ = maintenant_; recusDebut_ = recus_; tFini_ = -1; coupure_ = false;
}
void EtapePaquets::plusTard() {
    if (!ouvert()) return;
    if (etat_ == Etat::Telechargement && source_) source_->arreter();   // le .part est gardé : reprise à la prochaine installation
    finir(false, "« Plus tard »");
}
void EtapePaquets::continuer() { if (etat_ == Etat::Place) finir(false, "place insuffisante"); }

bool EtapePaquets::ouvrirProposition(double t) {
    if (!source_ || ouvert()) return false;
    long long recus = 0, taille = 0;
    if (source_->phase(recus, taille) != SourcePaquets::PRETE || !source_->proposition(nom_, formatPropose_)) return false;
    taille_ = taille; reseau_ = source_->reseau(); maintenant_ = t0_ = t; doigt_ = -1; menu_ = true;
    etat_ = Etat::Proposition;
    WFS_LOG("paquets (WFS-306) : proposition ouverte depuis le bandeau du menu (%s, %s)", texteMo(taille_).c_str(), reseau_ == 2 ? "données mobiles" : reseau_ == 1 ? "Wi-Fi" : "sans réseau");
    return true;
}

} // namespace ecrans
} // namespace wfs
