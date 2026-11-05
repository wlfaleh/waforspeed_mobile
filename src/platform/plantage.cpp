// WaForSpeed — rapport de plantage (WFS-126) : voir plantage.h.
// Le gestionnaire de signal n'utilise que des tampons statiques et des appels sûrs en signal (open, write, close,
// sigaction, raise) ; dladdr et _Unwind_Backtrace sont les deux exceptions prévues par RG-01 (préchauffés à
// l'installation pour que leur première utilisation, qui peut allouer, n'ait pas lieu pendant le plantage).
#include "plantage.h"
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include "../core/log.h"

#if defined(__linux__) || defined(__ANDROID__)
#include <dlfcn.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <ucontext.h>
#include <unistd.h>
#include <unwind.h>
#define WFS_PLANTAGE_POSIX 1
#endif

namespace wfs::plantage {

// ---------- anneau du journal (RG-02) ----------
namespace {
char g_lignes[LIGNES][LONGUEUR];
char g_niveaux[LIGNES];
std::atomic<uint64_t> g_suivant{0};

// longueur gardée ≤ n telle que le dernier caractère UTF-8 soit entier (texte partagé tel quel, jamais un caractère coupé)
size_t coupureUtf8(const char* s, size_t n) {
    if (n == 0) return 0;
    size_t i = n - 1;
    while (i > 0 && (static_cast<unsigned char>(s[i]) & 0xC0) == 0x80) i--;   // début du dernier caractère
    const unsigned char c = static_cast<unsigned char>(s[i]);
    const size_t len = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 1;
    return i + len > n ? i : n;
}
} // namespace

void anneauAjouter(char niveau, const char* fmt, ...) {
    const uint64_t i = g_suivant.fetch_add(1, std::memory_order_relaxed) % LIGNES;
    char* l = g_lignes[i];
    va_list a;
    va_start(a, fmt);
    const int n = std::vsnprintf(l, LONGUEUR, fmt, a);
    va_end(a);
    if (n < 0) l[0] = 0;
    else if (n >= LONGUEUR) l[coupureUtf8(l, LONGUEUR - 1)] = 0;   // ligne longue : coupée à 255 octets au plus
    for (char* c = l; *c; c++) if (*c == '\n' || *c == '\r') *c = ' ';   // une ligne du journal = une ligne du rapport
    g_niveaux[i] = niveau;
}
void anneauVider() { g_suivant = 0; std::memset(g_lignes, 0, sizeof g_lignes); }
int anneauNombre() { const uint64_t n = g_suivant.load(); return n < (uint64_t)LIGNES ? (int)n : LIGNES; }
const char* anneauLigne(int i) {
    const uint64_t n = g_suivant.load(), debut = n - (uint64_t)anneauNombre();
    return g_lignes[(debut + (uint64_t)i) % LIGNES];
}
char anneauNiveau(int i) {
    const uint64_t n = g_suivant.load(), debut = n - (uint64_t)anneauNombre();
    return g_niveaux[(debut + (uint64_t)i) % LIGNES];
}
void brancherAnneau() { wfsAnneau = &anneauAjouter; }

// ---------- mise en forme sans allocation (RG-06) ----------
namespace {
struct Ecrivain {   // ajoute au tampon sans jamais déborder
    char* t; size_t cap, n = 0;
    void txt(const char* s) { while (*s && n + 1 < cap) t[n++] = *s++; t[n] = 0; }
    void car(char c) { if (n + 1 < cap) { t[n++] = c; t[n] = 0; } }
    void dec(long long v) {
        char b[24]; int k = 0; bool neg = v < 0; unsigned long long u = neg ? 0ull - (unsigned long long)v : (unsigned long long)v;
        do { b[k++] = char('0' + u % 10); u /= 10; } while (u && k < 23);
        if (neg) car('-');
        while (k) car(b[--k]);
    }
    void hex(uintptr_t v) {
        char b[20]; int k = 0;
        do { b[k++] = "0123456789abcdef"[v & 15]; v >>= 4; } while (v && k < 19);
        txt("0x");
        while (k) car(b[--k]);
    }
};
} // namespace

const char* nomSignal(int s) {
    switch (s) {
#if defined(WFS_PLANTAGE_POSIX)
        case SIGSEGV: return "SIGSEGV (accès mémoire interdit)";
        case SIGABRT: return "SIGABRT (abandon)";
        case SIGBUS:  return "SIGBUS (erreur de bus)";
        case SIGFPE:  return "SIGFPE (erreur de calcul)";
        case SIGILL:  return "SIGILL (instruction illégale)";
#endif
        default: return "signal inconnu";
    }
}

size_t formaterAdresse(char* t, size_t cap, const char* module, uintptr_t decalage) {
    Ecrivain w{t, cap};
    if (cap) t[0] = 0;
    const char* nom = module ? module : "?";
    for (const char* c = nom; *c; c++) if (*c == '/') nom = c + 1;   // nom du module sans son dossier
    w.txt(*nom ? nom : "?"); w.car('+'); w.hex(decalage);
    return w.n;
}

size_t formaterRapport(char* tampon, size_t cap, const Infos& infos, const Etat& e) {
    if (!cap) return 0;
    tampon[0] = 0;
    Ecrivain w{tampon, cap};
    w.txt("WaForSpeed — rapport de plantage\n");
    w.txt("version : "); w.txt(infos.version); w.car('\n');
    w.txt("appareil : "); w.txt(infos.appareil); w.car('\n');
    w.txt("signal : "); w.dec(e.signal); w.txt(" "); w.txt(nomSignal(e.signal));
    w.txt(", code "); w.dec(e.code); w.txt(", adresse fautive "); w.hex(e.adresse); w.car('\n');
    w.txt("pile ("); w.dec(e.nPile); w.txt(" adresses) :\n");
    for (int i = 0; i < e.nPile && i < PILE_MAX; i++) {
        w.txt("  #"); if (i < 10) w.car('0'); w.dec(i); w.car(' '); w.txt(e.pile[i]); w.car('\n');
    }
    const int n = anneauNombre();
    w.txt("--- "); w.dec(n); w.txt(" dernières lignes du journal ---\n");
    for (int i = 0; i < n; i++) {
        w.car('['); w.car(anneauNiveau(i)); w.txt("] "); w.txt(anneauLigne(i)); w.car('\n');
    }
    w.txt("--- fin du rapport ---\n");
    return w.n;
}

// ---------- gestionnaire de signaux (RG-01) ----------
namespace {
Infos g_infos;
char g_chemin[512] = "";
bool g_installe = false;
std::atomic<bool> g_armer{false};
double g_auMenu = -1;
#if defined(WFS_PLANTAGE_POSIX)
constexpr int SIGNAUX[5] = {SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL};
struct sigaction g_precedents[5];
char g_rapport[RAPPORT_MAX];   // préparé d'avance : aucun malloc dans le gestionnaire
Etat g_etat;
uintptr_t g_pcs[PILE_MAX];
std::atomic<int> g_dedans{0};
alignas(16) char g_pileSecours[64 * 1024];   // RG-01 : pile de secours sigaltstack 64 Ko

struct Collecte { uintptr_t* pcs; int n, max; };
_Unwind_Reason_Code rappelPile(_Unwind_Context* ctx, void* arg) {
    Collecte* c = static_cast<Collecte*>(arg);
    const uintptr_t pc = _Unwind_GetIP(ctx);
    if (pc && c->n < c->max) c->pcs[c->n++] = pc;
    return c->n < c->max ? _URC_NO_REASON : _URC_END_OF_STACK;
}
int collecterPile(uintptr_t* pcs, int max) { Collecte c{pcs, 0, max}; _Unwind_Backtrace(rappelPile, &c); return c.n; }

void ajouterAdresse(uintptr_t pc, const char* prefixe) {
    if (g_etat.nPile >= PILE_MAX || !pc) return;
    char* t = g_etat.pile[g_etat.nPile++];
    Dl_info d{};
    size_t k = 0;
    if (prefixe) { Ecrivain w{t, sizeof g_etat.pile[0]}; w.txt(prefixe); k = w.n; }
    if (dladdr(reinterpret_cast<void*>(pc), &d) && d.dli_fname) {
        k += formaterAdresse(t + k, sizeof g_etat.pile[0] - k, d.dli_fname, pc - reinterpret_cast<uintptr_t>(d.dli_fbase));
        if (d.dli_sname && k + 4 < sizeof g_etat.pile[0]) {   // nom du symbole s'il est exporté (aide sans symbolisation)
            Ecrivain w{t + k, sizeof g_etat.pile[0] - k}; w.txt(" ("); w.txt(d.dli_sname); w.car(')');
        }
    } else {
        formaterAdresse(t + k, sizeof g_etat.pile[0] - k, "?", pc);
    }
}

uintptr_t pcDuContexte(void* uc, uintptr_t* lr) {
    *lr = 0;
    if (!uc) return 0;
    const ucontext_t* u = static_cast<const ucontext_t*>(uc);
#if defined(__aarch64__)
    *lr = (uintptr_t)u->uc_mcontext.regs[30];
    return (uintptr_t)u->uc_mcontext.pc;
#elif defined(__x86_64__)
#if defined(REG_RIP)
    return (uintptr_t)u->uc_mcontext.gregs[REG_RIP];
#else
    return (uintptr_t)u->uc_mcontext.gregs[16];   // REG_RIP (émulateur x86_64 : en-tête de bionic sans les noms)
#endif
#else
    (void)u; return 0;
#endif
}

void ecrireTout(int fd, const char* t, size_t n) {
    while (n > 0) { const ssize_t k = write(fd, t, n); if (k <= 0) break; t += k; n -= (size_t)k; }
}

void gestionnaire(int sig, siginfo_t* info, void* uc) {
    int idx = 0;
    while (idx < 5 && SIGNAUX[idx] != sig) idx++;
    if (g_dedans.exchange(1) == 0) {   // premier plantage seulement (un plantage dans le gestionnaire ne boucle pas)
        g_etat.signal = sig;
        g_etat.code = info ? info->si_code : 0;
        g_etat.adresse = info ? reinterpret_cast<uintptr_t>(info->si_addr) : 0;
        g_etat.nPile = 0;
        uintptr_t lr = 0;
        const uintptr_t pc = pcDuContexte(uc, &lr);
        ajouterAdresse(pc, "pc ");   // instruction fautive (lue dans le contexte du signal)
        ajouterAdresse(lr, "lr ");   // appelant (arm64)
        const int n = collecterPile(g_pcs, PILE_MAX);
        for (int i = 0; i < n; i++) ajouterAdresse(g_pcs[i], nullptr);
        const size_t len = formaterRapport(g_rapport, sizeof g_rapport, g_infos, g_etat);
        const int fd = open(g_chemin, O_WRONLY | O_CREAT | O_TRUNC, 0600);
        if (fd >= 0) { ecrireTout(fd, g_rapport, len); close(fd); }
        static const char msg[] = "[ERREUR] PLANTAGE : rapport écrit dans plantage.txt\n";
        ecrireTout(2, msg, sizeof msg - 1);
    }
    // RG-01 : rend la main au gestionnaire précédent (tombstone et logcat d'Android, ou fin du processus sur PC)
    if (idx < 5) sigaction(sig, &g_precedents[idx], nullptr);
    else signal(sig, SIG_DFL);
    if (!info || info->si_code <= 0) raise(sig);   // signal envoyé (abort, kill) : de nouveau, au gestionnaire précédent
    // faute matérielle : au retour, l'instruction fautive est rejouée et le gestionnaire précédent la reçoit
}
#endif
} // namespace

void installer(const char* dossier, const char* version, const char* appareil) {
    std::snprintf(g_infos.version, sizeof g_infos.version, "%s", version ? version : "?");
    std::snprintf(g_infos.appareil, sizeof g_infos.appareil, "%s", appareil ? appareil : "?");
    std::snprintf(g_chemin, sizeof g_chemin, "%s/plantage.txt", dossier && *dossier ? dossier : ".");
    brancherAnneau();
#if defined(WFS_PLANTAGE_POSIX)
    stack_t pile{};
    pile.ss_sp = g_pileSecours; pile.ss_size = sizeof g_pileSecours; pile.ss_flags = 0;
    sigaltstack(&pile, nullptr);   // thread appelant (le thread du jeu) : un débordement de pile est aussi rapporté
    if (!g_installe) {
        uintptr_t chauffe[4];
        collecterPile(chauffe, 4);   // préchauffage du dérouleur (première utilisation hors du gestionnaire)
        Dl_info d{}; dladdr(reinterpret_cast<void*>(&installer), &d);
        for (int i = 0; i < 5; i++) {
            struct sigaction sa{};
            sa.sa_sigaction = gestionnaire;
            sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
            sigemptyset(&sa.sa_mask);
            sigaction(SIGNAUX[i], &sa, &g_precedents[i]);
        }
    }
    g_installe = true;
    WFS_LOG("PLANTAGE gestionnaire installé : %s, version %s, %s", g_chemin, g_infos.version, g_infos.appareil);
#else
    WFS_LOG("PLANTAGE gestionnaire non disponible sur cette plateforme (anneau du journal seul)");
#endif
}
bool installe() { return g_installe; }
const char* cheminRapport() { return g_chemin; }

bool rapportPrecedent(const char* dossier) {
    char src[512], dst[512];
    std::snprintf(src, sizeof src, "%s/plantage.txt", dossier && *dossier ? dossier : ".");
    std::snprintf(dst, sizeof dst, "%s/plantage-precedent.txt", dossier && *dossier ? dossier : ".");
    FILE* f = std::fopen(src, "rb");
    if (!f) return false;
    long taille = 0;
    if (std::fseek(f, 0, SEEK_END) == 0) taille = std::ftell(f);
    std::fclose(f);
    std::remove(dst);   // un seul rapport gardé
    const bool ok = std::rename(src, dst) == 0;
    WFS_LOG("PLANTAGE rapport précédent trouvé : %s (%ld octets) → %s%s", src, taille, dst, ok ? "" : " (renommage impossible)");
    return true;
}

void armerDeclencheur(bool oui) {
    g_armer = oui; g_auMenu = -1;
    if (oui) WFS_LOG("PLANTAGE déclencheur de test armé : plantage natif volontaire 3 s après le menu (RG-05)");
}
bool declencheurArme() { return g_armer; }
void minuterieDeclencheur(double dt, bool auMenu) {
    if (!g_armer) return;
    if (!auMenu) { g_auMenu = -1; return; }
    if (g_auMenu < 0) { g_auMenu = 0; WFS_LOG("PLANTAGE déclencheur : menu atteint, plantage dans 3 s"); return; }
    g_auMenu += dt;
    if (g_auMenu >= 3.0) plantageVolontaire();
}
void plantageVolontaire() {
    WFS_ERR("PLANTAGE volontaire (déclencheur de test RG-05) : écriture à l'adresse 0");
    volatile int* p = nullptr;
    asm volatile("" : "+r"(p));   // le compilateur ne sait plus que p est nul : vrai accès (SIGSEGV), pas une instruction d'arrêt
    *p = 0x57F5;
    __builtin_trap();
}

} // namespace wfs::plantage
