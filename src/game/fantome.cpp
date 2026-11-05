// WaForSpeed — fantôme de ton record (WFS-125) : voir fantome.h.
#include "fantome.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "binarypack.h"

namespace wfs::fantome {

static constexpr const char* MAGIE = "WFSF1";
static constexpr double PI = 3.14159265358979323846;

void Enregistreur::demarrer(const std::string& terrain, const std::string& car, unsigned peinture) {
    trace = Trace{}; trace.terrain = terrain; trace.car = car; trace.peinture = peinture; plein = false;
}

bool Enregistreur::pas(double t, const Echantillon& s) {
    if (plein) return false;
    while (t + 1e-9 >= trace.e.size() * PAS_S) {
        if (trace.e.size() >= MAX_ECHANTILLONS) { plein = true; return false; }
        trace.e.push_back(s);
    }
    return true;
}

bool interpoler(const Trace& tr, double t, Echantillon& o) {
    if (tr.e.empty() || t < 0 || t > tr.duree() + 1e-9) return false;
    const double k = t / PAS_S;
    const size_t i = std::min((size_t)k, tr.e.size() - 1), j = std::min(i + 1, tr.e.size() - 1);
    const double f = std::clamp(k - (double)i, 0.0, 1.0);
    const Echantillon &a = tr.e[i], &b = tr.e[j];
    double dh = b.h - a.h;   // cap : plus court chemin (passage de −π à π)
    if (dh > PI) dh -= 2 * PI; else if (dh < -PI) dh += 2 * PI;
    o = {a.d + (b.d - a.d) * f, a.x + (b.x - a.x) * f, a.h + dh * f, a.v + (b.v - a.v) * f};
    return true;
}

static void message(std::string& out, const bp::Valeur& v) {
    const std::vector<uint8_t> b = bp::pack(v);
    const uint32_t n = (uint32_t)b.size();
    for (int s = 24; s >= 0; s -= 8) out += (char)((n >> s) & 255);
    out.append((const char*)b.data(), b.size());
}

std::string encoder(const Trace& tr) {
    std::string out = MAGIE;
    bp::Valeur h = bp::Valeur::objet();
    h.ajoute("terrain", bp::Valeur::texte(tr.terrain));
    h.ajoute("car", bp::Valeur::texte(tr.car));
    h.ajoute("peinture", bp::Valeur::nombre(tr.peinture));
    h.ajoute("score", bp::Valeur::nombre((double)tr.score));
    const size_t n = std::min(tr.e.size(), MAX_ECHANTILLONS);
    h.ajoute("n", bp::Valeur::nombre((double)n));
    message(out, h);
    for (size_t a = 0; a < n; a += BLOC) {
        bp::Valeur t = bp::Valeur::tableau();
        for (size_t i = a; i < std::min(n, a + BLOC); i++) {
            const Echantillon& s = tr.e[i];
            for (double v : {s.d, s.x, s.h, s.v}) t.tab.push_back(bp::Valeur::nombre(v));
        }
        message(out, t);
    }
    return out;
}

bool decoder(const std::string& o, Trace& out) {
    size_t p = 5;
    if (o.size() < p || o.compare(0, p, MAGIE) != 0) return false;
    auto suivant = [&](bp::Valeur& v) {
        if (o.size() - p < 4) return false;
        uint32_t n = 0;
        for (int k = 0; k < 4; k++) n = n << 8 | (uint8_t)o[p + k];
        p += 4;
        if (n > o.size() - p) return false;
        const bool ok = bp::unpack((const uint8_t*)o.data() + p, n, v);
        p += n;
        return ok;
    };
    bp::Valeur h;
    if (!suivant(h) || h.type != bp::Valeur::Objet) return false;
    const bp::Valeur *te = h.champ("terrain"), *ca = h.champ("car"), *pe = h.champ("peinture"), *sc = h.champ("score"), *nn = h.champ("n");
    if (!te || te->type != bp::Valeur::Texte || !ca || ca->type != bp::Valeur::Texte || !pe || pe->type != bp::Valeur::Nombre ||
        !sc || sc->type != bp::Valeur::Nombre || !nn || nn->type != bp::Valeur::Nombre) return false;
    if (nn->n < 0 || nn->n > MAX_ECHANTILLONS || pe->n < 0 || pe->n > 0xffffff) return false;
    Trace tr; tr.terrain = te->s; tr.car = ca->s; tr.peinture = (unsigned)pe->n; tr.score = (long long)sc->n;
    const size_t n = (size_t)nn->n;
    tr.e.reserve(n);
    while (tr.e.size() < n) {
        bp::Valeur t;
        if (!suivant(t) || t.type != bp::Valeur::Tableau || t.tab.empty() || t.tab.size() % 4 || t.tab.size() / 4 > n - tr.e.size()) return false;
        for (size_t i = 0; i < t.tab.size(); i += 4) {
            for (size_t k = 0; k < 4; k++) if (t.tab[i + k].type != bp::Valeur::Nombre || !std::isfinite(t.tab[i + k].n)) return false;
            tr.e.push_back({t.tab[i].n, t.tab[i + 1].n, t.tab[i + 2].n, t.tab[i + 3].n});
        }
    }
    out = std::move(tr);
    return true;
}

std::string nomFichier(const std::string& terrain) { return "fantome-" + terrain + ".bin"; }

bool lireFichier(const std::string& chemin, Trace& out) {
    std::FILE* f = std::fopen(chemin.c_str(), "rb");
    if (!f) return false;
    std::string c; char buf[8192]; size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0 && c.size() < (1u << 21)) c.append(buf, n);   // 2 Mo au plus
    std::fclose(f);
    return decoder(c, out);
}

} // namespace wfs::fantome
