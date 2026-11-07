// WaForSpeed — WFS-206 (RG-05) : mesures des revêtements (voir route_mesures.h).
#include "route_mesures.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include "ktx2.h"

namespace wfs::route {

namespace {
double lin(double c) { return c <= 0.04045 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4); }
bool charger(const std::string& chemin, std::vector<uint8_t>& o) {
    std::ifstream f(chemin, std::ios::binary);
    if (!f) return false;
    o.assign(std::istreambuf_iterator<char>(f), {});
    return true;
}
} // namespace

std::vector<MesureRevetement> mesurerBibliotheque(const std::string& depot) {
    std::vector<MesureRevetement> out;
    for (int i = 0; i < NB_REVETEMENTS; i++) {
        const InfoRevetement& in = info((Revetement)i);
        const std::string base = depot + "/assets/textures/route/" + in.id + "/route_" + in.id + "-";
        for (const char* carte : {"couleur", "orm"}) {
            std::vector<uint8_t> o; ktx2::Fichier f; std::string err;
            if (!charger(base + carte + ".rgba8.ktx2", o) || !ktx2::lire(o.data(), o.size(), f, err) || f.bloc.compresse || f.niveaux.empty()) continue;
            const ktx2::Niveau& n0 = f.niveaux[0];
            const size_t n = (size_t)n0.w * n0.h;
            if (n == 0 || n0.offset + n * 4 > o.size()) continue;
            const uint8_t* px = f.donnees + n0.offset;
            if (std::string(carte) == "couleur") {   // M-01 : luminance Rec. 709, sRGB décodé en linéaire
                double s = 0;
                for (size_t k = 0; k < n; k++) s += 0.2126 * lin(px[4 * k] / 255.0) + 0.7152 * lin(px[4 * k + 1] / 255.0) + 0.0722 * lin(px[4 * k + 2] / 255.0);
                out.push_back({in.id, "M-01 albédo moyen", s / n, in.albedoMin, in.albedoMax, false});
            } else {                                  // M-02 : canal G de l'ORM ; métal (canal B) jamais
                double g = 0, b = 0;
                for (size_t k = 0; k < n; k++) { g += px[4 * k + 1] / 255.0; b = std::max(b, px[4 * k + 2] / 255.0); }
                out.push_back({in.id, "M-02 rugosité sèche moyenne", g / n, in.rugositeMin, in.rugositeMax, b >= 0.01});
            }
        }
    }
    return out;
}

} // namespace wfs::route
