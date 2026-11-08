// WFS-301 : paquets de cartes « .wfcarte » (src/core/carte_cuite.*). Écriture puis lecture ; deux écritures identiques octet par octet ;
// verifier_carte bon / absent / magie fausse / format futur / autre carte / autre format de texture / autre empreinte / tronqué / CRC faux /
// « .part » ; bloc inconnu ignoré ; archive de fichiers ; 2e source de lecture (lireDansPaquet) ; CRC-32 de référence ; vérification
// d'un paquet de 24 Mo en moins de 200 ms.
// usage : test_carte_cuite [dossier temporaire]
#include <chrono>
#include <cstdio>
#include <fstream>
#include <string>
#include "../src/core/carte_cuite.h"
using namespace wfs;
using namespace wfs::carte;

static int g_echecs = 0;
#define VERIFIER(c) do { if (!(c)) { std::printf("ÉCHEC ligne %d : %s\n", __LINE__, #c); g_echecs++; } } while (0)

static void ecrireFichier(const std::string& chemin, const std::vector<uint8_t>& o) {
    std::ofstream f(chemin, std::ios::binary); f.write((const char*)o.data(), (std::streamsize)o.size());
}
static Paquet paquetEssai(size_t tailleTexture = 3000) {
    Paquet p; p.carte = "ville"; p.versionCarte = 2; p.format = FormatTexture::ASTC; p.empreinte = empreinte("ville", 1234, 2, {});
    std::vector<Fichier> tex = {{"textures/route/b.astc.ktx2", std::vector<uint8_t>(tailleTexture, 7)}, {"textures/route/a.astc.ktx2", {1, 2, 3}}};
    std::vector<Fichier> mai = {{"world/animaux/chien.wfa", {9, 9, 9, 9}}};
    p.blocs = {{"decor_regroupe", {}}, {"maillages", archive(mai)}, {"textures", archive(tex)}, {"trace", {}}, {"chocs", {}}};
    return p;
}

int main(int argc, char** argv) {
    const std::string dir = argc > 1 ? std::string(argv[1]) + "/" : "";
    const Attendu bon{"ville", FormatTexture::ASTC, empreinte("ville", 1234, 2, {})};

    // CRC-32 de référence (« 123456789 » → CBF43926) ; noms de fichiers
    { const char* s = "123456789"; VERIFIER(crc32((const uint8_t*)s, 9) == 0xCBF43926u); }
    VERIFIER(nomFichier("ville", FormatTexture::ASTC) == "ville_astc.wfcarte");
    VERIFIER(nomFichier("ville", FormatTexture::ETC2) == "ville_etc2.wfcarte");
    VERIFIER(nomFichier("ville", FormatTexture::RGBA8) == "ville_rgba8.wfcarte");

    // RG-02 : l'empreinte dépend de chaque entrée
    {
        const uint64_t e = empreinte("ville", 1234, 2, {});
        VERIFIER(e == empreinte("ville", 1234, 2, {}));
        VERIFIER(e != empreinte("plage", 1234, 2, {}));
        VERIFIER(e != empreinte("ville", 1235, 2, {}));
        VERIFIER(e != empreinte("ville", 1234, 1, {}));
        VERIFIER(e != empreinte("ville", 1234, 2, {"0.5"}));
    }

    // RG-01 / RG-04 : écriture déterministe, relecture fidèle ; archives triées
    const Paquet p = paquetEssai();
    const std::vector<uint8_t> o = ecrire(p);
    VERIFIER(o == ecrire(paquetEssai()));
    VERIFIER(o.size() > 12 && o[0] == 'W' && o[1] == 'F' && o[2] == 'S' && o[3] == 'C' && o[4] == VERSION_FORMAT);
    {
        Paquet l;
        VERIFIER(lire(o, bon, &l) == Verdict::Bon);
        VERIFIER(l.carte == "ville" && l.versionCarte == 2 && l.format == FormatTexture::ASTC && l.blocs.size() == 5);
        VERIFIER(l.bloc("trace") && l.bloc("trace")->donnees.empty() && l.bloc("chocs") && l.bloc("decor_regroupe"));
        std::vector<Fichier> tex;
        VERIFIER(l.bloc("textures") && lireArchive(l.bloc("textures")->donnees, tex));
        VERIFIER(tex.size() == 2 && tex[0].chemin == "textures/route/a.astc.ktx2" && tex[1].octets.size() == 3000);
    }

    // RG-05 : verdicts
    {
        std::vector<uint8_t> x = o; x[0] = 'X';
        VERIFIER(lire(x, bon, nullptr) == Verdict::MagieFausse);
        x = o; x[4] = (uint8_t)(VERSION_FORMAT + 1);
        VERIFIER(lire(x, bon, nullptr) == Verdict::VersionFuture);   // version lue avant tout le reste (même avec un CRC faux)
        x = o; x[o.size() - 10] ^= 0x01;
        VERIFIER(lire(x, bon, nullptr) == Verdict::CrcFaux);
        x = o; x[20] ^= 0x01;   // dans l'en-tête
        VERIFIER(lire(x, bon, nullptr) == Verdict::CrcFaux);
        x.assign(o.begin(), o.begin() + (long)(o.size() / 2));
        VERIFIER(lire(x, bon, nullptr) == Verdict::TailleFausse);
        x.assign(o.begin(), o.begin() + 9);
        VERIFIER(lire(x, bon, nullptr) == Verdict::Illisible);
        VERIFIER(lire({}, bon, nullptr) == Verdict::Illisible);
        Attendu a = bon; a.carte = "plage";
        VERIFIER(lire(o, a, nullptr) == Verdict::AutreCarte);
        a = bon; a.format = FormatTexture::RGBA8;
        VERIFIER(lire(o, a, nullptr) == Verdict::MauvaisFormat);
        a = bon; a.empreinte ^= 1;
        VERIFIER(lire(o, a, nullptr) == Verdict::EmpreinteFausse);
    }

    // bloc inconnu : lu sans erreur, ignoré par la 2e source
    {
        Paquet q = paquetEssai(); q.blocs.push_back({"bloc_du_futur", {1, 2, 3, 4}});
        Paquet l;
        VERIFIER(lire(ecrire(q), bon, &l) == Verdict::Bon && l.bloc("bloc_du_futur"));
    }

    // fichiers : absent, fini, « .part » jamais lu
    {
        const std::string chemin = dir + nomFichier("ville", FormatTexture::ASTC);
        std::remove(chemin.c_str());
        VERIFIER(verifier_carte(chemin, bon) == Verdict::Absent);
        ecrireFichier(chemin, o);
        Paquet l;
        VERIFIER(verifier_carte(chemin, bon, &l) == Verdict::Bon && l.blocs.size() == 5);
        ecrireFichier(chemin + ".part", o);
        VERIFIER(verifier_carte(chemin + ".part", bon) == Verdict::NonFini);
        std::remove(chemin.c_str()); std::remove((chemin + ".part").c_str());
    }

    // archive mal formée
    {
        std::vector<Fichier> f;
        std::vector<uint8_t> a = archive({{"x", {1, 2}}});
        VERIFIER(lireArchive(a, f) && f.size() == 1);
        a.pop_back();
        VERIFIER(!lireArchive(a, f) && f.empty());
        VERIFIER(!lireArchive({1, 2, 3}, f));
    }

    // RG-03 : 2e source de lecture
    {
        std::vector<uint8_t> out;
        desactiver();
        VERIFIER(!actif() && !lireDansPaquet("textures/route/a.astc.ktx2", out));
        Paquet l; lire(o, bon, &l);
        activer(std::move(l));
        VERIFIER(actif() && carteActive() == "ville");
        VERIFIER(lireDansPaquet("textures/route/a.astc.ktx2", out) && out == std::vector<uint8_t>({1, 2, 3}));
        VERIFIER(lireDansPaquet("world/animaux/chien.wfa", out) && out.size() == 4);
        VERIFIER(!lireDansPaquet("fonts/Rajdhani-Bold.ttf", out));   // absent du paquet : l'APK répond
        VERIFIER(lusDansPaquet() == 2);
        desactiver();
        VERIFIER(!lireDansPaquet("textures/route/a.astc.ktx2", out));
    }

    // RG-05 : vérification légère < 200 ms (paquet de 24 Mo, fichier sur disque)
    {
        const std::string chemin = dir + "test-carte-grande.wfcarte";
        ecrireFichier(chemin, ecrire(paquetEssai(24u << 20)));
        const auto t0 = std::chrono::steady_clock::now();
        const Verdict v = verifier_carte(chemin, bon);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        std::printf("vérification d'un paquet de 24 Mo : %.0f ms\n", ms);
        VERIFIER(v == Verdict::Bon);
        VERIFIER(ms < 200);
        std::remove(chemin.c_str());
    }

    if (g_echecs) { std::printf("test_carte_cuite : %d échec(s)\n", g_echecs); return 1; }
    std::printf("test_carte_cuite : OK\n");
    return 0;
}
