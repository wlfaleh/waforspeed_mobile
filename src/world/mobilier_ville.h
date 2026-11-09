// WaForSpeed — mobilier urbain de la Ville modélisé au niveau du vrai objet (ticket WFS-369, épic E15).
// Règle de l'utilisateur (2026-11-09 19:41 UTC) : plus aucune forme géométrique nue dans le monde 3D. Aucun modèle libre
// CC0 / CC BY aux vraies mesures n'est disponible par les sources permises (regles-reseau.md) pour ce mobilier de rue
// français ; RG-03 du ticket : chaque objet est donc modélisé par le code, pièce par pièce, aux mesures du vrai objet
// (sources dans mobilier_ville.cpp), avec chanfreins, profils tournés et tubes cintrés : jamais une boîte, un cylindre,
// un cône ou un disque posés tels quels.
// Toute la géométrie est écrite dans les tampons de la carte (morceaux préparés au chargement, WFS-268) : aucun objet n'est
// créé en course, un appel de dessin par matériau et par morceau comme avant.
// Repère local de chaque modèle : x vers l'arrière de l'objet (loin de la chaussée), y vers le haut, z le long de la rue ;
// origine au sol, au centre de l'objet. Les triangles sont orientés selon leur normale (faces avant vers l'extérieur).
#pragma once
#include <cstdint>
#include <vector>
#include "../core/math.h"
#include "../render/renderer.h"

namespace wfs::mobilier {

// tampons d'un maillage (veg::Geo, vie::Geo… : même forme)
struct Sortie {
    std::vector<Vertex>& v;
    std::vector<uint32_t>& i;
    template <class G> Sortie(G& g) : v(g.v), i(g.i) {}
};

// repère orthonormé : o origine, x, y, z axes
struct Repere {
    vec3 o, x, y, z;
    vec3 p(vec3 l) const { return o + x * l.x + y * l.y + z * l.z; }   // point local → monde
    vec3 d(vec3 l) const { return x * l.x + y * l.y + z * l.z; }       // direction locale → monde
};
// repère au sol en o : x et z horizontaux (z le long de la rue), y vertical
Repere repere(vec3 o, vec3 x, vec3 z);

// ---------- briques (couleur = case de palette uv) ----------
// pavé aux arêtes chanfreinées (ch), centré en c, demi-mesures h, axes du repère
void pave(Sortie s, const Repere& R, vec3 c, vec3 h, float ch, vec2 uv);
// pièce plate de a à b (repère local) : largeur l dans le plan (a, b, normale n), épaisseur e selon n, chanfrein ch
void barre(Sortie s, const Repere& R, vec3 a, vec3 b, vec3 n, float l, float e, float ch, vec2 uv);
// pièce tournée autour de l'axe y local passant par c : profil (rayon, hauteur) du bas extérieur vers le haut, puis vers
// l'axe ; n côtés ; plis > 0 : la surface ondule (tissu replié, plis côtes)
void tour(Sortie s, const Repere& R, vec3 c, const std::vector<vec2>& profil, int n, vec2 uv, int plis = 0, float amplitude = 0);
// tube cintré le long d'une ligne (repère local), rayon r, n côtés, bouts fermés par un bouchon
void tube(Sortie s, const Repere& R, const std::vector<vec3>& pts, float r, int n, vec2 uv, bool bouts = true);

// ---------- modèles ----------
// banc public à lattes (1,80 × 0,60 m, assise 0,45 m, dossier 0,85 m), flancs en fonte, accoudoirs
void banc(Sortie s, const Repere& R, vec2 bois, vec2 fonte);
// abribus (4,0 × 1,8 m, 2,55 m) : poteaux, toit en porte-à-faux, vitrages encadrés, panneau d'affichage, banc à lattes
void abribus(Sortie s, Sortie verre, const Repere& R, vec2 alu, vec2 affiche, vec2 bois);
// grille de parc entre deux poteaux a et b (monde, au sol), côté extérieur ext : barreaux à pointes, deux lisses
void grille(Sortie s, vec3 a, vec3 b, vec3 ext, vec2 fonte);
// fontaine : bassin mouluré de rayon rb, piédestal, vasque et pomme de pin ; k : échelle verticale du centre
void fontaine(Sortie s, const Repere& R, float rb, float k, vec2 pierre, vec2 eau);
// lampadaire de rue (mât conique de 7,4 m, crosse, luminaire) : x du repère vers la chaussée ; lentille dans « lampe »
void lampadaire(Sortie metal, Sortie lampe, const Repere& R, float portee, bool lod);
// panneau rond (Ø 700 mm) sur mât : R.x vers les conducteurs, centre du panneau à hauteur h ; fond, bord roulé, dos
void panneauRond(Sortie mat, Sortie s, const Repere& R, float h, vec2 fond, vec2 bord, vec2 dos);
// feu tricolore (caisson R11, 3 feux Ø 200 mm espacés de 0,30 m, visières) sur mât de 3,6 m ; R.x vers les conducteurs
void feuTricolore(Sortie mat, Sortie s, const Repere& R, vec2 noir);
// optique bombée Ø 200 mm du feu k (0 rouge en haut, 1 orange, 2 vert) du même repère (dessinée allumée ou éteinte à part)
void optique(Sortie s, const Repere& R, int k);
// terrasse de café : table de bistrot ronde à 4 pieds (Ø 0,70 m, 0,75 m) percée pour le parasol
void tableBistrot(Sortie s, const Repere& R, vec2 plateau, vec2 alu);
// chaise de bistrot (assise 0,45 m, dossier 0,86 m) : R.z vers la table
void chaiseBistrot(Sortie s, const Repere& R, vec2 cannage, vec2 alu);
// parasol de terrasse (Ø 2,7 m, 8 baleines, mât bois, pied lesté) ouvert ; toile à pans alternés t0 / t1
void parasolOuvert(Sortie s, const Repere& R, vec2 t0, vec2 t1, vec2 bois, vec2 metal);
// le même, fermé (la nuit) : toile roulée et sanglée
void parasolFerme(Sortie s, const Repere& R, vec2 toile, vec2 bois, vec2 metal);
// photophore (bougie dans un verre) posé sur la table
void photophore(Sortie s, const Repere& R);
// jardinière en terre cuite et buis taillé (graine : forme du buis)
void jardiniere(Sortie s, const Repere& R, vec2 pot, vec2 terre, vec2 buis0, vec2 buis1, uint32_t graine);

}  // namespace wfs::mobilier
