// WFS-550 (interface R19 3/5) : calculs des jauges communes de src/ui/jauges.h : fractions bornées (cadrans, anneaux, barres),
// angles du cadran de 240° ouvert en bas, place des 17 segments de la position (segment affiché allongé, rangée centrée, le
// nombre suit le catalogue), étapes du chargement (Moteur, Modèles 3D, Voitures, Ville) alignées sur les textes annoncés.
#include <cmath>
#include <cstdio>
#include <limits>
#include "../src/ui/jauges.h"
using namespace wfs::jauges;

static int echecs = 0;
static void cas(const char* quoi, bool ok) { std::printf("jauges : %-90s → %s\n", quoi, ok ? "OK" : "ÉCHEC"); if (!ok) echecs++; }
static bool proche(float a, float b, float e = 1e-5f) { return std::fabs(a - b) < e; }

int main() {
    // fractions
    cas("fraction : milieu, bornes, hors bornes (jamais < 0 ni > 1)", proche(fraction(5, 0, 10), 0.5f) && fraction(-3, 0, 10) == 0 && fraction(40, 0, 10) == 1);
    cas("fraction : bornes inversées ou égales → 0 ; NaN → 0", fraction(5, 10, 0) == 0 && fraction(5, 3, 3) == 0 && fraction(std::numeric_limits<float>::quiet_NaN(), 0, 1) == 0);
    // cadran de 240° ouvert en bas (angles de dessin2d : 0 en haut, sens horaire)
    cas("cadran : 240° de -120° à +120°, vide au début, plein à la fin, milieu en haut",
        proche(CADRAN_FIN - CADRAN_DEBUT, PI_J * 4 / 3) && proche(angleCadran(0), CADRAN_DEBUT) && proche(angleCadran(1), CADRAN_FIN) && proche(angleCadran(0.5f), 0));
    cas("cadran : fraction hors bornes ramenée dans l'arc", proche(angleCadran(-1), CADRAN_DEBUT) && proche(angleCadran(3), CADRAN_FIN));
    // anneaux du temps
    cas("anneau du cadeau : vide quand tout reste à attendre, plein à l'heure, moitié à 12 h sur 24 h",
        fractionAttente(86400, 86400) == 0 && fractionAttente(0, 86400) == 1 && proche(fractionAttente(43200, 86400), 0.5f));
    cas("anneau du prêt : plein au début (3 h), vide à la fin, jamais négatif", fractionRestant(10800, 10800) == 1 && fractionRestant(0, 10800) == 0 && fractionRestant(-5, 10800) == 0);
    cas("anneaux : durée totale nulle sans division par zéro", fractionAttente(5, 0) == 1 && fractionRestant(5, 0) == 0);
    // segments de la position (RG-04)
    {
        Segment e[17];
        for (int i = 0; i < 17; i++) e[i] = i < 3 ? Segment::Possedee : Segment::Bloquee;
        e[16] = Segment::Affichee;   // maquette 15 : 17e voiture affichée, 3 possédées
        const float l = 16, ec = 4;
        const float total = largeurSegments(e, 17, l, ec);
        cas("17 segments : 16 normaux + 1 allongé (×2,5) + 16 écarts", proche(total, 16 * l + 2.5f * l + 16 * ec));
        const PlaceSegment p0 = placeSegment(e, 17, 0, l, ec), p16 = placeSegment(e, 17, 16, l, ec);
        cas("1er segment au bord gauche, dernier (affiché) allongé et collé au bord droit", proche(p0.x, 0) && proche(p16.l, 2.5f * l) && proche(p16.x + p16.l, total));
        bool croissant = true;
        for (int i = 1; i < 17; i++) { const PlaceSegment a = placeSegment(e, 17, i - 1, l, ec), b = placeSegment(e, 17, i, l, ec); croissant = croissant && proche(b.x, a.x + a.l + ec); }
        cas("segments jointifs à écart constant, sans chevauchement (zones d'appui distinctes)", croissant);
        Segment e5[5] = {Segment::Possedee, Segment::Affichee, Segment::Bloquee, Segment::Bloquee, Segment::Bloquee};
        cas("le nombre de segments suit le catalogue (5 voitures : 4 normaux + 1 allongé)", proche(largeurSegments(e5, 5, l, ec), 4 * l + 2.5f * l + 4 * ec));
        cas("aucune voiture : rangée vide", largeurSegments(e, 0, l, ec) == 0);
    }
    // étapes du chargement (RG-02) : à chaque palier annoncé, le segment en cours est celui du texte affiché (revue de la PR #424)
    {
        // étape en cours = 1re étape pas finie (comme chargement.cpp)
        auto enCours = [](float p) { int e = 0; while (e < 3 && avancementEtape(p, e) >= 1) e++; return e; };
        struct Palier { float p; const char* texte; int segment; };
        const Palier paliers[] = {
            {0.00f, "0 % « Préparation du moteur » → Moteur", 0},
            {0.10f, "10 % « Construction de la route » → Moteur", 0},
            {0.20f, "20 % « Chargement du modèle 3D… » → Modèles 3D", 1},
            {0.70f, "70 % « Chargement des voitures » → Voitures", 2},
            {0.85f, "85 % « Construction de la ville » → Ville", 3},
        };
        for (const Palier& q : paliers) {
            bool ok = enCours(q.p) == q.segment;
            for (int e = 0; e < q.segment; e++) ok = ok && avancementEtape(q.p, e) == 1;    // celles d'avant : finies
            for (int e = q.segment; e < 4; e++) ok = ok && avancementEtape(q.p, e) < 1;     // celle-ci et les suivantes : pas finies
            cas(q.texte, ok);
        }
    }
    cas("chargement à 100 % : les 4 étapes finies", avancementEtape(1, 0) == 1 && avancementEtape(1, 1) == 1 && avancementEtape(1, 2) == 1 && avancementEtape(1, 3) == 1);
    cas("étape hors de 0..3 : 0 (aucune lecture hors du tableau)", avancementEtape(0.5f, -1) == 0 && avancementEtape(0.5f, 4) == 0);
    std::printf("jauges : %d échec(s)\n", echecs);
    return echecs ? 1 : 0;
}
