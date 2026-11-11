// WaForSpeed — WFS-425 : passe de scène et copie de la scène en MSAA ×1. Voir passe_scene.h.
#include "passe_scene.h"
#include "vk.h"

namespace wfs::passe_scene {

static const uint32_t SPV_PLEIN_ECRAN_VERT[] = {
#include "plein_ecran.vert.inc"
};
static const uint32_t SPV_SCENE_COPIE_FRAG[] = {
#include "scene_copie.frag.inc"
};

VkRenderPass creer(VkDevice dev, VkFormat hdr, VkFormat prof, VkSampleCountFlagBits e) {
    Description d;
    decrire(d, hdr, prof, e);
    VkRenderPass p = VK_NULL_HANDLE;
    if (vkCreateRenderPass(dev, &d.ci, nullptr, &p) != VK_SUCCESS) return VK_NULL_HANDLE;
    return p;
}

void Copie::creer(VkDevice dev) {
    if (disposition) return;
    const VkDescriptorSetLayoutBinding b{0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    VkDescriptorSetLayoutCreateInfo lci{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    lci.bindingCount = 1; lci.pBindings = &b;
    VK_OK(vkCreateDescriptorSetLayout(dev, &lci, nullptr, &disposition));
    VkPipelineLayoutCreateInfo plci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    plci.setLayoutCount = 1; plci.pSetLayouts = &disposition;
    VK_OK(vkCreatePipelineLayout(dev, &plci, nullptr, &dispositionPipeline));
    const VkDescriptorPoolSize taille{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1};
    VkDescriptorPoolCreateInfo pci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pci.maxSets = 1; pci.poolSizeCount = 1; pci.pPoolSizes = &taille;
    VK_OK(vkCreateDescriptorPool(dev, &pci, nullptr, &reserve));
    VkDescriptorSetAllocateInfo dai{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    dai.descriptorPool = reserve; dai.descriptorSetCount = 1; dai.pSetLayouts = &disposition;
    VK_OK(vkAllocateDescriptorSets(dev, &dai, &jeu));
    auto module = [&](const uint32_t* code, size_t octets) {
        VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        ci.codeSize = octets; ci.pCode = code;
        VkShaderModule m = VK_NULL_HANDLE; VK_OK(vkCreateShaderModule(dev, &ci, nullptr, &m));
        return m;
    };
    vs = module(SPV_PLEIN_ECRAN_VERT, sizeof SPV_PLEIN_ECRAN_VERT);
    fs = module(SPV_SCENE_COPIE_FRAG, sizeof SPV_SCENE_COPIE_FRAG);
}

void Copie::lier(VkDevice dev, VkImageView couleur) {
    if (!jeu || !couleur) return;
    const VkDescriptorImageInfo ii{VK_NULL_HANDLE, couleur, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    w.dstSet = jeu; w.dstBinding = 0; w.descriptorCount = 1; w.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT; w.pImageInfo = &ii;
    vkUpdateDescriptorSets(dev, 1, &w, 0, nullptr);
}

// triangle plein écran, sans sommets ni profondeur ni mélange : chaque pixel de la zone dessinée recopié une fois
VkPipeline Copie::faire(VkDevice dev, VkRenderPass passe, VkPipelineCache cache) const {
    VkPipelineShaderStageCreateInfo st[2]{};
    st[0].sType = st[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    st[0].stage = VK_SHADER_STAGE_VERTEX_BIT; st[0].module = vs; st[0].pName = "main";
    st[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT; st[1].module = fs; st[1].pName = "main";
    VkPipelineVertexInputStateCreateInfo vi{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo ia{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo vp{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vp.viewportCount = 1; vp.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo rs{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rs.polygonMode = VK_POLYGON_MODE_FILL; rs.cullMode = VK_CULL_MODE_NONE; rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; rs.lineWidth = 1.f;
    VkPipelineMultisampleStateCreateInfo ms{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineColorBlendAttachmentState cb{};
    cb.colorWriteMask = 0xF;
    VkPipelineColorBlendStateCreateInfo cbs{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cbs.attachmentCount = 1; cbs.pAttachments = &cb;
    const VkDynamicState dyn[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dy{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dy.dynamicStateCount = 2; dy.pDynamicStates = dyn;
    VkGraphicsPipelineCreateInfo gp{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    gp.stageCount = 2; gp.pStages = st; gp.pVertexInputState = &vi; gp.pInputAssemblyState = &ia; gp.pViewportState = &vp;
    gp.pRasterizationState = &rs; gp.pMultisampleState = &ms; gp.pColorBlendState = &cbs; gp.pDynamicState = &dy;
    gp.layout = dispositionPipeline; gp.renderPass = passe; gp.subpass = 1;   // sous-passe de copie
    VkPipeline p = VK_NULL_HANDLE;
    VK_OK(vkCreateGraphicsPipelines(dev, cache, 1, &gp, nullptr, &p));
    return p;
}

void Copie::enregistrer(VkCommandBuffer cb, uint32_t largeur, uint32_t hauteur) const {
    vkCmdNextSubpass(cb, VK_SUBPASS_CONTENTS_INLINE);
    const VkViewport v{0, 0, (float)largeur, (float)hauteur, 0, 1};
    const VkRect2D s{{0, 0}, {largeur, hauteur}};
    vkCmdSetViewport(cb, 0, 1, &v); vkCmdSetScissor(cb, 0, 1, &s);
    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, dispositionPipeline, 0, 1, &jeu, 0, nullptr);
    vkCmdDraw(cb, 3, 1, 0, 0);
}

void Copie::detruire(VkDevice dev) {
    if (pipeline) vkDestroyPipeline(dev, pipeline, nullptr);
    if (vs) vkDestroyShaderModule(dev, vs, nullptr);
    if (fs) vkDestroyShaderModule(dev, fs, nullptr);
    if (reserve) vkDestroyDescriptorPool(dev, reserve, nullptr);   // libère aussi le jeu
    if (dispositionPipeline) vkDestroyPipelineLayout(dev, dispositionPipeline, nullptr);
    if (disposition) vkDestroyDescriptorSetLayout(dev, disposition, nullptr);
    *this = Copie{};
}

} // namespace wfs::passe_scene
