// WaForSpeed — « Mise à jour et installation des paquets » (WFS-306) : machine d'états sans rendu (voir paquets.h). Ne dépend que de
// src/core : testée seule par tests/test_paquets.cpp avec une source factice. Le dessin, le toucher des zones, le bandeau du menu et le
// banc sont dans paquets.cpp.
#include "paquets.h"
#include <algorithm>
#include <filesystem>
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

// ---------------------------------------------------------------- WFS-307 : paquets de chaque carte (choix du terrain)
void CartesPaquets::lier(SourcePaquets* s, const std::vector<carte::FormatTexture>& f, int apk) { source_ = s; formats_ = f; apk_ = apk; }

CartesPaquets::Suivi* CartesPaquets::suivi(const std::string& c) {
    for (auto& [n, s] : cartes_) if (n == c) return &s;
    return nullptr;
}

void CartesPaquets::chercher(const std::vector<std::pair<std::string, uint64_t>>& cartes) {
    std::string e;
    for (const auto& [c, emp] : cartes) {
        if (!suivi(c)) cartes_.push_back({c, Suivi{}});
        suivi(c)->empreinte = emp; suivi(c)->verifie = false;
        e += std::string(e.empty() ? "" : ",") + c + ":" + hexa(emp);
    }
    if (!source_) return;
    source_->chercherTout(texteFormats(formats_), e, apk_);
    cherche_ = true;
    WFS_LOG("paquets (WFS-307) : liste lue pour les cartes %s", e.c_str());
}

void CartesPaquets::suivre(const std::string& c, uint64_t emp) {
    if (!suivi(c)) cartes_.push_back({c, Suivi{}});
    suivi(c)->empreinte = emp; suivi(c)->verifie = false;
}

// paquet juste sur le téléphone (un des formats lus par la carte graphique) ; « ancien » : un fichier est là mais refusé (autre génération)
void CartesPaquets::verifier(const std::string& c, Suivi& s) {
    s.installee = s.ancien = false; s.surDisque = 0;
    for (carte::FormatTexture f : formats_) {
        const std::string chemin = carte::dossierPaquets() + "/" + carte::nomFichier(c, f);
        std::error_code ec;
        if (!std::filesystem::exists(chemin, ec)) continue;
        if (carte::verifier_carte(chemin, {c, f, s.empreinte}) == carte::Verdict::Bon) { s.installee = true; s.surDisque = (long long)std::filesystem::file_size(chemin, ec); break; }
        s.ancien = true;
    }
    s.verifie = true;
}

EtatCarte CartesPaquets::etat(const std::string& c) {
    EtatCarte e; e.nom = c;
    Suivi* s = suivi(c);
    if (!s) return e;
    if (!s->verifie) verifier(c, *s);
    long long recus = 0, taille = 0;
    const SourcePaquets::Phase ph = source_ ? source_->phaseCarte(c, recus, taille) : SourcePaquets::ABSENTE;
    std::string nom, fmt;
    if (source_ && source_->propositionCarte(c, nom, fmt) && !nom.empty()) e.nom = nom;
    e.recus = recus; e.taille = taille; e.surDisque = s->surDisque;
    switch (ph) {
    case SourcePaquets::ATTENTE: e.etat = EtatCarte::Attente; return e;
    case SourcePaquets::TELECHARGE: e.etat = EtatCarte::Telechargement; return e;
    case SourcePaquets::COUPURE: e.etat = EtatCarte::Coupure; return e;
    case SourcePaquets::SHA_FAUX: case SourcePaquets::ERREUR: case SourcePaquets::PLACE: if (!s->installee) { e.etat = EtatCarte::Echec; return e; } break;
    default: break;
    }
    if (s->installee) { e.etat = EtatCarte::Installee; return e; }
    if (ph == SourcePaquets::LISTE || ph == SourcePaquets::RIEN) { e.etat = cherche_ && ph == SourcePaquets::LISTE ? EtatCarte::Recherche : EtatCarte::SansPaquet; return e; }
    if (ph == SourcePaquets::PRETE || ph == SourcePaquets::FINI) e.etat = s->ancien ? EtatCarte::MiseAJour : EtatCarte::Disponible;   // FINI puis fichier effacé
    return e;
}

void CartesPaquets::avancer() {
    if (!source_) return;
    for (auto& [c, s] : cartes_) {
        long long r = 0, t = 0;
        const SourcePaquets::Phase ph = source_->phaseCarte(c, r, t);
        if (ph == SourcePaquets::FINI && s.phase != SourcePaquets::FINI && s.phase != SourcePaquets::RIEN) {   // vient de finir pendant le jeu
            s.verifie = false;
            if (std::find(changees_.begin(), changees_.end(), c) == changees_.end()) changees_.push_back(c);
            pretes_.push_back(c);
            WFS_LOG("paquets (WFS-307) : %s prête (paquet téléchargé et vérifié), relue à la prochaine course", c.c_str());
        }
        s.phase = ph;
    }
}

bool CartesPaquets::telecharger(const std::string& c, std::string& message) {
    message.clear();
    if (!source_ || !suivi(c)) { message = "Paquet indisponible"; return false; }
    if (source_->reseau() == 0) { message = "Connexion nécessaire"; WFS_LOG("paquets (WFS-307) : %s : %s", c.c_str(), message.c_str()); return false; }   // RG-05
    long long r = 0, t = 0;
    source_->phaseCarte(c, r, t);
    const long long libre = source_->placeLibre(), besoin = t + t / 10;   // RG-07 de WFS-306
    if (t > 0 && libre >= 0 && libre < besoin) { message = "Place insuffisante : " + texteMo(besoin) + " nécessaires, " + texteMo(libre) + " libres"; WFS_LOG("paquets (WFS-307) : %s : %s", c.c_str(), message.c_str()); return false; }
    if (!source_->installerCarte(c)) { message = "Paquet indisponible"; return false; }
    suivi(c)->phase = SourcePaquets::ATTENTE;
    WFS_LOG("paquets (WFS-307) : %s : « Télécharger » (%s)", c.c_str(), texteMo(t).c_str());
    return true;
}
void CartesPaquets::annuler(const std::string& c) { if (source_) { source_->arreterCarte(c); WFS_LOG("paquets (WFS-307) : %s : téléchargement annulé (.part gardé)", c.c_str()); } }

bool CartesPaquets::supprimer(const std::string& c, long long& liberes) {
    liberes = 0;
    Suivi* s = suivi(c);
    if (!s) return false;
    long long r = 0, t = 0;
    const SourcePaquets::Phase ph = source_ ? source_->phaseCarte(c, r, t) : SourcePaquets::ABSENTE;
    if (ph == SourcePaquets::TELECHARGE || ph == SourcePaquets::COUPURE || ph == SourcePaquets::ATTENTE) return false;
    for (carte::FormatTexture f : carte::FORMATS)
        for (const std::string& suffixe : {std::string(), std::string(".part")}) {
            const std::string chemin = carte::dossierPaquets() + "/" + carte::nomFichier(c, f) + suffixe;
            std::error_code ec;
            const auto n = std::filesystem::file_size(chemin, ec);
            if (!ec && std::filesystem::remove(chemin, ec)) liberes += (long long)n;
        }
    s->verifie = false;
    if (std::find(changees_.begin(), changees_.end(), c) == changees_.end()) changees_.push_back(c);
    if (source_) source_->supprimee(c);
    WFS_LOG("paquets (WFS-307) : %s supprimée, %s libérés", c.c_str(), texteMo(liberes).c_str());
    return true;
}

bool CartesPaquets::changee(const std::string& c) const { return std::find(changees_.begin(), changees_.end(), c) != changees_.end(); }
void CartesPaquets::prise(const std::string& c) { changees_.erase(std::remove(changees_.begin(), changees_.end(), c), changees_.end()); }
std::vector<std::string> CartesPaquets::pretes() { std::vector<std::string> v; v.swap(pretes_); return v; }
void CartesPaquets::ralentir(bool oui) {
    if (oui == ralenti_ || !source_) return;
    ralenti_ = oui; source_->ralentir(oui);
    WFS_LOG("paquets (WFS-307) : téléchargement %s", oui ? "ralenti (course sous 30 images par seconde)" : "à pleine vitesse");
}
CartesPaquets& cartesPaquets() { static CartesPaquets c; return c; }

} // namespace ecrans
} // namespace wfs
