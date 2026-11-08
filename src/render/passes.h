// WaForSpeed — passes de rendu enfichables (WFS-150, épic E0, socle de l'axe Réalisme).
//
// RG-01 : une passe = une classe dérivée de PasseRendu, dans son propre fichier (zone de son épic), enregistrée par
// WFS_PASSE(Classe, ordre). Aucun fichier du Renderer à modifier : l'ordonnanceur (passes.cpp) les appelle aux points
// fixes de l'image, toujours dans cet ordre :
//     ombres du soleil (Renderer) → reflets de la chaussée (Renderer, WFS-16)
//     → étape Ombres (passes d'ombre en plus : cascades, WFS-153)
//     → étape PrePasse (pré-passe profondeur + normales, RG-02, seulement si une passe active la demande)
//     → étape Ecran (passes d'écran : capture d'environnement RG-03, occlusion, reflets d'écran)
//     → passe de scène (Renderer : ciel, opaques) → étape Scene → transparents (Renderer) → étape Transparents
//     → bloom (Renderer, WFS-100) → étape Post (post-traitement, avant la passe finale) → passe finale (ACES + interface).
// Dans une étape, les passes vont par ordre croissant. Les étapes Scene et Transparents s'exécutent DANS la passe de
// scène (HDR, MSAA du niveau) : leurs pipelines se refont dans redimensionner() (appelé après chaque changement de cible
// ou de MSAA). Les autres étapes sont hors de toute passe Vulkan : chaque passe ouvre et ferme la sienne.
//
// RG-04 : qualiteMin() = niveau le plus bas où la passe tourne (qualite::BASSE … qualite::ULTRA, qualite_auto.h).
// RG-05 : chaque passe active est horodatée sur le GPU (2 horodatages par image) ; couts() donne le coût moyen de chaque
// passe (enfichable ou du Renderer), « non mesurable » si le pilote n'a pas d'horodatages.
//
// Sans passe active, rien ne change dans l'image (RG-06) : les passes de ce ticket (pré-passe, capture, essai) ne
// tournent que sur demande.
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "vk.h"
#include "frame_ubo.h"
#include "renderer.h"

namespace wfs {

enum class EtapeRendu : int { Ombres = 0, PrePasse = 1, Ecran = 2, Scene = 3, Transparents = 4, Post = 5, N = 6 };
const char* nomEtape(EtapeRendu e);

// contexte d'une image, donné à PasseRendu::executer
struct ImagePasse {
    Renderer& r;
    VkCommandBuffer cb;
    int image;                  // tampon d'image en vol (0 ou 1) : ressources doublées par image
    uint32_t largeur, hauteur;  // taille de la scène 3D (pixels, échelle de rendu comprise)
    const FrameUBO& ubo;        // données de l'image (caméra, soleil, ciel, brouillard, ombre) déjà envoyées aux shaders
    int niveau;                 // niveau de qualité affiché (qualite::niveauAffiche : Basse 0 … Ultra 3)
};

class PasseRendu {
public:
    virtual ~PasseRendu() = default;
    virtual const char* nom() const = 0;
    virtual EtapeRendu etape() const = 0;
    virtual const char* cibles() const { return ""; }   // cibles lues / écrites (journal, rapport du banc)
    virtual int qualiteMin() const { return 0; }        // RG-01 : niveau de qualité minimal
    virtual bool demandePrePasse() const { return false; }   // RG-02 : lit la pré-passe (profondeur + normales)
    virtual bool active(const ImagePasse&) const { return true; }   // tourne à cette image (en plus du niveau)
    virtual bool toutVoir() const { return false; }     // image suivante sans aucun objet masqué (capture d'environnement)
    virtual void creer(Renderer&) {}                    // ressources (après Renderer::init)
    virtual void redimensionner(Renderer&) {}           // après chaque changement de cible ou de MSAA (Renderer::resize)
    virtual void detruire(Renderer&) {}                 // GPU à l'arrêt
    virtual void executer(const ImagePasse& img) = 0;   // enregistre ses commandes dans img.cb
};

namespace passes {

using Fabrique = PasseRendu* (*)();
void enregistrer(Fabrique f, int ordre);
#define WFS_PASSE_CAT2(a, b) a##b
#define WFS_PASSE_CAT(a, b) WFS_PASSE_CAT2(a, b)
// enregistre une passe (un fichier .cpp de la zone de son épic) ; ordre croissant dans son étape
#define WFS_PASSE(Classe, ordre) \
    static const bool WFS_PASSE_CAT(wfs_passe_, __LINE__) = (::wfs::passes::enregistrer([]() -> ::wfs::PasseRendu* { return new Classe(); }, ordre), true);

// liste des passes enregistrées, triées par étape puis ordre (créées à la première demande)
const std::vector<PasseRendu*>& liste();

// ---- RG-05 : coût GPU par passe ----
struct Cout {
    std::string nom; EtapeRendu etape = EtapeRendu::Ecran;
    double ms = 0;     // moyenne par image où la passe a tourné
    int images = 0;    // images mesurées
    bool mesurable = true;
    bool renderer = false;   // passe du Renderer (ombres, reflets, scène) et non enfichable
};
std::vector<Cout> coutsCumules();      // depuis remettreAZero() (banc)
void remettreAZero();
std::vector<Cout> coutsFenetre();      // dernière fenêtre de 120 images (ligne « Rendu » en mode diagnostic)
bool horodatagesDisponibles();
std::string texteCouts(const std::vector<Cout>& c);   // « ombres 0,42 · scène 3,10 · pré-passe 0,21 ms GPU »

// ---- RG-02 : pré-passe profondeur + normales (demi-résolution) ----
// lisible par les passes des étapes Ecran à Post de la même image (SHADER_READ_ONLY) quand prePasseFaite() est vrai.
// normales : A2B10G10R10 (normale MONDE × 0,5 + 0,5, a = 1 si objet) ; profondeur : inversée (1 près, 0 ciel)
bool prePasseFaite();
VkImageView prePasseNormales();
VkImageView prePasseProfondeur();
VkSampler echantillonneurPrePasse();
uint32_t prePasseLargeur();
uint32_t prePasseHauteur();

// ---- passe d'essai (critère d'acceptation) : lit la pré-passe, écrit une image de contrôle jamais affichée ----
void activerEssai(bool actif);
bool essaiActif();

// ---- outils du banc : relecture d'images (GPU à l'arrêt) ----
// cible HDR de la scène (R16G16B16A16, avant exposition et tonemapping) : rgba en flottants, ligne par ligne
bool lireHdr(Renderer& r, std::vector<float>& rgba, uint32_t& w, uint32_t& h);
// image de contrôle de la passe d'essai (RGBA8) : moitié gauche normales, moitié droite profondeur
bool lireEssai(Renderer& r, std::vector<uint8_t>& rgba, uint32_t& w, uint32_t& h);

// ---- accroches du Renderer (renderer.cpp, WFS-150) ----
void creer(Renderer& r);
void redimensionner(Renderer& r);
void detruire(Renderer& r);
void debutImage(Renderer& r);
void avantScene(Renderer& r, VkCommandBuffer cb);
void dansScene(Renderer& r, VkCommandBuffer cb, EtapeRendu e);
void apresScene(Renderer& r, VkCommandBuffer cb);

// relecture d'une couche d'image couleur vers la mémoire (GPU à l'arrêt) : layout d'origine rendu tel quel
bool lireCouche(VkCtx& vk, VkImage img, VkImageLayout layout, uint32_t couche, uint32_t w, uint32_t h, size_t octetsParPixel, std::vector<uint8_t>& out);
float demiVersFloat(uint16_t h);

} // namespace passes

// accès à l'intérieur du Renderer, réservé aux passes du socle (ami de Renderer)
struct AccesRendu {
    static VkCtx& vk(Renderer& r) { return *r.vk_; }
    static int image(const Renderer& r) { return r.cur_; }
    static constexpr int images() { return Renderer::FRAMES; }
    static uint32_t largeurScene(const Renderer& r) { return r.cw_; }   // WFS-246 (RG-07) : capacité, fixe en course
    static uint32_t hauteurScene(const Renderer& r) { return r.ch_; }
    static const FrameUBO& ubo(const Renderer& r) { return *static_cast<const FrameUBO*>(r.fr_[r.cur_].ubo.mapped); }
    static VkDescriptorSet setImage(const Renderer& r, int f) { return r.fr_[f].frameSet; }
    static VkDescriptorSetLayout dispositionImage(const Renderer& r) { return r.frameLayout_; }
    static VkPipelineLayout dispositionScene(const Renderer& r) { return r.sceneLayout_; }
    static VkPipelineLayout dispositionSquelette(const Renderer& r) { return r.skinLayout_; }
    static VkRenderPass passeReflet(const Renderer& r) { return r.refletPass_; }
    static VkPipeline pipeline(const Renderer& r, int quoi);   // 0 ciel, 1 opaque, 2 mélange, 3 additif (passe de reflet)
    static VkShaderModule module(const Renderer& r, int i) { return r.modScene_[i]; }   // scene.vert, scene.frag, sky.vert, sky.frag, skin.vert, skin.frag
    static VkPipeline faire(Renderer& r, VkShaderModule vs, VkShaderModule fs, int sommets, bool melange, bool test, bool ecriture,
                            VkRenderPass rp, VkPipelineLayout lay, int mode = 0) {
        return r.makePipeline(vs, fs, sommets, melange, false, test, ecriture, rp, VK_SAMPLE_COUNT_1_BIT, lay, false, mode);
    }
    static const std::vector<Renderer::Pending>& opaques(const Renderer& r) { return r.opaqueQ_; }
    static const std::vector<Renderer::Pending>& transparents(const Renderer& r) { return r.transparent_; }
    static void dessiner(Renderer& r, const Renderer::Pending& p) { r.flushDraw(p); }
    static bool squelette(const Renderer::Pending& p) { return p.osOfs >= 0; }
    static uint32_t drapeaux(const Renderer::Pending& p) { return p.mat->flags; }
    static bool visibleCamera(const Renderer::Pending& p) { return (p.vis & Renderer::VIS_CAM) != 0; }
    static void sansMasquage(Renderer& r, bool v) { r.sansMasquage_ = v; }
    static VkQueryPool horodatages(const Renderer& r) { return r.tsPool_; }
    static bool horodatagesEcrits(const Renderer& r, int f) { return r.tsWritten_[f]; }
    static const Image& hdr(const Renderer& r) { return r.hdr_; }
};

} // namespace wfs
