// WaForSpeed — sortie audio Android par AAudio (WFS-10). Compilé seulement pour Android.
#include "audio_out.h"
#ifdef __ANDROID__
#include <aaudio/AAudio.h>
#include <atomic>
#include <thread>
#include "../core/log.h"

namespace wfs {

static AAudioStream* g_stream = nullptr;
static AudioFill g_fill;
static std::atomic<bool> g_restart{false};

static aaudio_data_callback_result_t onData(AAudioStream*, void*, void* data, int32_t frames) {
    g_fill(static_cast<float*>(data), frames);   // aucune allocation ni verrou ici
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

static bool openStream();
// casque branché / débranché, Bluetooth : le flux est perdu, on le rouvre (hors du thread audio)
static void onError(AAudioStream*, void*, aaudio_result_t err) {
    WFS_LOG("son : flux perdu (%s), réouverture", AAudio_convertResultToText(err));
    if (g_restart.exchange(true)) return;
    std::thread([] {
        if (g_stream) { AAudioStream_close(g_stream); g_stream = nullptr; }
        openStream();
        g_restart = false;
    }).detach();
}

static bool openStream() {
    AAudioStreamBuilder* b = nullptr;
    if (AAudio_createStreamBuilder(&b) != AAUDIO_OK) return false;
    AAudioStreamBuilder_setDirection(b, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setPerformanceMode(b, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setSharingMode(b, AAUDIO_SHARING_MODE_SHARED);
    AAudioStreamBuilder_setFormat(b, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setChannelCount(b, 1);
    AAudioStreamBuilder_setSampleRate(b, 48000);   // le synthétiseur est réglé à 48 kHz avant le premier rappel
    AAudioStreamBuilder_setDataCallback(b, onData, nullptr);
    AAudioStreamBuilder_setErrorCallback(b, onError, nullptr);
    const aaudio_result_t r = AAudioStreamBuilder_openStream(b, &g_stream);
    AAudioStreamBuilder_delete(b);
    if (r != AAUDIO_OK) { WFS_ERR("son : ouverture AAudio impossible (%s)", AAudio_convertResultToText(r)); g_stream = nullptr; return false; }
    // tampon de 2 rafales : latence faible sans risque de trou (craquement)
    AAudioStream_setBufferSizeInFrames(g_stream, AAudioStream_getFramesPerBurst(g_stream) * 2);
    AAudioStream_requestStart(g_stream);
    WFS_LOG("son : AAudio %d Hz, rafale %d échantillons", AAudioStream_getSampleRate(g_stream), AAudioStream_getFramesPerBurst(g_stream));
    return true;
}

int audioOutStart(AudioFill fill) {
    audioOutStop();
    g_fill = std::move(fill);
    if (!openStream()) return 0;
    return AAudioStream_getSampleRate(g_stream);
}

void audioOutStop() {
    if (g_stream) { AAudioStream_requestStop(g_stream); AAudioStream_close(g_stream); g_stream = nullptr; }
}

} // namespace wfs
#else
namespace wfs {
int audioOutStart(AudioFill) { return 0; }   // banc d'essai : pas de carte son
void audioOutStop() {}
}
#endif
