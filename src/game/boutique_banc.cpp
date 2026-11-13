// WaForSpeed — banc d'équilibrage des prix (WFS-498, RG-11) : WFS_BANC=equilibrage, sans rendu ; voir boutique.h.
// Fichier à part (moteur et banc seulement) : il lit configgame.json (chargerConfig), que les tests CPU ne lient pas tous.
#include <filesystem>
#include <string>
#include <vector>
#include "../core/configgame.h"
#include "../core/log.h"
#include "boutique.h"

namespace wfs::boutique {

int benchEquilibrage(const std::string& out) {
    const BilanConfig bc = chargerConfig();   // PC : WFS_CONFIG, sinon ./configgame.json (un prix changé compte sans recompiler)
    WFS_LOG("ÉQUILIBRAGE : configuration %s (%d clés lues, %d erreur(s))", bc.fichier.empty() ? "du code (aucun fichier)" : bc.fichier.c_str(), bc.lues, bc.erreurs);
    const Equilibrage e = equilibrage();
    std::vector<std::string> lignes = bilanEquilibrage(e);
    const std::vector<std::string> ecarts = verifierEquilibrage(e);
    for (const std::string& l : ecarts) lignes.push_back("ÉQUILIBRAGE ÉCART : " + l);
    lignes.push_back(ecarts.empty() ? "ÉQUILIBRAGE : OK (bornes de RG-11 tenues)" : "ÉQUILIBRAGE : ÉCHEC (" + std::to_string(ecarts.size()) + " écart(s))");
    std::string texte;
    for (const std::string& l : lignes) { WFS_LOG("%s", l.c_str()); texte += l + "\n"; }
    std::error_code ec; std::filesystem::create_directories(out, ec);   // dossier des captures du banc (absent au 1er lancement)
    const std::string chemin = out + "/equilibrage.txt";
    if (!ecrireAtomique(chemin, texte)) WFS_ERR("équilibrage : %s non écrit", chemin.c_str());
    return (int)ecarts.size();
}

} // namespace wfs::boutique
