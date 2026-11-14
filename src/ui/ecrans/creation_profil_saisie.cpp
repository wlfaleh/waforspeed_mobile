// WaForSpeed — saisie de l'écran de création du profil (WFS-481) : voir creation_profil_saisie.h.
#include "creation_profil_saisie.h"
#include "../../core/math.h"   // Mulberry32

namespace wfs::ecrans::creation {
namespace {

// RG-03 : clavier de 5 lignes de 10 touches. Ligne 3 : majuscules à gauche, effacer à droite ; ligne 4 : espace au milieu.
// AZERTY (français) et QWERTY (anglais) gardent « - » et « _ » sur la ligne 3 (2 dernières places avant effacer en AZERTY,
// « - » en fin de ligne 2 et « _ » avant effacer en QWERTY, où la ligne 2 n'a que 9 lettres).
struct Table { Touche t[NB_TOUCHES]; };
Table construire(const char* l1, const char* l2, const char* l3) {
    Table tb;
    int n = 0;
    auto ligne = [&](const char* s, int y, float x0) { for (int i = 0; s[i]; i++) tb.t[n++] = {Touche::Caractere, s[i], y, x0 + i, 1}; };
    ligne("1234567890", 0, 0);
    ligne(l1, 1, 0);
    ligne(l2, 2, 0);
    tb.t[n++] = {Touche::Majuscules, 0, 3, 0, 1};
    ligne(l3, 3, 1);
    tb.t[n++] = {Touche::Effacer, 0, 3, 9, 1};
    tb.t[n++] = {Touche::Espace, ' ', 4, 2, 6};
    return tb;
}
const Table AZERTY = construire("azertyuiop", "qsdfghjklm", "wxcvbn-_");
const Table QWERTY = construire("qwertyuiop", "asdfghjkl-", "zxcvbnm_");

// hasard de la plaque de secours : Mulberry32 repris de la graine de l'écran à chaque proposition (plaque stable)
Mulberry32 g_hasard(1);
uint32_t hasard() { return (uint32_t)(g_hasard.next() * 4294967296.0); }

bool estLettre(char c) { return c >= 'a' && c <= 'z'; }
bool estChiffre(char c) { return c >= '0' && c <= '9'; }

} // namespace

bool FiltreAppuis::poser(int id, bool down) {
    for (int i = 0; i < nb_; i++)
        if (doigts_[i] == id) {
            if (!down) doigts_[i] = doigts_[--nb_];   // lever : le doigt est oublié
            return false;                             // mouvement d'un doigt déjà posé : pas un nouvel appui (RG-01)
        }
    if (!down || nb_ >= DOIGTS) return false;
    doigts_[nb_++] = id;
    return true;
}

bool FiltreAppuis::accepter(double t) {
    // t < dernier_ : horloge revenue en arrière (nouvelle partie, banc) : on accepte et on repart de t
    if (t >= dernier_ && t - dernier_ < DELAI) return false;
    dernier_ = t;
    return true;
}

const Touche* clavier(ui::langue::Langue l) { return l == ui::langue::Langue::Francais ? AZERTY.t : QWERTY.t; }

bool toucheActive(const Touche& t, Champ champ) {
    if (champ == Champ::Pseudo || t.genre == Touche::Effacer) return true;
    // plaque : lettres (mises en majuscules), chiffres et « - » ; ni espace, ni « _ », ni majuscules (toujours en capitales)
    return t.genre == Touche::Caractere && (estLettre(t.c) || estChiffre(t.c) || t.c == '-');
}

void Saisie::ouvrir(const std::string& pseudoPropose, uint32_t graine) {
    graine_ = graine;
    pseudo_ = pseudoPropose.substr(0, profil::PSEUDO_MAX);
    plaque_.clear();
    champ_ = Champ::Pseudo;
    maj_ = pseudo_.empty();
    plaqueTouchee_ = plein_ = false;
    pseudoEcrit_ = !pseudo_.empty();
    proposerPlaque();
}

void Saisie::modifier(const std::string& pseudo, const std::string& plaque) {
    pseudo_ = pseudo.substr(0, profil::PSEUDO_MAX);
    plaque_ = plaque.substr(0, profil::PLAQUE_MAX);
    champ_ = Champ::Pseudo;
    maj_ = pseudo_.empty();
    plaqueTouchee_ = true;   // plaque gardée telle quelle, même si le pseudo change
    plein_ = false;
    pseudoEcrit_ = !pseudo_.empty();
}

void Saisie::proposerPlaque() {
    if (plaqueTouchee_) return;
    g_hasard = Mulberry32(graine_);
    plaque_ = profil::plaqueProposee(pseudo_, hasard);
}

void Saisie::appuyer(const Touche& t) {
    if (!toucheActive(t, champ_)) return;
    std::string& s = champ_ == Champ::Pseudo ? pseudo_ : plaque_;
    const size_t max = champ_ == Champ::Pseudo ? (size_t)profil::PSEUDO_MAX : (size_t)profil::PLAQUE_MAX;
    if (champ_ == Champ::Plaque) plaqueTouchee_ = true;
    switch (t.genre) {
        case Touche::Majuscules: maj_ = !maj_; return;
        case Touche::Effacer:
            plein_ = false;
            if (!s.empty()) s.pop_back();
            if (champ_ == Champ::Pseudo && s.empty()) maj_ = true;   // champ vidé : majuscule de nouveau armée (clavier d'un téléphone)
            break;
        case Touche::Caractere:
        case Touche::Espace: {
            if (s.size() >= max) { plein_ = true; return; }
            char c = t.c;
            if (estLettre(c) && (champ_ == Champ::Plaque || maj_)) c = (char)(c - 'a' + 'A');
            if (champ_ == Champ::Pseudo && estLettre(t.c)) maj_ = false;   // majuscule pour une seule lettre
            s += c;
            if (champ_ == Champ::Pseudo) pseudoEcrit_ = true;
            break;
        }
    }
    if (champ_ == Champ::Pseudo) proposerPlaque();
}

const char* Saisie::erreur(Champ c) const {
    using profil::Refus;
    if (c == Champ::Pseudo) {
        if (plein_ && champ_ == Champ::Pseudo) return "creation.erreur.pseudo_long";
        switch (refusPseudo()) {
            case Refus::Aucun: return nullptr;
            case Refus::TropCourt: return pseudoEcrit_ ? "creation.erreur.pseudo_court" : nullptr;
            case Refus::TropLong: return "creation.erreur.pseudo_long";
            case Refus::CaractereInterdit: return "creation.erreur.espaces";   // seul cas possible au clavier du jeu : deux espaces de suite
            case Refus::MotInterdit: return "creation.erreur.mot";
        }
        return nullptr;
    }
    if (plein_ && champ_ == Champ::Plaque) return "creation.erreur.plaque_long";
    switch (refusPlaque()) {
        case Refus::Aucun: return nullptr;
        case Refus::TropCourt: return "creation.erreur.plaque_vide";
        case Refus::TropLong: return "creation.erreur.plaque_long";
        case Refus::CaractereInterdit: return "creation.erreur.tiret";   // seul cas possible : tiret au début, à la fin ou en double
        case Refus::MotInterdit: return "creation.erreur.mot";
    }
    return nullptr;
}

} // namespace wfs::ecrans::creation
