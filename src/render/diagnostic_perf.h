// WaForSpeed — WFS-419 : interrupteurs de diagnostic de performance (mesure sur le téléphone, aucune correction).
// Chaque interrupteur coupe UN SEUL effet du rendu, pour que l'utilisateur mesure ce qu'il coûte sur son téléphone en
// regardant la ligne technique de Réglages. Tous ÉTEINTS par défaut (RG-01) : le jeu est alors identique à main.
// Deux sources, l'une OU l'autre allume l'interrupteur (RG-02) :
//   - le menu caché de Réglages (5 touchers rapides sur le titre « Réglages ») : état en mémoire, perdu à la fermeture du jeu ;
//   - la section « diagnostic » de configgame.json (0 / 1), relue à chaque partie.
// Coût en jeu normal (RG-05) : une lecture de tableau par accroche (aucun calcul quand tout est éteint).
#pragma once
#include <string>

namespace wfs {
class VkCtx;
namespace diag {

enum Interrupteur : int {
    MSAA_X1 = 0,       // MSAA ×1 (au lieu de ×2 en Basse, ×4 sinon) : cibles de la scène recréées
    SANS_PREPASSE,     // pré-passe de profondeur (WFS-150) non faite
    SANS_REFLET,       // reflet de la route mouillée : passe miroir coupée, chaussée sans reflet de la scène
    SANS_OMBRES,       // ombres du soleil (passe d'ombre vide)
    SANS_PLUIE,        // pluie : gouttes à l'écran, gouttes sur la vitre, flaques
    SANS_HALOS,        // halos additifs (feux, lampadaires, lumière au sol)
    SANS_POST,         // bloom et halo du post-traitement (le tonemapping reste : image lisible)
    SANS_LUMIERES,     // 0 lumière de nuit (phares, lampadaires, feux des autres voitures)
    RESOLUTION_30,     // scène rendue à 30 % de la taille de l'écran : cibles recréées
    COULEUR_UNIE,      // shader « couleur unie » : chaque objet de sa couleur de base, sans texture ni lumière
    SANS_DECOR,        // décor coupé (bâtiments, végétation, mobilier) ; route, sol, trottoirs et voitures gardés
    DISTANCE_DEMI,     // distance d'affichage ÷ 2 (tous les objets, route comprise)
    N_INTERRUPTEURS
};

struct Info {
    const char* cle;       // clé de configgame.json (section diagnostic)
    const char* libelle;   // texte du menu caché
    const char* court;     // texte de la ligne technique
    bool recreeCibles;     // vrai : changer l'interrupteur recrée les cibles de la scène (Renderer::resize)
};
inline constexpr Info INFOS[N_INTERRUPTEURS] = {
    {"msaa_x1", "MSAA ×1", "MSAA1", true},
    {"sans_prepasse", "Sans pré-passe", "-prépasse", false},
    {"sans_reflet", "Sans reflet route mouillée", "-reflet", false},
    {"sans_ombres", "Sans ombres", "-ombres", false},
    {"sans_pluie", "Sans pluie (gouttes, flaques)", "-pluie", false},
    {"sans_halos", "Sans halos additifs", "-halos", false},
    {"sans_post", "Sans bloom ni post-traitement", "-post", false},
    {"sans_lumieres", "0 lumière de nuit", "-lumières", false},
    {"resolution_30", "Résolution 30 %", "30%", true},
    {"couleur_unie", "Shader couleur unie", "unie", false},
    {"sans_decor", "Décor coupé", "-décor", false},
    {"distance_demi", "Distance d'affichage ÷ 2", "dist/2", false},
};

// vrai si l'interrupteur est allumé (menu caché OU configgame.json)
bool actif(Interrupteur i);
bool menu(Interrupteur i);              // état du menu caché seul
void basculerMenu(Interrupteur i);      // menu caché : allume / éteint
void eteindreMenu();                    // menu caché : tout éteint
bool aucun();                           // vrai si tout est éteint (jeu identique à main)
// « diag : MSAA1 -ombres » (vide si tout est éteint)
std::string texteActifs();
// RG-03 : modèle du téléphone, GPU, version du pilote et de Vulkan (calculé une fois)
std::string texteAppareil(const VkCtx& vk);
// RG-04 : pourquoi « GPU ombre 0,0 · reflet 0,0 » (selon l'état du rendu au moment de la lecture)
std::string explicationZero(bool qualiteBasse, bool horodatages);
// menu caché ouvert au moins une fois : la ligne technique montre l'appareil et le coût de chaque passe
inline bool g_menuVu = false;

} // namespace diag
} // namespace wfs
