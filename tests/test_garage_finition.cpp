// WFS-491 (RG-01 à RG-03) : rangée « Finition » du garage : ordre affiché, prix (base × coefficient de la classe), lien avec le
// rendu de WFS-490, ce qui manque pour partir en course (finition comprise), refus sans débit, finition achetée pour une seule voiture.
#include <cstdio>
#include <cstring>
#include "../src/game/boutique.h"
#include "../src/game/cars.h"
#include "../src/ui/garage_achat.h"
#include "../src/ui/garage_finition.h"
using namespace wfs;
using namespace wfs::ecrans::garage;
namespace vf = wfs::vehicule_finitions;

static int g_echecs = 0;
static void cas(const char* nom, bool ok) {
    std::printf("%s %s\n", ok ? "OK   " : "ÉCHEC", nom);
    if (!ok) g_echecs++;
}
static int P(const char* id) { for (int i = 0; i < ecrans::garage::NB_PEINTURES; i++) if (!std::strcmp(PAINTS[i].id, id)) return i; return -1; }
static int V(const char* id) { for (int i = 0; i < CAR_COUNT; i++) if (!std::strcmp(CARS[i].id, id)) return i; return -1; }
static int F(const char* id) { for (int i = 0; i < BOUTIQUE_FINITIONS; i++) if (!std::strcmp(BOUTIQUE_FINITIONS_IDS[i], id)) return i; return -1; }
static int prix(const char* fin, const char* car) { const perso::Objet* o = perso::objet(fin); return o ? perso::prix(*o, car) : -1; }

int main() {
    // RG-01 : ordre affiché
    const char* attendu[] = {"brillant", "satine", "mat", "metallise", "nacree", "candy", "chrome", "cameleon"};
    bool ordre = FinitionGarage::NB == 8;
    for (int o = 0; o < 8; o++) ordre = ordre && !std::strcmp(BOUTIQUE_FINITIONS_IDS[FinitionGarage::catalogue(o)], attendu[o]);
    cas("ordre : Brillant, Satiné, Mat, Métallisé, Nacré, Candy, Chromé, Caméléon", ordre);
    cas("ordre hors bornes : brillant", FinitionGarage::catalogue(-1) == 0 && FinitionGarage::catalogue(8) == 0);
    // lien avec le rendu (vehicule_finitions.h, WFS-490)
    cas("rendu : nacree → NACRE, chrome → CHROME, cameleon → CAMELEON, mat → MAT",
        FinitionGarage::rendu(F("nacree")) == vf::NACRE && FinitionGarage::rendu(F("chrome")) == vf::CHROME &&
        FinitionGarage::rendu(F("cameleon")) == vf::CAMELEON && FinitionGarage::rendu(F("mat")) == vf::MAT);
    bool tous = true;
    for (int f = 0; f < BOUTIQUE_FINITIONS; f++) tous = tous && FinitionGarage::rendu(f) >= 0 && FinitionGarage::rendu(f) < vf::NOMBRE;
    cas("rendu : chaque finition du catalogue a son rendu ; hors bornes → brillant", tous && FinitionGarage::rendu(-1) == vf::BRILLANT && FinitionGarage::rendu(99) == vf::BRILLANT);
    cas("depuisId(\"nacree\") = NACRE (identifiant du catalogue)", vf::depuisId("nacree") == vf::NACRE && vf::depuisId("nacre") == vf::NACRE);
    // RG-01 : prix = base × coefficient de la classe (configgame.json)
    cas("500 (classe gratuite, × 1) : métallisé 100, nacré / mat / satiné 150, candy 250, chromé / caméléon 400 ★",
        prix("metallise", "abarth") == 100 && prix("nacree", "abarth") == 150 && prix("mat", "abarth") == 150 && prix("satine", "abarth") == 150 &&
        prix("candy", "abarth") == 250 && prix("chrome", "abarth") == 400 && prix("cameleon", "abarth") == 400);
    cas("Golf (classe 2, × 1,5) : caméléon 600 ★ ; Giulia (classe 3, × 2) : mat 300 ★", prix("cameleon", "golf") == 600 && prix("mat", "giulia") == 300);
    cas("brillant gratuit partout", prix("brillant", "golf") == 0 && prix("brillant", "giulia") == 0);
    // ce qui manque : la finition s'ajoute à la couleur
    Sauvegarde s;   // inactive : rien n'est écrit sur le disque
    EtatGarage e; e.index = V("golf"); e.peinture = P("rouge"); e.voiturePrete = true;
    cas("Golf Rouge Brillant : rien ne manque (« Choisir »)", manque(s, e).n == 0);
    e.finition = F("cameleon");
    cas("Golf Rouge Caméléon : 600 ★, 1 objet", manque(s, e).n == 1 && manque(s, e).total == 600);
    e.peinture = P("bleuelectrique"); e.peinture2 = P("rose");
    cas("Golf Bleu électrique + toit Rose + Caméléon : 75 + 75 + 75 + 600 = 825 ★, 4 objets", manque(s, e).n == 4 && manque(s, e).total == 825);
    e.peinture = P("rouge"); e.peinture2 = -1;
    cas("finition hors bornes : ignorée", [&] { EtatGarage x = e; x.finition = 42; return manque(s, x).n == 0; }());
    // achat : refus puis réussite, une seule voiture
    const perso::Objet* cam = perso::objet("cameleon");
    s.etoiles = 100;
    cas("refus faute d'étoiles : rien de débité", cam && boutique::acheterObjet(s, *cam, "golf") == perso::Achat::SoldeInsuffisant && s.etoiles == 100 && manque(s, e).total == 600);
    s.etoiles = 1000;
    cas("achat Caméléon sur la Golf : 600 ★ débitées une fois, plus rien ne manque",
        cam && boutique::acheterObjet(s, *cam, "golf") == perso::Achat::Achete && s.etoiles == 400 && manque(s, e).n == 0);
    cas("deuxième achat : déjà possédé, rien de débité", boutique::acheterObjet(s, *cam, "golf") == perso::Achat::DejaPossede && s.etoiles == 400);
    EtatGarage g = e; g.index = V("giulia");
    cas("Caméléon acheté pour la Golf : toujours payant sur la Giulia (800 ★)", manque(s, g).n == 1 && manque(s, g).total == 800);
    std::printf("%s : %d échec(s)\n", g_echecs ? "ÉCHEC" : "OK", g_echecs);
    return g_echecs ? 1 : 0;
}
