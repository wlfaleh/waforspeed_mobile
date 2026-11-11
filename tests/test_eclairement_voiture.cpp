// WFS-423 (RG-02, RG-03) : éclairement de la chaussée autour d'une voiture, calculé une fois par voiture sur le CPU
// (src/render/eclairement_voiture.h) : même formule que eclairementRouteAutour de shaders/vehicule.glsl (WFS-270).
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include "../src/render/eclairement_voiture.h"

using namespace wfs;
static int echecs = 0;
static void verifier(bool ok, const char* quoi) {
    std::printf("%s %s\n", ok ? "OK  " : "ECHEC", quoi);
    if (!ok) echecs++;
}
static bool proche(float a, float b, float tol = 1e-4f) { return std::fabs(a - b) <= tol * std::max(1.f, std::fabs(b)); }
static std::string lire(const char* chemin) {
    std::ifstream f(chemin);
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
// lampadaire au format du shader (renderer.cpp : PointLight, cône {-2, -1,5}, portée 26 m, décroissance 2)
static void lampadaire(eclairement_voiture::Lumieres& L, vec3 p, vec3 c) {
    const int i = L.n++;
    L.pos[i] = {p.x, p.y, p.z, 26.f}; L.dir[i] = {0, -1, 0, 2.f}; L.col[i] = {c.x, c.y, c.z, 0}; L.cone[i] = {-2.f, -1.5f, 0, 0};
}

int main() {
    namespace ev = eclairement_voiture;
    const mat4 I = mat4::identity();
    {
        ev::Lumieres L;
        const vec3 E = ev::routeAutour(L, I);
        verifier(E.x == 0 && E.y == 0 && E.z == 0, "aucune lumière (jour) : 0");
    }
    // un lampadaire à 6 m au-dessus du 2e point (6,5 m devant ; avant du modèle = −Z) : valeur attendue calculée à la main
    {
        ev::Lumieres L;
        const vec3 c{1.f, 0.7f, 0.4f}, pl{0, 6.f, -6.5f};
        lampadaire(L, pl, c);
        float attendu = 0;
        for (float devant : {4.f, 6.5f, 10.f}) {
            const vec3 p{0, 0.02f, -devant}, v = pl - p;
            const float d = length(v), NoL = v.y / d, att = std::pow(1.f - d / 26.f, 2.f);
            attendu += att * NoL;
        }
        attendu /= 3;
        const vec3 E = ev::routeAutour(L, I);
        verifier(proche(E.x, c.x * attendu) && proche(E.y, c.y * attendu) && proche(E.z, c.z * attendu), "lampadaire : moyenne des 3 points, 1 − d / portée au carré × cos");
        // même scène tournée de 90° autour de la verticale et déplacée : même éclairement (voiture du trafic dans un virage)
        const mat4 M = translate({50, 1, -20}) * rotateY(1.5707963f);
        ev::Lumieres L2;
        lampadaire(L2, M.transformPoint(pl), c);
        const vec3 E2 = ev::routeAutour(L2, M);
        verifier(proche(E2.x, E.x, 1e-3f) && proche(E2.y, E.y, 1e-3f), "RG-03 : même éclairement quelle que soit la place et le cap de la voiture");
        // lampadaire derrière la voiture, hors de portée (30 m) : rien
        ev::Lumieres L3;
        lampadaire(L3, {0, 6.f, 30.f}, c);
        const vec3 E3 = ev::routeAutour(L3, I);
        verifier(E3.x == 0, "lampadaire hors de portée : 0");
    }
    // feu de croisement : 1° sous l'horizon (sous la coupure de −0,57°, dans le plateau de −1,5°), à 5 m : intensité 1 et 1 / d²
    verifier(proche(ev::feuCroisement({0, -std::sin(0.0174533f), -std::cos(0.0174533f)}, {0, 0, -1}, 5.f, 200.f), 1.f / 25.f), "feu de croisement sous la coupure : 1 / d²");
    verifier(proche(ev::feuCroisement(normalize(vec3{0, 0.2f, -1}), {0, 0, -1}, 5.f, 200.f), 0.015f / 25.f, 0.05f), "feu de croisement au-dessus de la coupure : résidu 1,5 %");
    // phares d'une voiture en face (4 m de haut de route plus loin) : chaussée éclairée ; feu propre à moins de 2 m ignoré
    {
        ev::Lumieres L;
        const int i = L.n++;
        L.pos[i] = {0, 0.65f, -30.f, 200.f}; L.dir[i] = {0, 0, 1, 0}; L.col[i] = {100, 90, 80, 0}; L.cone[i] = {std::cos(1.45f), std::cos(1.45f), 1, 0};
        const vec3 E = ev::routeAutour(L, I);
        verifier(E.x > 0 && E.y > 0, "phares d'en face : la chaussée devant la voiture est éclairée");
        ev::Lumieres P;
        const int j = P.n++;
        P.pos[j] = {0, 0.65f, -4.f + 1.5f, 200.f}; P.dir[j] = {0, 0, -1, 0}; P.col[j] = {100, 90, 80, 0}; P.cone[j] = {std::cos(1.45f), std::cos(1.45f), 1, 0};
        verifier(ev::routeAutour(P, translate({0, 0, 0})).x >= 0, "phare à moins de 2 m d'un point : pas de valeur infinie");
    }
    // RG-02 : le shader lit la valeur du CPU (pc.nuit) et garde les mêmes 3 points que ce fichier
    {
        const std::string v = lire(WFS_SOURCE_DIR "/shaders/vehicule.glsl");
        verifier(v.find("pc.nuit.w > 0.5 ? pc.nuit.rgb : eclairementRouteAutour()") != std::string::npos, "vehicule.glsl : valeur du CPU lue en Basse");
        verifier(v.find("DEVANT[3] = float[3](4.0, 6.5, 10.0)") != std::string::npos, "vehicule.glsl : mêmes 3 points (4 ; 6,5 ; 10 m)");
        const std::string e = lire(WFS_SOURCE_DIR "/shaders/eclairage.glsl");
        verifier(e.find("i < n && vus < parPixel") != std::string::npos, "RG-01 : boucle des projecteurs bornée au vrai nombre (eclairage.glsl)");
        verifier(lire(WFS_SOURCE_DIR "/shaders/eau.glsl").find("for (int i = 0; i < n; i++)") != std::string::npos, "RG-01 : boucle des reflets de l'eau bornée (eau.glsl)");
    }
    std::printf(echecs ? "test_eclairement_voiture : %d échec(s)\n" : "test_eclairement_voiture : tout est bon\n", echecs);
    return echecs ? 1 : 0;
}
