// WaForSpeed — effets de conduite et d'accident (WFS-65, épic E4) : étincelles, fumée, lignes de vitesse sous
// nitro, vignette, flash des chocs (js/world/particles.js et js/world/fx.js du jeu web). Système enfichable (ordre 350).
#pragma once

namespace wfs {
// banc d'essai (rendu hors écran) : effets coupés par défaut pour garder les autres captures inchangées ;
// effetsBanc(true) avant Game::init les active (scènes de bench_effets.cpp)
void effetsBanc(bool on);
int effetsEtincelles();   // étincelles vivantes (journal du banc)
int effetsFumee();        // bouffées de fumée vivantes
int effetsDernierChoc();  // WFS-101 : étincelles émises au dernier choc (journal du banc)
float effetsCapDernierChoc();
// WFS-224 (RG-04) : banc seulement : fumée des pneus forcée comme en vrai dérapage (aspect des volutes de jour et de nuit)
void effetsBancDerapage(bool on);   // WFS-101 : cap de la route (th) au point du dernier choc (caméra de côté du banc)
}
