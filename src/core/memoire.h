// WaForSpeed — WFS-297 (RG-06) : chiffres de mémoire et de chargement, mesurés pour de vrai, affichés à l'écran de chargement
// et dans les Réglages. Compteurs seuls (aucune dépendance) : remplis par le moteur aux endroits où les choses se créent.
//   - mémoire de la carte graphique : octets alloués par vkAllocateMemory (VkCtx::createBuffer / createImage, vk.cpp), rendus
//     à la destruction ;
//   - octets lus : fichiers lus par readAsset (desktop_main.cpp, android_main.cpp) ;
//   - objets 3D : maillages vivants (Renderer::makeMesh / makeSkinnedMesh, rendus par destroy) ;
//   - personnes : piétons de la carte (pietons.cpp) ; numéro de la partie : courses démarrées depuis le lancement (départ au « GO »,
//     course_system.cpp) ; Mo lus du chargement en cours (début : Game::init) ;
//   - mémoire du jeu : mémoire résidente du processus (/proc/self/statm, Linux et Android).
#pragma once
#include <atomic>
#include <cstdint>
#include <cstdio>
#if defined(__linux__) || defined(__ANDROID__)
#include <unistd.h>
#endif

namespace wfs::memoire {

struct Compteurs {
    std::atomic<int64_t> octetsGpu{0};     // mémoire de la carte graphique allouée par le jeu
    std::atomic<int64_t> octetsLus{0};     // cumul des fichiers lus depuis le lancement
    std::atomic<int64_t> lusDebut{0};      // octetsLus au début du chargement en cours
    std::atomic<int64_t> objets3d{0};      // maillages vivants
    std::atomic<int> personnes{0};         // piétons de la carte en cours
    std::atomic<int> partie{0};            // courses démarrées depuis le lancement (0 : aucune encore)
};
inline Compteurs& compteurs() { static Compteurs c; return c; }

// mémoire résidente du processus (Mo) ; −1 si inconnue (plateforme sans /proc)
inline double processusMo() {
#if defined(__linux__) || defined(__ANDROID__)
    FILE* f = std::fopen("/proc/self/statm", "r");
    if (!f) return -1;
    long pages = 0, res = 0;
    const int n = std::fscanf(f, "%ld %ld", &pages, &res);
    std::fclose(f);
    return n == 2 ? res * (double)sysconf(_SC_PAGESIZE) / (1024.0 * 1024.0) : -1;   // pages de 4 ou 16 Ko selon le téléphone
#else
    return -1;
#endif
}
inline double gpuMo() { return compteurs().octetsGpu.load() / (1024.0 * 1024.0); }
inline double lusMo() { return compteurs().octetsLus.load() / (1024.0 * 1024.0); }
// chargement en cours : « Lus » compte ce qui a été lu depuis le début du chargement de la partie (Game::init)
inline void debutChargement() { compteurs().lusDebut = compteurs().octetsLus.load(); }
inline double lusChargementMo() { return (compteurs().octetsLus.load() - compteurs().lusDebut.load()) / (1024.0 * 1024.0); }
// une course qui démarre (passage du décompte au « GO » : « Rouler », « Rejouer », rechargement, manche de duel) : partie suivante
inline int nouvellePartie() { return ++compteurs().partie; }

} // namespace wfs::memoire
