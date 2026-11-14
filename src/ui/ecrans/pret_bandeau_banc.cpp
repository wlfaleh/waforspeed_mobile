// WaForSpeed — banc de la voiture prêtée (WFS-502, RG-09, RG-10) : WFS_BANC=cadeau avec WFS_CADEAU=pret-attente|pret-actif|pret-fin
// (sans WFS_CADEAU : après le banc de WFS-501). En français puis en anglais, avec une vraie sauvegarde du banc (fichier effacé au
// début et à la fin, la vraie sauvegarde n'est jamais touchée) et l'heure du téléphone forcée (13/11/2026 20:47, heure de Paris).
// Parcours complet RG-10, au doigt :
//   menu → cadeau (voiture prêtée, « Activer » montré) → « Retour » (le prêt attend : bandeau du menu, carte du garage) →
//   garage : « Activer le prêt » → compte à rebours sur la carte (2 h 41 min) → « Choisir » → menu (bandeau du compte à rebours) →
//   course avec la voiture prêtée, de jour et de nuit → retour au menu → fin du prêt (3 h + 5 s) → « Garde-la pour toujours » →
//   « Garage » (voiture de nouveau bloquée, prix) ; puis un 2e prêt jamais activé : « Prêt expiré » au menu ; relance (sauvegarde relue).
// Chaque contrôle au journal (« prêt (WFS-502) : parcours fr : … : OK / ÉCHEC ») ; rend le nombre d'échecs.
#include <cstdio>
#include <cstdlib>
#include <string>
#include "cadeau_jour.h"
#include "pret_bandeau.h"
#include "textes_banc.h"
#include "../garage.h"
#include "../langue.h"
#include "../../core/log.h"
#include "../../game/boutique.h"
#include "../../game/course_system.h"
#include "../../game/game.h"
#include "../../game/pret.h"
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

int benchPret(Game& g, Renderer& r, const std::string& out, const std::string& filtre) {
    int erreurs = 0;
    const ui::langue::Choix choixAvant = ui::langue::choix();
    const bool regles = courseBenchRulesOn();
    const std::string chemin = out + "/pret-sauvegarde-banc.txt";
    const cadeau::Instant heure{1794599220LL, 3600};   // 2026-11-13 19:47 UTC = 20:47 à Paris
    const long long H = 3600;
    double t = 7000;
    bool ouvert = false;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; g.render(t); };
    auto images = [&](int n) { for (int i = 0; i < n; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 10 == 9 || i == n - 1) g.render(t); } };
    auto etape = [&](bool ok, const std::string& quoi) {
        if (!ok) erreurs++;
        WFS_LOG("prêt (WFS-502) : parcours %s : %s : %s", ui::langue::code(), quoi.c_str(), ok ? "OK" : "ÉCHEC");
    };
    auto garder = [&](const char* type) { return filtre.empty() || filtre == type; };
    auto capture = [&](const std::string& nom, const char* type, const char* quoi) {
        if (!garder(type)) return;
        textes::ecranNote() = "pret-" + nom; g.render(t); g.render(t + 0.001); textes::ecranNote().clear();
        const std::string f = "pret-" + nom + "-" + ui::langue::code();
        r.capturePng(out + "/" + f + ".png");
        WFS_LOG("capture %s : %s", f.c_str(), quoi);
    };
    int doigt = 300;
    auto toucher = [&](float x, float y) { const int id = doigt++; g.onTouch({id, x, y, true}); image(); g.onTouch({id, x, y, false}); images(4); };
    auto garage = [&](const char* nom) { float x, y; if (!garageBouton(nom, x, y)) return false; toucher(x, y); return true; };
    auto fermer = [&]() { if (ouvert) g.shutdown(); ouvert = false; };
    auto partie = [&](const char* voiture, bool nuit) {
        fermer();
        if (nuit) setenv("WFS_MOMENT", "nuit", 1); else unsetenv("WFS_MOMENT");
        ouvert = g.init(r, voiture, "ville");
        if (ouvert) { g.setAutopilot(false); g.camMode = 0; for (int i = 0; i < 30; i++) g.update(1.0 / 60); }
        return ouvert;
    };
    auto menu = [&]() { g.allerA(Ecran::Menu); images(60); };
    Sauvegarde& s = sauvegarde();
    for (const ui::langue::Choix l : {ui::langue::Choix::Francais, ui::langue::Choix::Anglais}) {
        ui::langue::choisir(l);
        ui::langue::viderJournal();
        std::remove(chemin.c_str());
        s.charger(chemin);   // partie neuve : 208 gratuite, aucun prêt
        s.profil.cree = true;   // pilote déjà créé (WFS-481) : le menu n'est pas caché par « Crée ton pilote »
        courseBenchRules(true);
        cadeau_jour::detail::forcerHeure(&heure);
        pret::forcerHeure(heure.epoch);
        if (!partie(s.car.c_str(), false)) { WFS_ERR("prêt (WFS-502) : chargement impossible"); erreurs++; break; }
        menu();
        etape(!pret_bandeau::visible(), "partie neuve : aucun bandeau de prêt");
        // ---------- menu → cadeau (voiture prêtée) ----------
        const cadeau::Resultat giulia{cadeau::Gain::Voiture, 0, "giulia", ""};
        cadeau_jour::detail::forcerResultat(&giulia);
        float x, y;
        if (cadeau_jour::detail::centre("bouton", x, y)) toucher(x, y);
        // le tirage du banc suit la vraie logique (souvent des étoiles) : le prêt est posé comme l'aurait fait un tirage « voiture »
        if (pret::etat(s.cadeau) == pret::Etat::Aucun) pret::recevoir(s.cadeau, "giulia", heure.epoch);
        cadeau_jour::detail::forcerFinAnimation(); image();
        etape(cadeau_jour::ouvert() && cadeau_jour::detail::centre("activer", x, y), "cadeau : gain « voiture prêtée » avec « Activer »");
        capture("gain", "pret-attente", "gain « voiture prêtée » : « Activer » (24 h)");
        if (cadeau_jour::detail::centre("retour", x, y)) toucher(x, y);   // pas encore activée : montrer l'attente
        etape(!cadeau_jour::ouvert() && pret::etat(s.cadeau) == pret::Etat::EnAttente, "« Retour » : prêt en attente");
        pret::forcerHeure(heure.epoch + 47 * 60); images(20);
        capture("menu-attente", "pret-attente", "menu : bandeau « Voiture prêtée : à activer dans 23 h 13 min » et « Activer »");
        etape(pret_bandeau::visible(), "menu : bandeau du prêt en attente");
        // ---------- garage : « Activer le prêt » ----------
        garageRegarder(); garageOuvrirSur(s.cadeau.pretVoiture.c_str());
        g.allerA(Ecran::Garage); images(60);
        etape(garageVoitureAffichee() && std::string(garageVoitureAffichee()) == s.cadeau.pretVoiture, "garage ouvert sur la voiture prêtée");
        capture("garage-attente", "pret-attente", "garage : voiture prêtée pas encore activée : « Activer le prêt »");
        const std::string id = s.cadeau.pretVoiture;
        etape(garage("essai") && pret::utilisable(s, id), "garage : « Activer le prêt » → prêt de 3 h commencé");
        const long long debut = s.cadeau.pretFin - 3 * H;
        pret::forcerHeure(debut + 19 * 60); images(30);
        capture("garage-actif", "pret-actif", "garage : carte « Prêtée : 2 h 41 min », « Choisir » comme une voiture possédée");
        etape(garage("choisir") && s.car == id, "« Choisir » la voiture prêtée (car=" + id + ")");
        images(40);
        etape(g.ecran() == Ecran::Menu, "retour au menu après « Choisir »");
        capture("menu-actif", "pret-actif", "menu : bandeau « Encore 2 h 41 min au volant »");
        // ---------- course avec la voiture prêtée, de jour et de nuit ----------
        for (const bool nuit : {false, true}) {
            if (!partie(id.c_str(), nuit)) { etape(false, "partie avec la voiture prêtée"); continue; }
            g.allerA(Ecran::Course); images(2); g.setAutopilot(true);
            for (int i = 0; i < 60 * 7; i++) g.update(1.0 / 60);
            images(5);
            etape(g.ecran() == Ecran::Course && pret::utilisable(s, id), std::string("course avec la voiture prêtée, ") + (nuit ? "nuit" : "jour"));
            capture(nuit ? "course-nuit" : "course-jour", "pret-actif", nuit ? "course de nuit avec la voiture prêtée" : "course de jour avec la voiture prêtée");
            g.setAutopilot(false);
        }
        // fin du prêt pendant une course : rien n'est rendu avant la fin de la course (RG-05)
        pret::forcerHeure(debut + 3 * H + 5); images(90);
        etape(pret::utilisable(s, id) && g.ecran() == Ecran::Course, "fin du prêt pendant la course : voiture gardée jusqu'au retour");
        // ---------- retour au menu : fin du prêt, « Garde-la pour toujours » ----------
        menu();
        etape(!pret::utilisable(s, id) && s.car != id && boutique::possedeVoiture(s, s.car), "retour au menu : prêt fini, voiture d'avant reprise (" + s.car + ")");
        etape(pret::message(s.cadeau) == pret::Message::Fin, "message « Garde-la pour toujours » à montrer");
        capture("menu-fin", "pret-fin", "menu : « Fin du prêt » · « Garde-la pour toujours : <prix> ★ » · « Garage »");
        if (pret_bandeau::centre(x, y)) toucher(x, y);
        images(60);
        etape(g.ecran() == Ecran::Garage && garageVoitureAffichee() && id == garageVoitureAffichee() && pret::message(s.cadeau) == pret::Message::Aucun,
              "« Garage » : fiche de la voiture, message vu");
        capture("garage-fin", "pret-fin", "garage : la voiture est de nouveau bloquée, avec son prix");
        etape(s.essais.count(id) == 0, "les courses du prêt n'ont pas utilisé l'essai gratuit de la voiture (WFS-498 RG-08)");
        garage("retour"); images(30);
        // ---------- 2e prêt jamais activé : « Prêt expiré » ----------
        pret::recevoir(s.cadeau, "corsa", debut + 4 * H);
        pret::forcerHeure(debut + 4 * H + 24 * H + 1);
        menu();
        etape(pret::message(s.cadeau) == pret::Message::Expire && pret::etat(s.cadeau) == pret::Etat::Aucun, "24 h + 1 s sans activer : prêt perdu");
        capture("menu-expire", "pret-fin", "menu : « Prêt expiré » (une fois)");
        g.allerA(Ecran::Garage); images(10); menu();
        etape(!pret_bandeau::visible(), "message vu en quittant le menu : bandeau parti");
        // ---------- relance : état gardé ----------
        s.attendre();
        const std::string texte = s.texte();
        Sauvegarde relue; relue.lire(texte);
        etape(relue.cadeau.pretVoiture.empty() && relue.cadeau.pretMessage.empty() && relue.car == s.car, "relance : sauvegarde relue, aucun prêt ni message");
        fermer();
        unsetenv("WFS_MOMENT");
        erreurs += ui::langue::verifierJournal();   // garde-fou de WFS-504 : aucun texte français en anglais, aucun texte qui dépasse
    }
    fermer();
    s.attendre();
    s.desactiver();
    std::remove(chemin.c_str());
    courseBenchRules(regles);
    pret::forcerHeure(0);
    cadeau_jour::detail::forcerHeure(nullptr);
    cadeau_jour::detail::forcerResultat(nullptr);
    sauvegarde().cadeau = EtatCadeau{};
    ui::langue::choisir(choixAvant);
    WFS_LOG("prêt (WFS-502) : banc terminé, %d erreur(s)", erreurs);
    return erreurs;
}

} // namespace wfs::ecrans::cadeau_jour
