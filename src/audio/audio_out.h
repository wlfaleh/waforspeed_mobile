// WaForSpeed — sortie audio de la plateforme (WFS-10).
// Android : AAudio, flux flottant à faible latence, STÉRÉO depuis WFS-261 (RG-01) ; le rappel audio appelle `fill` sur son
// propre thread. iOS (gel, WFS-261 RG-01) : mono, inchangé. Bureau (banc d'essai) : pas de carte son, audioOutStart()
// renvoie 0 et le son est calculé hors ligne.
#pragma once
#include <functional>

namespace wfs {

// `fill` remplit `frames` trames de audioOutCanaux() échantillons entrelacés (gauche, droite en stéréo)
using AudioFill = std::function<void(float* out, int frames)>;
// démarre la sortie ; renvoie la fréquence d'échantillonnage obtenue (Hz) ou 0 si pas de sortie
int audioOutStart(AudioFill fill);
void audioOutStop();
#ifdef __ANDROID__
int audioOutCanaux();   // WFS-261 : canaux du flux ouvert (2 demandés ; 1 si l'appareil refuse la stéréo)
#else
inline int audioOutCanaux() { return 1; }   // WFS-261 : iOS (gel) et banc d'essai : mono, comme avant
#endif

} // namespace wfs
