// WaForSpeed — boutons de l'interface au modèle R19 (WFS-549, épic E23) : un seul dessin commun pour tous les écrans.
// Maquettes : references/maquettes-R19/ (01 menu, 14 Réglages, 19 pause, 27 achat…) ; règles : regles-de-gestion/WFS-549.md.
//   - startStop     : action principale de l'écran (RG-01), bouton de démarrage d'une voiture : bague chrome, centre sombre,
//                     liseré orange allumé, icône « marche » et texte en majuscules ;
//   - autocollant   : actions de sortie (Retour, Quitter, Annuler, Plus tard ; RG-02), étiquette blanche inclinée ;
//   - piluleChrome  : « Showroom / Garage » du menu (RG-03), bague chrome et liseré cyan ;
//   - prix          : boutons de prix (« ★ 6 000 », « 1,99 € », Inviter ; RG-04), orange, texte sombre ;
//   - selecteur     : rangée de verre d'un choix à plusieurs valeurs (RG-05, RG-06), choix actif = pilule blanche, texte sombre ;
//   - verreOrange   : bouton de verre cerclé d'orange (« ✕ Fermer » des Réglages, RG-06).
// Chaque fonction dessine DANS le rectangle (x, y, w, h) donné : la zone de toucher et la taille de chaque bouton restent celles
// de l'écran qui l'appelle (RG-07). L'appui (RG-08) se montre par `appuye` (bouton enfoncé : centre plus clair, liseré plus vif) ;
// le déclenchement unique reste celui de chaque écran (src/ui/appui.h). Interface 2D : skill design-street-racing.
// Couleurs en codes sRGB 0..1 (comme Renderer::uiRect) ; op = opacité de l'écran (fondus d'entrée).
#pragma once
#include <string>
#include "../core/math.h"

namespace wfs {
class Renderer;
namespace ui::boutons {

// contour d'un rectangle aux coins arrondis de rayon `rayon` (borné à min(w, h) / 2 : pilule), sommets dans le sens horaire
// à l'écran ; rend le nombre de sommets écrits dans p (MAX_SOMMETS au plus). Calcul seul : testé par tests/test_boutons.cpp.
constexpr int MAX_SOMMETS = 64;
int contourArrondi(vec2* p, float x, float y, float w, float h, float rayon);

// RG-01 : titre (une ligne, capitales) ; sousTitre facultatif (« ★ 6 000 », « Solo ») ; px = hauteur du titre (0 : d'après h)
void startStop(Renderer& r, float x, float y, float w, float h, const std::string& titre, float op = 1, bool appuye = false,
               const std::string& sousTitre = "", float px = 0, bool grise = false);
// RG-02 : texte (capitales) ; sousTitre facultatif (« Garder mes étoiles »)
void autocollant(Renderer& r, float x, float y, float w, float h, const std::string& texte, float op = 1, bool appuye = false,
                 float px = 0, const std::string& sousTitre = "");
// RG-03 : pilule chrome (liseré cyan, petite voiture dessinée au trait à gauche)
void piluleChrome(Renderer& r, float x, float y, float w, float h, const std::string& texte, float op = 1, bool appuye = false,
                  float px = 0, const std::string& sousTitre = "");
// RG-04 : bouton de prix orange, texte sombre (le texte peut contenir « ★ », dessinée par l'appelant s'il le faut)
void prix(Renderer& r, float x, float y, float w, float h, const std::string& texte, float op = 1, bool appuye = false,
          float px = 0, bool grise = false);
// RG-05 : rangée de verre (fond seul) puis un choix ; le choix actif est une pilule blanche à texte sombre
void selecteurFond(Renderer& r, float x, float y, float w, float h, float op = 1);
void selecteurChoix(Renderer& r, float x, float y, float w, float h, const std::string& texte, bool actif, float op = 1,
                    bool appuye = false, float px = 0, bool grise = false);
// RG-06 : bouton de verre cerclé d'orange (texte orange)
void verreOrange(Renderer& r, float x, float y, float w, float h, const std::string& texte, float op = 1, bool appuye = false,
                 float px = 0);
// bouton de verre neutre (bord clair) : actions secondaires des maquettes (« Crédits », « Conditions d'utilisation »)
void verre(Renderer& r, float x, float y, float w, float h, const std::string& texte, float op = 1, bool appuye = false,
           float px = 0, const vec4* couleurTexte = nullptr);

} // namespace ui::boutons
} // namespace wfs
