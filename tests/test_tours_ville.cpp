// WaForSpeed — WFS-571 (ANO-280) : tours de la Ville en vrais volumes et toits équipés (src/world/archi_kit.cpp, archi_geo.cpp,
// archi_details.cpp). Contrôles :
//   RG-03 : une tour (style moderne) est découpée en tronçons empilés (socle, fût en retrait, attique) : mêmes niveaux, même
//           hauteur totale, chaque tronçon conforme au kit (verifier), retraits croissants, profondeur ≥ 6 m ; deux tours voisines
//           d'un même côté (graines distantes de 22) n'ont jamais la même silhouette ; les autres styles restent d'un seul tenant ;
//   RG-04 : sur le dernier toit, l'édicule a sa porte (vantail, huisserie) et sa grille à chaque niveau de détail ; une centrale de
//           traitement d'air sur un grand toit ; garde-corps ; aucun acrotère sous le tronçon du dessus (pas de murs confondus) ;
//   sens des triangles (normale = sens de l'enroulement) de toute la géométrie construite.
#include "../src/world/archi_geo.h"
#include <cmath>
#include <cstdio>
#include <set>

using namespace wfs;
using namespace wfs::archi;

static int echecs = 0, controles = 0;
static void verifierQue(bool ok, const char* quoi) { controles++; if (!ok) { if (echecs < 25) std::printf("[ÉCHEC] %s\n", quoi); echecs++; } }

// pièces au-dessus de y dans la boîte x ∈ [x0, x1], z ∈ [z0, z1] (triangles de la pièce k dont les 3 sommets y sont)
static size_t trianglesDans(const Maillage& m, float y, float x0, float x1, float z0, float z1, float yMax = 1e9f) {
    size_t n = 0;
    for (size_t t = 0; t + 2 < m.i.size(); t += 3) {
        bool dedans = true;
        for (int c = 0; c < 3; c++) {
            const vec3 p = m.v[m.i[t + c]].p;
            dedans = dedans && p.y > y && p.y < yMax && p.x > x0 && p.x < x1 && p.z > z0 && p.z < z1;
        }
        n += dedans;
    }
    return n;
}

int main() {
    std::set<int> silhouettes;
    size_t tours = 0, avecCentrale = 0;
    for (uint32_t g = 0; g < 600; g++) {
        const float largeur = 10.6f + (float)(g % 21) * 0.1f, hauteur = 20.f + (float)((g * 53) % 860) / 10.f, D = 10.f + (float)((g * 37) % 140) / 10.f;
        const Plan p = assembler(Style::Moderne, largeur, hauteur, g * 2 + (g & 1));
        const std::vector<Troncon> t = troncons(p, D);
        if (p.etages < 3) continue;
        tours++;
        verifierQue(t.size() >= 2, "RG-03 : une tour moderne a au moins deux tronçons (plus de prisme du sol au toit)");
        int niveaux = 0; size_t baies = 0; float y = 0, retrait = -1;
        for (size_t k = 0; k < t.size(); k++) {
            const Troncon& c = t[k];
            const char* e = nullptr;
            const int ne = verifier(c.plan, &e); if (ne && echecs < 3) std::printf("  graine %u, tronçon %zu/%zu, %d étages, sur socle %d : %s\n", g, k, t.size(), c.plan.etages, (int)c.plan.surSocle, e);
            verifierQue(ne == 0, "RG-03 : tronçon conforme au kit");
            verifierQue(std::fabs(c.dy - y) < 1e-3f, "RG-03 : chaque tronçon posé sur le précédent");
            verifierQue(c.retrait >= retrait, "RG-03 : retraits croissants vers le haut");
            verifierQue(c.profondeur >= 6.f - 1e-4f && std::fabs(c.profondeur + c.retrait - D) < 1e-3f, "RG-03 : nu arrière commun, profondeur ≥ 6 m");
            verifierQue(c.dx >= 0 && c.dx + c.plan.largeur <= p.largeur + 1e-3f, "RG-03 : tronçon dans l'emprise de la tour");
            verifierQue(c.plan.surSocle == (k > 0), "RG-03 : seul le socle a un rez-de-chaussée");
            verifierQue(c.plan.couvert.actif == (k + 1 < t.size()), "toit couvert par le tronçon du dessus : sauf le dernier");
            verifierQue(c.plan.edicules == (k + 1 == t.size() && p.edicules), "édicule sur le dernier toit seulement");
            y = c.dy + c.plan.hauteur(); retrait = c.retrait;
            niveaux += c.plan.etages + 1; baies += c.plan.baies.size();
        }
        verifierQue(niveaux == p.etages + 1, "RG-03 : mêmes niveaux que le plan");
        verifierQue(std::fabs(y - p.hauteur()) < 1e-3f, "RG-03 : même hauteur totale");
        verifierQue(baies <= p.baies.size() && baies + 2 * (size_t)(t.back().plan.etages + 1) * 2 >= p.baies.size() - 0, "fenêtres gardées (attique : travées de rive en moins)");
        // silhouette : nombre de tronçons et retraits ; la tour voisine du même côté (graine + 22) en a une autre
        auto sil = [](const std::vector<Troncon>& v) { int s = (int)v.size() * 1000; for (const Troncon& c : v) s += (int)std::lround(c.retrait * 10) + (c.dx > 0 ? 500 : 0); return s; };
        silhouettes.insert(sil(t));
        const Plan voisin = assembler(Style::Moderne, largeur, hauteur, g * 2 + (g & 1) + 22);
        if (voisin.etages == p.etages && voisin.travees == p.travees && voisin.travees >= 5) {
            const auto tv = troncons(voisin, D);
            verifierQue(tv.size() != t.size() || (tv.size() > 1 && (tv[1].retrait != t[1].retrait || tv[1].dx != t[1].dx)), "RG-03 : deux tours voisines n'ont pas la même silhouette");
        }
        // géométrie : aucun acrotère ni couvertine sous le tronçon du dessus ; sens des triangles ; toit équipé à chaque niveau
        for (int lod = 0; lod < 3; lod++) {
            for (size_t k = 0; k < t.size(); k++) {
                const Troncon& c = t[k];
                Batiment b; construire(c.plan, c.profondeur, lod, b);
                for (int part = 0; part < NB_PARTS; part++) {
                    const Maillage& m = b.part[part];
                    for (size_t i = 0; i + 2 < m.i.size(); i += 3) {
                        const Sommet &A = m.v[m.i[i]], &B = m.v[m.i[i + 1]], &C = m.v[m.i[i + 2]];
                        const vec3 n = cross(B.p - A.p, C.p - A.p);
                        if (length(n) > 1e-8f) verifierQue(dot(n, A.n) > 0, "sens des triangles : normale dans le sens de l'enroulement");
                    }
                }
                const float H = c.plan.hauteur();
                if (c.plan.couvert.actif) {
                    const Plan::Couvert& cv = c.plan.couvert;
                    verifierQue(trianglesDans(b.part[MUR], H + 0.01f, cv.x0 + 0.01f, cv.x1 - 0.01f, -c.profondeur + 0.01f, cv.zAvant - 0.01f) == 0,
                                "aucun acrotère sous le tronçon du dessus (murs confondus)");
                    verifierQue(trianglesDans(b.part[METAL], H + 0.01f, cv.x0 + 0.01f, cv.x1 - 0.01f, -c.profondeur + 0.01f, cv.zAvant - 0.01f) == 0,
                                "aucune couvertine ni garde-corps sous le tronçon du dessus");
                }
                if (k + 1 == t.size() && c.plan.edicules && c.plan.largeur > 6 && c.profondeur > 6) {
                    const float L = c.plan.largeur;
                    verifierQue(trianglesDans(b.part[ENCADREMENT], H + 0.04f, L / 2 - 0.5f, L / 2 + 0.5f, -c.profondeur * 0.6f + 1.4f, -c.profondeur * 0.6f + 1.45f) >= 2, "RG-04 : porte de l'édicule (vantail) à chaque niveau de détail");
                    verifierQue(trianglesDans(b.part[METAL], H + 1.4f, 0, L, -c.profondeur, 0) >= 2, "RG-04 : grille de ventilation de l'édicule à chaque niveau de détail");
                    if (lod <= 1) verifierQue(trianglesDans(b.part[METAL], H + 1.0f, 0, L, -c.profondeur, -0.0f) >= 20, "RG-04 : garde-corps de toiture de la tour");
                    if (lod == 1 && trianglesDans(b.part[METAL], H + 1.9f, 0, L, -c.profondeur, 0, H + 2.4f) > 40) avecCentrale++;   // ventilateurs de la centrale (entre la grille et la dalle de l'édicule)
                }
            }
        }
        // construireTroncons : même géométrie que les tronçons construits un à un (banc du kit)
        Batiment tout; construireTroncons(p, D, 1, tout);
        size_t tri = 0, triSeuls = 0;
        for (int part = 0; part < NB_PARTS; part++) tri += tout.part[part].triangles();
        for (const Troncon& c : t) { Batiment b; construire(c.plan, c.profondeur, 1, b); for (int part = 0; part < NB_PARTS; part++) triSeuls += b.part[part].triangles(); }
        verifierQue(tri == triSeuls, "construireTroncons : tous les tronçons");
    }
    verifierQue(tours > 300, "assez de tours essayées");
    verifierQue(silhouettes.size() >= 6, "RG-03 : plusieurs silhouettes de tour");
    verifierQue(avecCentrale > 50, "RG-04 : centrales de traitement d'air sur les grands toits");
    // les autres styles ne changent pas : un seul tronçon, le plan tel quel
    for (int s = 0; s < (int)Style::Nombre; s++) {
        if (s == (int)Style::Moderne) continue;
        const Plan p = assembler((Style)s, 15.f, 25.f, 3);
        const auto t = troncons(p, 12.f);
        verifierQue(t.size() == 1 && t[0].retrait == 0 && t[0].dy == 0 && !t[0].plan.surSocle && t[0].plan.baies.size() == p.baies.size(), "autres styles : un seul tronçon, inchangé");
    }
    std::printf("[%s] tours : %zu essayées, %zu silhouettes, %zu toits avec centrale ; %d contrôles, %d échecs\n", echecs ? "ÉCHEC" : "OK", tours, silhouettes.size(), avecCentrale, controles, echecs);
    return echecs ? 1 : 0;
}
