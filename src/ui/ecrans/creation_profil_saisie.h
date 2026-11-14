// WaForSpeed — écran de création du profil au 1er lancement (WFS-481, épic E17, zone E8) : la saisie, sans rendu ni fichier
// (testée par tests/test_creation_profil.cpp). Le dessin, les touchers et le banc sont dans creation_profil.cpp.
//   RG-02 : deux champs (Pseudo, Plaque) ; plaque proposée d'après le pseudo (profil::plaqueProposee, WFS-480 RG-03) et remise
//           à jour tant que le joueur ne l'a pas touchée ; « C'est parti » actif seulement si le pseudo et la plaque sont valides.
//   RG-03 : clavier du jeu, AZERTY en français, QWERTY en anglais : chiffres, lettres, espace, « - », « _ », effacer, majuscules ;
//           sur la plaque, seulement A-Z, 0-9 et « - » (les autres touches sont grisées et sans effet).
//   RG-04 : erreur sous le champ dès la frappe (clé de la table de textes) ; aucune erreur ne répète le mot refusé.
#pragma once
#include <cstdint>
#include <string>
#include "../../game/profil.h"
#include "../langue_regles.h"

namespace wfs::ecrans::creation {

enum class Champ { Pseudo, Plaque };

// une touche du clavier : un caractère (lettres en minuscules), ou une touche spéciale
struct Touche {
    enum Genre { Caractere, Majuscules, Effacer, Espace } genre = Caractere;
    char c = 0;          // Caractere : 'a'..'z', '0'..'9', '-', '_'
    int ligne = 0;       // 0 = chiffres, 1 à 3 = lettres, 4 = espace
    float colonne = 0;   // position en largeurs de touche (0 à 9)
    float largeur = 1;   // en largeurs de touche
};
constexpr int COLONNES = 10, LIGNES = 5;
// RG-03 : 41 touches dans l'ordre de dessin (10 chiffres, 3 lignes de 10, espace) ; anglais → QWERTY, français → AZERTY
constexpr int NB_TOUCHES = 41;
const Touche* clavier(ui::langue::Langue l);
// RG-03 : touche utilisable dans ce champ (plaque : lettres, chiffres, « - », effacer seulement)
bool toucheActive(const Touche& t, Champ champ);

// WFS-545 : filtre des appuis d'un clavier du jeu (« Crée ton pilote » et code du salon du duel), sans rendu ni horloge.
//   RG-01 : un appui = un caractère : Android envoie chaque mouvement d'un doigt posé avec down = true ; seul le poser d'un
//           doigt (identifiant pas encore posé) est un nouvel appui ; ses mouvements n'écrivent rien jusqu'au lever.
//   RG-02 : après chaque caractère accepté, 150 ms pendant lesquelles aucun autre caractère n'est accepté (rebond du doigt).
class FiltreAppuis {
public:
    static constexpr double DELAI = 0.150;   // s (RG-02, demande de l'utilisateur : ≈ 6 touches par seconde restent possibles)
    // toucher reçu : vrai seulement au poser d'un doigt qui n'était pas posé (mouvement ou lever : faux ; le lever oublie le doigt)
    bool poser(int id, bool down);
    // caractère (ou effacement) demandé à l'heure t (s) : vrai s'il est accepté, faux pendant les 150 ms qui suivent le précédent
    bool accepter(double t);
    void oublier() { nb_ = 0; dernier_ = -1e9; }   // ouverture de l'écran : aucun doigt posé, aucun délai en cours
private:
    static constexpr int DOIGTS = 10;   // doigts suivis à la fois (au-delà, un doigt de plus n'écrit rien jusqu'à un lever)
    int doigts_[DOIGTS] = {};
    int nb_ = 0;
    double dernier_ = -1e9;
};

class Saisie {
public:
    // pseudoPropose : pseudo actuel d'un joueur déjà installé (Profil::pseudoPropose, WFS-480 RG-05), vide sinon ;
    // graine : hasard de la plaque de secours « WFS-123 » (même graine = même plaque proposée pendant tout l'écran)
    void ouvrir(const std::string& pseudoPropose, uint32_t graine);
    // WFS-482 (RG-07) : « Modifier » de l'écran Profil : pseudo et plaque actuels ; la plaque ne suit plus le pseudo (le joueur
    // l'a déjà choisie : changer de pseudo ne doit pas remplacer sa plaque) ; mêmes règles et même clavier ensuite
    void modifier(const std::string& pseudo, const std::string& plaque);
    void choisirChamp(Champ c) { champ_ = c; plein_ = false; }
    // RG-03 : appui sur une touche du clavier (sans effet si la touche n'est pas active dans le champ)
    void appuyer(const Touche& t);

    const std::string& pseudo() const { return pseudo_; }
    const std::string& plaque() const { return plaque_; }
    Champ champ() const { return champ_; }
    bool majuscules() const { return maj_; }
    bool plaqueTouchee() const { return plaqueTouchee_; }
    profil::Refus refusPseudo() const { return profil::verifierPseudo(pseudo_); }
    profil::Refus refusPlaque() const { return profil::verifierPlaque(plaque_); }
    // RG-02 : « C'est parti » actif
    bool valide() const { return refusPseudo() == profil::Refus::Aucun && refusPlaque() == profil::Refus::Aucun; }
    // RG-04 : clé du message d'erreur sous le champ (assets/textes), nullptr s'il n'y a rien à dire (champ vide encore jamais
    // rempli, ou texte accepté) ; le message ne contient jamais le texte saisi
    const char* erreur(Champ c) const;

private:
    void proposerPlaque();
    std::string pseudo_, plaque_;
    Champ champ_ = Champ::Pseudo;
    bool maj_ = true;              // majuscule pour la prochaine lettre (une seule lettre, comme le clavier d'un téléphone)
    bool plaqueTouchee_ = false;   // RG-02 : une touche a été appuyée dans la plaque : elle ne suit plus le pseudo
    bool plein_ = false;           // touche appuyée alors que le champ choisi est plein : « 16 caractères au plus »
    bool pseudoEcrit_ = false;     // le pseudo a déjà eu un caractère (champ vidé ensuite : « 3 caractères au moins »)
    uint32_t graine_ = 1;
};

} // namespace wfs::ecrans::creation
