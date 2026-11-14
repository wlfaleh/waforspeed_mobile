// WFS-502 (RG-08) : voiture prêtée par le cadeau du jour.
//   (a) réception → 24 h + 1 s sans activer → prêt perdu, message « Prêt expiré » ;
//   (b) activation → 3 h − 1 min : utilisable ; 3 h + 1 s : plus possédée, message « Garde-la pour toujours » ;
//   (c) heure du téléphone reculée de 1 h pendant le prêt → temps restant non augmenté (et la fin ne recule pas) ;
//   (d) course commencée à 3 h − 10 s : la voiture reste utilisable jusqu'à la fin de la course, gains comptés ;
//   (e) relance du jeu : état gardé (sauvegarde écrite puis relue), aucune minute rendue ;
//   (f) un seul prêt à la fois, choix rendu à la voiture d'avant, ancienne sauvegarde lue sans erreur.
#include <cstdio>
#include <string>
#include "../src/core/configgame.h"
#include "../src/game/boutique.h"
#include "../src/game/cadeau.h"
#include "../src/game/pret.h"
using namespace wfs;

static int g_echecs = 0;
static void cas(const std::string& nom, bool ok) {
    std::printf("%s %s\n", ok ? "OK   " : "ÉCHEC", nom.c_str());
    if (!ok) g_echecs++;
}

int main() {
    constexpr long long T0 = 1794600000LL;   // 2026-11-13 20:40 UTC
    constexpr long long H = 3600;
    const long long PRET = (long long)(config().cadeau.pretHeures * H), ACTIV = (long long)(config().cadeau.activationHeures * H);
    cas("durées du jeu : prêt 3 h, activation 24 h (configgame.json)", PRET == 3 * H && ACTIV == 24 * H);

    // ---- (a) 24 h + 1 s sans activer → prêt perdu
    {
        Sauvegarde s; s.defauts();
        pret::recevoir(s.cadeau, "m4", T0);
        cas("(a) réception : prêt en attente, voiture pas encore utilisable", pret::etat(s.cadeau) == pret::Etat::EnAttente && !pret::utilisable(s, "m4"));
        cas("(a) reste 24 h pour activer", pret::resteActivation(s.cadeau, T0) == ACTIV);
        cas("(a) 24 h − 1 s : toujours en attente", pret::mettreAJour(s, T0 + ACTIV - 1) == pret::Evenement::Rien && pret::etat(s.cadeau) == pret::Etat::EnAttente);
        cas("(a) 24 h + 1 s : prêt perdu", pret::mettreAJour(s, T0 + ACTIV + 1) == pret::Evenement::Expire && pret::etat(s.cadeau) == pret::Etat::Aucun);
        std::string v;
        cas("(a) message « Prêt expiré » pour la m4, une fois", pret::message(s.cadeau, &v) == pret::Message::Expire && v == "m4");
        pret::oublierMessage(s.cadeau);
        cas("(a) message vu : plus de message", pret::message(s.cadeau) == pret::Message::Aucun);
        cas("(a) « Activer » après le délai : refusé", !pret::activer(s, T0 + ACTIV + 2));
        Sauvegarde s2; s2.defauts(); pret::recevoir(s2.cadeau, "m4", T0);
        cas("(a) en course, le délai dépassé ne fait rien perdre avant la fin de la course", pret::mettreAJour(s2, T0 + ACTIV + 5, true) == pret::Evenement::Rien &&
            pret::etat(s2.cadeau) == pret::Etat::EnAttente);
    }

    // ---- (b) activation → 3 h − 1 min : utilisable ; 3 h + 1 s : plus possédée
    {
        Sauvegarde s; s.defauts();
        s.car = "p208";   // partie neuve : voiture gratuite possédée (WFS-498 RG-09)
        const std::string avant = s.car;
        pret::recevoir(s.cadeau, "m4", T0);
        const long long ta = T0 + 5 * H;   // activée 5 h après la réception
        cas("(b) « Activer » sous 24 h", pret::activer(s, ta) && pret::etat(s.cadeau) == pret::Etat::Actif);
        cas("(b) 2e « Activer » : rien (déjà actif)", !pret::activer(s, ta + 10) && s.cadeau.pretFin == ta + PRET);
        cas("(b) m4 utilisable, jamais achetée", pret::utilisable(s, "m4") && !boutique::possedeVoiture(s, "m4") && !pret::utilisable(s, "golf"));
        s.car = "m4";   // choisie au garage
        cas("(b) 3 h − 1 min : encore utilisable, il reste 60 s", pret::mettreAJour(s, ta + PRET - 60) != pret::Evenement::Fini && pret::utilisable(s, "m4") &&
            pret::restant(s.cadeau, ta + PRET - 60) == 60);
        cas("(b) 3 h + 1 s : fin du prêt", pret::mettreAJour(s, ta + PRET + 1) == pret::Evenement::Fini && !pret::utilisable(s, "m4") && pret::etat(s.cadeau) == pret::Etat::Aucun);
        std::string v;
        cas("(b) message « Garde-la pour toujours » pour la m4", pret::message(s.cadeau, &v) == pret::Message::Fin && v == "m4");
        cas("(b) le choix revient à la voiture d'avant (" + avant + ")", s.car == avant && boutique::possedeVoiture(s, s.car));
        cas("(b) prix affiché = prix de boutique.voitures (jamais en dur)", boutique::prixVoiture("m4") > 0);
    }

    // ---- (c) heure reculée de 1 h pendant le prêt
    {
        Sauvegarde s; s.defauts();
        pret::recevoir(s.cadeau, "corsa", T0);
        pret::activer(s, T0);
        pret::mettreAJour(s, T0 + H);   // 1 h jouée : il reste 2 h
        const long long avant = pret::restant(s.cadeau, T0 + H);
        cas("(c) 1 h après l'activation : il reste 2 h", avant == 2 * H);
        pret::mettreAJour(s, T0);   // le téléphone recule de 1 h
        cas("(c) heure reculée de 1 h : il reste toujours 2 h, pas 3 h", pret::restant(s.cadeau, T0) == 2 * H);
        pret::mettreAJour(s, T0 + 30 * 60);   // 30 min plus tard (heure reculée)
        cas("(c) puis le temps reprend : 1 h 30 min", pret::restant(s.cadeau, T0 + 30 * 60) == 90 * 60);
        cas("(c) fin du prêt à l'heure reculée + 2 h, jamais plus tard", pret::mettreAJour(s, T0 + 2 * H + 1) == pret::Evenement::Fini);
        // recul AVANT tout appel de mise à jour (relance du jeu après un recul) : restant() seul ne dépasse pas le dernier vu
        Sauvegarde u; u.defauts(); pret::recevoir(u.cadeau, "corsa", T0); pret::activer(u, T0); pret::mettreAJour(u, T0 + 2 * H);
        cas("(c) restant lu après un recul de 2 h, sans mise à jour : 1 h", pret::restant(u.cadeau, T0) == H);
    }

    // ---- (d) course commencée à 3 h − 10 s
    {
        Sauvegarde s; s.defauts();
        pret::recevoir(s.cadeau, "m4", T0); pret::activer(s, T0); s.car = "m4";
        const long long depart = T0 + PRET - 10, arrivee = depart + 4 * 60;   // course de 4 min
        pret::mettreAJour(s, depart);
        cas("(d) départ à 3 h − 10 s : voiture utilisable", pret::utilisable(s, "m4"));
        cas("(d) pendant la course (après 3 h) : rien n'est rendu", pret::mettreAJour(s, depart + 60, true) != pret::Evenement::Fini && pret::utilisable(s, "m4"));
        const long long solde = boutique::solde(s);
        const int gain = boutique::crediterCourse(s, 37);   // fin de course : étoiles du score, comme avec toute autre voiture
        cas("(d) arrivée après la fin du prêt : gains comptés (+37 ★)", gain == 37 && boutique::solde(s) == solde + 37 && pret::utilisable(s, "m4"));
        cas("(d) retour au menu : fin du prêt", pret::mettreAJour(s, arrivee + 5) == pret::Evenement::Fini && !pret::utilisable(s, "m4") && s.car != "m4");
    }

    // ---- (e) relance du jeu : état gardé
    {
        Sauvegarde s; s.defauts();
        pret::recevoir(s.cadeau, "giulia", T0); pret::activer(s, T0 + 60); pret::mettreAJour(s, T0 + 60 + H);
        const std::string texte = s.texte();
        cas("(e) clés pret-* écrites", texte.find("pret-voiture=giulia") != std::string::npos && texte.find("pret-fin=" + std::to_string(T0 + 60 + PRET)) != std::string::npos &&
            texte.find("pret-restant-vu=" + std::to_string(2 * H)) != std::string::npos && texte.find("pret-recu=" + std::to_string(T0)) != std::string::npos);
        Sauvegarde r; cas("(e) sauvegarde relue", r.lire(texte));
        cas("(e) relance : même prêt, même fin, même temps restant", r.cadeau.pretVoiture == "giulia" && r.cadeau.pretFin == s.cadeau.pretFin &&
            r.cadeau.pretRestantVu == 2 * H && pret::restant(r.cadeau, T0 + 60 + H) == 2 * H && pret::utilisable(r, "giulia"));
        cas("(e) relance avec l'heure reculée : aucune minute rendue", pret::restant(r.cadeau, T0) == 2 * H);
        // message de fin gardé jusqu'à ce qu'il soit vu (relance entre la fin et le menu)
        pret::mettreAJour(s, T0 + 60 + PRET + 1);
        Sauvegarde m; m.lire(s.texte());
        cas("(e) message « Garde-la » gardé à la relance, prêt effacé", pret::message(m.cadeau) == pret::Message::Fin && m.cadeau.pretVoiture.empty() &&
            m.texte().find("pret-voiture") == std::string::npos);
        Sauvegarde ancienne;
        cas("(e) sauvegarde d'avant WFS-502 (pret-voiture, pret-recu, pret-fin seulement) lue sans erreur",
            ancienne.lire("version=1\npret-voiture=m4\npret-recu=1794600000\npret-fin=1794610800\n") && pret::etat(ancienne.cadeau) == pret::Etat::Actif &&
            pret::restant(ancienne.cadeau, 1794600000 + H) == 2 * H);
        Sauvegarde mal;
        mal.lire("version=1\npret-voiture=m4\npret-recu=1794600000\npret-restant-vu=abc\npret-avant=dauphine\npret-message=FIN!\n");
        cas("(e) valeurs invalides : défaut de la clé seule", mal.cadeau.pretVoiture == "m4" && mal.cadeau.pretRestantVu == 0 && mal.cadeau.pretAvant.empty() && mal.cadeau.pretMessage.empty());
    }

    // ---- (f) un seul prêt à la fois, cadeau qui donne une voiture
    {
        Sauvegarde s; s.defauts();
        cadeau::Entree e = cadeau::entreeDepuis(s);
        cas("(f) sans prêt : des voitures à prêter", !e.voituresPretables.empty());
        // ouvrir le cadeau avec une entrée qui ne permet qu'une voiture (garantie au 30e) : le prêt est enregistré avec le cadeau
        cadeau::Entree seule; seule.voituresPretables = {"m4"};
        s.cadeau.sansRare = config().cadeau.garantie - 1;
        const auto r = cadeau::ouvrir(s, {T0, 3600}, seule, 7);
        cas("(f) cadeau « voiture » → prêt en attente enregistré", r && r->gain == cadeau::Gain::Voiture && s.cadeau.pretVoiture == "m4" && s.cadeau.pretRecu == T0 &&
            pret::etat(s.cadeau) == pret::Etat::EnAttente);
        e = cadeau::entreeDepuis(s);
        cas("(f) prêt en attente : aucune voiture à prêter (repli sur les étoiles)", e.voituresPretables.empty());
        pret::activer(s, T0 + 10);
        cas("(f) prêt actif : toujours aucune voiture à prêter", cadeau::entreeDepuis(s).voituresPretables.empty());
        s.car = "abarth";   // pendant le prêt, le joueur choisit une autre voiture possédée, puis la m4
        pret::mettreAJour(s, T0 + 20);
        s.car = "m4";
        pret::mettreAJour(s, T0 + 10 + PRET + 1);
        cas("(f) fin : la voiture d'avant est la dernière possédée choisie pendant le prêt (abarth, gratuite)", boutique::voitureGratuite("abarth") && s.car == "abarth");
        pret::oublierMessage(s.cadeau);
        cas("(f) après la fin : de nouveau des voitures à prêter", !cadeau::entreeDepuis(s).voituresPretables.empty());
    }

    std::printf("%s : %d échec(s)\n", g_echecs ? "ÉCHEC" : "OK", g_echecs);
    return g_echecs ? 1 : 0;
}
