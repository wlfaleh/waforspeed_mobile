// WaForSpeed — paquets de cartes « .wfcarte » (WFS-301, chantier « Paquets de cartes », socle 1).
// Une carte cuite est un fichier par carte ET par format de texture : <carte>_<astc|etc2|rgba8>.wfcarte, rangé dans
// <stockage interne>/paquets/ (Android : internalDataPath ; PC : build/paquets/ ou WFS_PAQUETS) ; jamais commité, jamais dans l'APK.
//
// Format (petit-boutiste), version VERSION_FORMAT :
//   0   « WFSC »                         magie
//   4   u32 version du format            LUE EN PREMIER : un jeu plus ancien refuse proprement un format plus récent
//   8   u32 taille de l'en-tête          octets de 0 à la fin du CRC de l'en-tête (début des données)
//   12  u16 + octets : nom de la carte ; u32 version de la carte ; u8 format de texture ; u64 empreinte (RG-02)
//       u32 nombre de blocs ; par bloc : u8 + octets (nom), u64 position (depuis le début du fichier), u64 taille, u32 CRC-32
//       u32 CRC-32 de l'en-tête (octets 0 à juste avant ce champ)
//   puis les données des blocs, dans l'ordre de la table.
// Blocs nommés : « decor_regroupe » (décor préparé, partie B), « maillages » et « textures » (archives de fichiers lus par
// readAsset, voir Archive), « trace » et « chocs » réservés (vides pour la Ville actuelle) ; un bloc inconnu est ignoré.
// Archive (blocs maillages / textures) : « WFSA », u32 nombre, puis par fichier (trié par chemin) : u16 + octets (chemin), u64 taille,
// octets. Textures KTX2 sans supercompression (le lecteur KTX2 du jeu la refuse).
//
// Lecture (RG-03) : readAsset() de chaque plateforme demande d'abord lireDansPaquet() (paquet vérifié et activé), puis l'APK.
// Aucun fichier « .part » n'est jamais lu : seul <carte>_<format>.wfcarte, fichier fini renommé après vérification (WFS-306).
// Ce module ne dépend que de la bibliothèque standard (src/core).
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace wfs {
namespace carte {

constexpr uint32_t VERSION_FORMAT = 1;
// à augmenter à chaque changement de ce que produit la cuisson (code de construction de la carte, fichiers mis dans le paquet) :
// change l'empreinte attendue, donc un paquet ancien est refusé et la carte du code est jouée (RG-02, RG-06)
constexpr uint32_t VERSION_GENERATION_CARTE = 1;

enum class FormatTexture : uint8_t { ASTC = 0, ETC2 = 1, RGBA8 = 2 };
constexpr FormatTexture FORMATS[3] = {FormatTexture::ASTC, FormatTexture::ETC2, FormatTexture::RGBA8};   // ordre de préférence
const char* nomFormat(FormatTexture f);          // « astc », « etc2 », « rgba8 »
std::string nomFichier(const std::string& carte, FormatTexture f);   // « ville_astc.wfcarte »

struct Bloc { std::string nom; std::vector<uint8_t> donnees; };
struct Paquet {
    std::string carte;
    uint32_t versionCarte = 1;
    FormatTexture format = FormatTexture::RGBA8;
    uint64_t empreinte = 0;
    std::vector<Bloc> blocs;
    const Bloc* bloc(const std::string& nom) const;   // nullptr si absent
};

// écriture déterministe (deux appels identiques → mêmes octets)
std::vector<uint8_t> ecrire(const Paquet& p);

// ---- vérification (RG-05)
enum class Verdict { Bon, Absent, Illisible, MagieFausse, VersionFuture, AutreCarte, MauvaisFormat, EmpreinteFausse, TailleFausse, CrcFaux, NonFini };
const char* texte(Verdict v);   // raison en français, écrite au journal du jeu (RG-06)
struct Attendu { std::string carte; FormatTexture format = FormatTexture::RGBA8; uint64_t empreinte = 0; };
// lit et vérifie un paquet en mémoire (en-tête, version, carte, format, empreinte, tailles, CRC de chaque bloc) ; sortie remplie si Bon
Verdict lire(const std::vector<uint8_t>& octets, const Attendu& a, Paquet* sortie);
// fichier fini seulement (refus d'un « .part ») ; Absent si le fichier n'existe pas ; octets gardés si Bon (activer sans relire)
Verdict verifier_carte(const std::string& chemin, const Attendu& a, Paquet* sortie = nullptr);

// ---- empreinte (RG-02) : FNV-1a 64 bits de la carte, de sa graine, de la version du tracé, de VERSION_GENERATION_CARTE et des
// valeurs de configgame.json lues par la construction (liste écrite par l'appelant, dans l'ordre)
uint64_t empreinte(const std::string& carte, uint32_t graine, int versionTrace, const std::vector<std::string>& valeursConfig);
uint32_t crc32(const uint8_t* d, size_t n);

// ---- archive de fichiers (blocs maillages / textures)
struct Fichier { std::string chemin; std::vector<uint8_t> octets; };
std::vector<uint8_t> archive(std::vector<Fichier> fichiers);   // triée par chemin (déterministe)
// lit une archive ; faux si elle est mal formée (sortie vide)
bool lireArchive(const std::vector<uint8_t>& a, std::vector<Fichier>& sortie);

// ---- 2e source de lecture (RG-03)
std::string& dossierPaquets();   // posé par la plateforme au démarrage
// active un paquet vérifié (Bon) : ses archives répondent à lireDansPaquet ; remplace le précédent
void activer(Paquet&& p);
void desactiver();
bool actif();
const std::string& carteActive();
// fichier du paquet actif (copie dans out) ; faux s'il n'y est pas (readAsset lit alors l'APK)
bool lireDansPaquet(const std::string& chemin, std::vector<uint8_t>& out);
long long lusDansPaquet();   // nombre de fichiers servis par le paquet depuis l'activation (journal, banc)

} // namespace carte
} // namespace wfs
