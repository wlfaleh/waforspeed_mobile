// WaForSpeed — sortie audio de la plateforme (WFS-10).
// Android : AAudio, flux mono flottant à faible latence ; le rappel audio appelle `fill` sur son propre thread.
// Bureau (banc d'essai) : pas de carte son, audioOutStart() renvoie false et le son est calculé hors ligne.
#pragma once
#include <functional>

namespace wfs {

using AudioFill = std::function<void(float* out, int frames)>;
// démarre la sortie ; renvoie la fréquence d'échantillonnage obtenue (Hz) ou 0 si pas de sortie
int audioOutStart(AudioFill fill);
void audioOutStop();

} // namespace wfs
