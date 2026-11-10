#pragma once
// WaForSpeed — WFS-161 (ST-6) : mesures de fidélité du kit d'architecture, écrites dans realisme-fidelite.txt (domaine
// « architecture »). Appelé par le banc du réalisme (bench_realisme.cpp) et par test_archi_kit. Valeurs du jeu lues dans le
// kit lui-même (plans assemblés, constantes de la géométrie), comparées au référentiel references/reel/architecture.json.
#include <string>
#include "../render/bench_realisme.h"
#include "archi_kit.h"

namespace wfs::archi {

inline void mesuresArchitecture(const reel::Referentiel& R, reel::Rapport& rap) {
    const char* D = "architecture";
    // M-01 : hauteurs de niveau, un plan type par style (façade de 15 m, hauteur voulue 15 m, graine 1)
    for (int s = 0; s < (int)Style::Nombre; s++) {
        const Plan p = assembler((Style)s, 15.f, 15.f, 1);
        const std::string nom = regles((Style)s).nom;
        rap.comparer(R, D, "archi.rdc.hauteur", p.hRdc, 1, "m", "M-01 : hauteur du rez-de-chaussée (" + nom + ")");
        rap.comparer(R, D, "archi.etage.hauteur", p.hEtage, 1, "m", "M-01 : hauteur d'étage (" + nom + ")");
    }
    // M-02 : ouvertures (plages courantes à sourcer : non mesurables tant que le référentiel n'a pas de valeur)
    const Plan c = assembler(Style::Classique, 15.f, 15.f, 1);
    for (const Baie& b : c.baies) if (b.niveau == 1) {
        rap.comparer(R, D, "archi.fenetre.largeur", b.l, 1, "m", "M-02 : largeur de fenêtre d'étage (classique)");
        rap.comparer(R, D, "archi.fenetre.hauteur", b.h, 1, "m", "M-02 : hauteur de fenêtre d'étage (classique)");
        break;
    }
    rap.comparer(R, D, "archi.garde_corps.hauteur", 1.0, 1, "m", "garde-corps des balcons (archi_geo.cpp, GARDE_CORPS)");
    // WFS-402 (RG-04) : porte d'entrée d'une maison (village) et son passage utile (huisserie de 35 mm, archi_ouvertures.cpp)
    const Plan v = assembler(Style::Village, 9.f, 7.f, 1);
    for (const Baie& b : v.baies) if (b.type == Ouverture::Porte) {
        rap.comparer(R, D, "archi.porte_entree.largeur", b.l, 1, "m", "WFS-402 : porte d'entrée d'une maison (village)");
        rap.comparer(R, D, "archi.porte_entree.passage_utile", b.l - 2 * 0.035, 1, "m", "WFS-402 : passage utile (bloc-porte moins l'huisserie)");
        break;
    }
    // M-03 : albédo moyen des façades = référentiel (béton, brique) ; pierre, crépi, zinc absents du référentiel
    for (const Albedo& a : {ALB_BETON, ALB_BRIQUE}) {
        const reel::Json* m = R.mesure(a.id); const reel::Json* v = m ? m->get("valeur") : nullptr;
        reel::LigneRapport l; l.domaine = D; l.id = a.id; l.grandeur = "M-03 : albédo moyen de la façade (appareil procédural, moyenne conservée)";
        l.jeu = reel::fmt(a.r, 3) + " " + reel::fmt(a.g, 3) + " " + reel::fmt(a.b, 3);
        if (v && v->a.size() == 3) {
            l.reel = reel::fmt(v->a[0].n, 3) + " " + reel::fmt(v->a[1].n, 3) + " " + reel::fmt(v->a[2].n, 3); l.tolerance = "± 10 %"; l.statutReel = m->texte("statut");
            const bool ok = std::fabs(a.r - v->a[0].n) <= 0.1 * v->a[0].n && std::fabs(a.g - v->a[1].n) <= 0.1 * v->a[1].n && std::fabs(a.b - v->a[2].n) <= 0.1 * v->a[2].n;
            l.etat = ok ? reel::Etat::Conforme : reel::Etat::NonConforme;
        } else l.raison = "albédo absent du référentiel";
        rap.ajouter(l);
    }
    rap.nonMesurable(D, "mat.pierre / mat.crepi / mat.zinc", "M-03 : albédo de la pierre, du crépi et du zinc",
                     "absents du référentiel (materiaux.json) : béton provisoire ; textures scannées Poly Haven non accessibles (LFS)");
    // M-04 : luminance d'une fenêtre allumée (photométrie publiée à sourcer)
    rap.comparer(R, D, "archi.fenetre_allumee.luminance", 0.5 * 200.0 / 3.14159265358979, 1, "cd/m²", "M-04 : luminance des murs d'une pièce allumée (conception)");
    rap.comparer(R, D, "archi.interieur.temperature_couleur", 2850, 1, "K", "RG-03 : température de couleur de l'éclairage intérieur");
}

} // namespace wfs::archi
