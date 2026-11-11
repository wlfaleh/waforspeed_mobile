// WFS-418 : règles de la voie de repli de la mémoire graphique (src/render/vk.h), vérifiables sans carte graphique.
//  - RG-01 : seules les deux réponses « plus de mémoire » de Vulkan donnent un tampon vide ; toute autre erreur arrête le jeu ;
//  - RG-04 : refus simulé du banc PC : à partir de la N-ième réservation (la mémoire reste pleine), jamais avec 0 ;
//  - RG-04 : valeur de WFS_MANQUE_MEMOIRE_GPU lue sans risque (vide, fausse, négative ou trop grande : aucun refus).
// Le chargement de la Ville avec une réservation refusée (kit de secours, sans arrêt du jeu) se vérifie au banc « batiments »
// (étape 5, WFS_BANC=batiments), qui a besoin de la carte graphique.
#include <cstdio>
#include "../src/render/vk.h"
using namespace wfs;

static int echecs = 0, cas = 0;
#define VERIFIE(c, ...) do { cas++; if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int main() {
    // RG-01 : plus de mémoire (carte graphique ou mémoire vive du pilote) → repli
    VERIFIE(manqueMemoire(VK_ERROR_OUT_OF_DEVICE_MEMORY), "VK_ERROR_OUT_OF_DEVICE_MEMORY → repli");
    VERIFIE(manqueMemoire(VK_ERROR_OUT_OF_HOST_MEMORY), "VK_ERROR_OUT_OF_HOST_MEMORY → repli");
    // toute autre réponse : pas de repli (succès, ou arrêt du jeu comme VK_OK)
    const VkResult autres[] = {VK_SUCCESS, VK_ERROR_DEVICE_LOST, VK_ERROR_INITIALIZATION_FAILED, VK_ERROR_MEMORY_MAP_FAILED,
                               VK_ERROR_FEATURE_NOT_PRESENT, VK_ERROR_TOO_MANY_OBJECTS, VK_ERROR_FORMAT_NOT_SUPPORTED, VK_ERROR_UNKNOWN};
    for (VkResult r : autres) VERIFIE(!manqueMemoire(r), "réponse %d : pas de repli (arrêt du jeu comme avant)", (int)r);
    static_assert(manqueMemoire(VK_ERROR_OUT_OF_DEVICE_MEMORY) && !manqueMemoire(VK_SUCCESS), "calcul à la compilation");

    // RG-04 : refus simulé (banc PC)
    for (uint32_t n = 1; n <= 1000; n++) VERIFIE(!refusSimule(n, 0), "echecA = 0 : la réservation n° %u n'est jamais refusée", n);
    for (uint32_t n = 1; n <= 4; n++) VERIFIE(!refusSimule(n, 5), "echecA = 5 : réservation n° %u acceptée", n);
    for (uint32_t n = 5; n <= 50; n++) VERIFIE(refusSimule(n, 5), "echecA = 5 : réservation n° %u refusée (mémoire pleine après le 1er refus)", n);
    VERIFIE(refusSimule(1, 1) && refusSimule(0xFFFFFFFFu, 1), "echecA = 1 : toutes les réservations refusées");

    // RG-04 : lecture de la variable du banc
    struct Cas { const char* texte; uint32_t attendu; };
    const Cas lectures[] = {{nullptr, 0}, {"", 0}, {"0", 0}, {"1", 1}, {"7", 7}, {"250", 250}, {"0042", 42}, {"10000000", 10000000},
                            {"10000001", 0}, {"99999999999999999999", 0}, {"-3", 0}, {"+3", 0}, {" 3", 0}, {"3 ", 0}, {"12x", 0},
                            {"x12", 0}, {"1.5", 0}, {"oui", 0}};
    for (const Cas& c : lectures)
        VERIFIE(rangRefusSimule(c.texte) == c.attendu, "WFS_MANQUE_MEMOIRE_GPU=« %s » → %u (attendu %u)", c.texte ? c.texte : "(absente)",
                rangRefusSimule(c.texte), c.attendu);

    std::printf("test_repli_memoire : %d vérifications, %s\n", cas, echecs ? "ÉCHEC" : "TOUT EST VERT");
    return echecs ? 1 : 0;
}
