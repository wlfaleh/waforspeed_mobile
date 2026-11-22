// WFS-173 (RG-12) : mini-carte du parcours au HUD, calculs purs de src/ui/minicarte.h.
//  - place : 7 écrans × commandes « Normaux » / « Grands » × droitier / gaucher × marges (nulles, encoche de 90 px à
//    gauche, barre de 24 px en bas) × Checkpoint / Poursuite : la carte est dans la zone sûre, de 100 à 150 k de côté (ou
//    absente), et ne recoupe aucun cadre de RG-03 (écart de 8 k au moins) ;
//  - projection GPS : joueur à 70 % de la hauteur, 500 m = côté intérieur, route vers le haut ; segments découpés dans le carré ;
//  - parcours entier : polyligne dans le carré, départ sous l'arrivée ; abscisse hors de [0, arrivée] ramenée au bout.
#include <cmath>
#include <cstdio>
#include <vector>
#include "../src/ui/minicarte.h"
using namespace wfs;
using namespace wfs::minicarte;

static int echecs = 0, cas = 0;
#define VERIFIE(c, ...) do { cas++; if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

int main() {
    // ---- place (RG-02, RG-03)
    const float ecrans[7][2] = {{1280, 720}, {1600, 720}, {1920, 1080}, {2400, 1080}, {3200, 1440}, {1024, 768}, {2560, 1600}};
    const Marges marges[3] = {{}, {90, 0, 0, 0}, {0, 0, 0, 24}};
    const char* nomsMarges[3] = {"sans marge", "encoche 90 px", "barre 24 px"};
    int dessinees = 0, masquees = 0;
    for (const auto& e : ecrans) for (int grands = 0; grands < 2; grands++) for (int gaucher = 0; gaucher < 2; gaucher++)
        for (int mi = 0; mi < 3; mi++) for (int pours = 0; pours < 2; pours++) {
            const float W = e[0], H = e[1], k = H / 720.f;
            grandsBoutons() = grands;
            const Marges& m = marges[mi];
            const Obstacles o = obstacles(W, H, m, pours, gaucher);
            const Cadre c = placer(W, H, m, o, pours);
            char quoi[160];
            std::snprintf(quoi, sizeof quoi, "%.0fx%.0f, boutons %s, %s, %s, %s", W, H, grands ? "Grands" : "Normaux", gaucher ? "gaucher" : "droitier",
                          nomsMarges[mi], pours ? "Poursuite" : "Checkpoint");
            if (c.w <= 0) { masquees++; std::printf("place : %s : carte masquée (pas assez de place)\n", quoi); continue; }
            dessinees++;
            VERIFIE(c.w >= c.h - 1e-3f && c.w <= c.h * LARGEUR_SUR_HAUT + 1e-3f && c.h >= COTE_MIN * k - 1e-3f && c.h <= COTE * k + 1e-3f, "%s : hauteur %.1f hors de [%.1f ; %.1f]", quoi, c.h, COTE_MIN * k, COTE * k);
            VERIFIE(c.x >= m.gauche && c.x + c.w <= W - m.droite && c.y >= m.haut && c.y + c.h <= H - m.bas, "%s : carte hors de la zone sûre", quoi);
            VERIFIE(std::fabs(c.x + c.w - (W - m.droite - GOUTTIERE * k)) < 1e-3f, "%s : bord droit pas sur celui du score", quoi);
            VERIFIE(std::fabs(c.y - (m.haut + (pours ? HAUT_POURSUITE : HAUT) * k)) < 1e-3f, "%s : haut à %.1f", quoi, c.y);
            for (int i = 0; i < o.n; i++)
                VERIFIE(!recoupe(c, o.c[i], ECART * k - 1e-3f), "%s : recoupe le cadre %d (%.0f, %.0f, %.0f x %.0f)", quoi, i, o.c[i].x, o.c[i].y, o.c[i].w, o.c[i].h);
            if (W == 1280 && H == 720 && !grands && !gaucher && mi == 0 && !pours) std::printf("place : 1280x720 : carte (%.0f, %.0f) côté %.0f\n", c.x, c.y, c.w);
            if (W == 2400 && H == 1080 && !grands && !gaucher && mi == 0) std::printf("place : 2400x1080 %s : carte (%.0f, %.0f) côté %.0f\n", pours ? "Poursuite" : "Checkpoint", c.x, c.y, c.w);
        }
    grandsBoutons() = false;
    std::printf("place : %d cas, %d cartes dessinées, %d masquées\n", dessinees + masquees, dessinees, masquees);
    VERIFIE(dessinees >= 7 * 2 * 2 * 3 * 2 - 8, "trop de cartes masquées (%d)", masquees);
    {   // les deux écrans de référence ont la taille pleine (WFS-557 : 149 px de haut en 1280 x 720, 223,5 px en 2400 x 1080)
        const Cadre a = placer(1280, 720, {}, obstacles(1280, 720, {}, false, false), false);
        const Cadre b = placer(2400, 1080, {}, obstacles(2400, 1080, {}, false, false), false);
        VERIFIE(std::fabs(a.h - 149) < 1e-3f && std::fabs(b.h - 223.5f) < 1e-3f, "tailles de référence : %.1f et %.1f", a.h, b.h);
        // un obstacle sous le coin : le carré rétrécit et garde l'écart ; trop près : pas de carte
        Obstacles o; o.ajouter({1100, 270, 200, 40});
        const Cadre c = placer(1280, 720, {}, o, false);
        VERIFIE(c.w > 0 && std::fabs(c.y + c.h - (270 - ECART)) < 1e-3f, "rétrécissement : hauteur %.1f", c.h);
        Obstacles o2; o2.ajouter({1100, 150, 200, 40});
        VERIFIE(placer(1280, 720, {}, o2, false).w == 0, "sous 100 k : la carte doit être masquée");
    }

    // ---- projection GPS (RG-06)
    {
        const Cadre carre{1114, 116, 150, 150};
        const VueGps v = vueGps(carre, 1);
        const float in = 150 - 2 * MARGE_INT;
        VERIFIE(std::fabs(v.joueurPx.y - (carre.y + MARGE_INT + 0.70f * in)) < 1e-3f, "GPS : joueur pas à 70 %% de la hauteur");
        VERIFIE(std::fabs(v.pxParM * 500 - in) < 1e-3f, "GPS : 500 m ≠ côté intérieur");
        // route droite vers le « nord » du monde (z croissant), le joueur au départ : 350 m devant au haut, 150 m derrière au bas
        const vec2 haut{0, 1}, droite0{1, 0};
        const Repere R{{0, 0}, haut, droiteDe(haut, vect(haut, droite0))};
        const vec2 avant = versEcranGps(v, versCarte(R, {0, 350})), arriere = versEcranGps(v, versCarte(R, {0, -150}));
        VERIFIE(std::fabs(avant.y - (carre.y + MARGE_INT)) < 1e-2f && std::fabs(arriere.y - (carre.y + carre.h - MARGE_INT)) < 1e-2f,
                "GPS : fenêtre de 150 m derrière à 350 m devant mal placée (%.2f, %.2f)", avant.y, arriere.y);
        const vec2 aDroite = versEcranGps(v, versCarte(R, {10, 0}));
        VERIFIE(aDroite.x > v.joueurPx.x, "GPS : la droite de la route doit être à droite de l'écran");
        // virage à angle droit vers la droite : la route sort du carré ; chaque segment découpé reste dans le carré (0,5 px près)
        int sortis = 0;
        for (int i = 0; i + 1 < GPS_POINTS; i++) {
            auto pt = [&](int j) { const float s = -150 + 5.f * j; return versEcranGps(v, versCarte(R, s <= 0 ? vec2{0, s} : vec2{s, 0})); };
            vec2 a = pt(i), b = pt(i + 1);
            if (!decouper(a, b, carre.x, carre.y, carre.x + carre.w, carre.y + carre.h)) { sortis++; continue; }
            for (const vec2& p : {a, b})
                VERIFIE(p.x >= carre.x - 0.5f && p.x <= carre.x + carre.w + 0.5f && p.y >= carre.y - 0.5f && p.y <= carre.y + carre.h + 0.5f,
                        "GPS : segment %d découpé hors du carré (%.2f, %.2f)", i, p.x, p.y);
        }
        VERIFIE(sortis > 0, "GPS : le virage serré du test devrait sortir du carré");
        vec2 a{0, 0}, b{10, 10};
        VERIFIE(!decouper(a, b, 20, 20, 30, 30), "découpe : segment dehors accepté");
        a = {0, 25}; b = {50, 25};
        VERIFIE(decouper(a, b, 20, 20, 30, 30) && std::fabs(a.x - 20) < 1e-4f && std::fabs(b.x - 30) < 1e-4f, "découpe : segment traversant mal coupé");
    }

    // ---- parcours entier (RG-07)
    {
        const Cadre carre{1114, 116, 150, 150};
        // parcours en S de 4 km, puis en épingle (départ et arrivée proches)
        for (int forme = 0; forme < 2; forme++) {
            std::vector<vec2> w(PARCOURS_MAX);
            for (int i = 0; i < PARCOURS_MAX; i++) {
                const float s = 4000.f * i / (PARCOURS_MAX - 1);
                w[i] = forme == 0 ? vec2{300 * std::sin(s / 700), s} : vec2{s < 2000 ? 0.f : 60.f, s < 2000 ? s : 4000 - s};
            }
            const vec2 d = moins(w.back(), w.front());
            const vec2 cap0 = unitaire(moins(w[1], w[0]));
            const vec2 haut = std::sqrt(scal(d, d)) >= DROITE_MIN_M ? unitaire(d) : cap0;
            const Repere R{w[0], haut, droiteDe(haut, vect(cap0, {1, 0}))};
            std::vector<vec2> uv(PARCOURS_MAX), px(PARCOURS_MAX);
            for (int i = 0; i < PARCOURS_MAX; i++) uv[i] = versCarte(R, w[i]);
            const Ajustement a = ajuster(uv.data(), PARCOURS_MAX, carre, 1);
            bool dedans = true;
            for (int i = 0; i < PARCOURS_MAX; i++) {
                px[i] = versEcran(a, uv[i]);
                dedans = dedans && px[i].x >= carre.x + MARGE_INT - 0.01f && px[i].x <= carre.x + carre.w - MARGE_INT + 0.01f &&
                         px[i].y >= carre.y + MARGE_INT - 0.01f && px[i].y <= carre.y + carre.h - MARGE_INT + 0.01f;
            }
            VERIFIE(dedans, "parcours %d : polyligne hors du carré intérieur", forme);
            if (forme == 0) VERIFIE(px.front().y > px.back().y, "parcours : le départ doit être sous l'arrivée (%.1f, %.1f)", px.front().y, px.back().y);
            else VERIFIE(px[1].y < px[0].y, "parcours en épingle : le cap du départ doit monter");
            const vec2 debut = surParcours(px.data(), PARCOURS_MAX, -500, 4000), finP = surParcours(px.data(), PARCOURS_MAX, 9e9, 4000);
            VERIFIE(debut.x == px.front().x && debut.y == px.front().y && finP.x == px.back().x && finP.y == px.back().y, "parcours : abscisse hors du parcours mal ramenée");
            int i = -1; const vec2 mil = surParcours(px.data(), PARCOURS_MAX, 2000, 4000, &i);
            VERIFIE(i >= 0 && i < PARCOURS_MAX - 1 && std::isfinite(mil.x), "parcours : milieu mal placé");
        }
        VERIFIE(ramener(-1, 10) == 0 && ramener(11, 10) == 10 && ramener(5, 10) == 5, "ramener");
    }
    // ---- WFS-596 : vue Cockpit : la carte va sur l'écran central de la planche (RG-02) ; projection en perspective (RG-01, RG-03)
    {
        VERIFIE(lieuCarte(true, true) == LieuCarte::Ecran, "WFS-596 : vue Cockpit avec intérieur : carte sur l'écran central, plus au HUD");
        VERIFIE(lieuCarte(false, true) == LieuCarte::Hud && lieuCarte(false, false) == LieuCarte::Hud,
                "WFS-596 : vues Poursuite, Capot, Proche : mini-carte du HUD inchangée");
        VERIFIE(lieuCarte(true, false) == LieuCarte::Hud, "WFS-596 : cockpit commun (sans écran) : mini-carte du HUD");
        const Cadre c = cadreEcran(0.209f, 0.079f);   // Giulia : 209 × 79 mm
        VERIFIE(c.h == COTE && std::fabs(c.w / c.h - 0.209f / 0.079f) < 1e-4f, "WFS-596 : cadre virtuel aux proportions de l'écran réel");
        // quadrilatère en perspective (écran tourné et vu de côté) : coins exacts, centre sur l'intersection des diagonales
        const vec2 q[4] = {{820, 470}, {1010, 452}, {1016, 530}, {818, 541}};
        const Homographie H = carreVers(q);
        const vec2 uv[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        float e = 0;
        for (int i = 0; i < 4; i++) { const vec2 p = appliquer(H, uv[i]); e = std::max(e, std::fabs(p.x - q[i].x) + std::fabs(p.y - q[i].y)); }
        VERIFIE(e < 1e-2f, "WFS-596 : homographie : coins à %.4f px des coins de l'écran", e);
        const vec2 d1 = moins(q[2], q[0]), d2 = moins(q[3], q[1]);
        const float t = vect(moins(q[1], q[0]), d2) / vect(d1, d2);
        const vec2 x = plus(q[0], fois(d1, t)), m = appliquer(H, {0.5f, 0.5f});
        VERIFIE(std::fabs(m.x - x.x) < 0.05f && std::fabs(m.y - x.y) < 0.05f, "WFS-596 : homographie : centre (%.2f, %.2f) au lieu de (%.2f, %.2f)", m.x, m.y, x.x, x.y);
        // parallélogramme : application affine (milieu d'un bord au milieu du bord)
        const vec2 r[4] = {{100, 100}, {300, 100}, {300, 180}, {100, 180}};
        const vec2 b = appliquer(carreVers(r), {0.5f, 1});
        VERIFIE(std::fabs(b.x - 200) < 1e-3f && std::fabs(b.y - 180) < 1e-3f, "WFS-596 : homographie d'un rectangle");
        // projection d'un point du monde : devant la caméra (repère de vue identité, projection simple), derrière : refusé
        mat4 vp = mat4::identity(); vp.at(3, 2) = -1; vp.at(3, 3) = 0;   // w = −z : l'avant est vers −z
        vec2 o{};
        VERIFIE(versUi(vp, {0, 0, -2}, 1280, 720, o) && std::fabs(o.x - 640) < 1e-3f && std::fabs(o.y - 360) < 1e-3f, "WFS-596 : point devant au centre");
        VERIFIE(!versUi(vp, {0, 0, 2}, 1280, 720, o), "WFS-596 : point derrière la caméra refusé");
        VERIFIE(ECRAN_HZ <= 30 && ECRAN_NUIT < 1 && ECRAN_NUIT > 0.3f, "WFS-596 : écran recalculé au plus 30 fois par seconde, plus sombre la nuit");
        auto cyan = [](unsigned h) { const int R = (h >> 16) & 255, G = (h >> 8) & 255, B = h & 255; return G > 150 && B > 150 && R < 100; };
        VERIFIE(!cyan(ECRAN_FOND) && !cyan(ECRAN_ITINERAIRE) && !cyan(ECRAN_GAINE) && !cyan(ECRAN_CHECKPOINT), "WFS-596 : aucun cyan sur l'écran de la voiture");
    }
    if (echecs) { std::printf("test_minicarte : %d échec(s) sur %d contrôles\n", echecs, cas); return 1; }
    std::printf("test_minicarte : OK (%d contrôles)\n", cas);
    return 0;
}
