// WaForSpeed — test des crédits (WFS-406) : lecture de CREDITS.txt (credits::paragraphes) et contenu du vrai fichier
// assets/licences/CREDITS.txt (RG-01, RG-02, RG-06). Lancé depuis la racine du dépôt (tools/qa/tests_cpu.sh).
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include "../src/ui/credits.h"

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
    std::printf("%s\n", g_echecs ? "test_credits : ÉCHEC" : "test_credits : tout est bon");
    return g_echecs ? 1 : 0;
}
