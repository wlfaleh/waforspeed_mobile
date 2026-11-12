// WaForSpeed — WFS-161 (ST-1) : géométrie du kit d'architecture (src/world/archi_geo.h).
// Sens des triangles (normale = sens de l'enroulement, sinon scene.frag retourne la normale et l'éclairage s'inverse),
// vrais trous dans la façade (aire du mur = façade − ouvertures), M-01 sur la géométrie (écart entre vitres de deux
// niveaux = hauteur d'étage à ± 1 %), garde-corps à la hauteur du référentiel, niveaux de détail de moins en moins chers.
#include "../src/world/archi_geo.h"
#include "../src/world/archi_ouvertures.h"   // WFS-402
#include "../src/render/bench_realisme.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

using namespace wfs::archi;
using wfs::vec3;

static int echecs = 0, controles = 0;
static void verifierQue(bool ok, const char* quoi) { controles++; if (!ok) { if (echecs < 15) std::printf("[ÉCHEC] %s\n", quoi); echecs++; } }

// ---------------------------------------------------------------- WFS-402 : fenêtres et portes réalistes
static void verifierWFS402(Style s) {
    namespace o = ouvertures;
    bool fen[4] = {}, por[4] = {};
    int volets = 0, fermes = 0;
    for (uint32_t g = 0; g < 400; g++) {
        const float largeur = 6.f + (float)((g * 37) % 300) / 10.f, hauteur = 6.f + (float)((g * 53) % 220) / 10.f;
        const Plan p = assembler(s, largeur, hauteur, g), q = assembler(s, largeur, hauteur, g + 1);
        fen[o::indiceModeleFenetre(p)] = por[o::indiceModelePorte(p)] = true;
        // RG-05 : deux bâtiments voisins (graines qui se suivent) n'ont jamais la même façade
        bool pareil = o::indiceModeleFenetre(p) == o::indiceModeleFenetre(q) && o::indiceModelePorte(p) == o::indiceModelePorte(q) && p.baies.size() == q.baies.size();
        for (size_t i = 0; pareil && i < p.baies.size(); i++) pareil = std::fabs(p.baies[i].l - q.baies[i].l) < 1e-4f && std::fabs(p.baies[i].h - q.baies[i].h) < 1e-4f;
        verifierQue(!pareil, "RG-05 : deux bâtiments voisins avec la même façade");
        // RG-02 : retrait du tableau de 15 à 25 cm (hors loggias)
        verifierQue(regles(s).retrait >= 0.15f - 1e-4f && regles(s).retrait <= 0.25f + 1e-4f, "RG-02 : retrait de la fenêtre de 15 à 25 cm");
        int portes = 0;
        for (const Baie& b : p.baies) {
            if (b.type == Ouverture::Porte) {   // RG-04 : porte d'entrée côté rue, aux bonnes mesures
                portes++;
                if (s == Style::Village || s == Style::Villa) {
                    verifierQue(std::fabs(b.l - 0.9f) <= 0.01f * 0.9f && std::fabs(b.h - 2.15f) <= 0.01f * 2.15f, "RG-04 : porte de maison 0,90 × 2,15 m (± 1 %)");
                    verifierQue(std::fabs((b.l - 2 * 0.035f) - 0.83f) <= 0.01f * 0.83f, "RG-04 : passage utile de 0,83 m (référentiel)");
                } else if (p.rdc == Rdc::Porche) verifierQue(b.l <= 2.5f + 1e-4f && b.h <= 4.f, "RG-04 : porte cochère au plus 2,5 × 4,0 m");
                else verifierQue(b.l >= 1.f - 1e-4f && b.l <= 1.4f + 1e-4f && b.h >= 2.2f - 1e-4f && b.h <= 3.f, "RG-04 : porte d'immeuble 1,0-1,4 × 2,2-3,0 m");
            }
            if ((b.type == Ouverture::Fenetre || b.type == Ouverture::PorteFenetre) && b.niveau > 0) {   // RG-02 : mesures courantes
                if (b.type == Ouverture::PorteFenetre) verifierQue(b.h >= 2.15f - 1e-4f, "RG-02 : porte-fenêtre d'au moins 2,15 m");
                else if (s != Style::Industriel) verifierQue(b.l >= 0.8f - 1e-4f && b.l <= 1.4f + 1e-4f && b.h >= 1.15f - 1e-4f && b.h <= 2.15f + 1e-4f, "RG-02 : fenêtre d'habitation 0,80-1,40 × 1,15-2,15 m");
            }
            if (o::voletDuStyle(s) != o::Volet::Aucun && (b.type == Ouverture::Fenetre || b.type == Ouverture::PorteFenetre)) {
                volets++; fermes += o::etatVolet(p, b) != o::EtatVolet::Ouvert;
            }
        }
        verifierQue(portes >= 1, "RG-04 : au moins une porte d'entrée par bâtiment");
        if (g < 40) {   // RG-02, RG-04, RG-07 : chaque baie a sa menuiserie au niveau 0 (cadres, vantaux, béquille)
            Batiment b; construire(p, 10.f, 0, b);
            for (const Baie& x : p.baies) {
                const int part = x.type == Ouverture::Porte ? (o::modelePorte(p).bois ? MENUISERIE : METAL)
                               : x.type == Ouverture::Vitrine ? METAL : o::partChassis(s);
                int n = 0;
                for (const Sommet& v : b.part[part].v) n += v.p.x > x.x - x.l / 2 - 1e-3f && v.p.x < x.x + x.l / 2 + 1e-3f && v.p.y > x.y - 0.01f && v.p.y < x.y + x.h + 0.01f && v.p.z < 0.01f;
                verifierQue(n >= 24, "RG-07 : menuiserie modélisée dans chaque baie (dormant, ouvrants ou vantail)");
            }
        }
    }
    for (int k = 0; k < 4; k++) verifierQue(fen[k] && por[k], "RG-05 : les 4 modèles de fenêtre et de porte du style sont tirés");
    if (volets) {
        const double f = 100.0 * fermes / volets;
        std::printf("[%s] %-14s : volets fermés ou entrouverts %.1f %% (RG-03 : 10 à 30 %%)\n", f >= 10 && f <= 30 ? "OK" : "ÉCHEC", regles(s).nom, f);
        verifierQue(f >= 10 && f <= 30, "RG-03 : 10 à 30 % de volets fermés ou entrouverts");
    }
}

// WFS-402 : baie complète posée hors du plan d'un bâtiment (fonction publique pour WFS-410 et WFS-411)
static void verifierBaieComplete() {
    namespace o = ouvertures;
    const Ouverture types[] = {Ouverture::Fenetre, Ouverture::PorteFenetre, Ouverture::Porte, Ouverture::Vitrine, Ouverture::PorteGarage};
    for (int s = 0; s < (int)Style::Nombre; s++) for (Ouverture t : types) for (uint32_t g = 0; g < 8; g++) {
        const float plancher = 3.4f, x = 5.f, l = t == Ouverture::Vitrine ? 3.f : t == Ouverture::PorteGarage ? 2.4f : 1.1f;
        const float h = t == Ouverture::Fenetre ? 1.4f : 2.2f, y = plancher + (t == Ouverture::Fenetre ? 0.9f : 0.f);
        Batiment b;
        o::baieComplete((Style)s, g, t, x, y, l, h, plancher, 0.8f, 0, b);
        size_t tris = 0;
        for (int k = 0; k < NB_PARTS; k++) {
            const Maillage& m = b.part[k];
            tris += m.triangles();
            for (size_t i = 0; i + 2 < m.i.size(); i += 3) {
                const Sommet &A = m.v[m.i[i]], &B = m.v[m.i[i + 1]], &C = m.v[m.i[i + 2]];
                const vec3 c = cross(B.p - A.p, C.p - A.p);
                const float aire = length(c);
                verifierQue(aire > 1e-8f && dot(c * (1.f / aire), A.n) > 0.99f, "baie complète : normale dans le sens de l'enroulement");
            }
            for (const Sommet& v : m.v) {
                verifierQue(v.p.y > y - 0.2f && v.p.y < y + h + 0.3f, "baie complète : posée à la hauteur demandée");
                verifierQue(v.p.x > x - l / 2 - 0.85f && v.p.x < x + l / 2 + 0.85f, "baie complète : dans la baie et son mur libre");
            }
        }
        const int part = t == Ouverture::Porte ? (o::modelePorte(Plan{(Style)s, g}).bois ? MENUISERIE : METAL)
                       : t == Ouverture::Vitrine ? METAL : t == Ouverture::PorteGarage ? o::partChassis((Style)s) : o::partChassis((Style)s);
        verifierQue(b.part[part].triangles() >= 12, "baie complète : menuiserie modélisée");
        verifierQue(tris >= 40, "baie complète : tableau, fond et menuiserie");
        // poignée de fenêtre ou béquille de porte entre 0,9 et 1,3 m au-dessus du plancher
        if (t == Ouverture::Fenetre || (t == Ouverture::Porte && o::modelePorte(Plan{(Style)s, g}).vantail == o::Vantail::Panneaux)) {
            bool poignee = false;
            for (int k : {CHASSIS, METAL}) for (const Sommet& v : b.part[k].v) poignee |= v.p.z > 0.0f - 0.3f && v.p.y > plancher + 0.85f && v.p.y < plancher + 1.3f;
            verifierQue(poignee, "baie complète : poignée à hauteur d'homme au-dessus du plancher");
        }
    }
}

// WFS-415 (RG-02, RG-03) : interrupteur des ouvertures à 0 (jeu) : baie d'avant WFS-402, trou en retrait et vitre ou porte au
// fond, sans menuiserie, volet ni appui ; moins de triangles qu'avec l'interrupteur à 1, et rien de changé à 1
static void verifierInterrupteurWFS415() {
    for (int s = 0; s < (int)Style::Nombre; s++) for (uint32_t g = 0; g < 12; g++) {
        const Plan p = assembler((Style)s, 8.f + g, 9.f + g % 5, g);
        size_t vitrees = 0;
        for (const Baie& x : p.baies) vitrees += x.type == Ouverture::Fenetre || x.type == Ouverture::PorteFenetre || x.type == Ouverture::Vitrine;
        for (int lod = 0; lod < 2; lod++) {
            Batiment reel, avant, reel2;
            choisirOuverturesReelles(true);  construire(p, 10.f, lod, reel);
            choisirOuverturesReelles(false); construire(p, 10.f, lod, avant);
            choisirOuverturesReelles(true);  construire(p, 10.f, lod, reel2);
            size_t tR = 0, tA = 0;
            for (int k = 0; k < NB_PARTS; k++) {
                tR += reel.part[k].triangles(); tA += avant.part[k].triangles();
                verifierQue(reel.part[k].v.size() == reel2.part[k].v.size() && reel.part[k].i == reel2.part[k].i, "WFS-415 : interrupteur à 1 : maillage de WFS-402 identique");
            }
            verifierQue(avant.part[CHASSIS].triangles() == 0 && avant.part[VOLET].triangles() == 0, "WFS-415 : interrupteur à 0 : ni châssis ni volet");
            verifierQue(avant.part[VITRE].triangles() >= 2 * vitrees, "WFS-415 : interrupteur à 0 : une vitre au fond de chaque baie vitrée (et des lucarnes)");
            verifierQue(tA < tR, "WFS-415 : interrupteur à 0 : moins de triangles qu'avec les ouvertures de WFS-402");
        }
    }
    choisirOuverturesReelles(true);
}

// ---------------------------------------------------------------- WFS-455 (ANO-254) : plus de pavé gris nu vu de l'arrière
static void verifierWFS455() {
    const int avant = echecs;
    for (int s = 0; s < (int)Style::Nombre; s++) for (uint32_t g = 0; g < 20; g++) {
        const Plan p = assembler((Style)s, 12.f + (float)(g % 5) * 3.f, 22.f, g * 7919u + 3u);
        const float D = 10.f + (float)(g % 4) * 3.f;
        for (int f = 1; f <= 3; f++) {
            const Plan q = facadeSecondaire(p, f == 1 ? p.largeur : D, f);
            const char* e = nullptr;
            verifierQue(verifier(q, &e) == 0, e ? e : "WFS-455 : façade secondaire dans les règles du kit");
            verifierQue(q.hRdc == p.hRdc && q.hEtage == p.hEtage && q.etages == p.etages, "WFS-455 : mêmes niveaux que la rue");
            int fen = 0; for (const Baie& b : q.baies) fen += b.type == Ouverture::Fenetre && !b.balcon;
            verifierQue(fen >= q.travees * q.etages, "WFS-455 : une fenêtre par travée et par étage");
        }
        for (int lod = 0; lod < 3; lod++) {
            Batiment b;
            construire(p, D, lod, b);
            // vitres de chaque face (normale −z : arrière ; ±x : côtés), au fond du tableau (≤ 0,5 m du nu) ou peintes sur le mur
            int arriere = 0, gauche = 0, droite = 0;
            const Maillage& vi = b.part[VITRE];
            for (size_t i = 0; i + 2 < vi.i.size(); i += 3) {
                const Sommet& A = vi.v[vi.i[i]];
                if (A.p.y > p.hauteur()) continue;   // lucarnes
                if (A.n.z < -0.99f && A.p.z < -D + 0.5f) arriere++;
                if (A.n.x < -0.99f && A.p.x < 0.5f) gauche++;
                if (A.n.x > 0.99f && A.p.x > p.largeur - 0.5f) droite++;
            }
            verifierQue(arriere >= 2 * p.etages, "WFS-455 : fenêtres sur la façade arrière, à chaque niveau de détail");
            verifierQue(gauche >= 2 * p.etages && droite >= 2 * p.etages, "WFS-455 : fenêtres sur les deux côtés, à chaque niveau de détail");
            // sens des triangles des murs tournés : normale dans le sens de l'enroulement (sinon l'éclairage s'inverse)
            for (int k = 0; k < NB_PARTS; k++) {
                const Maillage& m = b.part[k];
                for (size_t i = 0; i + 2 < m.i.size(); i += 3) {
                    const Sommet &A = m.v[m.i[i]], &B = m.v[m.i[i + 1]], &C = m.v[m.i[i + 2]];
                    const vec3 c = cross(B.p - A.p, C.p - A.p);
                    const float aire = length(c);
                    if (aire > 1e-6f && std::fabs(A.n.x) + std::fabs(A.n.z) > 0.99f && std::fabs(A.n.y) < 1e-3f)
                        verifierQue(dot(c * (1.f / aire), A.n) > 0.99f, "WFS-455 : murs et baies tournés gardent leur sens");
                }
            }
            if (p.toit == Toit::Terrasse && p.edicules && p.largeur > 6 && D > 6) {
                float haut = 0; for (const Sommet& v : b.part[MUR].v) haut = std::max(haut, v.p.y);
                verifierQue(haut > p.hauteur() + 2.5f, "WFS-455 : édicule de la terrasse visible à chaque niveau de détail");
            }
        }
    }
    std::printf("[%s] WFS-455 : façades arrière et latérales percées contrôlées\n", echecs == avant ? "OK" : "ÉCHEC");
}

int main() {
    verifierWFS455();
    choisirOuverturesReelles(true);   // WFS-415 (RG-02) : contrôles de WFS-402 avec l'interrupteur à 1
    verifierInterrupteurWFS415();
    verifierBaieComplete();
    wfs::reel::Referentiel R;
    verifierQue(R.charger(std::string(WFS_SOURCE_DIR) + "/references/reel"), "référentiel lisible");
    const wfs::reel::Json* gc = R.mesure("archi.garde_corps.hauteur");
    const double gcRef = gc ? gc->nombre("valeur", 1.0) : 1.0;
    size_t trisMax[3] = {};
    for (int s = 0; s < (int)Style::Nombre; s++) {
        size_t tris[3] = {};
        for (uint32_t g = 0; g < 60; g++) {
            const float largeur = 6.f + (float)((g * 37) % 300) / 10.f, hauteur = 6.f + (float)((g * 53) % 220) / 10.f, D = 8.f + g % 10;
            const Plan p = assembler((Style)s, largeur, hauteur, g);
            for (int lod = 0; lod < 3; lod++) {
                Batiment b;
                construire(p, D, lod, b);
                size_t t = 0;
                for (int k = 0; k < NB_PARTS; k++) {
                    const Maillage& m = b.part[k];
                    t += m.triangles();
                    for (size_t i = 0; i + 2 < m.i.size(); i += 3) {
                        const Sommet &A = m.v[m.i[i]], &B = m.v[m.i[i + 1]], &C = m.v[m.i[i + 2]];
                        const vec3 c = cross(B.p - A.p, C.p - A.p);
                        const float aire = length(c);
                        verifierQue(std::isfinite(aire) && aire > 1e-8f, "triangle non vide et fini");
                        if (aire > 1e-8f) verifierQue(dot(c * (1.f / aire), A.n) > 0.99f, "normale dans le sens de l'enroulement");
                        if (k == VITRE) {   // ST-3 : uv = données de la baie (largeur, allège, hauteur en dm) + position relative
                            const float l = std::floor(A.uv.x / 100) * 0.1f, h = std::fmod(std::floor(A.uv.y), 100.f) * 0.1f;
                            const float fu = A.uv.x - std::floor(A.uv.x), fv = A.uv.y - std::floor(A.uv.y);
                            verifierQue(l > 0.3f && l < 9.95f && h > 0.3f && h < 9.95f, "uv de vitre : largeur et hauteur de la baie lisibles");
                            verifierQue(fu >= 0 && fu < 1 && fv >= 0 && fv < 1, "uv de vitre : position relative dans 0..1");
                            verifierQue(A.uv.y >= 100.f && A.uv.y < 10100.f, "WFS-370 : graine de la baie (1 à 100) codée dans v");
                        }
                    }
                }
                tris[lod] += t;
                // façade : vrais trous (lod 0 et 1), mur plein peint (lod 2)
                double ouvertures = 0;
                for (const Baie& x : p.baies) ouvertures += (double)x.l * x.h;
                const double attendu = (double)p.largeur * p.hauteur() - (lod < 2 ? ouvertures : 0);
                const double aire = aireFacade(b.part[MUR], p.hauteur());
                verifierQue(std::fabs(aire - attendu) <= 1e-3 * attendu, "aire du mur de façade = façade − ouvertures");
                if (lod == 0) {
                    // ST-3 : chaque vitre porte les dimensions de sa baie (± 5 cm, arrondi au décimètre)
                    const Maillage& vi = b.part[VITRE];
                    for (const Sommet& v : vi.v) {
                        if (v.n.z < 0.99f) continue;   // WFS-455 : vitres de la façade sur rue seulement (arrière et côtés : verifierWFS455)
                        const float l = std::floor(v.uv.x / 100) * 0.1f, h = std::fmod(std::floor(v.uv.y), 100.f) * 0.1f;
                        bool trouve = false;
                        if (v.p.y > p.hauteur()) {   // WFS-370 : fenêtre de lucarne, dans le brisis, dans l'axe d'une travée
                            for (int t = 0; t < p.travees; t++) trouve |= std::fabs(v.p.x - p.axe(t)) <= l / 2 + 0.051f;
                            verifierQue(p.toit == Toit::Zinc && v.p.y < p.hauteur() + p.hEtage && l >= 0.55f && l <= 1.35f && h >= 0.55f && h <= 1.35f,
                                        "WFS-370 : lucarne dans le brisis du comble, fenêtre de 0,6 à 1,3 m");
                        }
                        for (const Baie& x : p.baies) trouve |= std::fabs(x.l - l) <= 0.051f && std::fabs(x.h - h) <= 0.051f && std::fabs(v.p.x - x.x) <= x.l / 2 + 1e-3f;
                        verifierQue(trouve, "ST-3 : dimensions codées dans la vitre = celles de sa baie");
                    }
                    // ST-2 : volets dans la façade, jamais devant une ouverture
                    const Maillage& vo = b.part[VOLET];
                    for (size_t i = 0; i + 2 < vo.i.size(); i += 3) {
                        const vec3 c = (vo.v[vo.i[i]].p + vo.v[vo.i[i + 1]].p + vo.v[vo.i[i + 2]].p) * (1.f / 3);
                        verifierQue(c.x > 0 && c.x < p.largeur, "volet dans la façade");
                        for (const Baie& x : p.baies)   // WFS-402 (RG-03) : seuls les volets fermés ou entrouverts passent devant leur fenêtre
                            if (c.x > x.x - x.l / 2 + 0.005f && c.x < x.x + x.l / 2 - 0.005f && c.y > x.y && c.y < x.y + x.h)   // (tranche du vantail ouvert sur le gond : au bord, pas devant)
                                verifierQue(ouvertures::voletsBattants(p, x) && ouvertures::etatVolet(p, x) != ouvertures::EtatVolet::Ouvert, "volet ouvert devant une ouverture");
                    }
                    if (p.style == Style::Mediterraneen) {
                        bool fen = false; for (const Baie& x : p.baies) fen |= x.type == Ouverture::Fenetre;
                        if (fen && p.travee >= 2.6f) verifierQue(vo.triangles() > 0, "ST-2 : volets du méditerranéen");
                    }
                    if (p.style == Style::Moderne && p.etages > 0) {   // bandeaux : nez de dalle en saillie de 0,1 m
                        bool bandeau = false; for (const Sommet& v : b.part[ENCADREMENT].v) bandeau |= std::fabs(v.p.z - 0.1f) < 1e-4f && v.p.x < 1e-4f;
                        verifierQue(bandeau, "ST-2 : bandeaux du moderne");
                    }
                    if (p.style == Style::Classique && p.etages >= 4) {   // balcons filants : garde-corps sur toute la façade au 2e étage
                        float gauche = 1e9f, droite = -1e9f;
                        for (const Sommet& v : b.part[METAL].v) if (std::fabs(v.p.y - (p.niveauY(2) + 1.0f)) < 1e-3f) { gauche = std::min(gauche, v.p.x); droite = std::max(droite, v.p.x); }
                        verifierQue(droite - gauche > p.largeur - 0.3f, "ST-2 : balcon filant du classique d'une rive à l'autre");
                    }
                    // M-01 : bas des vitres de deux étages successifs (même travée) espacés d'une hauteur d'étage à ± 1 %
                    for (const Baie& x : p.baies) for (const Baie& y : p.baies)
                        if (x.niveau >= 1 && y.niveau == x.niveau + 1 && x.travee == y.travee)
                            verifierQue(std::fabs((y.y - x.y) - p.hEtage) <= 0.01f * p.hEtage, "M-01 : niveaux espacés d'un étage à ± 1 %");
                    // garde-corps : dessus à la hauteur du référentiel au-dessus du plancher du balcon
                    const Maillage& met = b.part[METAL];
                    for (const Baie& x : p.baies) if (x.balcon) {
                        float haut = -1;
                        for (const Sommet& v : met.v) if (std::fabs(v.p.x - x.x) < x.l / 2 + 0.5f && v.p.y > haut && v.p.y < p.niveauY(x.niveau) + 2.f && v.p.y > p.niveauY(x.niveau) - 0.01f) haut = v.p.y;
                        verifierQue(std::fabs(haut - p.niveauY(x.niveau) - gcRef) <= 0.01 * gcRef, "garde-corps à la hauteur du référentiel (± 1 %)");
                    }
                }
            }
        }
        verifierQue(tris[0] > tris[1] && tris[1] > tris[2], "chaque niveau de détail est moins cher que le précédent");
        verifierWFS402((Style)s);
        std::printf("[%s] %-14s : triangles moyens par bâtiment lod0 %zu, lod1 %zu, lod2 %zu\n", tris[0] > tris[1] && tris[1] > tris[2] ? "OK" : "ÉCHEC",
                    regles((Style)s).nom, tris[0] / 60, tris[1] / 60, tris[2] / 60);
        for (int l = 0; l < 3; l++) trisMax[l] = std::max(trisMax[l], tris[l] / 60);
    }
    std::printf("archi_geo : %d contrôles, %d échec(s) ; garde-corps du référentiel %.2f m\n", controles, echecs, gcRef);
    return echecs ? 1 : 0;
}
