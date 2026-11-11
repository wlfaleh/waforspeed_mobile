// WaForSpeed — WFS-419 : interrupteurs de diagnostic de performance. Voir diagnostic_perf.h.
#include "diagnostic_perf.h"
#include <cstdio>
#include "vk.h"
#include "../core/configgame.h"
#if defined(__ANDROID__)
#include <sys/system_properties.h>
#endif

namespace wfs::diag {

namespace {
bool g_menu[N_INTERRUPTEURS] = {};   // état du menu caché (en mémoire seulement)
static_assert(sizeof(ConfigJeu::Diagnostic::coupe) / sizeof(int) == N_INTERRUPTEURS, "une clé de configgame par interrupteur");
}

bool actif(Interrupteur i) { return g_menu[i] || config().diagnostic.coupe[i] != 0; }
bool menu(Interrupteur i) { return g_menu[i]; }
void basculerMenu(Interrupteur i) { g_menu[i] = !g_menu[i]; }
void eteindreMenu() { for (bool& b : g_menu) b = false; }
bool aucun() {
    for (int i = 0; i < N_INTERRUPTEURS; i++) if (actif((Interrupteur)i)) return false;
    return true;
}

std::string texteActifs() {
    std::string s;
    for (int i = 0; i < N_INTERRUPTEURS; i++) if (actif((Interrupteur)i)) s += std::string(s.empty() ? "diag : " : " ") + INFOS[i].court;
    return s;
}

std::string texteAppareil(const VkCtx& vk) {
    static std::string texte;   // calculé une fois (le GPU ne change pas en cours de partie)
    if (!texte.empty()) return texte;
    std::string modele = "PC";
#if defined(__ANDROID__)
    char fab[PROP_VALUE_MAX] = {}, mod[PROP_VALUE_MAX] = {};
    __system_property_get("ro.product.manufacturer", fab);
    __system_property_get("ro.product.model", mod);   // ex. SM-G990B (S21 FE Exynos) / SM-G990B2 / SM-G990U (Snapdragon)
    modele = std::string(fab) + " " + mod;
#endif
    const VkPhysicalDeviceProperties& p = vk.props;
    const uint32_t d = p.driverVersion;
    char pilote[64];
    // version du pilote : codage propre au fabricant. Qualcomm (0x5143) et ARM (0x13B5) suivent celui de Vulkan
    // (10 bits majeur, 10 bits mineur, 12 bits correctif) ; NVIDIA (0x10DE) : 10.8.8.6 ; sinon : nombre brut
    if (p.vendorID == 0x10DE) std::snprintf(pilote, sizeof pilote, "%u.%u", (d >> 22) & 0x3ff, (d >> 14) & 0xff);
    else std::snprintf(pilote, sizeof pilote, "%u.%u.%u (0x%08x)", VK_VERSION_MAJOR(d), VK_VERSION_MINOR(d), VK_VERSION_PATCH(d), d);
    char b[256];
    std::snprintf(b, sizeof b, "%s · GPU %s · pilote %s · Vulkan %u.%u.%u", modele.c_str(), p.deviceName, pilote,
                  VK_VERSION_MAJOR(p.apiVersion), VK_VERSION_MINOR(p.apiVersion), VK_VERSION_PATCH(p.apiVersion));
    texte = b;
    return texte;
}

std::string explicationZero(bool qualiteBasse, bool horodatages) {
    if (!horodatages) return "Temps par passe : horodatages GPU non disponibles sur ce téléphone (les 0,0 ne sont pas des mesures).";
    if (qualiteBasse)
        return "« ombre 0,0 · reflet 0,0 » : en Basse, les ombres du soleil sont coupées (passe d'ombre vide) et le reflet "
               "de la route mouillée aussi (passe de reflet non faite) : ces passes ne coûtent rien, tout est dans « scène ».";
    return "« ombre » et « reflet » à 0,0 : passe d'ombre vide (nuit, ou ombres coupées) ; reflet seulement sous la pluie en Haute.";
}

} // namespace wfs::diag
