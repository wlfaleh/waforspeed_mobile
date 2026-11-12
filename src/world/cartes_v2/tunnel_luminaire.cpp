// WaForSpeed — WFS-392 (RG-02) : luminaire de tunnel et chemin de câbles (voir tunnel_luminaire.h).
#include "tunnel_luminaire.h"

namespace wfs::carte::tunnel {

void pave(Geo& g, vec3 c, vec3 r, vec3 f, float w, float h, float l, vec2 uv, float fuite) {
    auto C = [&](float sx, float sy, float sz) { return c + r * (sx * w / 2 * (sy ? fuite : 1)) + f * (sz * l / 2) + vec3{0, sy * h, 0}; };
    quad(g, C(-1, 1, -1), C(1, 1, -1), C(1, 1, 1), C(-1, 1, 1), uv);    // dessus
    quad(g, C(-1, 0, 1), C(1, 0, 1), C(1, 0, -1), C(-1, 0, -1), uv);    // dessous
    quad(g, C(1, 0, -1), C(1, 0, 1), C(1, 1, 1), C(1, 1, -1), uv);      // côté r
    quad(g, C(-1, 0, 1), C(-1, 0, -1), C(-1, 1, -1), C(-1, 1, 1), uv);  // côté −r
    quad(g, C(1, 0, 1), C(-1, 0, 1), C(-1, 1, 1), C(1, 1, 1), uv);      // avant
    quad(g, C(-1, 0, -1), C(1, 0, -1), C(1, 1, -1), C(-1, 1, -1), uv);  // arrière
}

void luminaire(Geo& uni, Geo& emis, vec3 d, vec3 r, vec3 f, bool led, bool lod, vec2 uvCorps, vec2 uvAcier, vec2 uvVitre) {
    const Cotes& k = led ? LED : SODIUM;
    const float yVitre = -VITRE_SOUS_DALLE_M, yCadre = yVitre - 0.005f, yCorps = yCadre + 0.015f, yDessus = yCorps + k.h;
    const float yChemin = -(CHEMIN_SOUS_DALLE_M + CHEMIN_H_M);   // dessous du chemin de câbles
    auto P = [&](float x, float y, float z) { return d + r * x + f * z + vec3{0, y, 0}; };
    // vitre émissive, tournée vers la chaussée, en retrait de 5 mm dans son cadre
    const float vx = k.vitreL2 / 2, vz = k.vitreL / 2;
    quad(emis, P(-vx, yVitre, -vz), P(vx, yVitre, -vz), P(vx, yVitre, vz), P(-vx, yVitre, vz), uvVitre);
    if (lod) {   // loin : corps et cadre seulement
        pave(uni, P(0, yCorps, 0), r, f, k.l, k.h, k.L, uvCorps, k.fuite);
        pave(uni, P(0, yCadre, 0), r, f, k.vitreL2 + 0.06f, 0.015f, k.vitreL + 0.06f, uvCorps);
        return;
    }
    pave(uni, P(0, yCorps, 0), r, f, k.l, k.h, k.L, uvCorps, k.fuite);                                       // corps (profilé)
    for (const float s : {-1.f, 1.f}) pave(uni, P(0, yCorps - 0.005f, s * (k.L / 2 + 0.005f)), r, f, k.l + 0.02f, k.h + 0.01f, 0.01f, uvCorps, k.fuite);   // embouts
    // cadre de la vitre : 2 longs montants et 2 traverses de 3 cm, en saillie de 15 mm sous le corps
    for (const float s : {-1.f, 1.f}) {
        pave(uni, P(s * (vx + 0.015f), yCadre, 0), r, f, 0.03f, 0.015f, k.vitreL + 0.06f, uvCorps);
        pave(uni, P(0, yCadre, s * (vz + 0.015f)), r, f, k.vitreL2, 0.015f, 0.03f, uvCorps);
    }
    if (led) {   // 3 traverses entre les 4 modules, 6 ailettes de refroidissement de 25 mm sur le dessus
        for (int i = 1; i <= 3; i++) pave(uni, P(0, yCadre, -vz + k.vitreL * (float)i / 4), r, f, k.vitreL2, 0.012f, 0.012f, uvCorps);
        for (int i = 0; i < 6; i++) pave(uni, P(-k.l * 0.4f + k.l * 0.8f * (float)i / 5, yDessus, 0), r, f, 0.004f, 0.025f, k.L * 0.9f, uvCorps);
    }
    // 2 étriers : plat de 40 × 5 mm qui coiffe le corps, descend sur ses flancs et monte au chemin de câbles
    const float lDessus = k.l * k.fuite;
    for (const float s : {-1.f, 1.f}) {
        const float z = s * k.L * 0.28f;
        pave(uni, P(0, yDessus, z), r, f, lDessus + 0.02f, 0.005f, 0.04f, uvAcier);
        for (const float c : {-1.f, 1.f}) pave(uni, P(c * (k.l + lDessus) / 4 + c * 0.006f, yCorps + k.h * 0.4f, z), r, f, 0.005f, k.h * 0.6f, 0.04f, uvAcier);
        if (yChemin - yDessus > 0.006f) pave(uni, P(0, yDessus + 0.005f, z), r, f, 0.04f, yChemin - yDessus - 0.005f, 0.005f, uvAcier);
    }
    // 4 grenouillères de fermeture du cadre, sur les grands côtés
    for (const float s : {-1.f, 1.f}) for (const float c : {-1.f, 1.f})
        pave(uni, P(c * (k.l / 2 + 0.004f), yCorps - 0.002f, s * k.L * 0.2f), r, f, 0.012f, 0.03f, 0.025f, uvAcier);
    // presse-étoupe en bout et câble qui remonte au chemin de câbles
    const float zBout = k.L / 2 + 0.01f;
    pave(uni, P(0, yCorps + k.h * 0.5f - 0.0125f, zBout + 0.015f), r, f, 0.025f, 0.025f, 0.03f, uvAcier);
    pave(uni, P(0, yCorps + k.h * 0.5f, zBout + 0.036f), r, f, 0.012f, yChemin - (yCorps + k.h * 0.5f), 0.012f, uvAcier);
}

void cheminCables(Geo& uni, vec3 m, vec3 r, vec3 f, float L, bool lod, vec2 uvAcier) {
    const vec3 bas = m + vec3{0, -(CHEMIN_SOUS_DALLE_M + CHEMIN_H_M), 0};
    if (lod) { pave(uni, bas, r, f, CHEMIN_L_M, CHEMIN_H_M, L, uvAcier); return; }
    pave(uni, bas, r, f, CHEMIN_L_M, 0.004f, L, uvAcier);                                                         // fond du U
    for (const float s : {-1.f, 1.f}) pave(uni, bas + r * (s * (CHEMIN_L_M / 2 - 0.002f)), r, f, 0.004f, CHEMIN_H_M, L, uvAcier);   // ailes
    // suspentes : 2 tiges filetées de 12 mm tous les 2 m, du fond du chemin à la dalle
    const int n = std::max(1, (int)(L / 2.f + 0.5f));
    for (int i = 0; i < n; i++) for (const float s : {-1.f, 1.f})
        pave(uni, bas + r * (s * 0.07f) + f * (-L / 2 + L * ((float)i + 0.5f) / (float)n), r, f, 0.012f, CHEMIN_SOUS_DALLE_M + CHEMIN_H_M, 0.012f, uvAcier);
}

} // namespace wfs::carte::tunnel
