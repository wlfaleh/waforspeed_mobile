// WFS-343 : cartes 100 % dynamiques (src/world/cartes_v2/carte_donnees.*) — lecture des blocs trace / chocs / decor des deux cartes
// d'essai (paquets/sources), refus des blocs faux (format futur, instruction inconnue, hors du tour, obstacle sur la chaussée), registre
// (mapById, carte du code jamais remplacée), tracé construit depuis les données (Track::build), cuisson en paquet, découverte dans le
// dossier des paquets (paquet juste → enregistrée ; abîmé ou d'une autre génération → absente), rien de propre à une carte dans le code.
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include "../src/core/carte_cuite.h"
#include "../src/game/track.h"
#include "../src/world/cartes_v2/carte_donnees.h"
using namespace wfs;
using namespace wfs::cartes_v2;

static int g_echecs = 0;
#define VERIFIER(c) do { if (!(c)) { std::printf("ÉCHEC ligne %d : %s\n", __LINE__, #c); g_echecs++; } } while (0)

static std::string lire(const std::string& f) { std::ifstream in(f, std::ios::binary); return std::string(std::istreambuf_iterator<char>(in), {}); }
static bool lireT(const std::string& t, std::string& e) { DonneesCarte d; d.id = "x"; return lireTrace(t, d, e); }
static const std::string TRACE_MIN = "format 1\nnom Essai\nroute 50 200 50 0 0\n";   // 300 segments

int main() {
    const std::string src = std::string(WFS_SOURCE_DIR) + "/assets";
    std::string e;

    // ---- les deux cartes d'essai se lisent (RG-01, RG-03, RG-05)
    carte::Paquet pn, pa;
    VERIFIER(sourcesCarte(src, "noel-test", carte::FormatTexture::RGBA8, pn, e));
    if (!e.empty()) std::printf("  %s\n", e.c_str());
    VERIFIER(sourcesCarte(src, "automne-test", carte::FormatTexture::RGBA8, pa, e));
    if (!e.empty()) std::printf("  %s\n", e.c_str());
    if (!pn.bloc("trace") || !pa.bloc("trace")) { std::printf("test_cartes_v2_donnees : sources des cartes d'essai illisibles\n"); return 1; }
    DonneesCarte n; n.id = "noel-test";
    VERIFIER(lireTrace(std::string(pn.bloc("trace")->donnees.begin(), pn.bloc("trace")->donnees.end()), n, e));
    VERIFIER(lireChocs(std::string(pn.bloc("chocs")->donnees.begin(), pn.bloc("chocs")->donnees.end()), n, e));
    VERIFIER(lireDecor(std::string(pn.bloc("decor")->donnees.begin(), pn.bloc("decor")->donnees.end()), n, e));
    VERIFIER(n.nom == "Village de Noël (essai)" && n.cache && n.segments() == 1220 && n.routes.size() == 10 && n.sections.size() == 5);
    VERIFIER(n.quartiers.size() == 3 && n.quartiers[1] == "Rue des chalets");
    VERIFIER(n.checkpointM == 400 && n.traficN == 12 && !n.herbe && n.sol.size() == 4);
    VERIFIER(n.obstacles.size() == 24 + 15 + 1);    // repete 50 de 25 à 1200 (24), 80 de 40 à 1200 (15), fontaine
    size_t lum = 0; for (const Objet& o : n.objets) if (o.type == Objet::Lumiere) lum++;
    VERIFIER(lum > 1000);                           // guirlandes dépliées en lumières
    DonneesCarte a; a.id = "automne-test";
    VERIFIER(lireTrace(std::string(pa.bloc("trace")->donnees.begin(), pa.bloc("trace")->donnees.end()), a, e) && a.herbe && a.checkpointM == 700);
    VERIFIER(lireDecor(std::string(pa.bloc("decor")->donnees.begin(), pa.bloc("decor")->donnees.end()), a, e) && a.herbe && a.sol[1].couleur == 0xa86a2a);

    // ---- blocs faux : refusés avec une raison (jamais de plantage)
    VERIFIER(lireT(TRACE_MIN, e));
    VERIFIER(!lireT("format 2\nnom Essai\nroute 50 200 50 0 0\n", e) && e.find("plus récent") != std::string::npos);
    VERIFIER(!lireT("nom Essai\n", e));                                       // pas de « format »
    VERIFIER(!lireT("format 1\nroute 50 200 50 0 0\n", e) && e.find("nom") != std::string::npos);
    VERIFIER(!lireT("format 1\nnom Essai\nroute 10 10 10 0 0\n", e));          // tour trop court
    VERIFIER(!lireT("format 1\nnom Essai\nroute 50 200 50 12 0\n", e));        // courbure impossible
    VERIFIER(!lireT("format 1\nnom Essai\nroute 50 200 50 0 0\nvolcan 3\n", e) && e.find("volcan") != std::string::npos && e.find("ligne 4") != std::string::npos);
    VERIFIER(!lireT(TRACE_MIN + "section rondpoint 0 10\n", e));               // pas de rond-point (WFS-283)
    VERIFIER(!lireT(TRACE_MIN + "section ligne 250 400\n", e));                // section hors du tour
    VERIFIER(!lireT(TRACE_MIN + "section ligne 0 100\nsection virage 50 150 1 60 20\n", e));   // chevauchement
    VERIFIER(!lireT("format 1\nnom Essai\nroute 50 x 50 0 0\n", e));
    { DonneesCarte d; d.id = "x"; lireTrace(TRACE_MIN, d, e);
      VERIFIER(lireChocs("format 1\nboite 10 0 -9 0.3 1 0 0x808080 1 mur\n", d, e));
      VERIFIER(!lireChocs("format 1\nboite 10 0 -3 0.3 1 0 0x808080 1 mur\n", d, e) && e.find("chaussée") != std::string::npos);
      VERIFIER(!lireChocs("format 1\ncylindre 900 0 9 0.3 0x808080 1 poteau\n", d, e));   // hors du tour (300 segments)
      VERIFIER(!lireChocs("format 1\ncylindre 10 0 9 0.3 0x808080 1 dragon\n", d, e));   // genre inconnu
      VERIFIER(!lireDecor("format 1\nmaison 10 -5 0 8 8 6 0xffffff 0x000000\n", d, e));   // sur la chaussée
      VERIFIER(!lireDecor("format 1\nbande 20 10 0 0 0xffffff\n", d, e));
      VERIFIER(!lireDecor("format 1\nrepete 0 0 10 sapin 0 -20 8\n", d, e));     // pas nul
      VERIFIER(lireDecor("format 1\nrepete 100 0 300 sapin 0 -20 8\n", d, e) && d.objets.size() == 3 && d.objets[2].seg == 200); }

    // ---- registre : mapById, tracé construit depuis les données, carte du code jamais remplacée
    { DonneesCarte v; v.id = "ville"; lireTrace(TRACE_MIN, v, e); VERIFIER(!enregistrer(v, e)); }
    { DonneesCarte v; v.id = "Mauvais Nom"; lireTrace(TRACE_MIN, v, e); VERIFIER(!enregistrer(v, e)); }
    VERIFIER(enregistrer(n, e));
    VERIFIER(std::string(mapById("noel-test").id) == "noel-test" && std::string(mapById("noel-test").name) == "Village de Noël (essai)");
    VERIFIER(std::string(mapById("ville").id) == "ville" && std::string(mapById("inconnue").id) == "ville");   // inchangé
    VERIFIER(mapById("noel-test").trafficN == 12 && mapById("noel-test").edge == 1.6);
    { Track t; t.build(mapById("noel-test"), 1);
      VERIFIER(t.segmentCount() == 1220 && t.version() == 2 && t.sections().size() == 5 && t.sections()[2].type == trace::Type::Virage);
      VERIFIER(std::fabs(t.segAt(121 + 30 + 40).curve - 2) < 1e-9);   // tenue de la 2e route : courbe 2
      Track v; v.build(mapById("ville"), 1);
      VERIFIER(v.segmentCount() != 1220 && v.sections().empty()); }   // cartes du code inchangées
    VERIFIER(cartesDonnees(false).empty() && cartesDonnees(true).size() == 1);   // carte d'essai cachée aux joueurs

    // ---- cuisson puis découverte dans le dossier des paquets
    oublier();
    const std::string dir = (std::filesystem::temp_directory_path() / "test-cartes-donnees").string();
    std::filesystem::remove_all(dir); std::filesystem::create_directories(dir);
    carte::dossierPaquets() = dir;
    auto ecrireFichier = [&](const std::string& nom, const std::vector<uint8_t>& o) { std::ofstream(dir + "/" + nom, std::ios::binary).write((const char*)o.data(), (std::streamsize)o.size()); };
    ecrireFichier(carte::nomFichier("noel-test", carte::FormatTexture::RGBA8), carte::ecrire(pn));
    VERIFIER(carte::ecrire(pn) == carte::ecrire(pn));   // déterministe
    std::vector<uint8_t> abime = carte::ecrire(pa); abime[abime.size() - 3] ^= 0x5a;
    ecrireFichier(carte::nomFichier("automne-test", carte::FormatTexture::RGBA8), abime);   // CRC faux → absente
    ecrireFichier(carte::nomFichier("feuille-test", carte::FormatTexture::RGBA8) + ".part", carte::ecrire(pa));   // jamais lu
    { carte::Paquet autre = pa; autre.carte = "autre-test"; autre.empreinte ^= 1;   // autre génération → absente
      ecrireFichier(carte::nomFichier("autre-test", carte::FormatTexture::RGBA8), carte::ecrire(autre)); }
    VERIFIER(decouvrir() == 1);
    VERIFIER(donnees("noel-test") && !donnees("automne-test") && !donnees("autre-test") && !donnees("feuille-test"));
    ecrireFichier(carte::nomFichier("automne-test", carte::FormatTexture::RGBA8), carte::ecrire(pa));
    VERIFIER(decouvrir() == 2 && donnees("automne-test") && donnees("automne-test")->segments() == 900);
    VERIFIER(empreinteDonnees("noel-test") != empreinteDonnees("automne-test"));
    std::filesystem::remove_all(dir);
    oublier();
    VERIFIER(std::string(mapById("noel-test").id) == "ville");   // registre vidé : retour aux seules cartes du code

    if (g_echecs) { std::printf("test_cartes_v2_donnees : %d échec(s)\n", g_echecs); return 1; }
    std::printf("test_cartes_v2_donnees : OK\n");
    return 0;
}
