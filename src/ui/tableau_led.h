// WaForSpeed — WFS-561 (épic E23, R19 maquette 11) : tableau d'affichage à LED ambre des résultats, partie calcul seule
// (aucun Vulkan : testée par tests/test_tableau_led.cpp). Le modèle 3D du panneau et son dessin sont dans resultats_parking.*.
//
// Le vrai objet (RG-02) : panneau d'affichage à LED d'extérieur et de parking, pas de 16 mm (« P16 »), une LED ambre AlInGaP
// par point (longueur d'onde dominante 590 nm, comme les panneaux d'information des gares et des parcs de stationnement) ;
// police matricielle 5 × 7 points, la police des afficheurs à matrice de points (caractères des contrôleurs HD44780 et des
// panneaux de gare), avec deux rangées au-dessus des capitales pour les accents (É, È, À…).
// Matrice : 186 colonnes × 88 rangées → surface active 2,976 × 1,408 m ; caisson 3,10 × 1,52 m (≈ 3 × 1,5 m demandé).
// Mise en page : 30 caractères de 6 points (5 + 1 d'espace) par ligne, 3 points de marge de chaque côté ; 6 lignes (en-tête + 5 pilotes) au pas de 14 rangées
// (caractère de 10 rangées, accents compris, + 4 rangées d'interligne).
#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace wfs::led {

inline constexpr int COLONNES = 186, RANGEES = 88;   // points de la matrice
inline constexpr float PAS = 0.016f;                 // pas des points (m), P16
inline constexpr int CAR_L = 6, CAR_H = 10;          // cellule d'un caractère (points) : 5 + 1 d'espace ; 2 rangées d'accent + 7 + 1
inline constexpr int NB_CAR = 30;                    // 30 caractères par ligne
inline constexpr int MARGE_G = (COLONNES - NB_CAR * CAR_L + 1) / 2;   // 3 points libres à gauche et à droite (avis de l'agent 3D : marge au cadre)
inline constexpr int LIGNE_PAS = 14, NB_LIGNES = 6;  // pas des lignes (rangées), en-tête + 5 pilotes
inline constexpr int MARGE_HAUT = (RANGEES - (NB_LIGNES - 1) * LIGNE_PAS - (CAR_H - 1)) / 2;   // rangées libres au-dessus

// colonnes du tableau (en caractères) : POS, PILOTE, TEMPS, ÉCART (maquette 11)
struct Colonne { int debut, largeur; };
inline constexpr Colonne COL_POS{0, 3}, COL_PILOTE{4, 12}, COL_TEMPS{17, 7}, COL_ECART{25, 5};

struct Point { int16_t c, r; };   // point allumé : colonne (0 à gauche), rangée (0 en haut)

// texte d'une ligne du tableau (déjà traduit) ; moi : la ligne du joueur
struct Ligne { std::string pos, pilote, temps, ecart; bool moi = false; };

// glyphe d'un caractère (point de code Unicode) : 9 rangées de 5 bits (bit 4 = colonne de gauche), rangées 0 et 1 = accent ;
// faux si le caractère n'a pas de glyphe (il est alors remplacé par sa lettre sans accent, ou sauté : voir poserTexte)
bool glyphe(char32_t c, uint8_t rangees[9]);
// capitales des lettres latines (é → É, ß → SS non : laissé), autres caractères inchangés
char32_t majuscule(char32_t c);
// découpe UTF-8 (octet invalide : sauté)
std::vector<char32_t> pointsDeCode(std::string_view utf8);

// pose un texte en capitales dans une zone de `largeur` caractères qui commence au caractère `debut` de la ligne `ligne`
// (0 = en-tête) ; texte trop long : coupé au dernier caractère qui tient. Ajoute les points allumés à `sortie` et rend le
// nombre de caractères posés.
int poserTexte(std::string_view utf8, int debut, int largeur, int ligne, std::vector<Point>& sortie);

// tout le tableau : en-tête (4 titres de colonne déjà traduits) et 5 lignes au plus (lignes en trop ignorées, lignes vides
// cachées : RG-01 / RG-04) ; rend les points allumés (vidé d'abord)
void composer(const std::string (&entete)[4], const std::vector<Ligne>& lignes, std::vector<Point>& sortie);

// rangée du haut (accents compris) de la ligne `ligne`
inline int rangeeLigne(int ligne) { return MARGE_HAUT + ligne * LIGNE_PAS; }

} // namespace wfs::led
