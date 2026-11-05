// BinaryPack (WFS-89) : mêmes octets que peerjs-js-binarypack 2.1.0 (PeerJS 1.5.4), lecture des messages du jeu web,
// messages hostiles refusés sans planter. Références : node + peerjs-js-binarypack (pack de chaque valeur, en hexadécimal).
// usage : test_binarypack
#include <cmath>
#include <cstdio>
#include <string>
#include "../src/game/binarypack.h"
using namespace wfs::bp;

static int echecs = 0;
#define VERIFIE(c, ...) do { if (!(c)) { std::printf("ÉCHEC : " __VA_ARGS__); std::printf("\n"); echecs++; } } while (0)

static std::string hexa(const std::vector<uint8_t>& o) { std::string s; char b[3]; for (uint8_t x : o) { std::snprintf(b, 3, "%02x", x); s += b; } return s; }
static std::vector<uint8_t> octets(const std::string& h) { std::vector<uint8_t> o; for (size_t i = 0; i + 1 < h.size(); i += 2) o.push_back((uint8_t)std::stoi(h.substr(i, 2), nullptr, 16)); return o; }
static Valeur nb(double v) { return Valeur::nombre(v); }
static Valeur tx(const char* s) { return Valeur::texte(s); }

int main() {
    // ---------- octets identiques à PeerJS ----------
    Valeur petits = Valeur::tableau();
    for (double v : {0., 1., 127., -1., -32., -33., 128., 255., -128., -129., 256., 65535., -32768., -32769., 65536., 4294967295., -2147483648., 5000000000., -5000000000.})
        petits.tab.push_back(nb(v));
    VERIFIE(hexa(pack(petits)) == "dc001300017fffe0d0dfcc80ccffd080d1ff7fcd0100cdffffd18000d2ffff7fffce00010000ceffffffffd280000000d3000000012a05f200d3fffffffed5fa0e00",
            "entiers : %s", hexa(pack(petits)).c_str());
    Valeur doubles = Valeur::tableau();
    for (double v : {3.14, -0.45, 0.012, 1e-7, 123456.789}) doubles.tab.push_back(nb(v));
    VERIFIE(hexa(pack(doubles)) == "95cb40091eb851eb851fcbbfdccccccccccccdcb3f889374bc6a7efacb3e7ad7f29abcaf48cb40fe240c9fbe76c9", "doubles : %s", hexa(pack(doubles)).c_str());
    Valeur textes = Valeur::tableau();
    for (const char* s : {"", "Alex", "Élodie é😀", "un texte de plus de quinze caractères"}) textes.tab.push_back(tx(s));
    VERIFIE(hexa(pack(textes)) == "94b0b4416c6578bec3896c6f64696520c3a9f09f9880d80026756e20746578746520646520706c7573206465207175696e7a6520636172616374c3a8726573",
            "textes : %s", hexa(pack(textes)).c_str());
    Valeur divers = Valeur::tableau();
    divers.tab = {Valeur::nul(), Valeur::booleen(true), Valeur::booleen(false), Valeur::tableau(), Valeur::objet()};
    VERIFIE(hexa(pack(divers)) == "95c0c3c29080", "divers : %s", hexa(pack(divers)).c_str());
    // présence envoyée au relais ({t:'p', presence}) : même ordre des clés que le jeu web
    Valeur pres = Valeur::objet();
    pres.ajoute("n", tx("Alex")); pres.ajoute("st", tx("lobby")); pres.ajoute("r", nb(0)); pres.ajoute("d", nb(12345)); pres.ajoute("x", nb(-0.45));
    pres.ajoute("v", nb(250)); pres.ajoute("h", nb(0.012)); pres.ajoute("b", nb(0)); pres.ajoute("ft", Valeur::nul()); pres.ajoute("car", tx("golf"));
    pres.ajoute("paint", tx("bleu")); pres.ajoute("host", Valeur::booleen(true)); pres.ajoute("map", tx("ville")); pres.ajoute("tod", tx("nuit")); pres.ajoute("km", nb(4));
    Valeur p = Valeur::objet(); p.ajoute("t", tx("p")); p.ajoute("presence", pres);
    const std::string refP = "82b174b170b870726573656e63658fb16eb4416c6578b27374b56c6f626279b17200b164cd3039b178cbbfdccccccccccccdb176ccfab168cb3f889374bc6a7efab16200b26674c0b3636172b4676f6c66b57061696e74b4626c6575b4686f7374c3b36d6170b576696c6c65b3746f64b46e756974b26b6d04";
    VERIFIE(hexa(pack(p)) == refP, "présence : %s", hexa(pack(p)).c_str());
    std::printf("BinaryPack : octets identiques à PeerJS (entiers, doubles, textes UTF-8, divers, présence) : %s\n", echecs ? "ÉCHEC" : "OK");

    // ---------- lecture d'un message du relais du jeu web ({t:'all', list}) ----------
    const std::string refAll = "82b174b3616c6cb46c6973749292d800167761666f7273706565642d7766732d6b3771782d763184b16eb54e61646961b27374b57265616479b17201b4686f7374c392b633663261396382b16eb3426f62b164ce00011170";
    Valeur all;
    VERIFIE(unpack(octets(refAll), all), "lecture de {t:'all'}");
    const Valeur* t = all.champ("t"); const Valeur* list = all.champ("list");
    VERIFIE(t && t->type == Valeur::Texte && t->s == "all" && list && list->type == Valeur::Tableau && list->tab.size() == 2, "structure de {t:'all'}");
    if (list && list->tab.size() == 2) {
        const Valeur& j0 = list->tab[0];
        VERIFIE(j0.tab.size() == 2 && j0.tab[0].s == "waforspeed-wfs-k7qx-v1" && j0.tab[1].champ("host") && j0.tab[1].champ("host")->b &&
                j0.tab[1].champ("r")->n == 1 && j0.tab[1].champ("n")->s == "Nadia", "premier joueur de la liste");
        VERIFIE(list->tab[1].tab[1].champ("d")->n == 70000, "distance 70000 (uint32)");
    }
    // aller-retour
    Valeur q;
    VERIFIE(unpack(pack(p), q) && hexa(pack(q)) == refP, "aller-retour de la présence");
    VERIFIE(unpack(pack(petits), q) && q.tab.size() == 19 && q.tab[17].n == 5000000000. && q.tab[18].n == -5000000000. && q.tab[9].n == -129, "aller-retour des entiers");
    VERIFIE(unpack(pack(doubles), q) && q.tab[1].n == -0.45 && q.tab[4].n == 123456.789, "aller-retour des doubles");
    std::printf("BinaryPack : lecture du relais du jeu web et allers-retours : %s\n", echecs ? "ÉCHEC" : "OK");

    // ---------- messages hostiles : refusés, jamais de plantage ----------
    std::vector<uint8_t> tronque = octets(refAll); tronque.resize(tronque.size() - 5);
    VERIFIE(!unpack(tronque, q), "message tronqué accepté");
    VERIFIE(!unpack(octets("dd7fffffff"), q), "tableau de 2 milliards d'éléments accepté");
    VERIFIE(!unpack(octets("df00100000"), q), "objet géant accepté");
    std::vector<uint8_t> profond(40, 0x91); profond.push_back(0x00);
    VERIFIE(!unpack(profond, q), "imbrication de 40 niveaux acceptée");
    VERIFIE(!unpack(octets("c4"), q) && !unpack(octets("d9ffffffff"), q) && !unpack(std::vector<uint8_t>{}, q), "octets inconnus ou vides acceptés");
    for (int k = 0; k < 2000; k++) {   // octets au hasard : aucun plantage
        std::vector<uint8_t> h(1 + k % 40); uint32_t s = 2463534242u + k;
        for (auto& x : h) { s ^= s << 13; s ^= s >> 17; s ^= s << 5; x = (uint8_t)s; }
        unpack(h, q);
    }
    std::printf("BinaryPack : messages hostiles refusés (tronqué, géant, trop profond, octets au hasard) : %s\n", echecs ? "ÉCHEC" : "OK");
    if (echecs) { std::printf("%d échec(s)\n", echecs); return 1; }
    std::printf("binarypack : tout est OK\n");
    return 0;
}
