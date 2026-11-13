// WaForSpeed — WFS-460 (RG-01, RG-02, ANO-250) : bandes « POLICE » posées sur la tôle (src/game/police_bandes.h).
// 1) caisse d'essai galbée (rentrée de caisse, jour de porte, pas de bande sans tôle) : chaque point de la bande est à 5 mm
//    de la tôle, des deux côtés, enroulement vers l'extérieur, texte à l'endroit des deux côtés, mesures de la livrée ;
// 2) vraie 208 de la police (assets/cars/p208.wfm) : bande sur les portières, entre les passages de roue, rien ne flotte.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include "../src/game/police_bandes.h"
#include "../src/game/roues.h"
#include "../src/render/vehicule_mesure.h"

using namespace wfs;
using namespace wfs::police_bandes;

static int g_echecs = 0;
#define VERIFIE(cond, ...) do { if (!(cond)) { g_echecs++; std::printf("ÉCHEC : "); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)

// caisse d'essai : flanc galbé |x| = demi(y) (rentrée de caisse : plus étroit en haut), de z = −2 à 2 m, y de 0,25 à 1,1 m ;
// un jour de porte de 6 mm à z = 0,2 (pas de tôle, une paroi intérieure 25 cm plus loin), des deux côtés
static float demi(float y) { return 0.86f - 0.12f * (y - 0.55f) * (y - 0.55f) - (y > 0.55f ? 0.05f * (y - 0.55f) : 0.f); }
static std::vector<Triangle> caisse(bool jour) {
    std::vector<Triangle> t;
    const int NZ = 80, NY = 34;
    for (int cote = -1; cote <= 1; cote += 2)
        for (int i = 0; i < NZ; i++) for (int j = 0; j < NY; j++) {
            const float z0 = -2.f + 4.f * i / NZ, z1 = -2.f + 4.f * (i + 1) / NZ, y0 = 0.25f + 0.85f * j / NY, y1 = 0.25f + 0.85f * (j + 1) / NY;
            const bool trou = jour && z1 > 0.197f && z0 < 0.203f;
            const float r = trou ? 0.25f : 0.f;   // dans le jour : paroi intérieure
            const vec3 a{cote * (demi(y0) - r), y0, z0}, b{cote * (demi(y0) - r), y0, z1}, c{cote * (demi(y1) - r), y1, z1}, d{cote * (demi(y1) - r), y1, z0};
            t.push_back({a, b, c}); t.push_back({a, c, d});
        }
    return t;
}

int main(int argc, char** argv) {
    // ---------- 1. caisse d'essai ----------
    {
        const float pneuAv = -1.0f, pneuAr = 1.0f, sol = 0.f, H = 1.43f;
        const Bandes b = construire(caisse(true), pneuAv, pneuAr, sol, H);
        VERIFIE(b.ok(), "caisse d'essai : aucune bande");
        VERIFIE((int)b.sommets.size() == 2 * RANGEES * COLONNES, "deux côtés attendus : %zu sommets", b.sommets.size());
        VERIFIE(std::fabs(b.zAvant - (pneuAv + MARGE_PASSAGE_M)) < 1e-5f && std::fabs(b.zArriere - (pneuAr - MARGE_PASSAGE_M)) < 1e-5f,
                "bande entre les passages de roue : z %.3f → %.3f", b.zAvant, b.zArriere);
        const float haut = b.yHaut - b.yBas;
        VERIFIE(haut >= 0.15f && haut <= 0.20f, "RG-02 : hauteur %.3f m (15 à 20 cm)", haut);
        VERIFIE(std::fabs((b.yBas + b.yHaut) / 2 - CENTRE_HAUTEUR * H) < 1e-4f, "centre de la bande à %.3f m", (b.yBas + b.yHaut) / 2);
        VERIFIE(b.manques == 0, "points sans tôle : %d", b.manques);
        VERIFIE(b.creux > 0, "le jour de porte (z = 0,2) n'a pas été reconnu");
        int gauche = 0, droite = 0; float ecartMax = 0;
        for (const Sommet& s : b.sommets) {
            (s.p.x > 0 ? droite : gauche)++;
            // distance à la tôle le long de x (normale presque horizontale) : 5 mm, jour de porte compris (bouché)
            const float e = std::fabs(s.p.x) - demi(s.p.y);
            ecartMax = std::max(ecartMax, std::fabs(e - DECALAGE_M));
            VERIFIE(s.n.x * s.p.x > 0.9f * std::fabs(s.p.x), "normale tournée vers l'intérieur en (%.2f, %.2f, %.2f)", s.p.x, s.p.y, s.p.z);
        }
        VERIFIE(gauche == droite && gauche == RANGEES * COLONNES, "côtés : %d à gauche, %d à droite", gauche, droite);
        VERIFIE(ecartMax < 0.0015f, "bande à plus de 5 ± 1,5 mm de la tôle : écart %.4f m", ecartMax);
        // enroulement : la face avant de chaque triangle regarde dehors (sinon le rendu la retire : ANO-250, côté droit absent)
        int dedans = 0;
        for (size_t k = 0; k + 2 < b.indices.size(); k += 3) {
            const vec3 &A = b.sommets[b.indices[k]].p, &B = b.sommets[b.indices[k + 1]].p, &C = b.sommets[b.indices[k + 2]].p;
            const vec3 n = cross(B - A, C - A);
            dedans += n.x * A.x <= 0;
        }
        VERIFIE(dedans == 0, "%d triangle(s) enroulé(s) vers l'intérieur", dedans);
        // texte à l'endroit : à droite (+x), u diminue vers l'arrière (+z) ; à gauche, u augmente vers l'arrière ; haut en v = 0
        const Sommet& dAv = b.sommets[RANGEES * COLONNES];   // côté droit, avant, en bas
        const Sommet& dAr = b.sommets[RANGEES * COLONNES + COLONNES - 1];
        const Sommet& gAv = b.sommets[0];
        const Sommet& gAr = b.sommets[COLONNES - 1];
        VERIFIE(dAv.p.x > 0 && dAv.u > dAr.u, "côté droit : texte en miroir (u avant %.2f, arrière %.2f)", dAv.u, dAr.u);
        VERIFIE(gAv.p.x < 0 && gAv.u < gAr.u, "côté gauche : texte en miroir (u avant %.2f, arrière %.2f)", gAv.u, gAr.u);
        VERIFIE(b.sommets[0].v == 1.f && b.sommets[(RANGEES - 1) * COLONNES].v == 0.f, "texte à l'envers (v bas %.2f, haut %.2f)",
                b.sommets[0].v, b.sommets[(RANGEES - 1) * COLONNES].v);
        std::printf("caisse d'essai : %zu sommets, %zu triangles, écart à la tôle ≤ %.2f mm, %d point(s) de jour de porte bouché(s)\n",
                    b.sommets.size(), b.indices.size() / 3, (ecartMax + DECALAGE_M) * 1000, b.creux);
    }
    // ---------- 2. pas de tôle, voiture trop courte : aucune bande (jamais un quadrilatère qui flotte) ----------
    {
        VERIFIE(!construire({}, -1.f, 1.f, 0.f, 1.43f).ok(), "bande posée sans aucune tôle");
        VERIFIE(!construire(caisse(false), -0.2f, 0.2f, 0.f, 1.43f).ok(), "bande posée entre deux passages de roue trop proches");
        // tôle d'un seul côté : la bande n'est posée que là
        std::vector<Triangle> moitie;
        for (const Triangle& t : caisse(false)) if (t.a.x > 0) moitie.push_back(t);
        const Bandes b = construire(moitie, -1.f, 1.f, 0.f, 1.43f);
        bool gaucheVide = true; for (const Sommet& s : b.sommets) gaucheVide = gaucheVide && s.p.x > 0;
        VERIFIE(b.ok() && gaucheVide, "tôle d'un seul côté : bande attendue de ce côté seulement");
    }
    // ---------- 3. vraie 208 de la police ----------
    {
        const std::string racine = argc > 1 ? argv[1] : WFS_SOURCE_DIR;
        vehicule::ModeleWfm m; std::string err;
        if (!vehicule::lireWfm(racine + "/assets/cars/p208.wfm", m, err)) { VERIFIE(false, "p208.wfm : %s", err.c_str()); }
        else {
            std::vector<Triangle> tris;
            for (size_t k = 0; k < m.sommets.size(); k++) {
                if (!(m.drapeaux[k] & 1)) continue;   // MAT_PAINT
                const float* v = m.sommets[k].data(); const auto& I = m.indices[k];
                for (size_t t = 0; t + 2 < I.size(); t += 3)
                    tris.push_back({{v[8 * I[t]], v[8 * I[t] + 1], v[8 * I[t] + 2]}, {v[8 * I[t + 1]], v[8 * I[t + 1] + 1], v[8 * I[t + 1] + 2]},
                                    {v[8 * I[t + 2]], v[8 * I[t + 2] + 1], v[8 * I[t + 2] + 2]}});
            }
            std::vector<roues::Maillage> ms;
            for (size_t k = 0; k < m.sommets.size(); k++)
                ms.push_back({m.sommets[k].data(), 8, (uint32_t)(m.sommets[k].size() / 8), m.indices[k].data(), (uint32_t)m.indices[k].size(), (m.drapeaux[k] & 128) != 0});
            const roues::Decoupe d = roues::decouper(ms);
            VERIFIE(d.n == 4, "208 : roues non trouvées");
            if (d.n == 4) {
                const auto& w = d.roues;
                const float zAv = std::max(w[roues::LF].centre.z + w[roues::LF].rayon, w[roues::RF].centre.z + w[roues::RF].rayon);
                const float zAr = std::min(w[roues::LB].centre.z - w[roues::LB].rayon, w[roues::RB].centre.z - w[roues::RB].rayon);
                const float sol = std::min({w[0].centre.y - w[0].rayon, w[1].centre.y - w[1].rayon, w[2].centre.y - w[2].rayon, w[3].centre.y - w[3].rayon});
                const Bandes b = construire(tris, zAv, zAr, sol, 1.43f);   // hauteur de la fiche (cars.h)
                VERIFIE(b.ok() && (int)b.sommets.size() == 2 * RANGEES * COLONNES, "208 : bandes absentes ou d'un seul côté (%zu sommets)", b.sommets.size());
                VERIFIE(b.manques == 0, "208 : %d point(s) sans tôle", b.manques);
                const float L = b.zArriere - b.zAvant;
                VERIFIE(L > 1.5f && L < 1.8f, "208 : longueur de la bande %.2f m (portières : 1,5 à 1,8 m)", L);
                // rien ne flotte : chaque sommet est à 5 mm ± 3 mm de la tôle (rayon relancé depuis le sommet)
                float pire = 0;
                for (const Sommet& s : b.sommets) {
                    float x; vec3 n;
                    if (toucher(tris, s.p.y, s.p.z, s.p.x > 0 ? 1 : -1, x, n)) pire = std::max(pire, std::fabs(std::fabs(s.p.x - x) - DECALAGE_M));
                }
                VERIFIE(pire < 0.003f, "208 : un point de la bande est à %.1f mm de plus que prévu de la tôle", pire * 1000);
                std::printf("208 : bande z %.2f → %.2f m (%.2f m), y %.2f → %.2f m, %d point(s) de jour de porte bouché(s), écart max %.1f mm\n",
                            b.zAvant, b.zArriere, L, b.yBas, b.yHaut, b.creux, pire * 1000);
            }
        }
    }
    if (g_echecs) { std::printf("test_police_bandes : %d échec(s)\n", g_echecs); return 1; }
    std::printf("test_police_bandes : OK\n");
    return 0;
}
