// WaForSpeed — paquets de cartes : choix « paquet ou secours », cuisson et banc (WFS-301) : voir paquet_carte.h.
#include "paquet_carte.h"
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include "../../core/log.h"
#include "../../core/platform.h"
#include "../../game/game.h"
#include "../../game/trace_cartes.h"
#include "../../game/track.h"
#include "../../render/environnement.h"   // banc : capture d'environnement remise à zéro avant chaque scène
#include "../../render/renderer.h"
#include "../../ui/text.h"
#include "../../../third_party/stb/stb_image.h"   // implémentation : model_loader.cpp

namespace wfs {
namespace cartes_v2 {
namespace fs = std::filesystem;

std::vector<std::string> valeursConfigConstruction(const std::string&) { return {}; }

uint64_t empreinteAttendue(const std::string& c) {
    return carte::empreinte(c, mapById(c.c_str()).seed, trace::versionCarte(c.c_str()), valeursConfigConstruction(c));
}

bool preparerCarte(const std::string& c) {
    const double t0 = nowSeconds();
    std::string raisons;
    for (carte::FormatTexture f : carte::FORMATS) {
        const std::string chemin = carte::dossierPaquets() + "/" + carte::nomFichier(c, f);
        carte::Paquet p;
        const carte::Verdict v = carte::verifier_carte(chemin, {c, f, empreinteAttendue(c)}, &p);
        if (v == carte::Verdict::Bon) {
            const size_t blocs = p.blocs.size();
            carte::activer(std::move(p));
            WFS_LOG("paquet de carte (WFS-301) : %s vérifié en %d ms (%zu blocs), lu avant l'APK", chemin.c_str(),
                    (int)std::lround((nowSeconds() - t0) * 1000), blocs);
            return true;
        }
        if (v != carte::Verdict::Absent) raisons += std::string(raisons.empty() ? "" : " ; ") + carte::nomFichier(c, f) + " : " + carte::texte(v);
    }
    carte::desactiver();
    WFS_LOG("paquet de carte (WFS-301) : %s construite par le code (secours) — %s", c.c_str(), raisons.empty() ? "paquet absent" : raisons.c_str());
    return false;
}

namespace {
bool lireFichier(const fs::path& p, std::vector<uint8_t>& o) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return false;
    o.assign(std::istreambuf_iterator<char>(f), {});
    return true;
}
// fichiers d'un dossier des assets (chemins relatifs à la racine, séparateur « / »), filtrés par suffixe
void ajouter(const fs::path& racine, const char* dossier, const std::string& suffixe, std::vector<carte::Fichier>& sortie) {
    std::error_code ec;
    const fs::path d = racine / dossier;
    if (!fs::exists(d, ec)) return;
    for (const auto& e : fs::recursive_directory_iterator(d, ec)) {
        if (!e.is_regular_file()) continue;
        const std::string rel = fs::relative(e.path(), racine).generic_string();
        if (rel.size() < suffixe.size() || rel.compare(rel.size() - suffixe.size(), suffixe.size(), suffixe) != 0) continue;
        carte::Fichier f; f.chemin = rel;
        if (lireFichier(e.path(), f.octets)) sortie.push_back(std::move(f));
    }
}
// part des pixels qui diffèrent de plus de 8 / 255 sur une couleur (RG-04 : ≤ 1 %) ; −1 si une image manque
double ecartPixels(const std::string& a, const std::string& b) {
    int wa, ha, ca, wb, hb, cb;
    unsigned char* pa = stbi_load(a.c_str(), &wa, &ha, &ca, 3);
    unsigned char* pb = stbi_load(b.c_str(), &wb, &hb, &cb, 3);
    double r = -1;
    if (pa && pb && wa == wb && ha == hb) {
        long n = 0;
        for (long i = 0; i < (long)wa * ha; i++)
            for (int k = 0; k < 3; k++) if (std::abs(pa[i * 3 + k] - pb[i * 3 + k]) > 8) { n++; break; }
        r = 100.0 * n / ((double)wa * ha);
    }
    stbi_image_free(pa); stbi_image_free(pb);
    return r;
}
bool ecrireFichier(const std::string& chemin, const std::vector<uint8_t>& o) {   // temporaire puis renommage : jamais de fichier à moitié écrit
    const std::string tmp = chemin + ".tmp";
    { std::ofstream f(tmp, std::ios::binary); if (!f) return false; f.write((const char*)o.data(), (std::streamsize)o.size()); if (!f) return false; }
    std::error_code ec; fs::rename(tmp, chemin, ec);
    return !ec;
}
} // namespace

carte::Paquet cuire(const std::string& racine, const std::string& c, carte::FormatTexture f) {
    carte::Paquet p;
    p.carte = c; p.versionCarte = (uint32_t)trace::versionCarte(c.c_str()); p.format = f; p.empreinte = empreinteAttendue(c);
    // partie A : fichiers de la carte tels quels (mêmes octets que l'APK → même image) ; decor_regroupe vide jusqu'à la partie B
    std::vector<carte::Fichier> maillages, textures;
    ajouter(racine, "world/animaux", ".wfa", maillages);
    ajouter(racine, "textures", std::string(".") + carte::nomFormat(f) + ".ktx2", textures);
    ajouter(racine, "world", "feuillage.png", textures);
    p.blocs.push_back({"decor_regroupe", {}});
    p.blocs.push_back({"maillages", carte::archive(std::move(maillages))});
    p.blocs.push_back({"textures", carte::archive(std::move(textures))});
    p.blocs.push_back({"trace", {}});   // réservé : tracé d'une future carte au tracé différent
    p.blocs.push_back({"chocs", {}});   // réservé : chocs du décor d'une future carte
    return p;
}

int benchCuisson(Game& g, Renderer& r, const std::string& out, const std::string& racine) {
    int ecarts = 0;
    auto verifier = [&](bool ok, const std::string& quoi) { WFS_LOG("cuisson (WFS-301) banc : %s : %s", quoi.c_str(), ok ? "OK" : "ÉCART"); if (!ok) ecarts++; };
    const std::string c = "ville", dossier = carte::dossierPaquets();
    std::error_code ec;
    fs::create_directories(dossier, ec);
    // 1) cuisson des 3 formats : deux cuissons identiques octet par octet (RG-04, partie déterministe), fichier écrit, vérification < 200 ms
    std::vector<std::vector<uint8_t>> bons(3);
    for (int i = 0; i < 3; i++) {
        const carte::FormatTexture f = carte::FORMATS[i];
        const double t0 = nowSeconds();
        bons[i] = carte::ecrire(cuire(racine, c, f));
        const double tc = nowSeconds() - t0;
        verifier(carte::ecrire(cuire(racine, c, f)) == bons[i], std::string("deux cuissons identiques (") + carte::nomFormat(f) + ")");
        const std::string chemin = dossier + "/" + carte::nomFichier(c, f);
        verifier(ecrireFichier(chemin, bons[i]), "écriture de " + chemin);
        carte::Paquet p;
        const double t1 = nowSeconds();
        const carte::Verdict v = carte::verifier_carte(chemin, {c, f, empreinteAttendue(c)}, &p);
        const double tv = nowSeconds() - t1;
        std::vector<carte::Fichier> tex, mai;
        if (const carte::Bloc* b = p.bloc("textures")) carte::lireArchive(b->donnees, tex);
        if (const carte::Bloc* b = p.bloc("maillages")) carte::lireArchive(b->donnees, mai);
        WFS_LOG("cuisson (WFS-301) : %s : %.2f Mo, %zu textures, %zu maillages, cuit en %d ms, vérifié en %d ms", carte::nomFichier(c, f).c_str(),
                bons[i].size() / 1048576.0, tex.size(), mai.size(), (int)std::lround(tc * 1000), (int)std::lround(tv * 1000));
        verifier(v == carte::Verdict::Bon, std::string("verifier_carte bon (") + carte::nomFormat(f) + ")");
    }
    // 2) parcours RG-08 : la Ville dans 4 états du dossier des paquets, de jour (et de nuit pour le paquet valide)
    auto retirer = [&]() { for (carte::FormatTexture f : carte::FORMATS) fs::remove(dossier + "/" + carte::nomFichier(c, f), ec); };
    auto poser = [&](int i, std::vector<uint8_t> o) { ecrireFichier(dossier + "/" + carte::nomFichier(c, carte::FORMATS[i]), o); };
    auto course = [&](const char* moment, const char* nom, const char* quoi) {
        setenv("WFS_MOMENT", moment, 1); setenv("WFS_DEPART", "0", 1);
        environnement::invalider();   // même point de départ du rendu pour chaque scène comparée
        if (!g.init(r, "golf", c.c_str())) { WFS_ERR("cuisson (WFS-301) banc : chargement de la Ville impossible"); ecarts++; return; }
        g.setAutopilot(true); g.camMode = 1;
        for (int i = 0; i < 240; i++) { g.update(1.0 / 60); if (i % 30 == 0) g.render(400 + i / 60.0); }
        g.render(404); g.render(404.02); r.capturePng(out + "/" + nom + ".png");
        WFS_LOG("capture %s : Ville, %s, %s ; %lld fichier(s) lus dans le paquet", nom, moment, quoi, carte::lusDansPaquet());
        g.shutdown();
    };
    ui::chargerPolice(r);
    retirer();
    verifier(!preparerCarte(c), "paquet absent → Ville du code (secours)");
    // la lumière du rendu (capture d'environnement et sondes, WFS-150 / 151) se construit d'une partie à l'autre : chaque scène comparée
    // est jouée deux fois de suite, seule la 2e est capturée (même passé pour les deux scènes comparées)
    course("jour", "cuisson-tmp", "sans paquet, 1er passage (non comparé)");
    course("jour", "cuisson-sans-paquet", "sans paquet (secours)");
    {   // paquets abîmés : un octet changé dans les données (CRC faux) ; tronqué ; version du format future ; mauvaise génération
        std::vector<uint8_t> corrompu = bons[0]; corrompu[corrompu.size() / 2] ^= 0x5A; poser(0, corrompu);
        std::vector<uint8_t> tronque(bons[1].begin(), bons[1].begin() + (long)(bons[1].size() / 3)); poser(1, tronque);
        std::vector<uint8_t> futur = bons[2]; futur[4] = (uint8_t)(carte::VERSION_FORMAT + 1); poser(2, futur);
        verifier(!preparerCarte(c), "paquets corrompu, tronqué et d'un format futur → Ville du code (secours, raisons au journal)");
        course("jour", "cuisson-paquet-corrompu", "paquets abîmés (secours)");
        carte::Paquet autre = cuire(racine, c, carte::FORMATS[2]); autre.empreinte ^= 1;
        retirer(); poser(2, carte::ecrire(autre));
        verifier(!preparerCarte(c), "paquet d'une autre génération (empreinte) → Ville du code");
    }
    retirer();
    poser(0, bons[0]); poser(2, bons[2]);   // ASTC et RGBA8 (pas de texture ETC2 dans le dépôt) : ASTC préféré
    verifier(preparerCarte(c) && carte::carteActive() == c, "paquet valide → activé, lu avant l'APK");
    course("jour", "cuisson-tmp", "paquet partiel valide, 1er passage (non comparé)");
    fs::remove(out + "/cuisson-tmp.png", ec);
    course("jour", "cuisson-paquet-jour", "paquet partiel valide");
    verifier(carte::lusDansPaquet() > 0, "des fichiers de la Ville ont été lus dans le paquet");
    verifier(preparerCarte(c), "paquet valide (nuit)");
    course("nuit", "cuisson-paquet-nuit", "paquet partiel valide");
    {   // même image (partie A) : chaque fichier servi par le paquet est identique, octet par octet, à celui de l'APK (preuve exacte) ;
        // l'écart de pixels est donné pour information : la capture d'environnement (reflets, WFS-150) est prise à un point qui dépend
        // du temps de chargement, et deux parties sans paquet diffèrent déjà de la même façon (mesure « secours / secours » ci-dessous)
        carte::Paquet p;
        carte::verifier_carte(dossier + "/" + carte::nomFichier(c, carte::FORMATS[0]), {c, carte::FORMATS[0], empreinteAttendue(c)}, &p);
        size_t n = 0, pareils = 0;
        for (const char* nom : {"maillages", "textures"}) {
            std::vector<carte::Fichier> f;
            if (const carte::Bloc* b = p.bloc(nom)) carte::lireArchive(b->donnees, f);
            for (const carte::Fichier& x : f) { std::vector<uint8_t> apk; n++; if (lireFichier(fs::path(racine) / x.chemin, apk) && apk == x.octets) pareils++; }
        }
        WFS_LOG("cuisson (WFS-301) banc : %zu / %zu fichiers du paquet identiques à l'APK", pareils, n);
        verifier(n > 0 && pareils == n, "fichiers du paquet identiques à ceux de l'APK (même image)");
        WFS_LOG("cuisson (WFS-301) banc : écart de pixels, pour information (reflets, WFS-150) : secours (2e passage) / secours (1er passage après un changement) %.2f %%, secours / paquet (2es passages) %.2f %%",
                ecartPixels(out + "/cuisson-sans-paquet.png", out + "/cuisson-paquet-corrompu.png"), ecartPixels(out + "/cuisson-sans-paquet.png", out + "/cuisson-paquet-jour.png"));
    }
    {   // un « .part » n'est jamais lu
        const std::string part = dossier + "/" + carte::nomFichier(c, carte::FORMATS[0]) + ".part";
        ecrireFichier(part, bons[0]);
        verifier(carte::verifier_carte(part, {c, carte::FORMATS[0], empreinteAttendue(c)}) == carte::Verdict::NonFini, "fichier .part refusé");
        fs::remove(part, ec);
    }
    carte::desactiver();
    retirer();   // le banc ne laisse aucun paquet : les autres scènes jouent la carte du code
    WFS_LOG("cuisson (WFS-301) banc : %d écart(s)%s", ecarts, ecarts ? " ÉCHEC" : " OK");
    return ecarts;
}

} // namespace cartes_v2
} // namespace wfs
