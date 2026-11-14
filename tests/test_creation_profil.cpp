// WFS-481 : saisie de l'écran « Crée ton pilote » (creation_profil_saisie.h) : claviers AZERTY / QWERTY (41 touches, chiffres,
// « - », « _ », espace, majuscules, effacer), touches de la plaque, majuscule pour une seule lettre, plaque proposée qui suit le
// pseudo tant qu'elle n'est pas touchée (et stable avec la même graine), limites 16 / 8, erreurs RG-04 (clé de la table,
// jamais le mot saisi), scénario RG-05 (6 lettres + « C'est parti »), clés creation.* présentes en français et en anglais ;
// WFS-545 (RG-05) : un appui = un caractère, 150 ms après chaque caractère (FiltreAppuis).
// usage : test_creation_profil [liste des mots interdits] [dossier des textes]   (à lancer depuis la racine du dépôt)
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include "../src/ui/ecrans/creation_profil_saisie.h"
using namespace wfs;
using namespace wfs::ecrans::creation;
using Langue = wfs::ui::langue::Langue;

static int g_echecs = 0;
#define VERIFIER(c) do { if (!(c)) { std::printf("ÉCHEC ligne %d : %s\n", __LINE__, #c); g_echecs++; } } while (0)

static std::string lireFichier(const std::string& p) { std::ifstream f(p, std::ios::binary); std::stringstream ss; ss << f.rdbuf(); return ss.str(); }
static const Touche* trouver(const Touche* cl, char c) { for (int i = 0; i < NB_TOUCHES; i++) if (cl[i].genre == Touche::Caractere && cl[i].c == c) return &cl[i]; return nullptr; }
static const Touche* special(const Touche* cl, Touche::Genre g) { for (int i = 0; i < NB_TOUCHES; i++) if (cl[i].genre == g) return &cl[i]; return nullptr; }
// tape un texte comme le joueur : majuscules par la touche (si la majuscule armée ne correspond pas), espace, caractères
static int taper(Saisie& s, const Touche* cl, const char* texte) {
    int appuis = 0;
    for (const char* p = texte; *p; p++) {
        char c = *p; bool maj = false;
        if (c >= 'A' && c <= 'Z') { c = (char)(c - 'A' + 'a'); maj = true; }
        if (s.champ() == Champ::Pseudo && c >= 'a' && c <= 'z' && maj != s.majuscules()) { s.appuyer(*special(cl, Touche::Majuscules)); appuis++; }
        s.appuyer(c == ' ' ? *special(cl, Touche::Espace) : *trouver(cl, c)); appuis++;
    }
    return appuis;
}
static bool estErreur(const char* e, const char* cle) { return e && std::strcmp(e, cle) == 0; }

int main(int argc, char** argv) {
    const std::string liste = argc > 1 ? argv[1] : "assets/profil/mots_interdits.txt";
    const std::string textes = argc > 2 ? argv[2] : "assets/textes";
    VERIFIER(profil::chargerMotsInterdits(lireFichier(liste)) > 0);

    // ---- RG-03 : claviers
    for (Langue l : {Langue::Francais, Langue::Anglais}) {
        const Touche* cl = clavier(l);
        std::set<char> car; int maj = 0, eff = 0, esp = 0;
        std::set<std::pair<int, int>> places;
        for (int i = 0; i < NB_TOUCHES; i++) {
            const Touche& t = cl[i];
            VERIFIER(t.ligne >= 0 && t.ligne < LIGNES && t.colonne >= 0 && t.colonne + t.largeur <= COLONNES);
            for (int k = 0; k < (int)t.largeur; k++) VERIFIER(places.insert({t.ligne, (int)t.colonne + k}).second);   // aucune touche sur une autre
            if (t.genre == Touche::Caractere) VERIFIER(car.insert(t.c).second);
            maj += t.genre == Touche::Majuscules; eff += t.genre == Touche::Effacer; esp += t.genre == Touche::Espace;
        }
        VERIFIER(maj == 1 && eff == 1 && esp == 1);
        VERIFIER(car.size() == 38);   // 26 lettres, 10 chiffres, « - », « _ »
        for (char c = 'a'; c <= 'z'; c++) VERIFIER(car.count(c));
        for (char c = '0'; c <= '9'; c++) VERIFIER(car.count(c));
        VERIFIER(car.count('-') && car.count('_'));
    }
    {   // AZERTY en français, QWERTY en anglais : 1re lettre de la 1re ligne de lettres
        const Touche* fr = clavier(Langue::Francais); const Touche* en = clavier(Langue::Anglais);
        VERIFIER(trouver(fr, 'a')->ligne == 1 && trouver(fr, 'a')->colonne == 0 && trouver(fr, 'z')->colonne == 1 && trouver(fr, 'm')->ligne == 2);
        VERIFIER(trouver(en, 'q')->ligne == 1 && trouver(en, 'q')->colonne == 0 && trouver(en, 'w')->colonne == 1 && trouver(en, 'a')->ligne == 2);
        // plaque : lettres, chiffres, « - », effacer seulement
        VERIFIER(toucheActive(*trouver(fr, 'k'), Champ::Plaque) && toucheActive(*trouver(fr, '7'), Champ::Plaque) && toucheActive(*trouver(fr, '-'), Champ::Plaque));
        VERIFIER(!toucheActive(*trouver(fr, '_'), Champ::Plaque) && !toucheActive(*special(fr, Touche::Espace), Champ::Plaque) && !toucheActive(*special(fr, Touche::Majuscules), Champ::Plaque));
        VERIFIER(toucheActive(*special(fr, Touche::Effacer), Champ::Plaque) && toucheActive(*trouver(fr, '_'), Champ::Pseudo) && toucheActive(*special(fr, Touche::Espace), Champ::Pseudo));
    }

    {   // émulateur du matin (tools/qa/emulateur.sh) : « Petit » avec p, e, t, i, à la même place en AZERTY et en QWERTY, accepté
        const Touche* fr = clavier(Langue::Francais); const Touche* en = clavier(Langue::Anglais);
        for (char c : {'p', 'e', 't', 'i'}) VERIFIER(trouver(fr, c)->ligne == trouver(en, c)->ligne && trouver(fr, c)->colonne == trouver(en, c)->colonne);
        Saisie s; s.ouvrir("", 5);
        for (char c : {'p', 'e', 't', 'i', 't'}) s.appuyer(*trouver(en, c));
        VERIFIER(s.pseudo() == "Petit" && s.valide());
    }
    const Touche* cl = clavier(Langue::Francais);
    // ---- RG-02, RG-05 : 1er lancement, 6 lettres, plaque proposée gardée
    {
        Saisie s; s.ouvrir("", 42);
        VERIFIER(s.pseudo().empty() && s.champ() == Champ::Pseudo && s.majuscules() && !s.valide());
        VERIFIER(s.erreur(Champ::Pseudo) == nullptr);   // champ jamais rempli : pas de message, bouton grisé
        const int appuis = taper(s, cl, "Karima");
        VERIFIER(appuis == 6);                           // majuscule armée au départ : 6 touches, puis « C'est parti »
        VERIFIER(s.pseudo() == "Karima" && !s.majuscules());
        VERIFIER(s.plaque() == profil::plaqueProposee("Karima", nullptr) && s.plaque() == "KARIMA");
        VERIFIER(s.valide() && s.erreur(Champ::Pseudo) == nullptr && s.erreur(Champ::Plaque) == nullptr);
        Profil p; std::string nick;
        VERIFIER(p.creer(s.pseudo(), s.plaque(), nick) && nick == "Karima" && p.plaque == "KARIMA" && p.cree);   // RG-07
    }
    // ---- RG-04 : erreurs à la frappe, jamais le mot saisi
    {
        Saisie s; s.ouvrir("", 1);
        taper(s, cl, "Ka");
        VERIFIER(estErreur(s.erreur(Champ::Pseudo), "creation.erreur.pseudo_court") && !s.valide());
        s.appuyer(*special(cl, Touche::Effacer)); s.appuyer(*special(cl, Touche::Effacer));
        VERIFIER(s.pseudo().empty() && estErreur(s.erreur(Champ::Pseudo), "creation.erreur.pseudo_court"));   // vidé après frappe : le message reste
        VERIFIER(s.majuscules());   // champ vidé : majuscule de nouveau armée
        taper(s, cl, "abruti");
        VERIFIER(estErreur(s.erreur(Champ::Pseudo), "creation.erreur.mot") && !s.valide());
        for (int i = 0; i < 6; i++) s.appuyer(*special(cl, Touche::Effacer));
        taper(s, cl, "Le  Pilote");
        VERIFIER(estErreur(s.erreur(Champ::Pseudo), "creation.erreur.espaces"));
    }
    {   // 16 caractères au plus : la 17e touche ne s'écrit pas, message « 16 caractères au plus » jusqu'à l'effacement
        Saisie s; s.ouvrir("", 1);
        taper(s, cl, "abcdefghijklmnop");
        VERIFIER(s.pseudo().size() == 16 && s.valide());
        taper(s, cl, "q");
        VERIFIER(s.pseudo().size() == 16 && estErreur(s.erreur(Champ::Pseudo), "creation.erreur.pseudo_long"));
        s.appuyer(*special(cl, Touche::Effacer));
        VERIFIER(s.pseudo().size() == 15 && s.erreur(Champ::Pseudo) == nullptr);
    }
    // ---- RG-02 : plaque qui suit le pseudo, puis plus après une touche ; RG-03 : capitales, limites, tiret
    {
        Saisie s; s.ouvrir("", 9);
        const std::string secours = s.plaque();
        VERIFIER(secours.size() == 7 && secours.compare(0, 4, "WFS-") == 0);   // pseudo vide : « WFS- » + 3 chiffres
        taper(s, cl, "__");
        VERIFIER(s.plaque() == secours);   // pseudo sans lettre ni chiffre : même plaque de secours (même graine), elle ne saute pas
        s.appuyer(*special(cl, Touche::Effacer)); s.appuyer(*special(cl, Touche::Effacer));
        taper(s, cl, "Ka");
        VERIFIER(s.plaque() == "KA");      // la plaque suit le pseudo dès la 1re lettre
        taper(s, cl, "rim");
        VERIFIER(s.plaque() == "KARIM");
        s.choisirChamp(Champ::Plaque);
        s.appuyer(*special(cl, Touche::Effacer));
        VERIFIER(s.plaque() == "KARI" && s.plaqueTouchee());
        taper(s, cl, "x-9");
        VERIFIER(s.plaque() == "KARIX-9");   // lettres en capitales sans la touche majuscules
        s.appuyer(*special(cl, Touche::Espace)); s.appuyer(*trouver(cl, '_'));
        VERIFIER(s.plaque() == "KARIX-9");   // espace et « _ » sans effet sur la plaque
        taper(s, cl, "12");
        VERIFIER(s.plaque() == "KARIX-91" && estErreur(s.erreur(Champ::Plaque), "creation.erreur.plaque_long"));   // 8 au plus
        s.choisirChamp(Champ::Pseudo);
        taper(s, cl, "a");
        VERIFIER(s.pseudo() == "Karima" && s.plaque() == "KARIX-91");   // plaque touchée : elle ne suit plus le pseudo
        s.choisirChamp(Champ::Plaque);
        for (int i = 0; i < 8; i++) s.appuyer(*special(cl, Touche::Effacer));
        VERIFIER(s.plaque().empty() && estErreur(s.erreur(Champ::Plaque), "creation.erreur.plaque_vide") && !s.valide());
        taper(s, cl, "ab-");
        VERIFIER(estErreur(s.erreur(Champ::Plaque), "creation.erreur.tiret") && !s.valide());
    }
    {   // joueur déjà installé : pseudo actuel proposé, plaque d'après lui, valide tout de suite
        Saisie s; s.ouvrir(Profil::pseudoPropose("Lina_77"), 3);
        VERIFIER(s.pseudo() == "Lina_77" && s.plaque() == "LINA77" && s.valide() && !s.majuscules());
        Saisie d; d.ouvrir(Profil::pseudoPropose("Pilote"), 3);
        VERIFIER(d.pseudo().empty());   // pseudo par défaut : rien de proposé
    }
    // ---- RG-06 : chaque clé de l'écran dans les deux tables, et chaque clé d'erreur rendue par la saisie
    {
        std::map<std::string, std::string> fr, en;
        VERIFIER(ui::langue::analyserTable(lireFichier(textes + "/fr.txt"), fr) == 0);
        VERIFIER(ui::langue::analyserTable(lireFichier(textes + "/en.txt"), en) == 0);
        const char* cles[] = {"creation.titre", "creation.pseudo", "creation.plaque", "creation.indice", "creation.valider", "creation.espace",
                              "creation.erreur.pseudo_court", "creation.erreur.pseudo_long", "creation.erreur.espaces", "creation.erreur.mot",
                              "creation.erreur.plaque_vide", "creation.erreur.plaque_long", "creation.erreur.tiret"};
        for (const char* c : cles) { VERIFIER(fr.count(c) && !fr[c].empty()); VERIFIER(en.count(c) && !en[c].empty()); }
        VERIFIER(fr["creation.titre"] == "Crée ton pilote" && en["creation.titre"] == "Create your driver");
        VERIFIER(en["creation.pseudo"] == "Nickname" && en["creation.plaque"] == "Licence plate" && en["creation.valider"] == "Let's go");
        VERIFIER(fr["creation.valider"] == "C'est parti");
    }
    // WFS-545 (RG-05) : filtre des appuis du clavier, branché comme dans toucher() de l'écran (creation_profil.cpp) :
    // poser(id, down) d'abord (un appui = le poser d'un doigt), puis accepter(heure) pour chaque caractère (150 ms)
    {
        const Touche* cl = clavier(Langue::Francais);
        auto toucher = [&](FiltreAppuis& f, Saisie& s, int id, bool down, double t, const Touche& k) {
            if (f.poser(id, down) && f.accepter(t)) s.appuyer(k);
        };
        // RG-01 : un appui suivi de 10 mouvements du même doigt, puis le lever → 1 caractère
        { FiltreAppuis f; Saisie s; s.ouvrir("", 1);
          toucher(f, s, 0, true, 1.000, *trouver(cl, 'r'));
          for (int k = 0; k < 10; k++) toucher(f, s, 0, true, 1.000 + 0.016 * (k + 1), *trouver(cl, 'r'));
          toucher(f, s, 0, false, 1.200, *trouver(cl, 'r'));
          VERIFIER(s.pseudo() == "R");
          // le doigt levé, un nouvel appui 300 ms plus tard écrit de nouveau
          toucher(f, s, 0, true, 1.500, *trouver(cl, 'r')); toucher(f, s, 0, false, 1.550, *trouver(cl, 'r'));
          VERIFIER(s.pseudo() == "Rr"); }
        // RG-02 : deux appuis à 50 ms d'écart (deux doigts) → 1 caractère ; à 200 ms → 2 caractères
        { FiltreAppuis f; Saisie s; s.ouvrir("", 1);
          toucher(f, s, 0, true, 2.000, *trouver(cl, 'j')); toucher(f, s, 0, false, 2.020, *trouver(cl, 'j'));
          toucher(f, s, 1, true, 2.050, *trouver(cl, 'j')); toucher(f, s, 1, false, 2.070, *trouver(cl, 'j'));
          VERIFIER(s.pseudo() == "J"); }
        { FiltreAppuis f; Saisie s; s.ouvrir("", 1);
          toucher(f, s, 0, true, 2.000, *trouver(cl, 'j')); toucher(f, s, 0, false, 2.020, *trouver(cl, 'j'));
          toucher(f, s, 1, true, 2.200, *trouver(cl, 'j')); toucher(f, s, 1, false, 2.220, *trouver(cl, 'j'));
          VERIFIER(s.pseudo() == "Jj"); }
        // même chose pour « effacer » : maintenu (10 mouvements) → 1 caractère retiré ; 2 appuis à 50 ms → 1 ; à 200 ms → 2
        const Touche& eff = *special(cl, Touche::Effacer);
        { FiltreAppuis f; Saisie s; s.ouvrir("Karima", 1);
          toucher(f, s, 3, true, 5.000, eff);
          for (int k = 0; k < 10; k++) toucher(f, s, 3, true, 5.000 + 0.016 * (k + 1), eff);
          toucher(f, s, 3, false, 5.300, eff);
          VERIFIER(s.pseudo() == "Karim");
          toucher(f, s, 4, true, 6.000, eff); toucher(f, s, 4, false, 6.010, eff);
          toucher(f, s, 5, true, 6.050, eff); toucher(f, s, 5, false, 6.060, eff);
          VERIFIER(s.pseudo() == "Kari");
          toucher(f, s, 6, true, 6.200, eff); toucher(f, s, 6, false, 6.210, eff);
          VERIFIER(s.pseudo() == "Kar"); }
        // tout juste 150 ms : accepté ; 149 ms : refusé ; horloge revenue en arrière (nouvelle partie) : accepté
        { FiltreAppuis f;
          VERIFIER(f.accepter(10.0)); VERIFIER(!f.accepter(10.149)); VERIFIER(f.accepter(10.150)); VERIFIER(f.accepter(3.0)); }
        // un mouvement ou un lever d'un doigt jamais posé n'est pas un appui ; oublier() remet le filtre à neuf
        { FiltreAppuis f;
          VERIFIER(!f.poser(7, false)); VERIFIER(f.poser(7, true)); VERIFIER(!f.poser(7, true));
          f.oublier(); VERIFIER(f.poser(7, true)); VERIFIER(f.accepter(0.0)); }
        // deux doigts posés en même temps : chacun est un appui (le délai de 150 ms départage les caractères)
        { FiltreAppuis f; VERIFIER(f.poser(0, true)); VERIFIER(f.poser(1, true)); VERIFIER(!f.poser(0, true)); VERIFIER(!f.poser(1, false)); VERIFIER(f.poser(1, true)); }
        std::printf("WFS-545 : un appui = un caractère, 150 ms après chaque caractère (caractères et « effacer ») : vérifié\n");
    }
    std::printf("test_creation_profil : %s (%d échec(s))\n", g_echecs ? "ÉCHEC" : "OK", g_echecs);
    return g_echecs ? 1 : 0;
}
