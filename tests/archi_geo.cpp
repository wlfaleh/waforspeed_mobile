// WaForSpeed — WFS-161 (ST-1) : géométrie du kit d'architecture (src/world/archi_geo.h).
// Sens des triangles (normale = sens de l'enroulement, sinon scene.frag retourne la normale et l'éclairage s'inverse),
// vrais trous dans la façade (aire du mur = façade − ouvertures), M-01 sur la géométrie (écart entre vitres de deux
// niveaux = hauteur d'étage à ± 1 %), garde-corps à la hauteur du référentiel, niveaux de détail de moins en moins chers.
#include "../src/world/archi_geo.h"
#include "../src/render/bench_realisme.h"
#include <cmath>
#include <cstdio>
#include <string>

using namespace wfs::archi;
using wfs::vec3;

static int echecs = 0, controles = 0;
static void verifierQue(bool ok, const char* quoi) { controles++; if (!ok) { if (echecs < 15) std::printf("[ÉCHEC] %s\n", quoi); echecs++; } }

int main() {
    wfs::reel::Referentiel R;
    verifierQue(R.charger(std::string(WFS_SOURCE_DIR) + "/references/reel"), "référentiel lisible");
    const wfs::reel::Json* gc = R.mesure("archi.garde_corps.hauteur");
    const double gcRef = gc ? gc->nombre("valeur", 1.0) : 1.0;
    size_t trisMax[3] = {};
    for (int s = 0; s < (int)Style::Nombre; s++) {
        size_t tris[3] = {};
        for (uint32_t g = 0; g < 60; g++) {
            const float largeur = 6.f + (float)((g * 37) % 300) / 10.f, hauteur = 6.f + (float)((g * 53) % 220) / 10.f, D = 8.f + g % 10;
            const Plan p = assembler((Style)s, largeur, hauteur, g);
            for (int lod = 0; lod < 3; lod++) {
                Batiment b;
                construire(p, D, lod, b);
                size_t t = 0;
                for (int k = 0; k < NB_PARTS; k++) {
                    const Maillage& m = b.part[k];
                    t += m.triangles();
                    for (size_t i = 0; i + 2 < m.i.size(); i += 3) {
                        const Sommet &A = m.v[m.i[i]], &B = m.v[m.i[i + 1]], &C = m.v[m.i[i + 2]];
                        const vec3 c = cross(B.p - A.p, C.p - A.p);
                        const float aire = length(c);
                        verifierQue(std::isfinite(aire) && aire > 1e-8f, "triangle non vide et fini");
                        if (aire > 1e-8f) verifierQue(dot(c * (1.f / aire), A.n) > 0.99f, "normale dans le sens de l'enroulement");
                        if (k == VITRE) verifierQue(A.uv.x >= -1e-4f && A.uv.x <= 1.0001f && A.uv.y >= -1e-4f && A.uv.y <= 1.0001f, "uv de vitre dans 0..1");
                    }
                }
                tris[lod] += t;
                // façade : vrais trous (lod 0 et 1), mur plein peint (lod 2)
                double ouvertures = 0;
                for (const Baie& x : p.baies) ouvertures += (double)x.l * x.h;
                const double attendu = (double)p.largeur * p.hauteur() - (lod < 2 ? ouvertures : 0);
                const double aire = aireFacade(b.part[MUR], p.hauteur());
                verifierQue(std::fabs(aire - attendu) <= 1e-3 * attendu, "aire du mur de façade = façade − ouvertures");
                if (lod == 0) {
                    // M-01 : bas des vitres de deux étages successifs (même travée) espacés d'une hauteur d'étage à ± 1 %
                    for (const Baie& x : p.baies) for (const Baie& y : p.baies)
                        if (x.niveau >= 1 && y.niveau == x.niveau + 1 && x.travee == y.travee)
                            verifierQue(std::fabs((y.y - x.y) - p.hEtage) <= 0.01f * p.hEtage, "M-01 : niveaux espacés d'un étage à ± 1 %");
                    // garde-corps : dessus à la hauteur du référentiel au-dessus du plancher du balcon
                    const Maillage& met = b.part[METAL];
                    for (const Baie& x : p.baies) if (x.balcon) {
                        float haut = -1;
                        for (const Sommet& v : met.v) if (std::fabs(v.p.x - x.x) < x.l / 2 + 0.5f && v.p.y > haut && v.p.y < p.niveauY(x.niveau) + 2.f && v.p.y > p.niveauY(x.niveau) - 0.01f) haut = v.p.y;
                        verifierQue(std::fabs(haut - p.niveauY(x.niveau) - gcRef) <= 0.01 * gcRef, "garde-corps à la hauteur du référentiel (± 1 %)");
                    }
                }
            }
        }
        verifierQue(tris[0] > tris[1] && tris[1] > tris[2], "chaque niveau de détail est moins cher que le précédent");
        std::printf("[%s] %-14s : triangles moyens par bâtiment lod0 %zu, lod1 %zu, lod2 %zu\n", tris[0] > tris[1] && tris[1] > tris[2] ? "OK" : "ÉCHEC",
                    regles((Style)s).nom, tris[0] / 60, tris[1] / 60, tris[2] / 60);
        for (int l = 0; l < 3; l++) trisMax[l] = std::max(trisMax[l], tris[l] / 60);
    }
    std::printf("archi_geo : %d contrôles, %d échec(s) ; garde-corps du référentiel %.2f m\n", controles, echecs, gcRef);
    return echecs ? 1 : 0;
}
