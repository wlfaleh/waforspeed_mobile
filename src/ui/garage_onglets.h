// WaForSpeed — garage : onglets et bouton rond (WFS-551, épic E23 interface R19, maquettes 06, 07, 13, 15, 16).
// Le garage suit les maquettes : en haut au centre, quatre onglets « VOITURE · PEINTURE · FINITION · SKINS » (l'onglet ouvert
// en néon cyan, les autres en plaques claires à liseré orange) ; en bas, un seul panneau de verre, celui de l'onglet ouvert
// (fiche de la voiture, gouttes, pistolets ou pastilles) ; à droite, le grand bouton rond Start/Stop (« Choisir » ou « Acheter »
// avec son prix), le nom de l'objet montré au-dessus et le solde d'étoiles dessous ; « Retour » en autocollant en bas à gauche.
#pragma once
#include <string>
#include "garage_commun.h"

namespace wfs::ecrans::garage::onglets {

// onglets en haut au centre ; pose les zones g_z.ongletsGarage
void dessiner(Renderer& r, const MiseEnPage& mp);
int appui(float x, float y);   // onglet touché, −1 si aucun
void viderZones();
// grand bouton rond (Start/Stop de WFS-549, forme ronde) : titre, sous-titre facultatif (peut contenir « ★ ») ; grisé = inactif
void boutonRond(Renderer& r, float cx, float cy, float R, const std::string& titre, const std::string& sousTitre, float op, bool grise);

} // namespace wfs::ecrans::garage::onglets
