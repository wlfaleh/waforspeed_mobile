// WaForSpeed — journal (logcat sur Android, stderr ailleurs).
#pragma once
#include <cstdio>
#include <cstdlib>

// WFS-126 (RG-02) : chaque ligne est aussi copiée dans l'anneau des 200 dernières lignes du rapport de plantage
// (src/platform/plantage.cpp) ; pointeur nul tant que le gestionnaire n'est pas installé (tests, outils : rien ne change)
inline void (*wfsAnneau)(char niveau, const char* fmt, ...) = nullptr;
#define WFS_ANNEAU(n, ...) do { if (wfsAnneau) wfsAnneau(n, __VA_ARGS__); } while (0)
#if defined(__ANDROID__)
#include <android/log.h>
#define WFS_LOG(...)  do { WFS_ANNEAU('I', __VA_ARGS__); __android_log_print(ANDROID_LOG_INFO, "WaForSpeed", __VA_ARGS__); } while (0)
#define WFS_WARN(...) do { WFS_ANNEAU('W', __VA_ARGS__); __android_log_print(ANDROID_LOG_WARN, "WaForSpeed", __VA_ARGS__); } while (0)
#define WFS_ERR(...)  do { WFS_ANNEAU('E', __VA_ARGS__); __android_log_print(ANDROID_LOG_ERROR, "WaForSpeed", __VA_ARGS__); } while (0)
#else
#define WFS_LOG(...)  do { WFS_ANNEAU('I', __VA_ARGS__); std::fprintf(stderr, "[info] " __VA_ARGS__); std::fputc('\n', stderr); } while (0)
#define WFS_WARN(...) do { WFS_ANNEAU('W', __VA_ARGS__); std::fprintf(stderr, "[attention] " __VA_ARGS__); std::fputc('\n', stderr); } while (0)
#define WFS_ERR(...)  do { WFS_ANNEAU('E', __VA_ARGS__); std::fprintf(stderr, "[ERREUR] " __VA_ARGS__); std::fputc('\n', stderr); } while (0)
#endif

#define WFS_CHECK(cond, ...) do { if (!(cond)) { WFS_ERR(__VA_ARGS__); std::abort(); } } while (0)
