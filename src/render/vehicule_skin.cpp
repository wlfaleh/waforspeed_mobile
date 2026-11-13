// WaForSpeed — WFS-492 : skins de la voiture du joueur (voir vehicule_skin.h). Chargement de la texture d'habillage (EAC RG11,
// repli décodé sur le processeur), matériaux habillés des pièces peintes, décodeur EAC.
#include "vehicule_skin.h"
#include <cstring>
#include <vector>
#include "../core/log.h"
#include "../core/platform.h"
#include "ktx2.h"
#include "renderer.h"

namespace wfs::vehicule_skin {

namespace {

// une seule skin en mémoire (RG-03)
struct Pose {
    int skin = AUCUNE;
    std::string voiture, id;
    Image texture;
    bool decodeCpu = false;
    std::vector<Material> mats;   // indice = matériau du modèle ; set nul : pièce non habillée
    const Model* modele = nullptr;
    VkBuffer vb = VK_NULL_HANDLE;   // tampon de sommets du modèle habillé (un modèle détruit puis refait à la même adresse n'est pas pris)
};
Pose g_pose;

} // namespace

void liberer(Renderer& r) {
    for (Material& m : g_pose.mats) r.destroy(m);   // avant la texture : plus aucun jeu ne la lit
    if (g_pose.texture.img) r.vk().destroy(g_pose.texture);
    if (g_pose.skin != AUCUNE) WFS_LOG("skin : %s de %s libérée (WFS-492)", g_pose.id.c_str(), g_pose.voiture.c_str());
    g_pose = Pose{};
}

bool poser(Renderer& r, const Model& m, const char* idVoiture, const char* idSkin) {
    liberer(r);   // RG-03 : une seule texture d'habillage en mémoire
    const int skin = depuisId(idSkin);
    if (skin == AUCUNE || !m.ok || !idVoiture) return false;
    const std::string fichier = chemin(idVoiture, idSkin) + ".etc2.ktx2";
    std::vector<uint8_t> octets;
    if (!readAsset(fichier, octets)) { WFS_LOG("skin : %s absente pour %s (%s) : aucune skin (WFS-492)", idSkin, idVoiture, fichier.c_str()); return false; }
    ktx2::Fichier f; std::string err;
    if (!ktx2::lire(octets.data(), octets.size(), f, err)) { WFS_ERR("%s : KTX2 illisible : %s (WFS-492)", fichier.c_str(), err.c_str()); return false; }
    if (f.format != VK_FORMAT_EAC_R11G11_UNORM_BLOCK) { WFS_ERR("%s : %s, EAC RG11 attendu (WFS-492)", fichier.c_str(), ktx2::nomFormat(f.format)); return false; }
    VkCtx& vk = r.vk();
    std::vector<const uint8_t*> niv; std::vector<size_t> tailles;
    std::vector<std::vector<uint8_t>> decodes;   // repli : niveaux décodés en RG8 (GPU sans ETC2)
    const bool cpu = !vk.formatEchantillonnable(f.format);
    for (const ktx2::Niveau& n : f.niveaux) {
        if (!cpu) { niv.push_back(f.donnees + n.offset); tailles.push_back(n.taille); continue; }
        std::vector<uint8_t> rg((size_t)n.w * n.h * 2);
        const uint32_t bx = (n.w + 3) / 4, by = (n.h + 3) / 4;
        for (uint32_t j = 0; j < by; j++) for (uint32_t i = 0; i < bx; i++) {
            uint8_t px[16][2];
            decoderEacRG11(f.donnees + n.offset + ((size_t)j * bx + i) * 16, px);
            for (uint32_t y = 0; y < 4; y++) for (uint32_t x = 0; x < 4; x++) {
                const uint32_t X = i * 4 + x, Y = j * 4 + y;
                if (X >= n.w || Y >= n.h) continue;   // niveaux de 2 × 2 et 1 × 1 : texels hors image ignorés
                std::memcpy(&rg[((size_t)Y * n.w + X) * 2], px[x + 4 * y], 2);
            }
        }
        decodes.push_back(std::move(rg));
    }
    for (const auto& d : decodes) { niv.push_back(d.data()); tailles.push_back(d.size()); }
    const VkFormat fmt = cpu ? VK_FORMAT_R8G8_UNORM : f.format;
    Image tex = vk.uploadNiveaux(fmt, f.w, f.h, niv.data(), tailles.data(), (uint32_t)niv.size());
    if (!tex.view) { WFS_ERR("%s : envoi au GPU impossible (WFS-492)", fichier.c_str()); return false; }
    // matériaux habillés : copie de chaque matériau peint, texture de couleur = la skin, normales et métal-rugosité d'origine
    std::vector<Material> mats(m.mats.size());
    int n = 0;
    for (size_t i = 0; i < m.mats.size(); i++) {
        const Material& src = m.mats[i];
        if (!(src.flags & MAT_PAINT)) continue;
        VkImageView v[3]{};
        if (!r.vuesMateriau(src.set, v)) continue;
        Image c, nrm, mr; c.view = tex.view; nrm.view = v[1]; mr.view = v[2];
        Material base = src; base.flags &= ~(uint32_t)(MAT_HASNORMAL | MAT_HASMR);   // drapeaux remis par makeMaterial (mêmes vues)
        mats[i] = r.makeMaterial(base, &c, (src.flags & MAT_HASNORMAL) ? &nrm : nullptr, (src.flags & MAT_HASMR) ? &mr : nullptr);
        n++;
    }
    g_pose.skin = skin; g_pose.voiture = idVoiture; g_pose.id = idSkin; g_pose.texture = tex; g_pose.decodeCpu = cpu;
    g_pose.mats = std::move(mats); g_pose.modele = &m; g_pose.vb = m.vb.buf;
    WFS_LOG("skin : %s posée sur %s : %s %ux%u, %zu niveaux, %.1f Ko sur le GPU, %d pièces peintes habillées ; 1 texture d'habillage en mémoire (WFS-492)",
            idSkin, idVoiture, cpu ? "RG8 (EAC décodé sur le processeur : GPU sans ETC2)" : "EAC RG11", f.w, f.h, f.niveaux.size(),
            tex.octetsTexture / 1024.0, n);
    return n > 0;
}

const Material* materiau(const Model& m, uint32_t i) {
    if (g_pose.skin == AUCUNE || &m != g_pose.modele || m.vb.buf != g_pose.vb || i >= g_pose.mats.size()) return nullptr;
    return g_pose.mats[i].set ? &g_pose.mats[i] : nullptr;
}

float coder(float metalCode) { return g_pose.skin == AUCUNE ? metalCode : metalCode + 16.f * (float)g_pose.skin; }

Etat etat() {
    Etat e;
    e.skin = g_pose.skin; e.voiture = g_pose.voiture; e.id = g_pose.id; e.textures = g_pose.texture.view ? 1 : 0;
    e.octetsGpu = g_pose.texture.octetsTexture; e.decodeCpu = g_pose.decodeCpu;
    for (const Material& m : g_pose.mats) e.materiaux += m.set ? 1 : 0;
    return e;
}

} // namespace wfs::vehicule_skin
