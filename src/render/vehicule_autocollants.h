// WaForSpeed — WFS-495 (épic E18, domaine E10 Véhicules) : autocollants de la voiture du joueur, posés comme de vrais adhésifs.
//
// Principe : un autocollant n'est JAMAIS une pièce de géométrie (pas de bande plate qui flotte, ANO-250, ni de forme nue) : il est
// cuit dans la texture d'habillage de la voiture (dépliage par projection de WFS-492, vehicule_skin.h, atlas), que le shader
// (shaders/vehicule.glsl, habillage) lit sur les pièces peintes seulement. Il épouse donc chaque bosse et chaque creux de la
// carrosserie, garde sa taille réelle (rien n'est étiré par la forme), s'arrête où la peinture s'arrête : jamais sur les vitres,
// les phares, la calandre ni les jantes (RG-01), et passe sous la finition et le vernis (RG-04).
//
// Calques (RG-02) : 20 au plus, dans l'ordre (le dernier par-dessus) ; chacun : motif, direction de projection (côté gauche,
// droit, capot, toit, coffre, avant, arrière), position (cm, autour du point d'ancrage de la direction), rotation (degrés, sens
// trigonométrique vu de dehors), échelle (10 % à 300 % de la taille du motif), couleur (28 couleurs du catalogue, PEINTURES de
// WFS-487), miroir (copie symétrique de l'autre côté de la voiture ; lettres et chiffres restent lisibles).
//
// Texte d'un habillage (RG-07, court, sans « = » ni retour à la ligne : se range tel quel dans la sauvegarde clé=valeur) :
//   <motif>,<direction>,<x cm>,<y cm>,<rotation °>,<échelle %>,<couleur>,<miroir 0|1>;…
//   direction : g (gauche), d (droite), c (capot), t (toit), k (coffre), a (avant), r (arrière)
//   ex. « flamme-1,g,-110,-5,0,100,orange,1;bande-double,c,0,0,0,220,blanc,0;chiffre-7,g,10,10,0,120,noir,1 »
// Repère de chaque direction, vu de dehors : x vers la droite de l'observateur, y vers le haut. Flanc gauche : x vers l'arrière ;
// flanc droit : x vers l'avant ; dessus (capot, toit, coffre), vu du côté gauche : x vers l'arrière, y vers la droite de la voiture
// (une bande sans rotation va de l'avant à l'arrière) ; avant : x vers la gauche de la voiture ; arrière : x vers sa droite.
//
// Cuisson (RG-03) : sur le processeur, au garage quand le joueur valide ou au chargement de la voiture (jamais en course) :
// skin éventuelle (masques A et B, RG-05 : les autocollants vont par-dessus) + calques → une seule texture RGBA 1024 × 1024
// (RGB = couleur en codes sRGB, A = couverture ; ailleurs la peinture choisie reste visible), chaîne complète de mipmaps,
// compressée en ETC2 RGBA8 (vehicule_etc2.h). Bords nets sans halo (RG-04) : chaque motif est vectoriel, rastérisé avec la
// couverture exacte de chaque texel de l'atlas (anticrénelage analytique), à toute échelle.
//
// Bibliothèque (RG-06, dessins originaux, aucun logo ni sponsor réel) : bandes (5), chiffres 0-9, flammes (3), éclairs (3),
// étoiles (3), lettres A-Z (police Rajdhani Bold, SIL OFL 1.1, déjà dans assets/fonts). Sans Vulkan ; thread principal (la
// cuisson répartit l'encodage sur les cœurs par la fonction donnée).
#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace wfs::vehicule_autocollants {

constexpr int MAX_CALQUES = 20;                 // RG-02
constexpr int ECHELLE_MIN = 10, ECHELLE_MAX = 300;   // % (RG-02)

enum class Direction : uint8_t { Gauche, Droite, Capot, Toit, Coffre, Avant, Arriere };
constexpr int NB_DIRECTIONS = 7;
char codeDirection(Direction d);                 // g, d, c, t, k, a, r
const char* nomDirection(Direction d);           // « gauche », « droite », « capot »…

struct Calque {
    std::string motif;                           // identifiant de la bibliothèque (ex. « flamme-1 »)
    Direction dir = Direction::Gauche;
    int x = 0, y = 0;                            // cm, autour du point d'ancrage de la direction
    int rotation = 0;                            // degrés, −180 à 180
    int echelle = 100;                           // %, ECHELLE_MIN à ECHELLE_MAX
    std::string couleur = "blanc";               // identifiant de PEINTURES (catalogue WFS-487)
    bool miroir = false;
};

// texte → calques ; un calque illisible (motif, direction ou couleur inconnus, champ manquant) est ignoré et compté dans *rejetes ;
// au-delà de MAX_CALQUES, les suivants sont ignorés (comptés) ; valeurs bornées (échelle, rotation). Vrai si aucun rejet.
bool lire(const std::string& texte, std::vector<Calque>& sortie, int* rejetes = nullptr);
std::string texte(const std::vector<Calque>& calques);   // calques → texte (lire(texte(c)) redonne c)

// ---- bibliothèque
enum class Famille : uint8_t { Bande, Chiffre, Flamme, Eclair, Etoile, Lettre };
struct Motif {
    const char* id;
    Famille famille;
    float largeur, hauteur;                      // taille à 100 % (m) ; lettres et chiffres : hauteur des capitales 30 cm
};
const std::vector<Motif>& bibliotheque();        // 50 motifs
const Motif* motif(const std::string& id);       // nullptr si inconnu
bool lisible(Famille f);                         // lettres et chiffres : jamais retournés par le miroir
// contours d'un motif, en mètres à 100 % (centre du motif à l'origine, y vers le haut) ; police = Rajdhani Bold (octets du .ttf),
// nécessaire aux lettres et aux chiffres. Contours fermés ; trous en sens inverse. Vide si motif inconnu ou police illisible.
using Contour = std::vector<std::pair<float, float>>;
std::vector<Contour> contours(const std::string& id, const std::vector<uint8_t>& police);

// ---- cuisson
struct Entree {
    const uint8_t* masques = nullptr;            // skin : masques RG8 1024 × 1024 (R = couleur A, G = couleur B) ; nullptr : aucune skin
    float couleurA[3] = {0, 0, 0}, couleurB[3] = {0, 0, 0};   // couleurs A et B de la skin (codes sRGB 0..1)
    const std::vector<Calque>* calques = nullptr;
    float bmin[3] = {-0.9f, 0.f, -2.2f}, bmax[3] = {0.9f, 1.45f, 2.2f};   // boîte du modèle (m) : points d'ancrage des directions
    const std::vector<uint8_t>* police = nullptr; // Rajdhani Bold (lettres et chiffres)
    // répartition de l'encodage sur les cœurs : repartir(n, corps) appelle corps(début, fin) sur des tranches de [0, n) ; vide : séquentiel
    std::function<void(int, const std::function<void(int, int)>&)> repartir;
    bool garderRgba = false;                     // garder aussi le niveau 0 en RGBA8 non compressé (bancs, test)
};
struct Resultat {
    uint32_t taille = 0;                         // 1024
    std::vector<std::vector<uint8_t>> niveaux;   // blocs ETC2 RGBA8 de chaque niveau (1024 … 1)
    std::vector<uint8_t> rgba;                   // niveau 0 RGBA8 avant compression (si garderRgba)
    int calques = 0;                             // calques posés (copies du miroir comprises)
    int motifsVides = 0;                         // calques dont le motif n'a rien donné (police absente…)
    double msDessin = 0, msMip = 0, msCompression = 0;   // durées mesurées (journal, RG-03 : < 1 s sur le S21 FE)
};
// skin + calques → texture ETC2 RGBA8 ; faux si rien à cuire (ni skin ni calque)
bool cuire(const Entree& e, Resultat& r);

// point d'ancrage d'une direction (repère de la voiture, m) d'après la boîte du modèle : flancs au milieu de la longueur à 42 %
// de la hauteur ; capot à 20 % de la longueur depuis l'avant, toit au milieu (+5 %), coffre à 10 % de l'arrière ; faces à 40 %
// de la hauteur. Renvoie (x, y, z).
void ancrage(Direction d, const float bmin[3], const float bmax[3], float sortie[3]);
// point (a, b) du plan de la direction (m, vu de dehors, autour de l'ancrage) → pixel (u, v) de l'atlas (0..1024)
void versAtlas(Direction d, const float ancre[3], float a, float b, float& u, float& v);

} // namespace wfs::vehicule_autocollants
