// WFS-500 (RG-11) : cadeau du jour. (a) 100 000 tirages, garantie coupée, toutes les catégories pleines : chaque chance à ±0,5 point ;
// (b) jamais moins de 10 ★ (replis compris) ; (c) garantie au 30e ; (d) catégories vides → étoiles ≥ 10 ; (e) jour local : 23:59 puis 00:01
// → nouveau cadeau, deux ouvertures le même jour → la 2e refusée ; (f) heure reculée de 2 jours → aucun cadeau tant que l'heure du
// dernier n'est pas repassée ; (g) sauvegarde écrite puis relue (toutes les clés), ancienne sauvegarde → cadeau disponible ;
// (h) total des chances ≠ 100 refusé. En plus : calendrier, résultat écrit / relu, entrée faite depuis la sauvegarde.
#include <cmath>
#include <cstdio>
#include <string>
#include "../src/core/configgame.h"
#include "../src/game/boutique.h"
#include "../src/game/cadeau.h"
using namespace wfs;
using namespace wfs::cadeau;

static int g_echecs = 0;
static void cas(const std::string& nom, bool ok) {
    std::printf("%s %s\n", ok ? "OK   " : "ÉCHEC", nom.c_str());
    if (!ok) g_echecs++;
}

// entrée « pleine » : 3 voitures avec des coloris et des skins à gagner, 4 voitures à prêter
static Entree entreePleine() {
    Entree e;
    e.voitures = {{"p208", {"rose", "cyan", "mat"}, {"rallye", "neonnuit"}},
                  {"abarth", {"magenta", "chrome"}, {"drifttokyo"}},
                  {"gt86", {"corail"}, {"camouflage", "rallye"}}};
    e.voituresPretables = {"m4", "golf", "sf90", "corsa"};
    return e;
}
// instant à une heure locale donnée : jour civil (AAAA, MM, JJ), hh:mm:ss locales, décalage du fuseau (s)
static Instant local(int a, int m, int j, int hh, int mm, int ss, int decalage) {
    char b[16]; std::snprintf(b, sizeof b, "%04d-%02d-%02d", a, m, j);
    long long jour = 0; lireJour(b, jour);
    return {jour * 86400 + hh * 3600LL + mm * 60LL + ss - decalage, decalage};
}

int main() {
    const ConfigJeu::Cadeau cfg{};   // valeurs du jeu (= configgame.json)

    // ---- calendrier et jour local (RC-E19-04)
    {
        long long j = -1;
        cas("1970-01-01 = jour 0", lireJour("1970-01-01", j) && j == 0 && texteJour(0) == "1970-01-01");
        cas("2024-02-29 existe, 2025-02-29 et 2026-04-31 refusés", lireJour("2024-02-29", j) && !lireJour("2025-02-29", j) && !lireJour("2026-04-31", j));
        cas("2026-11-13 aller-retour", lireJour("2026-11-13", j) && texteJour(j) == "2026-11-13");
        const Instant t = local(2026, 11, 13, 0, 30, 0, 3600);   // 00:30 à Paris = 23:30 UTC la veille
        cas("jour local à Paris (00:30) ≠ jour UTC (23:30 la veille)", texteJour(jourLocal(t)) == "2026-11-13" && texteJour(jourLocal({t.epoch, 0})) == "2026-11-12");
        const Instant u = local(2026, 11, 13, 23, 0, 0, -5 * 3600);   // 23:00 à New York = 04:00 UTC le lendemain
        cas("jour local à New York (23:00, décalage négatif)", texteJour(jourLocal(u)) == "2026-11-13");
        const Instant m = maintenant();
        cas("maintenant() : heure et décalage plausibles", m.epoch > 1700000000LL && m.decalage >= -18 * 3600 && m.decalage <= 18 * 3600);
    }

    // ---- (a) 100 000 tirages, garantie coupée (compteur remis à 0 avant chaque tirage), toutes les catégories pleines
    {
        const Entree e = entreePleine();
        Mulberry32 h(500);
        const int N = 100000;
        long long n[CADEAU_TRANCHES + 3] = {};
        for (int i = 0; i < N; i++) {
            int sansRare = 0;
            const Resultat r = tirer(cfg, e, sansRare, h);
            int k = 0;
            if (r.gain == Gain::Etoiles) { for (k = 0; k < CADEAU_TRANCHES; k++) if (r.etoiles >= cfg.tranche[k].min && r.etoiles <= cfg.tranche[k].max) break; }
            else k = r.gain == Gain::Coloris ? CADEAU_TRANCHES : r.gain == Gain::Skin ? CADEAU_TRANCHES + 1 : CADEAU_TRANCHES + 2;
            if (k < CADEAU_TRANCHES + 3) n[k]++;
        }
        const char* noms[] = {"10-15 ★", "16-25 ★", "26-40 ★", "41-50 ★", "coloris", "skin", "voiture prêtée"};
        const double attendu[] = {cfg.tranche[0].chance, cfg.tranche[1].chance, cfg.tranche[2].chance, cfg.tranche[3].chance, cfg.chanceColoris, cfg.chanceSkin, cfg.chanceVoiture};
        std::printf("  100 000 tirages (graine 500, garantie coupée) :\n  | gain | mesuré | attendu | écart |\n  |---|---|---|---|\n");
        bool ok = true; long long somme = 0;
        for (int k = 0; k < CADEAU_TRANCHES + 3; k++) {
            const double mesure = 100.0 * (double)n[k] / N, ecart = mesure - attendu[k];
            std::printf("  | %s | %.2f %% | %.2f %% | %+.2f pt |\n", noms[k], mesure, attendu[k], ecart);
            ok = ok && std::fabs(ecart) <= 0.5;
            somme += n[k];
        }
        cas("(a) chaque chance à ±0,5 point sur 100 000 tirages", ok && somme == N);
    }

    // ---- (b) jamais moins de 10 ★ sur 100 000 tirages (pleins, vides, avec garantie), même si une tranche descend sous le minimum
    {
        ConfigJeu::Cadeau bas = cfg; bas.tranche[0].min = 1;   // tranche réglée à 1-15 : le minimum de 10 ★ l'emporte
        const Entree pleine = entreePleine(), vide;
        Mulberry32 h(7);
        int mini = 1 << 30, sansRare = 0, sous = 0, etoiles = 0;
        for (int i = 0; i < 100000; i++) {
            const Resultat r = tirer(i % 2 ? cfg : bas, i % 3 ? vide : pleine, sansRare, h);
            if (r.gain == Gain::Etoiles) { etoiles++; mini = std::min(mini, r.etoiles); if (r.etoiles < 10) sous++; }
        }
        std::printf("  (b) %d cadeaux en étoiles, minimum %d ★\n", etoiles, mini);
        cas("(b) aucun gain < 10 ★ sur 100 000 tirages (replis et tranche 1-15 compris)", sous == 0 && mini >= 10);
        int maxi = 0; Mulberry32 g(9);
        for (int i = 0; i < 20000; i++) { int z = 0; const Resultat r = tirer(cfg, vide, z, g); maxi = std::max(maxi, r.etoiles); }
        cas("étoiles jamais au-dessus de 50 ★ (tranches du jeu)", maxi <= 50 && maxi >= 41);
    }

    // ---- (c) garantie : 29 cadeaux sans voiture ni skin forcés, le 30e en donne une ; compteur remis à 0
    {
        Entree sansRare = entreePleine();   // coloris seulement : aucune skin, aucune voiture à gagner
        for (auto& v : sansRare.voitures) v.skins.clear();
        sansRare.voituresPretables.clear();
        bool ok = true; int skins = 0, voitures = 0;
        for (uint32_t graine = 1; graine <= 2000 && ok; graine++) {
            Mulberry32 h(graine);
            int compteur = 0;
            for (int i = 0; i < 29; i++) {
                const Resultat r = tirer(cfg, sansRare, compteur, h);
                ok = ok && r.gain != Gain::Skin && r.gain != Gain::Voiture && !r.garantie;
            }
            ok = ok && compteur == 29;
            const Resultat r30 = tirer(cfg, entreePleine(), compteur, h);
            ok = ok && r30.garantie && (r30.gain == Gain::Skin || r30.gain == Gain::Voiture) && compteur == 0;
            (r30.gain == Gain::Skin ? skins : voitures)++;
        }
        std::printf("  (c) 2 000 garanties : %d skins, %d voitures (poids 4 / 3 : %.1f %% de skins)\n", skins, voitures, 100.0 * skins / 2000);
        cas("(c) 29 sans voiture ni skin → le 30e donne une voiture ou une skin, compteur à 0", ok);
        cas("(c) garantie : skin et voiture tirées avec les poids 4 et 3 (±3 points)", std::fabs(100.0 * skins / 2000 - 400.0 / 7) <= 3);
        Mulberry32 h(3);
        Entree seulVoiture; seulVoiture.voituresPretables = {"m4"};
        int c29 = 29; const Resultat v = tirer(cfg, seulVoiture, c29, h);
        cas("(c) une seule catégorie possible (voiture) : celle-là", v.garantie && v.gain == Gain::Voiture && v.voiture == "m4" && c29 == 0);
        Entree seulSkin; seulSkin.voitures = {{"gt86", {}, {"camouflage"}}};
        c29 = 29; const Resultat s = tirer(cfg, seulSkin, c29, h);
        cas("(c) une seule catégorie possible (skin) : celle-là", s.garantie && s.gain == Gain::Skin && s.voiture == "gt86" && s.objet == "camouflage" && c29 == 0);
        c29 = 29; const Resultat z = tirer(cfg, Entree{}, c29, h);
        cas("(c) aucune possible : étoiles (≥ 10) et compteur remis à 0", z.garantie && z.gain == Gain::Etoiles && z.etoiles >= 10 && c29 == 0);
        int c = 0; Mulberry32 g(11); int vus = 0;
        for (int i = 0; i < 200; i++) { const Resultat r = tirer(cfg, entreePleine(), c, g); if (r.gain == Gain::Skin || r.gain == Gain::Voiture) { vus++; if (c != 0) ok = false; } }
        cas("une voiture ou une skin gagnée remet le compteur à 0", ok && vus > 0);
    }

    // ---- (d) catégories vides (tout possédé) → étoiles ≥ 10, tranches renormalisées
    {
        Mulberry32 h(42);
        int replis = 0, autres = 0, mini = 1 << 30; long long tr[CADEAU_TRANCHES] = {}, nRepli = 0;
        for (int i = 0; i < 100000; i++) {
            int z = 0;
            const Resultat r = tirer(cfg, Entree{}, z, h);
            if (r.gain != Gain::Etoiles) autres++;
            mini = std::min(mini, r.etoiles);
            if (r.repli) { replis++; nRepli++; for (int k = 0; k < CADEAU_TRANCHES; k++) if (r.etoiles >= cfg.tranche[k].min && r.etoiles <= cfg.tranche[k].max) tr[k]++; }
        }
        cas("(d) catégories vides : toujours des étoiles ≥ 10", autres == 0 && mini >= 10);
        cas("(d) replis ≈ 15 % des tirages (coloris + skin + voiture)", std::fabs(100.0 * replis / 100000 - 15) <= 0.5);
        const double t45 = 100.0 * tr[0] / (double)nRepli;   // 45 / 85 = 52,9 %
        cas("(d) repli : tranches renormalisées 45 / 28 / 9 / 3 (10-15 ★ ≈ 52,9 %, ±1,5 pt)", std::fabs(t45 - 4500.0 / 85) <= 1.5);
    }

    // ---- (e) un cadeau par jour : 23:59 puis 00:01 locaux → nouveau cadeau ; deux ouvertures le même jour → la 2e refusée
    {
        Sauvegarde s; s.defauts();
        const Entree e = entreePleine();
        cas("(e) partie neuve : cadeau disponible tout de suite", disponible(s.cadeau, local(2026, 11, 13, 10, 0, 0, 3600)));
        const auto r1 = ouvrir(s, local(2026, 11, 13, 23, 59, 0, 3600), e, 1);
        const auto r2 = ouvrir(s, local(2026, 11, 13, 23, 59, 40, 3600), e, 2);
        cas("(e) 23:59 ouvert, 2e ouverture le même jour refusée", r1.has_value() && !r2.has_value() && s.cadeau.total == 1 && s.cadeau.jour == "2026-11-13");
        cas("(e) état « déjà ouvert » à 23:59:40", etat(s.cadeau, local(2026, 11, 13, 23, 59, 40, 3600)) == Etat::DejaOuvert);
        const auto r3 = ouvrir(s, local(2026, 11, 14, 0, 1, 0, 3600), e, 3);
        cas("(e) 00:01 le lendemain : nouveau cadeau", r3.has_value() && s.cadeau.total == 2 && s.cadeau.jour == "2026-11-14");
        cas("(e) sauvegarde à jour avant le gain (dernier = gain rendu)", r3 && s.cadeau.dernier == r3->texte() && s.cadeau.heure == local(2026, 11, 14, 0, 1, 0, 3600).epoch);
        const Instant tard = local(2026, 11, 20, 12, 0, 0, 3600);
        cas("(e) une semaine sans jouer : un seul cadeau (pas d'accumulation)", ouvrir(s, tard, e, 4).has_value() && !ouvrir(s, {tard.epoch + 60, 3600}, e, 5).has_value());
        Sauvegarde a, b; a.defauts(); b.defauts();
        const auto x = ouvrir(a, local(2026, 11, 13, 9, 0, 0, 3600), e, 77), y = ouvrir(b, local(2026, 11, 13, 9, 0, 0, 3600), e, 77);
        cas("même graine, même cadeau (RG-10)", x && y && x->texte() == y->texte());
    }

    // ---- (f) heure reculée de 2 jours → aucun cadeau tant que l'heure du dernier n'est pas repassée
    {
        Sauvegarde s; s.defauts();
        const Entree e = entreePleine();
        const Instant t0 = local(2026, 11, 13, 18, 0, 0, 3600);
        ouvrir(s, t0, e, 10);
        const Instant recul = {t0.epoch - 2 * 86400, 3600};
        cas("(f) heure reculée de 2 jours : aucun cadeau", etat(s.cadeau, recul) == Etat::HeureReculee && !ouvrir(s, recul, e, 11).has_value());
        cas("(f) 1 s avant l'heure du dernier cadeau : toujours bloqué", etat(s.cadeau, {t0.epoch - 1, 3600}) == Etat::HeureReculee);
        cas("(f) heure repassée, même jour : déjà ouvert", etat(s.cadeau, {t0.epoch + 3600, 3600}) == Etat::DejaOuvert);
        cas("(f) le lendemain : de nouveau disponible", ouvrir(s, local(2026, 11, 14, 8, 0, 0, 3600), e, 12).has_value());
        // changement de fuseau : seul le jour local compte, l'heure reculée reste bloquante
        Sauvegarde f; f.defauts();
        const Instant paris = local(2026, 11, 13, 23, 30, 0, 3600);   // 22:30 UTC
        ouvrir(f, paris, e, 13);
        cas("(f) fuseau à l'est (Dubaï, 02:30 le 14 local) : nouveau jour local → disponible", disponible(f.cadeau, {paris.epoch + 3600, 4 * 3600}));
        cas("(f) fuseau à l'ouest (New York, 18:30 le 13 local) : même jour → refusé", etat(f.cadeau, {paris.epoch + 3600, -5 * 3600}) == Etat::DejaOuvert);
        cas("(f) fuseau changé mais heure reculée : bloqué", etat(f.cadeau, {paris.epoch - 60, 4 * 3600}) == Etat::HeureReculee);
    }

    // ---- (g) sauvegarde écrite puis relue (toutes les clés) ; ancienne sauvegarde sans clés cadeau-* → disponible
    {
        Sauvegarde s; s.defauts();
        s.cadeau.heure = 1794600000; s.cadeau.decalage = -18000; s.cadeau.jour = "2026-11-13"; s.cadeau.sansRare = 17;
        s.cadeau.total = 123; s.cadeau.dernier = "coloris;golf:rose"; s.cadeau.pretVoiture = "m4"; s.cadeau.pretRecu = 1794600000; s.cadeau.pretFin = 1794610800;
        const std::string t = s.texte();
        Sauvegarde r; const bool lu = r.lire(t);
        const EtatCadeau& c = r.cadeau;
        cas("(g) 9 clés cadeau-* / pret-* écrites", t.find("cadeau-heure=1794600000") != std::string::npos && t.find("cadeau-decalage=-18000") != std::string::npos &&
            t.find("cadeau-jour=2026-11-13") != std::string::npos && t.find("cadeau-sans-rare=17") != std::string::npos && t.find("cadeau-total=123") != std::string::npos &&
            t.find("cadeau-dernier=coloris;golf:rose") != std::string::npos && t.find("pret-voiture=m4") != std::string::npos &&
            t.find("pret-recu=1794600000") != std::string::npos && t.find("pret-fin=1794610800") != std::string::npos);
        cas("(g) toutes les clés relues à l'identique", lu && c.heure == 1794600000 && c.decalage == -18000 && c.jour == "2026-11-13" && c.sansRare == 17 &&
            c.total == 123 && c.dernier == "coloris;golf:rose" && c.pretVoiture == "m4" && c.pretRecu == 1794600000 && c.pretFin == 1794610800);
        cas("(g) relu → texte identique", r.texte() == t);
        Sauvegarde neuve; neuve.defauts();
        cas("(g) aucun cadeau ouvert : aucune clé cadeau-* écrite", neuve.texte().find("cadeau-") == std::string::npos && neuve.texte().find("pret-") == std::string::npos);
        Sauvegarde ancienne;
        const bool ok = ancienne.lire("version=1\nwaforspeed-best=48210\ncar=m4\npaint-m4=rouge\netoiles=300\nperso=1\n");
        cas("(g) ancienne sauvegarde sans clés cadeau-* : lue, cadeau disponible", ok && ancienne.car == "m4" && ancienne.etoiles == 300 && ancienne.cadeau.total == 0 &&
            disponible(ancienne.cadeau, local(2026, 11, 13, 12, 0, 0, 3600)));
        Sauvegarde mal;
        mal.lire("version=1\ncadeau-heure=abc\ncadeau-decalage=999999\ncadeau-jour=2026-13-45\ncadeau-sans-rare=-3\ncadeau-dernier=<script>\n"
                 "pret-voiture=dauphine\npret-fin=x\ncadeau-futur=1\ncar=m4\n");
        cas("(g) valeurs mal écrites ignorées une à une (défauts), sans erreur", mal.cadeau.heure == 0 && mal.cadeau.decalage == 0 && mal.cadeau.jour.empty() &&
            mal.cadeau.sansRare == 0 && mal.cadeau.dernier.empty() && mal.cadeau.pretVoiture.empty() && mal.cadeau.pretFin == 0 && mal.car == "m4");
    }

    // ---- (h) configgame.json : total des chances ≠ 100 refusé ; tranche min > max refusée ; réglage juste accepté
    {
        auto lire = [](const std::string& json, ConfigJeu& c) { c = ConfigJeu{}; return lireConfig(json, "essai", c); };
        ConfigJeu c;
        BilanConfig b = lire("{\"cadeau\": {\"coloris\": {\"chance\": 9}}}", c);   // total 101 %
        bool message = false; for (const std::string& m : b.messages) if (m.find("total des chances") != std::string::npos) message = true;
        cas("(h) total 101 % : refusé, chances du jeu gardées, message au journal", c.cadeau.chanceColoris == 8 && message && b.erreurs >= 1);
        b = lire("{\"cadeau\": {\"coloris\": {\"chance\": 9}, \"tranches\": {\"t1\": {\"chance\": 44}}}}", c);   // total 100 %
        cas("(h) chances changées avec un total de 100 % : acceptées", c.cadeau.chanceColoris == 9 && c.cadeau.tranche[0].chance == 44);
        b = lire("{\"cadeau\": {\"tranches\": {\"t2\": {\"min\": 30, \"max\": 20}}}}", c);
        cas("(h) tranche min > max : tranche du jeu gardée", c.cadeau.tranche[1].min == 16 && c.cadeau.tranche[1].max == 25);
        b = lire("{\"cadeau\": {\"garantie\": 1, \"etoiles_min\": 0}}", c);
        cas("(h) garantie < 2 et minimum < 1 : hors bornes, valeurs du jeu", c.cadeau.garantie == 30 && c.cadeau.etoilesMin == 10);
        double total = cfg.chanceColoris + cfg.chanceSkin + cfg.chanceVoiture; for (const auto& t : cfg.tranche) total += t.chance;
        cas("valeurs du jeu : total des chances = 100 %, minimum 10 ★, garantie 30, prêt 3 h, activation 24 h",
            total == 100 && cfg.etoilesMin == 10 && cfg.garantie == 30 && cfg.pretHeures == 3 && cfg.activationHeures == 24);
    }

    // ---- résultat écrit / relu (RG-08) et entrée faite depuis la sauvegarde (RG-03)
    {
        Resultat r;
        bool ok = true;
        for (const char* t : {"etoiles;23", "coloris;golf:rose", "skin;gt86:rallye", "voiture;m4"}) ok = ok && lireResultat(t, r) && r.texte() == t;
        cas("résultat « type;valeur » écrit puis relu (4 types)", ok && !lireResultat("etoiles;", r) && !lireResultat("skin;golf", r) && !lireResultat("argent;5", r));
        Sauvegarde s; s.defauts();
        Entree e = entreeDepuis(s);
        bool gratuites = e.voitures.size() == 3, retirees = true, possedees = true;
        for (const auto& v : e.voitures) gratuites = gratuites && boutique::voitureGratuite(v.id) && !v.coloris.empty();
        for (const std::string& id : e.voituresPretables) { if (id == "x5" || id == "a45" || id == "elantra") retirees = false; if (boutique::possedeVoiture(s, id)) possedees = false; }
        cas("entrée : les 3 voitures gratuites du joueur, avec des coloris à gagner", gratuites);
        cas("entrée : 14 voitures à prêter, jamais une retirée (licence NC) ni une possédée", e.voituresPretables.size() == 14 && retirees && possedees);
        bool sansGratuit = true;
        for (const auto& v : e.voitures) for (const std::string& o : v.coloris) if (o == "blanc" || o == "brillant") sansGratuit = false;
        cas("entrée : les coloris gratuits (déjà possédés) n'y sont pas", sansGratuit);
        s.cadeau.pretVoiture = "m4";
        e = entreeDepuis(s);
        bool m4 = false; for (const std::string& id : e.voituresPretables) if (id == "m4") m4 = true;
        cas("entrée : la voiture déjà prêtée n'est pas reprêtée", !m4 && e.voituresPretables.size() == 13);
    }

    std::printf("%s : %d échec(s)\n", g_echecs ? "ÉCHEC" : "OK", g_echecs);
    return g_echecs ? 1 : 0;
}
