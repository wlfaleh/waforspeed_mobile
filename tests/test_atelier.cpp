// WFS-496 : logique de l'atelier d'autocollants (src/ui/ecrans/atelier_calques.h), sans écran.
//   RG-02 : 10 motifs gratuits (2 bandes, 3 numéros, 1 flamme, 1 éclair, 1 étoile, 2 lettres), les autres dans un pack payant ;
//   RG-03 : ± 5°, ± 5 % (bornes 10 à 300 %), miroir, couleur, supprimer, annuler / refaire (20 pas), geste continu = 1 pas ;
//   RG-04 : 20 calques au plus, monter, descendre, masquer ;
//   RG-05 / RC-E18-04 : texte gardé (calques masqués compris) relu à l'identique ; clé autocollants-<voiture> de la sauvegarde,
//           relue après relance, rien d'autre de cassé ; texte de la course : visibles et permis seulement.
// police des lettres et des chiffres de la bibliothèque (vehicule_autocollants.cpp), comme tests/test_vehicule_autocollants.cpp
#define STB_TRUETYPE_IMPLEMENTATION
#include "../third_party/stb/stb_truetype.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <set>
#include <string>
#include "../src/game/sauvegarde.h"
#include "../src/ui/ecrans/atelier_calques.h"
using namespace wfs;
using namespace wfs::ecrans::atelier;

static int g_echecs = 0;
static void cas(const char* nom, bool ok) {
    std::printf("%s %s\n", ok ? "OK   " : "ÉCHEC", nom);
    if (!ok) g_echecs++;
}

int main() {
    // ---- RG-02 : motifs gratuits et packs
    {
        int n = 0, familles[6] = {};
        for (const va::Motif& m : va::bibliotheque()) if (motifGratuit(m.id)) { n++; familles[(int)m.famille]++; }
        cas("10 motifs gratuits : 2 bandes, 3 numéros, 1 flamme, 1 éclair, 1 étoile, 2 lettres",
            n == 10 && motifsGratuits() == 10 && familles[0] == 2 && familles[1] == 3 && familles[2] == 1 && familles[3] == 1 && familles[4] == 1 && familles[5] == 2);
        bool packs = true;
        const std::set<std::string> connus = {"essentiels", "numeros", "bandes", "flammes", "etoiles"};
        for (const va::Motif& m : va::bibliotheque()) packs = packs && packDuMotif(m.id) && connus.count(packDuMotif(m.id));
        cas("chaque motif de la bibliothèque a son pack (essentiels, numeros, bandes, flammes, etoiles)", packs);
        cas("packs : chiffre-7 et lettre-z → numeros, bande-fleche → bandes, eclair-3 → flammes, etoile-2 → etoiles",
            !std::strcmp(packDuMotif("chiffre-7"), "numeros") && !std::strcmp(packDuMotif("lettre-z"), "numeros") && !std::strcmp(packDuMotif("bande-fleche"), "bandes") &&
            !std::strcmp(packDuMotif("eclair-3"), "flammes") && !std::strcmp(packDuMotif("etoile-2"), "etoiles"));
        cas("motif inconnu : aucun pack", packDuMotif("logo-marque") == nullptr && !motifPermis("logo-marque"));
        const auto chiffres = motifsDe(va::Famille::Chiffre);
        cas("bibliothèque d'une famille : gratuits d'abord (numéros : 3 gratuits puis 7)", chiffres.size() == 10 && motifGratuit(chiffres[2]->id) && !motifGratuit(chiffres[3]->id));
        cas("sans sauvegarde : gratuits permis, packs payants non", motifPermis("flamme-1") && !motifPermis("flamme-2"));
    }
    // ---- RG-03, RG-04 : actions et historique
    {
        Etat e;
        e.charger("");
        cas("atelier vide : aucun calque choisi, rien d'annulable", e.calques().empty() && e.choisi() == -1 && !e.annuler() && !e.modifie());
        cas("ajouter : nouveau calque choisi, au-dessus", e.ajouter("bande-large", va::Direction::Gauche) && e.choisi() == 0 && e.modifie());
        e.ajouter("chiffre-1", va::Direction::Gauche);
        cas("2e calque choisi", e.choisi() == 1 && e.calqueChoisi()->c.motif == "chiffre-1");
        e.tourner(PAS_ROTATION); e.tourner(PAS_ROTATION);
        cas("+5° deux fois : 10°", e.calqueChoisi()->c.rotation == 10);
        e.tourner(-180);
        cas("rotation ramenée dans [−180, 180[ : 10 − 180 = −170", e.calqueChoisi()->c.rotation == -170);
        e.agrandir(PAS_ECHELLE);
        cas("+5 % : 105 %", e.calqueChoisi()->c.echelle == 105);
        for (int i = 0; i < 80; i++) e.agrandir(PAS_ECHELLE);
        cas("échelle bornée à 300 %", e.calqueChoisi()->c.echelle == 300 && !e.agrandir(PAS_ECHELLE));
        e.miroir(); e.colorer("orange");
        cas("miroir et couleur orange", e.calqueChoisi()->c.miroir && e.calqueChoisi()->c.couleur == "orange");
        cas("couleur inconnue refusée", !e.colorer("chrome-marque") && e.calqueChoisi()->c.couleur == "orange");
        cas("descendre : le calque passe dessous", e.descendre() && e.choisi() == 0 && e.calques()[0].c.motif == "chiffre-1" && !e.descendre());
        cas("monter : il repasse dessus", e.monter() && e.choisi() == 1 && !e.monter());
        cas("masquer : calque masqué, absent du texte cuit", e.masquer() && e.calqueChoisi()->masque && e.texteCuisson().find("chiffre-1") == std::string::npos);
        cas("texte gardé : le masqué commence par « ! »", e.texteGarde().find(";!chiffre-1,g,") != std::string::npos);
        e.masquer();
        // annuler / refaire
        const std::string avant = e.texteGarde();
        e.supprimer();
        cas("supprimer : 1 calque, le précédent choisi", e.calques().size() == 1 && e.choisi() == 0);
        cas("annuler : le calque supprimé revient", e.annuler() && e.texteGarde() == avant);
        cas("refaire : supprimé de nouveau", e.refaire() && e.calques().size() == 1);
        e.annuler();
        // geste continu : un seul pas d'annulation (historique vidé : il était plein, 20 pas)
        e.charger(e.texteGarde()); e.choisir(0);
        const int pas = e.pasAnnuler(), rotAvant = e.calqueChoisi()->c.rotation;
        e.debutGeste();
        for (int i = 0; i < 30; i++) e.placer(i * 2, i);
        e.finGeste();
        cas("glisser (30 positions) : un seul pas d'annulation", e.pasAnnuler() == pas + 1 && e.calqueChoisi()->c.x == 58 && e.calqueChoisi()->c.y == 29);
        e.debutGeste(); e.transformer(30, 140); e.finGeste();
        cas("deux doigts : rotation 30°, 140 %, un pas", e.pasAnnuler() == pas + 2 && e.calqueChoisi()->c.rotation == 30 && e.calqueChoisi()->c.echelle == 140);
        e.debutGeste(); e.finGeste();
        cas("geste sans mouvement : aucun pas", e.pasAnnuler() == pas + 2);
        e.annuler();
        cas("annuler le geste à deux doigts : position du glisser gardée, rotation d'avant", e.calqueChoisi()->c.x == 58 && e.calqueChoisi()->c.rotation == rotAvant);
        // 20 pas d'annulation au plus
        for (int i = 0; i < 40; i++) e.tourner(PAS_ROTATION);
        cas("historique : 20 pas au plus", e.pasAnnuler() == MAX_HISTORIQUE);
        int n = 0; while (e.annuler()) n++;
        cas("20 annulations, pas une de plus", n == MAX_HISTORIQUE && e.pasRefaire() == MAX_HISTORIQUE);
        e.refaire();
        cas("une action après annuler vide « refaire »", e.tourner(PAS_ROTATION) && e.pasRefaire() == 0);
        // 20 calques
        Etat m; m.charger("");
        for (int i = 0; i < MAX_CALQUES; i++) m.ajouter("etoile-1", va::Direction::Capot);
        cas("20 calques, le 21e refusé", m.calques().size() == 20 && !m.ajouter("etoile-1", va::Direction::Capot) && m.calques().size() == 20);
    }
    // ---- RG-05 : texte gardé relu à l'identique
    {
        const std::string t = "bande-large,g,0,0,0,115,blanc,0;!chiffre-1,g,60,25,0,100,orange,0;flamme-1,d,-95,-18,30,140,rouge,1";
        Etat e; int rej = -1;
        e.charger(t, &rej);
        cas("texte gardé relu : 3 calques, 1 masqué, aucun rejet", rej == 0 && e.calques().size() == 3 && e.calques()[1].masque && e.choisi() == 2 && !e.modifie());
        cas("texte gardé réécrit à l'identique", e.texteGarde() == t);
        cas("texte cuit : 2 calques visibles, format de WFS-495", e.texteCuisson() == "bande-large,g,0,0,0,115,blanc,0;flamme-1,d,-95,-18,30,140,rouge,1");
        Etat abime; abime.charger("bande-large,g,0,0,0,100,blanc,0;inconnu,g,0,0,0,100,blanc,0;;!;eclair-1,x,0,0,0,100,blanc,0", &rej);
        cas("texte abîmé : calques illisibles ignorés et comptés, le reste gardé", rej == 3 && abime.calques().size() == 1);
        std::string vingtCinq;
        for (int i = 0; i < 25; i++) vingtCinq += (i ? ";" : "") + std::string("etoile-1,c,0,0,0,100,blanc,0");
        Etat trop; trop.charger(vingtCinq, &rej);
        cas("25 calques dans le texte : 20 gardés, 5 rejetés", trop.calques().size() == 20 && rej == 5);
    }
    // ---- RC-E18-04 : sauvegarde (clé autocollants-<voiture>), relance, texte de la course
    {
        const std::string chemin = "test_atelier_sauvegarde.txt";
        std::remove(chemin.c_str());
        Sauvegarde& s = sauvegarde();
        s.charger(chemin);
        const std::string t = "bande-large,g,0,0,0,115,blanc,0;!chiffre-1,g,60,25,0,100,orange,0;flamme-2,c,0,0,0,100,rouge,0";
        garder("p208", t);
        s.attendre();
        std::ifstream f(chemin); std::string contenu((std::istreambuf_iterator<char>(f)), {});
        cas("sauvegarde : ligne autocollants-p208=…", contenu.find("autocollants-p208=" + t + "\n") != std::string::npos);
        s.charger(chemin);
        cas("relance : texte relu à l'identique", texteGarde("p208") == t && texteGarde("golf").empty());
        cas("course : masqué et pack non possédé (flamme-2) retirés", calquesCourse("p208") == "bande-large,g,0,0,0,115,blanc,0");
        s.possedeDessins.insert("flammes");
        cas("pack « flammes » possédé : flamme-2 permise en course", motifPermis("flamme-2") && calquesCourse("p208").find("flamme-2") != std::string::npos);
        garder("p208", "");
        s.attendre(); s.charger(chemin);
        cas("texte vide : clé retirée", texteGarde("p208").empty() && calquesCourse("p208").empty());
        { std::ofstream o(chemin, std::ios::app); o << "autocollants-p208=bande-large,g,0,0,0,100,blanc,0\nautocollants-golf=BANDE=X\nautocollants-inconnue=bande-large,g,0,0,0,100,blanc,0\n"; }
        s.charger(chemin);
        cas("lecture : caractères interdits ou voiture inconnue ignorés", texteGarde("p208") == "bande-large,g,0,0,0,100,blanc,0" && texteGarde("golf").empty() &&
            !s.autocollants.count("inconnue"));
        s.attendre(); s.desactiver();
        std::remove(chemin.c_str());
    }
    std::printf("%s : %d échec(s)\n", g_echecs ? "ÉCHEC" : "OK", g_echecs);
    return g_echecs ? 1 : 0;
}
