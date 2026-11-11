// WaForSpeed — carte Grand Prix (circuit), ticket WFS-53 (ex-WFS-54), épic E2. Port du décor « circuit » de
// js/world/decor.js :
//   - zones Ligne droite des tribunes, Paddock, Virages rapides, Section boisée (ZONES.circuit, ZONE_LEN = 448) ;
//   - bas-côtés GROUND.circuit : vibreurs rouge / blanc (un segment sur deux), herbe ou bac à gravier, voie des stands ;
//   - murs de pneus, panneaux publicitaires (texte d'AD_TEXTS, WFS-93), tribunes à spectateurs, projecteurs, mur et
//     garages des stands, drapeaux, postes de commissaires, sapins et chênes des sections boisées ;
//   - chaussée : texture roadCircuit (asphalte clair, traces de gomme, lignes blanches de bord) ;
//   - ambiance envFor : jour brouillard 820 m ; nuit #0e1020 à 640 m.
// WFS-140 : décor posé à partir du tracé 2 (sections de WFS-136) :
//   - virages : vibreurs larges et dégagements en gravier aux épingles, chicanes et virages serrés, murs de pneus,
//     panneaux de freinage 150 / 100 / 50 avant l'épingle (RG-01) ;
//   - départ : grille peinte et portique de 5 feux rouges, allumés un par un au décompte, éteints au « GO » (RG-02) ;
//   - raidillon : tribune haute dans la pente, passerelle publicitaire au sommet (RG-03) ;
//   - paddock le long de la ligne droite : mur des stands, garages et camions des équipes (couleurs inventées), bars
//     d'accueil avec terrasse VIP sur le toit, tour de contrôle, hélicoptère posé (RG-04) ;
//   - public : spectateurs dans les tribunes (de près), drapeaux qui s'agitent, espace fans avec grande roue et stands,
//     camping avec tentes derrière un grillage (RG-05) ; quelques vols d'oiseaux au loin (RG-06) ;
//   - RG-C1 : chaque objet (décor des zones compris) est refusé s'il retombe à moins de RW + 1 m d'une chaussée (grille
//     des centres de la route : l'autre branche de l'épingle est à ≈ 40 m) ; les bandes de sol s'arrêtent avant ; rien de
//     haut du côté intérieur des virages serrés dans les 60 m qui les précèdent.
// WFS-373 (épic E15, ANO-220) : plus aucune forme nue : chaque objet est un modèle construit une fois au chargement
//   (decor_grandprix_modeles.*, version proche et allégée) et recopié à chaque pose ; projecteurs de piste propres au Grand
//   Prix (T_PROJ) ; herbe tondue en bandes et touffes semées au hasard loin de la piste (plus de rangées de touffes au bord).
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <unordered_map>
#include "../game/course_system.h"   // WFS-140 : décompte (feux de départ allumés un par un)
#include "../game/trace_cartes.h"    // WFS-140 : sections du tracé 2, état des feux de départ
#include "../render/eclairage.h"
#include "decor_carte.h"
#include "decor_grandprix_modeles.h"   // WFS-373 : objets modélisés
#include "enseignes.h"   // AtlasTextes (WFS-71) : texte des panneaux publicitaires (WFS-93)
#include "pietons.h"     // WFS-199 : spectateurs (personnages du jeu allégés, foule::silhouette)
#include "../audio/ambiance_lieux.h"   // WFS-261 (accroche) : tribunes déclarées au son des lieux
#include "../audio/parois_sons.h"      // WFS-275 (accroche) : murs des stands, garages, tribunes (écho)

namespace wfs {
namespace carte {
namespace {

enum { T_TIRES = CarteDecor::T_CARTE, T_ADBOARD, T_STAND, T_PITWALL, T_GARAGE, T_FLAG, T_MARSHAL,
       // WFS-140
       T_FREIN, T_PORTIQUE, T_GRILLE, T_PASSERELLE, T_CAMION, T_ACCUEIL, T_TOUR, T_HELICO, T_STANDFAN, T_ROUE, T_TENTE,
       T_GRILLAGE,
       T_PROJ, T_HERBE,   // WFS-373 : projecteur de piste modélisé, touffes d'herbe semées au hasard
       NB_SORTES };
// WFS-140 : textes de l'atlas en plus d'AD_TEXTS (panneaux de freinage, enseignes)
const char* const TEXTES[8] = {"150", "100", "50", "VIP", "BAR", "ESPACE FANS", "CAMPING", "PADDOCK"};
enum { TX_150, TX_100, TX_50, TX_VIP, TX_BAR, TX_FANS, TX_CAMPING, TX_PADDOCK };
// couleurs des maillots du public : sièges, drapeaux et couleurs des équipes de la palette commune
const int MAILLOTS[12] = {C_SEAT0, C_SEAT0 + 1, C_SEAT0 + 2, C_SEAT0 + 3, C_FLAG0, C_FLAG0 + 1, C_FLAG0 + 2, C_FLAG0 + 3,
                          C_FLAG0 + 4, C_FLAG0 + 5, C_GARAGE0 + 4, C_WOOD};
constexpr int64_t AVANT_VIRAGE = 50;   // 60 m (RG-C1)
constexpr int MAX_DRAPEAUX = 16;       // drapeaux animés dessinés au plus (un appel de dessin chacun)
// WFS-199 (RG-03) : tribune à l'échelle réelle : rang de 0,85 m, contremarche 0,42 m, muret avant de 0,9 m, sièges de 0,5 m
// (assise 0,45 m), allée d'escalier tous les 10 m ; RG-04 : spectateurs au plus par tribune (Haute / Basse), 400 / 150 en tout
constexpr float RANG = 0.85f, MARCHE = 0.42f, MURET = 0.9f, SIEGE = 0.5f, ALLEE = 1.2f, ENTRE_ALLEES = 10.f;
constexpr int SPECTATEURS_MAX[2][2] = {{250, 150}, {95, 55}};   // [Basse][tribune 0 départ, 1 épingle]
inline float hauteurRang(int k) { return MURET + k * MARCHE; }
// couleurs des spectateurs (cases de la palette commune : rien d'ajouté)
const int PEAUX[4] = {C_WALLS0 + 1, C_CHURCH + 10, C_WOOD, C_OAK + 3};
const int CHEVEUX[5] = {C_OAK, C_OAK + 2, C_CHURCH + 9, C_WOOD + 9, C_TIRE + 3};
const int BAS_[7] = {C_WOOD + 8, C_TIRE, C_WOOD + 6, C_WOOD + 7, C_VINE_A + 7, C_CHURCH + 5, C_VINE_A + 6};
const int SOULIERS[3] = {C_TIRE, C_WOOD + 2, C_CHURCH + 5};

// WFS-373 : cases de la palette propres au Grand Prix (après C_NB), couleurs sRGB des matériaux réels (albédos linéaires
// dans les plages de la charte DA-realisme.md § 3 : caoutchouc 0,01-0,02, béton 0,25-0,40, aluminium 0,40-0,50, enduit 0,5-0,6)
enum ColGrandPrix { C_SANGLE = C_NB, C_ALU, C_ALU_SOMBRE, C_BETON, C_BETON_NEZ, C_TOLE, C_BARDAGE, C_MENUIS, C_RIDEAU,
                    C_INTERIEUR, C_DALLE, C_ENDUIT, C_CABINE, C_CHASSIS, C_JANTE, C_PHARE, C_FEU_AR, C_BOIS, C_FUSELAGE, C_PALE, C_PLUME,
                    C_TONTE, C_FIN_GP };
static_assert(C_FIN_GP <= PAL_W, "palette trop petite");
constexpr uint32_t COULEURS_GP[(int)C_FIN_GP - (int)C_NB] = {
    0xc9a22a,   // sangles de polyester jaunes
    0xb3b7bb,   // aluminium brut
    0x2a2c30,   // aluminium laqué noir (caissons, cadres)
    0xa3a097,   // béton
    0xd8d4c8,   // nez de marche peints
    0xb9bcbf,   // bac acier de couverture
    0x5c6166,   // bardage gris anthracite
    0x34373b,   // menuiseries d'aluminium
    0xbfc2c4,   // rideau métallique galvanisé
    0x26272a,   // intérieur des garages (pénombre)
    0xd9d7d0,   // dalles et nez de dalle peints
    0xcfcbc2,   // enduit
    0xe9e9e7,   // caisse de la remorque
    0x1e1f22,   // châssis
    0xa7abb0,   // jantes d'aluminium
    0xf2f4f6,   // glaces des phares
    0xb3191c,   // feux arrière
    0x8b6a4c,   // platelage de bois
    0x2b3a55,   // peinture de l'hélicoptère
    0x3a3c40,   // pales et rotor
    0x2d2a28,   // plumage (oiseaux au loin)
    0x5a8a38,   // herbe tondue (bande claire)
};
grandprix::Couleurs couleursModeles() {
    grandprix::Couleurs c;
    c.pneu = C_TIRE; c.pneuRouge = C_TIRE_R; c.pneuBlanc = C_TIRE_W; c.sangle = C_SANGLE;
    c.alu = C_ALU; c.aluSombre = C_ALU_SOMBRE; c.beton = C_BETON; c.betonNez = C_BETON_NEZ; c.acier = C_STEEL; c.tole = C_TOLE; c.bardage = C_BARDAGE;
    c.verre = C_GLASS; c.menuiserie = C_MENUIS; c.rideau = C_RIDEAU; c.interieur = C_INTERIEUR; c.dalle = C_DALLE; c.enduit = C_ENDUIT;
    c.commissaire = C_MARSHAL; c.abri = C_MARSHAL_TOP; c.panneau = C_SIGNW; c.mat = C_FLAGPOLE;
    c.cabine = C_CABINE; c.chassis = C_CHASSIS; c.jante = C_JANTE; c.phare = C_PHARE; c.feuArriere = C_FEU_AR;
    c.toile = C_SIGNW; c.bois = C_BOIS; c.fuselage = C_FUSELAGE; c.pale = C_PALE; c.plume = C_PLUME; c.plumeSombre = C_PLUME;
    return c;
}
// WFS-373 : pose d'un modèle (repère local x, y, z → X, haut, Z), couleurs de la palette ; COUL_VAR → var
void poserModele(Geo& g, const grandprix::Maille& m, vec3 o, vec3 X, vec3 Z, int var = 0) {
    const vec3 Y{0, 1, 0};
    g.v.reserve(g.v.size() + m.v.size());
    const uint32_t k0 = (uint32_t)g.v.size();
    for (const marina::Sommet& v : m.v) {
        const vec3 q = o + X * v.p.x + Y * v.p.y + Z * v.p.z, n = X * v.n.x + Y * v.n.y + Z * v.n.z;
        const int c = v.couleur == grandprix::COUL_VAR ? var : v.couleur;
        g.v.push_back({q.x, q.y, q.z, n.x, n.y, n.z, (c + 0.5f) / PAL_W, 0.5f});
    }
    for (uint32_t i : m.i) g.i.push_back(k0 + i);
}
Mesh maillage(Renderer& r, const grandprix::Maille& m, int var = 0) { Geo g; poserModele(g, m, {0, 0, 0}, {1, 0, 0}, {0, 0, 1}, var); return r.makeMesh(g.v, g.i); }
inline uint32_t hacher(int64_t a, int b) { uint32_t h = (uint32_t)(a * 2654435761u) ^ (uint32_t)(b * 40503u + 0x9e37u); h ^= h >> 15; h *= 0x2c1b3c6du; h ^= h >> 12; return h; }

// AD_TEXTS de js/world/decor.js : textes ; fond dans la palette (C_AD0 + 2 · n) ; lettres : une part (un matériau de
// l'atlas) par couleur, mêmes valeurs que C_AD0 + 2 · n + 1
const char* const AD_TEXTS[4] = {"WAFORSPEED", "GRAND PRIX", "PNEUS 24/7", "TURBO"};
const unsigned LETTRES[3] = {0xffffff, 0xffd23a, 0x101014};
// WFS-94 (RG-07, ANO-51) : seuil alpha des lettres des panneaux du circuit abaissé (0,45 → 0,28) : de loin, les mipmaps
// moyennées gardent les mots entiers au lieu de couper les lettres ; les enseignes de la ville gardent 0,45
constexpr float SEUIL_PANNEAUX = 0.28f;
constexpr int PART_LETTRES[4] = {P_TEXTE0, P_TEXTE0 + 1, P_TEXTE0 + 2, P_TEXTE0};

// ligne de texte de l'atlas sur un plan vertical centré en c, lue selon x (horizontal, unitaire) ; lettres penchées
// d'un quart de leur hauteur (police « italic 700 » du jeu web : l'atlas n'a que Rajdhani Bold droite)
void ligne(Geo& g, vec3 c, vec3 x, float w, float h, const vie::Texte& T) {
    const vec3 up{0, 1, 0}, n = cross(x, up), hx = x * (w / 2), hy = up * (h / 2), sh = x * (0.125f * h);
    const vec3 P[4] = {c - hx - hy - sh, c + hx - hy - sh, c + hx + hy + sh, c - hx + hy + sh};
    const vec2 U[4] = {{T.u0, T.v1}, {T.u1, T.v1}, {T.u1, T.v0}, {T.u0, T.v0}};
    const uint32_t k = (uint32_t)g.v.size();
    for (int i = 0; i < 4; i++) g.v.push_back({P[i].x, P[i].y, P[i].z, n.x, n.y, n.z, U[i].x, U[i].y});
    g.i.insert(g.i.end(), {k, k + 1, k + 2, k, k + 2, k + 3});
}

class Circuit : public CarteDecor {
public:
    const char* name() const override { return "circuit"; }
    bool solNaturel(int col) const override { return col == C_TONTE || CarteDecor::solNaturel(col); }   // WFS-383 : herbe tondue en bandes
    void init(Renderer& r, const GameView& g) override {
        basse_ = r.qualiteBasse; pret_ = false; refuses_ = refusesSec_ = posesSec_ = tribunes_ = spectateurs_ = 0; drapeaux_.clear(); roues_.clear(); portique_ = -1; impair_ = 0;   // WFS-140
        parois::effacer("circuit");   // WFS-275 (accroche)
        for (int& v : poses_) v = 0;   // WFS-373
        grandprix::oublierObjets(); construireModeles();   // WFS-373 : une fois, avant la préparation du décor
        CarteDecor::init(r, g);
        if (active_) semerHerbe();   // WFS-373 (ANO-220) : touffes semées au hasard, loin de la piste
        for (auto& T : ad_) T = {};
        if (!active_) return;
        WFS_LOG("Circuit (WFS-140) : décor des zones : %d objet(s) refusé(s) (sur une autre chaussée ou cachant un virage)", refuses_ - refusesSec_);
        journalSortes();   // WFS-373 (RG-01)
        creerAnimes(r);   // WFS-140 : feux de départ, grande roue, drapeaux, oiseaux
        for (const char* t : AD_TEXTS) atlas_.ajouter(t);
        for (const char* t : TEXTES) atlas_.ajouter(t);
        if (!atlas_.construire(r)) return;   // police absente : panneaux sans texte
        for (int k = 0; k < 3; k++) mats_[P_TEXTE0 + k] = atlas_.materiau(r, LETTRES[k], 0.f, SEUIL_PANNEAUX);   // peinture : aucune émission
        // WFS-140 (RG-C3) : la nuit, les lettres jaunes (enseignes du paddock, panneaux) éclairent comme un néon
        if (night_) mats_[P_TEXTE0 + 1] = atlas_.materiau(r, LETTRES[1], 1.4f, SEUIL_PANNEAUX);
        for (int k = 0; k < 4; k++) ad_[k] = atlas_.texte(AD_TEXTS[k]);
        for (int k = 0; k < 8; k++) tx_[k] = atlas_.texte(TEXTES[k]);
        preparerDecor(r);   // WFS-268 (accroche) : décor fixe préparé ici, une fois les panneaux prêts
    }

protected:
    void shutdownCarte(Renderer& r) override {
        atlas_.detruire(r); for (auto& T : ad_) T = {}; for (auto& T : tx_) T = {};
        for (Mesh* m : {&feuMesh_, &roueMesh_, &roueNeon_, &oiseaux_}) if (m->vb.buf) { r.destroy(*m); *m = Mesh{}; }
        for (Mesh& m : drapeauMesh_) if (m.vb.buf) { r.destroy(m); m = Mesh{}; }
        for (Mesh& m : nacelles_) if (m.vb.buf) { r.destroy(m); m = Mesh{}; }   // WFS-373
        drapeaux_.clear(); roues_.clear(); portique_ = -1; pret_ = false;
        r.destroy(feuAllume_); r.destroy(feuEteint_);   // WFS-187 (accroche) : jeux de descripteurs rendus
    }
    const char* carteId() const override { return "circuit"; }
    // WFS-373 : couleurs propres au Grand Prix ; pas de rangées de touffes au bord de la piste (herbe tondue, semis : semerHerbe)
    void couleursCarte(std::array<uint32_t, PAL_W>& pal) const override { for (int k = C_NB; k < C_FIN_GP; k++) pal[(size_t)k] = COULEURS_GP[k - C_NB]; }
    bool touffes() const override { return false; }
    Env env() const override { return {0xc4d8e8, 820, 0x1f2440, 0x0e1020, 640}; }
    bool routePropre() const override { return true; }

    int profil(const char* z, int64_t a, int side, Band out[6]) const override {
        const float R = (float)RW;
        const int64_t n = mod(a);
        // WFS-140 (RG-01) : virage serré → vibreurs plus larges et dégagement en gravier du côté choisi
        const uint8_t c = coin_.empty() ? 0 : coin_[(size_t)n];
        const bool large = c & 1, gravierCoin = c & (side < 0 ? 2 : 4);
        const bool gravier = !std::strcmp(z, "gravier") || gravierCoin;
        const int64_t m = ((a % 2) + 2) % 2;
        const float kerb = large ? R + 2.2f : R + 1.3f;
        out[0] = {R, kerb, 0.04f, 0.04f, m ? C_RED : C_WHITE};   // vibreurs
        // WFS-373 (ANO-220) : herbe tondue en bandes de 6 m (passages de la tondeuse, en travers)
        const bool tonte = ((n / 5) % 2) != 0;
        out[1] = gravier ? Band{kerb, R + 15, 0.02f, 0, C_GRAVEL} : Band{kerb, R + 14, 0.02f, 0, tonte ? (int)C_TONTE : (int)C_GRASS};
        // le jeu web fait commencer cette bande à 14 m même après le gravier (15 m) : elle part ici du bord du gravier
        out[2] = {gravier ? R + 15 : R + 14, R + 100, 0, 0.5f, !std::strcmp(z, "paddock") ? C_PITLANE : tonte ? (int)C_TONTE : (int)C_GRASS2};
        // WFS-140 (RG-C1) : le sol s'arrête avant une autre chaussée (intérieur de l'épingle, branche voisine)
        const float lim = libre(n, side) - 0.5f;
        int k = 0;
        for (int i = 0; i < 3; i++) {
            if (out[i].a >= lim) break;
            Band b = out[i];
            if (b.b > lim) { b.yb = b.ya + (b.yb - b.ya) * (lim - b.a) / (b.b - b.a); b.b = lim; }
            out[k++] = b;
        }
        return k;
    }

    void place(const char* z, int64_t z0, int64_t z1, Mulberry32& Dm) override {
        if (!pret_) preparer();   // WFS-140 : tracé 2 lu une fois, avant le premier objet
        auto D = [&]() { return (float)Dm.next(); };
        auto fl = [](float v) { return (int64_t)std::floor(v); };
        const float R = (float)RW, SM = (float)SEG_M;
        auto tires = [&](int64_t n, int side, float lat) { Item it; it.t = T_TIRES; it.side = side; it.lat = lat; it.petit = true; poser(n, it); };
        auto lamp = [&](int64_t n, int side) {
            Item it; it.t = T_PROJ; it.side = side; it.k = 1; it.lat = R + 7;   // WFS-373 : projecteur modélisé (mât à RW + 7 m)
            // WFS-81 : éclairage de nuit sous le mât (scene.js : flood à RW + 4, 16 m, côté du mât) ; WFS-140 : seulement
            // si le mât est posé (pas sur une autre chaussée)
            if (poser(n, it)) lampadaires().push_back({((n % len()) + len()) % len(), side * (R + 4.f), 16.f, StyleLampadaire::Stade});
        };
        auto tree = [&](int64_t n, int side, float lat, float h, int kind, float along) {
            Item it; it.t = T_TREE; it.side = side; it.lat = lat; it.h = h; it.k = kind; it.along = along; poser(n, it);
        };
        if (!std::strcmp(z, "tribunes")) {
            for (int side : {-1, 1}) {
                for (int64_t n = z0; n < z1; n += 4) tires(n, side, R + 5.6f);
                for (int64_t n = z0; n < z1; n += 20) { Item it; it.t = T_ADBOARD; it.side = side; it.lat = R + 5.0f; it.col = (int)fl(D() * 4); poser(n, it); }
            }
            // WFS-199 (RG-01) : plus de tribune ici (2 au plus sur le tracé, posées par les sections) ; tirages gardés (RG-06)
            for (int64_t n = z0; n < z1 - 26; n += 26) for (int k = 0; k < 2; k++) if (D() < 0.9f) (void)D();
            for (int64_t n = z0 + 10; n < z1; n += 34) lamp(n, (n / 34) % 2 ? 1 : -1);
        } else if (!std::strcmp(z, "paddock")) {
            const int side = D() < 0.5f ? -1 : 1;
            for (int64_t n = z0; n < z1; n += 3) { Item it; it.t = T_PITWALL; it.side = side; it.lat = R + 1.4f; it.petit = true; poser(n, it); }
            for (int64_t n = z0 + 2; n < z1 - 14; n += 14) { Item it; it.t = T_GARAGE; it.side = side; it.lat = R + 16; it.l = 14 * SM - 0.5f; it.col = (int)fl(D() * 6); poser(n, it); }
            for (int64_t n = z0; n < z1; n += 4) tires(n, -side, R + 5.6f);
            for (int64_t n = z0; n < z1; n += 26) (void)D();   // WFS-199 (RG-01) : tribune retirée, tirage gardé
            for (int64_t n = z0 + 6; n < z1; n += 18) { Item it; it.t = T_FLAG; it.side = side; it.lat = R + 9; it.col = (int)fl(D() * 6); it.petit = true; poser(n, it); }
            for (int64_t n = z0 + 10; n < z1; n += 34) lamp(n, -side);
        } else if (!std::strcmp(z, "gravier")) {
            for (int side : {-1, 1}) {
                for (int64_t n = z0; n < z1; n += 4) tires(n, side, R + 16);
                for (int64_t n = z0 + 30; n < z1; n += 90) { Item it; it.t = T_MARSHAL; it.side = side; it.lat = R + 19; it.petit = true; poser(n, it); }
                for (int64_t n = z0 + fl(D() * 20); n < z1; n += 10 + fl(D() * 14)) {
                    const float lat = R + 40 + D() * 50, h = 8 + D() * 8; const int kind = D() < 0.5f ? K_PINE : K_ROUND; tree(n, side, lat, h, kind, 0);
                }
            }
            for (int64_t n = z0 + 10; n < z1; n += 40) lamp(n, (n / 40) % 2 ? 1 : -1);
        } else if (!std::strcmp(z, "foret")) {
            for (int side : {-1, 1}) {
                for (int64_t n = z0; n < z1; n += 4) tires(n, side, R + 5.6f);
                for (int64_t n = z0; n < z1; n += 2) {
                    const float lat = R + 10 + D() * 30, h = 12 + D() * 10; const int kind = D() < 0.6f ? K_PINE : K_ROUND; tree(n, side, lat, h, kind, (D() - 0.5f) * 2);
                }
            }
            for (int64_t n = z0 + 10; n < z1; n += 44) lamp(n, (n / 44) % 2 ? 1 : -1);
        }
    }

    // WFS-199 : places d'une tribune (rang k, position d le long de la tribune, occupée ?, tirage h) ; les allées restent libres ;
    // remplissage ≈ 82 % (RG-03), moins si le plafond de la tribune l'impose (RG-04 : 400 / 150 en tout ; en Basse, places
    // vides réparties sur tous les rangs), plafond jamais dépassé
    template <class F> void sieges(const Item& it, F f) const {
        const int plafond = SPECTATEURS_MAX[basse_ ? 1 : 0][it.col & 1];
        auto parcourir = [&](auto g) {
            for (int k = 0; k < it.k; k++) {
                int i = 0;
                for (float d = -it.l / 2 + 0.4f; d < it.l / 2 - 0.3f; d += SIEGE, i++) {
                    bool allee = false;
                    for (float x = -it.l / 2 + ENTRE_ALLEES; x < it.l / 2 - 3; x += ENTRE_ALLEES) allee = allee || std::fabs(d - x) < ALLEE * 0.5f + 0.2f;
                    if (allee) continue;
                    uint32_t h = (uint32_t)(i + 1) * 2654435761u ^ (uint32_t)(k + 7) * 40503u ^ (uint32_t)(it.col + 3) * 2246822519u;
                    h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
                    g(k, d, h);
                }
            }
        };
        int places = 0;
        parcourir([&](int, float, uint32_t) { places++; });
        const uint32_t taux = (uint32_t)std::min(82, places ? 100 * plafond / places : 0);
        int n = 0;
        parcourir([&](int k, float d, uint32_t h) {
            const bool occupe = n < plafond && (h >> 3) % 100 < taux;
            n += occupe;
            f(k, d, occupe, h);
        });
    }

    // ---------------------------------------------------------------- WFS-140 : place libre et sections du tracé 2
    int64_t mod(int64_t a) const { const int64_t L = len(); return ((a % L) + L) % L; }
    // distance latérale libre (m depuis l'axe) du côté side au segment n : au-delà, on retombe à moins de RW + 1 m d'une
    // autre chaussée (infini si rien jusqu'à RW + 100 m)
    float libre(int64_t n, int side) const {
        return libre_.empty() ? 1e9f : libre_[(size_t)mod(n) * 2 + (side > 0 ? 1 : 0)];
    }
    // extension extérieure d'un objet (m au-delà de son « lat »), pour le test de place libre
    static float etendue(const Item& it) {
        switch (it.t) {
        case T_TREE: return std::max(2.f, it.h * 0.35f);
        case T_LAMP: case T_PROJ: return 1.f;
        case T_STAND: return it.k * RANG + 2.f;
        case T_GARAGE: return 13.f;
        case T_ACCUEIL: return it.w + 1.f;
        case T_HELICO: return 14.f;
        case T_TOUR: return 7.f;
        case T_ROUE: return 4.f;
        case T_CAMION: return 3.f;
        case T_STANDFAN: return 4.f;
        case T_TENTE: return 3.f;
        default: return 2.f;
        }
    }
    static float longueur(const Item& it) {
        if (it.t == T_STAND || it.t == T_GARAGE || it.t == T_ACCUEIL) return it.l;
        if (it.t == T_PITWALL) return 3 * (float)SEG_M;
        if (it.t == T_ROUE) return 26.f;
        return 2.f;
    }
    // pose un objet si sa place est libre (RG-C1) ; réservation des côtés par le décor des sections ; Basse : un petit
    // objet sur deux (RG-C2)
    bool poser(int64_t n, const Item& it, bool section = false) {
        const int64_t m = mod(n);
        const uint8_t bit = it.side < 0 ? 1 : 2;
        if (!section && !reserve_.empty())   // WFS-373 : réservation vérifiée sur toute la longueur de l'objet
            for (int64_t k = n; k <= n + (int64_t)std::ceil(longueur(it) / SEG_M); k++) if (reserve_[(size_t)mod(k)] & bit) return false;
        refuses_++;   // compté comme refusé jusqu'à la pose
        if (section) refusesSec_++;
        const float lat = it.t == T_LAMP ? (float)RW + 7 : it.lat, ext = etendue(it), l = longueur(it);
        for (float d = 0; d <= l + 0.01f; d += std::max(1.2f, l / 4)) {
            const int64_t k = n + (int64_t)std::lround(d / SEG_M);
            if (lat + ext > libre(k, it.side)) return false;
        }
        // rien de haut du côté intérieur d'un virage serré dans les 60 m qui le précèdent
        if (!it.petit && lat < 60 && !cache_.empty() && (cache_[(size_t)m] & bit)) return false;
        if (basse_ && it.petit && section && (impair_++ & 1)) return false;
        item(n, it); compte(it.t); noter(n, it);
        // WFS-373 : la place d'un objet des sections est réservée sur toute sa longueur (le décor des zones ne s'y pose plus :
        // plus de garage du paddock autour d'un poste de commissaire)
        if (section && !reserve_.empty()) for (int64_t k = n - 1; k <= n + (int64_t)std::ceil(l / SEG_M) + 1; k++) reserve_[(size_t)mod(k)] |= bit;
        refuses_--; if (section) { refusesSec_--; posesSec_++; }
        if (it.t == T_PITWALL || it.t == T_GARAGE || it.t == T_STAND)   // WFS-275 (accroche) : murs qui renvoient le son (mur des stands vitré 2,7 m)
            parois::declarer("circuit", len(), it.t == T_PITWALL ? n : n + (int64_t)std::lround(it.l / SEG_M / 2), it.side, it.lat,
                             it.t == T_PITWALL ? 2.7f : it.t == T_GARAGE ? 7.f : hauteurRang(it.k - 1) + 2.f, it.t == T_PITWALL ? 3 * (float)SEG_M : it.l,
                             it.t == T_PITWALL ? 0.4f : it.t == T_GARAGE ? 12.f : it.k * RANG, it.t == T_GARAGE ? parois::ALPHA_DUR : parois::ALPHA_BETON);   // WFS-278 : obstacle, matériau
        return true;
    }

    // WFS-373 (RG-01) : objets posés par sorte (inventaire écrit au journal du banc)
    void compte(int t) { if (t >= 0 && t < NB_SORTES) poses_[t]++; }
    // banc des gros plans (WFS_BANC=grandprix-objets) : premier objet de chaque sorte posé sur le tour
    void noter(int64_t n, const Item& it) const {
        using namespace grandprix;
        static const int SORTE[NB_SORTES] = {-1, -1, -1, OB_PNEUS, OB_PUB, OB_TRIBUNE, OB_MUR_STANDS, OB_STAND, OB_DRAPEAU, OB_COMMISSAIRE, OB_FREIN,
                                             OB_PORTIQUE, -1, OB_PASSERELLE, OB_CAMION, OB_ACCUEIL, OB_TOUR, OB_HELICO, OB_FANS, OB_ROUE, OB_TENTE, OB_GRILLAGE, OB_PROJ, -1};
        if (it.t >= 0 && it.t < NB_SORTES && SORTE[it.t] >= 0) noterObjet(SORTE[it.t], mod(n), it.side, it.lat);
    }
    void journalSortes() const {
        static const char* const NOMS[NB_SORTES] = {"arbre", "touffe", "projecteur", "mur de pneus", "panneau publicitaire", "tribune", "mur des stands",
            "garage des stands", "drapeau", "poste de commissaire", "panneau de freinage", "portique de départ", "grille de départ", "passerelle",
            "camion", "bar d'accueil", "tour de contrôle", "hélicoptère", "stand de l'espace fans", "grande roue", "tente", "grillage", "projecteur", "touffes"};
        std::string t;
        for (int k = 0; k < NB_SORTES; k++) if (poses_[k]) t += std::string(t.empty() ? "" : ", ") + NOMS[k] + " " + std::to_string(poses_[k]);
        WFS_LOG("Circuit (WFS-373) : objets posés par sorte : %s", t.c_str());
    }

    void preparer() {
        pret_ = true;
        Track& t = *track_;
        const int64_t L = len();
        const auto& S = trace::sections(t);
        reserve_.assign((size_t)L, 0); cache_.assign((size_t)L, 0); coin_.assign((size_t)L, 0);
        // centres de la route sur un tour et demi (tour 1, avec la fin du tour 0 et le début du tour 2) dans une grille de 16 m
        const int64_t a0 = L - 450, a1 = 2 * L + 450;
        std::vector<vec3> c((size_t)(a1 - a0));
        for (int64_t a = a0; a < a1; a++) c[(size_t)(a - a0)] = poseAt(t, a).o;
        constexpr float CELL = 16;
        auto cle = [](int64_t i, int64_t j) { return (i << 32) ^ (j & 0xffffffff); };
        std::unordered_map<int64_t, std::vector<int32_t>> grille;
        for (size_t i = 0; i < c.size(); i++) grille[cle((int64_t)std::floor(c[i].x / CELL), (int64_t)std::floor(c[i].z / CELL))].push_back((int32_t)i);
        const float R = (float)RW, seuil = (R + 1) * (R + 1);
        auto surRoute = [&](vec3 q) {
            const int64_t ci = (int64_t)std::floor(q.x / CELL), cj = (int64_t)std::floor(q.z / CELL);
            for (int64_t di = -1; di <= 1; di++) for (int64_t dj = -1; dj <= 1; dj++) {
                auto f = grille.find(cle(ci + di, cj + dj));
                if (f == grille.end()) continue;
                for (int32_t i : f->second) { const float dx = q.x - c[(size_t)i].x, dz = q.z - c[(size_t)i].z; if (dx * dx + dz * dz < seuil) return true; }
            }
            return false;
        };
        libre_.assign((size_t)L * 2, 1e9f);
        for (int64_t n = 0; n < L; n++) {
            const Pose p = poseAt(t, n + L);
            for (int side : {-1, 1}) {
                for (float lat = R + 1.5f; lat <= R + 100; lat += 2) {
                    if (!surRoute(p.o + p.r * (side * lat))) continue;
                    libre_[(size_t)n * 2 + (side > 0 ? 1 : 0)] = lat - 2;
                    break;
                }
            }
        }
        int serres = 0;
        for (const trace::Section& s : S) {
            const bool chic = s.type == trace::Type::Chicane, ep = s.type == trace::Type::Epingle;
            const bool serre = chic || ep || (s.type == trace::Type::Virage && s.rayon <= 100);
            if (!serre) continue;
            serres++;
            const int interieur = s.angle > 0 ? 1 : -1;   // + : la route tourne à droite
            const uint8_t cacheBits = chic ? 3 : (interieur < 0 ? 1 : 2), gravier = chic ? 6 : (interieur < 0 ? 4 : 2);
            for (int64_t n = s.debut - AVANT_VIRAGE; n < s.fin; n++) cache_[(size_t)mod(n)] |= cacheBits;
            for (int64_t n = s.debut - 10; n < s.fin + 10; n++) coin_[(size_t)mod(n)] |= (uint8_t)(1 | gravier);
        }
        int nbLibres = 0; for (float v : libre_) nbLibres += v < 1e8f;
        WFS_LOG("Circuit (WFS-140) : %zu sections, %d virages serrés, %d bas-côtés bornés par une autre chaussée", S.size(), serres, nbLibres);
        if (!S.empty()) placerSections(S);
        WFS_LOG("Circuit (WFS-199) : %d tribune(s), %d spectateurs (personnages du jeu, aucune boîte)%s", tribunes_, spectateurs_, basse_ ? ", qualité Basse" : "");
        WFS_LOG("Circuit (WFS-140) : décor des sections : %d objets posés, %d refusés (place prise par une chaussée ou un virage)%s ; "
                "portique au segment %lld, %zu drapeaux, %zu grande(s) roue(s)", posesSec_, refusesSec_, basse_ ? ", qualité Basse" : "",
                (long long)portique_, drapeaux_.size(), roues_.size());
    }

    // décor des sections (RG-01 à RG-05)
    void placerSections(const std::vector<trace::Section>& S) {
        const float R = (float)RW, SM = (float)SEG_M;
        auto reserver = [&](int64_t n0, int64_t n1, uint8_t bits) { for (int64_t n = n0; n < n1; n++) reserve_[(size_t)mod(n)] |= bits; };
        auto objet = [&](int t, int side, float lat, bool petit = false) { Item it; it.t = t; it.side = side; it.lat = lat; it.petit = petit; return it; };
        auto lampe = [&](int64_t n, int side) {
            Item it = objet(T_PROJ, side, R + 7); it.k = 1;   // WFS-373
            if (poser(n, it, true)) lampadaires().push_back({mod(n), side * (R + 4.f), 16.f, StyleLampadaire::Stade});
        };
        auto pneus = [&](int64_t n0, int64_t n1, int side, float lat) { for (int64_t n = n0; n < n1; n += 4) poser(n, objet(T_TIRES, side, lat, true), true); };
        // WFS-199 (RG-01) : 2 tribunes au plus : id 0 face à la grille de départ, 1 à l'extérieur de l'épingle
        std::vector<lieux::Lieu> tribunesSon;   // WFS-261 (accroche) : centre de chaque tribune posée (foule du son des lieux)
        auto tribune = [&](int64_t n, int side, float lat, int rangs, int segs, int id) {
            Item it = objet(T_STAND, side, lat); it.l = segs * SM - 1; it.k = rangs; it.col = id;
            if (!poser(n, it, true)) return;
            tribunes_++;
            tribunesSon.push_back({mod(n + (int64_t)std::lround(it.l / SM / 2)), 0.f, side * (lat + (rangs - 1) * RANG * 0.5f), hauteurRang(rangs / 2) + 0.9f, 0});
            sieges(it, [&](int, float, bool occupe, uint32_t) { spectateurs_ += occupe; });
            for (float d = 4; d < it.l - 2; d += 9)   // drapeaux agités au dernier rang (animes)
                drapeaux_.push_back({mod(n + (int64_t)(d / SM)), side, lat + (rangs - 1) * RANG + 0.1f, hauteurRang(rangs - 1) + 1.2f, (int)(mod(n) + (int64_t)d) % 6});
        };
        const trace::Section *feux = nullptr, *epingle = nullptr, *montee = nullptr;
        std::vector<const trace::Section*> chicanes, serres;
        for (const trace::Section& s : S) {
            if (s.type == trace::Type::Feux && s.depart && !feux) feux = &s;
            if (s.type == trace::Type::Epingle && !epingle) epingle = &s;
            if (s.type == trace::Type::Montee && !montee) montee = &s;
            if (s.type == trace::Type::Chicane) chicanes.push_back(&s);
            if (s.type == trace::Type::Virage && s.rayon <= 100) serres.push_back(&s);
        }
        auto suivante = [&](const trace::Section* s) -> const trace::Section* {
            for (size_t i = 0; i + 1 < S.size(); i++) if (&S[i] == s) return &S[i + 1];
            return nullptr;
        };
        // ---------- RG-02 : départ (grille peinte, portique des 5 feux) ; RG-04 : paddock le long de la ligne droite
        if (feux) {
            const int64_t d = feux->debut, fin = std::min<int64_t>(d + 620, feux->fin + 600);
            reserver(d - 8, fin, 3);
            for (int k = 0; k < 10; k++) { Item it = objet(T_GRILLE, k % 2 ? 1 : -1, R * 0.5f); item(d + 4 + k * 7, it); compte(T_GRILLE); }
            portique_ = mod(d + 30);
            { Item it = objet(T_PORTIQUE, 1, 0); item(d + 30, it); compte(T_PORTIQUE); noter(d + 30, it); }   // 36 m devant la ligne : visible depuis la grille
            const int pad = -1;   // paddock à gauche, tribunes à droite
            for (int64_t n = d; n < fin; n += 3) poser(n, objet(T_PITWALL, pad, R + 1.4f, true), true);
            { Item it = objet(T_TOUR, pad, R + 12); poser(d + 2, it, true); }
            for (int64_t n = d + 24; n < fin - 14; n += 14) {
                Item g = objet(T_GARAGE, pad, R + 16); g.l = 14 * SM - 0.5f; g.col = (int)((n / 14) % 6); poser(n, g, true);
                if ((n / 14) % 2 == 0) { Item c = objet(T_CAMION, pad, R + 32); c.col = g.col; poser(n + 4, c, true); }
            }
            for (int64_t n = d + 40; n < fin - 40; n += 70) {
                Item b = objet(T_ACCUEIL, pad, R + 40); b.l = 30; b.w = 14; b.col = (int)((n / 70) % 2); poser(n, b, true);
            }
            { Item h = objet(T_HELICO, pad, R + 58); poser(fin - 30, h, true); }
            for (int64_t n = d + 10; n < fin; n += 34) lampe(n, pad);
            tribune(d + 23, -pad, R + 14, 6, 26, 0);   // WFS-199 : grande tribune centrée sur la grille (d + 4 … d + 67)
            for (int64_t n = d; n < fin; n += 20) { Item a = objet(T_ADBOARD, -pad, R + 5); a.col = (int)((n / 20) % 4); poser(n, a, true); }
            pneus(d, fin, -pad, R + 5.6f);
        }
        // ---------- RG-01 : épingle (panneaux de freinage, mur de pneus, tribune extérieure), chicanes, virages serrés
        if (epingle) {
            const int ext = epingle->angle > 0 ? -1 : 1;
            const int64_t d = epingle->debut;
            const int tx[3] = {TX_150, TX_100, TX_50};
            for (int k = 0; k < 3; k++) {
                const int64_t n = d - (int64_t)std::lround((150 - 50 * k) / SM);
                Item b = objet(T_FREIN, ext, R + 3, true); b.col = tx[k]; poser(n, b, true);
                reserver(n - 2, n + 3, ext < 0 ? 1 : 2);
            }
            reserver(d - 20, epingle->fin + 20, ext < 0 ? 1 : 2);
            pneus(d - 20, epingle->fin + 20, ext, R + 16);
            tribune((d + epingle->fin) / 2 - 10, ext, R + 22, 4, 20, 1);   // WFS-199 : une tribune, au sommet de l'épingle
            { Item m = objet(T_MARSHAL, ext, R + 19, true); poser(d - 25, m, true); }
            lampe(d - 30, ext);
        }
        for (const trace::Section* s : chicanes) {
            reserver(s->debut - 6, s->fin + 6, 3);
            for (int side : {-1, 1}) pneus(s->debut - 6, s->fin + 6, side, R + 13.5f);
            Item m = objet(T_MARSHAL, -s->cote, R + 18, true); poser(s->debut + 4, m, true);
        }
        for (const trace::Section* s : serres) {
            const int ext = s->angle > 0 ? -1 : 1;
            reserver(s->debut - 10, s->fin + 10, ext < 0 ? 1 : 2);
            pneus(s->debut - 10, s->fin + 10, ext, R + 16);
        }
        // ---------- RG-03 : raidillon (tribune haute dans la pente, passerelle publicitaire au sommet)
        if (montee) {
            const int ext = montee->angle > 0 ? -1 : 1;
            reserver(montee->debut, montee->fin + 12, ext < 0 ? 1 : 2);
            // WFS-199 (RG-01) : plus de tribune dans la pente : panneaux publicitaires à la place
            for (int64_t n = montee->debut + 20; n < montee->fin - 10; n += 20) { Item a = objet(T_ADBOARD, ext, R + 5); a.col = (int)((n / 20) % 4); poser(n, a, true); }
            pneus(montee->debut, montee->fin, ext, R + 5.6f);
            Item p = objet(T_PASSERELLE, 1, 0); p.col = 0; item(montee->fin, p); compte(T_PASSERELLE); noter(montee->fin, p);
            for (int64_t n = montee->debut + 10; n < montee->fin; n += 40) lampe(n, -ext);
        }
        // ---------- RG-05 : espace fans (grande roue, stands) après la 1re chicane ; camping après le virage suivant
        if (!chicanes.empty()) if (const trace::Section* f = suivante(chicanes[0]); f && f->type == trace::Type::Ligne) {
            const int side = 1;
            const int64_t n0 = f->debut + 30, n1 = f->fin - 40;
            reserver(n0 - 4, n1 + 4, side < 0 ? 1 : 2);
            for (int64_t n = n0; n < n1; n += 3) poser(n, objet(T_GRILLAGE, side, R + 14, true), true);
            for (int64_t n = n0 + 4; n < n1; n += 12) { Item b = objet(T_STANDFAN, side, R + 20, true); b.col = (int)((n / 12) % 6); poser(n, b, true); }
            const int64_t mr = (n0 + n1) / 2;
            Item w = objet(T_ROUE, side, R + 42);
            if (poser(mr, w, true)) roues_.push_back({mod(mr), side, R + 42});
            { Item e = objet(T_STANDFAN, side, R + 20); e.col = -1; poser(n0, e, true); }   // portail « ESPACE FANS »
            for (int64_t n = n0; n < n1; n += 34) lampe(n, side);
            if (const trace::Section* v = suivante(f); v) if (const trace::Section* c = suivante(v); c && c->type == trace::Type::Ligne) {
                const int64_t c0 = c->debut + 20, c1 = c->fin - 30;
                reserver(c0 - 4, c1 + 4, side < 0 ? 1 : 2);
                for (int64_t n = c0; n < c1; n += 3) poser(n, objet(T_GRILLAGE, side, R + 16, true), true);
                { Item e = objet(T_STANDFAN, side, R + 20); e.col = -2; poser(c0, e, true); }   // entrée « CAMPING »
                for (int64_t n = c0 + 6; n < c1; n += 7)
                    for (int r = 0; r < 4; r++) { Item te = objet(T_TENTE, side, R + 24 + r * 7.5f, true); te.col = (int)((n + r * 5) % 6); te.along = (r % 2) * 2.f; poser(n, te, true); }
            }
        }
        lieux::poser("circuit", lieux::TRIBUNE, std::move(tribunesSon));   // WFS-261 (accroche)
    }

    // ---------------------------------------------------------------- WFS-140 : objets animés (animes)
    void creerAnimes(Renderer& r) {
        // WFS-373 : feux (lentilles bombées), grande roue en treillis (nacelles à part, toujours d'aplomb), drapeaux tenus à la
        // main (hampe et toile ondulée), vol d'oiseaux modélisés
        const grandprix::Couleurs c = couleursModeles();
        feuMesh_ = maillage(r, grandprix::lentilleFeu(), C_CHECK_B);
        { grandprix::Maille neon; const grandprix::Maille m = grandprix::roue(c, neon); roueMesh_ = maillage(r, m); roueNeon_ = maillage(r, neon); }
        for (int k = 0; k < 6; k++) nacelles_[k] = maillage(r, grandprix::nacelle(c, C_FLAG0 + k));
        for (int k = 0; k < 6; k++) drapeauMesh_[k] = maillage(r, mod_.drapeauMain, C_FLAG0 + k);
        oiseaux_ = maillage(r, grandprix::vol(7, C_PLUME));
        Material on; on.rough = 0.3f; on.flags = MAT_DOUBLE; on.emissive = linHex(0xff1a1a) * 4.f;
        feuAllume_ = r.makeMaterial(on, nullptr, nullptr, nullptr);
        Material off; off.color = {0.05f, 0.02f, 0.02f, 1}; off.rough = 0.4f; off.flags = MAT_DOUBLE;
        feuEteint_ = r.makeMaterial(off, nullptr, nullptr, nullptr);
    }

    // RG-02 : feux de départ allumés un par un pendant le décompte (un toutes les 0,6 s), éteints au « GO »
    int feuxAllumes() const {
        const trace::Section* f = nullptr;
        for (const trace::Section& s : trace::sections(*track_)) if (s.type == trace::Type::Feux && s.depart) { f = &s; break; }
        if (!f || trace::feuA(f->feu) != trace::Feu::Rouge) return 0;
        const Course* c = activeCourse();
        if (!c || c->state != RaceState::Count) return 5;
        return std::clamp((int)std::floor((c->clock + 3) / 0.6) + 1, 1, 5);
    }

    int animes(Renderer& r, const GameView& g, int64_t c0, int64_t c1) override {
        if (!feuMesh_.vb.buf) return 0;
        Track& t = *g.track;
        int n = 0;
        int64_t a;
        if (portique_ >= 0 && dansMorceau(portique_, c0 * CH, (c1 + 1) * CH, a)) {   // 5 feux sous la poutre du portique
            const Pose p = poseAt(t, a);
            const int on = feuxAllumes();
            for (int k = 0; k < 5; k++) {
                const vec3 c = lateral(p, (k - 2) * 1.0f, 7.25f) - p.f * 0.075f;   // face au pilote (lentille posée sur le caisson)
                r.draw(feuMesh_, k < on ? feuAllume_ : feuEteint_, translate(c) * rotateY(PI - p.th));   // −z local → −f (vers la grille)
                n++;
            }
        }
        for (const Roue& w : roues_) {
            if (!dansMorceau(w.seg, c0 * CH, (c1 + 1) * CH, a)) continue;
            const Pose p = poseAt(t, a + 11);
            const mat4 B = translate(lateral(p, w.side * w.lat, 14.5f)) * rotateY(-p.th), Rx = rotateX(0.12f * (float)g.time), M = B * Rx;
            r.draw(roueMesh_, mats_[P_UNI], M); r.draw(roueNeon_, mats_[P_NEON], M); n += 2;
            for (int k = 0; k < 12; k++) {   // WFS-373 : nacelles pendues à la jante, toujours d'aplomb
                const float a = 2 * PI * k / 12;
                const vec3 e = Rx.transformPoint({0, 12.f * std::cos(a), 12.f * std::sin(a)});
                r.draw(nacelles_[k % 6], mats_[P_UNI], B * translate(e)); n++;
            }
        }
        // drapeaux les plus proches du joueur : balancement de ±35° autour de la hampe
        const int64_t pj = (int64_t)std::floor(g.player->dist / SEG);
        int nd = 0;
        for (size_t i = 0; i < drapeaux_.size() && nd < MAX_DRAPEAUX; i++) {
            const Drapeau& d = drapeaux_[i];
            if (!dansMorceau(d.seg, pj - 20, pj + 160, a)) continue;
            const Pose p = poseAt(t, a);
            const float ph = (float)g.time * 2.6f + (float)i * 1.7f;
            r.draw(drapeauMesh_[d.col], mats_[P_UNI], translate(lateral(p, d.side * d.lat, d.y)) * rotateY(-p.th + d.side * (0.6f * std::sin(ph) + 0.5f)));
            n++; nd++;
        }
        // RG-06 : trois vols d'oiseaux au loin, au-dessus du terrain, qui tournent lentement
        for (int k = 0; k < 3; k++) {
            const Pose p = poseAt(t, pj + 180 + k * 70);
            const float tt = (float)g.time * (0.12f + 0.03f * k) + k * 2.1f;
            const vec3 c = lateral(p, (k % 2 ? 1.f : -1.f) * (70 + 25 * k) + 30 * std::cos(tt), 38 + 6 * k) + p.f * (30 * std::sin(tt));
            r.draw(oiseaux_, mats_[P_UNI], translate(c) * rotateY(-tt - p.th)); n++;
        }
        return n;
    }

    // ---------------------------------------------------------------- WFS-256 (accroche, autorisée par le PO : ajout seul)
    // formes de choc des objets du Grand Prix, relevées sur dessine() ci-dessous (box : largeur selon r, longueur selon f)
    int solide(const Item& it, chocs::ObstacleRoute o[4]) const override {
        if (const int n = CarteDecor::solide(it, o)) return n;   // arbres, projecteurs
        using G = chocs::Genre; const float s = (float)it.side, R = (float)RW, SG = (float)SEG_M;
        int n = 0;
        auto bt = [&](float lat, float hx, float hz, G g, float al = 0) { chocs::ObstacleRoute& q = o[n++]; q = {}; q.forme = chocs::Forme::Boite; q.lat = lat; q.hx = hx; q.hz = hz; q.genre = g; q.along = al; };
        auto cy = [&](float lat, float r, G g, float al = 0) { chocs::ObstacleRoute& q = o[n++]; q = {}; q.lat = lat; q.r = r; q.genre = g; q.along = al; };
        switch (it.t) {
        case T_TIRES: for (int i = 0; i < 4; i++) cy(s * it.lat, 0.6f, G::Mur, i * 1.2f); break;   // 4 piles de pneus
        case T_ADBOARD: bt(s * (it.lat + 0.11f), 0.12f, 5.75f, G::Mur, 5 * SG); break;          // cadre de 11,5 m (pose a + 5)
        case T_PITWALL: bt(s * it.lat, 0.2f, 1.5f * SG, G::Mur); break;
        case T_MARSHAL: bt(s * it.lat, 0.8f, 0.8f, G::Batiment); break;
        case T_FREIN: for (float al : {-0.6f, 0.6f}) bt(s * it.lat, 0.04f, 0.04f, G::Poteau, al); break;
        case T_PORTIQUE: for (float sd : {-1.f, 1.f}) bt(sd * (R + 1.6f), 0.25f, 0.25f, G::Poteau); break;
        case T_PASSERELLE: for (float sd : {-1.f, 1.f}) bt(sd * (R + 2.5f), 0.5f, 0.5f, G::Poteau); break;
        case T_FLAG: cy(s * it.lat, 0.08f, G::Poteau); break;
        case T_PROJ: cy(s * (R + 7.f), 0.25f, G::Poteau); break;   // WFS-373 : mât du projecteur (comme le projecteur commun)
        case T_GRILLAGE: bt(s * it.lat, 0.03f, 1.8f, G::Mur, 1.8f); break;
        default: break;   // tribunes, garages, stands, camions : au-delà du bord de la carte ; grille de départ : peinte au sol
        }
        return n;
    }

    void dessine(Track& t, const Item& it, int64_t a, const Pose& p, bool lod, Geo out[P_COUNT]) const override {
        Geo& S = out[P_UNI];
        const float s = (float)it.side;
        const vec3 X = p.r * s;   // WFS-373 : x des modèles = vers l'extérieur (dos à la route)
        const int l = lod ? 1 : 0;
        switch (it.t) {
        case T_TIRES:   // WFS-373 : mur de pneus sanglé (7 piles de 4 pneus, pneu du haut peint), allégé en Basse
            poserModele(S, mod_.pneus[(int)((a + it.side + 2) & 1)][lod || basse_ ? 1 : 0], lateral(p, s * it.lat), X, p.f);
            break;
        case T_ADBOARD: {   // WFS-373 : bâche aux couleurs d'AD_TEXTS sur cadre d'aluminium, texte des deux côtés
            const Pose pm = poseAt(t, a + 5);
            poserModele(S, mod_.pub[l], lateral(pm, s * it.lat), pm.r * s, pm.f, C_AD0 + it.col * 2);
            const vie::Texte& T = ad_[it.col];
            if (!T.ok) break;
            // canevas du jeu web (1024 × 112, police de 80 px) : texte écrit deux fois, centré à ±2,75 m, un peu sous
            // le milieu du panneau ; lettres d'environ 0,6 m, plus petites si le texte dépasse sa moitié de panneau
            float h = 1.14f, w = h * T.aspect;
            if (w > 5.2f) { h *= 5.2f / w; w = 5.2f; }
            for (int face : {1, -1}) {   // 1 : vers la route ; −1 : au dos, lu dans l'autre sens
                const vec3 x = pm.f * (-s * (float)face), c = lateral(pm, s * (face > 0 ? it.lat - 0.03f : it.lat + 0.09f), 0.71f);
                for (float k : {-2.75f, 2.75f}) ligne(out[PART_LETTRES[it.col]], c + pm.f * k, x, w, h, T);
            }
            break;
        }
        case T_STAND: {   // WFS-199 : tribune réelle et vrais spectateurs ; WFS-373 : gradins, sièges coques et charpente modélisés
            const Pose pm = poseAt(t, a + (int64_t)std::lround(it.l / SEG_M / 2));
            const vec3 base = lateral(pm, s * it.lat), out_ = pm.r * s, up{0, 1, 0};
            const grandprix::Couleurs cm = couleursModeles();
            poserModele(S, grandprix::tribune(cm, it.l, it.k, RANG, MARCHE, MURET, ENTRE_ALLEES, ALLEE, lod), base, out_, pm.f);
            if (lod) {   // de loin : rangée de sièges allégée par rang (les spectateurs restent des personnages, version allégée)
                const grandprix::Maille rg = grandprix::rangeeSieges(cm, it.l * 0.98f);
                for (int k = 0; k < it.k; k++) poserModele(S, rg, base + out_ * (k * RANG + 0.12f) + up * hauteurRang(k), out_, pm.f, C_SEAT0 + (it.col * 2 + k) % 4);
            }
            // sièges (coques) et spectateurs assis ou debout (RG-02)
            const Pose& P = pm;
            int kPrec = -1; float dPrec = -1e9f; bool adultePrec = false;   // WFS-240 (RG-02) : place précédente du rang
            sieges(it, [&](int k, float d, bool occupe, uint32_t h) {
                const vec3 q = base + out_ * (k * RANG + 0.12f) + P.f * d;   // place : arrière du rang, jambes vers la route
                const float y = hauteurRang(k);
                const foule::Silhouette* sil = nullptr;
                const bool debout = occupe && h % 7 == 0;   // un spectateur sur sept debout devant sa place
                const bool enfant = occupe && foule::enfantEnTribune(h, !debout, adultePrec && k == kPrec && d - dPrec < SIEGE * 1.5f);   // WFS-240
                kPrec = k; dPrec = d; adultePrec = occupe && !debout && !enfant;   // WFS-240 : un enfant toujours à côté d'un adulte assis
                if (occupe) sil = enfant ? foule::silhouetteEnfant(h, (foule::Pose)(foule::ASSIS_REPOS + (h >> 3) % 2), (int)(h >> 4) % foule::VARIANTES, !lod) : foule::silhouette(h & 1, (int)(h >> 1) % foule::COIFFURES,
                                                    (foule::Pose)(debout ? foule::DEBOUT_REPOS + (h >> 3) % 2 : foule::ASSIS_REPOS + (h >> 3) % 2),
                                                    (int)(h >> 4) % foule::VARIANTES, !lod);
                const float assise = sil && !debout ? sil->assise : 0.45f;
                if (!lod) poserModele(S, mod_.siege, q + up * (y + assise - grandprix::SIEGE_ASSISE), out_, P.f, C_SEAT0 + (it.col * 2 + k) % 4);
                if (!sil) return;
                // repère du spectateur : face à la route (+z local → −out_), petit écart d'orientation, taille 0,95 à 1,05
                const float yaw = ((float)((h >> 6) % 21) - 10.f) * 0.025f, ech = 0.95f + (float)((h >> 11) % 11) * 0.01f;
                const vec3 ez = normalize(-out_ * std::cos(yaw) + P.f * std::sin(yaw)), ex = cross(up, ez);
                const vec3 o = debout ? q - out_ * 0.45f + up * y : q + up * (y + assise);
                const int c[5] = {PEAUX[(h >> 13) % 4], CHEVEUX[(h >> 15) % 5], MAILLOTS[(h >> 18) % 12], BAS_[(h >> 22) % 7], SOULIERS[(h >> 25) % 3]};
                const uint32_t k0 = (uint32_t)S.v.size();
                for (const foule::Sommet& v : sil->v) {
                    const vec3 w = o + (ex * v.p.x + up * v.p.y + ez * v.p.z) * ech, n = ex * v.n.x + up * v.n.y + ez * v.n.z;
                    const vec2 t2 = uv(c[v.zone]);
                    S.v.push_back({w.x, w.y, w.z, n.x, n.y, n.z, t2.x, t2.y});
                }
                for (uint32_t i : sil->i) S.i.push_back(k0 + i);
            });
            break;
        }
        case T_PITWALL:   // WFS-373 : muret de béton préfabriqué et grillage de protection (élément centré sur le segment)
            poserModele(S, mod_.murStands[lod || basse_ ? 1 : 0], lateral(p, s * it.lat) - p.f * (1.5f * (float)SEG_M), X, p.f);
            break;
        case T_GARAGE: {   // WFS-373 (ANO-220) : stand vitré à rideaux métalliques et auvent, bandeau aux couleurs de l'équipe
            const Pose pm = poseAt(t, a + (int64_t)std::lround(it.l / SEG_M / 2));
            poserModele(S, mod_.stand[l], lateral(pm, s * it.lat), pm.r * s, pm.f, C_GARAGE0 + it.col);
            poserModele(out[P_NEON], mod_.standNeon[l], lateral(pm, s * it.lat), pm.r * s, pm.f);
            break;
        }
        case T_FLAG: poserModele(S, mod_.drapeau[l], lateral(p, s * it.lat), X, p.f, C_FLAG0 + it.col); break;
        case T_MARSHAL: poserModele(S, mod_.poste[l], lateral(p, s * it.lat), X, p.f); break;
        // ---------------------------------------------------------------- WFS-140
        case T_FREIN: {   // panneau de freinage : 2 poteaux, panneau blanc 1,6 × 1,2 m encadré, chiffres noirs face à la route
            poserModele(S, mod_.frein, lateral(p, s * it.lat), X, p.f);
            const vie::Texte& T = tx_[it.col];
            if (T.ok) {
                const float h = 0.85f, w = std::min(1.4f, h * T.aspect);
                ligne(out[P_TEXTE0 + 2], lateral(p, s * (it.lat - 0.06f), 1.6f), p.f * -s, w, w / T.aspect, T);
            }
            break;
        }
        case T_GRILLE: {   // emplacement de la grille : équerre blanche peinte (barre de 2,4 m et retour de 1,2 m)
            const vec3 c = lateral(p, s * it.lat, 0.022f);
            const vec3 Xg = p.r * 1.2f, Fw = p.f * 0.15f;
            quadSol(S, c - Xg - Fw, c + Xg - Fw, c + Xg + Fw, c - Xg + Fw, uv(C_SIGNW));
            const vec3 bord = c + p.r * (-s * 1.2f);
            quadSol(S, bord - p.r * 0.15f, bord + p.r * 0.15f, bord + p.r * 0.15f - p.f * 1.2f, bord - p.r * 0.15f - p.f * 1.2f, uv(C_SIGNW));
            break;
        }
        case T_PORTIQUE: poserModele(S, mod_.portique, lateral(p, 0), p.r, p.f); break;   // WFS-373 : treillis et caisson des feux (feux : animes())
        case T_PASSERELLE: {   // passerelle publicitaire au sommet du raidillon (WFS-373 : piles, tablier, garde-corps modélisés), panneaux des deux côtés
            const float R = (float)RW;
            poserModele(S, mod_.passerelle, lateral(p, 0), p.r, p.f);
            const vie::Texte& T = ad_[0];
            for (int face : {1, -1}) {   // face à la montée (vers −f) et au dos
                const vec3 c = lateral(p, 0, 5.4f) + p.f * (face * -1.25f);
                quad(S, c - p.r * (R + 3) - vec3{0, 0.75f, 0}, c + p.r * (R + 3) - vec3{0, 0.75f, 0}, c + p.r * (R + 3) + vec3{0, 0.75f, 0}, c - p.r * (R + 3) + vec3{0, 0.75f, 0}, uv(C_AD0));
                if (T.ok) for (float k : {-5.f, 5.f}) ligne(out[P_TEXTE0], c + p.f * (face * -0.03f) + p.r * k, p.r * (float)face, std::min(8.f, 1.2f * T.aspect), std::min(8.f, 1.2f * T.aspect) / T.aspect, T);
            }
            break;
        }
        case T_CAMION: poserModele(S, mod_.camion[l], lateral(p, s * it.lat), p.r, p.f, C_GARAGE0 + it.col); break;   // WFS-373 : semi-remorque modélisé
        case T_ACCUEIL: {   // bar d'accueil : 2 étages vitrés, terrasse VIP sur le toit (garde-corps vitré, parasols), enseigne
            const Pose pm = poseAt(t, a + (int64_t)std::lround(it.l / SEG_M / 2));
            const int v = it.col ? 1 : 0;
            poserModele(S, mod_.accueil[v][l], lateral(pm, s * it.lat), pm.r * s, pm.f);
            poserModele(out[P_NEON], mod_.accueilNeon[v][l], lateral(pm, s * it.lat), pm.r * s, pm.f);
            const vie::Texte& T = tx_[it.col ? TX_BAR : TX_VIP];
            if (T.ok) ligne(out[P_TEXTE0 + 1], lateral(pm, s * (it.lat - 0.12f), 9.4f), pm.f * -s, std::min(6.f, 0.9f * T.aspect), std::min(6.f, 0.9f * T.aspect) / T.aspect, T);
            break;
        }
        case T_TOUR: {   // tour de contrôle : fût, vigie vitrée évasée (sous-face éclairée la nuit), toit et mât d'antennes
            const vec3 b = lateral(p, s * (it.lat + 3));
            poserModele(S, mod_.tour, b, X, p.f); poserModele(out[P_NEON], mod_.tourNeon, b, X, p.f);
            const vie::Texte& T = tx_[TX_PADDOCK];
            if (T.ok) ligne(out[P_TEXTE0], b + p.r * (-s * 1.78f) + vec3{0, 13.5f, 0}, p.f * -s, std::min(3.2f, 0.6f * T.aspect), std::min(3.2f, 0.6f * T.aspect) / T.aspect, T);
            break;
        }
        case T_HELICO: {   // hélicoptère posé sur son aire (H peint) ; WFS-373 : hélicoptère modélisé
            const vec3 c = lateral(p, s * (it.lat + 7), 0.03f);
            quadSol(S, c - p.r * 7 - p.f * 7, c + p.r * 7 - p.f * 7, c + p.r * 7 + p.f * 7, c - p.r * 7 + p.f * 7, uv(C_ROOF2));
            for (float x : {-2.f, 2.f}) quadSol(S, c + p.r * (x - 0.4f) - p.f * 3 + vec3{0, 0.01f, 0}, c + p.r * (x + 0.4f) - p.f * 3 + vec3{0, 0.01f, 0}, c + p.r * (x + 0.4f) + p.f * 3 + vec3{0, 0.01f, 0}, c + p.r * (x - 0.4f) + p.f * 3 + vec3{0, 0.01f, 0}, uv(C_SIGNW));
            quadSol(S, c - p.r * 1.6f - p.f * 0.4f + vec3{0, 0.02f, 0}, c + p.r * 1.6f - p.f * 0.4f + vec3{0, 0.02f, 0}, c + p.r * 1.6f + p.f * 0.4f + vec3{0, 0.02f, 0}, c - p.r * 1.6f + p.f * 0.4f + vec3{0, 0.02f, 0}, uv(C_SIGNW));
            poserModele(S, mod_.helico[l], c + vec3{0, 0.03f, 0} + p.f * 1.5f, p.r, p.f);
            break;
        }
        case T_STANDFAN: {   // stand de l'espace fans (comptoir, toit de toile, réglette) ; col < 0 : portail avec enseigne
            const vec3 b = lateral(p, s * (it.lat + 2));
            if (it.col < 0) {
                poserModele(S, mod_.portail, b, X, p.f); poserModele(out[P_NEON], mod_.portailNeon, b, X, p.f);
                const vie::Texte& T = tx_[it.col == -1 ? TX_FANS : TX_CAMPING];
                if (T.ok) ligne(out[P_TEXTE0 + 1], b + p.r * (-s * 0.18f) + vec3{0, 4.8f, 0}, p.f * -s, std::min(6.2f, 0.8f * T.aspect), std::min(6.2f, 0.8f * T.aspect) / T.aspect, T);
                break;
            }
            poserModele(S, mod_.fans, b, X, p.f, C_FLAG0 + it.col); poserModele(out[P_NEON], mod_.fansNeon, b, X, p.f);
            break;
        }
        case T_ROUE: {   // grande roue : pieds en A en tubes, paliers, socle et escalier (la roue tourne : animes)
            const Pose pm = poseAt(t, a + 11);
            poserModele(S, mod_.pieds, lateral(pm, s * it.lat), pm.r, pm.f);
            break;
        }
        case T_TENTE: poserModele(S, mod_.tente[(it.col + (int)a) % 3], lateral(p, s * it.lat) + p.f * it.along, X, p.f, C_FLAG0 + it.col); break;   // tente dôme
        case T_GRILLAGE: poserModele(S, mod_.grillage[lod || basse_ ? 1 : 0], lateral(p, s * it.lat), X, p.f); break;   // poteau tous les 3,6 m, maille losangée
        case T_PROJ: {   // WFS-373 : projecteur de piste (mât conique, rampe de 6 projecteurs dont le verre s'éclaire la nuit)
            const vec3 b = lateral(p, s * it.lat);
            poserModele(S, mod_.proj[l], b, X, p.f); poserModele(out[P_NEON], mod_.projNeon[l], b, X, p.f);
            break;
        }
        case T_HERBE: {   // WFS-373 (ANO-220) : touffe semée au hasard (3 à 5 brins de hauteur et d'orientation variées)
            const uint32_t h = hacher(a, it.k);
            const vec3 c0 = lateral(p, s * it.lat, solY(zone(a), a, it.side, it.lat) - 0.04f) + p.f * it.along;
            const int n = 3 + (int)(h % 3);
            for (int k = 0; k < n; k++) {
                const uint32_t g = hacher(a * 7 + k, it.k + k);
                const vec3 d{((float)(g % 100) / 100.f - 0.5f) * 0.9f, 0, ((float)((g >> 8) % 100) / 100.f - 0.5f) * 0.9f};
                veg::touffe(out[P_FEUILLU], c0 + d, 0.22f + (float)((g >> 16) % 100) / 100.f * 0.3f, (float)((g >> 4) % 628) / 100.f, 0.86f + (float)((g >> 20) % 16) / 100.f);
            }
            break;
        }
        default: break;
        }
    }

private:
    vie::AtlasTextes atlas_;
    vie::Texte ad_[4];   // place de chaque texte d'AD_TEXTS dans l'atlas (lue par les threads des morceaux)
    // WFS-140
    vie::Texte tx_[8];   // TEXTES
    bool pret_ = false, basse_ = false;
    int refuses_ = 0, refusesSec_ = 0, posesSec_ = 0, tribunes_ = 0, spectateurs_ = 0;   // WFS-199 : tribunes et spectateurs posés   // journal : objets refusés (décor des zones et des sections)
    unsigned impair_ = 0;
    int poses_[NB_SORTES] = {};   // WFS-373 : objets posés par sorte
    std::vector<float> libre_;                 // place libre de chaque côté de chaque segment (2 par segment)
    std::vector<uint8_t> reserve_, cache_, coin_;
    int64_t portique_ = -1;                    // segment du portique des feux de départ
    struct Drapeau { int64_t seg; int side; float lat, y; int col; };
    struct Roue { int64_t seg; int side; float lat; };
    std::vector<Drapeau> drapeaux_;
    std::vector<Roue> roues_;
    Mesh feuMesh_{}, roueMesh_{}, roueNeon_{}, oiseaux_{}, drapeauMesh_[6]{}, nacelles_[6]{};
    // WFS-373 : modèles du Grand Prix, construits une fois (version proche [0] et allégée [1])
    struct Modeles {
        grandprix::Maille pneus[2][2], pub[2], siege, murStands[2], stand[2], drapeau[2], drapeauMain, poste[2], frein, portique, passerelle,
                          camion[2], standNeon[2], accueil[2][2], accueilNeon[2][2], tour, tourNeon, helico[2], fans, fansNeon, portail, portailNeon, pieds, tente[3],
                          grillage[2], proj[2], projNeon[2];
        bool prets = false;
    } mod_;
    void construireModeles() {
        if (mod_.prets) return;
        using namespace grandprix;
        const Couleurs c = couleursModeles();
        const float R = (float)RW;
        for (int d = 0; d < 2; d++) for (int l = 0; l < 2; l++) mod_.pneus[d][l] = murPneus(c, l, d);
        for (int l = 0; l < 2; l++) {
            mod_.pub[l] = panneauPub(c, l); mod_.murStands[l] = murStands(c, 3 * (float)SEG_M, l); mod_.stand[l] = stand(c, 14 * (float)SEG_M - 0.5f, mod_.standNeon[l], l);
            mod_.drapeau[l] = drapeau(c, l); mod_.poste[l] = posteCommissaire(c, l); mod_.camion[l] = camion(c, l); mod_.helico[l] = helico(c, l);
            mod_.grillage[l] = grillage(c, l); mod_.proj[l] = projecteur(c, mod_.projNeon[l], l);
            for (int v = 0; v < 2; v++) mod_.accueil[v][l] = accueil(c, 14, 30, C_FLAG0, v, mod_.accueilNeon[v][l], l);
        }
        mod_.siege = siege(c); mod_.drapeauMain = drapeauMain(c); mod_.frein = panneauFrein(c);
        mod_.portique = portique(c, R + 1.6f); mod_.passerelle = passerelle(c, R + 2.5f); mod_.tour = tour(c, mod_.tourNeon, false);
        mod_.fans = standFans(c, mod_.fansNeon); mod_.portail = portail(c, mod_.portailNeon); mod_.pieds = piedsRoue(c);
        for (int v = 0; v < 3; v++) mod_.tente[v] = tente(c, v);
        mod_.prets = true;
    }
    // WFS-373 (ANO-220) : touffes d'herbe semées au hasard (graine du segment, aucun tirage du décor) : une touffe sur trois
    // segments environ, de 6 à 45 m du bord, jamais sur une autre chaussée, ni dans le gravier, le paddock ou une place réservée
    void semerHerbe() {
        const float R = (float)RW;
        for (int64_t a = 0; a < len(); a++) {
            const char* z = zone(a);
            if (!std::strcmp(z, "paddock")) continue;
            for (int side : {-1, 1}) {
                const uint32_t h = hacher(a, side + 11);
                if (h % 100 >= 18) continue;
                const float lat = R + 6 + (float)((h >> 8) % 1000) / 1000.f * 39.f;
                if ((coin_.size() && (coin_[(size_t)mod(a)] & (side < 0 ? 2 : 4)) && lat < R + 16) || lat + 1.5f > libre(a, side)) continue;
                if (!reserve_.empty() && (reserve_[(size_t)mod(a)] & (side < 0 ? 1 : 2)) && lat < R + 60) continue;
                Item it; it.t = T_HERBE; it.side = side; it.lat = lat; it.along = (float)((h >> 18) % 100) / 100.f * (float)SEG_M; it.k = (int)(h >> 26); it.petit = true;
                item(a, it); compte(T_HERBE);
            }
        }
    }
    Material feuAllume_{}, feuEteint_{};
};

} // namespace
} // namespace carte

WFS_SYSTEM(carte::Circuit, 108)   // décor (100-199) : après la campagne (107)

} // namespace wfs
