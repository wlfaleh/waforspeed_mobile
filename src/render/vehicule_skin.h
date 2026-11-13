// WaForSpeed — WFS-492 (épic E18, domaine E10 Véhicules) : skins, habillages complets de la carrosserie de la voiture du joueur.
//
// Une skin = une texture par couple voiture × skin, assets/cars/skins/<voiture>-<skin>.etc2.ktx2 (tools/skins/generer.py) :
//   - format EAC RG11 (famille ETC2, compressé, mipmaps calculées par l'outil), 1024 × 1024 : deux masques de couverture,
//     R = couleur A de la skin, G = couleur B (posée par-dessus A) ; ailleurs, la peinture choisie reste visible (RG-01) ;
//   - dessinée dans un dépliage par projection de la carrosserie, en mètres du repère de la voiture (même cadre pour toutes les
//     voitures, ATLAS ci-dessous) : flanc gauche, flanc droit, dessus, avant et arrière ; le shader (shaders/vehicule.glsl,
//     habillage) mêle les projections selon la normale lissée : motifs droits, sans étirement, ni couture aux jonctions des
//     panneaux (la projection est continue d'un panneau à l'autre, RG-04) ;
//   - les couleurs A et B de chaque skin sont dans le shader (SKIN_COULEURS) : la texture ne porte que les masques.
// Seules les pièces de peinture (MAT_PAINT) de la voiture du joueur la reçoivent : vitres, phares, calandre, jantes jamais peints ;
// trafic, adversaires et police sans skin (RG-06). La finition choisie (WFS-490) et le vernis restent par-dessus (RG-01).
//
// Mémoire (RG-03, RC-E18-02) : UNE seule texture d'habillage chargée à la fois (celle de la voiture du joueur), au chargement
// de la voiture (Game::setCar) ; libérée à la sortie (Game::shutdown) ou quand la voiture change. Rien n'est créé en course.
// GPU sans ETC2 (PC : lavapipe) : les niveaux EAC sont décodés sur le processeur en RG8 (même image que le téléphone).
// Transport sans constante de poussée en plus : le numéro de la skin voyage dans le métal codé des pièces peintes
// (pc.params.x = métal + 2 × finition + 16 × skin ; vehicule_finitions.h pour la finition). Thread principal.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace wfs {
struct Model;
struct Material;
class Renderer;
}

namespace wfs::vehicule_skin {

constexpr int AUCUNE = 0;   // numéro 0 : aucune skin (aspect d'avant, au bit près)
constexpr int NOMBRE = 4;   // skins du catalogue (BOUTIQUE_SKINS_IDS de configgame.h) : numéros 1 à 4

// cadre du dépliage, en mètres du repère de la voiture (avant vers −Z, posée au sol, centrée) ; mêmes valeurs dans le shader
// (SKIN_*) et dans tools/skins/generer.py (ATLAS). Lignes de la texture (pixels sur 1024) : zones séparées par 8 px de marge.
namespace atlas {
constexpr float DEMI_LARGEUR = 1.10f;   // |x| ≤ 1,10 m (rétroviseurs compris)
constexpr float DEMI_LONGUEUR = 2.50f;  // |z| ≤ 2,50 m (Giulia : 4,64 m)
constexpr float HAUTEUR = 1.60f;        // 0 ≤ y ≤ 1,60 m (500 : 1,49 m)
constexpr int TAILLE = 1024;
constexpr int GAUCHE0 = 0, GAUCHE1 = 272;      // flanc gauche (x < 0), vu de dehors : u = avant → arrière
constexpr int DROITE0 = 280, DROITE1 = 552;    // flanc droit (x > 0), vu de dehors : u = arrière → avant
constexpr int DESSUS0 = 560, DESSUS1 = 864;    // dessus : u = avant → arrière, v = gauche → droite
constexpr int FACES0 = 872, FACES1 = 1024;     // avant (u 0..0,5, vu de face : droite de la voiture à gauche) et arrière (u 0,5..1)
}

// numéro d'une skin d'après son identifiant (« rallye », « drifttokyo », « neonnuit », « camouflage ») ; 0 si vide ou inconnu
int depuisId(const char* id);
// chemin de la texture (sans « .etc2.ktx2 ») : cars/skins/<voiture>-<skin>
std::string chemin(const char* idVoiture, const char* idSkin);

// pose la skin idSkin sur le modèle m de la voiture du joueur (id idVoiture) : charge sa texture et prépare les matériaux peints ;
// la skin précédente est libérée d'abord (une seule texture en mémoire). idSkin vide ou inconnu, ou texture absente : aucune skin
// (aspect d'avant), faux. À appeler hors course (chargement de la voiture), GPU arrêté sur l'ancienne skin.
bool poser(Renderer& r, const Model& m, const char* idVoiture, const char* idSkin);
void liberer(Renderer& r);   // texture et matériaux rendus (GPU arrêté : à l'appelant d'attendre la fin des images en vol)

// matériau habillé de la pièce d'indice « materiau » du modèle m (peinture du joueur), sinon nullptr (aspect d'avant)
const Material* materiau(const Model& m, uint32_t materiau);
// métal codé (pc.params.x, déjà codé avec la finition) + 16 × numéro de la skin posée
float coder(float metalCode);

// état (journal du banc, RG-03 : une seule texture d'habillage en mémoire)
struct Etat { int skin = AUCUNE; std::string voiture, id; int textures = 0; size_t octetsGpu = 0; bool decodeCpu = false; int materiaux = 0; };
Etat etat();

// ---- EAC (Khronos Data Format Specification 1.3, § 21.3 « EAC R11 ») : décodage d'un bloc de 4 × 4 texels
// valeurs sur 11 bits (0 à 2047), ordre des texels : x + 4 × y
void decoderEacR11(const uint8_t bloc[8], uint16_t sortie[16]);
// bloc RG11 (16 octets : R puis G) → 16 paires (R, G) sur 8 bits, arrondies (même valeur que le filtrage du GPU, ±0,5 code)
void decoderEacRG11(const uint8_t bloc[16], uint8_t sortie[16][2]);

} // namespace wfs::vehicule_skin
