// WaForSpeed — banc de l'écran du cadeau du jour (WFS-501, RG-07, RG-08) : WFS_BANC=cadeau.
// En français puis en anglais, au doigt : menu avec la pastille → « Cadeau du jour » → animation (3 images) → gain → « Chances » →
// « Retour » → « Retour » → menu sans cadeau (« Prochain cadeau dans hh:mm ») → 2e appui (écran d'attente, aucun 2e cadeau) →
// relance de la partie (toujours aucun cadeau). Puis les 4 types de gain (étoiles, coloris, skin, voiture prêtée), l'écran
// d'attente et « Prochain cadeau bientôt » (heure reculée), de jour et le menu de nuit.
// La vraie sauvegarde n'est jamais touchée : sauvegarde inactive du banc (rien n'est écrit), état du cadeau remis à zéro au début,
// heure du téléphone forcée (13/11/2026 20:47, heure de Paris). WFS_CADEAU=etoiles|coloris|skin|voiture|chances|attente ne garde
// que les captures de ce type (et le menu). Rend le nombre d'erreurs (parcours et garde-fou des textes anglais de WFS-504).
#include <cstdlib>
#include <string>
#include "cadeau_jour.h"
#include "textes_banc.h"
#include "../langue.h"
#include "../../core/log.h"
#include "../../game/game.h"
#include "../../game/pret.h"   // WFS-502
#include "../../game/sauvegarde.h"
#include "../../render/renderer.h"

namespace wfs::ecrans::cadeau_jour {
namespace detail {
void forcerHeure(const cadeau::Instant* t);
void forcerResultat(const cadeau::Resultat* r);
void forcerFinAnimation();
int vue();
bool centre(const char* nom, float& x, float& y);
} // namespace detail

int benchCadeau(Game& g, Renderer& r, const std::string& out) {
    int erreurs = 0;
    const char* filtreEnv = std::getenv("WFS_CADEAU");
    const std::string filtre = filtreEnv ? filtreEnv : "";
    if (filtre.rfind("pret-", 0) == 0) return benchPret(g, r, out, filtre);   // WFS-502 : captures de la voiture prêtée seulement
    const ui::langue::Choix choixAvant = ui::langue::choix();
    const cadeau::Instant heure{1794599220LL, 3600};   // 2026-11-13 19:47 UTC = 20:47 à Paris : prochain cadeau dans 03:13
    detail::forcerHeure(&heure);
    double t = 900;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; g.render(t); };
    auto attendre = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 9) g.render(t); } };
    auto etape = [&](bool ok, const std::string& quoi) {
        if (!ok) erreurs++;
        WFS_LOG("cadeau (WFS-501) : parcours %s : %s : %s", ui::langue::code(), quoi.c_str(), ok ? "OK" : "ÉCHEC");
    };
    auto garder = [&](const char* type) { return filtre.empty() || filtre == type; };
    auto capture = [&](const std::string& nom, const char* type, const char* quoi) {
        if (!garder(type)) { g.render(t); return; }
        textes::ecranNote() = "cadeau-" + nom; g.render(t); g.render(t + 0.001); textes::ecranNote().clear();
        const std::string f = "cadeau-" + nom + "-" + ui::langue::code();
        r.capturePng(out + "/" + f + ".png");
        WFS_LOG("capture %s : %s", f.c_str(), quoi);
    };
    auto toucher = [&](int id, float x, float y) { g.onTouch({id, x, y, true}); image(); g.onTouch({id, x, y, false}); image(); };
    auto appuyer = [&](int id, const char* nom) {
        float x, y;
        const bool ok = detail::centre(nom, x, y);
        etape(ok, std::string("bouton « ") + nom + " » dessiné");
        if (ok) toucher(id, x, y);
    };
    auto partie = [&](bool nuit) {
        if (nuit) setenv("WFS_MOMENT", "nuit", 1); else unsetenv("WFS_MOMENT");
        const bool ok = g.init(r, "golf", "ville");
        if (ok) { g.setAutopilot(false); g.camMode = 0; for (int i = 0; i < 30; i++) g.update(1.0 / 60); g.allerA(Ecran::Menu); for (int i = 0; i < 60; i++) image(); }
        return ok;
    };
    for (const ui::langue::Choix l : {ui::langue::Choix::Francais, ui::langue::Choix::Anglais}) {
        ui::langue::choisir(l);
        ui::langue::viderJournal();
        sauvegarde().cadeau = EtatCadeau{};   // aucun cadeau encore ouvert : cadeau disponible
        detail::forcerResultat(nullptr);
        if (!partie(false)) { WFS_ERR("cadeau (WFS-501) : chargement impossible"); return erreurs + 1; }
        // ---------- RG-08 : parcours complet ----------
        capture("menu-pastille", "menu", "menu : bouton « Cadeau du jour » avec la pastille");
        const cadeau::Resultat etoiles{cadeau::Gain::Etoiles, 23};
        detail::forcerResultat(&etoiles);   // gain montré : « +23 ★ » (la sauvegarde suit le vrai tirage, inactive)
        appuyer(1, "bouton");
        etape(ouvert() && detail::vue() == 0, "« Cadeau du jour » → animation d'ouverture");
        etape(sauvegarde().cadeau.total == 1, "tirage fait avant l'animation (1 cadeau ouvert)");
        // animation : images à 0,3 s (tremble), 1,0 s (s'ouvre, lumière), 1,35 s (le gain apparaît)
        const double debut = t;
        g.render(t);   // 1re image : début de l'animation
        const double moments[3] = {0.30, 1.00, 1.35};
        for (int i = 0; i < 3; i++) {
            t = debut + moments[i];
            capture("ouverture-" + std::to_string(i + 1), "etoiles", "animation d'ouverture");
        }
        t = debut + 2.0; image();
        etape(detail::vue() == 1, "fin de l'animation → gain");
        capture("etoiles", "etoiles", "gain étoiles « +23 ★ »");
        appuyer(2, "chances");
        etape(detail::vue() == 3, "« Chances » → tableau des chances");
        capture("chances", "chances", "tableau « Chances » (7 lignes de configgame.json, minimum, garantie, compteur)");
        appuyer(3, "retour");
        etape(detail::vue() == 1, "« Retour » des chances → gain");
        appuyer(4, "retour");
        etape(!ouvert(), "« Retour » → menu");
        attendre(30); image();
        etape(!cadeau::disponible(sauvegarde().cadeau, heure), "menu : plus de cadeau (pastille partie)");
        capture("menu-attente", "menu", "menu sans cadeau : « Prochain cadeau dans 03:13 »");
        appuyer(5, "bouton");
        etape(ouvert() && detail::vue() == 2 && sauvegarde().cadeau.total == 1, "2e appui le même jour : écran d'attente, aucun 2e cadeau");
        capture("attente", "attente", "écran d'attente : compte à rebours et dernier gain réaffiché");
        appuyer(6, "retour");
        // relance de la partie (la sauvegarde reste en mémoire, comme après une relance du jeu) : toujours aucun cadeau
        g.shutdown();
        if (!partie(false)) { WFS_ERR("cadeau (WFS-501) : relance impossible"); return erreurs + 1; }
        appuyer(7, "bouton");
        etape(ouvert() && detail::vue() == 2 && sauvegarde().cadeau.total == 1, "après la relance : écran d'attente, aucun 2e cadeau");
        appuyer(8, "retour");
        // ---------- les 4 types de gain (RG-03), montrés sans changer l'état ----------
        const cadeau::Resultat gains[3] = {
            {cadeau::Gain::Coloris, 0, "golf", "rose"},
            {cadeau::Gain::Skin, 0, "gt86", "rallye"},
            {cadeau::Gain::Voiture, 0, "giulia", ""},
        };
        const char* noms[3] = {"coloris", "skin", "voiture"};
        for (int i = 0; i < 3; i++) {
            sauvegarde().cadeau = EtatCadeau{};   // un cadeau de nouveau disponible (banc seulement)
            detail::forcerResultat(&gains[i]);
            appuyer(10 + i, "bouton");
            if (i == 2) { pret::forcerHeure(heure.epoch); pret::recevoir(sauvegarde().cadeau, "giulia", heure.epoch); }   // WFS-502 : prêt en attente, comme après un vrai tirage « voiture »
            detail::forcerFinAnimation(); image();
            capture(noms[i], noms[i], "gain montré par le banc (WFS_CADEAU)");
            if (i == 2) {   // WFS-502 : « Activer » commence le prêt
                float x, y; etape(detail::centre("activer", x, y), "voiture prêtée : bouton « Activer »"); appuyer(20, "activer"); etape(!ouvert(), "« Activer » ferme l'écran");
                etape(pret::utilisable(sauvegarde(), "giulia"), "« Activer » : prêt de la giulia commencé (WFS-502)");
                EtatCadeau& c = sauvegarde().cadeau;   // prêt retiré : captures suivantes de WFS-501 inchangées (pas de bandeau au menu)
                c.pretVoiture.clear(); c.pretRecu = c.pretFin = c.pretRestantVu = 0; c.pretAvant.clear(); c.pretMessage.clear();
                pret::forcerHeure(0);
            }
            else appuyer(20 + i, "retour");
        }
        // heure reculée (WFS-500 RG-07) : « Prochain cadeau bientôt »
        const cadeau::Instant recul{heure.epoch - 3 * 86400, heure.decalage};
        detail::forcerHeure(&recul); image();
        capture("menu-bientot", "attente", "menu : heure du téléphone reculée → « Prochain cadeau bientôt »");
        detail::forcerHeure(&heure);
        // ---------- menu de nuit, avec la pastille ----------
        g.shutdown();
        sauvegarde().cadeau = EtatCadeau{};
        detail::forcerResultat(&etoiles);
        if (!partie(true)) { WFS_ERR("cadeau (WFS-501) : chargement de nuit impossible"); return erreurs + 1; }
        capture("menu-nuit", "menu", "menu de nuit : bouton « Cadeau du jour » avec la pastille");
        appuyer(30, "bouton"); detail::forcerFinAnimation(); image();
        capture("etoiles-nuit", "etoiles", "gain de nuit");
        appuyer(31, "retour");
        g.shutdown();
        unsetenv("WFS_MOMENT");
        // garde-fou de WFS-504, sur les textes de cette langue : aucune phrase française en anglais, aucun texte qui dépasse
        erreurs += ui::langue::verifierJournal();
    }
    ui::langue::choisir(choixAvant);
    sauvegarde().cadeau = EtatCadeau{};
    detail::forcerResultat(nullptr);
    detail::forcerHeure(nullptr);
    WFS_LOG("cadeau (WFS-501) : banc terminé, %d erreur(s)", erreurs);
    if (filtre.empty()) erreurs += benchPret(g, r, out, filtre);   // WFS-502 : puis la voiture prêtée
    return erreurs;
}

} // namespace wfs::ecrans::cadeau_jour
