// WFS-558 : digicode du duel en ligne (src/ui/salon_digicode.cpp) :
//   RG-06 : pavé 6 × 6, touches de 15 mm au pas de 19 mm, dans la face de 120 × 180 mm sous l'afficheur, sans chevauchement ;
//           les 31 caractères de duel::CODE_CHARS une fois chacun (2 à 9 puis A à Z sans I, L, O), ⌫ sur 2 colonnes, OK sur 3 ;
//   RG-05 : vue rapprochée : le digicode remplit la hauteur de l'écran (touches ≥ 85 px en 2400 × 1080), vue large : la voiture
//           et le poteau du digicode dans l'image ; glissé en 0,4 s, doux (aucun saut d'une image à l'autre), sans traverser le poteau.
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <cstring>
#include <string>
#include "../src/game/duel.h"
#include "../src/ui/salon_parking.h"
using namespace wfs;
using namespace wfs::ecrans::duel3d;

static int echecs = 0;
static void cas(const std::string& quoi, bool ok) { std::printf("digicode : %-100s → %s\n", quoi.c_str(), ok ? "OK" : "ÉCHEC"); if (!ok) echecs++; }

// point du repère du digicode → repère de la voiture (x du digicode = −x de la voiture, z sorti du poteau = −z)
static vec3 versVoiture(vec3 l) { return {POTEAU_X - l.x, DIGI_Y + l.y, POTEAU_FACE - l.z}; }
// projection (NDC, y vers le bas) d'un point du repère de la voiture par la caméra c
static bool ndc(const Camera& c, float aspect, vec3 p, vec2& out) {
    const mat4 VP = perspectiveVk(c.fovY, aspect, 0.05f) * lookAt(c.oeil, c.cible, {0, 1, 0});
    const float x = VP.at(0, 0) * p.x + VP.at(0, 1) * p.y + VP.at(0, 2) * p.z + VP.at(0, 3);
    const float y = VP.at(1, 0) * p.x + VP.at(1, 1) * p.y + VP.at(1, 2) * p.z + VP.at(1, 3);
    const float w = VP.at(3, 0) * p.x + VP.at(3, 1) * p.y + VP.at(3, 2) * p.z + VP.at(3, 3);
    if (w <= 1e-4f) return false;
    out = {x / w, y / w};
    return true;
}

int main() {
    // ---- RG-06 : caractères
    std::string vus;
    for (int k = 0; k < 31; k++) vus += caractere(k);
    bool tous = vus.size() == std::strlen(duel::CODE_CHARS);
    for (const char* p = duel::CODE_CHARS; *p; p++) tous = tous && std::count(vus.begin(), vus.end(), *p) == 1;
    cas("les 31 caractères du code, une fois chacun (« " + vus + " »)", tous);
    cas("2 à 9 d'abord, puis les lettres dans l'ordre", vus.substr(0, 8) == "23456789" && vus.substr(8) == "abcdefghjkmnpqrstuvwxyz");
    cas("effacer et OK sans caractère", caractere(EFFACER) == 0 && caractere(OK) == 0);
    // ---- RG-06 : dimensions, pas, place dans la face
    bool quinze = true, pas = true, dedans = true, separees = true, sousAfficheur = true;
    vec3 c[NB_TOUCHES]; vec2 d[NB_TOUCHES];
    for (int k = 0; k < NB_TOUCHES; k++) {
        touche(k, c[k], d[k]);
        quinze = quinze && std::fabs(d[k].y * 2 - 0.015f) < 1e-5f && (k >= 31 || std::fabs(d[k].x * 2 - 0.015f) < 1e-5f);
        dedans = dedans && std::fabs(c[k].x) + d[k].x <= FACE_L / 2 - 0.004f && std::fabs(c[k].y) + d[k].y <= FACE_H / 2 - 0.004f;
        sousAfficheur = sousAfficheur && c[k].y + d[k].y < Y_AFF - AFF_H / 2 - 0.002f;
    }
    for (int k = 1; k < 31; k++) if (k % COLONNES) pas = pas && std::fabs(c[k].x - c[k - 1].x - 0.019f) < 1e-5f;
    for (int k = COLONNES; k < 31; k++) pas = pas && std::fabs(c[k - COLONNES].y - c[k].y - 0.019f) < 1e-5f;
    for (int a = 0; a < NB_TOUCHES; a++) for (int b = a + 1; b < NB_TOUCHES; b++)
        separees = separees && (std::fabs(c[a].x - c[b].x) >= d[a].x + d[b].x + 0.003f || std::fabs(c[a].y - c[b].y) >= d[a].y + d[b].y + 0.003f);
    cas("touches de 15 mm (RG-06)", quinze);
    cas("pas de 19 mm entre deux touches voisines", pas);
    cas("toutes les touches dans la face de 120 × 180 mm (4 mm de bord au moins, sur le plat du chanfrein de 3 mm)", dedans);
    cas("aucune touche sous l'afficheur, 4 mm entre deux touches au moins", sousAfficheur && separees);
    cas("⌫ sur 2 colonnes et OK sur 3 colonnes, après Z sur la dernière ligne",
        std::fabs(d[EFFACER].x * 2 - (0.019f + 0.015f)) < 1e-5f && std::fabs(d[OK].x * 2 - (2 * 0.019f + 0.015f)) < 1e-5f &&
        c[EFFACER].y == c[30].y && c[OK].y == c[30].y && c[30].x < c[EFFACER].x && c[EFFACER].x < c[OK].x);
    // ---- RG-05 : vue rapprochée en 2400 × 1080 : digicode dans l'image, touches assez grandes pour le doigt
    const float aspect = 2400.f / 1080.f;
    {
        const Camera p = camera(1);
        vec2 h, b, t0, t1;
        const bool ok = ndc(p, aspect, versVoiture({0, FACE_H / 2, EPAIS}), h) && ndc(p, aspect, versVoiture({0, -FACE_H / 2, EPAIS}), b) &&
                        ndc(p, aspect, versVoiture({c[0].x, c[0].y - d[0].y, c[0].z}), t0) && ndc(p, aspect, versVoiture({c[0].x, c[0].y + d[0].y, c[0].z}), t1);
        const float hauteur = std::fabs(b.y - h.y) / 2, touchePx = std::fabs(t0.y - t1.y) / 2 * 1080;
        char m[160]; std::snprintf(m, sizeof m, "vue rapprochée : digicode sur %.0f %% de la hauteur, touche de %.0f px en 2400 × 1080", hauteur * 100, touchePx);
        cas(m, ok && hauteur > 0.80f && hauteur < 1.0f && touchePx >= 85);
    }
    // ---- vue large : voiture du joueur (avant à z ≈ −2,2 m) et poteau du digicode dans l'image, poteau à gauche
    {
        const Camera l = camera(0);
        vec2 v, po, voisine;
        const bool ok = ndc(l, aspect, {0, 0.7f, -2.2f}, v) && ndc(l, aspect, {POTEAU_X, 1.0f, POTEAU_FACE}, po) && ndc(l, aspect, {VOISINE_X, 0.7f, -2.2f}, voisine);
        cas("vue large : voiture, poteau (à gauche) et place voisine (à droite) dans l'image",
            ok && std::fabs(v.x) < 0.4f && po.x < -0.2f && po.x > -0.95f && voisine.x > 0.3f && voisine.x < 1.1f);
    }
    // ---- glissé : 0,4 s, doux, sans saut, sans traverser le poteau (œil toujours devant sa face)
    {
        float t = 0; int images = 0; bool doux = true, devant = true;
        vec3 avant = camera(0).oeil;
        const float moyen = length(camera(1).oeil - camera(0).oeil) / 24;   // pas moyen d'une image (0,4 s à 60 img/s)
        while (t < 1 && images < 1000) {
            t = avancer(t, true, 1.f / 60); images++;
            const Camera c1 = camera(t);
            doux = doux && length(c1.oeil - avant) <= 1.6f * moyen;   // courbe adoucie : jamais plus de 1,6 fois le pas moyen (aucun saut)
            devant = devant && c1.oeil.z < POTEAU_FACE - 0.05f;
            avant = c1.oeil;
        }
        cas("glissé vers le digicode en 0,4 s (" + std::to_string(images) + " images à 60 img/s)", images >= 23 && images <= 25);
        cas("glissé sans saut (≤ 1,6 × le pas moyen par image) et toujours devant le poteau", doux && devant);
        float r = 1; for (int i = 0; i < 30; i++) r = avancer(r, false, 1.f / 60);
        cas("retour au cadrage large (« Retour ») en 0,4 s", r == 0.f && avancer(0.f, false, 1.f) == 0.f && avancer(1.f, true, 1.f) == 1.f);
    }
    std::printf("digicode : %d échec(s)\n", echecs);
    return echecs ? 1 : 0;
}
