// WFS-489 (RG-03) : prix montrés par la rangée des couleurs du garage et ce qui manque pour partir en course.
// 8 couleurs gratuites sans prix ; couleur vive 50 ★ sur la 208 (classe gratuite), 75 ★ sur la Golf (× 1,5) ; « Toit et rétros » :
// droit de la couleur secondaire + couleur ; couleur achetée sur la 208 toujours payante sur la Golf ; même couleur en principale et
// en secondaire comptée une fois ; tout possédé → rien ne manque (« Choisir ») ; achat refusé sans assez d'étoiles, rien de débité.
#include <cstdio>
#include <cstring>
#include "../src/game/boutique.h"
#include "../src/game/cars.h"
#include "../src/ui/garage_achat.h"
using namespace wfs;
using namespace wfs::ecrans::garage;

static int g_echecs = 0;
static void cas(const char* nom, bool ok) {
    std::printf("%s %s\n", ok ? "OK   " : "ÉCHEC", nom);
    if (!ok) g_echecs++;
}
static int P(const char* id) { for (int i = 0; i < ecrans::garage::NB_PEINTURES; i++) if (!std::strcmp(PAINTS[i].id, id)) return i; return -1; }
static int V(const char* id) { for (int i = 0; i < CAR_COUNT; i++) if (!std::strcmp(CARS[i].id, id)) return i; return -1; }

int main() {
    Sauvegarde s;   // inactive : rien n'est écrit sur le disque
    cas("28 couleurs dans le garage", ecrans::garage::NB_PEINTURES == 28 && P("bleuciel") == 27);
    // RG-03 : cadenas et prix
    cas("couleur gratuite (Rouge) : aucun prix, sur la 208 et la Golf", prixPastille(s, "p208", P("rouge"), false) == 0 && prixPastille(s, "golf", P("rouge"), false) == 0);
    cas("Bleu électrique : 50 ★ sur la 208, 75 ★ sur la Golf", prixPastille(s, "p208", P("bleuelectrique"), false) == 50 && prixPastille(s, "golf", P("bleuelectrique"), false) == 75);
    cas("toit et rétros Noir (gratuit) sur la 208 : droit 50 ★", prixPastille(s, "p208", P("noir"), true) == 50);
    cas("toit et rétros Rose sur la Golf : droit 75 + couleur 75 = 150 ★", prixPastille(s, "golf", P("rose"), true) == 150);
    cas("indice hors liste : 0 (jamais de lecture hors du tableau)", prixPastille(s, "p208", -1, false) == 0 && prixPastille(s, "p208", ecrans::garage::NB_PEINTURES, true) == 0);
    // ce qui manque pour l'aspect montré
    EtatGarage e; e.index = V("p208"); e.peinture = P("rouge"); e.voiturePrete = true;
    cas("208 Rouge, comme la carrosserie : rien ne manque (« Choisir »)", manque(s, e).n == 0 && manque(s, e).total == 0);
    e.peinture = P("bleuelectrique"); e.peinture2 = P("noir");
    const Manque m = manque(s, e);
    cas("208 Bleu électrique + toit Noir : couleur 50 + droit 50 = 100 ★, 2 objets", m.n == 2 && m.total == 100);
    e.peinture2 = P("bleuelectrique");
    cas("même couleur en principale et en secondaire : comptée une fois (50 + 50)", manque(s, e).n == 2 && manque(s, e).total == 100);
    // achat : refus puis réussite
    e.peinture2 = -1;
    s.etoiles = 30;
    const perso::Objet* bleu = perso::couleur("bleuelectrique");
    cas("refus faute d'étoiles : rien de débité, couleur non possédée",
        bleu && boutique::acheterObjet(s, *bleu, "p208") == perso::Achat::SoldeInsuffisant && s.etoiles == 30 && manque(s, e).total == 50);
    s.etoiles = 200;
    cas("achat Bleu électrique sur la 208 : 50 ★ débitées une fois, plus rien ne manque",
        bleu && boutique::acheterObjet(s, *bleu, "p208") == perso::Achat::Achete && s.etoiles == 150 && manque(s, e).n == 0);
    cas("couleur achetée pour la 208 : toujours 75 ★ sur la Golf", prixPastille(s, "golf", P("bleuelectrique"), false) == 75);
    cas("toit et rétros Bleu électrique sur la 208 : seulement le droit (50 ★)", prixPastille(s, "p208", P("bleuelectrique"), true) == 50);
    std::printf("%s : %d échec(s)\n", g_echecs ? "ÉCHEC" : "OK", g_echecs);
    return g_echecs ? 1 : 0;
}
