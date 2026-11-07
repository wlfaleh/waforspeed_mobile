// WaForSpeed — moteur de rendu : passe de scène HDR (MSAA), passe finale (tonemapping + interface).
#pragma once
#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>
#include "vk.h"
#include "../core/math.h"
#include "../game/roues.h"   // WFS-60 : RoueModele, RouesPose
#include "masquage.h"   // WFS-82 : masquage hors champ et au loin
#include "../core/gpu_logiciel.h"

namespace wfs {

// drapeaux de matériau (identiques aux shaders et au format .wfm)
enum MatFlags : uint32_t {
    MAT_PAINT = 1, MAT_BLEND = 2, MAT_TAIL = 4, MAT_HEAD = 8, MAT_DOUBLE = 16, MAT_MASK = 32, MAT_GLASS = 64, MAT_WHEEL = 128,
    MAT_ROAD = 256, MAT_WALK = 512, MAT_GROUND = 1024, MAT_SHADOWBLOB = 2048, MAT_HASNORMAL = 4096, MAT_HASMR = 8192,
    MAT_VTINT = 16384,   // végétation (WFS-34) : la longueur de la normale porte la luminosité du sommet
    // WFS-09 (moteur seulement, jamais dans un .wfm) : tache des phares (multiplie le sol : sol × (1 + tache)) et
    // halo additif (dégradé radial, couleur × intensité dans la normale du sommet)
    MAT_POOL = 32768, MAT_ADD = 65536,
    MAT_FANTOME = 131072,   // WFS-134 (moteur seulement) : fantôme du record, translucide teinté cyan (scene.frag)
    MAT_EMTEX = 262144,     // WFS-134 (moteur seulement) : émission × couleur de la texture (pièces texturées émissives)
    MAT_TANGENTE = 524288,  // WFS-149 (moteur seulement) : carte de normales dans le repère tangent des sommets (sinon dérivées écran)
};

// WFS-149 (RG-04) : + tangente (xyz + signe de la bitangente), 4 octets signés normalisés (/127) ; nulle = sans tangente
// (calculée par Renderer::makeMesh pour la géométrie générée, au chargement pour les voitures : src/render/mesh.h)
struct Vertex { float px, py, pz, nx, ny, nz, u, v; int8_t tx = 0, ty = 0, tz = 0, tw = 0; };
static_assert(sizeof(Vertex) == 36 && offsetof(Vertex, tx) == 32, "Vertex : 36 octets, tangente après les uv (WFS-149)");

struct Material {
    vec4 color{1, 1, 1, 1}; float metal = 0, rough = 0.6f, cutoff = 0.5f; vec3 emissive{0, 0, 0};
    float detail = 0;   // WFS-42 : épaisseur typique des triangles (m) quand la pièce est très fine (< 4 mm), sinon 0
    float taille = 0;   // WFS-134 : plus grande dimension des pièces de ce matériau (m) ; 0 = inconnue
    bool filament = false;   // WFS-42 : pièce opaque faite de fils (baguettes, filets chromés) → dessinée en couverture partielle
    uint32_t flags = 0; VkDescriptorSet set = VK_NULL_HANDLE;
};

struct Mesh { Buffer vb, ib; uint32_t indexCount = 0; bool dynamic = false; vec3 centre{}; float rayon = -1; };   // WFS-82 : sphère englobante (repère local)

// maillage animé par squelette (WFS-63, point d'extension) : sommet = position, normale, uv + 4 os (indices dans la
// palette de l'instance) et 4 poids ; dessiné avec une palette de matrices d'os (skinning dans le vertex shader).
// WFS-72 : cloth = attribut _cloth du jeu web (bras, hauteur, jambe, partie 0 corps / 1 yeux / 2 cheveux)
struct SkinVertex { float px, py, pz, nx, ny, nz, u, v; uint8_t os[4]; float poids[4]; float cloth[4]; };
static_assert(sizeof(SkinVertex) == 68, "SkinVertex : 68 octets (format .wfp version 2)");
// WFS-72 : apparence d'une instance (uniformes de pedMaterial du jeu web, couleurs en codes sRGB 0..1 comme THREE.Color) :
//   peau = multiplicateur de la texture (uSkin), cheveux, haut, bas, chaussures, maillot (uHair … uSwim) ;
//   vetement = (manches, longueur du bas, habillé 0/1, 0) (uCloth) ; corps = (hauteur du cou, taille, profondeur du cou, 0) (uBody,
//   donné par le modèle : Personnage::corps). Sans apparence (nullptr) : rendu de WFS-63 (texture seule).
struct ApparencePersonnage {
    vec3 peau{1, 1, 1}, cheveux{}, haut{}, bas{}, chaussures{}, maillot{};
    vec4 vetement{0.45f, 1.9f, 1, 0}, corps{};
};
struct SkinnedMesh { Buffer vb, ib; uint32_t indexCount = 0; vec3 centre{}; float rayon = -1; bool i16 = false; };   // WFS-210 (RG-05) : indices 16 bits   // WFS-82 : sphère englobante (pose de repos × 1,5)
constexpr int SKIN_MAX_OS = 64;           // matrices d'os par instance (taille de la palette du shader)
constexpr int SKIN_MAX_INSTANCES = 48;    // instances animées par image (RG-05 de WFS-19 : 8 piétons × 2 ; WFS-73 RG-06 : + 16 conducteurs × 2)

struct ModelPart { uint32_t firstIndex, indexCount; int32_t vertexOffset; uint32_t material; int32_t roue = -1; int8_t feuArriere = 0; };   // WFS-60 : roue 0-3, −1 = fixe ; WFS-109 : 1 feu arrière découpé, 2 3e feu stop
// WFS-09 : feux d'une voiture (repère du modèle : avant vers −z). Hauteurs mesurées sur les pièces des feux quand le
// modèle en a (lampY / headY de js/cars/models.js), sinon proportions des voitures du jeu web (d.tail − 0,25, d.nose − 0,12)
// arriereX / avantX : écart latéral du centre de chaque feu (pièces des feux, sinon largeur / 2 − 0,18 / 0,3 du jeu web)
struct FeuxModele { float arriereY = 0, avantY = 0; float largeur = 0, avant = 0, arriere = 0, arriereX = 0, avantX = 0; bool pieceArriere = false; float arriereZ = 0;
                   vec3 recul{}; bool reculTrouve = false; };   // WFS-109 : feu arrière du modèle trouvé, arrière de sa pièce ; WFS-171 : feu de recul mesuré (côté +x)
struct Model {
    Buffer vb, ib; std::vector<ModelPart> parts; std::vector<Material> mats; std::vector<Image> textures;
    std::vector<ModelPart> partsLod;   // WFS-134 (RG-11, RG-12) : version allégée (<id>.lod), vide si absente
    std::vector<ModelPart> partsBasse; // WFS-134 (RG-11) : 4 pièces au plus (peinture, feux, vitres, reste), qualité Basse au loin
    std::vector<std::vector<ModelPart>> partsNiveaux;   // WFS-158 (RG-04) : niveaux de détail 2 et 3 (<id>.lod), après partsLod
    vec3 bmin, bmax; bool ok = false;
    RoueModele roues[4]; int nRoues = 0;   // WFS-60 : roues séparées au chargement (src/game/roues.h)
    FeuxModele feux;
    vec4 siege{};   // WFS-73 (RG-01) : y du pavillon au-dessus de la place du conducteur, x et z de la place, 1 si mesuré (conducteurs::mesurerSiege)
};

// WFS-09 : état des feux d'une voiture (setLights du jeu web : nuit, frein, recul)
struct FeuxVoiture { bool nuit = false, stop = false, recul = false; float frein = -1; };   // WFS-109 : niveau 0-1 du stop (80 ms), < 0 : stop net

struct FrameParams {
    mat4 view, proj; vec3 camPos;
    vec3 sunDir{0.4f, 0.6f, 0.3f}; float sunIntensity = 3.0f; vec3 sunColor{1.0f, 0.95f, 0.88f};
    vec3 skyTop{0.18f, 0.36f, 0.78f}, skyHorizon{0.62f, 0.74f, 0.88f}, ground{0.22f, 0.21f, 0.2f};
    vec3 fogColor{0.6f, 0.7f, 0.82f}; float fogDensity = 0.0022f;
    float exposure = 1.0f, ambient = 1.0f, reflections = 1.0f, time = 0; bool night = false;
    // ombres du soleil (WFS-07) : la boîte d'ombre suit le point focus (la voiture), coupées la nuit
    bool shadows = true, hasFocus = false; vec3 focus{};
    // WFS-45 : projecteurs (three SpotLight, r128 sans lumières physiques), hémisphère, brouillard linéaire. Couleurs linéaires.
    struct Spot { vec3 pos, target; vec3 color{1, 1, 1}; float intensity = 1, distance = 0, angle = PI / 3, penumbra = 0, decay = 1; };
    // WFS-09 : 8 au plus (phares, feu de recul, 4 lampadaires) ; angle ≥ PI = lumière ponctuelle (PointLight)
    Spot spots[8]; int spotCount = 0;                        // 0 projecteur par défaut : rendu inchangé
    vec3 hemiSky{}, hemiGround{}; float hemiIntensity = 0;   // HemisphereLight ; 0 = ambiance du ciel actuelle
    float fogNear = 0, fogFar = 0;                           // fogFar > 0 : brouillard linéaire (Fog) au lieu de l'exponentiel
    // WFS-09 : feux des voitures allumés (nuit) ; WFS-16 : chaussée mouillée, flaques, reflets temps réel de la scène
    // (passe miroir autour du plan horizontal y = refletY, demi-résolution). Tout à faux le jour : rendu inchangé.
    bool feux = false, mouille = false, flaques = false, reflets = false; float refletY = 0;
    float bloom = -1.f;   // WFS-100 : force du bloom de la scène (garage du jeu web : 0,55) ; < 0 : nuit 0,8, jour 0,3
    // WFS-134 (RG-05) : ombre de contact posée sous chaque voiture (draw(Model)) qui n'a pas déjà la sienne ; faux par
    // défaut (garage, scènes dédiées : inchangés), vrai en course (Game::render)
    bool ombresContact = false;
};

struct UiVertex { float x, y, u, v, r, g, b, a; };

struct AccesRendu;   // WFS-150 : passes enfichables (src/render/passes.h), seules à lire l'intérieur du Renderer

class Renderer {
    friend struct AccesRendu;   // WFS-150 (RG-01) : ordonnanceur des passes, pré-passe, capture d'environnement
public:
    bool init(VkCtx& vk);
    void shutdown();
    bool resize();                         // après recréation de la swapchain
    // taille logique (paysage) dans laquelle on dessine la scène et l'interface
    // taille d'affichage (fenêtre) : proportions de la caméra et coordonnées de l'interface / du tactile.
    // Si le tampon n'a pas la même forme que la fenêtre (rotation mal signalée), l'image reste non déformée.
    uint32_t width() const { return vw_ ? vw_ : lw_; }
    uint32_t height() const { return vh_ ? vh_ : lh_; }
    void setViewSize(uint32_t w, uint32_t h) { vw_ = w; vh_ = h; }

    // ressources
    Material makeMaterial(const Material& m, const Image* color, const Image* normal, const Image* mr);
    Mesh makeMesh(const std::vector<Vertex>& v, const std::vector<uint32_t>& idx, bool dynamic = false);
    void updateMesh(Mesh& m, const std::vector<Vertex>& v, const std::vector<uint32_t>& idx);
    void destroy(Mesh& m);
    void destroy(Model& m);
    void destroy(Material& m);   // WFS-187 (accroche E0) : rend le jeu de descripteurs du matériau (GPU arrêté sur ce jeu) ; vide : rien
    // WFS-63 : maillage à squelette (les maillages rigides ne changent pas)
    SkinnedMesh makeSkinnedMesh(const std::vector<SkinVertex>& v, const std::vector<uint32_t>& idx);
    void destroy(SkinnedMesh& m);
    VkCtx& vk() { return *vk_; }

    // image par image
    bool beginFrame(const FrameParams& fp);
    void draw(const Model& m, const mat4& world, const vec3* paint = nullptr, const RouesPose* roues = nullptr);   // WFS-60 : sans pose = rendu inchangé
    void draw(const Mesh& m, const Material& mat, const mat4& world, float roadHalfWidth = 0);
    // WFS-63 : une instance animée (indices [first, first + count) du maillage), palette de `nOs` ≤ SKIN_MAX_OS matrices
    // (espace du modèle : pose courante × inverse de la pose de liaison) ; au-delà de SKIN_MAX_INSTANCES par image : ignorée
    void drawSkinned(const SkinnedMesh& m, const Material& mat, const mat4& world, const mat4* os, int nOs,
                     uint32_t first = 0, uint32_t count = 0, const ApparencePersonnage* apparence = nullptr);
    // WFS-09 : quads transitoires de l'image (4 sommets par quad, triangles 0-1-2 / 0-2-3), dessinés avec les objets
    // transparents ; nQuads ≤ 4096 par image. halo : quad face à la caméra (MAT_ADD), couleur linéaire × intensité ;
    // haloSol : quad couché (repère monde : centre c, axes r et f, demi-tailles), additif ; tache des phares : MAT_POOL
    void drawQuads(const Vertex* v, int nQuads, const Material& mat);
    void halo(const vec3& c, float taille, const vec3& couleur);
    void haloSol(const vec3& c, const vec3& r, const vec3& f, float demiR, float demiF, const vec3& couleur);
    // feux de la prochaine voiture dessinée par draw(Model) (la voiture du joueur : WFS-09, src/render/nuit.cpp) ;
    // remis à l'état de nuit de FrameParams::feux à chaque beginFrame
    void feuxProchaineVoiture(const FeuxVoiture& f) { feuxSuivant_ = f; feuxSuivantPose_ = true; }
    // WFS-134 (RG-07, ANO-60) : la prochaine voiture dessinée par draw(Model) est le fantôme du record : translucide
    // (α 0,45), teintée cyan, sans ombre de contact, sans feux
    void fantomeProchaineVoiture() { fantomeSuivant_ = true; feuxProchaineVoiture(FeuxVoiture{}); }
    // WFS-144 (RG-03 a) : en qualité Basse, la prochaine voiture dessinée par draw(Model) prend sa version en lots de matériaux
    // (partsBasse : peinture, feux, vitres, reste) à toute distance : police de la poursuite, moins d'appels de dessin, jamais de cubes
    void lotsProchaineVoiture() { lotsSuivant_ = true; }
    Material poolMat;   // WFS-09 : matériau de la tache des phares (couleur 0xfff0dc du jeu web)
    void uiRect(float x, float y, float w, float h, const vec4& c);
    void uiCircle(float cx, float cy, float r, const vec4& c);
    void uiQuad(const UiVertex q[4]);   // WFS-65 : quadrilatère à couleurs par sommet (dégradés), u = v = −1
    // texte (WFS-26) : atlas de police en champ de distance signé (canal rouge) ; uiGlyph dessine un caractère
    void uiSetFontAtlas(const Image& atlas);
    // WFS-48 : biais = décalage en x du bord haut par rapport au bord bas (italique synthétique), 0 = droit
    void uiGlyph(float x, float y, float w, float h, float u0, float v0, float u1, float v1, const vec4& c, float biais = 0);
    void endFrame();
    bool capturePng(const std::string& path);   // rendu hors écran seulement

    Image white, flatNormal;
    bool forceNoShadows = false;   // banc d'essai (WFS-07) : rendu sans ombres du soleil, comme la nuit
    bool forceSansReflets = false; // WFS-222 (RG-03) : baisse automatique : pas de passe de reflet (route mouillée)
    bool rendu221 = false;         // WFS-222 : banc seulement (WFS_AVANT_222=1) : rendu d'avant WFS-222, pour comparer images et coûts
    bool qualiteBasse = false;     // WFS-82 : petits objets coupés plus tôt (150 m) en qualité Basse
    uint32_t objetsMasques() const { return masquesPrec_; }   // WFS-82 : objets non dessinés à la dernière image (journal)
    float bloomForce = 1.f;        // WFS-15 : bloom (UnrealBloomPass du jeu web) : 1 en Moyenne / Haute, 0 en Basse
    uint32_t appelsDessin() const { return appelsPrec_; }   // WFS-15 : appels de dessin de la dernière image (journal)
    // WFS-222 (RG-01) : banc de profilage (WFS_BANC=perf) : attente du GPU dans beginFrame, temps processeur de endFrame
    // (enregistrement des passes et envoi) et triangles de la dernière image ; mesure seule, aucun effet sur le rendu
    struct ChronoImage { double attenteGpuMs = 0, finImageMs = 0; uint32_t triangles = 0; };
    const ChronoImage& chronoImage() const { return chronoPrec_; }
    // objets en file pour la passe de scène (vus par la caméra) : appels et triangles, comptés par système au banc
    void fileScene(uint32_t& appels, uint32_t& triangles) const {
        appels = triangles = 0;
        for (const auto* q : {&opaqueQ_, &transparent_}) for (const Pending& p : *q) if (p.vis & VIS_CAM) { appels++; triangles += p.count / 3; }
    }
    // WFS-59 : mode GPU logiciel actif (émulateur de la QA : GPU SwiftShader / llvmpipe, ou intent qa_simu=true)
    bool gpuLogiciel() const { return modeGpuLogiciel(); }
    uint32_t shadowMapSize() const { return shadowSize_; }
    // WFS-149 : coût cumulé du calcul des tangentes de la géométrie générée (makeMesh) : ms et sommets (banc du réalisme)
    double tangentesMs() const { return tangentesMs_; }
    size_t tangentesSommets() const { return tangentesSommets_; }
    float aniso() const { return aniso_; }   // WFS-134 (RG-04) : filtrage anisotrope des textures (×8 si le GPU le permet ; WFS-144 : ×4 en Basse)
private:
    struct PC { mat4 model; vec4 color; vec4 params; vec4 emissive; };
    struct Pending { VkBuffer vb, ib; uint32_t first, count; int32_t vofs; const Material* mat; PC pc; float depth; int32_t osOfs = -1; uint8_t vis = 7; bool i16 = false; };
    // WFS-82 : visibilité d'une sphère monde : bits 1 caméra, 2 reflet, 4 ombre ; 0 = rien à dessiner (objet masqué)
    // WFS-222 : VIS_CAPTURE = gardé pour la capture d'environnement seulement (image d'une capture : la passe de scène, l'ombre
    // et le reflet gardent leur masquage ; avant, les 6 images d'une capture dessinaient tout dans toutes les passes)
    enum : uint8_t { VIS_CAM = 1, VIS_REFLET = 2, VIS_OMBRE = 4, VIS_CAPTURE = 8 };
    uint8_t classer(const masquage::Sphere& s);
    masquage::Frustum fCam_, fReflet_, fOmbre_; float finBrouillard_ = 1e9f; uint32_t masques_ = 0, masquesPrec_ = 0;
    void createTargets();
    void destroyTargets();
    void createPipelines();
    void createScenePipelines();   // WFS-144 : pipelines de la passe de scène (nombre d'échantillons vk.msaa)
    bool appliquerNettete();       // WFS-144 (RG-04) : MSAA et anisotrope du niveau de netteté ; true : passe de scène à refaire
    VkPipeline makePipeline(VkShaderModule vs, VkShaderModule fs, int vertexKind, bool blend, bool premul, bool depthTest, bool depthWrite,
                            VkRenderPass rp, VkSampleCountFlagBits samples, VkPipelineLayout layout, bool depthOnly = false, int melange = 0,
                            bool sansDiscard = false);   // WFS-222 : constante SANS_DISCARD du shader (common.glsl)
    void flushDraw(const Pending& d);
    // WFS-222 : l'objet a besoin des « discard » du shader (alpha masqué, tramage du fondu des voitures, WFS-134 RG-12)
    static bool aDecoupe(const Pending& p) {
        const uint32_t f = (uint32_t)(p.pc.params.w + 0.5f);
        return (f & MAT_MASK) || (p.pc.emissive.w < -0.5f && !(f & (MAT_ROAD | MAT_WALK | MAT_GROUND)));
    }
    void createShadows();
    void shadowPass(VkCommandBuffer cb);
    void readShadowTiming();
    void createReflets();     // WFS-16 : cibles et pipelines de la passe miroir
    void destroyReflets();
    void refletPass(VkCommandBuffer cb);
    void feuxVoiture(const Model& m, const mat4& world, const FeuxVoiture& f);
    void rotationMatrix(float out[4]) const;

    VkCtx* vk_ = nullptr;
    uint32_t lw_ = 0, lh_ = 0;   // taille logique (tampon)
    uint32_t vw_ = 0, vh_ = 0;   // taille d'affichage (fenêtre), 0 = identique
    uint32_t sw_ = 0, sh_ = 0;   // taille de la scène 3D (logique × renderScale)
    VkRenderPass scenePass_ = VK_NULL_HANDLE, finalPass_ = VK_NULL_HANDLE;
    Image msaaColor_, depth_, hdr_;
    VkFramebuffer sceneFb_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> finalFbs_;
    VkDescriptorSetLayout frameLayout_ = VK_NULL_HANDLE, matLayout_ = VK_NULL_HANDLE, postLayout_ = VK_NULL_HANDLE;
    VkPipelineLayout sceneLayout_ = VK_NULL_HANDLE, postPLayout_ = VK_NULL_HANDLE;
    VkPipeline opaque_ = VK_NULL_HANDLE, blend_ = VK_NULL_HANDLE, sky_ = VK_NULL_HANDLE, post_ = VK_NULL_HANDLE, ui_ = VK_NULL_HANDLE;
    VkDescriptorPool pool_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE, clampSampler_ = VK_NULL_HANDLE;
    float aniso_ = 1.f;
    // WFS-144 (RG-04) : netteté de la Basse (MSAA ×2, anisotrope ×4). msaaMax_ : MSAA du GPU (vk.cpp) ; samplerBasse_ : ×4 ;
    // matTex_ : textures de chaque matériau, pour réécrire leur échantillonneur quand la netteté change
    VkSampleCountFlagBits msaaMax_ = VK_SAMPLE_COUNT_1_BIT;
    VkSampler samplerBasse_ = VK_NULL_HANDLE; float anisoMax_ = 1.f; bool texBasse_ = false;
    struct MatTex { VkDescriptorSet set; VkImageView v[3]; };
    std::vector<MatTex> matTex_;
    std::unordered_set<VkDescriptorSet> jeuxMat_;   // WFS-187 : jeux des matériaux vivants (copies d'un matériau : libéré une fois)
    VkShaderModule modScene_[6]{};   // scene.vert, scene.frag, sky.vert, sky.frag, scene_skin.vert, scene_skin.frag
    VkDescriptorSet postSet_ = VK_NULL_HANDLE, fontSet_ = VK_NULL_HANDLE;

    static constexpr int FRAMES = 2;
    struct FrameRes {
        VkCommandBuffer cb = VK_NULL_HANDLE; VkFence fence = VK_NULL_HANDLE;
        VkSemaphore acquired = VK_NULL_HANDLE, done = VK_NULL_HANDLE;
        Buffer ubo, uiVb; VkDescriptorSet frameSet = VK_NULL_HANDLE;
    } fr_[FRAMES];
    int cur_ = 0; uint32_t imageIndex_ = 0; bool inFrame_ = false;
    std::vector<Pending> transparent_, opaqueQ_;   // dessinés à endFrame (après la passe d'ombre)

    // ombres du soleil (WFS-07)
    Image shadowMap_;
    uint32_t shadowSize_ = 2048;
    VkRenderPass shadowPass_ = VK_NULL_HANDLE;
    VkFramebuffer shadowFb_ = VK_NULL_HANDLE;
    VkPipeline shadowPipe_ = VK_NULL_HANDLE, shadowMaskPipe_ = VK_NULL_HANDLE;
    // maillages à squelette (WFS-63) : palettes d'os de l'image dans un tampon uniforme (offset dynamique, set 2)
    VkDescriptorSetLayout osLayout_ = VK_NULL_HANDLE;
    VkPipelineLayout skinLayout_ = VK_NULL_HANDLE;
    VkPipeline skinOpaque_ = VK_NULL_HANDLE, skinShadow_ = VK_NULL_HANDLE, skinShadowMask_ = VK_NULL_HANDLE;
    // WFS-222 : opaques avec découpe (shader avec discard) ; opaque_ et skinOpaque_ sont sans discard (early-Z)
    VkPipeline opaqueDecoupe_ = VK_NULL_HANDLE, skinOpaqueDecoupe_ = VK_NULL_HANDLE;
    VkPipeline skyApres_ = VK_NULL_HANDLE;   // WFS-222 : ciel dessiné après les opaques, test de profondeur (pixels vides seulement)
    std::vector<uint32_t> ordreOpaques_;     // WFS-222 : opaques de la passe de scène, du plus proche au plus loin
    Buffer osBuf_[2]; VkDescriptorSet osSet_[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    int skinUsed_ = 0;
    VkSampler shadowSampler_ = VK_NULL_HANDLE;
    struct Bloom; Bloom* bloom_ = nullptr;   // WFS-100 : chaîne du bloom « dual filter » (renderer.cpp)
    bool shadowsOn_ = false, night_ = false, nuitScene_ = false; uint32_t appels_ = 0, appelsPrec_ = 0;   // WFS-15
    ChronoImage chrono_, chronoPrec_;   // WFS-222
    VkQueryPool tsPool_ = VK_NULL_HANDLE;
    bool tsWritten_[2] = {false, false};
    double shadowMsSum_ = 0, sceneMsSum_ = 0; int shadowMsN_ = 0;
    std::vector<UiVertex> ui_vtx_;
    // WFS-09 / WFS-16 : lumières de nuit et reflets
    VkPipeline poolPipe_ = VK_NULL_HANDLE, addPipe_ = VK_NULL_HANDLE;
    VkRenderPass refletPass_ = VK_NULL_HANDLE;
    Image refletColor_[2], refletDepth_;
    VkFramebuffer refletFb_[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    uint32_t rw_ = 0, rh_ = 0;   // taille de la cible de reflet (demi-résolution de la scène)
    VkPipeline refOpaque_ = VK_NULL_HANDLE, refBlend_ = VK_NULL_HANDLE, refSky_ = VK_NULL_HANDLE, refAdd_ = VK_NULL_HANDLE;
    Buffer refletUbo_[2]; VkDescriptorSet refletSet_[2] = {VK_NULL_HANDLE, VK_NULL_HANDLE};
    bool refletsOn_ = false;
    Buffer quadVb_[2], quadIb_; int quadsUsed_ = 0;
    Material haloMat_;
    mat4 view_ = mat4::identity();
    FeuxVoiture feuxNuit_, feuxSuivant_; bool feuxSuivantPose_ = false;
    // WFS-134 : fantôme, ombres de contact posées par le moteur (quad et matériau propres)
    bool fantomeSuivant_ = false, ombreFaite_ = false, ombresContact_ = false, lotsSuivant_ = false;
    Mesh ombreQuad_; Material ombreMat_;
    double refletMsSum_ = 0;
    vec3 camPos_{};
    float focalePx_ = 864.f;   // WFS-158 (RG-04) : |proj(1,1)| × hauteur de la scène / 2 (taille des voitures à l'écran)
    float exposure_ = 1;
    std::vector<VkShaderModule> modules_;
    double tangentesMs_ = 0; size_t tangentesSommets_ = 0;   // WFS-149
    bool sansMasquage_ = false;   // WFS-150 (RG-03) : image d'une capture d'environnement : aucun objet masqué (6 directions)
};

} // namespace wfs
