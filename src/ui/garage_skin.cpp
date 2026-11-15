// WaForSpeed — garage : rangée « Skin » et fenêtre de la skin offerte (WFS-494) : voir garage_skin.h.
#include "garage_skin.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "../core/log.h"
#include "../core/system.h"               // GameView
#include "../game/commandes_tactiles.h"   // dessin2d : échantillons, fond de la fenêtre
#include "../game/personnalisation.h"
#include "../game/skin_offerte.h"
#include "../platform/manette.h"          // WFS-80 : bouton au focus de la manette
#include "../render/renderer.h"
#include "../render/vehicule_skin.h"
#include "garage_achat.h"   // texte avec ★, équivalents, nombres
#include "garage_finition.h"
#include "langue.h"
#include "text.h"
#include "boutons.h"   // WFS-549 : boutons au modèle R19
#include "ecrans/atelier_calques.h"   // WFS-496 (accroche) : autocollants de l'atelier

namespace wfs::ecrans::garage {
namespace {
// mesures de la rangée, unités de 720 px de haut (× k) : mêmes que la rangée des finitions (WFS-491)
constexpr float BOUTON_H = 30, BOUTON_W = 104, LIGNE_PRIX = 14, ECART = 8, ECHANTILLON = 18, NOM = 190, ESPACE_RANGEES = 10, CHEVRON = 16;
constexpr const int (&ORDRE)[SkinGarage::NB] = SkinGarage::ORDRE;

const char* car(int index) { return CARS[std::clamp(index, 0, CAR_COUNT - 1)].id; }
const perso::Objet* objetSkin(int s) { return s >= 0 && s < BOUTIQUE_SKINS ? perso::objet(BOUTIQUE_SKINS_IDS[s]) : nullptr; }
bool possedee(const Sauvegarde& sv, int s, const char* carId) { const perso::Objet* o = objetSkin(s); return !o || perso::possede(sv, *o, carId); }
bool cadeauSeul(int s) { const perso::Objet* o = objetSkin(s); return o && perso::cadeau(*o); }
// prix à payer pour la montrer en course : 0 si aucune, possédée ou « cadeau » (pas en vente)
long long prixSkin(const Sauvegarde& sv, int s, const char* carId) {
    const perso::Objet* o = objetSkin(s);
    return o && !perso::possede(sv, *o, carId) && !perso::cadeau(*o) ? perso::prix(*o, carId) : 0;
}
const std::string& nomSkin(int s) { return ui::tr(s < 0 ? "garage.skin.aucune" : ("skin." + std::string(BOUTIQUE_SKINS_IDS[s])).c_str()); }
int indiceSkin(const char* id) { for (int s = 0; s < BOUTIQUE_SKINS; s++) if (id && std::strcmp(id, BOUTIQUE_SKINS_IDS[s]) == 0) return s; return -1; }

// échantillon 2D de la skin (interface seulement ; le vrai rendu est sur la voiture, WFS-492) : couleur A en haut, B en bas ;
// « Aucune » : la peinture montrée
dessin2d::Teinte echantillon(int s, unsigned peinture, float op, float y0, float y1) {
    float a[3], b[3];
    if (s >= 0 && vehicule_skin::couleurs(vehicule_skin::depuisId(BOUTIQUE_SKINS_IDS[s]), a, b))
        return {vec4{a[0], a[1], a[2], op}, vec4{b[0], b[1], b[2], op}, y0, y1};
    const vec4 c = hex(peinture, op);
    return {c, vec4{c.x * 0.8f, c.y * 0.8f, c.z * 0.8f, op}, y0, y1};
}

// skins de la fenêtre de la skin offerte : en vente pour cette voiture, dans l'ordre affiché de la rangée
std::vector<int> skinsOffrables(const char* carId) {
    std::vector<int> v;
    for (int o = 1; o < SkinGarage::NB; o++) {
        const perso::Objet* x = objetSkin(ORDRE[o]);
        if (x && perso::disponible(*x, carId) && !perso::cadeau(*x) && !perso::possede(sauvegarde(), *x, carId)) v.push_back(ORDRE[o]);
    }
    return v;
}
} // namespace

float SkinGarage::hauteur(float k) { return (BOUTON_H + LIGNE_PRIX + ESPACE_RANGEES) * k; }
bool SkinGarage::visible(int index) { return index >= 0 && index < CAR_COUNT && skins::aDesSkins(CARS[index].id); }
float SkinGarage::largeurBouton(float k) { return BOUTON_W * k; }

bool SkinGarage::police(float k, float& px) {
    // même règle que les finitions (RG-04 de WFS-491, E20) : 12 px, réduite de 20 % au plus pour faire tenir le plus long nom
    const float dispo = (BOUTON_W - ECHANTILLON - 7 - 6 - 6) * k;
    float l = 0;
    for (int o = 0; o < NB; o++) l = std::max(l, ui::width(12 * k, ui::majuscules(nomSkin(ORDRE[o])), 0.06f));
    px = l > dispo ? std::max(12 * k * 0.8f, 12 * k * dispo / l) : 12 * k;
    return l * px / (12 * k) <= dispo + 0.01f;
}

void SkinGarage::ouvrir() {
    fenetre_ = false; fenetreIndex_ = -1;
    const Sauvegarde& s = sauvegarde();
    if (!s.actif()) return;   // banc d'essai sans sauvegarde : skins de la session gardées
    for (int i = 0; i < CAR_COUNT; i++) skin_[i] = indiceSkin(perso::aspectCourse(s, CARS[i].id).skin);   // aperçu non acheté oublié
}

int SkinGarage::skin(int index) const {
    if (index < 0 || index >= CAR_COUNT) return -1;
    // fenêtre de la skin offerte ouverte : la skin touchée n'est qu'un aperçu sur la voiture (apercu() lit skin_) ; la rangée et le
    // bouton « Choisir / Acheter » gardent la skin d'avant (la skin offerte ne s'achète pas)
    return fenetre_ && index == fenetreIndex_ ? skinAvant_ : skin_[index];
}

void SkinGarage::appuyer(int zone, int index) {
    if (index < 0 || index >= CAR_COUNT) return;
    Sauvegarde& s = sauvegarde();
    const char* c = car(index);
    if (zone == CHEVRON_GAUCHE || zone == CHEVRON_DROIT) {   // défilement d'une page (boutons visibles)
        premier_ = std::clamp(premier_ + (zone == CHEVRON_GAUCHE ? -visibles_ : visibles_), 0, std::max(0, NB - visibles_));
        return;
    }
    if (zone == ACTION) {   // « Choisis ta skin offerte » (RG-02), sinon bon de skin du market sur la skin montrée
        if (skins::offerteDue(s, c)) { proposer(index); return; }
        const int k = skin_[index];
        if (k >= 0 && skins::utiliserBon(s, c, BOUTIQUE_SKINS_IDS[k]) && s.actif()) s.enregistrer();
        return;
    }
    if (zone < 0 || zone >= NB) return;
    const int k = catalogue(zone);
    // revue du lead dev (PR #419) : skin offerte encore due et skin en vente pas encore possédée : la fenêtre de la skin offerte s'ouvre
    // sur elle (jamais « Acheter N ★ » pour une skin que le joueur peut prendre gratuitement)
    if (k >= 0 && skins::offerteDue(s, c)) {
        const std::vector<int> v = skinsOffrables(c);
        if (const auto it = std::find(v.begin(), v.end(), k); it != v.end()) {
            proposer(index);
            if (fenetre_) { choixFenetre_ = (int)(it - v.begin()); skin_[index] = k; }
            return;
        }
    }
    skin_[index] = k;
    WFS_LOG("garage : skin %s%s", k < 0 ? "aucune" : BOUTIQUE_SKINS_IDS[k],
            !possedee(s, k, c) ? (cadeauSeul(k) ? " (aperçu, cadeau du jour seulement)" : (" (aperçu, " + std::to_string(prixSkin(s, k, c)) + " ★)").c_str()) : "");
    if (possedee(s, k, c) && s.actif()) {   // RG-05 : choix possédé gardé tout de suite, pour cette voiture seulement
        if (k < 0) s.skin.erase(c); else s.skin[c] = BOUTIQUE_SKINS_IDS[k];
        s.enregistrer();
    }
}

void SkinGarage::apercu(Renderer& r, const Model& m, int index) {
    const int k = m.ok && index >= 0 && index < CAR_COUNT ? skin_[index] : -1;
    // la texture posée habille-t-elle déjà CE modèle (et pas la voiture de la course : Game::setCar après un achat) ?
    bool surCeModele = false;
    for (uint32_t i = 0; i < m.mats.size() && !surCeModele; i++) surCeModele = vehicule_skin::materiau(m, i) != nullptr;
    if (k < 0) {   // aucune : la texture de l'aperçu (s'il y en a une) est rendue, la voiture est dessinée sans skin
        if (surCeModele) { r.vk().waitIdle(); vehicule_skin::liberer(r); }
        return;
    }
    const vehicule_skin::Etat e = vehicule_skin::etat();
    if (surCeModele && e.voiture == CARS[index].id && e.id == BOUTIQUE_SKINS_IDS[k]) return;
    r.vk().waitIdle();   // l'ancienne texture peut être lue par une image en vol
    vehicule_skin::poser(r, m, CARS[index].id, BOUTIQUE_SKINS_IDS[k]);
    pose_ = true;
}

void SkinGarage::quitter(Renderer& r, const GameView& g, bool reposer) {
    fenetre_ = false;
    if (!pose_) return;
    pose_ = false;
    r.vk().waitIdle();
    vehicule_skin::liberer(r);
    if (!reposer || !g.modele || !g.car) return;
    // même choix que Game::setCar (WFS-492, WFS-495) : skin possédée de la voiture de la course ; bancs : WFS_SKIN, WFS_AUTOCOLLANTS
    const char* sk = std::getenv("WFS_SKIN") ? std::getenv("WFS_SKIN") : sauvegarde().actif() ? perso::aspectCourse(sauvegarde(), g.car->id).skin : "";
    const char* ac = std::getenv("WFS_AUTOCOLLANTS");
    vehicule_skin::poser(r, *g.modele, g.car->id, sk, ac ? std::string(ac) : atelier::calquesCourse(g.car->id));   // WFS-496 (accroche) : autocollants de l'atelier
}

// ---------- rangée « Skin » ----------
void SkinGarage::dessiner(Renderer& r, const MiseEnPage& mp, const EtatGarage& e) {
    viderZones();
    if (!visible(e.index)) return;
    const float k = mp.k, op = mp.op, h = BOUTON_H * k, gap = ECART * k, w = BOUTON_W * k, pas = w + gap;
    const Sauvegarde& sv = sauvegarde();
    const char* carId = car(e.index);
    const float yy = mp.y1 + mp.statH + 14 * k + mp.paintH + FinitionGarage::hauteur(k) + ESPACE_RANGEES * k;
    float cx = mp.px0 + mp.padX;
    const std::string titre = ui::majuscules(ui::tr("garage.skin"));
    ui::text(r, cx, yy + 9 * k, 13 * k, titre, hex(MUTED, op), ui::GAUCHE, 0.16f, false);
    // les boutons commencent au même x que ceux des finitions (titres de longueurs différentes : colonne alignée)
    cx += std::max(ui::width(13 * k, titre, 0.16f), ui::width(13 * k, ui::majuscules(ui::tr("garage.finition")), 0.16f)) + 16 * k;
    const float place = mp.bx - 32 * k - NOM * k - cx;
    const bool defile = NB * pas - gap > place;
    const float x0 = defile ? cx + CHEVRON * k : cx;
    visibles_ = defile ? std::clamp((int)((place - 2 * CHEVRON * k + gap) / pas), 1, NB) : NB;
    int sel = 0;
    for (int o = 0; o < NB; o++) if (ORDRE[o] == e.skin) sel = o;
    if (sel != selVue_ || e.index != indexVu_) {   // nouvelle skin montrée ou autre voiture : la rendre visible
        if (sel < premier_) premier_ = sel;
        if (sel >= premier_ + visibles_) premier_ = sel - visibles_ + 1;
        selVue_ = sel; indexVu_ = e.index;
    }
    premier_ = std::clamp(premier_, 0, NB - visibles_);
    float px = 12 * k;
    police(k, px);
    for (int c = 0; c < 2; c++) {   // chevrons (seulement s'il reste des boutons de ce côté)
        const bool reste = c == 0 ? premier_ > 0 : premier_ + visibles_ < NB;
        if (!defile || !reste) continue;
        const float xc = c == 0 ? cx + CHEVRON * k * 0.4f : x0 + visibles_ * pas - gap + CHEVRON * k * 0.6f, yc = yy + h / 2, d = c == 0 ? 1.f : -1.f;
        const vec2 ch[3] = {{xc + 4 * k * d, yc - 8 * k}, {xc - 4 * k * d, yc}, {xc + 4 * k * d, yc + 8 * k}};
        dessin2d::trait(r, ch, 3, 2.4f * k, hex(ACCENT2, op));
        g_z.chevronsSkin[c] = {xc - CHEVRON * k * 0.6f, yy - 4 * k, CHEVRON * k * 1.2f, h + 8 * k};
        manette::focus(g_z.chevronsSkin[c]);
    }
    const unsigned peinture = PAINTS[std::clamp(e.peinture, 0, NB_PEINTURES - 1)].hex;
    for (int o = premier_; o < premier_ + visibles_; o++) {
        const int s = ORDRE[o];
        const float x = x0 + (o - premier_) * pas;
        const bool on = o == sel, aMoi = possedee(sv, s, carId), cadeau = !aMoi && cadeauSeul(s);
        const long long prix = prixSkin(sv, s, carId);
        if (on) biseau(r, x - 4 * k, yy - 4 * k, w + 8 * k, h + 8 * k, h * 0.18f, hex(ACCENT2, 0.35f * op));   // halo
        biseau(r, x - 2 * k, yy - 2 * k, w + 4 * k, h + 4 * k, h * 0.18f, on ? hex(ACCENT2, op) : vec4{1, 1, 1, 0.08f * op});
        biseau(r, x, yy, w, h, h * 0.18f, hex(SURFACE2, op));
        const float es = ECHANTILLON * k, ex = x + 7 * k, ey = yy + (h - es) / 2;
        dessin2d::biseau(r, ex, ey, es, es, es * 0.18f, echantillon(s, peinture, op, ey, ey + es));
        if (!aMoi) cadenas(r, ex + es / 2 + es * 0.09f, ey + es * 0.55f, 11 * k, op);   // RG-01, RG-04 : non possédée
        ui::text(r, ex + es + 6 * k, yy + (h - px) / 2 - 1 * k, px, ui::majuscules(nomSkin(s)), hex(on ? TEXTE : TAG, op), ui::GAUCHE, 0.06f, false);
        if (prix > 0) texteEtoiles(r, x + w / 2, yy + h + 3 * k, 10 * k, ui::nombre(prix) + " ★", hex(TAG, op), ui::CENTRE);
        else if (cadeau) ui::text(r, x + w / 2, yy + h + 3 * k, 10 * k, ui::tr("garage.skin.cadeau_court"), hex(ACCENT2, op), ui::CENTRE, 0, false);
        g_z.skins[o] = {x - 4 * k, yy - 4 * k, w + 8 * k, (BOUTON_H + LIGNE_PRIX) * k + 8 * k};
        manette::focus(g_z.skins[o]);   // WFS-80 : A = toucher
    }
    // nom de la skin montrée ; dessous : « Utiliser un bon (N) », son prix et ses équivalents, ou « Bientôt dans le cadeau du jour »
    const float xn = mp.bx - 32 * k - NOM * k + 10 * k, wn = NOM * k - 20 * k;
    ui::textBoite(r, xn, yy - 2 * k, wn, 20 * k, 15 * k, nomSkin(e.skin), hex(TEXTE, op), ui::GAUCHE, 0, false);
    const bool offerte = skins::offerteDue(sv, carId);
    if (offerte || (e.skin >= 0 && skins::bonUtilisable(sv, carId, BOUTIQUE_SKINS_IDS[e.skin]))) {
        // RG-02 : « Choisis ta skin offerte » (orange : action principale de la rangée) ; bon du market : « Utiliser un bon (N) » (cyan)
        const Zone z{xn, yy + 19 * k, wn, 22 * k};
        const unsigned c = offerte ? ACCENT : ACCENT2;
        biseau(r, z.x, z.y, z.w, z.h, 6 * k, hex(c, op));
        if (!offerte) biseau(r, z.x + 1.5f * k, z.y + 1.5f * k, z.w - 3 * k, z.h - 3 * k, 6 * k, hex(SURFACE2, op));
        const std::string t = offerte ? ui::tr("garage.skin.offerte.bouton") : ui::tr("garage.skin.bon", std::to_string(sv.market.bonsSkin));
        ui::textBoite(r, z.x + z.w / 2, z.y, z.w - 12 * k, z.h, 11 * k, ui::majuscules(t), hex(offerte ? ON_ACCENT : ACCENT2, op), ui::CENTRE, 0.1f, false);
        g_z.actionSkin = {z.x - 2 * k, z.y - 4 * k, z.w + 4 * k, z.h + 8 * k};
        manette::focus(g_z.actionSkin);
    } else if (const long long p = prixSkin(sv, e.skin, carId); p > 0) {
        texteEtoiles(r, xn, yy + 21 * k, 11 * k, texteEquivalent(p), hex(MUTED, op), ui::GAUCHE);
    } else if (!possedee(sv, e.skin, carId) && cadeauSeul(e.skin)) {
        ui::textBoite(r, xn, yy + 19 * k, wn, 16 * k, 11 * k, ui::tr("garage.skin.cadeau"), hex(ACCENT2, op), ui::GAUCHE, 0, false);
    }
}

int SkinGarage::appui(float x, float y) const {
    if (g_z.actionSkin.contient(x, y)) return ACTION;
    for (int o = 0; o < NB; o++) if (g_z.skins[o].contient(x, y)) return o;
    for (int c = 0; c < 2; c++) if (g_z.chevronsSkin[c].contient(x, y)) return c == 0 ? CHEVRON_GAUCHE : CHEVRON_DROIT;
    return -1;
}

void SkinGarage::viderZones() {
    for (Zone& z : g_z.skins) z = Zone{};
    g_z.chevronsSkin[0] = g_z.chevronsSkin[1] = g_z.actionSkin = Zone{};
}

// ---------- fenêtre « Choisis ta skin offerte » (RG-02) ----------
void SkinGarage::proposer(int index) {
    if (fenetre_ || index < 0 || index >= CAR_COUNT) return;
    if (!skins::offerteDue(sauvegarde(), car(index)) || skinsOffrables(car(index)).empty()) return;
    fenetre_ = true; fenetreIndex_ = index; choixFenetre_ = 0; skinAvant_ = skin_[index];
    skin_[index] = skinsOffrables(car(index))[0];   // 1re skin montrée tout de suite sur la voiture (aperçu)
    WFS_LOG("garage : fenêtre « Choisis ta skin offerte » pour %s", car(index));
}

bool SkinGarage::toucherFenetre(float x, float y, int index) {
    if (!fenetre_) return false;
    const std::vector<int> v = skinsOffrables(car(fenetreIndex_));
    for (int i = 0; i < (int)v.size() && i < BOUTIQUE_SKINS; i++)
        if (zSkinsFenetre_[i].contient(x, y)) { choixFenetre_ = i; skin_[fenetreIndex_] = v[i]; return true; }
    if (zPlusTard_.contient(x, y)) {   // la skin reste due : bouton « Choisis ta skin offerte » de la rangée, ou appui sur une skin en vente
        skin_[fenetreIndex_] = skinAvant_; fenetre_ = false;
        WFS_LOG("garage : skin offerte de %s : « Plus tard »", car(fenetreIndex_));
        return true;
    }
    if (zPrendre_.contient(x, y) && choixFenetre_ < (int)v.size()) {
        Sauvegarde& s = sauvegarde();
        if (skins::offrir(s, car(fenetreIndex_), BOUTIQUE_SKINS_IDS[v[choixFenetre_]])) { skin_[fenetreIndex_] = v[choixFenetre_]; s.enregistrer(); }
        fenetre_ = false;
        return true;
    }
    (void)index;
    return false;
}

void SkinGarage::dessinerFenetre(Renderer& r, const MiseEnPage& mp, int index) {
    zPlusTard_ = zPrendre_ = Zone{};
    for (Zone& z : zSkinsFenetre_) z = Zone{};
    if (!fenetre_) return;
    if (index != fenetreIndex_) { skin_[fenetreIndex_] = skinAvant_; fenetre_ = false; return; }   // autre voiture : rien de choisi
    const float k = mp.k, W = mp.W, H = mp.H;
    const char* carId = car(fenetreIndex_);
    const std::vector<int> v = skinsOffrables(carId);
    if (v.empty()) { fenetre_ = false; return; }
    // voile léger : la voiture reste visible au-dessus de la fenêtre (aperçu de la skin choisie)
    r.uiRect(0, 0, W, H, {0.02f, 0.02f, 0.03f, 0.45f});
    const int n = (int)v.size();
    const float pad = 28 * k, cw = 168 * k, ch = 64 * k, cg = 12 * k, bw = 200 * k, hb = 52 * k, hg = 46 * k, gapB = 16 * k;
    const float wMax = W - mp.gl - mp.gr;
    const float w = std::min(wMax, std::max(n * cw + (n - 1) * cg, 2 * bw + gapB) + 2 * pad);
    const float h = 22 * k + 12 * k + 14 * k + 24 * k + 10 * k + 14 * k + 20 * k + ch + 22 * k + hb + 24 * k;
    const float x = mp.gl + (wMax - w) / 2, y = H - mp.gb - h;   // en bas, sur le panneau : la voiture reste au-dessus
    const float b = 18 * k;
    const vec2 p[5] = {{x, y}, {x + w - b, y}, {x + w, y + b}, {x + w, y + h}, {x, y + h}};
    dessin2d::polygone(r, p, 5, dessin2d::uni(hex(0x14161F, 0.97f)));
    dessin2d::contour(r, p, 5, std::max(1.f, k), BORD);
    r.uiRect(x, y, w - b, 3 * k, hex(ACCENT));
    float yy = y + 22 * k;
    ui::text(r, x + w / 2, yy, 12 * k, ui::majuscules(ui::tr("garage.skin.offerte.eyebrow")), hex(ACCENT2), ui::CENTRE, 0.22f, false);
    yy += 12 * k + 14 * k;
    ui::textBoite(r, x + w / 2, yy, w - 2 * pad, 26 * k, 24 * k, ui::tr("garage.skin.offerte.titre"), hex(TEXTE), ui::CENTRE, 0, false);
    yy += 24 * k + 10 * k;
    ui::textBoite(r, x + w / 2, yy, w - 2 * pad, 16 * k, 14 * k, ui::tr("garage.skin.offerte.texte"), hex(TAG), ui::CENTRE, 0, false);
    yy += 14 * k + 20 * k;
    // une carte par skin en vente : échantillon, nom, valeur en ★ ; la carte choisie a le liseré cyan
    float px = 15 * k;
    for (int i = 0; i < n; i++) px = std::min(px, 15 * k * std::min(1.f, (cw - 64 * k) / std::max(1.f, ui::width(15 * k, ui::majuscules(nomSkin(v[i])), 0.06f))));
    px = std::max(px, 15 * k * 0.8f);
    float cx = x + (w - (n * cw + (n - 1) * cg)) / 2;
    for (int i = 0; i < n && i < BOUTIQUE_SKINS; i++, cx += cw + cg) {
        const bool on = i == choixFenetre_;
        if (on) biseau(r, cx - 4 * k, yy - 4 * k, cw + 8 * k, ch + 8 * k, 10 * k, hex(ACCENT2, 0.35f));
        biseau(r, cx - 2 * k, yy - 2 * k, cw + 4 * k, ch + 4 * k, 10 * k, on ? hex(ACCENT2) : vec4{1, 1, 1, 0.10f});
        biseau(r, cx, yy, cw, ch, 10 * k, hex(SURFACE2));
        const float es = 36 * k, ex = cx + 12 * k, ey = yy + (ch - es) / 2;
        dessin2d::biseau(r, ex, ey, es, es, es * 0.18f, echantillon(v[i], 0x808080, 1, ey, ey + es));
        ui::text(r, ex + es + 10 * k, yy + 12 * k, px, ui::majuscules(nomSkin(v[i])), hex(on ? TEXTE : TAG), ui::GAUCHE, 0.06f, false);
        const perso::Objet* o = objetSkin(v[i]);
        texteEtoiles(r, ex + es + 10 * k, yy + 36 * k, 11 * k, ui::tr("garage.skin.offerte.valeur", nombreLangue(o ? perso::prix(*o, carId) : 0)), hex(MUTED), ui::GAUCHE);
        zSkinsFenetre_[i] = {cx - 4 * k, yy - 4 * k, cw + 8 * k, ch + 8 * k};
        manette::focus(zSkinsFenetre_[i]);
    }
    yy += ch + 22 * k;
    // boutons : « Plus tard » (.btn--ghost 200 × 46) et « Prendre » (.btn orange 200 × 52), tailles du garage
    const float xa = x + (w - (2 * bw + gapB)) / 2;
    zPlusTard_ = {xa, yy + (hb - hg) / 2, bw, hg};
    // WFS-549 (RG-01, RG-02, RG-07) : « Plus tard » en autocollant, « Prendre » en Start/Stop, mêmes rectangles qu'avant
    ui::boutons::autocollant(r, zPlusTard_.x, zPlusTard_.y, bw, hg, ui::majuscules(ui::tr("garage.skin.offerte.plustard")), 1, false, 17 * k);
    zPrendre_ = {xa + bw + gapB, yy, bw, hb};
    ui::boutons::startStop(r, zPrendre_.x, yy, bw, hb, ui::majuscules(ui::tr("garage.skin.offerte.prendre")), 1, false, "", 19 * k);
    manette::focus(zPrendre_, true);
    manette::focus(zPlusTard_);
}

bool SkinGarage::boutonFenetre(const char* nom, float& x, float& y) const {
    const Zone* z = std::strcmp(nom, "plustard") == 0 ? &zPlusTard_ : std::strcmp(nom, "prendre") == 0 ? &zPrendre_ : nullptr;
    if (!z && std::strncmp(nom, "skin", 4) == 0) { const int i = std::atoi(nom + 4); if (i >= 0 && i < BOUTIQUE_SKINS) z = &zSkinsFenetre_[i]; }
    if (!z || z->w <= 0) return false;
    x = z->x + z->w / 2; y = z->y + z->h / 2;
    return true;
}

} // namespace wfs::ecrans::garage
