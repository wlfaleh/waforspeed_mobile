// WaForSpeed — banc « textes » des menus (WFS-505, épic E20) : accueil, choix du mode, choix du terrain, chargement, pause,
// salon du duel et crédits, dans la langue de WFS_LANGUE (téléphone simulé). Chaque écran note ses propres textes au journal
// de WFS-504 (ui::langue::noter, par ui::text) quand le banc le demande ; puis le garde-fou vérifie qu'aucune phrase française
// ni aucun texte qui dépasse de son bouton n'apparaît en anglais (ui::langue::verifierJournal).
#pragma once
#include <string>
#include "../langue.h"

namespace wfs {
class Game;
class Renderer;
namespace ecrans {
namespace textes {

// nom de l'écran dont les textes sont notés (vide : rien n'est noté, hors banc) ; posé par le banc autour de ses images
std::string& ecranNote();
// à poser au début du dessin d'un écran du ticket : ouvre le journal de WFS-504 pour cet écran, le referme à la fin
struct Journal {
    Journal() { if (!ecranNote().empty()) ui::langue::journalActif(true, ecranNote().c_str()); }
    ~Journal() { if (!ecranNote().empty()) ui::langue::journalActif(false); }
    Journal(const Journal&) = delete;
    Journal& operator=(const Journal&) = delete;
};

// nom propre (carte, voiture : inchangés en anglais, RC-E20-03) : noté à part au journal (ligne non vérifiée), pas comme une phrase
struct NomPropre {
    NomPropre() { ui::langue::journalTechnique(true); }
    ~NomPropre() { ui::langue::journalTechnique(false); }
    NomPropre(const NomPropre&) = delete;
    NomPropre& operator=(const NomPropre&) = delete;
};

// banc WFS_BANC=textes (après les Réglages de WFS-504) : captures textes-<fr|en>-<écran> et parcours complet ; rend le nombre d'échecs
int benchMenusTextes(Game& g, Renderer& r, const std::string& out);

} // namespace textes
} // namespace ecrans
} // namespace wfs
