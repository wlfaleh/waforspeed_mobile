// WaForSpeed — moteur de rendu : passe de scène HDR (MSAA), passe finale (tonemapping + interface).
#pragma once
#include <string>
#include <vector>
#include "vk.h"
#include "../core/math.h"

namespace wfs {

// drapeaux de matériau (identiques aux shaders et au format .wfm)
enum MatFlags : uint32_t {
    MAT_PAINT = 1, MAT_BLEND = 2, MAT_TAIL = 4, MAT_HEAD = 8, MAT_DOUBLE = 16, MAT_MASK = 32, MAT_GLASS = 64, MAT_WHEEL = 128,
    MAT_ROAD = 256, MAT_WALK = 512, MAT_GROUND = 1024, MAT_HASNORMAL = 4096, MAT_HASMR = 8192,
};

struct Vertex { float px, py, pz, nx, ny, nz, u, v; };

struct Material {
    vec4 color{1, 1, 1, 1}; float metal = 0, rough = 0.6f, cutoff = 0.5f; vec3 emissive{0, 0, 0};
    uint32_t flags = 0; VkDescriptorSet set = VK_NULL_HANDLE;
};

struct Mesh { Buffer vb, ib; uint32_t indexCount = 0; bool dynamic = false; };

struct ModelPart { uint32_t firstIndex, indexCount; int32_t vertexOffset; uint32_t material; };
struct Model {
    Buffer vb, ib; std::vector<ModelPart> parts; std::vector<Material> mats; std::vector<Image> textures;
    vec3 bmin, bmax; bool ok = false;
};

struct FrameParams {
    mat4 view, proj; vec3 camPos;
    vec3 sunDir{0.4f, 0.6f, 0.3f}; float sunIntensity = 3.0f; vec3 sunColor{1.0f, 0.95f, 0.88f};
    vec3 skyTop{0.18f, 0.36f, 0.78f}, skyHorizon{0.62f, 0.74f, 0.88f}, ground{0.22f, 0.21f, 0.2f};
    vec3 fogColor{0.6f, 0.7f, 0.82f}; float fogDensity = 0.0022f;
    float exposure = 1.0f, ambient = 1.0f, reflections = 1.0f, time = 0; bool night = false;
};

struct UiVertex { float x, y, u, v, r, g, b, a; };

class Renderer {
public:
    bool init(VkCtx& vk);
    void shutdown();
    bool resize();                         // après recréation de la swapchain
    // taille logique (paysage) dans laquelle on dessine la scène et l'interface
    uint32_t width() const { return lw_; }
    uint32_t height() const { return lh_; }

    // ressources
    Material makeMaterial(const Material& m, const Image* color, const Image* normal, const Image* mr);
    Mesh makeMesh(const std::vector<Vertex>& v, const std::vector<uint32_t>& idx, bool dynamic = false);
    void updateMesh(Mesh& m, const std::vector<Vertex>& v, const std::vector<uint32_t>& idx);
    void destroy(Mesh& m);
    void destroy(Model& m);
    VkCtx& vk() { return *vk_; }

    // image par image
    bool beginFrame(const FrameParams& fp);
    void draw(const Model& m, const mat4& world, const vec3* paint = nullptr);
    void draw(const Mesh& m, const Material& mat, const mat4& world, float roadHalfWidth = 0);
    void uiRect(float x, float y, float w, float h, const vec4& c);
    void uiCircle(float cx, float cy, float r, const vec4& c);
    void endFrame();
    bool capturePng(const std::string& path);   // rendu hors écran seulement

    Image white, flatNormal;
private:
    struct PC { mat4 model; vec4 color; vec4 params; vec4 emissive; };
    struct Pending { VkBuffer vb, ib; uint32_t first, count; int32_t vofs; const Material* mat; PC pc; float depth; };
    void createTargets();
    void destroyTargets();
    void createPipelines();
    VkPipeline makePipeline(VkShaderModule vs, VkShaderModule fs, int vertexKind, bool blend, bool premul, bool depthTest, bool depthWrite,
                            VkRenderPass rp, VkSampleCountFlagBits samples, VkPipelineLayout layout);
    void flushDraw(const Pending& d);
    void rotationMatrix(float out[4]) const;

    VkCtx* vk_ = nullptr;
    uint32_t lw_ = 0, lh_ = 0;   // taille logique
    VkRenderPass scenePass_ = VK_NULL_HANDLE, finalPass_ = VK_NULL_HANDLE;
    Image msaaColor_, depth_, hdr_;
    VkFramebuffer sceneFb_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> finalFbs_;
    VkDescriptorSetLayout frameLayout_ = VK_NULL_HANDLE, matLayout_ = VK_NULL_HANDLE, postLayout_ = VK_NULL_HANDLE;
    VkPipelineLayout sceneLayout_ = VK_NULL_HANDLE, postPLayout_ = VK_NULL_HANDLE;
    VkPipeline opaque_ = VK_NULL_HANDLE, blend_ = VK_NULL_HANDLE, sky_ = VK_NULL_HANDLE, post_ = VK_NULL_HANDLE, ui_ = VK_NULL_HANDLE;
    VkDescriptorPool pool_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE, clampSampler_ = VK_NULL_HANDLE;
    VkDescriptorSet postSet_ = VK_NULL_HANDLE;

    static constexpr int FRAMES = 2;
    struct FrameRes {
        VkCommandBuffer cb = VK_NULL_HANDLE; VkFence fence = VK_NULL_HANDLE;
        VkSemaphore acquired = VK_NULL_HANDLE, done = VK_NULL_HANDLE;
        Buffer ubo, uiVb; VkDescriptorSet frameSet = VK_NULL_HANDLE;
    } fr_[FRAMES];
    int cur_ = 0; uint32_t imageIndex_ = 0; bool inFrame_ = false;
    std::vector<Pending> transparent_;
    std::vector<UiVertex> ui_vtx_;
    vec3 camPos_{};
    float exposure_ = 1;
    std::vector<VkShaderModule> modules_;
};

} // namespace wfs
