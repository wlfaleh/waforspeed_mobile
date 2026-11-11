// WaForSpeed — WFS-425 : description et création de la passe de scène, et copie de la scène en MSAA ×1.
//
// Cause du défaut « MSAA ×1 plus lent que ×2 » (62 → 108 ms sur le S21 FE, Adreno 660) : en ×2 / ×4, les shaders de la scène
// n'écrivent que dans une couleur MSAA et une profondeur TEMPORAIRES (TRANSIENT, mémoire « paresseuse » : elles restent dans
// la mémoire de tuile du GPU mobile), et la cible HDR n'est écrite qu'une fois par pixel, par la résolution de fin de passe.
// En ×1, la passe faisait écrire les shaders DIRECTEMENT dans la cible HDR, une vraie image en mémoire (64 bits par pixel,
// relue ensuite par le bloom et la passe finale) : le pilote peut alors rendre cette passe hors de la mémoire de tuile
// (rendu « direct ») : plus de tri des pixels cachés par la passe de tri des tuiles, et chaque écriture va en mémoire
// principale. Sur un GPU sans tuiles (banc PC, lavapipe), le ×1 est bien plus rapide que le ×4 (98 contre 218 ms).
//
// Correction (×1 seulement) : même schéma que le ×2. Sous-passe 0 : la scène, dans une couleur et une profondeur temporaires
// ×1 ; sous-passe 1 : copie de cette couleur (attachement d'entrée, même tuile, BY_REGION) dans la cible HDR. Les shaders
// n'écrivent plus jamais dans la cible HDR. ×2 et ×4 : description identique à celle d'avant (RG-03).
//
// Attachements (les mêmes indices pour tous les MSAA) : 0 = couleur de la scène (MSAA ou ×1, temporaire),
// 1 = profondeur (temporaire), 2 = cible HDR (résolution en ×2 / ×4, copie en ×1).
#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>

namespace wfs::passe_scene {

constexpr uint32_t ATTACHEMENTS = 3;
constexpr uint32_t COULEUR = 0, PROFONDEUR = 1, CIBLE_HDR = 2;
inline bool avecCopie(VkSampleCountFlagBits e) { return e == VK_SAMPLE_COUNT_1_BIT; }   // ×1 : 2e sous-passe de copie
inline uint32_t sousPasses(VkSampleCountFlagBits e) { return avecCopie(e) ? 2u : 1u; }

// description complète (données seulement, testée sans GPU : tests/test_passe_scene.cpp) ; ci pointe dans la structure,
// qui ne se copie donc pas
struct Description {
    VkAttachmentDescription at[ATTACHEMENTS]{};
    VkAttachmentReference couleur{}, profondeur{}, resolution{}, entree{}, cible{};
    VkSubpassDescription sp[2]{};
    VkSubpassDependency dep[4]{};
    VkRenderPassCreateInfo ci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    Description() = default;
    Description(const Description&) = delete;
    Description& operator=(const Description&) = delete;
};

// d doit être neuve (valeurs par défaut) ; e : nombre d'échantillons de la scène
inline void decrire(Description& d, VkFormat hdr, VkFormat prof, VkSampleCountFlagBits e) {
    const bool ms = !avecCopie(e);
    VkAttachmentDescription* at = d.at;
    for (VkAttachmentDescription& a : d.at) { a.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; a.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; a.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; }
    // 0 : couleur de la scène, temporaire (jamais écrite en mémoire)
    at[COULEUR].format = hdr; at[COULEUR].samples = e;
    at[COULEUR].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; at[COULEUR].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    at[COULEUR].finalLayout = ms ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;   // ×1 : lue en entrée en dernier
    // 1 : profondeur, temporaire
    at[PROFONDEUR].format = prof; at[PROFONDEUR].samples = e;
    at[PROFONDEUR].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; at[PROFONDEUR].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    at[PROFONDEUR].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    // 2 : cible HDR (1 échantillon), seule image gardée : relue par le bloom et la passe finale
    at[CIBLE_HDR].format = hdr; at[CIBLE_HDR].samples = VK_SAMPLE_COUNT_1_BIT;
    at[CIBLE_HDR].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; at[CIBLE_HDR].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    at[CIBLE_HDR].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    d.couleur = {COULEUR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    d.profondeur = {PROFONDEUR, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    d.resolution = {CIBLE_HDR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    d.entree = {COULEUR, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    d.cible = {CIBLE_HDR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    // sous-passe 0 : la scène (ciel, opaques, transparents, passes enfichables des étapes Scene et Transparents)
    VkSubpassDescription& s0 = d.sp[0];
    s0.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    s0.colorAttachmentCount = 1; s0.pColorAttachments = &d.couleur; s0.pDepthStencilAttachment = &d.profondeur;
    if (ms) s0.pResolveAttachments = &d.resolution;
    // dépendances : ×2 / ×4 exactement celles d'avant WFS-425 (RG-03)
    VkSubpassDependency* dep = d.dep;
    dep[0].srcSubpass = VK_SUBPASS_EXTERNAL; dep[0].dstSubpass = 0;
    dep[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dep[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    uint32_t n = 1;
    if (!ms) {
        // sous-passe 1 : copie de la couleur temporaire (attachement d'entrée) dans la cible HDR, dans la même tuile
        VkSubpassDescription& s1 = d.sp[1];
        s1.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        s1.inputAttachmentCount = 1; s1.pInputAttachments = &d.entree;
        s1.colorAttachmentCount = 1; s1.pColorAttachments = &d.cible;
        // image précédente : la cible HDR lue par le bloom et la passe finale avant d'être réécrite (lecture puis écriture)
        dep[n].srcSubpass = VK_SUBPASS_EXTERNAL; dep[n].dstSubpass = 1;
        dep[n].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep[n].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep[n].srcAccessMask = VK_ACCESS_SHADER_READ_BIT; dep[n].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        n++;
        // scène écrite → lue en entrée, pixel par pixel (BY_REGION : le pilote reste dans la tuile)
        dep[n].srcSubpass = 0; dep[n].dstSubpass = 1;
        dep[n].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; dep[n].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dep[n].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; dep[n].dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        dep[n].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        n++;
    }
    // fin de passe : cible HDR écrite → lue par le bloom et la passe finale
    dep[n].srcSubpass = ms ? 0 : 1; dep[n].dstSubpass = VK_SUBPASS_EXTERNAL;
    dep[n].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; dep[n].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dep[n].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; dep[n].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    n++;
    d.ci.attachmentCount = ATTACHEMENTS; d.ci.pAttachments = d.at;
    d.ci.subpassCount = sousPasses(e); d.ci.pSubpasses = d.sp;
    d.ci.dependencyCount = n; d.ci.pDependencies = d.dep;
}

// passe de scène créée (VK_NULL_HANDLE en cas d'échec)
VkRenderPass creer(VkDevice dev, VkFormat hdr, VkFormat prof, VkSampleCountFlagBits e);

// ×1 : sous-passe de copie (couleur temporaire → cible HDR). Disposition, réserve de descripteurs, jeu et shaders créés une
// fois (creer) ; jeu relié à la couleur temporaire à chaque création des cibles (lier) ; pipeline refait avec la passe
// (faire) ; enregistrée en fin de passe de scène (enregistrer : sous-passe suivante + triangle plein écran).
struct Copie {
    VkDescriptorSetLayout disposition = VK_NULL_HANDLE;
    VkPipelineLayout dispositionPipeline = VK_NULL_HANDLE;
    VkDescriptorPool reserve = VK_NULL_HANDLE;
    VkDescriptorSet jeu = VK_NULL_HANDLE;
    VkShaderModule vs = VK_NULL_HANDLE, fs = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    void creer(VkDevice dev);
    void lier(VkDevice dev, VkImageView couleur);
    VkPipeline faire(VkDevice dev, VkRenderPass passe, VkPipelineCache cache) const;
    void enregistrer(VkCommandBuffer cb, uint32_t largeur, uint32_t hauteur) const;
    void detruire(VkDevice dev);   // tout, pipeline compris
};

} // namespace wfs::passe_scene
