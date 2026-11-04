// WaForSpeed — BinaryPack (WFS-89) : voir binarypack.h. Port de peerjs-js-binarypack 2.1.0 (dist/binarypack.mjs).
#include "binarypack.h"
#include <cmath>
#include <cstring>

namespace wfs::bp {
namespace {

struct Ecrivain {
    std::vector<uint8_t> o;
    void u8(unsigned v) { o.push_back((uint8_t)(v & 0xff)); }
    void u16(unsigned v) { u8(v >> 8); u8(v); }
    void u32(uint32_t v) { u8(v >> 24); u8(v >> 16); u8(v >> 8); u8(v); }
    void octets(const void* p, size_t n) { const uint8_t* b = (const uint8_t*)p; o.insert(o.end(), b, b + n); }
};

// pack_integer du jeu web (mêmes bornes, même ordre des cas)
void entier(Ecrivain& w, double num) {
    if (num >= -32 && num <= 0x7f) { w.u8((unsigned)((int64_t)num & 0xff)); return; }
    if (num >= 0 && num <= 0xff) { w.u8(0xcc); w.u8((unsigned)num); return; }
    if (num >= -128 && num <= 0x7f) { w.u8(0xd0); w.u8((unsigned)((int64_t)num & 0xff)); return; }
    if (num >= 0 && num <= 0xffff) { w.u8(0xcd); w.u16((unsigned)num); return; }
    if (num >= -32768 && num <= 0x7fff) { w.u8(0xd1); w.u16((unsigned)((int64_t)num & 0xffff)); return; }
    if (num >= 0 && num <= 4294967295.0) { w.u8(0xce); w.u32((uint32_t)num); return; }
    if (num >= -2147483648.0 && num <= 2147483647.0) { w.u8(0xd2); w.u32((uint32_t)(int32_t)num); return; }
    // int64 (pack_int64 du web : partie haute = floor(num / 2^32), partie basse = num mod 2^32)
    const double haut = std::floor(num / 4294967296.0), bas = num - haut * 4294967296.0;
    w.u8(0xd3); w.u32((uint32_t)(int64_t)haut); w.u32((uint32_t)bas);
}
// pack_double du web : 0xcb puis les 64 bits IEEE 754 (gros-boutiste)
void flottant(Ecrivain& w, double num) {
    uint64_t bits; std::memcpy(&bits, &num, 8);
    w.u8(0xcb); w.u32((uint32_t)(bits >> 32)); w.u32((uint32_t)bits);
}
void texte(Ecrivain& w, const std::string& s) {
    const size_t n = s.size();
    if (n <= 0x0f) w.u8(0xb0 + (unsigned)n);
    else if (n <= 0xffff) { w.u8(0xd8); w.u16((unsigned)n); }
    else { w.u8(0xd9); w.u32((uint32_t)n); }
    w.octets(s.data(), n);
}
void ecrire(Ecrivain& w, const Valeur& v) {
    switch (v.type) {
        case Valeur::Nul: w.u8(0xc0); break;
        case Valeur::Booleen: w.u8(v.b ? 0xc3 : 0xc2); break;
        case Valeur::Nombre:
            if (!std::isfinite(v.n)) { w.u8(0xc0); break; }   // NaN, infini : jamais envoyés
            if (std::floor(v.n) == v.n) entier(w, v.n); else flottant(w, v.n);
            break;
        case Valeur::Texte: texte(w, v.s); break;
        case Valeur::Tableau: {
            const size_t n = v.tab.size();
            if (n <= 0x0f) w.u8(0x90 + (unsigned)n); else if (n <= 0xffff) { w.u8(0xdc); w.u16((unsigned)n); } else { w.u8(0xdd); w.u32((uint32_t)n); }
            for (const Valeur& e : v.tab) ecrire(w, e);
            break;
        }
        case Valeur::Objet: {
            const size_t n = v.obj.size();
            if (n <= 0x0f) w.u8(0x80 + (unsigned)n); else if (n <= 0xffff) { w.u8(0xde); w.u16((unsigned)n); } else { w.u8(0xdf); w.u32((uint32_t)n); }
            for (const auto& [k, e] : v.obj) { texte(w, k); ecrire(w, e); }
            break;
        }
    }
}

// ---------------------------------------------------------------- lecture (Unpacker), bornée
struct Lecteur {
    const uint8_t* d; size_t n, i = 0; int elements = 0;
    bool lire(size_t k, const uint8_t*& p) { if (i + k > n) return false; p = d + i; i += k; return true; }
    bool u8(uint32_t& v) { const uint8_t* p; if (!lire(1, p)) return false; v = p[0]; return true; }
    bool u16(uint32_t& v) { const uint8_t* p; if (!lire(2, p)) return false; v = (uint32_t)p[0] << 8 | p[1]; return true; }
    bool u32(uint32_t& v) { const uint8_t* p; if (!lire(4, p)) return false; v = (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3]; return true; }
    bool chaine(size_t k, std::string& s) { const uint8_t* p; if (!lire(k, p)) return false; s.assign((const char*)p, k); return true; }
};
constexpr int PROFONDEUR_MAX = 16, ELEMENTS_MAX = 4096;

bool lireValeur(Lecteur& r, Valeur& v, int prof);
bool lireTableau(Lecteur& r, Valeur& v, size_t k, int prof) {
    v = Valeur::tableau();
    if (k > (size_t)ELEMENTS_MAX || (r.elements += (int)k) > ELEMENTS_MAX) return false;
    v.tab.resize(k);
    for (size_t j = 0; j < k; j++) if (!lireValeur(r, v.tab[j], prof + 1)) return false;
    return true;
}
bool lireObjet(Lecteur& r, Valeur& v, size_t k, int prof) {
    v = Valeur::objet();
    if (k > (size_t)ELEMENTS_MAX || (r.elements += (int)k) > ELEMENTS_MAX) return false;
    for (size_t j = 0; j < k; j++) {
        Valeur cle, e;
        if (!lireValeur(r, cle, prof + 1) || !lireValeur(r, e, prof + 1)) return false;
        std::string c = cle.type == Valeur::Texte ? cle.s : cle.type == Valeur::Nombre ? std::to_string((long long)cle.n) : "";
        v.obj.emplace_back(std::move(c), std::move(e));
    }
    return true;
}
bool lireValeur(Lecteur& r, Valeur& v, int prof) {
    if (prof > PROFONDEUR_MAX) return false;
    uint32_t t, k;
    if (!r.u8(t)) return false;
    if (t < 0x80) { v = Valeur::nombre(t); return true; }
    if ((t ^ 0xe0) < 0x20) { v = Valeur::nombre((double)(t ^ 0xe0) - 0x20); return true; }
    if ((t ^ 0xa0) <= 0x0f) { v = Valeur::texte(""); return r.chaine(t ^ 0xa0, v.s); }   // données binaires : gardées en texte brut
    if ((t ^ 0xb0) <= 0x0f) { v = Valeur::texte(""); return r.chaine(t ^ 0xb0, v.s); }
    if ((t ^ 0x90) <= 0x0f) return lireTableau(r, v, t ^ 0x90, prof);
    if ((t ^ 0x80) <= 0x0f) return lireObjet(r, v, t ^ 0x80, prof);
    switch (t) {
        case 0xc0: case 0xc1: case 0xd4: case 0xd5: case 0xd6: case 0xd7: v = Valeur::nul(); return true;
        case 0xc2: v = Valeur::booleen(false); return true;
        case 0xc3: v = Valeur::booleen(true); return true;
        case 0xca: { if (!r.u32(k)) return false; float f; std::memcpy(&f, &k, 4); v = Valeur::nombre(f); return true; }
        case 0xcb: {
            uint32_t h, l; if (!r.u32(h) || !r.u32(l)) return false;
            const uint64_t bits = (uint64_t)h << 32 | l; double x; std::memcpy(&x, &bits, 8); v = Valeur::nombre(x); return true;
        }
        case 0xcc: if (!r.u8(k)) return false; v = Valeur::nombre(k); return true;
        case 0xcd: if (!r.u16(k)) return false; v = Valeur::nombre(k); return true;
        case 0xce: if (!r.u32(k)) return false; v = Valeur::nombre(k); return true;
        case 0xd0: if (!r.u8(k)) return false; v = Valeur::nombre((int8_t)k); return true;
        case 0xd1: if (!r.u16(k)) return false; v = Valeur::nombre((int16_t)k); return true;
        case 0xd2: if (!r.u32(k)) return false; v = Valeur::nombre((int32_t)k); return true;
        case 0xcf: case 0xd3: {
            uint32_t h, l; if (!r.u32(h) || !r.u32(l)) return false;
            const uint64_t u = (uint64_t)h << 32 | l;
            v = Valeur::nombre(t == 0xcf ? (double)u : (double)(int64_t)u); return true;
        }
        case 0xd8: if (!r.u16(k)) return false; v = Valeur::texte(""); return r.chaine(k, v.s);
        case 0xd9: if (!r.u32(k)) return false; v = Valeur::texte(""); return r.chaine(k, v.s);
        case 0xda: if (!r.u16(k)) return false; v = Valeur::texte(""); return r.chaine(k, v.s);
        case 0xdb: if (!r.u32(k)) return false; v = Valeur::texte(""); return r.chaine(k, v.s);
        case 0xdc: if (!r.u16(k)) return false; return lireTableau(r, v, k, prof);
        case 0xdd: if (!r.u32(k)) return false; return lireTableau(r, v, k, prof);
        case 0xde: if (!r.u16(k)) return false; return lireObjet(r, v, k, prof);
        case 0xdf: if (!r.u32(k)) return false; return lireObjet(r, v, k, prof);
        default: return false;   // 0xc4 à 0xc9 : inconnus de BinaryPack
    }
}

} // namespace

std::vector<uint8_t> pack(const Valeur& v) { Ecrivain w; ecrire(w, v); return std::move(w.o); }

bool unpack(const uint8_t* data, size_t n, Valeur& out) {
    if (!data || n == 0 || n > 65536) return false;
    Lecteur r{data, n};
    Valeur v;
    if (!lireValeur(r, v, 0)) return false;
    out = std::move(v);
    return true;
}

} // namespace wfs::bp
