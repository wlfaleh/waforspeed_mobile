// WaForSpeed — versions allégées des voitures (WFS-134, RG-11) : voir voitures_lod.h.
#include "voitures_lod.h"
#include <cstring>
#include <map>
#include <string>
#include "../core/log.h"
#include "../core/system.h"
#include "model_loader.h"

namespace wfs {
namespace {

std::map<std::string, Model> g_lod;   // modèles chargés (ok = false : absent, déjà essayé)
Renderer* g_r = nullptr;

class VoituresLod : public System {
public:
    const char* name() const override { return "voitures allégées"; }
    void init(Renderer& r, const GameView&) override { g_r = &r; }
    void shutdown(Renderer& r) override {
        for (auto& [id, m] : g_lod) if (m.ok) r.destroy(m);
        g_lod.clear(); g_r = nullptr;
    }
};
WFS_SYSTEM(VoituresLod, 301)

} // namespace

const Model* voitures_lod::modele(Renderer& r, const char* id) {
    if (!id || g_r != &r) return nullptr;   // hors partie : rien (aucune ressource sans propriétaire)
    auto it = g_lod.find(id);
    if (it == g_lod.end()) {
        Model m;
        const std::string f = std::string("cars/") + id + "-lod.wfm";
        if (loadCarModel(r, f, m)) WFS_LOG("voiture allégée %s : %zu pièces (de 60 à 150 m, WFS-134)", id, m.parts.size());
        else WFS_LOG("voiture allégée %s : absente (modèle complet gardé)", id);
        it = g_lod.emplace(id, std::move(m)).first;
    }
    return it->second.ok ? &it->second : nullptr;
}

} // namespace wfs
