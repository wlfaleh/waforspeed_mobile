#pragma once
#include <string>
#include "renderer.h"

namespace wfs {
// charge une voiture convertie (.wfm) depuis assets/
// WFS-446 (RG-02) : repli = vrai (trafic seulement) : tampons et textures par la voie de repli de WFS-418 (VkCtx::essayer…) ; une
// réservation refusée par la carte graphique rend tout ce qui a été pris pour ce modèle et renvoie faux (out.ok = faux : modèle sauté),
// au lieu d'arrêter le jeu. Faux par défaut : voiture du joueur, adversaires, fantôme et duel inchangés (WFS-448)
bool loadCarModel(Renderer& r, const std::string& path, Model& out, bool repli = false);
}
