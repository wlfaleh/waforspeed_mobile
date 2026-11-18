// WaForSpeed — portique du prochain checkpoint (WFS-71 ; WFS-562 : vrai modèle). Placé au prochain checkpoint de la course
// (lecture seule de activeCourse()->nextCp), reconstruit à chaque checkpoint passé. Ville et Corniche (RG-09 de WFS-71).
// WFS-562 (RG-05 bis, RG-05 ter ; ANO-279 partie checkpoint, ANO-315) : les poteaux carrés, la poutre en boîte, les barres néon
// cyan et la bande lumineuse au sol sont remplacés par un portique d'événementiel en treillis d'aluminium à deux poutres, une
// bâche imprimée « CHECKPOINT N » (noir sur orange, non lumineuse) et une ligne de chronométrage peinte en blanc
// (portique_checkpoint_modele.*). La nuit, deux projecteurs LED éclairent la bâche (lumière posée par PortiqueLumiere).
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include "../core/log.h"
#include "../core/system.h"
#include "../game/course_system.h"
#include "../game/player.h"
#include "../game/trace_cartes.h"   // WFS-139 (accroche)
#include "../game/track.h"
#include "enseignes.h"
#include "portique_checkpoint_modele.h"

namespace wfs {
namespace {
using namespace portique_checkpoint;

constexpr float DEGAGEMENT = 1.f;      // pieds à 1 m du bord de la chaussée (comme avant)
constexpr unsigned ENCRE = 0x141414;   // encre noire de l'impression (texte)

// bâche et projecteurs du portique dessiné, lus par la lumière de nuit (PortiqueLumiere) ; vide sans portique
struct BachePose { bool visible = false; vec3 centre{}, avant{}; };
BachePose g_bache;

class Portique : public System {
public:
    const char* name() const override { return "portique"; }
    void init(Renderer& r, const GameView& g) override {
        actif_ = false; cle_ = -1; g_bache = {};
        if (!g.map) return;
        if (std::strcmp(g.map->id, "ville") && std::strcmp(g.map->id, "plage")) return;   // RG-09
        atlas_.ajouter("CHECKPOINT");
        for (int d = 0; d <= 9; d++) atlas_.ajouter(std::to_string(d));
        if (!atlas_.construire(r)) return;
        // matériaux PBR (regles-de-gestion/DA-realisme.md, § 3) : aluminium brut, acier galvanisé terni, aluminium laqué noir et
        // sandows (diélectrique sombre), bâche PVC orange satinée, encres mates, laiton nickelé, vitre des projecteurs, peinture
        // routière thermoplastique (albédo ≈ 0,75, réfléchissante par ses billes de verre : un peu moins rugueuse)
        auto mat = [&](vec3 c, float metal, float rough) { Material m; m.color = {c.x, c.y, c.z, 1}; m.metal = metal; m.rough = rough; return r.makeMaterial(m, nullptr, nullptr, nullptr); };
        mats_[ALU] = mat({0.913f, 0.922f, 0.924f}, 1, 0.38f);
        mats_[ACIER] = mat({0.56f, 0.57f, 0.58f}, 1, 0.6f);
        mats_[NOIR] = mat({0.035f, 0.035f, 0.038f}, 0, 0.5f);
        mats_[BACHE] = mat(vie::lin(0xF06414), 0, 0.55f);
        mats_[ENCRE_NOIRE] = mat(vie::lin(ENCRE), 0, 0.7f);
        mats_[ENCRE_BLANCHE] = mat(vie::lin(0xE8E8E4), 0, 0.6f);
        mats_[LAITON] = mat({0.75f, 0.74f, 0.70f}, 1, 0.35f);
        mats_[VERRE] = mat({0.02f, 0.02f, 0.022f}, 0, 0.06f);
        mats_[PEINTURE] = mat({0.75f, 0.75f, 0.73f}, 0, 0.5f);
        mats_[ATTACHE] = Material{};   // tendeurs : matériau NOIR (aucun matériau créé)
        Material allume; allume.color = {0.9f, 0.9f, 0.88f, 1}; allume.rough = 0.06f; allume.emissive = vie::lin(0xFFF2E0) * 3.f;   // LED 5 000 K
        verreNuit_ = r.makeMaterial(allume, nullptr, nullptr, nullptr);
        actif_ = true;
    }
    void render(Renderer& r, const GameView& g) override {
        g_bache.visible = false;
        if (!actif_ || g.sceneDediee || !g.player || !g.track) return;
        const Course* c = activeCourse();
        // solo en course (décompte compris) : portique au prochain checkpoint ; sinon rien (menu, fin, scènes du banc)
        const bool visible = c && g.ecran == Ecran::Course && c->state != RaceState::Over && c->timeLeft < 1e6;
        if (!visible) return;
        const Player& pl = *g.player;
        const double s = pl.position + (c->nextCp - pl.dist);   // abscisse du checkpoint sur la piste (unités)
        const long long cle = (long long)c->level * 100000 + (long long)std::llround(c->nextCp);
        if (cle != cle_) { construire(r, *g.track, s, c->level + 1); cle_ = cle; }
        const mat4 I = mat4::identity();
        const bool details = length(centre_ - g.camPos) < DETAILS_M;   // œillets et tendeurs : de près seulement
        for (int p = 0; p < NB_PIECES; p++)
            if (pieces_[p].indexCount && (details || (p != LAITON && p != ATTACHE)))
                r.draw(pieces_[p], p == VERRE && g.night ? verreNuit_ : mats_[p == ATTACHE ? NOIR : p], I);
        if (texte_.indexCount) r.draw(texte_, atlas_.materiau(r, ENCRE, 0.f), I);
        g_bache = {bache_, centre_, avant_};
    }
    void shutdown(Renderer& r) override {
        if (!actif_) return;
        r.vk().waitIdle();
        liberer(r);
        atlas_.detruire(r);
        for (int k = 0; k < NB_PIECES; k++) if (k != ATTACHE) r.destroy(mats_[k]);
        r.destroy(verreNuit_);
        actif_ = false; g_bache = {};
    }
private:
    void liberer(Renderer& r) {
        for (Mesh& m : pieces_) if (m.indexCount) { r.destroy(m); m = Mesh{}; }
        if (texte_.indexCount) { r.destroy(texte_); texte_ = Mesh{}; }
    }
    void construire(Renderer& r, Track& t, double s, int numero) {
        r.vk().waitIdle();   // une fois par checkpoint : les anciens maillages ne sont plus utilisés par une image en vol
        liberer(r);
        const vie::Repere p = vie::repere(t, s);
        const float R = (float)RW;
        // WFS-139 (accroche) : tracé 2 : ni pieds ni poutres dans une courbe serrée (ils tomberaient sur la chaussée) ni dans
        // un tunnel (voûte de 7,6 m) ; la ligne peinte marque seule le checkpoint
        const trace::Section* sec = trace::sectionA(t, s);
        const bool ligneSeule = std::min({trace::rayonInterieur(t, s - 2500).m, trace::rayonInterieur(t, s).m, trace::rayonInterieur(t, s + 2500).m}) < 60
                                || (sec && sec->type == trace::Type::Tunnel);
        const Modele md = portique_checkpoint::construire(2 * (R + DEGAGEMENT), 2 * R, ligneSeule);
        // modèle local (x à droite, y en haut, z vers l'arrière) → monde, au repère de la route au checkpoint
        const vec3 X = p.r, Y{0, 1, 0}, Z = p.f * -1.f;
        auto monde = [&](vec3 q) { return p.o + X * q.x + Y * q.y + Z * q.z; };
        int tri = 0;
        for (int k = 0; k < NB_PIECES; k++) {
            const Maille& m = md.pieces[k];
            tri += m.triangles();
            if (m.v.empty()) continue;
            std::vector<Vertex> v; v.reserve(m.v.size());
            for (const marina::Sommet& so : m.v) {
                const vec3 q = monde(so.p), n = X * so.n.x + Y * so.n.y + Z * so.n.z;
                v.push_back({q.x, q.y, q.z, n.x, n.y, n.z, 0.5f, 0.5f});
            }
            pieces_[k] = r.makeMesh(v, m.i);
        }
        // impression « CHECKPOINT N » au milieu des deux bâches, entre les damiers (hauteur de ligne 1,2 m : capitales de ≈ 0,8 m), encre à 4 mm de la toile
        vie::Geo T;
        if (!ligneSeule) {
            const std::string num = std::to_string(numero);
            const float h = 1.2f, esp = 0.6f;
            float larg = atlas_.texte("CHECKPOINT").aspect * h + esp;
            for (char ch : num) larg += atlas_.texte(std::string(1, ch)).aspect * h;
            for (float face : {1.f, -1.f}) {
                const vec3 droite = X * face, n = Z * face;   // face avant (+z local) : lue de gauche à droite par le pilote
                const vec3 base = monde({0, md.bacheCentre.y, face * (md.bacheCentre.z + 0.004f)});
                float x = -larg / 2;
                auto mot = [&](const std::string& m) {
                    const vie::Texte& tx = atlas_.texte(m);
                    const float w = tx.aspect * h;
                    vie::plan(T, base + droite * (x + w / 2), droite, {0, 1, 0}, n, w, h, tx.u0, tx.v0, tx.u1, tx.v1);
                    x += w;
                };
                mot("CHECKPOINT"); x += esp;
                for (char ch : num) mot(std::string(1, ch));
            }
            texte_ = r.makeMesh(T.v, T.i);
        }
        bache_ = !ligneSeule; centre_ = monde(md.bacheCentre); avant_ = Z;
        WFS_LOG("portique de checkpoint (WFS-562) : « CHECKPOINT %d »%s, %d triangles, ouverture %.2f m, hauteur %.2f m", numero,
                ligneSeule ? " (ligne peinte seule : courbe serrée ou tunnel)" : "", tri, md.ouverture, md.hauteurTotale);
    }
    bool actif_ = false, bache_ = false;
    long long cle_ = -1;
    vie::AtlasTextes atlas_;
    Material mats_[NB_PIECES]{}, verreNuit_{};
    Mesh pieces_[NB_PIECES]{}, texte_{};
    vec3 centre_{}, avant_{};
};

// La nuit, les deux projecteurs éclairent la bâche (une lumière en cône, s'il en reste une). Ordre 960 : après la nuit (950),
// qui remet la liste des lumières à zéro (même règle que le portique de départ, WFS-559).
class PortiqueLumiere : public System {
public:
    const char* name() const override { return "portique : lumière"; }
    void ambiance(FrameParams& fp, const GameView& g) override {
        if (!g_bache.visible || !g.night || g.sceneDediee || fp.spotCount >= FrameParams::SPOTS_MAX) return;
        if (length(g_bache.centre - g.camPos) > 300) return;
        FrameParams::Spot& s = fp.spots[fp.spotCount++];
        // projecteur équivalent aux deux LED : 3,5 m devant la bâche et 2 m plus bas, cône assez large pour ses 16,8 m
        s.pos = g_bache.centre + g_bache.avant * 3.5f + vec3{0, -2.f, 0};
        s.target = g_bache.centre;
        s.angle = 1.3f; s.penumbra = 0.4f; s.decay = 2; s.distance = 14;
        s.color = vie::lin(0xFFF2E0);
        s.intensity = 0.02f;
        s.rang = 1;   // éclairage fixe (WFS-450)
    }
};

} // namespace

// banc « portique-checkpoint » : bâche du portique dessinée à la dernière image (faux : ligne peinte seule, ou rien)
bool portiqueCheckpointVisible() { return g_bache.visible; }

WFS_SYSTEM(Portique, 108)          // décor (100-199)
WFS_SYSTEM(PortiqueLumiere, 961)   // après la nuit (950)

} // namespace wfs
