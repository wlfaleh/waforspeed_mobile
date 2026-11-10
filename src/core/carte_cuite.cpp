// WaForSpeed — paquets de cartes « .wfcarte » (WFS-301) : voir carte_cuite.h.
#include "carte_cuite.h"
#include <memory>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <fstream>
#include <map>
#include <mutex>

namespace wfs {
namespace carte {
namespace {

// ---------------------------------------------------------------- écriture et lecture petit-boutistes
struct Ecrivain {
    std::vector<uint8_t>& o;
    void u8(uint8_t v) { o.push_back(v); }
    void u16(uint16_t v) { for (int i = 0; i < 2; i++) o.push_back((uint8_t)(v >> (8 * i))); }
    void u32(uint32_t v) { for (int i = 0; i < 4; i++) o.push_back((uint8_t)(v >> (8 * i))); }
    void u64(uint64_t v) { for (int i = 0; i < 8; i++) o.push_back((uint8_t)(v >> (8 * i))); }
    void octets(const void* p, size_t n) { const uint8_t* b = (const uint8_t*)p; o.insert(o.end(), b, b + n); }
    void poserU64(size_t pos, uint64_t v) { for (int i = 0; i < 8; i++) o[pos + i] = (uint8_t)(v >> (8 * i)); }
    void poserU32(size_t pos, uint32_t v) { for (int i = 0; i < 4; i++) o[pos + i] = (uint8_t)(v >> (8 * i)); }
};
// lecteur borné : toute lecture hors du tampon met ok à faux (jamais de lecture hors limites)
struct Lecteur {
    const uint8_t* d; size_t n, p = 0; bool ok = true;
    bool place(size_t k) { if (!ok || k > n - std::min(p, n)) { ok = false; return false; } return true; }
    uint64_t lireN(int k) { if (!place((size_t)k)) return 0; uint64_t v = 0; for (int i = 0; i < k; i++) v |= (uint64_t)d[p + i] << (8 * i); p += (size_t)k; return v; }
    uint8_t u8() { return (uint8_t)lireN(1); }
    uint16_t u16() { return (uint16_t)lireN(2); }
    uint32_t u32() { return (uint32_t)lireN(4); }
    uint64_t u64() { return lireN(8); }
    std::string texte(size_t k) { if (!place(k)) return {}; std::string s((const char*)d + p, k); p += k; return s; }
};

const char MAGIE[4] = {'W', 'F', 'S', 'C'};
const char MAGIE_ARCHIVE[4] = {'W', 'F', 'S', 'A'};

// ---------------------------------------------------------------- paquet actif (2e source)
struct Actif {
    std::mutex m;   // activation sur le fil principal ; lectures possibles depuis les tâches de chargement
    std::string carte;
    std::vector<Fichier> fichiers;               // fichiers des archives maillages / textures
    std::map<std::string, size_t> index;         // chemin → fichiers[i]
    std::vector<std::pair<std::string, std::shared_ptr<const std::vector<uint8_t>>>> blocs;   // WFS-416 : autres blocs non vides (ex. « batiments »)
    std::atomic<long long> lus{0};
};
Actif& actif_() { static Actif a; return a; }

} // namespace

const char* nomFormat(FormatTexture f) {
    switch (f) { case FormatTexture::ASTC: return "astc"; case FormatTexture::ETC2: return "etc2"; default: return "rgba8"; }
}
std::string nomFichier(const std::string& c, FormatTexture f) { return c + "_" + nomFormat(f) + ".wfcarte"; }

const Bloc* Paquet::bloc(const std::string& nom) const {
    for (const Bloc& b : blocs) if (b.nom == nom) return &b;
    return nullptr;
}

uint32_t crc32(const uint8_t* d, size_t n) {
    static uint32_t table[256];
    static const bool pret = [] {
        for (uint32_t i = 0; i < 256; i++) { uint32_t c = i; for (int k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1; table[i] = c; }
        return true;
    }();
    (void)pret;
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) c = table[(c ^ d[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

uint64_t empreinte(const std::string& c, uint32_t graine, int versionTrace, const std::vector<std::string>& valeurs) {
    uint64_t h = 1469598103934665603ull;   // FNV-1a 64 bits
    auto melanger = [&](const void* p, size_t n) { const uint8_t* b = (const uint8_t*)p; for (size_t i = 0; i < n; i++) { h ^= b[i]; h *= 1099511628211ull; } };
    auto chaine = [&](const std::string& s) { const uint32_t n = (uint32_t)s.size(); melanger(&n, 4); melanger(s.data(), s.size()); };
    const uint32_t gen = VERSION_GENERATION_CARTE, vt = (uint32_t)versionTrace, nv = (uint32_t)valeurs.size();
    chaine(c); melanger(&graine, 4); melanger(&vt, 4); melanger(&gen, 4); melanger(&nv, 4);
    for (const std::string& v : valeurs) chaine(v);
    return h;
}

std::vector<uint8_t> ecrire(const Paquet& p) {
    std::vector<uint8_t> o;
    Ecrivain e{o};
    e.octets(MAGIE, 4);
    e.u32(VERSION_FORMAT);
    const size_t posTaille = o.size(); e.u32(0);
    e.u16((uint16_t)std::min<size_t>(p.carte.size(), 65535)); e.octets(p.carte.data(), std::min<size_t>(p.carte.size(), 65535));
    e.u32(p.versionCarte);
    e.u8((uint8_t)p.format);
    e.u64(p.empreinte);
    e.u32((uint32_t)p.blocs.size());
    std::vector<size_t> posPosition;
    for (const Bloc& b : p.blocs) {
        const size_t n = std::min<size_t>(b.nom.size(), 255);
        e.u8((uint8_t)n); e.octets(b.nom.data(), n);
        posPosition.push_back(o.size()); e.u64(0);
        e.u64(b.donnees.size());
        e.u32(crc32(b.donnees.data(), b.donnees.size()));
    }
    const size_t tailleEnTete = o.size() + 4;
    e.poserU32(posTaille, (uint32_t)tailleEnTete);
    size_t pos = tailleEnTete;
    for (size_t i = 0; i < p.blocs.size(); i++) { e.poserU64(posPosition[i], pos); pos += p.blocs[i].donnees.size(); }
    e.u32(crc32(o.data(), o.size()));
    for (const Bloc& b : p.blocs) e.octets(b.donnees.data(), b.donnees.size());
    return o;
}

const char* texte(Verdict v) {
    switch (v) {
    case Verdict::Bon: return "paquet vérifié";
    case Verdict::Absent: return "paquet absent";
    case Verdict::Illisible: return "paquet illisible (fichier tronqué ou en-tête invalide)";
    case Verdict::MagieFausse: return "ce n'est pas un paquet de carte (magie WFSC absente)";
    case Verdict::VersionFuture: return "paquet d'un format plus récent que ce jeu (mise à jour du jeu nécessaire)";
    case Verdict::AutreCarte: return "paquet d'une autre carte";
    case Verdict::MauvaisFormat: return "paquet d'un autre format de texture";
    case Verdict::EmpreinteFausse: return "paquet d'une autre génération de la carte (empreinte différente)";
    case Verdict::TailleFausse: return "taille d'un bloc incohérente (paquet abîmé)";
    case Verdict::CrcFaux: return "somme de contrôle fausse (paquet abîmé)";
    case Verdict::NonFini: return "fichier non fini (.part), jamais lu";
    }
    return "?";
}

Verdict lire(const std::vector<uint8_t>& o, const Attendu& a, Paquet* sortie) {
    if (o.size() < 12) return o.size() >= 4 && std::memcmp(o.data(), MAGIE, 4) != 0 ? Verdict::MagieFausse : Verdict::Illisible;
    if (std::memcmp(o.data(), MAGIE, 4) != 0) return Verdict::MagieFausse;
    Lecteur l{o.data(), o.size()};
    l.p = 4;
    const uint32_t version = l.u32();   // lue en premier (RG-01)
    if (version > VERSION_FORMAT) return Verdict::VersionFuture;
    if (version == 0) return Verdict::Illisible;
    const uint32_t tailleEnTete = l.u32();
    if (tailleEnTete < 12 || tailleEnTete > o.size()) return Verdict::Illisible;
    {   // CRC de l'en-tête avant de croire la table
        Lecteur c{o.data(), o.size()}; c.p = tailleEnTete - 4;
        if (crc32(o.data(), tailleEnTete - 4) != c.u32()) return Verdict::CrcFaux;
    }
    Paquet p;
    p.carte = l.texte(l.u16());
    p.versionCarte = l.u32();
    const uint8_t f = l.u8();
    p.empreinte = l.u64();
    const uint32_t nb = l.u32();
    if (!l.ok || f > 2 || nb > 64) return Verdict::Illisible;
    p.format = (FormatTexture)f;
    if (p.carte != a.carte) return Verdict::AutreCarte;
    if (p.format != a.format) return Verdict::MauvaisFormat;
    if (p.empreinte != a.empreinte) return Verdict::EmpreinteFausse;
    struct Entree { std::string nom; uint64_t pos, taille; uint32_t crc; };
    std::vector<Entree> table;
    for (uint32_t i = 0; i < nb; i++) {
        Entree e; e.nom = l.texte(l.u8()); e.pos = l.u64(); e.taille = l.u64(); e.crc = l.u32();
        table.push_back(e);
    }
    if (!l.ok || l.p + 4 != tailleEnTete) return Verdict::Illisible;
    for (const Entree& e : table) {
        if (e.pos < tailleEnTete || e.pos > o.size() || e.taille > o.size() - e.pos) return Verdict::TailleFausse;
        if (crc32(o.data() + e.pos, (size_t)e.taille) != e.crc) return Verdict::CrcFaux;
    }
    if (sortie) {
        for (const Entree& e : table) p.blocs.push_back({e.nom, std::vector<uint8_t>(o.begin() + (long)e.pos, o.begin() + (long)(e.pos + e.taille))});
        *sortie = std::move(p);
    }
    return Verdict::Bon;
}

Verdict verifier_carte(const std::string& chemin, const Attendu& a, Paquet* sortie) {
    if (chemin.size() >= 5 && chemin.compare(chemin.size() - 5, 5, ".part") == 0) return Verdict::NonFini;
    std::ifstream f(chemin, std::ios::binary | std::ios::ate);
    if (!f) return Verdict::Absent;
    const std::streamoff n = f.tellg();
    if (n < 0) return Verdict::Illisible;
    std::vector<uint8_t> o((size_t)n);
    f.seekg(0);
    if (n > 0 && !f.read((char*)o.data(), n)) return Verdict::Illisible;   // lecture en un bloc (rapide sur le téléphone)
    return lire(o, a, sortie);
}

std::vector<uint8_t> archive(std::vector<Fichier> fichiers) {
    std::sort(fichiers.begin(), fichiers.end(), [](const Fichier& x, const Fichier& y) { return x.chemin < y.chemin; });
    std::vector<uint8_t> o;
    Ecrivain e{o};
    e.octets(MAGIE_ARCHIVE, 4);
    e.u32((uint32_t)fichiers.size());
    for (const Fichier& f : fichiers) {
        const size_t n = std::min<size_t>(f.chemin.size(), 65535);
        e.u16((uint16_t)n); e.octets(f.chemin.data(), n);
        e.u64(f.octets.size()); e.octets(f.octets.data(), f.octets.size());
    }
    return o;
}

bool lireArchive(const std::vector<uint8_t>& a, std::vector<Fichier>& sortie) {
    sortie.clear();
    if (a.size() < 8 || std::memcmp(a.data(), MAGIE_ARCHIVE, 4) != 0) return false;
    Lecteur l{a.data(), a.size()}; l.p = 4;
    const uint32_t n = l.u32();
    for (uint32_t i = 0; i < n && l.ok; i++) {
        Fichier f; f.chemin = l.texte(l.u16());
        const uint64_t t = l.u64();
        if (!l.place((size_t)std::min<uint64_t>(t, a.size()))) break;
        if (t > a.size() - l.p) { l.ok = false; break; }
        f.octets.assign(a.begin() + (long)l.p, a.begin() + (long)(l.p + t)); l.p += (size_t)t;
        sortie.push_back(std::move(f));
    }
    if (!l.ok || l.p != a.size()) { sortie.clear(); return false; }
    return true;
}

std::string& dossierPaquets() { static std::string d; return d; }

void activer(Paquet&& p) {
    Actif& A = actif_();
    std::lock_guard<std::mutex> v(A.m);
    A.carte = p.carte; A.fichiers.clear(); A.index.clear(); A.blocs.clear(); A.lus = 0;
    for (Bloc& b : p.blocs) if (b.nom != "maillages" && b.nom != "textures" && !b.donnees.empty())   // WFS-416
        A.blocs.emplace_back(b.nom, std::make_shared<const std::vector<uint8_t>>(std::move(b.donnees)));
    for (const char* nom : {"maillages", "textures"}) {
        const Bloc* b = p.bloc(nom);
        std::vector<Fichier> f;
        if (!b || b->donnees.empty() || !lireArchive(b->donnees, f)) continue;
        for (Fichier& x : f) { A.index[x.chemin] = A.fichiers.size(); A.fichiers.push_back(std::move(x)); }
    }
}
void desactiver() {
    Actif& A = actif_();
    std::lock_guard<std::mutex> v(A.m);
    A.carte.clear(); A.fichiers.clear(); A.index.clear(); A.blocs.clear(); A.lus = 0;
}
std::shared_ptr<const std::vector<uint8_t>> blocDuPaquet(const std::string& carte, const std::string& nom) {
    Actif& A = actif_();
    std::lock_guard<std::mutex> v(A.m);
    if (A.carte != carte) return nullptr;
    for (const auto& b : A.blocs) if (b.first == nom) return b.second;
    return nullptr;
}
bool actif() { Actif& A = actif_(); std::lock_guard<std::mutex> v(A.m); return !A.carte.empty(); }
std::string carteActive() { Actif& A = actif_(); std::lock_guard<std::mutex> v(A.m); return A.carte; }

bool lireDansPaquet(const std::string& chemin, std::vector<uint8_t>& out) {
    Actif& A = actif_();
    std::lock_guard<std::mutex> v(A.m);
    const auto it = A.index.find(chemin);
    if (it == A.index.end()) return false;
    out = A.fichiers[it->second].octets;
    A.lus++;
    return true;
}
long long lusDansPaquet() { return actif_().lus.load(); }

} // namespace carte
} // namespace wfs
