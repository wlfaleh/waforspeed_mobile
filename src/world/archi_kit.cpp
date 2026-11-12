// WaForSpeed — WFS-161 (ST-1) : kit d'architecture, règles d'assemblage (voir archi_kit.h).
#include "archi_kit.h"
#include <algorithm>
#include <cmath>

namespace wfs::archi {
namespace {

constexpr uint8_t bit(Rdc r) { return (uint8_t)(1u << (unsigned)r); }
constexpr uint8_t bit(Etage e) { return (uint8_t)(1u << (unsigned)e); }
constexpr uint8_t bit(Toit t) { return (uint8_t)(1u << (unsigned)t); }

constexpr float LINTEAU = 0.2f;    // maçonnerie au-dessus d'une ouverture, sous le plancher suivant (m)
constexpr float TRUMEAU = 0.3f;    // mur plein minimal entre deux fenêtres d'une même travée voisine (m)
constexpr float RIVE = 0.15f;      // mur plein minimal au bord de la façade (m)

// WFS-402 (RG-02, RG-04) : retrait des tableaux de 15 à 25 cm ; fenêtre d'habitation 0,80-1,40 × 1,15-2,15 m, porte-fenêtre
// ≥ 2,15 m ; porte d'immeuble 1,0-1,4 × 2,2-3,0 m (cochère ≤ 2,5 m) ; porte de maison 0,90 × 2,15 m (village, villa).
// RG-02 : 6 styles. Ouvertures : plages courantes (à confirmer au référentiel, M-02) ; chaque plage est cohérente avec
// RG-01 : allège max + hauteur max ≤ étage min − linteau (2,7 − 0,2 = 2,5 m), fenêtre max ≤ travée min − trumeau.
const ReglesStyle STYLES[(int)Style::Nombre] = {
    // immeuble classique en pierre : portes-fenêtres à garde-corps, corniche moulurée, zinc
    {"classique", {2.6f, 3.2f}, {1.1f, 1.3f}, {2.15f, 2.3f}, {0.f, 0.f}, {1.1f, 1.4f}, {2.4f, 2.6f}, {2.6f, 3.0f},
     0.25f, 0.6f, 0.35f, 4, 6, (uint8_t)(bit(Rdc::Commerce) | bit(Rdc::Porche)), (uint8_t)(bit(Etage::Fenetres) | bit(Etage::Balcons)),
     (uint8_t)bit(Toit::Zinc)},
    // immeuble moderne verre et métal : bandeaux vitrés, allège basse, toiture-terrasse
    {"moderne", {1.6f, 1.9f}, {1.2f, 1.4f}, {1.8f, 2.0f}, {0.3f, 0.5f}, {1.0f, 1.2f}, {2.2f, 2.4f}, {2.6f, 3.0f},
     0.15f, 0.3f, 0.05f, 3, 8, (uint8_t)(bit(Rdc::Commerce) | bit(Rdc::Logement)), (uint8_t)(bit(Etage::Fenetres) | bit(Etage::Loggias)),
     (uint8_t)bit(Toit::Terrasse)},
    // méditerranéen crépi à volets : fenêtres plus hautes que larges, génoise, tuiles ou terrasse
    {"mediterraneen", {2.4f, 3.0f}, {0.9f, 1.1f}, {1.35f, 1.5f}, {0.9f, 1.0f}, {1.0f, 1.1f}, {2.2f, 2.3f}, {2.4f, 2.8f},
     0.2f, 0.35f, 0.25f, 2, 5, (uint8_t)(bit(Rdc::Commerce) | bit(Rdc::Logement) | bit(Rdc::Porche)),
     (uint8_t)(bit(Etage::Fenetres) | bit(Etage::Balcons)), (uint8_t)(bit(Toit::Tuiles) | bit(Toit::Terrasse))},
    // maison de village en pierre : murs épais (tableaux profonds), petites fenêtres, tuiles
    {"village", {2.6f, 3.6f}, {0.8f, 1.0f}, {1.15f, 1.4f}, {0.9f, 1.0f}, {0.9f, 0.9f}, {2.15f, 2.17f}, {2.2f, 2.6f},
     0.25f, 0.2f, 0.2f, 1, 2, (uint8_t)(bit(Rdc::Logement) | bit(Rdc::Garage)), (uint8_t)bit(Etage::Fenetres),
     (uint8_t)bit(Toit::Tuiles)},
    // bâtiment industriel en brique : grandes baies, portes de garage, zinc ou terrasse
    {"industriel", {3.5f, 4.5f}, {1.8f, 2.6f}, {1.3f, 1.5f}, {0.8f, 1.0f}, {1.0f, 1.2f}, {2.2f, 2.5f}, {2.6f, 3.0f},
     0.25f, 0.25f, 0.1f, 1, 3, (uint8_t)(bit(Rdc::Garage) | bit(Rdc::Porche)), (uint8_t)bit(Etage::Fenetres),
     (uint8_t)(bit(Toit::Zinc) | bit(Toit::Terrasse))},
    // villa contemporaine : grandes baies toute hauteur, toiture-terrasse
    {"villa", {3.0f, 4.5f}, {1.8f, 3.0f}, {2.15f, 2.3f}, {0.f, 0.f}, {0.9f, 0.9f}, {2.15f, 2.17f}, {2.4f, 2.8f},
     0.15f, 0.15f, 0.3f, 0, 1, (uint8_t)bit(Rdc::Logement), (uint8_t)(bit(Etage::Fenetres) | bit(Etage::Loggias)),
     (uint8_t)bit(Toit::Terrasse)},
};

// tire un élément parmi les bits permis
template <class E> E choisir(uint8_t bits, Mulberry32& R) {
    int n = 0;
    for (int i = 0; i < 8; i++) n += (bits >> i) & 1;
    int k = std::min(n - 1, (int)(R.next() * n));
    for (int i = 0; i < 8; i++) if ((bits >> i) & 1) { if (k-- == 0) return (E)i; }
    return (E)0;
}

// découpe de la façade : n travées de largeur dans la plage, centrées ; le reste en murs pleins de rive (un vrai
// bâtiment n'a pas toujours une largeur multiple de sa travée). Façade plus étroite que la travée minimale : une seule.
void decouper(Plan& p, const Plage& t, float voulue) {
    if (p.largeur < t.min) { p.travees = 1; p.travee = p.largeur; p.rive = 0; return; }
    const int nMax = std::max(1, (int)std::floor(p.largeur / t.min + 1e-4f));
    p.travees = std::clamp((int)std::lround(p.largeur / voulue), 1, nMax);
    p.travee = std::clamp(p.largeur / p.travees, t.min, t.max);
    p.rive = std::max(0.f, (p.largeur - p.travees * p.travee) / 2);
}

} // namespace

const ReglesStyle& regles(Style s) { return STYLES[std::min((int)s, (int)Style::Nombre - 1)]; }

Plan assembler(Style s, float largeur, float hauteurVoulue, uint32_t graine) {
    const ReglesStyle& S = regles(s);
    Mulberry32 R(graine * 2654435761u + (uint32_t)s * 97u + 1u);
    Plan p;
    p.style = s; p.largeur = std::max(largeur, 1.f);
    p.graine = graine;   // WFS-402 : modèles de fenêtre et de porte (archi_ouvertures.cpp), sans toucher aux tirages ci-dessous
    decouper(p, S.travee, S.travee.tirer(R));
    p.hRdc = H_RDC.tirer(R);
    p.hEtage = H_ETAGE.tirer(R);
    p.corniche = S.corniche;
    p.etages = std::clamp((int)std::lround((hauteurVoulue - p.hRdc - p.corniche) / p.hEtage), S.etagesMin, S.etagesMax);
    p.rdc = choisir<Rdc>(S.rdcPossibles, R);
    p.etage = choisir<Etage>(S.etagesPossibles, R);
    p.toit = choisir<Toit>(S.toitsPossibles, R);
    p.edicules = p.toit == Toit::Terrasse && p.etages >= 3;

    // ouvertures d'étage : mêmes dimensions sur toute la façade (une façade réelle est régulière)
    const float allege = S.allege.tirer(R);
    const float fenL = std::min(S.fenL.tirer(R), p.travee - TRUMEAU);
    const float fenH = std::min(S.fenH.tirer(R), p.hEtage - LINTEAU - allege);
    const Ouverture typeEtage = allege < 0.05f ? Ouverture::PorteFenetre : Ouverture::Fenetre;
    // rez-de-chaussée
    const float porteL = S.porteL.tirer(R), porteH = std::min(S.porteH.tirer(R), p.hRdc - LINTEAU);
    const float vitH = std::min(S.vitrineH.tirer(R), p.hRdc - LINTEAU);
    const float garL = std::min(2.4f + 0.6f * (float)R.next(), p.travee - TRUMEAU), garH = std::min(2.1f + 0.3f * (float)R.next(), p.hRdc - LINTEAU);
    const int entree = p.travees / 2;   // porte d'entrée au milieu de la façade
    for (int t = 0; t < p.travees; t++) {
        const float x = p.axe(t);
        if (t == entree) {
            const bool porche = p.rdc == Rdc::Porche;
            const float l = std::min(porche ? std::min(2.5f, p.travee - TRUMEAU) : porteL, p.travee - TRUMEAU);   // WFS-402 : porte cochère ≤ 2,5 m (RG-04)
            p.baies.push_back({Ouverture::Porte, 0, t, x, 0, l, porteH, false});
            continue;
        }
        switch (p.rdc) {
        case Rdc::Commerce: p.baies.push_back({Ouverture::Vitrine, 0, t, x, 0, p.travee - TRUMEAU, vitH, false}); break;
        case Rdc::Garage:
            if (t % 2 == 0 && garL > 2.f) { p.baies.push_back({Ouverture::PorteGarage, 0, t, x, 0, garL, garH, false}); break; }
            [[fallthrough]];
        default: {
            const float al = std::max(allege, 0.9f), h = std::min(fenH, p.hRdc - LINTEAU - al);
            p.baies.push_back({Ouverture::Fenetre, 0, t, x, al, fenL, h, false});
        }
        }
    }
    for (int n = 1; n <= p.etages; n++)
        for (int t = 0; t < p.travees; t++)
            p.baies.push_back({typeEtage, n, t, p.axe(t), p.niveauY(n) + allege, fenL, fenH, p.etage == Etage::Balcons});
    return p;
}

Plan facadeSecondaire(const Plan& p, float largeur, int face) {
    const ReglesStyle& S = regles(p.style);
    Mulberry32 R(p.graine * 2246822519u + (uint32_t)face * 3266489917u + (uint32_t)p.style * 97u + 11u);
    Plan q;
    q.style = p.style; q.graine = p.graine ^ (0x9e3779b9u * (uint32_t)face); q.secondaire = true;
    q.largeur = std::max(largeur, 1.f);
    q.hRdc = p.hRdc; q.hEtage = p.hEtage; q.etages = p.etages; q.corniche = p.corniche;
    q.rdc = Rdc::Logement; q.etage = Etage::Fenetres; q.toit = p.toit; q.edicules = p.edicules;
    decouper(q, S.travee, S.travee.tirer(R));
    // fenêtres d'étage de la rue (première baie d'étage) ; porte-fenêtre sur rue → fenêtre à allège de 0,9 m sur cour
    float fenL = S.fenL.min, fenH = S.fenH.min, allege = std::max(S.allege.min, 0.9f);
    for (const Baie& b : p.baies) if (b.niveau == 1) {
        fenL = b.l; fenH = b.h; const float al = b.y - p.niveauY(1);
        if (al >= 0.05f) allege = al;
        break;
    }
    fenL = std::min(fenL, q.travee - TRUMEAU);
    fenH = std::min(fenH, q.hEtage - LINTEAU - allege);
    if (fenL < 0.5f || fenH < 0.6f || q.largeur < fenL + 2 * RIVE) return q;   // mur trop étroit : aveugle
    const int porte = face == 1 && q.travees >= 3 ? q.travees / 2 : -1;   // porte de service sur cour
    const float porteL = std::min(S.porteL.min, q.travee - TRUMEAU), porteH = std::min(S.porteH.min, q.hRdc - LINTEAU);
    for (int n = 0; n <= q.etages; n++)
        for (int t = 0; t < q.travees; t++) {
            // travée = indice propre à la face : la graine de chaque vitre (pièce de nuit, graineBaie) diffère de la rue
            const int tr = t + 64 * face;
            if (n == 0 && t == porte) { q.baies.push_back({Ouverture::Porte, 0, tr, q.axe(t), 0, porteL, porteH, false}); continue; }
            const float al = n == 0 ? std::max(allege, 0.9f) : allege;
            const float h = std::min(fenH, q.niveauH(n) - LINTEAU - al);
            if (h < 0.6f) continue;
            q.baies.push_back({Ouverture::Fenetre, n, tr, q.axe(t), q.niveauY(n) + al, fenL, h, false});
        }
    return q;
}

int verifier(const Plan& p, const char** erreur) {
    const ReglesStyle& S = regles(p.style);
    int e = 0;
    auto faute = [&](const char* m) { if (e++ == 0 && erreur) *erreur = m; };
    if (!H_RDC.contient(p.hRdc)) faute("hauteur du rez-de-chaussée hors de 3,5-4,5 m (RG-01)");
    if (!H_ETAGE.contient(p.hEtage)) faute("hauteur d'étage hors de 2,7-3,2 m (RG-01)");
    if (p.etages < S.etagesMin || p.etages > S.etagesMax) faute("nombre d'étages hors des règles du style");
    if (p.travees < 1 || std::fabs(p.travee * p.travees + 2 * p.rive - p.largeur) > 1e-3f) faute("travées et rives qui ne couvrent pas la façade");
    if (p.largeur >= S.travee.min && !S.travee.contient(p.travee)) faute("largeur de travée hors de la plage du style");
    for (size_t i = 0; i < p.baies.size(); i++) {
        const Baie& b = p.baies[i];
        const float bas = p.niveauY(b.niveau), haut = bas + p.niveauH(b.niveau);
        if (b.x - b.l / 2 < RIVE - 1e-3f || b.x + b.l / 2 > p.largeur - RIVE + 1e-3f) faute("ouverture qui déborde de la façade");
        if (b.y < bas - 1e-3f || b.y + b.h > haut - LINTEAU + 1e-3f) faute("ouverture qui sort de son niveau (linteau de 0,2 m)");
        if (b.l <= 0 || b.h <= 0) faute("ouverture vide");
        if (b.niveau > 0 && b.l > p.travee - TRUMEAU + 1e-3f) faute("fenêtre plus large que sa travée moins le trumeau");
        if (b.niveau > 0 && p.travee >= S.travee.min && b.l >= S.fenL.min - 1e-3f && !S.fenL.contient(b.l)) faute("largeur de fenêtre hors de la plage du style");
        if (b.niveau > 0 && !S.fenH.contient(b.h) && b.h > S.fenH.max) faute("hauteur de fenêtre au-dessus de la plage du style");
        for (size_t j = i + 1; j < p.baies.size(); j++) {
            const Baie& c = p.baies[j];
            if (c.niveau == b.niveau && std::fabs(c.x - b.x) < (c.l + b.l) / 2 - 1e-3f) faute("deux ouvertures qui se chevauchent");
        }
    }
    return e;
}

} // namespace wfs::archi
