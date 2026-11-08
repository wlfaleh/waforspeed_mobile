// WaForSpeed — « Mise à jour et installation des paquets » (WFS-306, socle 2 du chantier « Paquets de cartes ») : étape de l'écran de
// chargement (après les CGU, avant le chargement de la carte) et bandeau du menu.
//   RG-01 : paquet de la carte déjà là, juste et à jour (verifier_carte, WFS-301) → rien de plus ; sinon lecture de cartes.json par la
//           plateforme (format de texture accepté par la carte graphique, empreinte attendue, version de l'APK).
//   RG-02 : proposition : nom, taille annoncée avant tout envoi, « Wi-Fi conseillé », « Installer » / « Plus tard » ; sur données mobiles,
//           « Vous êtes sur les données mobiles (N Mo) » et « Installer » n'est plus mis en avant ; rien ne part sans appui.
//   RG-04 : barre de la carte et barre totale, temps restant (après 5 s), astuces changées toutes les 7 s (assets/textes/astuces-paquets.txt).
//   RG-05 : coupure → « Connexion perdue, reprise automatique » ; fichier faux (2 essais) ou erreur → « Réessayer » / « Plus tard ».
//   RG-06 : pas de liste (hors ligne, 404, illisible), « Plus tard » ou échec → la carte du code est jouée, sans message bloquant ;
//           bandeau au menu « Paquet de la Ville disponible — Installer » ; connexion revenue (vérifiée au menu, une fois par minute au
//           plus) → proposition de nouveau.
//   RG-07 : place libre < reste + 10 % → « Place insuffisante : X Mo nécessaires, Y Mo libres » puis secours.
// La plateforme fournit la source (Android : Paquets.java par JNI, src/platform/paquets_android.cpp ; banc PC : source factice).
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "../../core/carte_cuite.h"
#include "../../core/system.h"   // Marges

namespace wfs {
class Renderer;
class Game;
class VkCtx;
namespace ecrans {

// ce que la plateforme sait faire (téléchargement hors du fil du jeu ; chaque appel est rapide)
struct SourcePaquets {
    enum Phase { RIEN = 0, LISTE, PRETE, ABSENTE, TELECHARGE, FINI, COUPURE, SHA_FAUX, PLACE, ERREUR };
    virtual ~SourcePaquets() = default;
    // lecture de cartes.json et choix du paquet (formats « astc,etc2,rgba8 » dans l'ordre accepté, empreinte en hexadécimal)
    virtual void chercher(const std::string& carte, const std::string& formats, const std::string& empreinte, int versionApk) = 0;
    virtual Phase phase(long long& recus, long long& taille) = 0;
    virtual bool proposition(std::string& nom, std::string& format) = 0;   // paquet proposé (après PRETE)
    virtual int reseau() = 0;              // 0 aucun, 1 Wi-Fi (non facturé), 2 données mobiles
    virtual long long placeLibre() = 0;    // octets libres dans le dossier des paquets
    virtual void installer() = 0;
    virtual void arreter() = 0;            // « Plus tard » pendant le téléchargement : le .part est gardé (reprise plus tard)
};

// formats de texture que la carte graphique sait lire, dans l'ordre de préférence (ASTC, ETC2, RGBA8 toujours)
std::vector<carte::FormatTexture> formatsAcceptes(const VkCtx& vk);
std::string texteFormats(const std::vector<carte::FormatTexture>& f);   // « astc,etc2,rgba8 »
int versionApk();                     // N de « 0.1.N » (versionJeu, WFS-279) ; 0 hors CI
std::string texteMo(long long octets);   // « 210 Mo » (Mo = 1 048 576 octets, comme le journal)

class EtapePaquets {
public:
    enum class Etat { Ferme, Recherche, Proposition, Telechargement, Place, Echec, Fini };
    // RG-01 : paquet juste déjà là → faux, rien à faire ; sinon l'étape s'ouvre (recherche de la liste) → vrai
    bool demarrer(Renderer& r, SourcePaquets& s, const std::string& carte, const std::vector<carte::FormatTexture>& formats);
    // une image dans l'écran de chargement (rotation vérifiée comme les CGU) ; vrai une seule fois, quand l'étape est finie
    bool image(VkCtx& vk, uint32_t largeurFenetre, uint32_t hauteurFenetre, double maintenant);
    // dessin et état (sans beginFrame / endFrame) ; aussi appelé par le banc
    void avancer(double maintenant);
    void dessiner(Renderer& r, float W, float H, const Marges& m, double maintenant);
    bool toucher(int id, float x, float y, bool down);
    // fenêtre perdue (touche Accueil) : police libérée, l'étape reste ouverte (le téléchargement continue) ; retrouvée : reprise
    void perdreFenetre(Renderer& r);
    bool reprendre(Renderer& r);           // vrai si l'étape était ouverte (initGraphics s'arrête là)
    bool ouvert() const { return etat_ != Etat::Ferme && etat_ != Etat::Fini; }
    bool tientLaFenetre() const { return ouvert() && fenetre_; }
    Etat etat() const { return etat_; }
    const std::string& carte() const { return carte_; }
    // banc : zones des boutons (centre)
    struct Zone { float x = 0, y = 0, w = 0, h = 0; bool contient(float px, float py) const { return w > 0 && px >= x && px <= x + w && py >= y && py <= y + h; } };
    const Zone& zoneInstaller() const { return zInstaller_; }
    const Zone& zonePlusTard() const { return zPlusTard_; }
    const Zone& zoneContinuer() const { return zContinuer_; }

    // ---- menu (RG-06) : source et carte gardées après l'étape pour le bandeau
    SourcePaquets* source() const { return source_; }
    std::string empreinte_, formats_;
    bool installe_ = false;                 // paquet installé pendant cette session (bandeau « Installé »)
    bool secours_ = false;                  // l'étape s'est finie sans paquet (bandeau possible au menu)

private:
    void finir(bool avecPaquet, const char* raison);
    Renderer* r_ = nullptr;
    SourcePaquets* source_ = nullptr;
    Etat etat_ = Etat::Ferme;
    std::string carte_, nom_, formatPropose_, message_;
    long long recus_ = 0, taille_ = 0, recusDebut_ = 0;
    int reseau_ = 0, essaisRotation_ = 0, doigt_ = -1, cible_ = 0;
    bool fenetre_ = true, coupure_ = false;
    double t0_ = 0, tDebutTelechargement_ = -1, tFini_ = -1, maintenant_ = 0;
    Zone zInstaller_, zPlusTard_, zContinuer_;
};

EtapePaquets& paquets();
const std::vector<std::string>& astuces();   // ≥ 15 textes courts (assets/textes/astuces-paquets.txt), ligne vide et « # » ignorées

// banc d'essai (WFS_BANC=paquets) : source factice qui copie pas à pas un paquet cuit (WFS-301) ; captures paquets-* et parcours
int benchPaquets(Game& g, Renderer& r, const std::string& out, const std::string& racine);

} // namespace ecrans
} // namespace wfs
