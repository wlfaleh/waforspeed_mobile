// WaForSpeed — bandeau « Voiture prêtée » du menu (WFS-502) : voir pret_bandeau.h.
#include "pret_bandeau.h"
#include <algorithm>
#include <cstdio>
#include <string>
#include "reglages_appui.h"   // appui::dansCible : cible de 48 dp (comme le bouton « Cadeau du jour »)
#include "textes_banc.h"      // banc « textes » (journal des textes de cet écran)
#include "../garage.h"        // garageRegarder, garageOuvrirSur
#include "../garage_achat.h"  // texteEtoiles (★ dessinée), texteDuree, nombreLangue
#include "cadeau_jour.h"   // WFS-550 : place de la carte « Cadeau du jour » (la carte du prêt se pose au-dessus)
#include "../jauges.h"   // WFS-550 (RG-03) : anneau du temps restant
#include "../langue.h"
#include "../petit_ecran.h"
#include "../text.h"
#include "../verre.h"    // WFS-550 (RG-03) : pilule de verre
#include "../../core/configgame.h"
#include "../../core/log.h"
#include "../../game/boutique.h"
#include "../../game/cars.h"
#include "../../game/commandes_tactiles.h"   // dessin2d
#include "../../game/pret.h"
#include "../../platform/manette.h"
#include "../../render/renderer.h"

namespace wfs {
namespace ecrans::pret_bandeau {
namespace {

vec4 hex(unsigned h, float a = 1) { return {((h >> 16) & 255) / 255.f, ((h >> 8) & 255) / 255.f, (h & 255) / 255.f, a}; }
const unsigned TEXTE = 0xF2F4F8, MUTED = 0x8A90A6, ACCENT = 0xFF6A00, ACCENT2 = 0x00E5FF, ON_ACCENT = 0x0A0B10, TAG = 0xC9CDDA, DANGER = 0xFF1F4B;

struct Zone { float x = 0, y = 0, w = 0, h = 0; };
Zone g_bandeau, g_bouton;
float g_dp = 1;
bool g_messageVu = false;   // message (expiré / fin) dessiné au menu : oublié en quittant le menu

// nom affiché (configgame.json, section « voitures ») : jamais une marque
std::string nomVoiture(const std::string& id) {
    const CarDef& c = carById(id.c_str());
    return id == c.id ? std::string(modele(c)) : id;
}

enum class Cas { Rien, Attente, Actif, Expire, Fin };
Cas casActuel(std::string& voiture) {
    const EtatCadeau& c = sauvegarde().cadeau;
    switch (pret::message(c, &voiture)) {   // un message passe avant un nouveau prêt (il est montré une fois)
        case pret::Message::Expire: return Cas::Expire;
        case pret::Message::Fin: return Cas::Fin;
        default: break;
    }
    voiture = c.pretVoiture;
    switch (pret::etat(c)) {
        case pret::Etat::EnAttente: return Cas::Attente;
        case pret::Etat::Actif: return Cas::Actif;
        default: return Cas::Rien;
    }
}

void oublier() {
    Sauvegarde& s = sauvegarde();
    if (pret::message(s.cadeau) == pret::Message::Aucun) return;
    pret::oublierMessage(s.cadeau);
    s.enregistrer();
    g_messageVu = false;
    WFS_LOG("prêt (WFS-502) : message du menu vu");
}

// le message est vu quand le joueur quitte le menu après l'avoir eu sous les yeux (une seule fois, RG-01 et RG-05)
class PretBandeau final : public System {
public:
    const char* name() const override { return "pret-bandeau"; }
    void onEcran(Ecran ancien, Ecran, const GameView&) override {
        if (ancien == Ecran::Menu && g_messageVu) oublier();
        g_bandeau = g_bouton = Zone{};
    }
};

// WFS-550 (maquette 26) : petite voiture vue de face au trait (toit, caisse, deux phares), largeur s, centrée en (cx, cy)
void voitureDeFace(Renderer& r, float cx, float cy, float s, const vec4& c) {
    const float ep = std::max(1.3f, s * 0.08f), hw = s / 2, hh = s * 0.36f;
    const vec2 toit[4] = {{cx - hw * 0.78f, cy - hh * 0.05f}, {cx - hw * 0.55f, cy - hh * 0.75f}, {cx + hw * 0.55f, cy - hh * 0.75f}, {cx + hw * 0.78f, cy - hh * 0.05f}};
    dessin2d::trait(r, toit, 4, ep, c);
    const vec2 caisse[5] = {{cx - hw, cy - hh * 0.05f}, {cx + hw, cy - hh * 0.05f}, {cx + hw, cy + hh * 0.75f}, {cx - hw, cy + hh * 0.75f}, {cx - hw, cy - hh * 0.05f}};
    dessin2d::trait(r, caisse, 5, ep, c);
    for (float sg : {-1.f, 1.f}) {   // phares
        const vec2 ph[2] = {{cx + sg * hw * 0.70f, cy + hh * 0.30f}, {cx + sg * hw * 0.38f, cy + hh * 0.30f}};
        dessin2d::trait(r, ph, 2, ep * 1.2f, c);
    }
    const vec2 pied[2] = {{cx - hw * 0.8f, cy + hh * 0.75f}, {cx - hw * 0.8f, cy + hh * 1.0f}}, pied2[2] = {{cx + hw * 0.8f, cy + hh * 0.75f}, {cx + hw * 0.8f, cy + hh * 1.0f}};
    dessin2d::trait(r, pied, 2, ep, c); dessin2d::trait(r, pied2, 2, ep, c);   // roues
}

} // namespace

bool visible() { std::string v; return casActuel(v) != Cas::Rien; }

void dessiner(Renderer& r, float W, float H, const Marges& m, bool presse) {
    g_bandeau = g_bouton = Zone{};
    std::string id;
    const Cas cas = casActuel(id);
    if (cas == Cas::Rien || !ui::fontReady()) return;
    const textes::Journal journal;
    const Sauvegarde& s = sauvegarde();
    const long long t = pret::maintenant();
    const bool cp = ui::petitEcran(W, H);
    const float k = cp ? ui::unitePetitEcran(W, H) : H / 720.f, kb = H / 720.f;
    g_dp = ui::pixelsParDp(W);
    // WFS-550 (maquette 26 ; ordre de l'utilisateur du 2026-11-15 : la maquette à 100 %) : carte de verre arrondie en pilule,
    // bordée de cyan (rouge : prêt perdu), anneau cyan → orange du temps restant autour d'une voiture vue de face ;
    // « VOITURE PRÊTÉE · <NOM> » en petites capitales grises, durée en grand (« 2 h 41 au volant »), aide dessous.
    // Mesures de la maquette (1600 × 720) : 380 × 81, au-dessus de la carte « Cadeau du jour » (12 px), même bord droit.
    const std::string nom = nomVoiture(id);
    auto duree = [](long long sec) {   // « 2 h 41 » (maquette), « 41 min » sous 1 h
        const long long mn = sec <= 0 ? 0 : (sec + 59) / 60;
        if (mn < 60) return ui::tr("pret.duree.min", std::to_string(mn));
        const long long r2 = mn % 60;
        return ui::tr("pret.carte.duree", std::to_string(mn / 60), (r2 < 10 ? "0" : "") + std::to_string(r2));
    };
    std::string titre, grand, suite, aide, bouton;
    switch (cas) {
        case Cas::Attente: titre = ui::tr("pret.carte.titre", nom); grand = duree(pret::resteActivation(s.cadeau, t)); suite = ui::tr("pret.carte.a_activer");
                           bouton = "pret.bandeau.activer"; break;
        case Cas::Actif: titre = ui::tr("pret.carte.titre", nom); grand = duree(pret::restant(s.cadeau, t)); suite = ui::tr("pret.carte.au_volant");
                         aide = ui::tr("pret.carte.toucher"); break;
        case Cas::Expire: {
            char h[16]; std::snprintf(h, sizeof h, "%g", config().cadeau.activationHeures);
            titre = ui::tr("pret.bandeau.expire", nom); aide = ui::tr("pret.bandeau.expire.aide", h); g_messageVu = true; break;
        }
        case Cas::Fin: titre = ui::tr("pret.bandeau.fin", nom); aide = ui::tr("pret.garder", garage::nombreLangue(std::max(0, boutique::prixVoiture(id))));
                       bouton = "pret.bandeau.garage"; g_messageVu = true; break;
        default: break;
    }
    titre = ui::majuscules(titre);
    float droite, haut;
    cadeau_jour::placeCarte(W, H, m, droite, haut);
    const float w = 380 * kb, h = 81 * kb, x = droite - w, y = haut - 12 * kb - h;
    verre::Style st; st.rayon = h / 2; st.lisere = 1.5f * kb; st.bord = cas == Cas::Expire ? DANGER : ACCENT2;
    verre::panneau(r, x, y, w, h, st);
    const float bp = 15 * kb, bw = bouton.empty() ? 0 : ui::width(bp, ui::majuscules(ui::tr(bouton.c_str())), 0.1f) + 36 * kb, bh = 34 * kb;
    if (presse && bw <= 0) {   // appuyé : voile cyan dans la carte
        vec2 c[verre::NP_CONTOUR];
        verre::contourArrondi(x, y, w, h, h / 2, c);
        dessin2d::polygone(r, c, verre::NP_CONTOUR, dessin2d::uni(hex(ACCENT2, 0.18f)));
    }
    // anneau (rayon 27, à 43 du bord gauche) : remplissage du temps restant, départ en haut ; voiture vue de face au centre
    float fr = 0;
    if (cas == Cas::Attente) fr = jauges::fractionRestant(pret::resteActivation(s.cadeau, t), (long long)(config().cadeau.activationHeures * 3600));
    else if (cas == Cas::Actif) fr = jauges::fractionRestant(pret::restant(s.cadeau, t), (long long)(config().cadeau.pretHeures * 3600));
    const float cx = x + 43 * kb, cy = y + h / 2, ra = 27 * kb;
    dessin2d::disqueRadial(r, cx, cy, ra - 3 * kb, cx, cy - ra * 0.3f, ra, hex(0x1A1C24, 0.95f), hex(0x0E0F14, 0.95f));
    jauges::anneauGradue(r, cx, cy, ra, 5 * kb, fr, 1, -jauges::PI_J * 0.17f, 0, false);
    voitureDeFace(r, cx, cy, 22 * kb, hex(TEXTE, 0.95f));
    const float xt = x + 88 * kb;
    { const textes::NomPropre np; verre::surTitre(r, xt, y + 9 * kb, 12.5f * kb, titre); }   // nom de la voiture : inchangé en anglais
    if (!grand.empty()) {
        const float gp = 32 * kb;
        ui::text(r, xt, y + 20 * kb, gp, grand, hex(TEXTE), ui::GAUCHE, 0, false, true);
        ui::text(r, xt + ui::width(gp, grand) + 8 * kb, y + 20 * kb + gp * 0.30f, 21 * kb, suite, hex(TAG), ui::GAUCHE, 0, false, true);
    }
    if (!aide.empty()) garage::texteEtoiles(r, xt, grand.empty() ? y + 34 * kb : y + 56 * kb, (grand.empty() ? 15 : 13) * k, aide, hex(grand.empty() ? TEXTE : MUTED), ui::GAUCHE);
    g_bandeau = {x, y, w, h};
    if (bw > 0) {   // action principale de la carte (orange en biseau), à droite
        const float bx = x + w - 22 * kb - bw, by = y + (h - bh) / 2;
        dessin2d::biseau(r, bx, by, bw, bh, 10 * kb, dessin2d::uni(hex(presse ? 0xFF9A4D : ACCENT)));
        ui::textBoite(r, bx + bw / 2, by, bw - 16 * kb, bh, bp, ui::majuscules(ui::tr(bouton.c_str())), hex(ON_ACCENT), ui::CENTRE, 0.1f, false);
        g_bouton = {bx, by, bw, bh};
        manette::focus(g_bouton);
    } else manette::focus(g_bandeau);
}

bool dans(float x, float y) {
    const Zone& z = g_bouton.w > 0 ? g_bouton : g_bandeau;   // avec un bouton, seul le bouton agit
    return z.w > 0 && appui::dansCible(z.x, z.y, z.w, z.h, g_dp, x, y);
}

void appuyer(const GameView& g) {
    std::string id;
    Sauvegarde& s = sauvegarde();
    switch (casActuel(id)) {
        case Cas::Attente:
            if (pret::activer(s, pret::maintenant())) { WFS_LOG("menu : bandeau du prêt : « Activer » (%s)", id.c_str()); s.enregistrer(); }
            break;
        case Cas::Actif:
            WFS_LOG("menu : bandeau du prêt → garage (%s)", id.c_str());
            garageRegarder(); garageOuvrirSur(id.c_str()); g.allerA(Ecran::Garage);
            break;
        case Cas::Fin:   // RG-05 : bouton « Garage » : fiche de la voiture, où elle s'achète
            WFS_LOG("menu : « Garde-la pour toujours » → garage (%s)", id.c_str());
            oublier(); garageRegarder(); garageOuvrirSur(id.c_str()); g.allerA(Ecran::Garage);
            break;
        case Cas::Expire: WFS_LOG("menu : « Prêt expiré » vu"); oublier(); break;
        default: break;
    }
}

bool centre(float& x, float& y) {
    const Zone& z = g_bouton.w > 0 ? g_bouton : g_bandeau;
    if (z.w <= 0) return false;
    x = z.x + z.w / 2; y = z.y + z.h / 2; return true;
}

} // namespace ecrans::pret_bandeau

WFS_SYSTEM(ecrans::pret_bandeau::PretBandeau, 451)   // interface : juste après le menu (450), qui le dessine

} // namespace wfs
