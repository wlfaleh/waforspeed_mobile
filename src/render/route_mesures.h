// WaForSpeed — WFS-206 (RG-05) : mesures M-01 / M-02 de la bibliothèque des revêtements (WFS-205), faites sur les fichiers :
// les mêmes pour tests/fidelite_routes.cpp et pour le banc « realisme » (indice des routes dans realisme-fidelite.txt).
// Calcul seul, sans GPU : niveau 0 du repli RGBA8 de chaque carte (= l'ASTC décompressé, réduit à 512).
#pragma once
#include <string>
#include <vector>
#include "route_revetements.h"

namespace wfs::route {

struct MesureRevetement {
    std::string id;          // revêtement (enrobe-use…)
    std::string grandeur;    // « M-01 albédo moyen », « M-02 rugosité sèche moyenne »
    double valeur = 0, min = 0, max = 0;
    bool metal = false;      // ORM : canal B non nul (jamais permis)
    bool dansPlage() const { return valeur >= min - 1e-9 && valeur <= max + 1e-9; }
};
// depot : dossier du dépôt (fichiers sous assets/textures/route/) ; carte absente ou illisible : mesure omise
std::vector<MesureRevetement> mesurerBibliotheque(const std::string& depot);

} // namespace wfs::route
