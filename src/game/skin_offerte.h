// WaForSpeed — skins : skin offerte par voiture, skins données (cadeau du jour, succès), bons de skin (WFS-494, épic E18, zone E3).
// Règles seules, sans écran (le garage les montre : src/ui/garage_skin.*). Voir regles-de-gestion/WFS-494.md.
//   - RG-02 : chaque voiture possédée qui a des skins donne droit à UNE skin offerte, choisie parmi les skins en vente
//     (jamais une skin « cadeau ») ; les voitures gratuites (208, 500, GT86) dès le départ, les autres à leur achat ;
//     le droit utilisé est gardé dans la sauvegarde (clé skins-offertes=<voiture>|<voiture>) ;
//   - RG-03 : une skin possédée vaut pour une seule voiture (possede-<voiture>, catalogue de WFS-487) ;
//   - RG-04 : donnerSkin(voiture, skin) : pour le cadeau du jour et les succès (WFS-503), toute skin de la voiture, « cadeau » comprise ;
//   - pack débutant du market (WFS-510 RG-02) : un bon de skin = une skin en vente au choix, pour une voiture possédée.
// Aucune de ces fonctions n'écrit le fichier (sauf donnerSkin sans sauvegarde en argument) : l'appelant appelle enregistrer().
// Thread principal (comme la sauvegarde).
#pragma once
#include <string>
#include <vector>
#include "personnalisation.h"
#include "sauvegarde.h"

namespace wfs::skins {

bool aDesSkins(const std::string& carId);                         // au moins une skin du catalogue existe pour cette voiture
std::vector<const perso::Objet*> enVente(const std::string& carId);   // skins de la voiture qu'on peut acheter (ni « cadeau »)

// RG-02 : la voiture affichée attend son choix de skin offerte (sauvegarde active, voiture possédée, droit pas encore utilisé,
// au moins une skin en vente pas encore possédée)
bool offerteDue(const Sauvegarde& s, const std::string& carId);
// donne la skin offerte : skin en vente pour cette voiture et offerteDue ; la skin devient possédée et choisie (skin-<voiture>),
// le droit est noté ; faux sinon (rien de changé)
bool offrir(Sauvegarde& s, const std::string& carId, const std::string& skinId);

// RG-04 : donne une skin (cadeau du jour, succès) : skin de cette voiture, pas encore possédée ; faux sinon (rien de changé).
// Elle n'est pas choisie : le joueur la pose au garage.
bool donnerSkin(Sauvegarde& s, const std::string& carId, const std::string& skinId);
// même chose sur la sauvegarde du jeu, écrite si elle est active (appel du cadeau du jour et des succès, WFS-503)
bool donnerSkin(const std::string& carId, const std::string& skinId);

// bon de skin (market, pack débutant) : skin en vente, pas encore possédée, voiture possédée, au moins un bon ; le bon est
// utilisé, la skin devient possédée et choisie ; faux sinon (rien de changé)
bool bonUtilisable(const Sauvegarde& s, const std::string& carId, const std::string& skinId);
bool utiliserBon(Sauvegarde& s, const std::string& carId, const std::string& skinId);

} // namespace wfs::skins
