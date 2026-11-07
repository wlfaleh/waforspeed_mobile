// WaForSpeed — décor fixe préparé au chargement (WFS-268, épic E4 ; ordre de l'utilisateur : « les grands jeux chargent la map
// avant de lancer le jeu, ils préparent tout »).
// Avant : chaque carte (Ville, Corniche, socle Campagne / Grand Prix) construisait ses morceaux de décor EN COURSE (CH segments,
// ≈ 38 m) au fil de la route : création et destruction de tampons GPU pendant la course, un appel de dessin par morceau et par
// matériau (≈ 25 morceaux × 10 à 20 matériaux).
// Constat (WFS-268, journal) : la route ne se referme jamais en 3D, mais chaque tour est EXACTEMENT le premier, tourné et décalé
// (écart mesuré 0,0000 m sur les 4 cartes) : le décor d'un tour suffit, dessiné avec une matrice par tour.
// Ici :
//   - preparer() (derrière l'écran de chargement) construit tout le décor du tour 0, en détail et allégé (LOD), avec les
//     fonctions de construction des cartes (inchangées, en parallèle), puis le regroupe en BLOCS de B morceaux (≈ 150 m) par
//     matériau : un maillage par bloc, par matériau et par niveau de détail, envoyé au GPU une fois ;
//   - dessiner() ne crée et n'alloue rien : il parcourt les blocs de la fenêtre de vue (même fenêtre qu'avant), choisit le
//     niveau de détail par la distance le long de la route, et appelle la carte avec la matrice du tour du bloc ; chaque bloc
//     garde sa sphère englobante (masquage du renderer, WFS-82) : un bloc hors champ n'est pas dessiné.
// Tout ce qui bouge (voitures, piétons, feux, effets, objets animés des cartes) reste dessiné à part par les cartes.
#pragma once
#include <array>
#include <climits>
#include <cstdint>
#include <functional>
#include <vector>
#include "lots_decor_tour.h"
#include "renderer.h"

namespace wfs {
namespace lots {

constexpr int MAX_PARTS = 40;   // matériaux d'une carte au plus

// interrupteur (consigne du PO) : false = ancienne gestion par morceaux construits en course, gardée en repli ;
// valeur de départ : variable d'environnement WFS_LOTS (« 0 » : ancienne gestion), sinon vrai
extern bool g_lotsDecor;

// construction d'un morceau (indice ABSOLU du tour 0, CH segments, borné au tour par la carte) : v[p], i[p] par matériau
using Construire = std::function<void(int64_t chunk, bool lod, std::vector<Vertex>* v, std::vector<uint32_t>* i)>;

class DecorFixe {
public:
    // nom : carte (journal) ; parts : nombre de matériaux ; blocChunks : morceaux par bloc (4 = 154 m)
    // deuxNiveaux = false : un seul niveau de détail (route de Game) : la version allégée n'est ni construite ni gardée
    bool preparer(Renderer& r, Track& t, int parts, int blocChunks, const Construire& construire, const char* nom, bool deuxNiveaux = true);
    void liberer(Renderer& r);
    bool pret() const { return pret_; }
    int64_t longueurTour() const { return L_; }   // segments d'un tour (fin du dernier morceau : les cartes s'y arrêtent)
    // tour du segment absolu a (division par défaut) et matrice tour k → monde, pour les objets construits une fois dans le
    // tour 0 et dessinés au tour du joueur (feux tricolores) ; tour 0 : identité
    int64_t tourDe(int64_t a) const { return L_ <= 0 ? 0 : (a >= 0 ? a / L_ : -((-a + L_ - 1) / L_)); }
    const mat4& matrice(Track& t, int64_t k) { return matriceTour(t, k); }

    // blocs de la fenêtre [aP − arriere, aP + avant] (segments absolus) ; lodLoinM : bloc entièrement au-delà → version allégée.
    // dessin(part, maillage, monde) est appelé pour chaque maillage non vide ; rend le nombre d'appels. N'alloue rien.
    template <class F> int dessiner(Track& t, int64_t aP, int64_t arriere, int64_t avant, double lodLoinM, F&& dessin);

    // journal : sommets, octets GPU, durée de préparation
    size_t sommets() const { return sommets_; }
    size_t octets() const { return octets_; }
    double dureeMs() const { return dureeMs_; }
    int blocs() const { return (int)blocs_.size(); }
    // RG-04 : maillages et matériaux créés (tout le jeu, compteurs du renderer) depuis le premier appel (première image de la
    // course) ; le premier appel fixe l'origine
    void creationsEnCourse(const Renderer& r, unsigned long long& maillages, unsigned long long& materiaux) {
        if (orig_[0] == UINT64_MAX) { orig_[0] = r.creationsMaillages(); orig_[1] = r.creationsMateriaux(); }
        maillages = r.creationsMaillages() - orig_[0]; materiaux = r.creationsMateriaux() - orig_[1];
    }

private:
    struct Bloc { int64_t s0 = 0, s1 = 0; std::array<std::array<Mesh, MAX_PARTS>, 2> m{}; };   // segments [s0, s1) du tour 0 ; m[lod][part]
    const mat4& matriceTour(Track& t, int64_t k);   // tour k → monde (tour 0 : identité) ; cache fixe, aucune allocation
    std::vector<Bloc> blocs_;
    int parts_ = 0;
    int64_t L_ = 0;
    bool pret_ = false, deuxNiveaux_ = true;
    size_t sommets_ = 0, octets_ = 0;
    double dureeMs_ = 0;
    // matrice d'un tour : monde = R(k·θ) (x − p0) + q_k (lots_decor_tour.h) ; cache des 8 derniers tours demandés
    Tour tour_{};
    struct Cache { int64_t k = INT64_MIN; mat4 m = mat4::identity(); };
    std::array<Cache, 8> cache_{};
    uint64_t orig_[2] = {UINT64_MAX, UINT64_MAX};
};

inline double segmentsVersMetres(int64_t n) { return (double)n * SEG_M; }

template <class F> int DecorFixe::dessiner(Track& t, int64_t aP, int64_t arriere, int64_t avant, double lodLoinM, F&& dessin) {
    if (!pret_ || L_ <= 0) return 0;
    int n = 0;
    const int64_t s0 = aP - arriere, s1 = aP + avant;
    const int64_t k0 = tourDe(s0), k1 = tourDe(s1);
    for (int64_t k = k0; k <= k1; k++) {
        const int64_t base = k * L_;
        const mat4& M = matriceTour(t, k);
        for (const Bloc& b : blocs_) {
            if (b.s1 + base <= s0 || b.s0 + base > s1) continue;
            // détail si le point du bloc le plus proche du joueur est à moins de lodLoinM : jamais moins détaillé que les
            // morceaux d'avant (choisis morceau par morceau), un peu plus au loin dans le bloc de la limite
            const int64_t a0 = b.s0 + base, a1 = b.s1 + base, proche = aP < a0 ? a0 - aP : aP >= a1 ? aP - a1 + 1 : 0;
            const bool lod = deuxNiveaux_ && segmentsVersMetres(proche) > lodLoinM;
            for (int p = 0; p < parts_; p++) if (b.m[lod][(size_t)p].indexCount) { dessin(p, b.m[lod][(size_t)p], M); n++; }
        }
    }
    return n;
}

} // namespace lots
} // namespace wfs
