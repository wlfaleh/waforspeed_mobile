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
    enum Phase { RIEN = 0, LISTE, PRETE, ABSENTE, TELECHARGE, FINI, COUPURE, SHA_FAUX, PLACE, ERREUR, ATTENTE };   // ATTENTE : WFS-307 (file)
    virtual ~SourcePaquets() = default;
    // lecture de cartes.json et choix du paquet (formats « astc,etc2,rgba8 » dans l'ordre accepté, empreinte en hexadécimal)
    virtual void chercher(const std::string& carte, const std::string& formats, const std::string& empreinte, int versionApk) = 0;
    virtual Phase phase(long long& recus, long long& taille) = 0;
    virtual bool proposition(std::string& nom, std::string& format) = 0;   // paquet proposé (après PRETE)
    virtual int reseau() = 0;              // 0 aucun, 1 Wi-Fi (non facturé), 2 données mobiles
    virtual long long placeLibre() = 0;    // octets libres dans le dossier des paquets
    virtual void installer() = 0;
    virtual void arreter() = 0;            // « Plus tard » pendant le téléchargement : le .part est gardé (reprise plus tard)
    // ---- WFS-307 : toutes les cartes (choix du terrain), file d'attente une carte à la fois ; empreintes « ville:c30f…,plage:… »
    virtual void chercherTout(const std::string& /*formats*/, const std::string& /*empreintes*/, int /*versionApk*/) {}
    virtual Phase phaseCarte(const std::string& /*carte*/, long long& recus, long long& taille) { recus = taille = 0; return ABSENTE; }
    virtual bool propositionCarte(const std::string& /*carte*/, std::string& /*nom*/, std::string& /*format*/) { return false; }
    virtual bool installerCarte(const std::string& /*carte*/) { return false; }
    virtual void arreterCarte(const std::string& /*carte*/) {}
    virtual void supprimee(const std::string& /*carte*/) {}   // fichier effacé par le jeu : la carte redevient proposable
    virtual void ralentir(bool /*oui*/) {}                   // RG-03 : débit réduit pendant la course sous 30 img/s
};

// ---- WFS-307 : paquets de chaque carte, vus depuis le choix du terrain (machine sans rendu, paquets_etape.cpp)
struct EtatCarte {
    enum Etat { SansPaquet, Recherche, Disponible, MiseAJour, Attente, Telechargement, Coupure, Installee, Echec } etat = SansPaquet;
    long long recus = 0, taille = 0, surDisque = 0;   // octets : reçus, annoncés (cartes.json), du paquet installé
    std::string nom;                                  // nom affiché (cartes.json), sinon l'identifiant
};
class CartesPaquets {
public:
    // source et formats (posés par l'étape du chargement) ; cartes connues et empreinte attendue de chacune ; lit la liste (une fois)
    void lier(SourcePaquets* s, const std::vector<carte::FormatTexture>& formats, int versionApk);
    void chercher(const std::vector<std::pair<std::string, uint64_t>>& cartes);
    void suivre(const std::string& carte, uint64_t empreinte);   // carte suivie sans relire la liste (carte de l'étape du chargement, WFS-306)
    bool cherche() const { return cherche_; }
    // état (fichier vérifié sur le téléphone, puis phase de la source) ; met à jour les annonces « prête » et les changements (RG-08)
    EtatCarte etat(const std::string& carte);
    void avancer();   // une fois par image : suit les cartes en cours (« prête », changement)
    // RG-02 / RG-05 : vrai si la carte part dans la file ; sinon message (« Connexion nécessaire », place insuffisante)
    bool telecharger(const std::string& carte, std::string& message);
    void annuler(const std::string& carte);
    // RG-04 : efface le paquet (tous les formats et le .part) ; faux pendant son téléchargement ; octets libérés dans liberes
    bool supprimer(const std::string& carte, long long& liberes);
    // RG-08 : paquet de la carte installé, mis à jour ou supprimé depuis son dernier chargement ; prise() efface la marque
    bool changee(const std::string& carte) const;
    void prise(const std::string& carte);
    // RG-03 : cartes devenues prêtes, à annoncer au retour au menu (vidé par la lecture)
    std::vector<std::string> pretes();
    void ralentir(bool oui);
    int reseau() const { return source_ ? source_->reseau() : 0; }
    SourcePaquets* source() const { return source_; }
private:
    struct Suivi { uint64_t empreinte = 0; SourcePaquets::Phase phase = SourcePaquets::RIEN; bool verifie = false, installee = false, ancien = false; long long surDisque = 0; };
    void verifier(const std::string& carte, Suivi& s);
    SourcePaquets* source_ = nullptr;
    std::vector<carte::FormatTexture> formats_;
    int apk_ = 0;
    bool cherche_ = false, ralenti_ = false;
    std::vector<std::pair<std::string, Suivi>> cartes_;
    std::vector<std::string> changees_, pretes_;
    Suivi* suivi(const std::string& carte);
};
CartesPaquets& cartesPaquets();
// choix du terrain (terrain.cpp) : vignette de la carte (x, y, w, h ; k = pixels par dp)
void assombrirCarte(Renderer& r, const std::string& carte, float x, float y, float w, float h);   // RG-01 : paquet publié pas encore là
void dessinerPaquetCarte(Renderer& r, const std::string& carte, float x, float y, float w, float h, float k, double maintenant);
bool toucherPaquetCarte(const std::string& carte, int id, float x, float y, bool down, double maintenant);   // vrai : toucher pris
// banc : centre de la zone d'action posée à la dernière image (0 bouton principal, 1 Supprimer / Annuler, 2 confirmer, 3 refuser)
bool centrePaquetCarte(int quelle, float& x, float& y);

// formats de texture que la carte graphique sait lire, dans l'ordre de préférence (ASTC, ETC2, RGBA8 toujours)
std::vector<carte::FormatTexture> formatsAcceptes(const VkCtx& vk);
std::string texteFormats(const std::vector<carte::FormatTexture>& f);   // « astc,etc2,rgba8 »
int versionApk(const char* versionName);   // N de « 0.1.N » ; 0 pour « dev » ou un texte sans numéro
int versionApk();                     // versionApk(versionJeu()) (WFS-279) ; 0 hors CI
std::string texteMo(long long octets);   // « 210 Mo » (Mo = 1 048 576 octets, comme le journal ; au moins 1 Mo)

class EtapePaquets {
public:
    enum class Etat { Ferme, Recherche, Proposition, Telechargement, Place, Echec, Fini };
    // RG-01 : paquet juste déjà là → faux, rien à faire ; sinon l'étape s'ouvre (recherche de la liste) → vrai
    bool demarrer(Renderer& r, SourcePaquets& s, const std::string& carte, const std::vector<carte::FormatTexture>& formats);
    // même chose sans rendu (machine d'états seule, paquets_etape.cpp ; tests/test_paquets.cpp) : empreinte attendue donnée
    bool demarrerAvec(SourcePaquets& s, const std::string& carte, const std::vector<carte::FormatTexture>& formats, uint64_t empreinte, int versionApk, double maintenant);
    // actions des boutons (toucher les appelle ; le test aussi) : « Installer » / « Réessayer », « Plus tard », « Continuer »
    void installer();
    void plusTard();
    void continuer();
    // RG-06 : bandeau du menu touché → la proposition de RG-02 s'ouvre par-dessus le menu (taille, Wi-Fi, données mobiles) ;
    // « Installer » lance le téléchargement (le bandeau montre la progression), « Plus tard » referme ; faux sans proposition
    bool ouvrirProposition(double maintenant);
    bool enMenu() const { return menu_ && ouvert(); }
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
    bool tientLaFenetre() const { return ouvert() && fenetre_ && !menu_; }   // étape du chargement seulement (la proposition du menu vit dans le jeu)
    void fermerMenu() { if (menu_) { etat_ = Etat::Ferme; menu_ = false; } }
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
    int apk_ = 0;
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
    bool fenetre_ = true, coupure_ = false, menu_ = false;
    double t0_ = 0, tDebutTelechargement_ = -1, tFini_ = -1, maintenant_ = 0;
    Zone zInstaller_, zPlusTard_, zContinuer_;
};

EtapePaquets& paquets();
// menu (RG-06) : vrai si le toucher va au bandeau des paquets ou à la proposition ouverte par-dessus le menu (le menu le laisse passer)
bool toucheBandeauPaquets(float x, float y);
const std::vector<std::string>& astuces();   // ≥ 15 textes courts (assets/textes/astuces-paquets.txt), ligne vide et « # » ignorées

// banc d'essai (WFS_BANC=paquets) : source factice qui copie pas à pas un paquet cuit (WFS-301) ; captures paquets-* et parcours
int benchPaquets(Game& g, Renderer& r, const std::string& out, const std::string& racine);

} // namespace ecrans
} // namespace wfs
