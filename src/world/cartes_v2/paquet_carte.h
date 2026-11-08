// WaForSpeed — paquets de cartes : choix « paquet ou secours » et cuisson (WFS-301, épic E14 ; format : src/core/carte_cuite.h).
// Avant le chargement d'une carte, la plateforme appelle preparerCarte(carte) : le paquet de la carte (format de texture préféré
// d'abord : ASTC, ETC2, RGBA8) est vérifié puis activé (readAsset le lit avant l'APK) ; absent, faux ou trop récent → la carte
// construite par le code est jouée (secours, RG-06), une ligne du journal du jeu dit pourquoi. Jamais de plantage.
// Partie A (PR 1) : le paquet contient les fichiers de la carte (maillages, textures) ; le décor préparé (bloc decor_regroupe) et la
// cuisson complète de la Ville viennent avec la partie B, après la fusion de WFS-268.
#pragma once
#include <string>
#include <vector>
#include "../../core/carte_cuite.h"

namespace wfs {
class Game;
class Renderer;
namespace cartes_v2 {

// valeurs de configgame.json lues par la construction des cartes (RG-02) : aucune aujourd'hui (le décor ne lit aucune clé) ;
// toute clé ajoutée à la construction doit être ajoutée ici (l'empreinte change, les anciens paquets sont refusés)
std::vector<std::string> valeursConfigConstruction(const std::string& carte);
uint64_t empreinteAttendue(const std::string& carte);
// RG-03, RG-06 : paquet vérifié → activé, vrai ; sinon carte du code (paquet désactivé), faux ; raison au journal du jeu
bool preparerCarte(const std::string& carte);
// cuisson (banc, PC) : fichiers de la carte pris dans le dossier des assets (racine) pour un format de texture
carte::Paquet cuire(const std::string& racine, const std::string& carte, carte::FormatTexture f);

// banc d'essai (WFS_BANC=cuisson) : écrit les paquets de la Ville (3 formats) dans dossierPaquets(), vérifie deux cuissons identiques,
// puis paquet absent, corrompu, format futur, valide (captures cuisson-*), RG-07 (chargement de la Ville, médiane de 5, sans puis avec
// paquet ; tailles ; mémoire) et RG-08 : parcours complet accueil → Ville → course → retour (fonction fournie par la plateforme) avec le
// paquet présent, puis absent (captures dans out/avec-paquet et out/sans-paquet) ; renvoie le nombre d'écarts
int benchCuisson(Game& g, Renderer& r, const std::string& out, const std::string& racine,
                 int (*parcours)(Game&, Renderer&, const std::string&) = nullptr);

} // namespace cartes_v2
} // namespace wfs
