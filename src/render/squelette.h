// WaForSpeed — personnages animés par squelette (WFS-63, point d'extension E0).
// Chargement d'un fichier .wfp (tools/convert_people.mjs, personnages du jeu web : models/people/homme.json, femme.json
// et animations.json, Quaternius CC0) et lecture d'une animation sur le CPU (RG-04 : interpolation linéaire des
// positions, sphérique des rotations, boucle ; aucune allocation par image ; appelable depuis un thread de travail :
// la PoseSquelette appartient à l'appelant, le Personnage n'est que lu).
//
// Utilisation (WFS-19 : piétons et conducteurs) :
//   Personnage homme; chargerPersonnage(r, "people/homme.wfp", homme);
//   PoseSquelette pose; preparerPose(homme, pose);                         // une fois par instance (allocation)
//   echantillonner(homme, clipParNom(homme, "Walk_Loop"), t, pose); // chaque image (aucune allocation)
//   const Plage* p = plageParNom(homme, "corps-0");                // corps du niveau de détail 0 (+ coiffure au choix)
//   r.drawSkinned(homme.maillage, homme.materiau, monde, pose.palette.data(), (int)pose.palette.size(), p->premier, p->nombre,
//                 &apparence);   // WFS-72 : ApparencePersonnage (renderer.h), corps = homme.corps ; nullptr = texture seule
// Repère : mètres, Y vers le haut, pieds au sol (y = 0), le personnage regarde vers +Z (repère glTF du jeu web).
#pragma once
#include <string>
#include <vector>
#include "renderer.h"

namespace wfs {

struct Quat { float x = 0, y = 0, z = 0, w = 1; };

struct OsSquelette { std::string nom; int parent = -1; vec3 t{}; Quat r{}; vec3 s{1, 1, 1}; mat4 bindInverse = mat4::identity(); };
struct CanalAnim { int os = 0; int chemin = 0; std::vector<float> temps, valeurs; };   // chemin : 0 translation, 1 rotation, 2 échelle
struct ClipAnim { std::string nom; float duree = 0; std::vector<CanalAnim> canaux; };
struct Plage { std::string nom; uint32_t premier = 0, nombre = 0; };   // « corps-<lod> », « <coiffure>-<lod> »

struct Personnage {
    SkinnedMesh maillage; Material materiau; Image textures[2];
    std::vector<OsSquelette> os;      // parents avant enfants
    std::vector<int> palette;         // os portant des poids, dans l'ordre des indices des sommets (≤ SKIN_MAX_OS)
    std::vector<ClipAnim> clips;
    std::vector<Plage> plages;
    float hauteur = 0;
    vec4 corps{};   // WFS-72 : (hauteur du cou, taille, profondeur du cou, 0) → ApparencePersonnage::corps
    bool ok = false;
};

// pose d'une instance : locales (t, r, s), matrices du modèle, palette envoyée au renderer
struct PoseSquelette {
    std::vector<vec3> t, s; std::vector<Quat> r;
    std::vector<mat4> modele, palette;
};

bool chargerPersonnage(Renderer& r, const std::string& chemin, Personnage& out);
// WFS-299 : ajoute « <nom>-3 » (très simple) pour chaque plage « <nom>-2 », par regroupement des sommets ; rend le nombre de plages ajoutées
int ajouterNiveauTresSimple(const std::vector<SkinVertex>& v, std::vector<uint32_t>& idx, std::vector<Plage>& plages);
void detruirePersonnage(Renderer& r, Personnage& p);
int clipParNom(const Personnage& p, const std::string& nom);              // -1 si absent
const Plage* plageParNom(const Personnage& p, const std::string& nom);   // nullptr si absente
void preparerPose(const Personnage& p, PoseSquelette& pose);
// pose du clip `clip` à l'instant `temps` (s, en boucle sur la durée du clip) ; clip < 0 : pose de repos
void echantillonner(const Personnage& p, int clip, double temps, PoseSquelette& pose);
// WFS-233 : matrices du modèle et palette recalculées depuis les locales (t, r, s) de la pose (après une retouche des locales)
void recalculerPose(const Personnage& p, PoseSquelette& pose);
// WFS-233 : IK analytique à 2 os (épaule → coude → main) : la main va au point `cible` (repère du modèle, mètres), le coude reste
// dans le plan qu'il avait après l'échantillonnage ; cible hors de portée : bras tendu vers elle. Appeler après echantillonner ;
// aucune allocation. Renvoie false si un os est invalide (pose inchangée).
bool atteindre(const Personnage& p, PoseSquelette& pose, int epaule, int coude, int main, const vec3& cible);
int osParNom(const Personnage& p, const std::string& nom);   // -1 si absent

} // namespace wfs
