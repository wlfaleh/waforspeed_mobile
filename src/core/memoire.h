// WaForSpeed — WFS-297 (RG-06) : chiffres de mémoire et de chargement, mesurés pour de vrai, affichés à l'écran de chargement
// et dans les Réglages. Compteurs seuls (aucune dépendance) : remplis par le moteur aux endroits où les choses se créent.
//   - mémoire de la carte graphique : octets alloués par vkAllocateMemory (VkCtx::createBuffer / createImage, vk.cpp), rendus
//     à la destruction ;
//   - octets lus : fichiers lus par readAsset (desktop_main.cpp, android_main.cpp) ;
//   - objets 3D : maillages vivants (Renderer::makeMesh / makeSkinnedMesh, rendus par destroy) ;
//   - personnes : piétons de la carte (pietons.cpp) ; numéro de la partie : chargements réussis depuis le lancement (Game::init) ;
//   - mémoire du jeu : mémoire résidente du processus (/proc/self/statm, Linux et Android).
#pragma once
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <string>
#if defined(__linux__) || defined(__ANDROID__)
#include <unistd.h>
#endif

namespace wfs::memoire {

struct Compteurs {
    std::atomic<int64_t> octetsGpu{0};     // mémoire de la carte graphique allouée par le jeu
    std::atomic<int64_t> octetsLus{0};     // cumul des fichiers lus depuis le lancement
    std::atomic<int64_t> objets3d{0};      // maillages vivants
    std::atomic<int> personnes{0};         // piétons de la carte en cours
    std::atomic<int> partie{0};            // chargements de partie réussis depuis le lancement
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
// « Mémoire : jeu 1 380 Mo · carte graphique 620 Mo · partie 2 » (Réglages) ; espaces fines insécables entre les milliers
inline std::string milliers(long long v) {
    std::string s = std::to_string(v < 0 ? -v : v), o;
    for (size_t i = 0; i < s.size(); i++) { if (i && (s.size() - i) % 3 == 0) o += "\u202F"; o += s[i]; }
    return (v < 0 ? "-" : "") + o;
}
inline std::string texteMemoire() {
    const double p = processusMo();
    return "Mémoire : jeu " + (p < 0 ? std::string("?") : milliers((long long)(p + 0.5))) + " Mo · carte graphique " + milliers((long long)(gpuMo() + 0.5)) +
           " Mo · partie " + std::to_string(compteurs().partie.load());
}
// « Lus : 152 Mo · 1 234 objets 3D · 47 voitures · 114 personnes » (écran de chargement) ; voitures : passées par l'appelant
inline std::string texteChargement(int voitures) {
    return "Lus : " + milliers((long long)(lusMo() + 0.5)) + " Mo · " + milliers(compteurs().objets3d.load()) + " objets 3D · " + std::to_string(voitures) +
           " voitures · " + std::to_string(compteurs().personnes.load()) + " personnes";
}

} // namespace wfs::memoire
