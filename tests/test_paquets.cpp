// WFS-306 : machine d'états de « Mise à jour et installation des paquets » (src/ui/ecrans/paquets_etape.cpp), sans GPU, avec une source
// factice : paquet présent → rien ; absent : Recherche → secours (pas de liste), Proposition, Place, Téléchargement (coupure, fin), Échec,
// « Plus tard » (aussi pendant une coupure) ; proposition ouverte depuis le bandeau du menu ; versionApk, texteMo, texteFormats.
// usage : test_paquets [dossier temporaire]
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include "../src/core/carte_cuite.h"
#include "../src/ui/ecrans/paquets.h"
using namespace wfs;
using namespace wfs::ecrans;

static int g_echecs = 0;
#define VERIFIER(c) do { if (!(c)) { std::printf("ÉCHEC ligne %d : %s\n", __LINE__, #c); g_echecs++; } } while (0)

struct Factice : SourcePaquets {
    Phase ph = RIEN; long long recus = 0, taille = 20 << 20, place = 1LL << 40; int reseauF = 1, chercheS = 0, installeS = 0, arretS = 0;
    bool liste = true;
    void chercher(const std::string&, const std::string&, const std::string&, int) override { chercheS++; ph = liste ? PRETE : ABSENTE; }
    Phase phase(long long& r, long long& t) override { r = recus; t = taille; return ph; }
    bool proposition(std::string& n, std::string& f) override { if (ph == ABSENTE || ph == RIEN) return false; n = "Ville"; f = "astc"; return true; }
    int reseau() override { return reseauF; }
    long long placeLibre() override { return place; }
    void installer() override { installeS++; ph = TELECHARGE; }
    void arreter() override { arretS++; ph = PRETE; }
};

int main(int argc, char** argv) {
    const std::string dir = (argc > 1 ? std::string(argv[1]) : std::string(".")) + "/test-paquets";
    std::filesystem::create_directories(dir);
    carte::dossierPaquets() = dir;
    const std::vector<carte::FormatTexture> formats = {carte::FormatTexture::ASTC, carte::FormatTexture::RGBA8};
    const uint64_t emp = carte::empreinte("ville", 1, 2, {});
    for (carte::FormatTexture f : carte::FORMATS) std::filesystem::remove(dir + "/" + carte::nomFichier("ville", f));

    // outils
    VERIFIER(versionApk("0.1.912") == 912);
    VERIFIER(versionApk("v0.1.7") == 7);
    VERIFIER(versionApk("dev") == 0);
    VERIFIER(versionApk(nullptr) == 0);
    VERIFIER(texteMo(22771234) == "22 Mo");
    VERIFIER(texteMo(10) == "1 Mo");
    VERIFIER(texteMo(210LL << 20) == "210 Mo");
    VERIFIER(texteFormats(formats) == "astc,rgba8");

    EtapePaquets e;
    double t = 0;
    // pas de liste (hors ligne, 404) → secours silencieux
    { Factice s; s.liste = false;
      VERIFIER(e.demarrerAvec(s, "ville", formats, emp, 950, t));
      VERIFIER(s.chercheS == 1 && e.etat() == EtapePaquets::Etat::Recherche);
      e.avancer(t += 0.1);
      VERIFIER(e.etat() == EtapePaquets::Etat::Fini && e.secours_ && !e.installe_); }
    // proposition : rien ne part sans « Installer » ; coupure ; fin vérifiée
    { Factice s;
      e.demarrerAvec(s, "ville", formats, emp, 950, t); e.avancer(t += 0.1);
      VERIFIER(e.etat() == EtapePaquets::Etat::Proposition && s.installeS == 0);
      e.avancer(t += 5);
      VERIFIER(s.installeS == 0);
      e.installer();
      VERIFIER(s.installeS == 1 && e.etat() == EtapePaquets::Etat::Telechargement);
      s.recus = 5 << 20; s.ph = SourcePaquets::COUPURE; e.avancer(t += 0.1);
      VERIFIER(e.etat() == EtapePaquets::Etat::Telechargement);   // coupure : reprise automatique, l'étape attend
      s.ph = SourcePaquets::TELECHARGE; s.recus = s.taille; s.ph = SourcePaquets::FINI; e.avancer(t += 0.1);
      VERIFIER(e.ouvert() && e.installe_);   // « Ville installée » montrée un instant
      e.avancer(t += 1);
      VERIFIER(e.etat() == EtapePaquets::Etat::Fini && !e.secours_); }
    // place insuffisante → message puis « Continuer » → secours
    { Factice s; s.place = 1 << 20;
      e.demarrerAvec(s, "ville", formats, emp, 950, t); e.avancer(t += 0.1);
      VERIFIER(e.etat() == EtapePaquets::Etat::Place);
      e.installer(); VERIFIER(s.installeS == 0);   // aucun téléchargement possible
      e.continuer();
      VERIFIER(e.etat() == EtapePaquets::Etat::Fini && e.secours_); }
    // paquet abîmé → Échec → « Réessayer » relance, « Plus tard » → secours
    { Factice s;
      e.demarrerAvec(s, "ville", formats, emp, 950, t); e.avancer(t += 0.1); e.installer();
      s.ph = SourcePaquets::SHA_FAUX; e.avancer(t += 0.1);
      VERIFIER(e.etat() == EtapePaquets::Etat::Echec);
      e.installer();
      VERIFIER(s.installeS == 2 && e.etat() == EtapePaquets::Etat::Telechargement);
      s.ph = SourcePaquets::ERREUR; e.avancer(t += 0.1);
      VERIFIER(e.etat() == EtapePaquets::Etat::Echec);
      e.plusTard();
      VERIFIER(e.etat() == EtapePaquets::Etat::Fini && e.secours_); }
    // « Plus tard » pendant une coupure → arrêt demandé, phase PRETE, secours ; bandeau : proposition par-dessus le menu
    { Factice s;
      e.demarrerAvec(s, "ville", formats, emp, 950, t); e.avancer(t += 0.1); e.installer();
      s.ph = SourcePaquets::COUPURE; e.avancer(t += 0.1);
      e.plusTard();
      long long r = 0, tt = 0;
      VERIFIER(s.arretS == 1 && s.phase(r, tt) == SourcePaquets::PRETE && e.etat() == EtapePaquets::Etat::Fini && e.secours_);
      VERIFIER(!e.tientLaFenetre());
      s.reseauF = 2;
      VERIFIER(e.ouvrirProposition(t += 1));
      VERIFIER(e.enMenu() && e.etat() == EtapePaquets::Etat::Proposition && !e.tientLaFenetre() && s.installeS == 1);
      e.plusTard();
      VERIFIER(!e.ouvert() && !e.enMenu() && s.installeS == 1);   // « Plus tard » : refermée, rien ne part
      VERIFIER(e.ouvrirProposition(t += 1));
      e.installer();
      VERIFIER(s.installeS == 2 && !e.enMenu() && !e.ouvert());   // téléchargement au menu : le bandeau montre la progression
      VERIFIER(!e.ouvrirProposition(t += 1)); }                  // plus de proposition pendant le téléchargement
    // paquet juste déjà là → pas d'étape (RG-01)
    { carte::Paquet p; p.carte = "ville"; p.format = carte::FormatTexture::RGBA8; p.empreinte = emp; p.blocs = {{"textures", {}}};
      const std::vector<uint8_t> o = carte::ecrire(p);
      std::ofstream(dir + "/" + carte::nomFichier("ville", carte::FormatTexture::RGBA8), std::ios::binary).write((const char*)o.data(), (std::streamsize)o.size());
      Factice s;
      VERIFIER(!e.demarrerAvec(s, "ville", formats, emp, 950, t) && s.chercheS == 0 && !e.ouvert());
      VERIFIER(e.demarrerAvec(s, "ville", formats, emp ^ 1, 950, t) && s.chercheS == 1);   // autre génération attendue : liste lue
      std::filesystem::remove(dir + "/" + carte::nomFichier("ville", carte::FormatTexture::RGBA8)); }

    if (g_echecs) { std::printf("test_paquets : %d échec(s)\n", g_echecs); return 1; }
    std::printf("test_paquets : OK\n");
    return 0;
}
