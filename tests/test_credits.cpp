// WaForSpeed — test des crédits (WFS-406) : lecture de CREDITS.txt (credits::paragraphes) et contenu du vrai fichier
// assets/licences/CREDITS.txt (RG-01, RG-02, RG-06). Lancé depuis la racine du dépôt (tools/qa/tests_cpu.sh).
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include "../src/ui/credits.h"
#include "../src/ui/langue_regles.h"   // WFS-505 : garde-fou « phrase française » de WFS-504

using namespace wfs::credits;
static int g_echecs = 0;
static void cas(const char* nom, bool ok) { std::printf("%s %s\n", ok ? "OK   " : "ÉCHEC", nom); if (!ok) g_echecs++; }

static std::string tout(const std::vector<Paragraphe>& v) { std::string s; for (const auto& p : v) s += p.texte + "\n"; return s; }

int main() {
    // règles de lecture
    const std::string essai =
        "Titre\n\nIntro du fichier.\n\nVoitures (modèles) :\n  Norvik Lume : auteur — licence CC BY 4.0 : https://x\n\n"
        "Texte de la licence MIT (goéland) :\nMIT License\n\nPermission is hereby granted,\nfree of charge.\n\nTHE SOFTWARE\n";
    {
        const auto v = paragraphes(essai, "");
        cas("1re ligne : titre", !v.empty() && v[0].style == Paragraphe::TITRE && v[0].texte == "Titre");
        bool section = false, entree = false, recolle = false, sansConf = true;
        for (const auto& p : v) {
            section |= p.style == Paragraphe::SECTION && p.texte == "Voitures (modèles)";
            entree |= p.style == Paragraphe::ENTREE && p.texte.rfind("Norvik Lume :", 0) == 0;
            recolle |= p.style == Paragraphe::TEXTE && p.texte == "Permission is hereby granted, free of charge.";
            sansConf &= p.texte.find("confidentialité") == std::string::npos;
        }
        cas("ligne « … : » sans retrait : section (sans les deux-points)", section);
        cas("ligne en retrait : entrée", entree);
        cas("texte de licence : lignes d'un paragraphe recollées", recolle);
        cas("RG-06 : adresse vide → aucune ligne de confidentialité (rien de provisoire)", sansConf);
        cas("pas de ligne vide en fin", v.back().style != Paragraphe::VIDE);
    }
    {
        const auto v = paragraphes(essai, "https://exemple.org/confidentialite");
        cas("RG-06 : adresse donnée → « Politique de confidentialité : <adresse> » juste après le titre",
            v.size() > 1 && v[1].texte == "Politique de confidentialité : https://exemple.org/confidentialite");
    }
    cas("texte vide : aucun paragraphe, pas de plantage", paragraphes("", "").empty() && paragraphes("\n\n\n", "").empty());

    // vrai fichier livré dans l'APK
    std::ifstream f("assets/licences/CREDITS.txt", std::ios::binary);
    std::stringstream ss; ss << f.rdbuf();
    const std::string texte = ss.str();
    cas("assets/licences/CREDITS.txt lu", !texte.empty());
    const std::string t = tout(paragraphes(texte, ""));
    for (const char* attendu : {"Norvik Lume", "Draven Vantor GT", "Alvar Selene TS",                         // RG-01
                                "neshallads", "DevanirGrau", "valvetin",                                      // deux-roues
                                "Mike Koenig", "Stephan", "SpeedJunkie", "nhaudio", "Quaternius", "MakeHuman", "svartmc", "Poly Haven",
                                "Rajdhani", "android_native_app_glue", "The Android Open Source Project", "Apache License",
                                "https://creativecommons.org/licenses/by/4.0/", "https://www.apache.org/licenses/LICENSE-2.0"})
        cas((std::string("crédits : ") + attendu).c_str(), t.find(attendu) != std::string::npos);
    for (const char* interdit : {"Folg", "Bnv", "R4 Competizione", "Giuliva", "Trefoil", "Volkswagen", "BMW", "Porsche", "Ferrari",
                                 "Lamborghini", "Kawasaki", "Honda"})
        cas((std::string("crédits sans « ") + interdit + " »").c_str(), t.find(interdit) == std::string::npos);
    // WFS-505 (RG-02) : crédits en anglais. Règles de la traduction, puis le vrai fichier traduit par textes/credits-en.txt :
    // aucun mot français (garde-fou de WFS-504) hors texte des licences ; auteurs, œuvres et licences inchangés
    {
        const std::string table = "# commentaire\nmodèle 3D par |3D model by \n — licence | — license \nmodèle |model \n";
        cas("traduction : plus long morceau d'abord, typographie anglaise",
            traduireCredits("Norvik Lume : modèle 3D par Ricy — licence CC BY 4.0 : https://x", table) == "Norvik Lume: 3D model by Ricy — license CC BY 4.0: https://x");
        cas("traduction : guillemets « » → “ ”", traduireCredits("modèle « seagull »", table) == "model \xE2\x80\x9Cseagull\xE2\x80\x9D");
        cas("traduction : table vide → texte inchangé sauf typographie", traduireCredits("Rajdhani : OFL", "") == "Rajdhani: OFL");
    }
    {
        std::ifstream fe("assets/textes/credits-en.txt", std::ios::binary);
        std::stringstream se; se << fe.rdbuf();
        const std::string table = se.str();
        cas("assets/textes/credits-en.txt lu", !table.empty());
        std::vector<Paragraphe> v = paragraphes(texte, "https://exemple.org/confidentialite");
        traduireParagraphes(v, table);
        int francais = 0;
        bool licence = false;
        for (const Paragraphe& p : v) {
            if (p.style == Paragraphe::SECTION) licence = p.texte.find("license text") != std::string::npos;
            if (p.style == Paragraphe::VIDE || (licence && p.style == Paragraphe::TEXTE)) continue;
            const std::string raison = wfs::ui::langue::phraseFrancaise(p.texte);
            if (!raison.empty()) { francais++; std::printf("ÉCHEC crédits en anglais (%s) : « %s » → ajouter sa ligne dans assets/textes/credits-en.txt\n", raison.c_str(), p.texte.c_str()); }
        }
        cas("crédits en anglais : aucun mot français (hors texte des licences)", francais == 0);
        const std::string en = tout(v);
        cas("crédits en anglais : titre « WaForSpeed — credits »", !v.empty() && v[0].texte == "WaForSpeed — credits");
        cas("crédits en anglais : « Privacy policy: <adresse> »", v.size() > 1 && v[1].texte == "Privacy policy: https://exemple.org/confidentialite");
        for (const char* garde : {"Norvik Lume: 3D model by GT Cars: Hyperspeed (Sketchfab) — license CC BY 4.0: https://creativecommons.org/licenses/by/4.0/",
                                  "Quaternius", "MakeHuman", "svartmc", "Poly Haven", "Indian Type Foundry", "SIL OFL 1.1", "CC BY-SA 3.0",
                                  "CC0 1.0", "Permission is hereby granted", "Apache License"})
            cas((std::string("crédits en anglais : gardé « ") + garde + " »").c_str(), en.find(garde) != std::string::npos);
    }
    std::printf("%s\n", g_echecs ? "test_credits : ÉCHEC" : "test_credits : tout est bon");
    return g_echecs ? 1 : 0;
}
