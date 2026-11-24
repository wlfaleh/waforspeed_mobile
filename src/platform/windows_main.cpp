// WaForSpeed — version PC Windows (WFS-602, épic E24) : point d'entrée Win32.
// Fenêtre redimensionnable (F11 : plein écran), surface Vulkan VK_KHR_win32_surface, boucle de messages, temps haute
// résolution. Le jeu, le renderer et les écrans sont ceux d'Android, tels quels (RG-01) : ce fichier ne fait que les
// brancher, comme android_main.cpp. Commandes (RG-02) : le clavier et la manette XInput passent par le module manette
// (src/platform/manette.*, mêmes actions que la manette Android) ; la souris joue le doigt (clic = toucher).
//   clavier : flèches, ZQSD ou WASD = volant / gaz / frein (et déplacement dans les menus) ; Entrée = valider ;
//             Maj ou N = nitro ; C = changer de vue ; Échap = touche Retour du téléphone ; F11 = plein écran ; Alt+F4 = quitter
// Hors de ce ticket (RG-06) : son (WFS-603), réseau et duel (WFS-604), téléchargement des paquets (WFS-605).
// Pas d'achats ni de publicité sur PC (RG-05).
#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <xinput.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include "../core/configgame.h"
#include "../core/jobs.h"
#include "../core/log.h"
#include "../core/memoire.h"
#include "../core/platform.h"
#include "../game/game.h"
#include "../game/sauvegarde.h"
#include "../render/cache_pipelines.h"
#include "../render/qualite_auto.h"
#include "../render/renderer.h"
#include "../render/vk.h"
#include "../ui/ecrans/cgu.h"
#include "../ui/ecrans/chargement.h"
#include "../ui/langue.h"
#include "../world/cartes_v2/paquet_carte.h"
#include "chauffe.h"
#include "manette.h"
#include "manette_jeu.h"
#include "plantage.h"
#include "retour_systeme.h"

namespace wfs {
static std::string g_racine = "assets/";   // dossier assets/ à côté de WaForSpeed.exe (installé par l'installateur)
bool readAsset(const std::string& path, std::vector<uint8_t>& out) {
    if (carte::lireDansPaquet(path, out)) { memoire::compteurs().octetsLus += (int64_t)out.size(); return true; }
    std::ifstream f(g_racine + path, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f), {});
    memoire::compteurs().octetsLus += (int64_t)out.size();
    return true;
}
double nowSeconds() {
    using namespace std::chrono;
    static const auto t0 = steady_clock::now();
    return duration<double>(steady_clock::now() - t0).count();
}
} // namespace wfs

using namespace wfs;

namespace {
struct App {
    HWND fenetre = nullptr;
    VkCtx vk; Renderer renderer; Game game;
    bool pret = false, quitter = false, reduite = false, tailleChangee = false, pleinEcran = false;
    WINDOWPLACEMENT avantPleinEcran{sizeof(WINDOWPLACEMENT)};
    double dernier = 0, debut = 0;
    int compteur = 0;
};
App g_app;

void tailleFenetre(uint32_t& w, uint32_t& h) {
    RECT r{}; GetClientRect(g_app.fenetre, &r);
    w = (uint32_t)std::max<LONG>(1, r.right - r.left); h = (uint32_t)std::max<LONG>(1, r.bottom - r.top);
}

// ---------- clavier → boutons de la manette (mêmes actions que la manette Android) ----------
void touche(WPARAM vk, bool appui) {
    using namespace manette;
    switch (vk) {
        case VK_UP: case 'W': case 'Z': bouton(R2, appui); bouton(HAUT, appui); break;       // gaz / haut
        case VK_DOWN: case 'S': bouton(L2, appui); bouton(BAS, appui); break;                 // frein / bas
        case VK_LEFT: case 'A': case 'Q': bouton(GAUCHE, appui); break;                       // volant à gauche
        case VK_RIGHT: case 'D': bouton(DROITE, appui); break;                                // volant à droite
        case VK_RETURN: bouton(A, appui); break;                                              // valider
        case VK_SHIFT: case 'N': if (g_app.game.ecran() == Ecran::Course) bouton(A, appui); break;   // nitro
        case 'C': bouton(Y, appui); break;                                                    // changer de vue
        case VK_BACK: bouton(B, appui); break;                                                // retour
        case VK_ESCAPE:   // = touche Retour du téléphone (pause en course, retour dans les menus ; menu principal : rien)
            if (g_app.pret) retourSysteme(g_app.game, appui, 0);
            break;
        default: break;
    }
}

void basculerPleinEcran() {
    App& a = g_app;
    const DWORD style = (DWORD)GetWindowLongPtrW(a.fenetre, GWL_STYLE);
    if (!a.pleinEcran) {
        MONITORINFO mi{sizeof mi};
        if (GetWindowPlacement(a.fenetre, &a.avantPleinEcran) && GetMonitorInfoW(MonitorFromWindow(a.fenetre, MONITOR_DEFAULTTOPRIMARY), &mi)) {
            SetWindowLongPtrW(a.fenetre, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(a.fenetre, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left,
                         mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            a.pleinEcran = true;
        }
    } else {
        SetWindowLongPtrW(a.fenetre, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(a.fenetre, &a.avantPleinEcran);
        SetWindowPos(a.fenetre, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        a.pleinEcran = false;
    }
    a.tailleChangee = true;
}

// ---------- souris : le clic joue le doigt (identifiant 0) ----------
void souris(LPARAM lp, bool enfonce) {
    const float x = (float)GET_X_LPARAM(lp), y = (float)GET_Y_LPARAM(lp);
    static bool premier = true;
    if (enfonce) manette::toucherEcran();   // RG-03 de WFS-22 : retour au « tactile » (commandes à l'écran visibles)
    if (!g_app.pret) { if (ecrans::cgu().premierLancement()) ecrans::cgu().toucher(0, x, y, enfonce, premier && enfonce); else ecrans::chargement().toucher(0, x, y, enfonce); }
    else g_app.game.onTouch({0, x, y, enfonce});
    if (enfonce) premier = false;
}

LRESULT CALLBACK procedure(HWND h, UINT m, WPARAM wp, LPARAM lp) {
    static bool boutonTenu = false;
    switch (m) {
        case WM_CLOSE: g_app.quitter = true; return 0;
        case WM_DESTROY: PostQuitMessage(0); return 0;
        case WM_SIZE: g_app.reduite = (wp == SIZE_MINIMIZED); g_app.tailleChangee = true; return 0;
        case WM_KILLFOCUS:   // fenêtre quittée (Alt+Tab) : touches relâchées (pas de gaz bloqué) et pause en course
            for (WPARAM k : {(WPARAM)VK_UP, (WPARAM)VK_DOWN, (WPARAM)VK_LEFT, (WPARAM)VK_RIGHT, (WPARAM)VK_RETURN, (WPARAM)VK_SHIFT, (WPARAM)'C', (WPARAM)VK_BACK}) touche(k, false);
            if (g_app.pret && g_app.game.ecran() == Ecran::Course) g_app.game.allerA(Ecran::Pause);
            return 0;
        case WM_KEYDOWN: case WM_SYSKEYDOWN:
            if (wp == VK_F11 && !(lp & (1 << 30))) { basculerPleinEcran(); return 0; }
            if (wp == VK_F4 && (GetKeyState(VK_MENU) & 0x8000)) { g_app.quitter = true; return 0; }
            if (!(lp & (1 << 30))) touche(wp, true);   // répétition automatique ignorée
            return 0;
        case WM_KEYUP: case WM_SYSKEYUP: touche(wp, false); return 0;
        case WM_LBUTTONDOWN: SetCapture(h); boutonTenu = true; souris(lp, true); return 0;
        case WM_MOUSEMOVE: if (boutonTenu) souris(lp, true); return 0;
        case WM_LBUTTONUP: ReleaseCapture(); boutonTenu = false; souris(lp, false); return 0;
        default: return DefWindowProcW(h, m, wp, lp);
    }
}

// ---------- manette XInput (chargée à la demande : aucun pilote requis pour lancer le jeu) ----------
using FnXInput = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);
FnXInput g_xinput = nullptr;
void chargerXInput() {
    for (const wchar_t* nom : {L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll"})
        if (HMODULE d = LoadLibraryW(nom)) { g_xinput = (FnXInput)(void*)GetProcAddress(d, "XInputGetState"); if (g_xinput) return; }
}
void lireXInput() {
    static DWORD paquet = 0; static bool branchee = false; static double prochainEssai = 0;
    if (!g_xinput || (!branchee && nowSeconds() < prochainEssai)) return;   // manette absente : un essai par seconde
    XINPUT_STATE s{};
    if (g_xinput(0, &s) != ERROR_SUCCESS) { if (branchee) manette::deconnexion(); branchee = false; prochainEssai = nowSeconds() + 1; return; }
    branchee = true;
    if (s.dwPacketNumber == paquet) return;
    paquet = s.dwPacketNumber;
    using namespace manette;
    const WORD b = s.Gamepad.wButtons;
    auto btn = [&](Bouton x, WORD masque) { const bool v = (b & masque) != 0; if (v != etat().bouton[x]) bouton(x, v); };
    btn(A, XINPUT_GAMEPAD_A); btn(B, XINPUT_GAMEPAD_B); btn(X, XINPUT_GAMEPAD_X); btn(Y, XINPUT_GAMEPAD_Y);
    btn(START, XINPUT_GAMEPAD_START); btn(SELECT, XINPUT_GAMEPAD_BACK);
    btn(L1, XINPUT_GAMEPAD_LEFT_SHOULDER); btn(R1, XINPUT_GAMEPAD_RIGHT_SHOULDER);
    const float croixX = (b & XINPUT_GAMEPAD_DPAD_RIGHT ? 1.f : 0.f) - (b & XINPUT_GAMEPAD_DPAD_LEFT ? 1.f : 0.f);
    const float croixY = (b & XINPUT_GAMEPAD_DPAD_DOWN ? 1.f : 0.f) - (b & XINPUT_GAMEPAD_DPAD_UP ? 1.f : 0.f);
    // Y de XInput vers le haut positif ; le module manette suit Android (vers le bas positif)
    axes(s.Gamepad.sThumbLX / 32767.f, -s.Gamepad.sThumbLY / 32767.f, s.Gamepad.bLeftTrigger / 255.f, s.Gamepad.bRightTrigger / 255.f, croixX, croixY);
}

void chargerJeu(double tVk) {
    App& a = g_app;
    ecrans::chargement().debut(a.renderer, tVk);
    cartes_v2::preparerCarte(sauvegarde().map);   // paquet vérifié de la carte s'il est là, sinon la carte du code
    if (!a.game.init(a.renderer, sauvegarde().car.c_str(), sauvegarde().map.c_str(), ecrans::Chargement::progression)) { ecrans::chargement().erreur(true); return; }
    ecrans::chargement().fin();
    a.game.allerA(Ecran::Menu);
    Game::demandes().debutChargement = [] { ecrans::chargement().debut(g_app.renderer, nowSeconds()); cartes_v2::preparerCarte(sauvegarde().map); };
    Game::demandes().progression = ecrans::Chargement::progression;
    Game::demandes().finChargement = [](bool ok) { if (ok) ecrans::chargement().fin(); else ecrans::chargement().erreur(); };
    uint32_t w, h; tailleFenetre(w, h); a.renderer.setViewSize(w, h);
    a.pret = true; a.dernier = nowSeconds();
    WFS_LOG("PC Windows : moteur initialisé");
}

std::string dossierJoueur() {   // %APPDATA%\WaForSpeed : sauvegarde, cache des pipelines, paquets (gardé à la désinstallation)
    const char* ad = std::getenv("APPDATA");
    std::filesystem::path p = ad && *ad ? std::filesystem::path(ad) / "WaForSpeed" : std::filesystem::path("sauvegarde");
    std::error_code ec; std::filesystem::create_directories(p / "paquets", ec);
    return p.string();
}

std::string langueWindows() {
    const LANGID l = GetUserDefaultUILanguage();
    return PRIMARYLANGID(l) == LANG_FRENCH ? "fr" : "en";
}
} // namespace

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int montrer) {
    // dossier du programme : assets/ est à côté de WaForSpeed.exe, quel que soit le raccourci utilisé
    wchar_t exe[MAX_PATH]; const DWORD n = GetModuleFileNameW(nullptr, exe, MAX_PATH);
    if (n > 0 && n < MAX_PATH) SetCurrentDirectoryW(std::filesystem::path(exe).parent_path().c_str());
    SetProcessDPIAware();   // pixels réels de l'écran (pas d'image floue agrandie par Windows)
    const std::string joueur = dossierJoueur();
    plantage::installer(joueur.c_str(), "PC Windows", "PC Windows");
    jobs().start();
    ui::langue::definirLangueTelephone(langueWindows());
    sauvegarde().charger(joueur + "/sauvegarde.txt");
    cache_pipelines::dossier = joueur;
    qualite::dossierMesure = joueur;
    carte::dossierPaquets() = joueur + "/paquets";
    SetEnvironmentVariableA("WFS_MOMENT", sauvegarde().tod.c_str()); _putenv_s("WFS_MOMENT", sauvegarde().tod.c_str());
    chargerXInput();

    WNDCLASSEXW wc{sizeof wc};
    wc.style = CS_HREDRAW | CS_VREDRAW; wc.lpfnWndProc = procedure; wc.hInstance = inst;
    wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW); wc.lpszClassName = L"WaForSpeed";
    wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1)); wc.hIconSm = wc.hIcon;
    RegisterClassExW(&wc);
    // 1280 × 720 de zone de jeu (paysage, comme le téléphone), centrée
    RECT r{0, 0, 1280, 720}; AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    const int lw = r.right - r.left, lh = r.bottom - r.top;
    g_app.fenetre = CreateWindowExW(0, wc.lpszClassName, L"WaForSpeed", WS_OVERLAPPEDWINDOW,
                                    (GetSystemMetrics(SM_CXSCREEN) - lw) / 2, (GetSystemMetrics(SM_CYSCREEN) - lh) / 2, lw, lh,
                                    nullptr, nullptr, inst, nullptr);
    if (!g_app.fenetre) return 1;
    ShowWindow(g_app.fenetre, montrer); UpdateWindow(g_app.fenetre);

    App& a = g_app;
    if (!a.vk.init((ANativeWindow*)(void*)a.fenetre)) {
        MessageBoxW(a.fenetre, L"Vulkan est indisponible sur ce PC.\nMettez à jour le pilote de la carte graphique (NVIDIA, AMD ou Intel), puis relancez le jeu.",
                    L"WaForSpeed", MB_ICONERROR);
        return 2;
    }
    const double tVk = nowSeconds();
    a.renderer.init(a.vk);
    if (!ecrans::cgu().demarrer(a.renderer)) chargerJeu(tVk);   // CGU au premier lancement, puis chargement
    a.debut = nowSeconds();

    while (!a.quitter) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) a.quitter = true;
            TranslateMessage(&msg); DispatchMessageW(&msg);
        }
        if (a.quitter) break;
        if (a.reduite) { WaitMessage(); continue; }   // fenêtre réduite : rien à dessiner, aucune charge
        lireXInput();
        uint32_t w, h; tailleFenetre(w, h);
        if (!a.pret) {
            switch (ecrans::cgu().image(a.vk, w, h, nowSeconds())) {
                case ecrans::Cgu::Etat::Quitter: a.quitter = true; break;
                case ecrans::Cgu::Etat::Accepte: chargerJeu(nowSeconds()); break;
                default: break;
            }
            if (ecrans::chargement().reessaiDemande()) a.quitter = true;
            if (!a.pret) { if (ecrans::chargement().enErreur()) ecrans::chargement().dessiner(); std::this_thread::sleep_for(std::chrono::milliseconds(16)); continue; }
        }
        if (ecrans::chargement().reessaiDemande()) { Game::demandes().carte = sauvegarde().map; Game::demandes().nuit = sauvegarde().tod == "nuit"; }
        if (a.tailleChangee || ++a.compteur >= 15) {   // taille de la fenêtre (redimensionnement, plein écran)
            a.compteur = 0; a.tailleChangee = false;
            if (a.vk.surfaceChanged()) { a.vk.waitIdle(); if (a.vk.createSwapchain()) a.renderer.resize(); }
        }
        a.renderer.setViewSize(w, h);
        const double t = nowSeconds(), dt = t - a.dernier; a.dernier = t;
        if (ecrans::chargement().enErreur()) ecrans::chargement().dessiner();
        else { manette::appliquer(a.game); a.game.update(dt); retourSystemeApresImage(a.game); a.game.render(t - a.debut); }
    }
    if (a.pret) a.renderer.enregistrerCachePipelines();
    a.vk.waitIdle();
    if (a.pret) a.game.shutdown();
    a.renderer.shutdown();
    a.vk.shutdown();
    jobs().stop();
    sauvegarde().enregistrer();
    DestroyWindow(a.fenetre);
    return 0;
}
#endif
