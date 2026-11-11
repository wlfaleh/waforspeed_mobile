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
// WFS-426 : cartes du code cuites en paquet par la CI (WFS_BANC=cuire) et dont le paquet contient les bâtiments cuits (WFS-416) :
// la Ville, puis la Corniche (identifiant « plage »)
inline constexpr const char* CARTES_CUITES[] = {"ville", "plage"};
bool cuiteEnPaquet(const std::string& carte);
// cuisson (banc, PC) : fichiers de la carte pris dans le dossier des assets (racine) pour un format de texture
carte::Paquet cuire(const std::string& racine, const std::string& carte, carte::FormatTexture f);

// WFS_BANC=cuire (publication des paquets, WFS-302 ; complément de WFS-301, RG-C1 à C3) : cuit les 3 formats de chaque carte de CARTES_CUITES (Ville, Corniche) deux fois, compare
// octet par octet, vérifie (verifier_carte) et LAISSE les fichiers dans dossierPaquets() ; une ligne par fichier (nom, taille, SHA-256,
// empreinte) ; sans Vulkan ni capture. Renvoie le nombre d'écarts (la plateforme sort avec le code 3 s'il y en a).
int cuireSeul(const std::string& racine);
std::string sha256Hex(const std::vector<uint8_t>& o);   // SHA-256 (FIPS 180-4) en hexadécimal minuscule

// banc d'essai (WFS_BANC=cuisson) : écrit les paquets de la Ville (3 formats) dans dossierPaquets(), vérifie deux cuissons identiques,
// puis paquet absent, corrompu, format futur, valide (captures cuisson-*), RG-07 (chargement de la Ville, médiane de 5, sans puis avec
// paquet ; tailles ; mémoire) et RG-08 : parcours complet accueil → Ville → course → retour (fonction fournie par la plateforme) avec le
// paquet présent, puis absent (captures dans out/avec-paquet et out/sans-paquet) ; renvoie le nombre d'écarts
int benchCuisson(Game& g, Renderer& r, const std::string& out, const std::string& racine,
                 int (*parcours)(Game&, Renderer&, const std::string&) = nullptr);

// WFS-416 : banc « batiments » (WFS_BANC=batiments, batiments_banc.cpp) : bâtiments cuits dans le paquet de la Ville, kit de secours et
// paquet joués tour à tour (captures batiments-*), aucune construction en course avec le paquet ; renvoie le nombre d'écarts
int benchBatiments(Game& g, Renderer& r, const std::string& out, const std::string& racine);

// WFS-346 : banc « neige » (WFS_BANC=neige, neige_banc.cpp) : saison lue du paquet sur les cartes d'essai (neige au sol, flocons dehors
// seulement, adhérence), captures neige-* ; renvoie le nombre d'écarts
int benchNeige(Game& g, Renderer& r, const std::string& out, const std::string& racine);

} // namespace cartes_v2
} // namespace wfs
