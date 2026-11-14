// WaForSpeed — bandeau « Voiture prêtée » du menu (WFS-502) : voir pret_bandeau.h.
#include "pret_bandeau.h"
#include <algorithm>
#include <cstdio>
#include <string>
#include "reglages_appui.h"   // appui::dansCible : cible de 48 dp (comme le bouton « Cadeau du jour »)
#include "textes_banc.h"      // banc « textes » (journal des textes de cet écran)
#include "../garage.h"        // garageRegarder, garageOuvrirSur
#include "../garage_achat.h"  // texteEtoiles (★ dessinée), texteDuree, nombreLangue
#include "../langue.h"
#include "../petit_ecran.h"
#include "../text.h"
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
const vec4 GLASS{20 / 255.f, 22 / 255.f, 31 / 255.f, 0.86f}, BORD{1, 1, 1, 0.16f};

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
    // textes : titre (tag), phrase, bouton éventuel
    const std::string nom = nomVoiture(id);
    std::string titre, phrase, bouton;
    unsigned lisere = ACCENT2;
    switch (cas) {
        case Cas::Attente: titre = ui::tr("pret.bandeau.titre", nom); phrase = ui::tr("pret.bandeau.attente", garage::texteDuree(pret::resteActivation(s.cadeau, t)));
                           bouton = "pret.bandeau.activer"; break;
        case Cas::Actif: titre = ui::tr("pret.bandeau.titre", nom); phrase = ui::tr("pret.bandeau.actif", garage::texteDuree(pret::restant(s.cadeau, t)));
                         lisere = ACCENT; break;
        case Cas::Expire: {
            char h[16]; std::snprintf(h, sizeof h, "%g", config().cadeau.activationHeures);
            titre = ui::tr("pret.bandeau.expire", nom); phrase = ui::tr("pret.bandeau.expire.aide", h); lisere = DANGER; g_messageVu = true; break;
        }
        case Cas::Fin: titre = ui::tr("pret.bandeau.fin", nom); phrase = ui::tr("pret.garder", garage::nombreLangue(std::max(0, boutique::prixVoiture(id))));
                       bouton = "pret.bandeau.garage"; lisere = ACCENT; g_messageVu = true; break;
        default: break;
    }
    titre = ui::majuscules(titre);
    // mise en page : au-dessus de « Profil » (WFS-482), lui-même au-dessus de « Cadeau du jour » et de « Crédits » (boutons de 44 px,
    // écarts de 10 px) : même calcul que profil.cpp ; aucun bouton existant ne bouge, aucune zone d'appui ne se recouvre
    const float hBtn = 44 * kb, h = 58 * kb, pad = 16 * kb, tp1 = 12 * k, tp2 = 16 * k;
    const float yProfil = H - m.bas - 14 * k - 3 * hBtn - 20 * k;
    const float bp = 15 * k, bw = bouton.empty() ? 0 : ui::width(bp, ui::majuscules(ui::tr(bouton.c_str())), 0.1f) + 40 * kb, bh = 38 * kb;
    float wTexte = 0;
    { const textes::NomPropre np; wTexte = ui::width(tp1, titre, 0.14f); }
    wTexte = std::max(wTexte, garage::largeurEtoiles(tp2, phrase));
    const float w = pad + 4 * kb + wTexte + (bw > 0 ? 14 * kb + bw : 0) + pad;
    const float x = W - m.droite - 16 * k - w, y = yProfil - 10 * k - h;
    // .card : verre teinté, coin supérieur droit coupé, lisere à gauche (cyan : à faire ; orange : temps compté ; rouge : perdu)
    const float b = 12 * kb;
    const vec2 c[5] = {{x, y}, {x + w - b, y}, {x + w, y + b}, {x + w, y + h}, {x, y + h}};
    dessin2d::polygone(r, c, 5, dessin2d::uni(GLASS));
    dessin2d::contour(r, c, 5, std::max(1.f, kb), BORD);
    if (presse && bw <= 0) dessin2d::polygone(r, c, 5, dessin2d::uni(hex(ACCENT2, 0.18f)));
    r.uiRect(x, y, 3 * kb, h, hex(lisere));
    const float xt = x + pad + 4 * kb;
    { const textes::NomPropre np; ui::text(r, xt, y + 10 * kb, tp1, titre, hex(TAG), ui::GAUCHE, 0.14f, false); }   // nom de la voiture : inchangé en anglais
    garage::texteEtoiles(r, xt, y + 28 * kb, tp2, phrase, hex(cas == Cas::Expire ? MUTED : TEXTE), ui::GAUCHE);
    g_bandeau = {x, y, w, h};
    if (bw > 0) {   // .btn orange en biseau (action principale du bandeau)
        const float bx = x + w - pad - bw, by = y + (h - bh) / 2;
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
