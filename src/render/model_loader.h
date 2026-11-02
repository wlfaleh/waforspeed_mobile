#pragma once
#include <string>
#include "renderer.h"

namespace wfs {
// charge une voiture convertie (.wfm) depuis assets/
bool loadCarModel(Renderer& r, const std::string& path, Model& out);
}
