// WaForSpeed — WFS-205 : bibliothèque des revêtements de route (voir route_revetements.h).
#include "route_revetements.h"
#include <algorithm>
#include <string>
#include <vector>
#include "ktx2.h"
#include "../core/log.h"
#include "../core/platform.h"
#include "../world/cartes_v2/carte_donnees.h"   // WFS-346 : saison de la partie (neige au sol)

namespace wfs::route {

namespace {

constexpr const char* CARTES[3] = {"couleur", "normale", "orm"};

struct Charge {
    Image cartes[3]; size_t octets = 0; bool tente = false, ok = false; const char* variante = "absente";
};
Charge g_charges[NB_REVETEMENTS];
int g_qualite = 2;   // Haute
Carte g_carte = Carte::Ville;   // WFS-206
vec3 g_abri{1, 1, 1};           // WFS-346 : 1er tunnel de la carte de données (début, fin, longueur du tour, m) ; {1, 1, 1} : aucun

// une carte : première variante que le GPU sait lire, envoyée sans ses « sauter » premiers niveaux (jamais sous 4×4)
bool chargerCarte(VkCtx& vk, const std::string& base, uint32_t maxi, Image& out, size_t& octets, const char*& variante) {
    for (const char* v : {"astc", "etc2", "rgba8"}) {
        std::vector<uint8_t> brut;
        const std::string chemin = base + "." + v + ".ktx2";
        if (!readAsset(chemin, brut)) continue;
        ktx2::Fichier f; std::string err;
        if (!ktx2::lire(brut.data(), brut.size(), f, err)) { WFS_ERR("%s : KTX2 illisible : %s", chemin.c_str(), err.c_str()); continue; }
        if (!vk.formatEchantillonnable(f.format)) continue;
        int sauter = std::min(niveauxSautes(f.w, maxi), (int)f.niveaux.size() - 1);
        std::vector<const uint8_t*> niv; std::vector<size_t> tailles; size_t total = 0;
        for (size_t k = (size_t)sauter; k < f.niveaux.size(); k++) {
            niv.push_back(f.donnees + f.niveaux[k].offset); tailles.push_back(f.niveaux[k].taille); total += f.niveaux[k].taille;
        }
        out = vk.uploadNiveaux(f.format, f.niveaux[sauter].w, f.niveaux[sauter].h, niv.data(), tailles.data(), (uint32_t)niv.size());
        if (!out.view) continue;
        octets += total; variante = v;
        WFS_LOG("route : %s : %s, %ux%u (%d niveau(x) sauté(s)), %.1f Ko (WFS-205)", chemin.c_str(), ktx2::nomFormat(f.format),
                out.w, out.h, sauter, total / 1024.0);
        return true;
    }
    return false;
}

} // namespace

void preparer(int qualite, const char* mapId) {
    g_qualite = std::clamp(qualite, 0, 3); g_carte = carteDe(mapId);
    // WFS-346 : neige au sol : la chaussée reste sèche sous le toit du 1er tunnel de la carte (route.glsl : abriRoute) ; données du paquet
    g_abri = {1, 1, 1};
    const auto d = mapId ? cartes_v2::donnees(mapId) : nullptr;
    if (d && d->saison.neigeChaussee > 0) {
        int n = 0;
        for (const trace::Section& t : d->sections) if (t.type == trace::Type::Tunnel && n++ == 0) g_abri = {(float)(t.debut * SEG_M), (float)(t.fin * SEG_M), (float)(d->segments() * SEG_M)};
        if (n > 1) WFS_LOG("route (WFS-346) : %s : %d tunnels, chaussée sèche sous le 1er seulement", mapId, n);
    }
}
Revetement chaussee() { return chaussee(g_carte); }

Material creerMateriau(Renderer& r, Revetement rev, const Material& base) {
    const int i = std::clamp((int)rev, 0, NB_REVETEMENTS - 1);
    Charge& c = g_charges[i];
    if (!c.tente) {
        c.tente = true;
        const std::string dossier = std::string("textures/route/") + info(rev).id + "/route_" + info(rev).id + "-";
        bool tout = true;
        for (int k = 0; k < 3 && tout; k++) tout = chargerCarte(r.vk(), dossier + CARTES[k], coteMax(g_qualite, k == 2), c.cartes[k], c.octets, c.variante);
        if (!tout) {   // RG-04 : repli procédural (une carte manque : aucune n'est gardée)
            r.vk().waitIdle();
            for (Image& im : c.cartes) r.vk().destroy(im);
            c.octets = 0; c.variante = "absente";
            WFS_LOG("route : %s : textures absentes ou illisibles, surface procédurale (WFS-205, RG-04)", info(rev).nom);
        }
        c.ok = tout;
    }
    Material m = base;
    m.cutoff = codeMateriau(g_qualite, g_carte, cartes_v2::saisonPartie().neigeChaussee);   // WFS-346 : + neige au sol
    if (g_abri.z > 1) m.color = {g_abri.x, g_abri.y, g_abri.z, 1};   // WFS-346 : couleur inutilisée par la chaussée et le trottoir (route.glsl)   // mode de lecture et carte (route.glsl : pc.params.z) ; repli compris (WFS-206)
    if (!c.ok) return r.makeMaterial(m, nullptr, nullptr, nullptr);
    return r.makeMaterial(m, &c.cartes[0], &c.cartes[1], &c.cartes[2]);
}

void liberer(Renderer& r) {
    bool attendre = false;
    for (const Charge& c : g_charges) attendre = attendre || c.ok;
    if (attendre) r.vk().waitIdle();
    for (Charge& c : g_charges) { for (Image& im : c.cartes) r.vk().destroy(im); c = Charge{}; }
}

size_t octetsGpu() { size_t s = 0; for (const Charge& c : g_charges) s += c.octets; return s; }
const char* variante(Revetement r) { return g_charges[std::clamp((int)r, 0, NB_REVETEMENTS - 1)].variante; }

} // namespace wfs::route
