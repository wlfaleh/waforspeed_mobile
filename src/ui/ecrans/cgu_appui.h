// WaForSpeed — appuis de l'écran des CGU avant le jeu (WFS-355, ANO-199) : calculs seuls, testés par tests/test_cgu_appui.cpp.
//   - destination : à qui va un appui avant le jeu (onInput ET appuis rejoués du veilleur, android_main.cpp, même ordre).
//     Cause n° 1 de WFS-355 : les appuis lus par le veilleur (WFS-284, image de plus d'1 s) n'étaient rejoués qu'au jeu ou à
//     l'écran d'erreur : sur les CGU et les paquets ils étaient perdus (pose ou lever d'« Accepter » jamais reçu).
//   - cibleSous : bouton visé, cible élargie à 48 dp de haut et 4 dp de chaque côté (appui::dansCible, comme les Réglages) ;
//   - cibleAuLever : appui terminé. Pose perdue (lever seul) : le bouton sous le doigt au lever compte quand même.
#pragma once
#include "reglages_appui.h"

namespace wfs::ecrans::cgu_appui {

enum class Destination { Rien, Jeu, Erreur, Paquets, Cgu };

// erreurChargement : écran d'erreur du chargement ; paquetsTient / cguPremier : l'écran tient la fenêtre ; pret : jeu chargé
inline Destination destination(bool erreurChargement, bool paquetsTient, bool cguPremier, bool pret) {
    if (paquetsTient && !pret) return Destination::Paquets;
    if (cguPremier && !pret) return Destination::Cgu;
    if (erreurChargement) return Destination::Erreur;
    return pret ? Destination::Jeu : Destination::Rien;
}

enum { RIEN = 0, TEXTE = 1, CASE = 2, ACCEPTER = 3, REFUSER = 4, FERMER = 5 };
struct Rect { float x = 0, y = 0, w = 0, h = 0; };

inline bool dans(const Rect& r, float dp, float x, float y) { return appui::dansCible(r.x, r.y, r.w, r.h, dp, x, y); }
inline bool dansTexte(const Rect& r, float x, float y) { return r.w > 0 && x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h; }

// bouton sous le doigt (zones non dessinées : w = 0) ; les boutons passent avant le texte
inline int cibleSous(const Rect& caseZ, const Rect& accepter, const Rect& refuser, const Rect& fermer, const Rect& texte, float dp, float x, float y) {
    if (dans(caseZ, dp, x, y)) return CASE;
    if (dans(accepter, dp, x, y)) return ACCEPTER;
    if (dans(refuser, dp, x, y)) return REFUSER;
    if (dans(fermer, dp, x, y)) return FERMER;
    return dansTexte(texte, x, y) ? TEXTE : RIEN;
}

// cible de l'appui au lever : posé sur un bouton et levé sur le même → ce bouton ; pose jamais reçue → bouton sous le doigt ;
// doigt glissé hors du bouton → rien (l'appui est annulé, comme sur Android)
inline int cibleAuLever(bool poseConnue, int cibleDuPose, int cibleSousLeDoigt) {
    if (cibleSousLeDoigt == TEXTE) return RIEN;
    if (!poseConnue) return cibleSousLeDoigt;
    return cibleDuPose == cibleSousLeDoigt ? cibleDuPose : RIEN;
}

} // namespace wfs::ecrans::cgu_appui
