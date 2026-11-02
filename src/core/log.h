// WaForSpeed — journal (logcat sur Android, stderr ailleurs).
#pragma once
#include <cstdio>
#include <cstdlib>

#if defined(__ANDROID__)
#include <android/log.h>
#define WFS_LOG(...)  __android_log_print(ANDROID_LOG_INFO, "WaForSpeed", __VA_ARGS__)
#define WFS_WARN(...) __android_log_print(ANDROID_LOG_WARN, "WaForSpeed", __VA_ARGS__)
#define WFS_ERR(...)  __android_log_print(ANDROID_LOG_ERROR, "WaForSpeed", __VA_ARGS__)
#else
#define WFS_LOG(...)  do { std::fprintf(stderr, "[info] " __VA_ARGS__); std::fputc('\n', stderr); } while (0)
#define WFS_WARN(...) do { std::fprintf(stderr, "[attention] " __VA_ARGS__); std::fputc('\n', stderr); } while (0)
#define WFS_ERR(...)  do { std::fprintf(stderr, "[ERREUR] " __VA_ARGS__); std::fputc('\n', stderr); } while (0)
#endif

#define WFS_CHECK(cond, ...) do { if (!(cond)) { WFS_ERR(__VA_ARGS__); std::abort(); } } while (0)
