// WFS-349 (RG-02) : vue Cockpit, habitacle fermé sur les côtés : par le calcul, sur les 20 intérieurs, aucun rayon de l'œil
// ne sort de l'habitacle sous la ligne de ceinture (plan de la vitre latérale) ni sous la planche (pied du pare-brise) sans
// rencontrer une portière, le plancher ou le tablier (boitesFermeture, les boîtes dessinées). Complète le banc
// « cockpit-voisine » (images) pour les fentes fines, que les images ne voient pas.
// Rayons : azimut −80° à +80°, site −70° à +15°, pas de 0,2° ; œil au repos, relevé de 4 cm (WFS-282) et déplacé de ±2 cm
// (mouvement de tête de WFS-235). Cotes réelles (DA-realisme) : portière de 10 à 15 cm, ceinture à ±2 cm du haut de planche,
// matière jamais noire (luminance linéaire ≥ 0,04).
// usage : test_cockpit_ferme
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "../src/render/vehicule_cockpit.h"
using namespace wfs;
using namespace wfs::cockpit;

static int echecs = 0;
static void cas(const char* quoi, bool ok) { std::printf("cockpit fermé : %-86s → %s\n", quoi, ok ? "OK" : "ÉCHEC"); if (!ok) echecs++; }

// distance (paramètre t du rayon o + t·v) à l'entrée d'une boîte, ou −1 si le rayon la manque
static float entree(const BoiteHab& b, vec3 o, vec3 v) {
    float t0 = 0, t1 = 1e9f;
    const float c[3] = {b.centre.x, b.centre.y, b.centre.z}, h[3] = {b.taille.x * 0.5f, b.taille.y * 0.5f, b.taille.z * 0.5f};
    const float oo[3] = {o.x, o.y, o.z}, vv[3] = {v.x, v.y, v.z};
    for (int k = 0; k < 3; k++) {
        if (std::fabs(vv[k]) < 1e-9f) { if (std::fabs(oo[k] - c[k]) > h[k]) return -1; continue; }
        float a = (c[k] - h[k] - oo[k]) / vv[k], z = (c[k] + h[k] - oo[k]) / vv[k];
        if (a > z) std::swap(a, z);
        t0 = std::max(t0, a); t1 = std::min(t1, z);
        if (t0 > t1) return -1;
    }
    return t0;
}

int main() {
    int fuitesTotal = 0;
    for (int i = 0; i < nombreInterieurs(); i++) {
        const InterieurDef& d = interieur(i);
        const Largeur l = largeurPour(d);
        const float cx = l.oeil, demi = l.coudes * 0.5f, yc = ligneCeinture(d), xv = faceVitre(demi);
        const auto boites = boitesFermeture(d, cx, demi);
        long rayons = 0, fuites = 0;
        for (const vec3 o : {vec3{0, 0.04f, 0}, vec3{-0.02f, 0.04f, 0.02f}, vec3{0.02f, 0.04f, -0.02f}, vec3{0, 0.02f, 0}})
            for (float az = -80.f; az <= 80.f; az += 0.2f) for (float si = -70.f; si <= 15.f; si += 0.2f) {
                const float a = az * PI / 180.f, e = si * PI / 180.f;
                const vec3 v{std::sin(a) * std::cos(e), std::sin(e), -std::cos(a) * std::cos(e)};
                // sortie par le plan d'une vitre latérale, sous la ceinture, entre le pied du pare-brise et 0,6 m derrière l'œil
                float tSortie = -1;
                for (const float sx : {-1.f, 1.f}) {
                    const float px = cx + sx * xv;
                    if (v.x * (px - o.x) <= 0) continue;
                    const float t = (px - o.x) / v.x, y = o.y + v.y * t, z = o.z + v.z * t;
                    if (y < yc && z >= d.pareBriseZ && z <= 0.60f) tSortie = t;
                }
                // ou par le plan du pied du pare-brise, sous la planche, entre les deux vitres
                if (tSortie < 0 && v.z < 0) {
                    const float t = (d.pareBriseZ - 0.011f - o.z) / v.z, x = o.x + v.x * t, y = o.y + v.y * t;
                    if (y < d.pareBriseY && std::fabs(x - cx) < xv) tSortie = t;
                }
                if (tSortie < 0) continue;
                rayons++;
                bool arrete = false;
                for (const BoiteHab& b : boites) { const float t = entree(b, o, v); if (t >= 0 && t <= tSortie + 1e-4f) { arrete = true; break; } }
                if (!arrete) fuites++;
            }
        char m[200];
        std::snprintf(m, sizeof m, "%-9s : %ld rayons sortant sous la ceinture ou sous la planche, %ld sans paroi (0 visé)", d.car, rayons, fuites);
        cas(m, rayons > 0 && fuites == 0);
        fuitesTotal += (int)fuites;
        // cotes réelles
        std::snprintf(m, sizeof m, "%-9s : portière %.0f cm d'épaisseur (10 à 15), ceinture %.3f m (planche %.3f, ±0,02)", d.car,
                      EPAISSEUR_PORTE * 100, yc, d.plancheY);
        cas(m, EPAISSEUR_PORTE >= 0.10f && EPAISSEUR_PORTE <= 0.15f && std::fabs(yc - d.plancheY) <= 0.02f);
        // jonctions : portière collée à la planche (face intérieure dans la largeur de la planche), sous le pied du montant
        // (devant le pied du pare-brise d'au moins la demi-largeur du montant), plancher et tablier jusqu'aux portières
        const BoiteHab& p = boites[0];
        const float avPorte = p.centre.z - p.taille.z * 0.5f, intPorte = cx - (p.centre.x + p.taille.x * 0.5f);
        std::snprintf(m, sizeof m, "%-9s : jonctions (portière dans la planche %.3f ≤ %.3f, avant %.3f ≤ %.3f)", d.car, intPorte, demi,
                      avPorte, d.pareBriseZ - LARGEUR_MONTANT * 0.5f);
        cas(m, intPorte <= demi && avPorte <= d.pareBriseZ - LARGEUR_MONTANT * 0.5f && boites[2].taille.x >= 2 * demi - 1e-5f &&
                   boites[3].taille.x >= 2 * demi - 1e-5f);
        const vec3 c = couleurPorte(d);
        std::snprintf(m, sizeof m, "%-9s : portières, luminance linéaire %.3f (≥ 0,04, jamais noir pur)", d.car, 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z);
        cas(m, 0.2126f * c.x + 0.7152f * c.y + 0.0722f * c.z >= 0.04f);
    }
    std::printf("cockpit fermé : %d rayon(s) sans paroi au total ; %d échec(s)\n", fuitesTotal, echecs);
    return echecs ? 1 : 0;
}
