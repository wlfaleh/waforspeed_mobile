// WaForSpeed — sortie audio iOS par Core Audio (WFS-79) : unité RemoteIO, flux mono flottant 32 bits à 48 kHz.
// Même contrat que audio_out_android.cpp : le rappel audio appelle `fill` ; audioOutStart renvoie la fréquence obtenue
// ou 0 (le jeu continue sans son). API C d'AudioToolbox seulement ; la session audio passe par le runtime Objective-C.
#include "audio_out.h"
#if defined(__APPLE__)
#include <AudioToolbox/AudioToolbox.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <atomic>
#include <chrono>
#include <thread>
#include "../core/log.h"

namespace wfs {

static AudioUnit g_unit = nullptr;
static AudioFill g_fill;
static std::atomic<long> g_rappels{0};
static std::atomic<int> g_generation{0};   // invalide le compteur différé quand le flux est arrêté

static OSStatus onRender(void*, AudioUnitRenderActionFlags*, const AudioTimeStamp*, UInt32, UInt32 frames, AudioBufferList* io) {
    g_fill(static_cast<float*>(io->mBuffers[0].mData), static_cast<int>(frames));   // aucune allocation ni verrou ici
    g_rappels.fetch_add(1, std::memory_order_relaxed);
    return noErr;
}

// session « ambiante » (RG-03) : respecte le bouton silencieux et se mélange à la musique de l'utilisateur
static void sessionAmbiante() {
    using Msg0 = id (*)(id, SEL);
    using MsgCat = BOOL (*)(id, SEL, id, id*);
    using MsgActif = BOOL (*)(id, SEL, BOOL, id*);
    id cls = reinterpret_cast<id>(objc_getClass("AVAudioSession"));
    if (!cls) { WFS_WARN("audio iOS : AVAudioSession absente, catégorie par défaut"); return; }
    id s = reinterpret_cast<Msg0>(objc_msgSend)(cls, sel_registerName("sharedInstance"));
    // AVAudioSessionCategoryAmbient vaut la chaîne « AVAudioSessionCategoryAmbient » (CFString = NSString)
    id cat = (id)CFSTR("AVAudioSessionCategoryAmbient");
    const BOOL ok = reinterpret_cast<MsgCat>(objc_msgSend)(s, sel_registerName("setCategory:error:"), cat, nullptr);
    reinterpret_cast<MsgActif>(objc_msgSend)(s, sel_registerName("setActive:error:"), YES, nullptr);
    if (!ok) WFS_WARN("audio iOS : catégorie ambiante refusée");
}

static bool echec(OSStatus st, const char* quoi) {
    if (st == noErr) return false;
    WFS_ERR("audio iOS : %s impossible (%d), pas de son", quoi, static_cast<int>(st));
    if (g_unit) { AudioComponentInstanceDispose(g_unit); g_unit = nullptr; }
    return true;
}

int audioOutStart(AudioFill fill) {
    audioOutStop();
    g_fill = std::move(fill);
    sessionAmbiante();
    AudioComponentDescription d{};
    d.componentType = kAudioUnitType_Output;
    d.componentSubType = kAudioUnitSubType_RemoteIO;
    d.componentManufacturer = kAudioUnitManufacturer_Apple;
    AudioComponent comp = AudioComponentFindNext(nullptr, &d);
    if (!comp) { WFS_ERR("audio iOS : unité RemoteIO introuvable, pas de son"); return 0; }
    if (echec(AudioComponentInstanceNew(comp, &g_unit), "création RemoteIO")) return 0;

    AudioStreamBasicDescription f{};
    f.mSampleRate = 48000.0;   // le synthétiseur est réglé sur la fréquence renvoyée avant le premier rappel
    f.mFormatID = kAudioFormatLinearPCM;
    f.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    f.mChannelsPerFrame = 1;
    f.mBitsPerChannel = 32;
    f.mFramesPerPacket = 1;
    f.mBytesPerFrame = f.mBytesPerPacket = 4;
    if (echec(AudioUnitSetProperty(g_unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &f, sizeof f), "format mono flottant")) return 0;
    AURenderCallbackStruct cb{onRender, nullptr};
    if (echec(AudioUnitSetProperty(g_unit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &cb, sizeof cb), "rappel")) return 0;
    if (echec(AudioUnitInitialize(g_unit), "initialisation")) return 0;

    AudioStreamBasicDescription obtenu{};
    UInt32 taille = sizeof obtenu;
    AudioUnitGetProperty(g_unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &obtenu, &taille);
    const int rate = obtenu.mSampleRate > 0 ? static_cast<int>(obtenu.mSampleRate) : 48000;
    g_rappels = 0;
    if (echec(AudioOutputUnitStart(g_unit), "démarrage")) return 0;
    WFS_LOG("audio iOS : %d Hz", rate);
    // critère d'acceptation : nombre de rappels au bout de 10 s (journal du simulateur)
    const int gen = ++g_generation;
    std::thread([gen] {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        if (gen == g_generation.load()) WFS_LOG("audio iOS : %ld rappels", g_rappels.load());
    }).detach();
    return rate;
}

void audioOutStop() {
    ++g_generation;
    if (g_unit) {
        AudioOutputUnitStop(g_unit);
        AudioUnitUninitialize(g_unit);
        AudioComponentInstanceDispose(g_unit);
        g_unit = nullptr;
    }
}

} // namespace wfs
#endif
