// WaForSpeed — module « son » (WFS-10) : voir audio_system.cpp.
#pragma once
#include <string>

namespace wfs {

class Game;
class Renderer;

// état du moteur à un pas de conduite (journal du banc d'essai)
struct EngineFrame { float t, rpm; int gear; float load, kmh; bool throttle; };

// banc d'essai PC : scénario ralenti / plein gaz / lever de pied / freinage / gaz, son calculé hors ligne,
// enregistré dans <out>/moteur.wav et tracé dans <out>/moteur-courbe.png (QA) ; journal : une ligne par seconde
void benchAudioScene(Game& g, Renderer& r, const std::string& out);

} // namespace wfs
