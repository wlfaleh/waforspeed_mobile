// WaForSpeed — banc d'essai de WFS-503 (WFS_BANC=cadeau-gains) : les gains du cadeau du jour donnés pour de vrai.
// Vraie sauvegarde (fichier du banc, effacé au début et à la fin), vrai tirage (cadeau::ouvrir appelé par l'écran du cadeau, rien de forcé
// à l'affichage), vrais touchers. Pour obtenir chaque type de gain, le banc ne garde dans config().cadeau que la chance de ce type
// (valeurs du jeu remises à la fin) ; les finitions sont possédées d'avance pour que le coloris soit une couleur (rangée des couleurs).
// En français puis en anglais :
//   1. étoiles : menu (solde avant) → « Cadeau du jour » → gain → menu (solde après = avant + gain, crédité une fois) ;
//   2. coloris : gain → garage de la voiture gagnée, pastille sans cadenas, « Choisir » (gratuit, rien de débité) ;
//   3. skin : gain → garage, rangée « Skin », bouton sans cadenas ;
//   2 bis. (en français) course de jour et de nuit avec le coloris gagné (caméra de poursuite, Ville) ;
//   4. relance du jeu (sauvegarde relue) : solde, coloris et skin gardés, coloris toujours choisi.
// Captures cadeau-gains-<langue>-*.png ; chaque contrôle au journal (« cadeau-gains (WFS-503) : OK / ÉCART ») ; rend le nombre d'écarts.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../core/log.h"
#include "../render/renderer.h"
#include "../ui/ecrans/cadeau_jour.h"
#include "../ui/garage.h"
#include "../ui/garage_commun.h"
#include "../ui/garage_skin.h"
#include "../ui/langue.h"
#include "../ui/text.h"
#include "boutique.h"
#include "cadeau.h"
#include "game.h"
#include "personnalisation.h"

namespace wfs::ecrans::cadeau_jour::detail {   // accès du banc de l'écran du cadeau (cadeau_jour.cpp, WFS-501)
void forcerHeure(const cadeau::Instant* t);
void forcerResultat(const cadeau::Resultat* r);
void forcerFinAnimation();
int vue();
bool centre(const char* nom, float& x, float& y);
} // namespace wfs::ecrans::cadeau_jour::detail

namespace wfs::cadeau {
using namespace ecrans::garage;
namespace ecran = ecrans::cadeau_jour;

int benchGains(Game& g, Renderer& r, const std::string& out) {
    ui::chargerPolice(r);
    int ecarts = 0;
    auto verifier = [&](bool ok, const std::string& quoi) { WFS_LOG("cadeau-gains (WFS-503) : %s : %s : %s", ui::langue::code(), ok ? "OK" : "ÉCART", quoi.c_str()); if (!ok) ecarts++; };
    const std::string chemin = out + "/cadeau-gains-sauvegarde-banc.txt";
    const ConfigJeu::Cadeau chancesJeu = config().cadeau;   // remises à la fin
    const ui::langue::Choix choixAvant = ui::langue::choix();
    Sauvegarde& s = sauvegarde();
    double t = 6000;
    bool ouvert = false;
    int doigt = 70;
    auto image = [&]() { g.update(1.0 / 60); t += 1.0 / 60; g.render(t); };
    auto images = [&](int n) { for (int i = 0; i < n; i++) image(); };
    auto capture = [&](const std::string& nom, const char* quoi) {
        g.render(t); g.render(t + 0.01);
        const std::string f = std::string("cadeau-gains-") + ui::langue::code() + "-" + nom;
        r.capturePng(out + "/" + f + ".png");
        WFS_LOG("capture %s : %s (solde %lld ★)", f.c_str(), quoi, boutique::solde(s));
    };
    auto toucher = [&](float x, float y) { const int id = doigt++; g.onTouch({id, x, y, true}); image(); g.onTouch({id, x, y, false}); images(3); };
    auto toucherZone = [&](Zone z) { if (z.w > 0) toucher(z.x + z.w / 2, z.y + z.h / 2); return z.w > 0; };   // copie : zones réécrites
    auto fermer = [&]() { if (ouvert) g.shutdown(); ouvert = false; };
    auto partie = [&](const char* car, Ecran ecranVoulu) {
        fermer();
        if (!g.init(r, car, "ville")) { WFS_ERR("cadeau-gains : chargement impossible (%s)", car); return false; }
        ouvert = true; g.setAutopilot(false); g.camMode = 0;
        for (int i = 0; i < 30; i++) g.update(1.0 / 60);
        g.allerA(ecranVoulu); images(45);
        return true;
    };
    auto boutonCadeau = [&](const char* nom) { float x, y; if (!ecran::detail::centre(nom, x, y)) return false; toucher(x, y); return true; };
    auto boutonGarage = [&](const char* nom) { float x, y; if (!ecrans::garageBouton(nom, x, y)) return false; toucher(x, y); images(4); return true; };
    // ne garde que la chance du type voulu : le vrai tirage de l'écran (cadeau::ouvrir) donne ce type
    auto seulement = [&](Gain gain) {
        ConfigJeu::Cadeau& c = config().cadeau;
        c = chancesJeu;
        for (auto& tr : c.tranche) if (gain != Gain::Etoiles) tr.chance = 0;
        if (gain != Gain::Coloris) c.chanceColoris = 0;
        if (gain != Gain::Skin) c.chanceSkin = 0;
        c.chanceVoiture = 0;
    };
    // ouvre le cadeau du jour n (heure forcée : un jour de plus à chaque cadeau) depuis le menu ; rend le gain réellement donné
    auto ouvrirCadeau = [&](int jour, Gain type, const char* nom, Resultat& gain) {
        seulement(type);   // après Game::init (qui relit configgame.json)
        const Instant h{1794599220LL + jour * 86400LL, 3600};   // 13/11/2026 20:47 à Paris, + n jours
        ecran::detail::forcerHeure(&h);
        ecran::detail::forcerResultat(nullptr);   // rien de forcé : l'écran montre le vrai tirage
        images(4);
        const long long total = s.cadeau.total;
        if (!boutonCadeau("bouton")) { verifier(false, std::string(nom) + " : bouton « Cadeau du jour » du menu"); return false; }
        ecran::detail::forcerFinAnimation(); images(2);
        const bool lu = lireResultat(s.cadeau.dernier, gain);
        verifier(ecran::ouvert() && ecran::detail::vue() == 1 && s.cadeau.total == total + 1 && lu && gain.gain == type, std::string(nom) + " : cadeau ouvert, gain " + s.cadeau.dernier);
        config().cadeau = chancesJeu;
        return lu;
    };
    auto possede = [&](const std::string& car, const std::string& id) { const perso::Objet* o = perso::objet(id); return o && perso::possede(s, *o, car); };

    for (const ui::langue::Choix langue : {ui::langue::Choix::Francais, ui::langue::Choix::Anglais}) {
        const bool fr = langue == ui::langue::Choix::Francais;
        ui::langue::choisir(langue);
        ui::langue::viderJournal();
        std::remove(chemin.c_str());
        s.charger(chemin);   // partie neuve : 3 voitures gratuites, 0 ★
        { std::string nick; s.profil.creer("Banc 503", "WFS-503", nick); s.nick = nick; }   // pilote déjà créé (WFS-481) : le menu, pas « Crée ton pilote »
        for (const char* car : {"p208", "abarth", "gt86"})   // finitions possédées d'avance : le coloris tiré est une couleur
            for (const perso::Objet& o : perso::catalogue()) if (o.type == perso::Type::Finition && perso::disponible(o, car)) s.possede[car].insert(o.id);

        // ---------- 1. étoiles ----------
        if (!partie("p208", Ecran::Menu)) break;
        const long long avant = boutique::solde(s);
        capture("etoiles-menu-avant", "menu avant le cadeau : solde du portefeuille");
        Resultat gain;
        if (ouvrirCadeau(0, Gain::Etoiles, "étoiles", gain)) {
            capture("etoiles-gain", "gain d'étoiles (vrai tirage)");
            verifier(gain.gain == Gain::Etoiles && gain.etoiles >= 10 && boutique::solde(s) == avant + gain.etoiles, "étoiles créditées au portefeuille (solde = avant + gain)");
            boutonCadeau("retour"); images(30);
            capture("etoiles-menu-apres", "menu après le cadeau : solde crédité");
            verifier(!ecran::ouvert() && boutique::solde(s) == avant + gain.etoiles, "retour au menu : solde crédité une seule fois");
            boutonCadeau("bouton"); images(4);
            verifier(ecran::detail::vue() == 2 && boutique::solde(s) == avant + gain.etoiles, "2e appui le même jour : attente, aucune étoile de plus");
            boutonCadeau("retour"); images(10);
        }

        // ---------- 2. coloris ----------
        Resultat coloris;
        const long long soldeColoris = boutique::solde(s);
        if (ouvrirCadeau(1, Gain::Coloris, "coloris", coloris)) {
            capture("coloris-gain", "gain d'un coloris (vrai tirage)");
            verifier(coloris.gain == Gain::Coloris && possede(coloris.voiture, coloris.objet) && !s.paint.count(coloris.voiture) &&
                     estNouveau(s, coloris.voiture, coloris.objet), "coloris possédé, « nouveau », pas appliqué d'office");
            boutonCadeau("retour"); images(10);
            const int p = peintureParId(coloris.objet.c_str());
            if (p >= 0 && partie(coloris.voiture.c_str(), Ecran::Garage)) {
                choisirOnglet(ONGLET_PEINTURE); images(2);   // WFS-557 : garage en onglets depuis WFS-551 (pastilles sur l'onglet « Peinture »)
                for (int i = 0; i < 12 && g_z.chevrons[0].w > 0; i++) toucherZone(g_z.chevrons[0]);   // début de la rangée
                bool vu = false;
                for (int i = 0; i < 12 && !vu; i++) { vu = toucherZone(g_z.peintures[p]); if (!vu && !toucherZone(g_z.chevrons[1])) break; }
                images(6);
                verifier(vu, "garage : pastille du coloris gagné trouvée");
                capture("coloris-garage", "garage : coloris gagné débloqué (sans cadenas), en aperçu");
                boutonGarage("choisir");
                verifier(s.paint[coloris.voiture] == coloris.objet && boutique::solde(s) == soldeColoris, "« Choisir » : coloris posé, rien de débité");
                capture("coloris-choisi-terrain", "« Choisir » au garage : coloris posé, étape suivante du parcours (choix du terrain)");
            } else verifier(false, "coloris : couleur du garage introuvable (" + coloris.objet + ")");
        }

        // ---------- 2 bis. course de jour et de nuit avec le coloris gagné (avant la skin, qui le cacherait) ----------
        if (fr) {
            for (const char* moment : {"jour", "nuit"}) {
                setenv("WFS_MOMENT", moment, 1);
                if (!g.init(r, coloris.voiture.c_str(), "ville")) { WFS_ERR("cadeau-gains : course impossible"); ecarts++; continue; }
                ouvert = true; g.camMode = 0; g.allerA(Ecran::Course); g.setAutopilot(true);
                for (int i = 0; i < 300; i++) { g.update(1.0 / 60); t += 1.0 / 60; if (i % 6 == 0) g.render(t); }
                capture(std::string("course-") + moment, "course avec le coloris gagné (caméra de poursuite, Ville)");
                g.setAutopilot(false); fermer();
            }
            unsetenv("WFS_MOMENT");
        }
        // ---------- 3. skin ----------
        Resultat skin;
        if (partie("p208", Ecran::Menu) && ouvrirCadeau(2, Gain::Skin, "skin", skin)) {
            capture("skin-gain", "gain d'une skin (vrai tirage)");
            verifier(skin.gain == Gain::Skin && possede(skin.voiture, skin.objet) && !s.skin.count(skin.voiture), "skin possédée, pas posée d'office");
            boutonCadeau("retour"); images(10);
            int o = 0;
            for (int i = 1; i < SkinGarage::NB; i++) if (skin.objet == BOUTIQUE_SKINS_IDS[SkinGarage::catalogue(i)]) o = i;
            if (o > 0 && partie(skin.voiture.c_str(), Ecran::Garage)) {
                choisirOnglet(ONGLET_SKINS); images(2);   // WFS-557 : garage en onglets depuis WFS-551 (skins sur l'onglet « Skins »)
                for (int i = 0; i < 6 && g_z.skins[o].w <= 0 && g_z.chevronsSkin[1].w > 0; i++) toucherZone(g_z.chevronsSkin[1]);
                verifier(toucherZone(g_z.skins[o]), "garage : bouton de la skin gagnée trouvé");
                images(6);
                capture("skin-garage", "garage : skin gagnée débloquée (sans cadenas), en aperçu");
            } else verifier(false, "skin : bouton du garage introuvable (" + skin.objet + ")");
        }

        // ---------- 4. relance : sauvegarde relue ----------
        fermer();
        const long long solde = boutique::solde(s);
        s.attendre(); s.charger(chemin);
        verifier(boutique::solde(s) == solde && possede(coloris.voiture, coloris.objet) && s.paint[coloris.voiture] == coloris.objet &&
                 possede(skin.voiture, skin.objet) && s.cadeau.total == 3, "relance : solde, coloris (choisi) et skin gardés, 3 cadeaux");
        if (partie(coloris.voiture.c_str(), Ecran::Garage)) capture("relance-garage", "après relance : la voiture garde le coloris gagné");
        fermer();

        ecarts += ui::langue::verifierJournal();
    }
    fermer();
    config().cadeau = chancesJeu;
    ecran::detail::forcerHeure(nullptr);
    ecran::detail::forcerResultat(nullptr);
    ui::langue::choisir(choixAvant);
    s.attendre(); s.desactiver(); std::remove(chemin.c_str());
    WFS_LOG("cadeau-gains (WFS-503) : %d écart(s)", ecarts);
    return ecarts;
}

} // namespace wfs::cadeau
