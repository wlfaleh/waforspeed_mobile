// WaForSpeed — WFS-161 (ST-1) : kit d'architecture, règles d'assemblage (src/world/archi_kit.h).
// M-01 : hauteurs d'étage et de rez-de-chaussée dans les plages de RG-01 ; M-02 (partie règles) : ouvertures dans les plages
// du style, dans leur niveau et dans la façade, sans chevauchement ; déterminisme ; hauteur obtenue proche de la voulue.
#include "../src/world/archi_kit.h"
#include "../src/render/bench_realisme.h"   // référentiel du réel (lecture JSON seulement)
#include <cmath>
#include <cstdio>
#include <string>

using namespace wfs::archi;

static int echecs = 0, controles = 0;
static void verifierQue(bool ok, const char* quoi) { controles++; if (!ok) { if (echecs < 20) std::printf("[ÉCHEC] %s\n", quoi); echecs++; } }

int main() {
    int vus[3][4] = {};   // [rdc / étage / toit][valeur] : chaque module apparaît au moins une fois
    for (int s = 0; s < (int)Style::Nombre; s++) {
        const ReglesStyle& S = regles((Style)s);
        int plans = 0, erreurs = 0; double ecartMax = 0;
        for (uint32_t g = 0; g < 400; g++) {
            const float largeur = 4.f + (float)((g * 37) % 360) / 10.f;   // 4 à 40 m
            const float hauteur = 5.f + (float)((g * 53) % 250) / 10.f;   // 5 à 30 m
            const Plan p = assembler((Style)s, largeur, hauteur, g);
            const char* err = nullptr;
            const int n = verifier(p, &err);
            if (n) { erreurs++; if (erreurs <= 3) std::printf("  %s, graine %u (%.1f × %.1f m) : %s\n", S.nom, g, largeur, hauteur, err); }
            verifierQue(n == 0, "plan conforme aux règles (verifier)");
            // M-01 : hauteurs choisies dans les plages de RG-01
            verifierQue(H_RDC.contient(p.hRdc), "M-01 : rez-de-chaussée 3,5-4,5 m");
            verifierQue(H_ETAGE.contient(p.hEtage), "M-01 : étage 2,7-3,2 m");
            // M-01 : chaque niveau commence exactement où finit le précédent (± 1 % de la hauteur choisie)
            for (int k = 1; k <= p.etages; k++)
                verifierQue(std::fabs(p.niveauY(k + 1) - p.niveauY(k) - p.hEtage) <= 0.01f * p.hEtage, "M-01 : étages réguliers à ± 1 %");
            // M-02 (règles) : ouvertures d'étage dans les plages du style (sauf façade plus étroite qu'une travée)
            for (const Baie& b : p.baies) {
                if (b.niveau == 0) continue;
                verifierQue(S.fenH.contient(b.h), "M-02 : hauteur de fenêtre dans la plage du style");
                verifierQue(S.allege.contient(b.y - p.niveauY(b.niveau)), "M-02 : allège dans la plage du style");
                if (p.travee >= S.fenL.min + 0.3f) verifierQue(S.fenL.contient(b.l), "M-02 : largeur de fenêtre dans la plage du style");
            }
            // déterminisme : même graine → même plan
            const Plan q = assembler((Style)s, largeur, hauteur, g);
            verifierQue(q.baies.size() == p.baies.size() && q.hRdc == p.hRdc && q.hEtage == p.hEtage && q.etages == p.etages && q.toit == p.toit,
                        "même graine, même plan");
            // hauteur : la plus proche de la voulue quand le style le permet
            if (p.etages > S.etagesMin && p.etages < S.etagesMax) {
                const double e = std::fabs(p.hauteur() - hauteur);
                ecartMax = std::max(ecartMax, e);
                verifierQue(e <= p.hEtage / 2 + 1e-3, "hauteur obtenue à moins d'un demi-étage de la voulue");
            }
            vus[0][(int)p.rdc]++; vus[1][(int)p.etage]++; vus[2][(int)p.toit]++;
            plans++;
        }
        std::printf("[%s] %-14s : %d plans, %d non conformes, écart de hauteur max %.2f m\n", erreurs ? "ÉCHEC" : "OK", S.nom, plans, erreurs, ecartMax);
    }
    // référentiel (references/reel/architecture.json) : les plages codées dans archi_kit.h sont celles du référentiel
    {
        wfs::reel::Referentiel R;
        verifierQue(R.charger(std::string(WFS_SOURCE_DIR) + "/references/reel"), "référentiel lisible");
        auto plage = [&](const char* id, Plage p) {
            const wfs::reel::Json* m = R.mesure(id);
            const bool ok = m && std::fabs(m->nombre("min", -1) - p.min) < 1e-4 && std::fabs(m->nombre("max", -1) - p.max) < 1e-4;
            std::printf("[%s] %s : %.2f-%.2f m (kit %.2f-%.2f m)\n", ok ? "OK" : "ÉCHEC", id, m ? m->nombre("min", -1) : -1, m ? m->nombre("max", -1) : -1, p.min, p.max);
            verifierQue(ok, "plage du kit = plage du référentiel");
        };
        plage("archi.rdc.hauteur", H_RDC);
        plage("archi.etage.hauteur", H_ETAGE);
        // cohérence : un étage de la plage laisse la hauteur sous plafond minimale sous un plancher de 0,3 m
        const wfs::reel::Json* hsp = R.mesure("archi.logement.hauteur_sous_plafond_min");
        verifierQue(hsp && H_ETAGE.min - 0.3 >= hsp->nombre("valeur", 99), "étage minimal − plancher ≥ hauteur sous plafond minimale");
    }
    for (int r = 0; r < 4; r++) verifierQue(vus[0][r] > 0, "chaque type de rez-de-chaussée apparaît (RG-01)");
    for (int e = 0; e < 3; e++) verifierQue(vus[1][e] > 0, "chaque type d'étage apparaît (RG-01)");
    for (int t = 0; t < 3; t++) verifierQue(vus[2][t] > 0, "chaque toiture apparaît (RG-01)");
    std::printf("archi_kit : %d contrôles, %d échec(s)\n", controles, echecs);
    return echecs ? 1 : 0;
}
