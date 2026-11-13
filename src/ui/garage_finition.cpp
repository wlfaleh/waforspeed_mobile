// WaForSpeed — garage : rangée « Finition » (WFS-491) : voir garage_finition.h.
#include "garage_finition.h"
#include <algorithm>
#include <string>
#include <string_view>
#include "../core/log.h"
#include "../game/commandes_tactiles.h"   // dessin2d : biseau en dégradé (échantillon)
#include "../game/personnalisation.h"
#include "../platform/manette.h"   // WFS-80 : bouton au focus de la manette
#include "../render/renderer.h"
#include "garage_achat.h"   // texte avec ★, équivalents
#include "langue.h"
#include "text.h"

namespace wfs::ecrans::garage {
namespace {
// mesures de la rangée, unités de 720 px de haut (× k) ; mêmes hauteurs que la rangée des couleurs (RG-04)
constexpr float BOUTON_H = 30, LIGNE_PRIX = 14, ECART = 8, BOUTON_MAX = 112, ECHANTILLON = 18, NOM = 190, ESPACE_RANGEES = 10;
constexpr const int (&ORDRE)[FinitionGarage::NB] = FinitionGarage::ORDRE;

const char* car(int index) { return CARS[std::clamp(index, 0, CAR_COUNT - 1)].id; }
const perso::Objet* objetFinition(int f) { return f >= 0 && f < BOUTIQUE_FINITIONS ? perso::objet(BOUTIQUE_FINITIONS_IDS[f]) : nullptr; }
long long prixFinition(const Sauvegarde& s, int f, const char* carId) {
    const perso::Objet* o = objetFinition(f);
    return o && !perso::possede(s, *o, carId) ? perso::prix(*o, carId) : 0;
}
const std::string& nomFinition(int f) { return ui::tr(("finition." + std::string(BOUTIQUE_FINITIONS_IDS[f])).c_str()); }

// échantillon 2D de la finition sur la couleur montrée (interface seulement : le vrai rendu est sur la voiture, WFS-490)
dessin2d::Teinte echantillon(int f, unsigned peinture, float op, float y0, float y1) {
    const vec3 c{((peinture >> 16) & 255) / 255.f, ((peinture >> 8) & 255) / 255.f, (peinture & 255) / 255.f};
    auto melange = [](vec3 a, vec3 b, float t) { return a + (b - a) * t; };
    auto v = [&](vec3 x) { return vec4{std::clamp(x.x, 0.f, 1.f), std::clamp(x.y, 0.f, 1.f), std::clamp(x.z, 0.f, 1.f), op}; };
    const vec3 blanc{1, 1, 1};
    vec3 haut = c, bas = c;
    switch (f) {   // indice dans BOUTIQUE_FINITIONS_IDS
        case 0: haut = melange(c, blanc, 0.35f); bas = c * 0.85f; break;               // brillant : reflet net
        case 1: haut = melange(c, blanc, 0.55f); bas = c * 0.45f; break;               // métallisé : flop marqué
        case 2: haut = melange(c, vec3{1, 0.95f, 1}, 0.6f); bas = melange(c, vec3{0.8f, 0.9f, 1}, 0.3f); break;   // nacré
        case 3: haut = bas = c * 0.8f; break;                                           // mat : sans reflet
        case 4: haut = melange(c, blanc, 0.15f); bas = c * 0.75f; break;               // satiné : reflet doux
        case 5: haut = c; bas = c * 0.3f; break;                                        // candy : profondeur
        case 6: haut = {0.93f, 0.95f, 0.97f}; bas = {0.30f, 0.33f, 0.37f}; break;      // chromé : miroir
        default: haut = {0.48f, 0.17f, 1.f}; bas = {0.f, 0.76f, 0.66f}; break;         // caméléon : violet → vert
    }
    return {v(haut), v(bas), y0, y1};
}
} // namespace

float FinitionGarage::hauteur(float k) { return (BOUTON_H + LIGNE_PRIX + ESPACE_RANGEES) * k; }

void FinitionGarage::ouvrir() {
    const Sauvegarde& s = sauvegarde();
    if (!s.actif()) return;   // banc d'essai sans sauvegarde : finitions de la session gardées
    for (int i = 0; i < CAR_COUNT; i++) {   // RG-03 : aperçu non acheté oublié, dernière finition possédée
        const std::string_view id = perso::aspectCourse(s, CARS[i].id).finition;
        fin_[i] = 0;
        for (int f = 0; f < BOUTIQUE_FINITIONS; f++) if (id == BOUTIQUE_FINITIONS_IDS[f]) fin_[i] = f;
    }
}

int FinitionGarage::finition(int index) const { return index >= 0 && index < CAR_COUNT ? fin_[index] : 0; }

void FinitionGarage::choisir(int ordre, int index) {
    if (ordre < 0 || ordre >= NB || index < 0 || index >= CAR_COUNT) return;
    const int f = catalogue(ordre);
    fin_[index] = f;
    Sauvegarde& s = sauvegarde();
    const long long prix = prixFinition(s, f, car(index));
    WFS_LOG("garage : finition %s%s", BOUTIQUE_FINITIONS_IDS[f], prix > 0 ? (" (aperçu, " + std::to_string(prix) + " ★)").c_str() : "");
    if (prix == 0 && s.actif()) {   // RG-03 : choix possédé gardé tout de suite, pour cette voiture seulement
        if (f == 0) s.finition.erase(car(index)); else s.finition[car(index)] = BOUTIQUE_FINITIONS_IDS[f];
        s.enregistrer();
    }
}

void FinitionGarage::dessiner(Renderer& r, const MiseEnPage& mp, const EtatGarage& e) {
    const float k = mp.k, op = mp.op, h = BOUTON_H * k, gap = ECART * k;
    const Sauvegarde& sv = sauvegarde();
    const char* carId = car(e.index);
    const float yy = mp.y1 + mp.statH + 14 * k + mp.paintH + ESPACE_RANGEES * k;
    float cx = mp.px0 + mp.padX;
    const std::string titre = ui::majuscules(ui::tr("garage.finition"));
    ui::text(r, cx, yy + 9 * k, 13 * k, titre, hex(MUTED, op), ui::GAUCHE, 0.16f, false);
    cx += ui::width(13 * k, titre, 0.16f) + 16 * k;
    // RG-04 : 8 boutons de même taille ; la largeur suit la place laissée par la colonne du nom (écrans étroits)
    const float place = mp.bx - 32 * k - NOM * k - cx;
    const float w = std::clamp((place + gap) / NB - gap, 40 * k, BOUTON_MAX * k);
    // une seule taille de texte pour les 8 noms : la plus grande qui fait tenir le plus long (12 px au plus, 8 px au moins)
    float px = 12 * k;
    for (int f = 0; f < NB; f++) {
        const float l = ui::width(12 * k, ui::majuscules(nomFinition(f)), 0.06f), dispo = w - (ECHANTILLON + 18) * k;
        if (l > dispo) px = std::min(px, std::max(8 * k, 12 * k * dispo / l));
    }
    const unsigned peinture = PAINTS[std::clamp(e.peinture, 0, NB_PEINTURES - 1)].hex;
    for (int o = 0; o < NB; o++) {
        const int f = ORDRE[o];
        const float x = cx + o * (w + gap);
        const bool on = f == e.finition;
        const long long prix = prixFinition(sv, f, carId);
        if (on) biseau(r, x - 4 * k, yy - 4 * k, w + 8 * k, h + 8 * k, h * 0.18f, hex(ACCENT2, 0.35f * op));   // halo
        biseau(r, x - 2 * k, yy - 2 * k, w + 4 * k, h + 4 * k, h * 0.18f, on ? hex(ACCENT2, op) : vec4{1, 1, 1, 0.08f * op});
        biseau(r, x, yy, w, h, h * 0.18f, hex(SURFACE2, op));
        const float es = ECHANTILLON * k, ex = x + 7 * k, ey = yy + (h - es) / 2;
        dessin2d::biseau(r, ex, ey, es, es, es * 0.18f, echantillon(f, peinture, op, ey, ey + es));
        if (prix > 0) cadenas(r, ex + es / 2 + es * 0.09f, ey + es * 0.55f, 11 * k, op);   // RG-01 : non possédée
        ui::text(r, ex + es + 6 * k, yy + (h - px) / 2 - 1 * k, px, ui::majuscules(nomFinition(f)), hex(on ? TEXTE : TAG, op), ui::GAUCHE, 0.06f, false);
        if (prix > 0) texteEtoiles(r, x + w / 2, yy + h + 3 * k, 10 * k, ui::nombre(prix) + " ★", hex(TAG, op), ui::CENTRE);
        g_z.finitions[o] = {x - 4 * k, yy - 4 * k, w + 8 * k, (BOUTON_H + LIGNE_PRIX) * k + 8 * k};
        manette::focus(g_z.finitions[o]);   // WFS-80 : A = toucher
    }
    // nom de la finition montrée et, si elle n'est pas possédée, son prix et ses équivalents (RC-E18-11)
    const float xn = mp.bx - 32 * k - NOM * k + 10 * k;
    const long long prixSel = prixFinition(sv, e.finition, carId);
    ui::text(r, xn, yy + 1 * k, 15 * k, nomFinition(std::clamp(e.finition, 0, NB - 1)), hex(TEXTE, op), ui::GAUCHE, 0, false);
    if (prixSel > 0) texteEtoiles(r, xn, yy + 21 * k, 11 * k, texteEquivalent(prixSel), hex(MUTED, op), ui::GAUCHE);
}

int FinitionGarage::appui(float x, float y) const {
    for (int o = 0; o < NB; o++) if (g_z.finitions[o].contient(x, y)) return o;
    return -1;
}

void FinitionGarage::viderZones() { for (Zone& z : g_z.finitions) z = Zone{}; }

} // namespace wfs::ecrans::garage
