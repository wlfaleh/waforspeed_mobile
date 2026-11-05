// WaForSpeed — module « chauffe » (WFS-123, épic E7), branché par les systèmes enfichables (WFS-30) : voir chauffe.h.
// Lit le niveau de chauffe dans la boucle de jeu, rétrograde la qualité d'un niveau en course (chauffe sévère, une fois
// par course), la rétablit au menu (chauffe retombée depuis 60 s), sans jamais l'enregistrer, et publie le plafond
// d'images/s de l'écran (30 hors course, 60 en course) pour android_main. Inactif sans source (banc sans WFS_CHAUFFE).
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/course5.h"
#include "../game/course_system.h"
#include "../game/game.h"
#include "../game/sauvegarde.h"
#include "../render/qualite_auto.h"
#include "../render/renderer.h"
#include "../ui/ecrans/reglages.h"
#include "../ui/ecrans/terrain.h"
#include "../ui/garage.h"
#include "chauffe.h"

namespace wfs {
namespace {

chauffe::Regulateur g_reg;   // gardé d'une partie à l'autre (Game::init est rappelé à chaque carte)
int g_niveauVu = -99;        // dernier niveau écrit au journal (RG-07 : une ligne par changement)
int g_plafondVu = 0;
int g_retros = 0;            // rétrogradations de chauffe depuis le lancement (banc d'essai)
double g_retroTemps = -1;    // raceTime de la dernière rétrogradation (banc d'essai)

class ChauffeSystem : public System {
public:
    const char* name() const override { return "chauffe"; }
    void init(Renderer& r, const GameView&) override { r_ = &r; }

    void update(double dt, const GameView& g) override {
        if (!chauffe::sourceActive() || !r_) return;
        const Course* c = activeCourse();
        const bool enCourse = g.ecran == Ecran::Course && !g.sceneDediee && c && c->playing();
        if (enCourse) chauffe::avancerTempsCourse(dt);
        const int st = chauffe::statut(), n = chauffe::niveauDepuisStatut(st);
        if (n != g_niveauVu) { g_niveauVu = n; WFS_LOG("CHAUFFE niveau=%d (statut Android %d)", n, st); }
        ecrans::Reglages& R = ecrans::reglages();
        int q = R.qualite;
        const int avant = q;
        const bool horsCourse = g.ecran != Ecran::Course && g.ecran != Ecran::Pause;
        const int e = g_reg.pas(dt, n, enCourse, horsCourse, c ? c->raceTime : -1, q);
        if (e != chauffe::Regulateur::RIEN) {
            R.qualite = q;   // non enregistré (RG-02) : qualiteChoisie et la sauvegarde ne changent pas
            ecrans::appliquerQualiteNiveau(*r_);
        }
        if (e == chauffe::Regulateur::RETRO) {
            bandeauGraphismesRapides();
            g_retros++; g_retroTemps = c ? c->raceTime : -1;
            WFS_LOG("CHAUFFE rétrogradation %s → %s (chauffe sévère, %.1f s de course, qualité %s)", chauffe::nomQualite(avant),
                    chauffe::nomQualite(q), g_retroTemps, R.qualiteChoisie ? "choisie par le joueur" : "automatique");
        } else if (e == chauffe::Regulateur::RETABLI) {
            WFS_LOG("CHAUFFE rétabli %s (chauffe retombée depuis %.0f s)", chauffe::nomQualite(q), chauffe::FROID_RETABLIR);
        }
        // RG-02 : la qualité rétrogradée n'est jamais enregistrée ; un autre réglage touché pendant la chauffe l'aurait
        // écrite dans la sauvegarde commune (reglages.cpp, ecrire) : la qualité choisie y est remise
        Sauvegarde& s = sauvegarde();
        if (g_reg.choisi >= 0 && R.qualiteChoisie && s.actif() && s.qualityChoisie && s.qualiteIndex() != g_reg.choisi) {
            s.setQualiteIndex(g_reg.choisi); s.enregistrer();
            WFS_LOG("CHAUFFE sauvegarde : qualité choisie (%s) gardée", chauffe::nomQualite(g_reg.choisi));
        }
    }

    // RG-06 : plafond de l'écran affiché, à chaque image (la pause ne fait pas d'update)
    void render(Renderer&, const GameView& g) override {
        if (!chauffe::sourceActive()) return;
        const int p = chauffe::plafondIps(g.ecran);
        chauffe::definirPlafond(p);
        if (p != g_plafondVu) { g_plafondVu = p; WFS_LOG("IPS plafond %d (écran %s)", p, nomEcran(g.ecran)); }
    }

private:
    Renderer* r_ = nullptr;
};
WFS_SYSTEM(ChauffeSystem, 361)   // juste après la qualité automatique (360)

} // namespace

// ---------------------------------------------------------------- banc d'essai (WFS_BANC=chauffe, RG-10)
int chauffe::benchChauffe(Game& g, Renderer& r, const std::string& out, Source source) {
    // RG-08 : chauffe sévère simulée après 5 s de course (sauf valeurs données au lancement)
    const bool envChauffe = std::getenv("WFS_CHAUFFE") != nullptr, envApres = std::getenv("WFS_CHAUFFE_APRES") != nullptr;
    if (!envChauffe) setenv("WFS_CHAUFFE", "2", 1);
    if (!envApres) setenv("WFS_CHAUFFE_APRES", "5", 1);
    const int niveauSimule = std::atoi(std::getenv("WFS_CHAUFFE"));
    const double apres = std::atof(std::getenv("WFS_CHAUFFE_APRES"));
    definirSource(source);
    courseBenchRules(true);
    ecrans::Reglages& R = ecrans::reglages();
    const int q0 = R.qualite; const bool choisie0 = R.qualiteChoisie;
    R.qualite = qualite::HAUTE; R.qualiteChoisie = true;   // RG-02 : qualité choisie par le joueur (Haute)
    ecrans::appliquerQualiteNiveau(r);
    const std::string sauv0 = sauvegarde().texte();
    const int retros0 = g_retros;
    int ecarts = 0;
    auto verifier = [&](bool ok, const char* quoi) {
        if (ok) WFS_LOG("chauffe (parcours) : %s (ok)", quoi); else { ecarts++; WFS_ERR("chauffe (parcours) : %s (ÉCHEC)", quoi); }
    };
    auto fin = [&]() {
        choixModeBanc(false); modeCourse() = ModeCourse::Checkpoint; courseBenchRules(false);
        definirSource(nullptr);
        if (!envChauffe) unsetenv("WFS_CHAUFFE");
        if (!envApres) unsetenv("WFS_CHAUFFE_APRES");
        R.qualite = q0; R.qualiteChoisie = choisie0; ecrans::appliquerQualiteNiveau(r);
    };
    if (!g.init(r, "golf", "ville")) { WFS_ERR("chauffe : chargement impossible"); fin(); return 1; }
    g.setAutopilot(false); g.camMode = 0;
    choixModeBanc(true);
    double t = 1700;
    auto images = [&](int n, int tous) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); if (i % tous == 0) g.render(t += tous / 60.0); } };
    auto capture = [&](const char* nom) {
        g.render(t += 0.02); g.render(t += 0.02); r.capturePng(out + "/" + nom + ".png");
        const Course* c = activeCourse();
        WFS_LOG("capture %s : écran %s, qualité %s, chauffe %d, %.1f s de course, plafond %d images/s", nom, nomEcran(g.ecran()),
                nomQualite(R.qualite), chauffeNiveau(), c ? c->raceTime : -1.0, plafondCourant());
    };
    int doigt = 80;
    auto appuyer = [&](float x, float y) {   // doigt Android : down, 4 mouvements, up (une image dessinée entre chaque)
        const int id = doigt++;
        g.render(t += 1.0 / 60);
        g.onTouch({id, x, y, true}); g.update(1.0 / 60); g.render(t += 1.0 / 60);
        for (int k = 1; k <= 4; k++) { g.onTouch({id, x + 0.4f * k, y, true}); g.update(1.0 / 60); g.render(t += 1.0 / 60); }
        g.onTouch({id, x + 2, y, false}); images(4, 2);
    };
    auto attendu = [&](Ecran e, const char* etape) {
        if (g.ecran() == e) WFS_LOG("chauffe (parcours) : %s → écran %s (attendu)", etape, nomEcran(e));
        else { ecarts++; WFS_ERR("chauffe (parcours) : %s → écran %s, attendu %s", etape, nomEcran(g.ecran()), nomEcran(e)); }
    };
    const float W = (float)r.width(), H = (float)r.height(), u = H / 100.f, cx = W / 2;
    // accueil → « Course solo » (panneau des modes) → carte « Checkpoint » → « Continuer » → garage → « Choisir » → terrain
    // → « Rouler » (mise en page de modes_ecran.cpp, marges nulles au banc, comme le parcours de la poursuite)
    g.allerA(Ecran::Menu); images(30, 10);
    verifier(plafondCourant() == 30, "accueil : plafond 30 images/s");
    ouvrirChoixMode(); images(10, 5);
    verifier(choixModeOuvert(), "« Course solo » : panneau des modes ouvert");
    const float gap = 2.4f * u, cw = std::min(42 * u, (W - 8 * u - 3 * gap) / 4), x0 = cx - (4 * cw + 3 * gap) / 2;
    appuyer(x0 + cw / 2, 26 * u + 17 * u);                                  // carte 01 « Checkpoint »
    appuyer(x0 + 4 * cw + 3 * gap - 14 * u, H - 8 * u - 6 * u + 4 * u);      // « Continuer »
    attendu(Ecran::Garage, "« Checkpoint » puis « Continuer »");
    verifier(modeCourse() == ModeCourse::Checkpoint, "mode Checkpoint retenu");
    float x = 0, y = 0;
    for (int i = 0; i < 600 && !ecrans::garageBouton("choisir", x, y); i++) images(1, 4);
    images(30, 4);
    verifier(plafondCourant() == 30, "garage : plafond 30 images/s");
    if (ecrans::garageBouton("choisir", x, y)) appuyer(x, y); else { ecarts++; WFS_ERR("chauffe (parcours) : « Choisir » absent"); }
    attendu(Ecran::Terrain, "garage puis « Choisir »");
    images(20, 4);
    if (ecrans::centreRouler(x, y)) appuyer(x, y); else { ecarts++; WFS_ERR("chauffe (parcours) : « Rouler » absent"); }
    for (int i = 0; i < 120 && g.ecran() != Ecran::Course; i++) images(1, 1);
    attendu(Ecran::Course, "terrain puis « Rouler »");
    choixModeBanc(false);
    // course : décompte (3 s), puis chauffe simulée après `apres` s de course → rétrogradation et bandeau
    g.setAutopilot(true);
    images(3 * 60, 8);
    verifier(plafondCourant() == 60, "décompte : plafond 60 images/s");
    for (int i = 0; i < 20 * 60 && g_retros == retros0; i++) { g.update(1.0 / 60); if (i % 8 == 0) g.render(t += 8 / 60.0); }
    images(20, 4);   // bandeau entré (2,5 s à l'écran)
    capture("chauffe-bandeau");
    char quoi[160];
    std::snprintf(quoi, sizeof quoi, "chauffe %d après %.0f s de course : rétrogradation Haute → Moyenne à %.1f s de course",
                  niveauSimule, apres, g_retroTemps);
    verifier(g_retros == retros0 + 1 && R.qualite == qualite::MOYENNE && std::fabs(g_retroTemps - apres) < 1.0, quoi);
    images(5 * 60, 10);   // la chauffe reste sévère : aucune autre rétrogradation dans cette course
    verifier(g_retros == retros0 + 1 && R.qualite == qualite::MOYENNE, "5 s de plus à chauffe sévère : une seule rétrogradation");
    verifier(R.qualiteChoisie, "qualité toujours « choisie par le joueur »");
    g.setAutopilot(false);
    courseBenchSetTime(0.05);
    images(80, 8);
    capture("chauffe-fin");
    // bouton « Menu » de l'écran de fin (CourseSystem::drawOver, comme le banc du menu)
    const float bw = 40 * u, bx = W / 2 - (bw * 1.7f + 2 * u) / 2 + bw + 2 * u;
    g.onTouch({79, bx + bw * 0.35f, 75 * u, true}); images(2, 1); g.onTouch({79, bx + bw * 0.35f, 75 * u, false});
    images(60, 6);
    attendu(Ecran::Menu, "fin puis « Menu »");
    verifier(plafondCourant() == 30, "retour à l'accueil : plafond 30 images/s");
    verifier(R.qualite == qualite::MOYENNE, "accueil, chauffe encore sévère : qualité Moyenne gardée");
    // la chauffe retombe : qualité choisie rétablie au menu 60 s plus tard, pas avant
    setenv("WFS_CHAUFFE", "0", 1);
    images(59 * 60, 30);
    verifier(R.qualite == qualite::MOYENNE, "59 s au niveau 0 : pas encore rétablie");
    images(2 * 60, 30);
    verifier(R.qualite == qualite::HAUTE, "60 s au niveau 0 : qualité choisie (Haute) rétablie au menu");
    capture("chauffe-retabli");
    verifier(sauvegarde().texte() == sauv0, "sauvegarde inchangée");
    WFS_LOG("chauffe (parcours) : accueil → Course solo → Checkpoint → Continuer → garage → Choisir → terrain → Rouler → course "
            "(rétrogradation à %.1f s) → fin → Menu → accueil : %d écart(s) %s", g_retroTemps, ecarts, ecarts ? "ÉCHEC" : "OK");
    // RG-03 : chauffe modérée → la résolution dynamique ne remonte plus (watchPerf simulé : 25 images/s puis 60)
    {
        setenv("WFS_CHAUFFE", "1", 1);
        qualite::Auto a; a.choixJoueur = true;
        for (int i = 0; i < 4 * 25; i++) a.imageChauffe(1.0 / 25, true, chauffeNiveau());
        const float bas = a.echelle;
        for (int i = 0; i < 6 * 60; i++) a.imageChauffe(1.0 / 60, true, chauffeNiveau());
        WFS_LOG("CHAUFFE niveau=%d simulé : 25 images/s pendant 4 s → échelle %.2f, puis 60 images/s pendant 6 s → échelle %.2f (bloquée)",
                chauffeNiveau(), bas, a.echelle);
        const bool bloquee = a.echelle == bas && bas < 1.f;
        setenv("WFS_CHAUFFE", "0", 1);
        for (int i = 0; i < 6 * 60; i++) a.imageChauffe(1.0 / 60, true, chauffeNiveau());
        WFS_LOG("CHAUFFE niveau=%d simulé : 60 images/s pendant 6 s → échelle %.2f (remonte)", chauffeNiveau(), a.echelle);
        if (bloquee && a.echelle > bas) WFS_LOG("chauffe (RG-03) : échelle bloquée à chauffe modérée, remontée sans chauffe : OK");
        else { ecarts++; WFS_ERR("chauffe (RG-03) : échelle %.2f → %.2f, attendu bloquée puis remontée (ÉCHEC)", bas, a.echelle); }
    }
    g.shutdown();
    fin();
    return ecarts;
}

} // namespace wfs
