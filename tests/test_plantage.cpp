// Rapport de plantage (WFS-126, RG-06) : anneau des 200 dernières lignes (débordement, lignes longues coupées à 256
// caractères sans couper un caractère UTF-8), mise en forme du rapport dans un tampon fixe sans allocation, champs
// attendus ; puis un vrai plantage dans un processus fils (RG-01 : fichier écrit, ≥ 3 adresses, signal rendu) et le
// renommage au lancement suivant (RG-04).
// usage : test_plantage
#include <sys/wait.h>
#include <unistd.h>
#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include "../src/core/log.h"
#include "../src/platform/plantage.h"
using namespace wfs;
using namespace wfs::plantage;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

// compteur d'allocations (RG-06 : la mise en forme n'en fait aucune)
static std::atomic<long> g_allocs{0};
void* operator new(size_t n) { g_allocs++; if (void* p = std::malloc(n ? n : 1)) return p; throw std::bad_alloc(); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, size_t) noexcept { std::free(p); }

static std::string lire(const std::string& chemin) {
    std::string s;
    if (FILE* f = std::fopen(chemin.c_str(), "rb")) { char b[4096]; size_t n; while ((n = std::fread(b, 1, sizeof b, f)) > 0) s.append(b, n); std::fclose(f); }
    return s;
}
static int compter(const std::string& s, const char* motif) {
    int n = 0; for (size_t p = s.find(motif); p != std::string::npos; p = s.find(motif, p + 1)) n++; return n;
}

int main() {
    // ---------- anneau : vide, puis débordement ----------
    VERIFIE(wfsAnneau == nullptr, "avant installation, log.h ne copie rien");
    WFS_LOG("ligne avant branchement");
    VERIFIE(anneauNombre() == 0, "rien de copié sans branchement (%d)", anneauNombre());
    brancherAnneau();
    VERIFIE(wfsAnneau == &anneauAjouter, "log.h branché sur l'anneau");
    for (int i = 0; i < 250; i++) WFS_LOG("ligne %d", i);
    VERIFIE(anneauNombre() == LIGNES, "200 lignes gardées (%d)", anneauNombre());
    VERIFIE(!std::strcmp(anneauLigne(0), "ligne 50"), "la plus ancienne gardée est la 50 (« %s »)", anneauLigne(0));
    VERIFIE(!std::strcmp(anneauLigne(LIGNES - 1), "ligne 249"), "la dernière est la 249 (« %s »)", anneauLigne(LIGNES - 1));
    VERIFIE(anneauNiveau(0) == 'I', "niveau info");
    WFS_ERR("erreur %s", "grave"); WFS_WARN("attention");
    VERIFIE(anneauNiveau(LIGNES - 2) == 'E' && anneauNiveau(LIGNES - 1) == 'W', "niveaux erreur et attention");
    std::printf("anneau : 200 dernières lignes, débordement, niveaux : OK\n");

    // ---------- lignes longues : coupées à 256 (255 octets + zéro), jamais au milieu d'un caractère ----------
    anneauVider();
    std::string longue(400, 'a');
    WFS_LOG("%s", longue.c_str());
    VERIFIE(std::strlen(anneauLigne(0)) == LONGUEUR - 1, "ligne longue coupée à 255 octets (%zu)", std::strlen(anneauLigne(0)));
    std::string accents;
    for (int i = 0; i < 200; i++) accents += "é";   // 2 octets chacun : la coupure à 255 tombe au milieu d'un « é »
    WFS_LOG("%s", accents.c_str());
    const size_t la = std::strlen(anneauLigne(1));
    VERIFIE(la == 254, "UTF-8 : coupure reculée au caractère entier (%zu octets)", la);
    WFS_LOG("deux\nlignes");
    VERIFIE(!std::strcmp(anneauLigne(2), "deux lignes"), "retour à la ligne remplacé (« %s »)", anneauLigne(2));
    std::printf("anneau : lignes longues coupées à 256 caractères, UTF-8 entier, une ligne par entrée : OK\n");

    // ---------- mise en forme sans allocation, dans un tampon fixe ----------
    anneauVider();
    for (int i = 0; i < 230; i++) WFS_LOG("écran Course, mode Checkpoint, terrain ville, image %d", i);
    Infos in; std::snprintf(in.version, sizeof in.version, "0.1.321 (321)"); std::snprintf(in.appareil, sizeof in.appareil, "Samsung SM-A546B, Android 14 (API 34)");
    Etat e; e.signal = SIGSEGV; e.code = 1; e.adresse = 0x10; e.nPile = 3;
    formaterAdresse(e.pile[0], sizeof e.pile[0], "/data/app/x/lib/arm64/libwaforspeed.so", 0x1a2b3c);
    formaterAdresse(e.pile[1], sizeof e.pile[1], "libwaforspeed.so", 0x42);
    formaterAdresse(e.pile[2], sizeof e.pile[2], "/system/lib64/libc.so", 0xfff0);
    static char tampon[RAPPORT_MAX];
    const long avant = g_allocs.load();
    const size_t n = formaterRapport(tampon, sizeof tampon, in, e);
    VERIFIE(g_allocs.load() == avant, "aucune allocation pendant la mise en forme (%ld)", g_allocs.load() - avant);
    const std::string r(tampon, n);
    VERIFIE(n == std::strlen(tampon) && n < sizeof tampon, "longueur rendue = texte écrit");
    VERIFIE(r.find("version : 0.1.321 (321)") != std::string::npos, "version");
    VERIFIE(r.find("appareil : Samsung SM-A546B, Android 14 (API 34)") != std::string::npos, "appareil");
    VERIFIE(r.find("signal : 11 SIGSEGV") != std::string::npos, "signal");
    VERIFIE(r.find("adresse fautive 0x10") != std::string::npos, "adresse fautive");
    VERIFIE(r.find("#00 libwaforspeed.so+0x1a2b3c") != std::string::npos, "adresse en module+0x… sans dossier");
    VERIFIE(r.find("#02 libc.so+0xfff0") != std::string::npos, "3e adresse");
    VERIFIE(r.find("--- 200 dernières lignes du journal ---") != std::string::npos, "titre du journal");
    VERIFIE(compter(r, "[I] écran Course") == 200, "200 lignes du journal (%d)", compter(r, "[I] écran Course"));
    VERIFIE(r.find("image 30\n") != std::string::npos && r.find("image 29\n") == std::string::npos, "les 200 dernières (30 à 229)");
    // tampon trop petit : coupé proprement, jamais de débordement
    char petit[64]; std::memset(petit, 'x', sizeof petit);
    const size_t np = formaterRapport(petit, 40, in, e);
    VERIFIE(np == 39 && petit[39] == 0 && petit[40] == 'x', "tampon trop petit : coupé, zéro final, rien au-delà");
    std::printf("rapport : tampon fixe, sans allocation, champs attendus : OK\n");

    // ---------- vrai plantage dans un processus fils (RG-01) puis lancement suivant (RG-04) ----------
    char modele[] = "/tmp/wfs_plantage_XXXXXX";
    const char* dossier = mkdtemp(modele);
    VERIFIE(dossier != nullptr, "dossier temporaire");
    if (dossier) {
        const pid_t fils = fork();
        if (fils == 0) {
            installer(dossier, "essai (0)", "test_plantage");
            for (int i = 0; i < 210; i++) WFS_LOG("fils : ligne %d", i);
            armerDeclencheur(true);
            minuterieDeclencheur(1.0 / 60, true);
            for (int i = 0; i < 400; i++) minuterieDeclencheur(1.0 / 60, true);   // 3 s au menu → plantage volontaire
            std::_Exit(0);   // ne doit pas arriver
        }
        int st = 0; waitpid(fils, &st, 0);
        VERIFIE(WIFSIGNALED(st) && WTERMSIG(st) == SIGSEGV, "le fils meurt de SIGSEGV, rendu au gestionnaire précédent (état %d)", st);
        const std::string rp = lire(std::string(dossier) + "/plantage.txt");
        VERIFIE(rp.find("signal : 11 SIGSEGV") != std::string::npos, "plantage.txt : signal");
        VERIFIE(rp.find("version : essai (0)") != std::string::npos, "plantage.txt : version");
        VERIFIE(compter(rp, "\n  #") >= 3, "plantage.txt : ≥ 3 adresses de pile (%d)", compter(rp, "\n  #"));
        VERIFIE(rp.find("--- 200 dernières lignes du journal ---") != std::string::npos, "plantage.txt : 200 lignes");
        VERIFIE(rp.find("PLANTAGE volontaire") != std::string::npos, "plantage.txt : dernière ligne du journal");
        VERIFIE(rp.find("--- fin du rapport ---") != std::string::npos, "plantage.txt : complet");
        VERIFIE(rapportPrecedent(dossier), "lancement suivant : rapport précédent trouvé");
        VERIFIE(lire(std::string(dossier) + "/plantage.txt").empty(), "plantage.txt renommé");
        VERIFIE(lire(std::string(dossier) + "/plantage-precedent.txt") == rp, "plantage-precedent.txt = le rapport");
        VERIFIE(!rapportPrecedent(dossier), "lancement d'après : plus rien");
        if (std::getenv("WFS_TEST_MONTRER")) std::printf("%s", rp.c_str());
        std::remove((std::string(dossier) + "/plantage-precedent.txt").c_str());
        rmdir(dossier);
        std::printf("plantage réel (processus fils) : fichier écrit, %d adresses, renommé au lancement suivant : OK\n", compter(rp, "\n  #"));
    }

    if (echecs) { std::printf("test_plantage : %d échec(s)\n", echecs); return 1; }
    std::printf("test_plantage : tout est conforme\n");
    return 0;
}
