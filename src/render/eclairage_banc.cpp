// WaForSpeed — WFS-151 : banc de la lumière physique (voir eclairage_banc.h).
#include "eclairage_banc.h"
#include <array>
#include <cstdlib>
#include <sstream>
#include "../core/log.h"
#include "../core/platform.h"   // nowSeconds
#include "../core/system.h"
#include "../game/game.h"
#include "../game/trace_cartes.h"
#include "../game/track.h"
#include "../ui/ecrans/reglages.h"
#include "atmosphere_physique.h"   // WFS-207 : astres et atmosphère (M-01 à M-03)
#include "eclairage_physique.h"
#include "passes.h"
#include "renderer.h"

namespace wfs {
namespace {

using reel::Etat; using reel::LigneRapport; using reel::fmt;
constexpr Ecran ECRAN = Ecran::Chargement;   // comme le banc du réalisme : scène sans interface, voiture immobile
constexpr int PAS = 90;                       // pas de simulation avant la mesure (chargement des morceaux)

// charte (même disposition que la scène d'étalonnage de WFS-148) : 24 cases de 0,30 m, charte grise de 0,6 m ;
// en plus : charte grise à l'ombre d'une plaque (M-03)
constexpr float CHARTE_A = 10, CASE = 0.30f, PAS_C = 0.35f, GRISE = 0.6f, GRISE_A = CHARTE_A + 1.25f, SOL = 0.02f;
constexpr float PLAQUE_H = 1.5f, PLAQUE = 1.2f;   // plaque d'ombre : 1,2 m de côté, 1,5 m au-dessus de la charte à l'ombre
enum Mode { AUCUN, ETALON, LIBRE };

struct EtatBanc {
    Mode mode = AUCUN; bool noir = false;
    std::vector<vec3> albedos;        // 24 cases puis la charte grise
    vec3 oeil{}, cible{}, haut{0, 1, 0}; float fov = 50; bool monde = false;   // monde : oeil / cible en coordonnées du monde
    vec3 oeilR{}, cibleR{};           // repère de la route (a devant, l à droite, h)
    mat4 view = mat4::identity(), proj = mat4::identity();
    vec3 soleil{0, 1, 0};             // direction du soleil de la dernière image
    vec3 centreGrise{}, centreOmbre{}, centreBlanc{};
    Track* piste = nullptr; double dist = 0, position = 0; vec3 oeilVu{};   // dernière image (repère de la route)
};
EtatBanc g_l;

vec3 pointRoute(Track& t, double s, float a, float l, float h) { const TrackPoint q = t.point(s + a / M, l); return {(float)q.x, (float)q.y + h, (float)q.z}; }
vec3 casePos(int k) { if (k == 24) return {GRISE_A, 0, 0}; const int col = k % 6, rang = k / 6; return {CHARTE_A + 1.5f * PAS_C - rang * PAS_C, -2.5f * PAS_C + col * PAS_C, 0}; }

Mesh quadSol(Renderer& r, const vec3 c[4]) {
    std::vector<Vertex> v; for (int i = 0; i < 4; i++) v.push_back({c[i].x, c[i].y, c[i].z, 0, 1, 0, 0, 0});
    const vec3 n = cross(c[1] - c[0], c[2] - c[0]);
    return r.makeMesh(v, n.y > 0 ? std::vector<uint32_t>{0, 1, 2, 0, 2, 3} : std::vector<uint32_t>{0, 2, 1, 0, 3, 2});
}

class LumiereBanc : public System {
public:
    const char* name() const override { return "lumiere-banc"; }
    // cases 0 à 23 : ColorChecker ; 24 : charte grise au soleil ; 25 : charte grise à l'ombre ; 26 : plaque ; 27 : blanc de
    // référence (réflectance 1, diffuseur parfait : étalonnage des réflectances, comme le carreau blanc d'un spectrophotomètre)
    void init(Renderer& r, const GameView& g) override {
        if (g_l.mode != ETALON || g_l.albedos.size() != 25 || !g.track || !g.player) return;
        const double s = g.player->dist;
        auto P = [&](float a, float l) { return pointRoute(*g.track, s, a, l, SOL); };
        auto charte = [&](int k, const vec3& c, float d, const vec3& albedo) {   // carré horizontal de demi-côté d centré en c (monde)
            const vec3 q[4] = {c + vec3{-d, 0, -d}, c + vec3{-d, 0, d}, c + vec3{d, 0, d}, c + vec3{d, 0, -d}};
            mesh_[k] = quadSol(r, q);
            Material m; m.color = {albedo.x, albedo.y, albedo.z, 1}; m.rough = 0.9f; m.metal = 0;   // charte mate (Physically Based, « Gray Card »)
            mat_[k] = r.makeMaterial(m, &r.white, nullptr, nullptr);
            Material n = m; n.color = {0, 0, 0, 1}; noir_[k] = r.makeMaterial(n, &r.white, nullptr, nullptr);
        };
        for (int k = 0; k < 25; k++) {
            const vec3 c = casePos(k); const float d = (k == 24 ? GRISE : CASE) / 2;
            const vec3 q[4] = {P(c.x - d, c.y - d), P(c.x - d, c.y + d), P(c.x + d, c.y + d), P(c.x + d, c.y - d)};
            mesh_[k] = quadSol(r, q);
            Material m; m.color = {g_l.albedos[k].x, g_l.albedos[k].y, g_l.albedos[k].z, 1}; m.rough = 0.9f; m.metal = 0;
            mat_[k] = r.makeMaterial(m, &r.white, nullptr, nullptr);
            Material n = m; n.color = {0, 0, 0, 1}; noir_[k] = r.makeMaterial(n, &r.white, nullptr, nullptr);
        }
        // charte à l'ombre : 2,2 m sur le côté de la route tourné vers le soleil (la plaque, plus loin vers le soleil, ne
        // cache rien à la caméra placée au-dessus de la charte) ; plaque de 1,2 m, 1,5 m au-dessus, sur le rayon du soleil
        const vec3 sd = soleil_, centre = P(CHARTE_A + 0.35f, 0), cote = normalize(P(CHARTE_A + 0.35f, 1) - centre);
        const float signe = dot(vec3{sd.x, 0, sd.z}, cote) >= 0 ? 1.f : -1.f;
        g_l.centreOmbre = P(CHARTE_A + 0.35f, 2.2f * signe);
        charte(25, g_l.centreOmbre, GRISE / 2, g_l.albedos[24]);
        const vec3 pc = g_l.centreOmbre + sd * (PLAQUE_H / std::max(sd.y, 0.2f));
        const float p = PLAQUE / 2;
        const vec3 qp[4] = {pc + vec3{-p, 0, -p}, pc + vec3{-p, 0, p}, pc + vec3{p, 0, p}, pc + vec3{p, 0, -p}};
        mesh_[26] = quadSol(r, qp);
        Material mp; mp.color = {0.3f, 0.3f, 0.3f, 1}; mp.rough = 0.9f; mp.flags = MAT_DOUBLE;
        mat_[26] = r.makeMaterial(mp, &r.white, nullptr, nullptr); noir_[26] = mat_[26];
        g_l.centreBlanc = P(GRISE_A, -0.75f * signe);
        charte(27, g_l.centreBlanc, CASE / 2, {1, 1, 1});
        g_l.centreGrise = P(GRISE_A, 0);
        cree_ = true;
    }
    void ambiance(FrameParams& fp, const GameView& g) override {
        if (g_l.mode == AUCUN || !g.track || !g.player) return;
        vec3 oeil = g_l.oeil, cible = g_l.cible;
        if (!g_l.monde) { const double s = g.player->dist; oeil = pointRoute(*g.track, s, g_l.oeilR.x, g_l.oeilR.y, g_l.oeilR.z); cible = pointRoute(*g.track, s, g_l.cibleR.x, g_l.cibleR.y, g_l.cibleR.z); }
        const float aspect = fp.proj.at(0, 0) != 0 ? -fp.proj.at(1, 1) / fp.proj.at(0, 0) : 16.f / 9.f;
        fp.view = lookAt(oeil, cible, g_l.haut);
        fp.proj = perspectiveVk(g_l.fov * PI / 180.f, aspect, 0.1f);
        fp.camPos = oeil;
        g_l.view = fp.view; g_l.proj = fp.proj; g_l.oeilVu = oeil;
        g_l.piste = g.track; g_l.dist = g.player->dist; g_l.position = g.player->position;
    }
    void render(Renderer& r, const GameView&) override {
        if (g_l.mode != ETALON || !cree_) return;
        const mat4 I = mat4::identity();
        for (int k = 0; k < 28; k++) r.draw(mesh_[k], g_l.noir ? noir_[k] : mat_[k], I);
    }
    void shutdown(Renderer& r) override {
        if (!cree_) return;
        r.vk().waitIdle();
        for (int k = 0; k < 28; k++) { r.destroy(mesh_[k]); r.destroy(mat_[k]); if (k != 26) r.destroy(noir_[k]); }
        cree_ = false;
    }
    static vec3 soleil_;
private:
    Mesh mesh_[28]; Material mat_[28], noir_[28]; bool cree_ = false;
};
vec3 LumiereBanc::soleil_{0, 1, 0};
WFS_SYSTEM(LumiereBanc, 985)   // banc (900-999) : après les bancs du réalisme (970) et des passes (975)

// ---------------------------------------------------------------- mesures dans la cible HDR
struct Hdr { std::vector<float> px; uint32_t w = 0, h = 0; };
bool lire(Renderer& r, Hdr& h) { return passes::lireHdr(r, h.px, h.w, h.h); }
// moyenne rgb d'un carré de (2n + 1)² pixels autour du point du monde p (vue de la dernière image)
bool moyenne(const Hdr& h, const vec3& p, int n, double out[3]) {
    const mat4 vp = g_l.proj * g_l.view;
    const float x = vp.at(0, 0) * p.x + vp.at(0, 1) * p.y + vp.at(0, 2) * p.z + vp.at(0, 3), y = vp.at(1, 0) * p.x + vp.at(1, 1) * p.y + vp.at(1, 2) * p.z + vp.at(1, 3);
    const float w = vp.at(3, 0) * p.x + vp.at(3, 1) * p.y + vp.at(3, 2) * p.z + vp.at(3, 3);
    if (w <= 0) return false;
    const int cx = (int)((x / w * 0.5f + 0.5f) * h.w), cy = (int)((y / w * 0.5f + 0.5f) * h.h);
    double s[3] = {0, 0, 0}; int k = 0;
    for (int dy = -n; dy <= n; dy++) for (int dx = -n; dx <= n; dx++) {
        const int X = cx + dx, Y = cy + dy; if (X < 0 || Y < 0 || X >= (int)h.w || Y >= (int)h.h) continue;
        for (int c = 0; c < 3; c++) s[c] += h.px[((size_t)Y * h.w + X) * 4 + c];
        k++;
    }
    if (!k) return false;
    for (int c = 0; c < 3; c++) out[c] = s[c] / k;
    return true;
}
bool centre(const Hdr& h, int n, double out[3]) {
    double s[3] = {0, 0, 0}; int k = 0;
    for (int dy = -n; dy <= n; dy++) for (int dx = -n; dx <= n; dx++) { const size_t i = ((size_t)(h.h / 2 + dy) * h.w + h.w / 2 + dx) * 4; for (int c = 0; c < 3; c++) s[c] += h.px[i + c]; k++; }
    for (int c = 0; c < 3; c++) out[c] = s[c] / k;
    return true;
}
double Y(const double c[3]) { return lumiere::luminanceRgb(c); }

void preparer(Game& g, Renderer& r, const char* carte, const char* moment, int depart, int qualite) {
    setenv("WFS_MOMENT", moment, 1);
    if (depart >= 0) setenv("WFS_DEPART", std::to_string(depart).c_str(), 1); else unsetenv("WFS_DEPART");
    ecrans::reglages().qualite = qualite; ecrans::appliquerQualiteNiveau(r);
    (void)g; (void)carte;
}
// rend des images jusqu'à ce qu'une sonde capturée soit en place (au plus 40 images), temps figé
void attendreSonde(Game& g, double t) { attendreSondes(g, t); }

// hauteur du soleil où le sol horizontal reçoit 100 000 lx du soleil seul (lum.soleil_midi.intensite, valeur typique)
double hauteurMidi(double cible) {
    double a = 5, b = 90;
    for (int i = 0; i < 60; i++) { const double m = 0.5 * (a + b); (lumiere::eclairementSoleilNormal(m) * std::sin(m * lumiere::PI_L / 180) < cible ? a : b) = m; }
    return 0.5 * (a + b);
}

LigneRapport ligne(const std::string& id, const std::string& grandeur, const std::string& jeu, const std::string& reel, const std::string& tol, bool ok, const std::string& statut, const std::string& raison = "") {
    LigneRapport l; l.domaine = "lumiere"; l.id = id; l.grandeur = grandeur; l.jeu = jeu; l.reel = reel; l.tolerance = tol;
    l.etat = ok ? Etat::Conforme : Etat::NonConforme; l.statutReel = statut; l.raison = raison; return l;
}

} // namespace

void attendreSondes(Game& g, double t) {
    if (!eclairage::etat().actif) return;
    for (int i = 0; i < 60 && !eclairage::sondeCapturePrete(); i++) g.render(t);
    for (int i = 0; i < 3; i++) g.render(t);
}

int benchLumiere(Game& g, Renderer& r, const std::string& out, const reel::Referentiel& R, reel::Rapport& rap, std::ostream& mes, std::ostream& infos) {
    using namespace lumiere;
    int erreurs = 0; const double t = 2000;
    ecrans::Reglages& reglages = ecrans::reglages(); const int q0 = reglages.qualite;
    mes << "\n== lumière physique (WFS-151) ==\n";
    infos << "\n== Lumière physique (WFS-151) ==\n";
    // ---------------- M-01 à M-03 : étalonnage (Campagne, départ, jour, Haute, soleil de midi, EV100 15)
    g_l = EtatBanc{};
    for (int k = 1; k <= 25; k++) {
        char id[32]; std::snprintf(id, sizeof id, "cc.%02d.albedo", k);
        const reel::Json* m = k == 25 ? R.mesure("mat.charte_grise_18.albedo") : R.mesure(id);
        const reel::Json* v = m ? m->get("valeur") : nullptr;
        if (!v || v->a.size() != 3) { WFS_ERR("lumière (WFS-151) : étalonnage : %s absent du référentiel", k == 25 ? "mat.charte_grise_18.albedo" : id); erreurs++; break; }
        g_l.albedos.push_back({(float)v->a[0].n, (float)v->a[1].n, (float)v->a[2].n});
    }
    const reel::Json* jSoleil = R.mesure("lum.soleil_midi.intensite"); const reel::Json* jCiel = R.mesure("lum.ciel_clair.intensite");
    const double eMidi = jSoleil ? jSoleil->nombre("typique", 100000) : 100000, eOmbreMesure = jCiel ? jCiel->nombre("valeur", 6000) : 6000;
    const double hMidi = hauteurMidi(eMidi);
    // WFS-214 (RG-02, option A) : la charte à l'ombre reçoit le ciel ENTIER (Krochmann, lum.ciel_clair.plein_ciel) : le moteur n'occulte pas
    // la lumière du ciel par la plaque (pas d'occlusion du ciel sur les objets mobiles) ; les 6 000 lx « à l'ombre » sont notés pour mémoire
    const double eOmbre = eclairementCielDiffus(hMidi);
    const bool SEULM = std::getenv("WFS_MOMENTS_SEUL") != nullptr;   // WFS-214 : essai local des seuls moments de WFS-208 (avec WFS_LUMIERE_SEUL)
    if (g_l.albedos.size() == 25 && !SEULM) {
        preparer(g, r, "campagne", "jour", -1, ecrans::QUALITE_HAUTE);
        g_l.mode = ETALON; g_l.monde = false; g_l.oeilR = {CHARTE_A + 0.35f, 0, 3.2f}; g_l.cibleR = {CHARTE_A + 0.35f, 0, 0}; g_l.fov = 50;
        // WFS-207 : l'étalon garde l'azimut de WFS-151 (318°, ancien soleil du jeu (−0,45 ; −0,5)) : mêmes conditions de mesure
        eclairage::Forcage f; f.actif = true; f.hauteurSoleil = hMidi; f.ev = 15; f.azimutSoleil = 318.0; eclairage::forcer(f);
        {   // soleil de l'étalon : azimut forcé ci-dessus, hauteur forcée
            const vec3 sj = atmosphere::directionMonde(0, 318.0), hz = normalize(vec3{sj.x, 0, sj.z}); const float hr = (float)(hMidi * PI_L / 180);
            LumiereBanc::soleil_ = hz * std::cos(hr) + vec3{0, std::sin(hr), 0};
        }
        if (g.init(r, "golf", "campagne")) {
            g.allerA(ECRAN);
            g_l.haut = {1, 0, 0};
            for (int i = 0; i < PAS; i++) { g.update(1.0 / 60); if (i % 10 == 0) g.render(t); }
            if (g_l.piste) {   // vue de dessus : le haut de l'image vers l'avant de la route (comme WFS-148)
                const vec3 a = pointRoute(*g_l.piste, g_l.dist, CHARTE_A + 1.35f, 0, 0), b = pointRoute(*g_l.piste, g_l.dist, CHARTE_A + 0.35f, 0, 0);
                g_l.haut = normalize(a - b);
            }
            attendreSonde(g, t);
            r.capturePng(out + "/realisme-etalon-hdr.png");
            Hdr hc, hn;
            const bool okC = lire(r, hc);
            g_l.noir = true; for (int i = 0; i < 3; i++) g.render(t);
            const bool okN = lire(r, hn);
            g_l.noir = false;
            const eclairage::Etat e = eclairage::etat();
            if (!okC || !okN || !g_l.piste) { WFS_ERR("lumière (WFS-151) : étalonnage HDR (Campagne) : cible HDR illisible ou piste absente"); erreurs++; rap.nonMesurable("lumiere", "lum.etalon", "charte rendue (HDR)", "cible HDR illisible"); }
            else {
                // différence couleur − noir : part diffuse seule (le reflet spéculaire, identique, s'annule) ; ÷ k : cd/m²
                const double k = e.k;
                auto diffus = [&](const vec3& p, double o[3]) {
                    double a[3], b[3]; if (!moyenne(hc, p, 4, a) || !moyenne(hn, p, 4, b)) return false;
                    for (int c = 0; c < 3; c++) o[c] = (a[c] - b[c]) / k;
                    return true;
                };
                std::vector<std::array<double, 3>> D(25);
                for (int q = 0; q < 25; q++) {
                    const vec3 c = casePos(q); const vec3 p = pointRoute(*g_l.piste, g_l.dist, c.x, c.y, SOL);
                    double o[3]; if (!diffus(p, o)) { WFS_ERR("lumière (WFS-151) : étalonnage HDR : case %d de la charte illisible", q + 1); erreurs++; o[0] = o[1] = o[2] = 0; }
                    D[q] = {o[0], o[1], o[2]};
                }
                // M-01 : charte grise : luminance diffuse attendue (même modèle : Lambert (1 − F) au soleil + irradiance de la sonde)
                const vec3 pg = pointRoute(*g_l.piste, g_l.dist, GRISE_A, 0, SOL);
                const vec3 Ls = LumiereBanc::soleil_, V = normalize(g_l.oeilVu - pg);
                const vec3 H = normalize(V + Ls);
                const double VoH = std::max(0.f, dot(V, H)), Fs = 0.04 + 0.96 * std::pow(1 - VoH, 5), NoL = std::max(0.f, Ls.y);
                double Esh[3]; eclairage::irradiance({0, 1, 0}, Esh);
                const double rho = 0.18;
                double att[3];
                for (int c = 0; c < 3; c++) att[c] = (1 - Fs) * rho / PI_L * e.eSoleilNormal * NoL * (c == 0 ? e.couleurSoleil.x : c == 1 ? e.couleurSoleil.y : e.couleurSoleil.z) + rho * Esh[c] / PI_L;
                const double yR = Y(D[24].data()), yA = Y(att), ecart = yR / yA - 1;
                rap.ajouter(ligne("lum.m01.charte_grise_ev15", "M-01 : charte grise 18 % au soleil de midi (" + fmt(eMidi, 0) + " lx au sol, h " + fmt(e.hauteurSoleil, 2) + "°), EV100 15 : valeur rendue (HDR) ÷ valeur attendue",
                                  fmt(yR * k, 5) + " (pré-exposée ; " + fmt(yR, 1) + " cd/m²)", fmt(yA * k, 5) + " (" + fmt(yA, 1) + " cd/m², Lambert (1 − F) + sonde)", "± 1 %",
                                  std::fabs(ecart) <= 0.01, "derive", "écart " + fmt(ecart * 100, 2) + " % ; k = exposition(EV100 15) = " + fmt(k, 8)));
                // M-02 : ColorChecker : réflectance = D / D(blanc de référence, réflectance 1) → L*a*b* (D65 → D50, Bradford)
                double Db[3]; const bool okB = diffus(g_l.centreBlanc, Db) && Db[0] > 0 && Db[1] > 0 && Db[2] > 0;
                int bons = 0;
                for (int q = 0; q < 24 && okB; q++) {
                    char id[16]; std::snprintf(id, sizeof id, "cc.%02d", q + 1);
                    const reel::Json* ref = R.mesure(id); if (!ref || !ref->get("valeur")) continue;
                    double rgb[3]; for (int c = 0; c < 3; c++) rgb[c] = D[q][c] / Db[c];
                    const auto& rv = ref->get("valeur")->a;
                    const reel::Lab m = reel::lineaireVersLab(rgb), a{rv[0].n, rv[1].n, rv[2].n};
                    const double dE = reel::deltaE2000(m, a);
                    if (dE <= 2) bons++;
                    LigneRapport l = ligne(std::string("lum.m02.") + id, "M-02 : " + ref->texte("grandeur") + ", réflectance rendue avant tonemapping (référence : blanc de réflectance 1)",
                                           "L*a*b* " + fmt(m.L, 2) + " " + fmt(m.a, 2) + " " + fmt(m.b, 2) + ", ΔE2000 " + fmt(dE, 2), "L*a*b* " + fmt(a.L, 2) + " " + fmt(a.a, 2) + " " + fmt(a.b, 2),
                                           "ΔE2000 ≤ 2", dE <= 2, ref->texte("statut"));
                    char ida[24]; std::snprintf(ida, sizeof ida, "cc.%02d.albedo", q + 1);
                    if (const reel::Json* al = R.mesure(ida)) if (const reel::Json* v = al->get("valeur")) for (const reel::Json& x : v->a) if (x.n < 0)
                        { l.raison = "hors du gamut sRGB : albédo du référentiel négatif (" + fmt(x.n, 4) + "), ramené à 0 par un rendu en primaires sRGB : valeur réelle non reproductible, hors indice (M-02 en tient compte : 23 cases sur 24)"; l.etat = Etat::NonMesurable; }
                    rap.ajouter(l);
                }
                if (!okB) rap.nonMesurable("lumiere", "lum.m02", "ColorChecker", "blanc de référence hors de l'image");
                if (okB) rap.ajouter(ligne("lum.m02.colorchecker", "M-02 : ColorChecker, cases à ΔE2000 ≤ 2 (réflectance avant tonemapping)", std::to_string(bons) + " / 24", "≥ 23 / 24",
                                           "-", bons >= 23, "a_confirmer"));
                infos << "M-02 : " << bons << " cases sur 24 à ΔE2000 ≤ 2 (critère : au moins 23 sur 24).\n";
                // M-03 : rapport soleil / ombre sur la charte grise (part diffuse) contre (E soleil + E ombre) / E ombre publiés
                double Do[3]; const bool okO = diffus(g_l.centreOmbre, Do);
                if (okO && Y(Do) > 0) {
                    const double rapport = Y(D[24].data()) / Y(Do), attendu = (eMidi + eOmbre) / eOmbre, e3 = rapport / attendu - 1;
                    rap.ajouter(ligne("lum.m03.soleil_ombre", "M-03 : rapport soleil / ombre de la charte grise (part diffuse, HDR)", fmt(rapport, 3),
                                      fmt(attendu, 3) + " ((" + fmt(eMidi, 0) + " + " + fmt(eOmbre, 0) + ") / " + fmt(eOmbre, 0) + " lx : soleil de photometrie.json, ciel entier de Krochmann ; " + fmt(std::log2(attendu), 2) + " IL)", "± 5 %", std::fabs(e3) <= 0.05, "derive",
                                      "écart " + fmt(e3 * 100, 2) + " % ; à l'ombre : " + fmt(Y(Do) * PI_L / 0.18, 0) + " lx équivalents (mesure réelle à l'ombre d'un obstacle : " + fmt(eOmbreMesure, 0) + " lx, sans occlusion du ciel dans le moteur)"));
                } else rap.nonMesurable("lumiere", "lum.m03.soleil_ombre", "rapport soleil / ombre", "charte à l'ombre hors de l'image");
                // photométrie de l'étalon (informations et mesures)
                rap.ajouter(ligne("lum.soleil_midi.intensite", "éclairement du soleil seul au sol horizontal, étalon (modèle : masse d'air de Kasten et Young, trouble 2)",
                                  fmt(e.eSoleilHorizontal, 0) + " lx", fmt(eMidi, 0) + " lx (valeur typique)", "± 5 %", std::fabs(e.eSoleilHorizontal / eMidi - 1) <= 0.05, jSoleil ? jSoleil->texte("statut") : ""));
                double Eh[3]; eclairage::irradiance({0, 1, 0}, Eh);
                infos << "Étalon : soleil h " << fmt(e.hauteurSoleil, 2) << "°, " << fmt(e.eSoleilNormal, 0) << " lx (normal), " << fmt(e.eSoleilHorizontal, 0) << " lx (sol) ; ciel (modèle) "
                      << fmt(e.eCielDiffus, 0) << " lx ; sonde (SH9, sol horizontal) " << fmt(Y(Eh), 0) << " lx ; EV100 " << fmt(e.ev, 2) << ", k " << fmt(e.k, 8)
                      << " ; sonde capturée " << (e.sondeCaptureActive ? "oui" : "NON") << " (traitement " << fmt(e.msTraitement, 1) << " ms CPU).\n";
            }
            g.shutdown();
        } else { WFS_ERR("lumière (WFS-151) : étalonnage HDR : chargement de la Campagne impossible"); erreurs++; }
        g_l.mode = AUCUN; eclairage::forcer({});
    }
    // ---------------- M-04 : ciel au zénith et 12° au-dessus de l'horizon (azimut du soleil + 90°), 3 hauteurs du soleil
    for (double hs : {20.0, 45.0, 70.0}) {
        if (SEULM) break;
        preparer(g, r, "campagne", "jour", -1, ecrans::QUALITE_HAUTE);
        eclairage::Forcage f; f.actif = true; f.hauteurSoleil = hs; f.ev = 15; eclairage::forcer(f);
        g_l = EtatBanc{}; g_l.mode = LIBRE; g_l.monde = true; g_l.fov = 20;
        if (!g.init(r, "golf", "campagne")) { WFS_ERR("lumière (WFS-151) : ciel M-04 (soleil à %.0f°) : chargement de la Campagne impossible", hs); erreurs++; continue; }
        g.allerA(ECRAN);
        for (int i = 0; i < PAS; i++) { g.update(1.0 / 60); if (i % 10 == 0) g.render(t); }
        const vec3 sa = eclairage::etat().astre, soleil = normalize(vec3{sa.x, 0, sa.z});   // WFS-207 : azimut du soleil calculé (NOAA)
        const vec3 lateral = normalize(cross(soleil, vec3{0, 1, 0}));
        const double hs2 = 12;
        for (int vue = 0; vue < 2; vue++) {
            if (!g_l.piste) { WFS_ERR("lumière (WFS-151) : ciel M-04 (soleil à %.0f°) : piste absente", hs); erreurs++; break; }
            const vec3 o = pointRoute(*g_l.piste, g_l.dist, 0, 0, 300.f);   // WFS-207 : au-dessus des arbres (le soleil calculé change l'azimut visé)
            vec3 d = vue == 0 ? vec3{0, 1, 0} : normalize(lateral * (float)std::cos(hs2 * PI_L / 180) + vec3{0, (float)std::sin(hs2 * PI_L / 180), 0});
            g_l.oeil = o; g_l.cible = o + d * 10.f; g_l.haut = vue == 0 ? vec3{1, 0, 0} : vec3{0, 1, 0};
            for (int i = 0; i < 3; i++) g.render(t);
            Hdr h; double c[3];
            if (!lire(r, h)) { WFS_ERR("lumière (WFS-151) : ciel M-04 (soleil à %.0f°, vue %d) : image HDR illisible", hs, vue); erreurs++; continue; }
            centre(h, 2, c);
            const eclairage::Etat e = eclairage::etat();
            const double Lr = Y(c) / e.k;
            const vec3 sd = normalize(soleil * (float)std::cos(hs * PI_L / 180) + vec3{0, (float)std::sin(hs * PI_L / 180), 0});
            const double dd[3] = {d.x, d.y, d.z}, ss[3] = {sd.x, sd.y, sd.z};
            const double La = e.facteurCie * cieRelative(dd, ss), ec = Lr / La - 1;
            rap.ajouter(ligne(std::string("lum.m04.h") + fmt(hs, 0) + (vue == 0 ? ".zenith" : ".horizon"),
                              std::string("M-04 : luminance du ciel ") + (vue == 0 ? "au zénith" : "à 12° au-dessus de l'horizon, azimut du soleil + 90°") + ", soleil à " + fmt(hs, 0) + "°",
                              fmt(Lr, 1) + " cd/m²", fmt(La, 1) + " cd/m² (CIE type 12, E_d " + fmt(e.eCielDiffus, 0) + " lx)", "± 5 %", std::fabs(ec) <= 0.05, "conception",
                              "écart " + fmt(ec * 100, 2) + " %"));
            r.capturePng(out + "/realisme-ciel-h" + fmt(hs, 0) + (vue == 0 ? "-zenith" : "-horizon") + ".png");
        }
        g.shutdown();
        g_l.mode = AUCUN; eclairage::forcer({});
    }
    // ---------------- WFS-207 : M-01 soleil à l'écran contre la NOAA (hauteur, azimut), M-02 diamètre apparent du soleil et
    // de la lune, M-03 chromaticité du ciel au zénith et à l'horizon contre le modèle d'atmosphère (Hillaire 2020) ;
    // 3 heures (midi solaire, soleil à 10° et à 2°) × 2 cartes, caméra à 300 m au-dessus de la route, champ de 1,5°
    if (!SEULM) {
        const atmosphere::Lieu& Lx = atmosphere::lieu();
        auto noaa = [&](double hu) { return reel::soleil(Lx.lat, Lx.lon, Lx.annee, Lx.mois, Lx.jour, hu); };
        double hMidi = 9; for (double hu = 9; hu <= 15; hu += 0.005) if (noaa(hu).hauteurApparente > noaa(hMidi).hauteurApparente) hMidi = hu;
        auto heure = [&](double cible) { double a = hMidi, b = 21; for (int i = 0; i < 60; i++) { const double m = 0.5 * (a + b); (noaa(m).hauteurApparente > cible ? a : b) = m; } return 0.5 * (a + b); };
        const double FOV = 1.5;
        // disque : pixels au-dessus du quart du maximum ; centre (direction mesurée) et diamètre équivalent (°)
        auto disque = [&](const Hdr& hd, const vec3& oeil, const vec3& fwd, const vec3& haut, vec3& dir, double& diam) {
            double mx = 0; for (size_t i = 0; i < (size_t)hd.w * hd.h; i++) { const double c[3] = {hd.px[i * 4], hd.px[i * 4 + 1], hd.px[i * 4 + 2]}; mx = std::max(mx, Y(c)); }
            double sx = 0, sy = 0; size_t n = 0;
            for (uint32_t y = 0; y < hd.h; y++) for (uint32_t x = 0; x < hd.w; x++) {
                const size_t i = ((size_t)y * hd.w + x) * 4; const double c[3] = {hd.px[i], hd.px[i + 1], hd.px[i + 2]};
                if (Y(c) > 0.25 * mx) { sx += x + 0.5; sy += y + 0.5; n++; }
            }
            if (!n || mx <= 0) return false;
            const double t = std::tan(FOV * PI_L / 360) / (hd.h / 2.0);   // tangente par pixel (champ vertical)
            const vec3 dr = normalize(cross(fwd, haut)), up = cross(dr, fwd);
            dir = normalize(fwd + dr * (float)((sx / n - hd.w / 2.0) * t) - up * (float)((sy / n - hd.h / 2.0) * t));
            diam = 2 * std::sqrt(n / PI_L) * FOV / hd.h; (void)oeil;
            return true;
        };
        auto xyDe = [](const double c[3], double& x, double& y) { atmosphere::chromaticite(c, x, y); };
        for (const char* carte : {"campagne", "circuit"}) for (int k = 0; k < 3; k++) {
            const double hu = k == 0 ? hMidi : heure(k == 1 ? 10.0 : 2.0);
            const reel::Soleil ns = noaa(hu);
            const vec3 attendu = atmosphere::directionMonde(ns.hauteurApparente, ns.azimut);
            preparer(g, r, carte, "jour", -1, ecrans::QUALITE_HAUTE);
            eclairage::Forcage f; f.actif = true; f.heureUtc = hu; f.sansNuages = true; eclairage::forcer(f);
            g_l = EtatBanc{}; g_l.mode = LIBRE; g_l.monde = true; g_l.fov = (float)FOV;
            if (!g.init(r, "golf", carte)) { WFS_ERR("lumière (WFS-207) : soleil et ciel : chargement de %s impossible", carte); erreurs++; continue; }
            g.allerA(ECRAN);
            for (int i = 0; i < PAS; i++) { g.update(1.0 / 60); if (i % 10 == 0) g.render(t); }
            if (!g_l.piste) { WFS_ERR("lumière (WFS-207) : soleil et ciel (%s) : piste absente", carte); erreurs++; g.shutdown(); continue; }
            const vec3 o = pointRoute(*g_l.piste, g_l.dist, 0, 0, 300.f);
            const std::string id = std::string("lum.wfs207.") + carte + ".h" + fmt(std::round(ns.hauteurApparente), 0);
            // M-01 et M-02 : caméra vers la direction de la NOAA
            g_l.oeil = o; g_l.cible = o + attendu * 10.f; g_l.haut = std::fabs(attendu.y) > 0.95f ? vec3{1, 0, 0} : vec3{0, 1, 0};
            for (int i = 0; i < 3; i++) g.render(t);
            Hdr hd; vec3 mes; double diam = 0;
            if (lire(r, hd) && disque(hd, o, attendu, g_l.haut, mes, diam)) {
                const double hm = std::asin(std::clamp((double)mes.y, -1.0, 1.0)) * 180 / PI_L, am = atmosphere::norm360(std::atan2((double)mes.x, -(double)mes.z) * 180 / PI_L);
                double da = std::fmod(am - ns.azimut + 540.0, 360.0) - 180.0; da *= std::cos(ns.hauteurApparente * PI_L / 180);
                const double dh = hm - ns.hauteurApparente;
                rap.ajouter(ligne(id + ".hauteur", std::string("M-01 : hauteur du centre du disque du soleil à l'écran (") + carte + ", " + fmt(hu, 2) + " h UTC)",
                                  fmt(hm, 3) + "°", fmt(ns.hauteurApparente, 3) + "° (NOAA, réfraction comprise)", "± 0,5°", std::fabs(dh) <= 0.5, "conception", "écart " + fmt(dh, 3) + "°"));
                rap.ajouter(ligne(id + ".azimut", std::string("M-01 : azimut du centre du disque (écart sur le ciel, Δaz × cos h)"),
                                  fmt(am, 3) + "°", fmt(ns.azimut, 3) + "° (NOAA)", "± 0,5°", std::fabs(da) <= 0.5, "conception", "écart " + fmt(da, 3) + "°"));
                if (k == 0) rap.ajouter(ligne(std::string("lum.wfs207.") + carte + ".diametre_soleil", "M-02 : diamètre apparent du soleil (disque à l'écran, champ 1,5°)",
                                              fmt(diam, 4) + "°", "0,533° (32,0', NASA « Sun Fact Sheet »)", "± 1 %", std::fabs(diam / 0.533 - 1) <= 0.01, "conception", "écart " + fmt((diam / 0.533 - 1) * 100, 2) + " %"));
            } else rap.nonMesurable("lumiere", id + ".position", "M-01 : disque du soleil", "disque introuvable dans l'image (caché ?)");
            // M-03 : zénith, puis horizon (azimut du soleil + 90°, 0,05° au-dessus de l'horizon) contre le modèle
            const atmosphere::Ciel A = atmosphere::ciel(attendu);
            const vec3 lat = normalize(cross(vec3{attendu.x, 0, attendu.z}, vec3{0, 1, 0}));
            for (int vue = 0; vue < 2; vue++) {
                const vec3 d = vue == 0 ? vec3{0, 1, 0} : normalize(lat + vec3{0, 0.000873f, 0});
                g_l.cible = o + d * 10.f; g_l.haut = vue == 0 ? vec3{1, 0, 0} : vec3{0, 1, 0};
                for (int i = 0; i < 3; i++) g.render(t);
                double c[3], x, y, xm, ym;
                if (!lire(r, hd)) { WFS_ERR("lumière (WFS-207) : ciel (%s, vue %d) : image HDR illisible", carte, vue); erreurs++; continue; }
                centre(hd, 2, c); xyDe(c, x, y); xyDe(vue == 0 ? A.zenith : A.horizon, xm, ym);
                const double ec = std::max(std::fabs(x - xm), std::fabs(y - ym));
                rap.ajouter(ligne(id + (vue == 0 ? ".zenith_xy" : ".horizon_xy"),
                                  std::string("M-03 : chromaticité du ciel ") + (vue == 0 ? "au zénith" : "à l'horizon (moyenne des azimuts du modèle)") + ", soleil à " + fmt(ns.hauteurApparente, 1) + "°",
                                  "x " + fmt(x, 4) + " y " + fmt(y, 4), "x " + fmt(xm, 4) + " y " + fmt(ym, 4) + " (Rayleigh, Mie, ozone, diffusion multiple : Hillaire 2020)",
                                  "≤ 0,01", ec <= 0.01, "conception", "écart " + fmt(ec, 4)));
                if (vue == 0 && carte[1] == 'a') {   // information : zénith de Preetham, Shirley et Smits (1999), trouble T = 2,5
                    const double th = (90 - ns.hauteurApparente) * PI_L / 180, T = 2.5, t3 = th * th * th, t2 = th * th;
                    const double xp = T * T * (0.00166 * t3 - 0.00375 * t2 + 0.00209 * th) + T * (-0.02903 * t3 + 0.06377 * t2 - 0.03202 * th + 0.00394) + (0.11693 * t3 - 0.21196 * t2 + 0.06052 * th + 0.25886);
                    const double yp = T * T * (0.00275 * t3 - 0.00610 * t2 + 0.00317 * th) + T * (-0.04214 * t3 + 0.08970 * t2 - 0.04153 * th + 0.00516) + (0.15346 * t3 - 0.26756 * t2 + 0.06670 * th + 0.26688);
                    infos << "WFS-207, information : zénith soleil à " << fmt(ns.hauteurApparente, 1) << "° : jeu x " << fmt(x, 4) << " y " << fmt(y, 4)
                          << " ; Preetham 1999 (T 2,5) x " << fmt(xp, 4) << " y " << fmt(yp, 4) << " (modèle analytique publié, pour comparaison).\n";
                }
                r.capturePng(out + "/realisme-ciel-" + carte + "-h" + fmt(std::round(ns.hauteurApparente), 0) + (vue == 0 ? "-zenith" : "-horizon") + ".png");
            }
            // vue d'ensemble : 25° au-dessus de l'horizon, vers le soleil, champ de 60° (soleil, dégradé, brume au loin)
            g_l.fov = 60; g_l.oeil = pointRoute(*g_l.piste, g_l.dist, 0, 0, 3.f);
            g_l.cible = g_l.oeil + normalize(vec3{attendu.x, 0, attendu.z} + vec3{0, 0.25f, 0}) * 10.f; g_l.haut = {0, 1, 0};
            for (int i = 0; i < 3; i++) g.render(t);
            r.capturePng(out + "/realisme-ciel-" + carte + "-h" + fmt(std::round(ns.hauteurApparente), 0) + ".png");
            g.shutdown();
            g_l.mode = AUCUN; eclairage::forcer({});
        }
        // M-02 : la lune (nuit), même mesure ; vue d'ensemble de nuit
        {
            const atmosphere::Astre lu = atmosphere::lune(Lx.heureNuitUtc);
            preparer(g, r, "campagne", "nuit", -1, ecrans::QUALITE_HAUTE);
            g_l = EtatBanc{}; g_l.mode = LIBRE; g_l.monde = true; g_l.fov = (float)FOV;
            if (g.init(r, "golf", "campagne")) {
                g.allerA(ECRAN);
                for (int i = 0; i < PAS; i++) { g.update(1.0 / 60); if (i % 10 == 0) g.render(t); }
                if (g_l.piste) {
                    const vec3 o = pointRoute(*g_l.piste, g_l.dist, 0, 0, 300.f);
                    g_l.oeil = o; g_l.cible = o + lu.dir * 10.f; g_l.haut = {0, 1, 0};
                    for (int i = 0; i < 3; i++) g.render(t);
                    Hdr hd; vec3 mes; double diam = 0;
                    if (lire(r, hd) && disque(hd, o, lu.dir, g_l.haut, mes, diam)) {
                        rap.ajouter(ligne("lum.wfs207.diametre_lune", "M-02 : diamètre apparent de la lune (disque à l'écran, phase " + fmt(lu.phase, 1) + "°, fraction éclairée " + fmt(lu.fraction, 3) + ")",
                                          fmt(diam, 4) + "°", "0,53° (règle de gestion WFS-207)", "± 1 %", std::fabs(diam / 0.53 - 1) <= 0.01, "conception", "écart " + fmt((diam / 0.53 - 1) * 100, 2) + " %"));
                        const double ec = std::acos(std::clamp((double)dot(mes, lu.dir), -1.0, 1.0)) * 180 / PI_L;
                        infos << "WFS-207 : lune à l'écran à " << fmt(ec, 3) << "° de sa position calculée (h " << fmt(lu.hauteur, 2) << "°, az " << fmt(lu.azimut, 2) << "°).\n";
                    } else rap.nonMesurable("lumiere", "lum.wfs207.diametre_lune", "M-02 : disque de la lune", "disque introuvable dans l'image");
                    r.capturePng(out + "/realisme-ciel-lune-zoom.png");
                    g_l.fov = 60; g_l.oeil = pointRoute(*g_l.piste, g_l.dist, 0, 0, 3.f);
                    g_l.cible = g_l.oeil + normalize(vec3{lu.dir.x, 0, lu.dir.z} + vec3{0, 0.3f, 0}) * 10.f;
                    for (int i = 0; i < 3; i++) g.render(t);
                    r.capturePng(out + "/realisme-ciel-campagne-nuit.png");
                }
                g.shutdown();
            } else { WFS_ERR("lumière (WFS-207) : lune M-02 (Campagne, nuit) : chargement impossible"); erreurs++; }
            g_l.mode = AUCUN;
        }
    }
    // ---------------- WFS-208 : 6 moments × 2 cartes (realisme-ciel-<moment>-<carte>), M-04 (lampadaires sous 50 lx), M-05
    // (heure dorée, heure bleue), puis 3 min en heure dorée accélérée × 20 (realisme-transition-0..3, aucun saut d'exposition)
    for (const char* carte : {"campagne", "circuit"}) for (int m = 0; m < 6; m++) {
        const atmosphere::Moment mo = (atmosphere::Moment)m;
        const bool nuitM = atmosphere::momentDeNuit(mo);
        preparer(g, r, carte, nuitM ? "nuit" : "jour", -1, ecrans::QUALITE_HAUTE);
        eclairage::choisirMoment(m);
        g_l = EtatBanc{}; g_l.mode = LIBRE; g_l.monde = true; g_l.fov = 60;
        if (!g.init(r, "golf", carte)) { WFS_ERR("lumière (WFS-208) : moment %d de %s : chargement impossible", m, carte); erreurs++; continue; }
        g.allerA(ECRAN); eclairage::remettreAZero();   // exposition recalée sur la scène (temps du banc figé : pas d'adaptation)
        for (int i = 0; i < PAS; i++) { g.update(1.0 / 60); if (i % 10 == 0) g.render(t); }
        if (!g_l.piste) { WFS_ERR("lumière (WFS-208) : moment %d de %s : piste absente", m, carte); erreurs++; g.shutdown(); continue; }
        const eclairage::Etat e0 = eclairage::etat();
        const vec3 az = normalize(vec3{e0.astre.x, 0, e0.astre.z});
        g_l.oeil = pointRoute(*g_l.piste, g_l.dist, 0, 0, 3.f); g_l.cible = g_l.oeil + normalize(az + vec3{0, 0.25f, 0}) * 10.f; g_l.haut = {0, 1, 0};
        attendreSondes(g, t);
        // WFS-214 (retour du lead dev, PR #314) : le temps du banc est figé et la scène n'est pas « neuve » (même carte que le moment
        // précédent) : l'exposition restait celle de l'aube (EV 8,2 à midi, image blanche, 7 IL trop claire). Comme dans le jeu, elle
        // s'adapte maintenant à sa cible (temps qui avance, 0,1 s par image, 20 s au plus) avant la capture
        double tm = t;
        for (int i = 0; i < 200 && std::fabs(eclairage::etat().ev - eclairage::etat().evCible) > 0.05; i++) { tm += 0.1; g.render(tm); }
        for (int i = 0; i < 2; i++) { tm += 0.1; g.render(tm); }
        const eclairage::Etat e = eclairage::etat();
        r.capturePng(out + "/realisme-ciel-" + atmosphere::nomMoment(mo) + "-" + carte + ".png");
        WFS_LOG("lumière (WFS-208) : %s, %s : EV %.2f (visé %.2f)", carte, atmosphere::nomMoment(mo), e.ev, e.evCible);
        infos << "WFS-208 : " << carte << ", " << atmosphere::nomMoment(mo) << " : " << fmt(e.heureUtc, 2) << " h UTC, astre h " << fmt(e.astreHauteur, 2) << "°, E sol "
              << fmt(e.eSol, 3) << " lx, EV100 " << fmt(e.ev, 2) << ", lumières de nuit " << (nuitM ? "allumées" : "éteintes") << ".\n";
        if (carte[0] == 'c' && carte[1] == 'a') {
            if (mo != atmosphere::Moment::Nuit)   // M-04 : lampadaires allumés ⇔ éclairement au sol sous 50 lx (± 5 %)
                rap.ajouter(ligne(std::string("lum.wfs208.m04.") + atmosphere::nomMoment(mo), std::string("M-04 : lampadaires ") + (nuitM ? "allumés" : "éteints") + ", moment « " + atmosphere::nomMoment(mo) + " » (éclairement au sol)",
                                  fmt(e.eSol, 1) + " lx", "allumés sous 50 lx (± 5 %), éteints au-dessus", "± 5 %",
                                  nuitM ? e.eSol < atmosphere::SEUIL_LAMPADAIRES_LX * 1.05 : e.eSol > atmosphere::SEUIL_LAMPADAIRES_LX * 0.95, "conception"));
            if (mo == atmosphere::Moment::Doree || mo == atmosphere::Moment::Bleue) {   // M-05 : hauteurs publiées
                const bool dor = mo == atmosphere::Moment::Doree; const double a = dor ? 0 : -6, b = dor ? 6 : -4;
                rap.ajouter(ligne(std::string("lum.wfs208.m05.") + atmosphere::nomMoment(mo), std::string("M-05 : hauteur du soleil à l'") + (dor ? "heure dorée" : "heure bleue"),
                                  fmt(e.astreHauteur, 2) + "°", "[" + fmt(a, 0) + "° ; " + fmt(b, 0) + "°] (définitions courantes : dorée 0 à 6°, bleue −4 à −6°)", "dans la plage",
                                  e.astreHauteur >= a && e.astreHauteur <= b, "a_confirmer"));
            }
        }
        g.shutdown();
        g_l.mode = AUCUN;
    }
    if (SEULM) return erreurs;
    {   // passage continu : heure dorée, 3 min de course, une image toutes les 0,5 s ; d'abord temps × 1 (référence : petits
        // recalages de l'exposition aux nouvelles sondes de la voiture qui roule, WFS-151), puis temps × 20 (1 h de soleil)
        double sautRef = 0, sautMax = 0, hSaut = 0, hDebut = 0, hFin = 0, dLogE = 0;
        for (int passe = 0; passe < 2; passe++) {
            const bool vite = passe == 1;
            preparer(g, r, "campagne", "jour", -1, ecrans::QUALITE_HAUTE);
            eclairage::choisirMoment((int)atmosphere::Moment::Doree); eclairage::accelererTemps(vite ? 20 : 1);
            g_l = EtatBanc{};
            if (!g.init(r, "golf", "campagne")) { WFS_ERR("lumière (WFS-208) : passage continu (heure dorée) : chargement de la Campagne impossible"); erreurs++; continue; }
            g.setAutopilot(true); g.camMode = 0; g.allerA(Ecran::Course); eclairage::remettreAZero();
            double tp = 0, evPrec = -100, ePrec = -1, tSonde = -1, saut = 0; int cap = 0;
            for (int i = 0; i <= 180 * 60; i++) {
                g.update(1.0 / 60); tp += 1.0 / 60;
                if (i % 30) continue;
                g.render(3000 + tp);
                const eclairage::Etat e = eclairage::etat();
                if (vite && i == 0) hDebut = e.astreHauteur;
                if (vite) hFin = e.astreHauteur;
                if (tSonde < 0 && eclairage::sondeCapturePrete()) tSonde = tp;   // 1re sonde capturée : recalage voulu de WFS-151 (hors mesure)
                if (evPrec > -50 && tSonde >= 0 && tp > tSonde + 2.0 && std::fabs(e.ev - evPrec) > saut) { saut = std::fabs(e.ev - evPrec); if (vite) hSaut = e.astreHauteur; }
                if (vite && ePrec > 0 && e.eSol > 0) dLogE = std::max(dLogE, std::fabs(std::log2(e.eSol / ePrec)));
                evPrec = e.ev; ePrec = e.eSol;
                if (vite && i % 3600 == 0 && cap < 4) r.capturePng(out + "/realisme-transition-" + std::to_string(cap++) + ".png");
            }
            (vite ? sautMax : sautRef) = saut;
            g.shutdown();
        }
        rap.ajouter(ligne("lum.wfs208.transition", "RG-02 : passage continu, heure dorée × 20 pendant 3 min (soleil de " + fmt(hDebut, 2) + "° à " + fmt(hFin, 2) + "°) : plus grand écart d'exposition entre deux images (0,5 s)",
                          fmt(sautMax, 3) + " EV", fmt(sautRef, 3) + " EV à × 1 (sondes de la voiture qui roule) + 0,1 EV au plus", "≤ référence + 0,1 EV", sautMax <= sautRef + 0.1, "conception",
                          "plus grand écart à h " + fmt(hSaut, 2) + "° ; éclairement au sol : au plus " + fmt(dLogE, 3) + " IL entre deux images"));
        eclairage::accelererTemps(1); eclairage::choisirMoment(-1);   // les bancs suivants gardent jour / nuit
    }
    // ---------------- tunnel de la Corniche : voiture dans le tunnel, puis sortie (adaptation de l'exposition)
    {
        int64_t debut = -1, fin = -1;
        preparer(g, r, "plage", "jour", -1, ecrans::QUALITE_HAUTE);
        g_l = EtatBanc{}; g_l.mode = LIBRE; g_l.monde = true; g_l.oeil = {0, 2, 0}; g_l.cible = {0, 2, 10};
        if (g.init(r, "golf", "plage")) {
            g.allerA(ECRAN); g.render(t);
            if (g_l.piste) for (const trace::Section& s : trace::sections(*g_l.piste)) if (s.type == trace::Type::Tunnel) { debut = s.debut; fin = s.fin; break; }
            g.shutdown();
        }
        if (debut < 0) { rap.nonMesurable("lumiere", "lum.tunnel", "tunnel de la Corniche", "aucune section tunnel dans le tracé de la plage"); }
        else {
            const int64_t depart = std::max<int64_t>(debut + 10, fin - 50);   // voiture à 60 m de la sortie
            preparer(g, r, "plage", "jour", (int)depart, ecrans::QUALITE_HAUTE);
            if (g.init(r, "golf", "plage")) {
                g.setAutopilot(false); g.camMode = 0; g.allerA(ECRAN);
                for (int i = 0; i < PAS; i++) { g.update(1.0 / 60); if (i % 10 == 0) g.render(t); }
                if (!g_l.piste) { g_l.mode = LIBRE; g.render(t); }
                Track& tr = *g_l.piste;
                const double s = g_l.dist;
                // voiture dans le tunnel (trois quarts avant, comme realisme-voiture-34-avant)
                g_l.fov = 50;
                g_l.oeil = pointRoute(tr, s, 4.5f, 3.2f, 1.25f); g_l.cible = pointRoute(tr, s, 0, 0, 0.65f);
                eclairage::Forcage f; f.actif = true; f.abscisseForcee = true; f.abscisse = g_l.position; eclairage::forcer(f);
                double tt = t;
                for (int i = 0; i < 60; i++) { tt += 1.0 / 30; g.render(tt); }   // 2 s : sondes du tunnel, exposition adaptée
                r.capturePng(out + "/realisme-voiture-tunnel.png");
                infos << "Voiture dans le tunnel : EV100 " << fmt(eclairage::etat().ev, 2) << " (visé " << fmt(eclairage::etat().evCible, 2) << "), luminance mesurée "
                      << fmt(eclairage::etat().luminanceMesuree, 1) << " cd/m², sonde capturée " << (eclairage::etat().sondeCaptureActive ? "oui" : "non") << ".\n";
                // sortie : caméra à 1,6 m, 20 m/s, du point 40 m avant la sortie jusqu'à 60 m après ; 30 images/s
                const double zSortie = (double)fin * SEG;   // abscisse de la sortie (unités)
                const double z0 = zSortie - 40 / M;
                double evSortie[3] = {0, 0, 0}; int n = 0;
                for (int i = 0; i <= 150; i++) {
                    const double tps = i / 30.0, z = z0 + 20 * tps / M;
                    const TrackPoint a = tr.point(z, 0), b = tr.point(z + 25 / M, 0);
                    g_l.oeil = {(float)a.x, (float)a.y + 1.6f, (float)a.z}; g_l.cible = {(float)b.x, (float)b.y + 1.2f, (float)b.z}; g_l.haut = {0, 1, 0}; g_l.fov = 60;
                    eclairage::Forcage fo; fo.actif = true; fo.abscisseForcee = true; fo.abscisse = z; fo.pointForce = true; fo.point = g_l.oeil; eclairage::forcer(fo);
                    tt += 1.0 / 30; g.render(tt);
                    const int capture = i == 54 ? 0 : i == 84 ? 1 : i == 150 ? 2 : -1;   // 4 m avant la sortie, 0,8 s après (16 m), puis 3 s après
                    if (capture >= 0) {
                        r.capturePng(out + "/realisme-sortie-tunnel-" + std::to_string(capture) + ".png");
                        evSortie[capture] = eclairage::etat().ev; n++;
                        mes << "  realisme-sortie-tunnel-" << capture << " : " << fmt(tps, 2) << " s, " << fmt((z - zSortie) * M, 1) << " m de la sortie, EV100 " << fmt(eclairage::etat().ev, 2)
                            << " (visé " << fmt(eclairage::etat().evCible, 2) << "), luminance mesurée " << fmt(eclairage::etat().luminanceMesuree, 1) << " cd/m²\n";
                    }
                }
                if (n == 3) {
                    const bool adapte = evSortie[1] > evSortie[0] + 0.3 && evSortie[2] > evSortie[1] + 0.3;
                    infos << "Sortie du tunnel : EV100 " << fmt(evSortie[0], 2) << " → " << fmt(evSortie[1], 2) << " → " << fmt(evSortie[2], 2)
                          << (adapte ? " : l'exposition s'adapte (critère d'acceptation)" : " : PAS D'ADAPTATION VISIBLE") << ".\n";
                    if (!adapte) { WFS_ERR("lumière (WFS-151) : sortie du tunnel de la Corniche : pas d'adaptation de l'exposition (EV100 %.2f → %.2f → %.2f ; + 0,3 voulu à chaque étape)", evSortie[0], evSortie[1], evSortie[2]); erreurs++; }
                }
                g.shutdown();
            } else { WFS_ERR("lumière (WFS-151) : tunnel de la Corniche : chargement de la plage impossible"); erreurs++; }
            g_l.mode = AUCUN; eclairage::forcer({});
        }
    }
    // ---------------- RG-06 : lampadaire de la Ville la nuit : éclairement de la chaussée sous la tête (DA-realisme.md § 3 : 10 à 30 lx)
    {
        preparer(g, r, "ville", "nuit", -1, ecrans::QUALITE_HAUTE);
        g_l = EtatBanc{};
        if (g.init(r, "golf", "ville")) {
            g.allerA(ECRAN);
            double eMax = -1;
            for (int i = 0; i < PAS; i++) { g.update(1.0 / 60); if (i % 10 == 0) { g.render(t); eMax = std::max(eMax, eclairage::etat().eLampadaireSol); } }
            const eclairage::Etat e = eclairage::etat();
            if (eMax < 0) rap.nonMesurable("lumiere", "lum.lampadaire.sol", "éclairement sous un lampadaire", "aucun lampadaire allumé près du départ de la Ville");
            else rap.ajouter(ligne("lum.lampadaire.sol", "RG-06 : éclairement de la chaussée sous un lampadaire de la Ville, la nuit (intensité du jeu convertie à EV100 4)",
                                   fmt(eMax, 1) + " lx", "[10 ; 30] lx (DA-realisme.md § 3)", "± 10 %", eMax >= 9 && eMax <= 33, "conception",
                                   "EV100 de la rue : " + fmt(e.ev, 2) + " (bornes de la nuit 3 à 5)"));
            g.shutdown();
        } else { WFS_ERR("lumière (WFS-151) : lampadaire de la Ville la nuit : chargement impossible"); erreurs++; }
    }
    // ---------------- RC-06 : coût de l'éclairage physique, même scène, même processus : sondes en place, puis rendu d'origine
    // (Ville et Campagne, départ, jour, Basse / Moyenne / Haute ; 60 images chacune, scène figée)
    mes << "  coût (ms par image, mur ; banc lavapipe : à comparer entre eux, pas à un téléphone) : carte | qualité | physique + IBL | origine | écart\n";
    for (const char* carte : {"ville", "campagne"}) for (int q : {ecrans::QUALITE_BASSE, ecrans::QUALITE_MOYENNE, ecrans::QUALITE_HAUTE}) {
        preparer(g, r, carte, "jour", -1, q);
        g_l = EtatBanc{};
        if (!g.init(r, "golf", carte)) { WFS_ERR("lumière (WFS-151) : coût de l'éclairage (%s, qualité %d) : chargement impossible", carte, q); erreurs++; continue; }
        g.allerA(ECRAN);
        for (int i = 0; i < PAS; i++) { g.update(1.0 / 60); if (i % 10 == 0) g.render(t); }
        attendreSondes(g, t);
        double ms[2];
        for (int k = 0; k < 2; k++) {
            eclairage::desactiver(k == 1);
            for (int i = 0; i < 3; i++) g.render(t);
            const double t0 = nowSeconds();
            for (int i = 0; i < 60; i++) g.render(t);
            ms[k] = (nowSeconds() - t0) / 60 * 1000;
        }
        eclairage::desactiver(false);
        const char* nq[3] = {"basse", "moyenne", "haute"};
        mes << "  " << carte << " | " << nq[q] << " | " << fmt(ms[0], 1) << " | " << fmt(ms[1], 1) << " | " << fmt((ms[0] / ms[1] - 1) * 100, 1) << " %\n";
        g.shutdown();
    }
    g_l = EtatBanc{};
    reglages.qualite = q0; ecrans::appliquerQualiteNiveau(r);
    unsetenv("WFS_MOMENT"); unsetenv("WFS_DEPART");
    return erreurs;
}

} // namespace wfs
