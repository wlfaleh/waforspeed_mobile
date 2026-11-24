// WaForSpeed — version PC Windows (MinGW) : petites fonctions POSIX absentes de Windows, incluses de force
// (-include) dans chaque fichier de la version PC. Aucun effet sur Android, iOS ni sur le banc Linux.
#pragma once
#if defined(_WIN32)
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>
inline int setenv(const char* nom, const char* valeur, int remplacer) {
    if (!remplacer && std::getenv(nom)) return 0;
    return _putenv_s(nom, valeur ? valeur : "");
}
inline int unsetenv(const char* nom) { return _putenv_s(nom, ""); }
#endif
