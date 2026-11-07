// WaForSpeed — obstacles solides des cartes pour les chocs de la voiture du joueur (WFS-256, ANO-129) : voir chocs_decor.h.
// Système « chocs-decor » (ordre 99, avant les cartes 100-199) :
//  - init : registre vidé et dimensionné pour la piste ; la Ville est déclarée ici (son plan, ses carrefours et ses ouvrages
//    sont publics : ville_quartiers.h) ; la Plage, la Campagne et le Grand Prix déclarent leurs objets à leur init (accroches
//    de plage.cpp et decor_carte.cpp, qui appellent declarer…() ci-dessous) ; l'îlot de chaque rond-point est déclaré ici
//    pour toutes les cartes (giratoire.h). Les filtres du dessin sont repris à l'identique (objet sur une autre partie de la
//    route ou dans l'ouvrage d'un giratoire : pas dessiné, donc pas d'obstacle : RG-05, aucun mur invisible).
//  - collide (après le pas de conduite) : chocs du pas → règles de la course (frottement, petit choc, accident), comme les
//    chocs du trafic (Course::applyHits) ; les sons et vibrations suivent (WFS-28, WFS-70, WFS-255).
// Formes : relevées sur le code de dessin de chaque objet (rayon des mâts, emprise des bancs…), écart ≤ 10 cm (RG-02).
#include <cmath>
#include <cstring>
#include "../core/log.h"
#include "../core/system.h"
#include "../world/chaussee.h"
#include "../world/giratoire.h"
#include "../world/ville_quartiers.h"
#include "chocs_decor.h"
#include "course_system.h"
#include "player.h"

namespace wfs {

namespace {
using chocs::Forme; using chocs::Genre; using chocs::ObstacleRoute;
constexpr float R = (float)RW;

ObstacleRoute cylindre(float lat, float r, Genre g, float along = 0) { ObstacleRoute o; o.lat = lat; o.r = r; o.genre = g; o.along = along; return o; }
ObstacleRoute boite(float lat, float hx, float hz, Genre g, float along = 0) {
    ObstacleRoute o; o.forme = Forme::Boite; o.lat = lat; o.hx = hx; o.hz = hz; o.genre = g; o.along = along; return o;
}

// filtres du dessin (ville.cpp, decor_carte.cpp : surAutreChaussee, surGiratoire, marge 1 m)
struct Filtre {
    std::vector<chaussee::Axe> serres;
    std::vector<giratoire::Giratoire> gir;
    int64_t L = 1;
    void preparer(Track& t) {
        L = (int64_t)t.segmentCount();
        serres = chaussee::axes(t, {trace::Type::RondPoint, trace::Type::Epingle, trace::Type::Lacets});
        gir.clear();
        for (const trace::Section& s : trace::sections(t))
            if (s.type == trace::Type::RondPoint) { giratoire::Giratoire g = giratoire::calculer(t, s); if (g.ok) gir.push_back(g); }
    }
    bool dessine(Track& t, int64_t seg, float lat) const {
        const TrackPoint P = t.point(seg * SEG, lat);
        const vec3 q{(float)P.x, (float)P.y, (float)P.z};
        if (chaussee::surAutre(serres, L, q, seg, 1.0f)) return false;
        for (const giratoire::Giratoire& g : gir) if (giratoire::dansOuvrage(g, q, 1.0f)) return false;
        return true;
    }
};
Filtre g_filtre;

void declarer(Track& t, int64_t seg, const ObstacleRoute& o, bool filtrer = true) {
    if (filtrer && !g_filtre.dessine(t, seg, o.lat)) return;
    chocs::registre().ajouter(seg, o);
}

// ---------------------------------------------------------------- Ville (ville.cpp : objet(), lieux(), ouvrages)
int declarerVille(Track& t, const MapDef& map) {
    const ville::Plan& P = ville::adapter(map, t);
    const int64_t L = (int64_t)P.objets.size();
    size_t avant = chocs::registre().taille();
    for (int64_t a = 0; a < L; a++) {
        for (const ville::Objet& o : P.objets[(size_t)a]) {
            const float s = (float)o.side;
            switch (o.t) {
            case ville::LAMPE: declarer(t, a, cylindre(s * (R + 0.8f), 0.09f, Genre::Poteau)); break;              // mât de 9 cm
            case ville::ARBRE_RUE: declarer(t, a, cylindre(s * o.lat, 0.22f, Genre::Arbre)); break;               // tronc de platane
            case ville::ABRIBUS: declarer(t, a, boite(s * (R + 4.32f), 0.38f, 2.1f, Genre::Mobilier)); break;     // banc + paroi du fond
            case ville::GRILLE: declarer(t, a, boite(s * o.lat, 0.04f, 2 * (float)SEG_M, Genre::Mur)); break;     // lisse de 4 segments
            case ville::GRILLAGE: declarer(t, a, boite(s * o.lat, 0.05f, 1.5f * (float)SEG_M, Genre::Mur)); break;
            case ville::BANC: declarer(t, a, boite(s * o.lat, 0.25f, 0.9f, Genre::Mobilier)); break;              // 0,5 × 1,8 m
            case ville::ARBRE: declarer(t, a, cylindre(s * o.lat, 0.25f, Genre::Arbre, o.along)); break;
            case ville::FONTAINE: declarer(t, a, cylindre(s * o.lat, 4.0f, Genre::Mobilier)); break;              // bassin de 4 m
            case ville::IMMEUBLE: {   // boîte : façade à RW + 6,5 m, profondeur d, longueur len à partir du segment
                const float lat = s * (R + 6.5f + o.d / 2);
                declarer(t, a, boite(lat, o.d / 2, o.len / 2, Genre::Batiment, o.len / 2), false);   // jamais filtré au dessin
                break;
            }
            default: break;   // passages zébrés, entrepôts, conteneurs, grues : sols ou hors de portée (au-delà du bord de la carte)
            }
        }
    }
    // panneaux néon (enseignes.cpp) : deux poteaux de 30 × 30 cm à RW + 1 et RW + 5
    for (const ville::Panneau& b : P.panneaux) for (float l : {R + 1.f, R + 5.f}) declarer(t, b.seg, boite((float)b.side * l, 0.15f, 0.15f, Genre::Poteau));
    // carrefours à feux : 4 mâts de 7 cm à RW + 0,8 m (ligne d'arrêt, et 7,7 m après le milieu de la rue transversale)
    for (const ville::Carrefour& cf : P.carrefours)
        for (float sd : {-1.f, 1.f}) {
            declarer(t, cf.ligne, cylindre(sd * (R + 0.8f), 0.07f, Genre::Poteau), false);
            declarer(t, cf.croise, cylindre(sd * (R + 0.8f), 0.07f, Genre::Poteau, ville::RUE_TRANS_M + 3.2f), false);
        }
    // panneaux « sens giratoire » : 6 m avant l'anneau, mât de 4 cm à RW + 1,2 m des deux côtés
    for (const ville::RondPoint& rp : P.ronds)
        for (float sd : {-1.f, 1.f}) declarer(t, rp.debut - (int64_t)(6 / SEG_M), cylindre(sd * (R + 1.2f), 0.04f, Genre::Poteau), false);
    // ouvrages (WFS-175) : autoroute (glissière à droite RW + 3,6 ; séparateur béton DBA à −(RW + 4,95), 60 cm), tunnel
    // (piédroits à ±(RW + 4)), pont (corniche et garde-corps à ±(RW + 4,45)) : une boîte par segment
    if (!P.voie.empty())
        for (int64_t a = 0; a < L; a++) {
            const float demi = (float)SEG_M / 2 + 0.05f;
            switch (P.voie[(size_t)a]) {
            case ville::AUTOROUTE:
                declarer(t, a, boite(R + 3.6f + 0.1f, 0.1f, demi, Genre::Mur, (float)SEG_M / 2), false);
                declarer(t, a, boite(-(R + 4.95f), 0.3f, demi, Genre::Mur, (float)SEG_M / 2), false);
                break;
            case ville::TUNNEL:
                for (float sd : {-1.f, 1.f}) declarer(t, a, boite(sd * (R + 4.0f + 0.15f), 0.15f, demi, Genre::Mur, (float)SEG_M / 2), false);
                break;
            case ville::PONT:
                for (float sd : {-1.f, 1.f}) declarer(t, a, boite(sd * (R + 4.45f + 0.15f), 0.15f, demi, Genre::Mur, (float)SEG_M / 2), false);
                break;
            default: break;
            }
        }
    return (int)(chocs::registre().taille() - avant);
}

// îlot central de chaque rond-point (giratoire.h : bordure de 15 cm à R_ILOT = 8 m du centre, fontaine, arbres) : déclaré
// par rapport au segment du début de l'anneau (le centre est loin de l'axe : la fenêtre de recherche suit, chocs_decor.h)
int declarerIlots(Track& t) {
    int n = 0;
    for (const giratoire::Giratoire& g : g_filtre.gir) {
        const TrackPoint P = t.pointLisse(g.debut * SEG, 0);
        const double fx = std::sin(P.th), fz = -std::cos(P.th), rx = std::cos(P.th), rz = std::sin(P.th);
        const double dx = g.c.x - P.x, dz = g.c.z - P.z;
        ObstacleRoute o = cylindre((float)(dx * rx + dz * rz), giratoire::R_ILOT, Genre::Ilot, (float)(dx * fx + dz * fz));
        chocs::registre().ajouter(g.debut, o); n++;
    }
    return n;
}
} // namespace

namespace chocs {
// accroches des cartes (plage.cpp, decor_carte.cpp) : un objet de leur décor, au segment seg, après le filtre du dessin
void declarerObjet(Track& t, int64_t seg, const ObstacleRoute& o, bool filtrer) { declarer(t, seg, o, filtrer); }
}

class ChocsDecor : public System {
public:
    const char* name() const override { return "chocs-decor"; }
    void init(Renderer&, const GameView& g) override {
        chocs::registre().vider();
        if (!g.track || !g.map) return;
        Track& t = *g.track;
        chocs::registre().preparer((int64_t)t.segmentCount());
        g_filtre.preparer(t);
        int nv = 0;
        if (!std::strcmp(g.map->id, "ville")) nv = declarerVille(t, *g.map);
        const int ni = declarerIlots(t);
        WFS_LOG("chocs (WFS-256) : %s : %d obstacle(s) de la Ville, %d îlot(s) de rond-point (les autres cartes déclarent leur décor à leur chargement)",
                g.map->id, nv, ni);
    }
    void shutdown(Renderer&) override { chocs::registre().vider(); }
    // après le pas de conduite (les systèmes ne sont appelés qu'après Player::update) : règles de la course
    void collide(double dt, Player& pl, const GameView&) override {
        cdFrottement_ -= dt;
        if (pl.chocsDecor.n == 0) return;
        const Player::ChocsDuPas c = pl.chocsDecor;
        pl.chocsDecor = {};
        Course* course = activeCourseMut();
        if (!course || !course->playing()) return;
        HitEvents e;
        const double v = c.impact;   // m/s, vitesse d'approche de l'obstacle
        if (v < 2.0) { if (cdFrottement_ <= 0) { e.scrapes++; cdFrottement_ = 0.18; } }   // frottement (même cadence que le trafic)
        else if (v < 14.0) e.push('b', v / (MAX_SPEED * M));                                 // petit choc (impact comme le trafic : vitesse / vitesse max)
        else e.push('a', v / (MAX_SPEED * M));                                                // accident : 50 km/h et plus contre un obstacle fixe
        course->applyHits(e);
        if (v >= 2.0) WFS_LOG("choc contre %s à %.0f km/h%s", chocs::nomGenre(c.genre), v * 3.6, v >= 14.0 ? " (accident)" : "");
    }
private:
    double cdFrottement_ = 0;
};

} // namespace wfs

WFS_SYSTEM(wfs::ChocsDecor, 99)   // avant les cartes (100-199), qui déclarent leurs objets à leur init
