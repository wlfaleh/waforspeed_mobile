// WaForSpeed — données du market gardées dans la sauvegarde (WFS-510, épic E21, zone E3). Les règles sont dans market.h ;
// ici seulement les valeurs, lues et écrites par sauvegarde.cpp (nouvelles clés, aucune ancienne retirée) :
//   achats-credites=<jeton>|<jeton>…   livre des achats (RG-03) : jetons des achats déjà crédités, du plus ancien au plus récent, 200 au plus
//   pack-debutant=1                    pack débutant acheté (RG-02 : plus jamais proposé)
//   bons-skin=<n>                      bons de skin à utiliser au garage (WFS-494)
//   sans-pub=1                         « sans pub » possédé (RG-05 ; gardé pour marcher sans réseau)
//   x2=<AAAAMMJJ>;<nombre>;<course>    ×2 de fin de course (RG-06) : jour le plus récent vu, ×2 donnés ce jour-là, n° de la dernière course doublée
// Valeur mal écrite : défaut de sa clé, sans erreur (comme toute la sauvegarde).
#pragma once
#include <string>
#include <vector>

namespace wfs {

struct EtatMarket {
    static constexpr int LIVRE_MAX = 200;      // RG-03 : jetons gardés (les plus vieux effacés)
    static constexpr size_t JETON_MAX = 512;   // jeton de Google : ≈ 150 caractères ; au-delà, refusé
    std::vector<std::string> achats;           // livre des achats
    bool packDebutant = false;
    int bonsSkin = 0;
    bool sansPub = false;
    int x2Jour = 0;                            // AAAAMMJJ (0 : jamais)
    int x2Nombre = 0;
    long long x2Course = 0;                    // n° (≥ 1) de la dernière course doublée ; 0 : aucune
};

// jeton accepté dans le livre : 1 à JETON_MAX caractères, lettres et chiffres ASCII, « . », « - », « _ », « : » (jamais « | » ni « = »)
inline bool jetonValide(const std::string& j) {
    if (j.empty() || j.size() > EtatMarket::JETON_MAX) return false;
    for (const char c : j)
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_' || c == ':')) return false;
    return true;
}

} // namespace wfs
