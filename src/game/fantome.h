// WaForSpeed — fantôme de ton record en Checkpoint (WFS-125, épic E3) : partie pure, sans rendu (testée par
// tests/test_records.cpp). Le jeu web n'a pas de fantôme solo : la trace reprend les champs des fantômes du duel
// (struct Ghost de duel.h : distance, écart latéral, cap, vitesse) ; le fichier est écrit par binarypack.* (WFS-89).
//
// RG-05 : un échantillon toutes les 0,1 s de course ; 6 000 au plus (10 min), au-delà on arrête d'enregistrer.
// RG-06 : relecture interpolée entre deux échantillons ; trace finie → plus de fantôme.
// Fichier « fantome-<terrain>.bin » : « WFSF1 » puis des messages BinaryPack précédés de leur taille (4 octets, gros
// boutiste) : un en-tête {terrain, car, peinture, score, n} puis des blocs de 250 échantillons (tableaux de nombres
// d, x, h, v), chacun sous les limites de bp::unpack (4 096 éléments, 64 Ko).
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace wfs::fantome {

constexpr double PAS_S = 0.1;
constexpr size_t MAX_ECHANTILLONS = 6000;
constexpr size_t BLOC = 250;

struct Echantillon { double d = 0, x = 0, h = 0, v = 0; };   // distance (unités), latéral (demi-largeurs), cap (rad), vitesse
struct Trace {
    std::string terrain, car;
    unsigned peinture = 0;        // 0xRRGGBB
    long long score = 0;          // score de la course qui a écrit la trace (record du terrain)
    std::vector<Echantillon> e;   // e[i] au temps i × 0,1 s de course
    double duree() const { return e.empty() ? 0 : (e.size() - 1) * PAS_S; }
};

// enregistrement : un échantillon dès que le temps de course atteint le suivant (i × 0,1 s) ; faux si la trace est pleine
struct Enregistreur {
    Trace trace;
    bool plein = false;
    void demarrer(const std::string& terrain, const std::string& car, unsigned peinture);
    bool pas(double tempsCourse, const Echantillon& s);
};

// position du fantôme au temps t (s) ; faux si la trace est vide ou finie (t au-delà du dernier échantillon)
bool interpoler(const Trace& tr, double t, Echantillon& out);

std::string encoder(const Trace& tr);
bool decoder(const std::string& octets, Trace& out);
std::string nomFichier(const std::string& terrain);   // « fantome-<terrain>.bin »
bool lireFichier(const std::string& chemin, Trace& out);

} // namespace wfs::fantome
