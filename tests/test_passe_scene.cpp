// WFS-425 : description de la passe de scène (sans GPU). usage : test_passe_scene
//   - RG-01 (cause corrigée) : en MSAA ×1, les shaders de la scène n'écrivent plus dans la cible HDR : la sous-passe 0 écrit une
//     couleur et une profondeur temporaires (jamais gardées en mémoire), la sous-passe 1 les recopie dans la cible HDR, pixel
//     par pixel (attachement d'entrée, dépendance BY_REGION) ; la cible HDR n'est écrite qu'une fois, comme en ×2 / ×4 ;
//   - RG-03 : en ×2 et ×4, description identique au bit près à celle d'avant WFS-425 (copie littérale de l'ancien
//     createTargets ci-dessous) ;
//   - même attachements (mêmes indices, mêmes formats) pour tous les MSAA : la passe du cache des variantes (WFS-262) est la
//     même que celle de la course.
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include "../src/render/passe_scene.h"
using namespace wfs;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

constexpr VkFormat HDR = VK_FORMAT_R16G16B16A16_SFLOAT, PROF = VK_FORMAT_D32_SFLOAT;

// passe de scène ×2 / ×4 telle que createTargets la décrivait avant WFS-425 (renderer.cpp, MSAA ms = vrai)
struct Ancienne {
    VkAttachmentDescription at[3]{};
    VkAttachmentReference colRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}, depRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL},
                          resRef{2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sp{};
    VkSubpassDependency dep[2]{};
    explicit Ancienne(VkSampleCountFlagBits e) {
        const bool ms = true;
        at[0].format = HDR; at[0].samples = e;
        at[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; at[0].storeOp = ms ? VK_ATTACHMENT_STORE_OP_DONT_CARE : VK_ATTACHMENT_STORE_OP_STORE;
        at[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; at[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        at[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        at[0].finalLayout = ms ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        at[1].format = PROF; at[1].samples = e;
        at[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; at[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        at[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; at[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        at[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; at[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        at[2].format = HDR; at[2].samples = VK_SAMPLE_COUNT_1_BIT;
        at[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; at[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        at[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; at[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        at[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; at[2].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sp.colorAttachmentCount = 1; sp.pColorAttachments = &colRef; sp.pDepthStencilAttachment = &depRef;
        if (ms) sp.pResolveAttachments = &resRef;
        dep[0].srcSubpass = VK_SUBPASS_EXTERNAL; dep[0].dstSubpass = 0;
        dep[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dep[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dep[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dep[1].srcSubpass = 0; dep[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dep[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; dep[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dep[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; dep[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    }
};

static bool memeRef(const VkAttachmentReference* a, const VkAttachmentReference* b) {
    if (!a || !b) return a == b;
    return a->attachment == b->attachment && a->layout == b->layout;
}
static bool memeDep(const VkSubpassDependency& a, const VkSubpassDependency& b) { return std::memcmp(&a, &b, sizeof a) == 0; }
static bool memeAt(const VkAttachmentDescription& a, const VkAttachmentDescription& b) { return std::memcmp(&a, &b, sizeof a) == 0; }

// numéros des sous-passes qui écrivent l'attachement k (couleur ou résolution)
static int ecrivains(const passe_scene::Description& d, uint32_t k, int* sp0) {
    int n = 0;
    for (uint32_t s = 0; s < d.ci.subpassCount; s++) {
        const VkSubpassDescription& p = d.ci.pSubpasses[s];
        bool ecrit = false;
        for (uint32_t c = 0; c < p.colorAttachmentCount; c++) ecrit |= p.pColorAttachments[c].attachment == k;
        if (p.pResolveAttachments) for (uint32_t c = 0; c < p.colorAttachmentCount; c++) ecrit |= p.pResolveAttachments[c].attachment == k;
        if (ecrit) { if (!n) *sp0 = (int)s; n++; }
    }
    return n;
}

int main() {
    // RG-03 : ×2 et ×4 inchangés
    for (VkSampleCountFlagBits e : {VK_SAMPLE_COUNT_2_BIT, VK_SAMPLE_COUNT_4_BIT}) {
        passe_scene::Description d; passe_scene::decrire(d, HDR, PROF, e);
        const Ancienne a(e);
        VERIFIE(d.ci.attachmentCount == 3 && d.ci.subpassCount == 1 && d.ci.dependencyCount == 2, "×%d : 3 attachements, 1 sous-passe, 2 dépendances (%u, %u, %u)",
                (int)e, d.ci.attachmentCount, d.ci.subpassCount, d.ci.dependencyCount);
        for (int k = 0; k < 3; k++) VERIFIE(memeAt(d.at[k], a.at[k]), "×%d : attachement %d différent d'avant WFS-425", (int)e, k);
        const VkSubpassDescription& s = d.ci.pSubpasses[0];
        VERIFIE(s.colorAttachmentCount == 1 && memeRef(s.pColorAttachments, &a.colRef) && memeRef(s.pDepthStencilAttachment, &a.depRef) &&
                memeRef(s.pResolveAttachments, &a.resRef) && s.inputAttachmentCount == 0, "×%d : sous-passe différente d'avant WFS-425", (int)e);
        for (int k = 0; k < 2; k++) VERIFIE(memeDep(d.ci.pDependencies[k], a.dep[k]), "×%d : dépendance %d différente d'avant WFS-425", (int)e, k);
        VERIFIE(!passe_scene::avecCopie(e) && passe_scene::sousPasses(e) == 1, "×%d : pas de copie", (int)e);
    }

    // RG-01 : ×1, scène dans des attachements temporaires, copiée dans la cible HDR par la 2e sous-passe
    {
        const VkSampleCountFlagBits e = VK_SAMPLE_COUNT_1_BIT;
        passe_scene::Description d; passe_scene::decrire(d, HDR, PROF, e);
        VERIFIE(passe_scene::avecCopie(e) && d.ci.subpassCount == 2 && d.ci.attachmentCount == 3, "×1 : 2 sous-passes, 3 attachements");
        const VkSubpassDescription& s0 = d.ci.pSubpasses[0];
        const VkSubpassDescription& s1 = d.ci.pSubpasses[1];
        VERIFIE(s0.colorAttachmentCount == 1 && s0.pColorAttachments[0].attachment == passe_scene::COULEUR && s0.pResolveAttachments == nullptr &&
                s0.pDepthStencilAttachment && s0.pDepthStencilAttachment->attachment == passe_scene::PROFONDEUR,
                "×1 : la sous-passe 0 écrit la couleur et la profondeur temporaires");
        int premier = -1;
        VERIFIE(ecrivains(d, passe_scene::CIBLE_HDR, &premier) == 1 && premier == 1, "×1 : la cible HDR n'est écrite que par la sous-passe de copie (1)");
        VERIFIE(s1.inputAttachmentCount == 1 && s1.pInputAttachments[0].attachment == passe_scene::COULEUR &&
                s1.pInputAttachments[0].layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && s1.pDepthStencilAttachment == nullptr,
                "×1 : la sous-passe 1 lit la couleur temporaire en entrée, sans profondeur");
        const VkAttachmentDescription& c = d.at[passe_scene::COULEUR];
        const VkAttachmentDescription& z = d.at[passe_scene::PROFONDEUR];
        const VkAttachmentDescription& h = d.at[passe_scene::CIBLE_HDR];
        VERIFIE(c.samples == e && c.format == HDR && c.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR && c.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE,
                "×1 : couleur temporaire effacée au début, jamais gardée en mémoire");
        VERIFIE(c.finalLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, "×1 : couleur temporaire laissée dans son dernier usage (entrée)");
        VERIFIE(z.samples == e && z.format == PROF && z.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR && z.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE,
                "×1 : profondeur temporaire");
        VERIFIE(h.samples == VK_SAMPLE_COUNT_1_BIT && h.format == HDR && h.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE && h.storeOp == VK_ATTACHMENT_STORE_OP_STORE &&
                h.finalLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, "×1 : cible HDR écrite une fois, gardée, lisible par le bloom et la passe finale");
        // dépendances : 0 → 1 pixel par pixel, image précédente → 1 (cible relue avant d'être réécrite), 1 → extérieur
        bool interne = false, avant = false, apres = false;
        for (uint32_t k = 0; k < d.ci.dependencyCount; k++) {
            const VkSubpassDependency& p = d.ci.pDependencies[k];
            if (p.srcSubpass == 0 && p.dstSubpass == 1)
                interne = (p.dependencyFlags & VK_DEPENDENCY_BY_REGION_BIT) && (p.srcAccessMask & VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT) &&
                          (p.dstAccessMask & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT) && (p.dstStageMask & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
            if (p.srcSubpass == VK_SUBPASS_EXTERNAL && p.dstSubpass == 1)
                avant = (p.srcStageMask & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT) && (p.dstAccessMask & VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
            if (p.srcSubpass == 1 && p.dstSubpass == VK_SUBPASS_EXTERNAL)
                apres = (p.srcAccessMask & VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT) && (p.dstAccessMask & VK_ACCESS_SHADER_READ_BIT);
            VERIFIE(!(p.srcSubpass == 0 && p.dstSubpass == VK_SUBPASS_EXTERNAL), "×1 : pas de sortie depuis la sous-passe 0 (la cible est écrite par la 1)");
        }
        VERIFIE(interne, "×1 : dépendance 0 → 1 BY_REGION (écriture couleur → lecture d'entrée)");
        VERIFIE(avant, "×1 : dépendance image précédente → copie (cible HDR relue avant d'être réécrite)");
        VERIFIE(apres, "×1 : dépendance copie → bloom et passe finale");
    }

    // mêmes attachements (formats, indices) pour tous les MSAA : seul le nombre d'échantillons des temporaires change
    {
        passe_scene::Description d1, d4;
        passe_scene::decrire(d1, HDR, PROF, VK_SAMPLE_COUNT_1_BIT); passe_scene::decrire(d4, HDR, PROF, VK_SAMPLE_COUNT_4_BIT);
        for (uint32_t k = 0; k < passe_scene::ATTACHEMENTS; k++) VERIFIE(d1.at[k].format == d4.at[k].format, "attachement %u : même format en ×1 et ×4", k);
        VERIFIE(d1.at[passe_scene::CIBLE_HDR].samples == VK_SAMPLE_COUNT_1_BIT && d4.at[passe_scene::CIBLE_HDR].samples == VK_SAMPLE_COUNT_1_BIT, "cible HDR à 1 échantillon");
    }

    if (echecs) { std::printf("test_passe_scene : %d échec(s)\n", echecs); return 1; }
    std::printf("test_passe_scene : OK (×2 / ×4 inchangés, ×1 : scène temporaire copiée dans la cible HDR)\n");
    return 0;
}
