// WaForSpeed — WFS-410 : contrôles des façades de lieux hors kit (archi_lieux.h), sans Vulkan.
//   1. le mur est vraiment percé : aucun triangle du mur dans une baie, aire du mur = façade − baies (±0,1 %) ;
//   2. chaque baie a sa menuiserie réelle de WFS-402 au niveau 0, même interrupteur de WFS-415 coupé (jeu, APK) ;
//   3. niveau 2 : mur plein, aucune menuiserie (loin) ;
//   4. arcs en plein cintre : aucun triangle du mur dans l'ouverture (rectangle + demi-cercle), claveaux, vitrail ou porte ;
//   5. versMonde : repère direct (normales unitaires, la façade regarde z) ;
//   6. budget : triangles d'une façade de casino de 44 m (33 baies) au niveau 0 sous 60 000 ;
//   7. église de la Campagne (RG-04) : nef 9 × 17 m, portail 2,2 × 4,4 m (±1 %), clocher-mur, budget, aucun triangle dégénéré.
#include <cmath>
#include <cstdio>
#include "../src/world/archi_lieux.h"
#include "../src/world/decor_campagne_eglise.h"

using namespace wfs;
using namespace wfs::archi;

static int g_err = 0;
#define VERIFIER(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); g_err++; } } while (0)

static double aire(const Maillage& m, vec3 n) {
    double a = 0;
    for (size_t k = 0; k + 2 < m.i.size(); k += 3) {
        const vec3 p = m.v[m.i[k]].p, q = m.v[m.i[k + 1]].p, r = m.v[m.i[k + 2]].p;
        const vec3 c = cross(q - p, r - p);
        if (dot(m.v[m.i[k]].n, n) > 0.99f) a += 0.5 * length(c);
    }
    return a;
}

int main() {
    choisirOuverturesReelles(false);   // comme le jeu (WFS-415) : les lieux gardent leurs menuiseries réelles quand même
    lieux::Facade f;
    f.style = Style::Classique; f.graine = 7; f.largeur = 12; f.y0 = 0; f.y1 = 8;
    f.baies = {{Ouverture::Vitrine, 3.f, 0.4f, 4.f, 2.8f, 0.4f}, {Ouverture::Porte, 7.f, 0.2f, 1.1f, 2.3f, 0.2f},
               {Ouverture::Fenetre, 2.f, 4.6f, 1.2f, 1.6f, 3.6f}, {Ouverture::Fenetre, 6.f, 4.6f, 1.2f, 1.6f, 3.6f},
               {Ouverture::Fenetre, 10.f, 4.6f, 1.2f, 1.6f, 3.6f}};
    Batiment b;
    lieux::construire(f, 0, b);
    // 1. mur percé
    double trous = 0;
    for (const lieux::Ouv& o : f.baies) trous += o.l * o.h;
    const double am = aire(b.part[MUR], {0, 0, 1}), attendu = f.largeur * (f.y1 - f.y0) - trous;
    VERIFIER(std::fabs(am - attendu) < attendu * 1e-3, "aire du mur %.3f m², attendu %.3f m²", am, attendu);
    for (size_t k = 0; k + 2 < b.part[MUR].i.size(); k += 3) {
        vec3 c{0, 0, 0};
        for (int j = 0; j < 3; j++) c = c + b.part[MUR].v[b.part[MUR].i[k + (size_t)j]].p;
        c = c * (1.f / 3);
        if (std::fabs(c.z) > 1e-3f) continue;
        for (const lieux::Ouv& o : f.baies)
            VERIFIER(!(c.x > o.x - o.l / 2 + 1e-3f && c.x < o.x + o.l / 2 - 1e-3f && c.y > o.y + 1e-3f && c.y < o.y + o.h - 1e-3f),
                     "triangle du mur dans la baie en x = %.2f, y = %.2f", c.x, c.y);
    }
    // 2. menuiseries réelles (dormants et ouvrants dans CHASSIS, porte ou soubassement dans MENUISERIE, quincaillerie METAL)
    VERIFIER(b.part[CHASSIS].triangles() > 200, "menuiseries des fenêtres absentes (%zu triangles)", b.part[CHASSIS].triangles());
    VERIFIER(b.part[MENUISERIE].triangles() + b.part[METAL].triangles() > 100, "porte et vitrine sans menuiserie");
    VERIFIER(b.part[VITRE].triangles() > 0, "aucune vitre");
    VERIFIER(b.part[ENCADREMENT].triangles() >= 5 * 8, "tableaux absents");
    std::printf("façade de lieu 12 × 8 m, 5 baies : %zu triangles au niveau 0\n", lieux::triangles(b));
    {   // 2 bis. WFS-410 (B1) : vitrines et portes à part (pièce de nuit des commerces), fenêtres d'étage dans le bâtiment ; uv de la pièce de nuit
        Batiment b2, v2;
        lieux::construire(f, 0, b2, &v2);
        VERIFIER(v2.part[VITRE].triangles() > 0 && b2.part[VITRE].triangles() > 0, "vitrines et fenêtres d'étage non séparées");
        VERIFIER(b2.part[MUR].triangles() == b.part[MUR].triangles(), "le mur doit rester entier dans le bâtiment");
        bool uv = true;
        for (const Sommet& s : b2.part[VITRE].v) uv = uv && s.uv.x >= 100.f && s.uv.y >= 100.f;   // largeur × 1000 + allège, graine × 100 + hauteur
        VERIFIER(uv, "vitre sans les uv de la pièce de nuit");
    }
    // 3. loin : mur plein, baies peintes
    Batiment loin;
    lieux::construire(f, 2, loin);
    VERIFIER(loin.part[CHASSIS].triangles() == 0 && loin.part[ENCADREMENT].triangles() == 0, "niveau 2 : menuiseries ou tableaux construits");
    VERIFIER(std::fabs(aire(loin.part[MUR], {0, 0, 1}) - f.largeur * (f.y1 - f.y0)) < 1e-2, "niveau 2 : mur non plein");
    // 4. arcs
    Batiment eg;
    const std::vector<lieux::Arc> arcs = {{4.f, 0.f, 2.f, 4.f, true}, {8.f, 3.f, 0.8f, 2.f, false}};
    lieux::murArcs(10.f, 0.f, 9.f, arcs, 0.45f, 0, eg);
    for (size_t k = 0; k + 2 < eg.part[MUR].i.size(); k += 3) {
        vec3 c{0, 0, 0};
        for (int j = 0; j < 3; j++) c = c + eg.part[MUR].v[eg.part[MUR].i[k + (size_t)j]].p;
        c = c * (1.f / 3);
        for (const lieux::Arc& a : arcs) {
            const float r = a.l / 2, yn = a.y + a.h - r;
            const bool dans = std::fabs(c.x - a.x) < r - 0.02f && c.y > a.y + 0.02f &&
                              (c.y < yn || std::hypot(c.x - a.x, c.y - yn) < r - 0.05f);
            VERIFIER(!dans, "arc : triangle du mur dans l'ouverture (%.2f, %.2f)", c.x, c.y);
        }
    }
    VERIFIER(eg.part[MENUISERIE].triangles() > 20, "portail sans vantaux de planches");
    VERIFIER(eg.part[VITRE].triangles() > 0, "baie sans vitrail");
    VERIFIER(eg.part[METAL].triangles() > 20, "pentures ou barlotières absentes");
    VERIFIER(eg.part[ENCADREMENT].triangles() > 100, "claveaux et ébrasement absents");
    // 5. versMonde
    Batiment w;
    lieux::construire(f, 1, w);
    const vec3 z = normalize(vec3{0.6f, 0, -0.8f}), x = lieux::axeFacade(z);
    VERIFIER(std::fabs(dot(cross(x, vec3{0, 1, 0}), z) - 1) < 1e-5f, "repère de façade non direct");
    lieux::versMonde(w.part[MUR], {100, 2, -50}, x, z);
    for (const Sommet& s : w.part[MUR].v) {
        VERIFIER(std::fabs(length(s.n) - 1) < 1e-4f, "normale non unitaire");
        VERIFIER(dot(s.n, z) > 0.999f, "le mur ne regarde plus la rue");
        if (g_err) break;
    }
    // 6. budget d'un casino (11 travées de 4 m, 3 niveaux de baies)
    size_t tri = 0;
    for (int k = 0; k < 11; k++) {
        lieux::Facade c;
        c.style = Style::Classique; c.graine = (uint32_t)k; c.largeur = 4; c.y0 = 0; c.y1 = 13;
        c.baies = {{Ouverture::Fenetre, 2.f, 1.f, 2.f, 3.4f, 0.6f}, {Ouverture::Fenetre, 2.f, 6.1f, 1.4f, 2.3f, 5.3f}, {Ouverture::Fenetre, 2.f, 10.1f, 1.1f, 1.5f, 9.3f}};
        Batiment x0; lieux::construire(c, 0, x0); tri += lieux::triangles(x0);
    }
    std::printf("casino de 11 travées : %zu triangles au niveau 0\n", tri);
    VERIFIER(tri < 60000, "casino trop lourd : %zu triangles", tri);
    // 7. église
    {
        campagne::CouleursEglise c;
        c.pierre = 1; c.pierre2 = 2; c.tuile = 3; c.porte = 4; c.fer = 5; c.bronze = 6; c.bois = 7; c.vitrail[0] = 8; c.vitrail[1] = 9; c.vitrail[2] = 10;
        for (int loin = 0; loin < 2; loin++) {
            const campagne::Eglise e = campagne::eglise(c, loin != 0);
            float xmin = 1e9f, xmax = -1e9f, zmin = 1e9f, ymax = -1e9f;
            for (const marina::Sommet& s : e.m.v) if (s.couleur == c.pierre && std::fabs(s.p.z) < campagne::EGLISE_LONGUEUR / 2 - 0.1f) { xmin = std::min(xmin, s.p.x); xmax = std::max(xmax, s.p.x); }
            for (const marina::Sommet& s : e.m.v) if (s.couleur == c.pierre) zmin = std::min(zmin, s.p.z);
            for (const marina::Sommet& s : e.m.v) ymax = std::max(ymax, s.p.y);
            VERIFIER(std::fabs((xmax - xmin) - campagne::EGLISE_LARGEUR) < campagne::EGLISE_LARGEUR * 0.01f, "église : largeur de la nef %.2f m", xmax - xmin);
            VERIFIER(std::fabs(-zmin * 2 - campagne::EGLISE_LONGUEUR) < campagne::EGLISE_LONGUEUR * 0.01f, "église : longueur de la nef %.2f m", -zmin * 2);
            VERIFIER(ymax > campagne::EGLISE_CLOCHER_H + 1.5f && ymax < campagne::EGLISE_CLOCHER_H + 2.5f, "église : haut de la croix %.2f m", ymax);
            // portail : la porte (vantaux) va de y = 0 à 4,4 m et de z = −1,1 à 1,1 m, au fond de l'ébrasement
            float pz0 = 1e9f, pz1 = -1e9f, py1 = -1e9f;
            for (const marina::Sommet& s : e.m.v) if (s.couleur == c.porte) { pz0 = std::min(pz0, s.p.z); pz1 = std::max(pz1, s.p.z); py1 = std::max(py1, s.p.y); }
            VERIFIER(std::fabs((pz1 - pz0) - campagne::EGLISE_PORTAIL_L) < 0.03f, "église : portail de %.2f m de large", pz1 - pz0);
            VERIFIER(std::fabs(py1 - campagne::EGLISE_PORTAIL_H) < 0.05f, "église : portail de %.2f m de haut", py1);
            VERIFIER(!e.vitraux.v.empty(), "église sans vitraux");
            int deg = 0;
            for (const marina::Maille* mm : {&e.m, &e.vitraux})
                for (size_t k = 0; k + 2 < mm->i.size(); k += 3) {
                    const vec3 a = mm->v[mm->i[k]].p, b = mm->v[mm->i[k + 1]].p, d = mm->v[mm->i[k + 2]].p;
                    if (length(cross(b - a, d - a)) < 1e-9f) deg++;
                }
            VERIFIER(deg == 0, "église : %d triangle(s) dégénéré(s)", deg);
            const int tri = e.m.triangles() + e.vitraux.triangles();
            std::printf("église (%s) : %d triangles\n", loin ? "loin" : "près", tri);
            VERIFIER(tri < (loin ? 8000 : 40000), "église trop lourde : %d triangles", tri);
        }
    }
    std::printf(g_err ? "test_archi_lieux : %d échec(s)\n" : "test_archi_lieux : tout est bon\n", g_err);
    return g_err ? 1 : 0;
}
