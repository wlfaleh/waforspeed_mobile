// WaForSpeed — services fournis par la plateforme (Android, bureau de test).
#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace wfs {

// lit un fichier du dossier assets/ (APK sur Android, dossier sur le bureau)
bool readAsset(const std::string& path, std::vector<uint8_t>& out);
// temps écoulé (s) depuis le démarrage
double nowSeconds();

} // namespace wfs
