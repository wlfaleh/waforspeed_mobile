// WFS-416 : bâtiments du kit d'architecture cuits dans le paquet de la carte (src/world/archi_cuit.*).
//   RG-01 : la cuisson appelle le kit existant : le niveau 0 recopié du bloc a les mêmes triangles et la même enveloppe que le
//           bâtiment construit par le code avec les menuiseries réelles de WFS-402 ;
//   RG-02 : 3 niveaux par morceau de route, deux cuissons identiques à l'octet ;
//   RG-03 : lire puis recopier le bloc n'appelle jamais le kit (compteur de constructions = 0) ;
//   RG-04 : niveau 1 = une image avec relief par baie (rectangle, uv dans la tuile de son modèle), atlas dessiné (découpe du
//           vitrage, relief non plat), aucune menuiserie en volume ; niveau 2 sans image ;
//   RG-05 : un modèle de baie cuit une fois puis recopié (bien moins de modèles que de baies) ;
//   RG-06 : bloc abîmé, tronqué, d'une autre version, d'un autre plan, indice faux → refusé avec une raison, sans plantage ;
//   paquet : le bloc voyage dans le « .wfcarte » sans changer la version du format, un paquet sans ce bloc reste bon
//            (blocDuPaquet nul → kit de secours).
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include "../src/core/carte_cuite.h"
#include "../src/world/archi_cuit.h"
using namespace wfs;
using namespace wfs::archi;

static int g_echecs = 0;
#define VERIFIER(c) do { if (!(c)) { std::printf("ÉCHEC ligne %d : %s\n", __LINE__, #c); g_echecs++; } } while (0)

// une petite rue : 9 bâtiments de 3 styles, des deux côtés, sur 3 morceaux de route
static std::vector<cuit::Bati> rue() {
    std::vector<cuit::Bati> b;
    const Style styles[3] = {Style::Classique, Style::Moderne, Style::Industriel};
    for (int k = 0; k < 9; k++) {
        const float cote = k % 2 ? 1.f : -1.f;
        const vec3 x{0, 0, -cote}, z{-cote, 0, 0};   // façade le long de la route (axe z du monde), tournée vers elle
        const Style s = styles[k % 3];
        b.push_back({(int64_t)(k * 20), vec3{cote * 14.f, 0.f, -(float)k * 24.f}, x, z, 12.f, assembler(s, 22.f + (float)k, s == Style::Industriel ? 9.f : 21.f, 77u + (uint32_t)k)});
    }
    return b;
}
static std::shared_ptr<const std::vector<uint8_t>> partage(std::vector<uint8_t> o) { return std::make_shared<const std::vector<uint8_t>>(std::move(o)); }

int main() {
    const std::vector<cuit::Bati> bats = rue();
    const uint64_t sig = cuit::signature(bats);

    // ---- RG-02 : cuisson, 3 niveaux, déterministe
    const cuit::Decor d = cuit::cuire(bats);
    const std::vector<uint8_t> octets = cuit::ecrire(d);
    VERIFIER(cuit::ecrire(cuit::cuire(bats)) == octets);
    VERIFIER(d.signature == sig && d.batis.size() == bats.size());
    VERIFIER(d.morceaux.size() == 4);   // segments 0 à 160 → morceaux 0, 1, 2 et 3
    size_t baies = 0;
    for (const cuit::Bati& b : bats) baies += b.plan.baies.size();
    size_t instances = 0, images = 0;
    for (const cuit::Morceau& m : d.morceaux) {
        instances += m.instances.size();
        images += m.lot[1][cuit::S_IMAGE].i.size() / 6;
        for (int n = 0; n < cuit::NIVEAUX; n++) { size_t t = 0; for (const cuit::Lot& l : m.lot[n]) t += l.i.size(); VERIFIER(t > 0); }
        VERIFIER(m.rayon > 5 && m.rayon < 200);
        // RG-04 : aucune menuiserie en volume au niveau 1 (châssis), aucune image aux niveaux 0 et 2
        VERIFIER(m.lot[1][cuit::S_CHASSIS].i.empty());
        VERIFIER(m.lot[0][cuit::S_IMAGE].i.empty() && m.lot[2][cuit::S_IMAGE].i.empty());
    }
    VERIFIER(instances == baies);   // chaque baie du niveau 0 est une instance
    VERIFIER(images == baies);      // chaque baie du niveau 1 a son image avec relief
    // RG-05 : un modèle par sorte de baie, recopié
    std::printf("%zu baies, %zu modèles cuits (recopie ×%.1f), bloc de %.2f Mo\n", baies, d.modeles.size(), (double)baies / (double)d.modeles.size(), octets.size() / 1048576.0);
    VERIFIER(d.modeles.size() * 3 < baies);

    // ---- RG-04 : atlas (une tuile par modèle présent), uv des images dans leur tuile, vitrage découpé, relief non plat
    VERIFIER(d.atlas.l == cuit::TUILE_L * cuit::COLONNES && d.atlas.h >= cuit::TUILE_H && !d.atlas.tuiles.empty());
    VERIFIER(d.atlas.couleur.size() == (size_t)d.atlas.l * d.atlas.h * 4 && d.atlas.relief.size() == d.atlas.couleur.size());
    {
        size_t jour = 0, plein = 0, pente = 0;
        const size_t nt = d.atlas.tuiles.size();
        for (uint32_t y = 0; y < d.atlas.h; y++) for (uint32_t x = 0; x < d.atlas.l; x++) {
            const size_t t = (size_t)(y / cuit::TUILE_H) * cuit::COLONNES + x / cuit::TUILE_L, q = ((size_t)y * d.atlas.l + x) * 4;
            if (t >= nt) continue;
            if (d.atlas.couleur[q + 3] < 128) jour++; else plein++;
            if (std::abs((int)d.atlas.relief[q] - 128) > 24 || std::abs((int)d.atlas.relief[q + 1] - 128) > 24) pente++;
        }
        std::printf("atlas %ux%u, %zu tuiles : vitrage %.0f %%, menuiserie %.0f %%, pixels en relief %.1f %%\n", d.atlas.l, d.atlas.h, nt,
                    100.0 * jour / (jour + plein), 100.0 * plein / (jour + plein), 100.0 * pente / (jour + plein));
        VERIFIER(jour > (jour + plein) / 10 && plein > (jour + plein) / 10);   // du vitrage ET du cadre
        VERIFIER(pente > (jour + plein) / 100);                                 // le relief existe (bords des cadres)
        for (const cuit::Morceau& m : d.morceaux) for (const cuit::Sommet& s : m.lot[1][cuit::S_IMAGE].v) VERIFIER(s.u > 0 && s.u < 1 && s.v > 0 && s.v < 1);
    }

    // ---- lecture sans copie ; RG-03 : aucune construction pour lire ni pour recopier
    const size_t avant = archi::constructions();
    cuit::Lu lu;
    const char* raison = nullptr;
    VERIFIER(cuit::lire(partage(octets), sig, lu, &raison));
    VERIFIER(lu.morceaux.size() == d.morceaux.size() && lu.modeles.size() == d.modeles.size() && lu.instances.size() == instances);
    VERIFIER(lu.atlasL == d.atlas.l && lu.atlasH == d.atlas.h);
    VERIFIER(!std::memcmp(lu.octets->data() + lu.posCouleur, d.atlas.couleur.data(), d.atlas.couleur.size()));
    VERIFIER(!std::memcmp(lu.octets->data() + lu.posRelief, d.atlas.relief.data(), d.atlas.relief.size()));
    // niveau 0 recopié du bloc (gros œuvre + instances), comparé au bâtiment construit par le code, morceau par morceau
    size_t triCuits[3] = {}, triKit0 = 0;
    for (size_t k = 0; k < lu.morceaux.size(); k++) {
        const cuit::MorceauLu& m = lu.morceaux[k];
        for (int s = 0; s < cuit::NB_SLOTS; s++) {
            std::vector<cuit::Sommet> v(m.nv0[s]); std::vector<uint32_t> i(m.ni0[s]);
            uint32_t nv = 0, ni = 0;
            const cuit::Vue& base = m.lot[0][s];
            lu.copier(base, v.data(), i.data(), 0); nv += base.nv; ni += base.ni;
            for (size_t q = 0; q < m.nbInstances; q++) {
                const cuit::Vue l = lu.developper(lu.instances[m.premiereInstance + q], s, v.data() + nv, i.data() + ni, nv);
                nv += l.nv; ni += l.ni;
            }
            VERIFIER(nv == m.nv0[s] && ni == m.ni0[s]);
            VERIFIER(m.nv0[s] <= lu.nvMax[s] && m.ni0[s] <= lu.niMax[s]);   // tient dans les tampons réservés
            for (uint32_t x : i) if (x >= nv) { VERIFIER(x < nv); break; }
            // même géométrie que le kit : construit ici par le code (hors du comptage de la lecture), même emplacement
            cuit::Lot kit[cuit::NB_SLOTS];
            const size_t c0 = archi::constructions();
            for (const cuit::Bati& b : bats) if (b.seg / cuit::MORCEAU == m.c) cuit::ajouter(b, 0, Baies::Reelles, kit);
            const size_t construits = archi::constructions() - c0;
            VERIFIER(construits > 0);
            VERIFIER(kit[s].i.size() == ni && kit[s].v.size() == nv);
            // même enveloppe et même somme des positions à 1 mm près (la recopie place chaque modèle au bon endroit)
            double sa[3] = {}, sb[3] = {}; float la[3] = {1e9f, 1e9f, 1e9f}, lb[3] = {1e9f, 1e9f, 1e9f}, ha[3] = {-1e9f, -1e9f, -1e9f}, hb[3] = {-1e9f, -1e9f, -1e9f};
            auto cumul = [](const std::vector<cuit::Sommet>& w, size_t n, double* sm, float* lo, float* hi) {
                for (size_t q = 0; q < n; q++) { const float p[3] = {w[q].px, w[q].py, w[q].pz}; for (int a = 0; a < 3; a++) { sm[a] += p[a]; lo[a] = std::min(lo[a], p[a]); hi[a] = std::max(hi[a], p[a]); } }
            };
            cumul(v, nv, sa, la, ha); cumul(kit[s].v, kit[s].v.size(), sb, lb, hb);
            for (int a = 0; a < 3; a++) if (nv) {
                VERIFIER(std::fabs(sa[a] - sb[a]) / nv < 1e-3);
                VERIFIER(std::fabs(la[a] - lb[a]) < 1e-3f && std::fabs(ha[a] - hb[a]) < 1e-3f);
            }
            triCuits[0] += ni / 3; triKit0 += kit[s].i.size() / 3;
            // le comptage « lecture » ne doit pas voir les constructions de contrôle ci-dessus
            (void)construits;
        }
        for (int n = 1; n < cuit::NIVEAUX; n++) for (int s = 0; s < cuit::NB_SLOTS; s++) triCuits[n] += m.lot[n][s].ni / 3;
    }
    VERIFIER(triCuits[0] == triKit0 && lu.triangles(0) == triCuits[0]);
    VERIFIER(triCuits[0] > 4 * triCuits[1] && triCuits[1] > 2 * triCuits[2]);   // 3 niveaux de plus en plus légers
    std::printf("triangles par niveau : %zu / %zu / %zu\n", triCuits[0], triCuits[1], triCuits[2]);
    {   // RG-03 : lire et recopier seuls (sans le contrôle contre le kit) : zéro construction
        const size_t c0 = archi::constructions();
        cuit::Lu l2;
        VERIFIER(cuit::lire(partage(octets), sig, l2));
        for (const cuit::MorceauLu& m : l2.morceaux) for (int s = 0; s < cuit::NB_SLOTS; s++) {
            std::vector<cuit::Sommet> v(m.nv0[s]); std::vector<uint32_t> i(m.ni0[s]);
            uint32_t nv = m.lot[0][s].nv, ni = m.lot[0][s].ni;
            l2.copier(m.lot[0][s], v.data(), i.data(), 0);
            for (size_t q = 0; q < m.nbInstances; q++) { const cuit::Vue l = l2.developper(l2.instances[m.premiereInstance + q], s, v.data() + nv, i.data() + ni, nv); nv += l.nv; ni += l.ni; }
        }
        VERIFIER(archi::constructions() == c0);
    }
    (void)avant;

    // ---- RG-06 : blocs refusés, toujours avec une raison, jamais de plantage
    {
        cuit::Lu x; const char* r = nullptr;
        VERIFIER(!cuit::lire(nullptr, sig, x, &r) && r);
        r = nullptr; VERIFIER(!cuit::lire(partage({}), sig, x, &r) && r);
        r = nullptr; VERIFIER(!cuit::lire(partage(octets), sig ^ 1, x, &r) && r && std::strstr(r, "autre plan"));
        std::vector<uint8_t> o = octets; o[0] = 'X';
        r = nullptr; VERIFIER(!cuit::lire(partage(o), sig, x, &r) && r);
        o = octets; o[4] = (uint8_t)(cuit::VERSION_BLOC + 1);
        r = nullptr; VERIFIER(!cuit::lire(partage(o), sig, x, &r) && r && std::strstr(r, "version"));
        o = octets; o.push_back(0);
        r = nullptr; VERIFIER(!cuit::lire(partage(o), sig, x, &r) && r);
        for (size_t n : {octets.size() / 7, octets.size() / 2, octets.size() - 1, (size_t)40, (size_t)3}) {   // tronqués
            o.assign(octets.begin(), octets.begin() + (long)n);
            r = nullptr; VERIFIER(!cuit::lire(partage(o), sig, x, &r) && r);
            VERIFIER(x.morceaux.empty() && !x.octets);
        }
        // un indice de sommet hors de son lot (dernier indice du bloc : dernière instance, donc dernier lot avant elle)
        cuit::Lu bon; VERIFIER(cuit::lire(partage(octets), sig, bon));
        const cuit::Vue* cible = nullptr;
        for (const auto& m : bon.modeles) for (const cuit::Vue& l : m) if (l.ni) cible = &l;
        VERIFIER(cible);
        if (cible) {
            o = octets;
            const size_t pos = cible->pos + (size_t)cible->nv * sizeof(cuit::Sommet);
            o[pos] = 0xFF; o[pos + 1] = 0xFF; o[pos + 2] = 0xFF; o[pos + 3] = 0x7F;
            r = nullptr; VERIFIER(!cuit::lire(partage(o), sig, x, &r) && r);
        }
        // octets au hasard (graine fixe) dans les 200 premiers octets : refusé ou lu, jamais de plantage
        uint32_t g = 12345u;
        for (int k = 0; k < 200; k++) {
            o = octets; g = g * 1664525u + 1013904223u;
            o[(g >> 8) % 200] ^= (uint8_t)(1u << (g % 8));
            cuit::Lu y; cuit::lire(partage(o), sig, y);
        }
    }

    // ---- signature : nombres entiers du plan seulement (la même sur le PC qui cuit et sur le téléphone)
    {
        std::vector<cuit::Bati> b2 = bats;
        b2[0].o.x += 0.001f;                        // un bâtiment déplacé d'un millimètre (bruit de calcul) : même signature
        VERIFIER(cuit::signature(b2) == sig);
        b2 = bats; b2[3].plan.graine ^= 1;          // autre bâtiment : autre signature
        VERIFIER(cuit::signature(b2) != sig);
        b2 = bats; b2.pop_back();
        VERIFIER(cuit::signature(b2) != sig);
    }

    // ---- paquet : le bloc voyage dans le « .wfcarte » ; version du format inchangée ; paquet sans le bloc toujours bon
    {
        VERIFIER(carte::VERSION_FORMAT == 1);
        carte::Paquet p; p.carte = "ville"; p.format = carte::FormatTexture::RGBA8; p.empreinte = 42;
        p.blocs = {{"decor_regroupe", {}}, {"maillages", carte::archive({})}, {"textures", carte::archive({})}, {cuit::NOM_BLOC, octets}};
        carte::Paquet relu;
        VERIFIER(carte::lire(carte::ecrire(p), {"ville", carte::FormatTexture::RGBA8, 42}, &relu) == carte::Verdict::Bon);
        carte::activer(std::move(relu));
        VERIFIER(!carte::blocDuPaquet("plage", cuit::NOM_BLOC));   // autre carte : rien
        const auto b = carte::blocDuPaquet("ville", cuit::NOM_BLOC);
        VERIFIER(b && *b == octets);
        cuit::Lu y; VERIFIER(cuit::lire(b, sig, y));
        carte::desactiver();
        VERIFIER(!carte::blocDuPaquet("ville", cuit::NOM_BLOC));
        VERIFIER(y.octets && y.octets->size() == octets.size());   // le bloc lu reste valable après le changement de paquet
        p.blocs.pop_back();                                        // paquet d'avant WFS-416 : bon, sans bloc → kit de secours
        VERIFIER(carte::lire(carte::ecrire(p), {"ville", carte::FormatTexture::RGBA8, 42}, &relu) == carte::Verdict::Bon);
        carte::activer(std::move(relu));
        VERIFIER(carte::actif() && !carte::blocDuPaquet("ville", cuit::NOM_BLOC));
        carte::desactiver();
    }

    if (g_echecs) { std::printf("test_batiments_cuits : %d échec(s)\n", g_echecs); return 1; }
    std::printf("test_batiments_cuits : OK\n");
    return 0;
}
