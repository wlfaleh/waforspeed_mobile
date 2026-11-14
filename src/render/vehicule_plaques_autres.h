// WaForSpeed — WFS-484 (épic E17, zone E10) : plaques des autres voitures (trafic, adversaires de la Course à 5, police,
// duel, fantôme), dans le même style que la plaque du joueur (WFS-483, vehicule_plaque.h).
// RG-01 : même plaque : mêmes maillages (porte-plaque dont la jupe suit la caisse de chaque voiture, plaque en aluminium à bords
//         roulés, 2 vis), mêmes matériaux, même place par modèle (vehicule_plaque_defs.cpp), plaque arrière éclairée la nuit.
// RG-02 : texte au hasard, 7 caractères (« AB123CD » ou « AB-1234 »), sans I, O ni U (lisibilité, comme les plaques réelles),
//         jamais un mot interdit (filtre de WFS-480), jamais la plaque du joueur, jamais deux fois le même texte dans l'atlas ;
//         tiré au chargement de la carte : le même numéro garde la même plaque toute la course, à l'avant comme à l'arrière.
// RG-03 : duel : la plaque envoyée par l'adversaire (champ « pl » de la présence, filtré à la réception) ; sinon une plaque au
//         hasard ; fantôme : la plaque du joueur, translucide comme le fantôme.
// RG-04 : police : même style de plaque.
// RG-05 : une seule texture (atlas de 64 plaques, couleur + normales) préparée au chargement de la carte ; chaque case a son
//         maillage de plaque (uv de la case) ; rien n'est créé pendant la course (seulement entre deux courses : plaque du
//         joueur changée, plaques du duel reçues dans le salon) ; en Basse : version simple, trafic sans plaque au-delà de 40 m.
#pragma once
#include <array>
#include <cstdint>
#include <string>
#include "../core/math.h"

namespace wfs {
class Renderer;
}

namespace wfs::plaques_autres {

// ---- atlas (RG-05) : 8 × 8 cases de 256 × 64 px ; la plaque (256 × 54 px) est centrée en hauteur, 5 px de marge
// (bord de la case recopié) : les mipmaps ne mélangent pas deux plaques voisines
constexpr int CASES = 64, COLONNES = 8, LIGNES = 8;
constexpr int CASE_L = 256, CASE_H = 64, PLAQUE_PX_H = 54, MARGE_PX = (CASE_H - PLAQUE_PX_H) / 2;
constexpr int ATLAS_L = CASE_L * COLONNES, ATLAS_H = CASE_H * LIGNES;   // 2048 × 512
// répartition des cases
constexpr int TRAFIC_0 = 0, TRAFIC_N = 44;        // voitures du trafic : numéro % 44
constexpr int COURSE5_0 = 44, COURSE5_N = 4;      // 4 adversaires de la Course à 5
constexpr int POLICE_0 = 48, POLICE_N = 8;        // voitures de police : numéro % 8
constexpr int DUEL_0 = 56, DUEL_N = 7;            // adversaires du duel (7 au plus)
constexpr int FANTOME = 63;                       // fantôme : plaque du joueur
constexpr float TRAFIC_BASSE_MAX_M = 40.f;        // RG-05 : en Basse, plaques du trafic absentes au-delà de 40 m

// ---- tirage (sans GPU, tests/test_plaques_autres.cpp)
constexpr int TEXTE_N = 7;
// texte au hasard (RG-02) : « AB123CD » ou « AB-1234 » (1 fois sur 4), sans I, O, U ; refait tant qu'il contient un mot
// interdit (profil::contientMotInterdit) ou qu'il est égal à la plaque du joueur (comparée en majuscules)
std::string tirer(Mulberry32& h, const std::string& plaqueJoueur);
// textes des cases : tirés depuis la graine (tous différents) ; case FANTOME = plaque du joueur ; DUEL_* : au hasard
// (remplacés par celles du salon si elles arrivent avant le départ)
std::array<std::string, CASES> tirerAtlas(uint32_t graine, const std::string& plaqueJoueur);
// graine d'une carte (FNV-1a de son identifiant) : mêmes plaques à chaque chargement de la carte (bancs reproductibles)
uint32_t graineCarte(const char* idCarte);
// plaque reçue du relais (RG-03) : acceptée si profil::verifierPlaque l'accepte (1 à 8 caractères, mots interdits), en
// majuscules ; sinon vide (plaque au hasard)
std::string filtrerPlaqueRecue(const std::string& brut);
// u, v (0..1) de la plaque WFS-483 → u, v de la case dans l'atlas
void uvCase(int cas, float u, float v, float& ua, float& va);

// ---- dessin (thread principal, vehicule_plaques_autres.cpp)
enum class Genre { Trafic, Adversaire, Police, Fantome };
void preparer(Renderer& r, const char* idCarte);   // au chargement de la carte (Game::init) : atlas, maillages, porte-plaques
// entre deux courses (hors écran Course) : refait les cases changées (plaque du joueur, plaques du duel) ; rien sinon
void entreCourses(Renderer& r, bool enCourse);
// duel (RG-03) : plaque reçue d'un adversaire du salon (champ « pl » brut, filtré ici) ; chaque adversaire garde sa case
// (DUEL_N au plus ; au-delà, la case vue il y a le plus longtemps est reprise). Prise en compte au prochain entreCourses hors
// course (jamais pendant la course) ; caseDuel : case de cet adversaire (inconnu : une case du duel au hasard de son id)
void plaqueDuel(const std::string& id, const std::string& brut);
int caseDuel(const std::string& id);
// dessine les 2 plaques de la voiture idVoiture posée en monde ; echelle : échelle du modèle dessiné (trafic : longueur du
// gabarit / longueur du modèle) ; la place est mise à l'échelle, la plaque garde ses vraies mesures ; cas : case de l'atlas
void dessiner(Renderer& r, const char* idVoiture, const mat4& monde, float echelle, int cas, Genre genre, bool nuit, const vec3& camera);
void liberer(Renderer& r);
const std::string& texteCase(int cas);   // banc et tests : texte actuel d'une case
void masquerBanc(bool masque);            // banc seulement : plaques des autres voitures non dessinées (images « avant »)

} // namespace wfs::plaques_autres
