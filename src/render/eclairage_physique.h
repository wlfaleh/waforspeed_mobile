// WaForSpeed — WFS-151 (épic E13, palier 1 « Lumière ») : éclairage en unités physiques, exposition physique et
// automatique, éclairage d'image (IBL) préfiltré par sondes locales.
//
// RG-01 : à chaque image de course (pas les scènes dédiées : garage, essais), Game::render appelle physique(fp) après les
//   ambiances des cartes. Le soleil passe en lux (eclairementSoleilNormal : hauteur du soleil, masse d'air, trouble), la
//   lune en lux (0,32 lx), le ciel en cd/m² (ciel clair CIE type 12, niveau du référentiel), le sol et le brouillard en
//   cd/m². Toutes les valeurs envoyées au GPU sont « pré-exposées » : × k = exposition(EV100) (eclairage_unites.h).
// RG-02 : EV100 suivi par l'exposition automatique : histogramme des luminances de la sonde (moyenne pondérée au centre de
//   la vue, bande 80 à 98,3 %), adaptation 0,5 s vers le clair et 2 s vers le sombre, bornée par moment (jour 10 à 15,
//   nuit 3 à 5). Le post-traitement reçoit une exposition de 1 (fp.exposure) : même courbe ACES partout (RG-05).
// RG-03 : sondes : la capture d'environnement de WFS-150 (cubemap HDR) est relue sans arrêter le GPU (copie dans un
//   tampon, lue deux images plus tard), puis, sur un fil de travail : irradiance en harmoniques sphériques (9
//   coefficients), cube préfiltré GGX (un niveau par rugosité), carte des luminances pour l'exposition. La table BRDF
//   (split-sum) est calculée une fois. Deux sondes (A ancienne, B récente) sont mélangées par distance.
// RG-03 / ST-4 : sondes locales : une nouvelle capture tous les N mètres de la voiture (Basse 200 m, Moyenne 100 m,
//   Haute 60 m, Ultra 40 m), à l'entrée et à la sortie de chaque tunnel et pont, et tous les 40 m dessous. Avant la
//   première capture : sonde du ciel calculée sur le CPU (même modèle que sky.frag).
// RG-04 : en mode physique, scene.frag et scene_skin.frag prennent l'ambiance et les reflets dans l'IBL (iblRemplacer,
//   eclairage.glsl) ; fp.ambient vaut 1 (plus de facteur d'ambiance arbitraire) ; skyColor (reflets du vernis, des
//   vitres) lit le cube préfiltré. Les captures sont éclairées par les sondes en place : la lumière rebondit d'une
//   capture à la suivante. Capture éclairée par une sonde lointaine (ou celle du ciel) : la même place est recapturée
//   (2 fois au plus), pour que la lumière du dehors s'éteigne dans un tunnel (× albédo à chaque rebond, sans occlusion).
// RG-06 : intensités artistiques (émissions, halos, projecteurs, brouillard de nuit) : luminance physique
//   L = e × exposition d'origine / k(EV de référence du moment) ; elles suivent donc l'exposition comme le reste.
#pragma once
#include <cstdint>
#include <vector>
#include "../core/math.h"
#include "eclairage_unites.h"
#include "frame_ubo.h"
#include "vk.h"

namespace wfs {
struct FrameParams;
struct GameView;

namespace eclairage {

// état de la dernière image (banc, journal)
struct Etat {
    bool actif = false, nuit = false;
    double hauteurSoleil = 0;        // ° (direction de la lumière de la carte)
    double eSoleilNormal = 0;        // lx, incidence normale (soleil ou lune)
    double eSoleilHorizontal = 0;    // lx, sol horizontal
    double eCielDiffus = 0;          // lx, ciel seul sur le plan horizontal (modèle) ; nuit : ciel sans lune
    double luminanceZenith = 0;      // cd/m² (modèle)
    double facteurCie = 0;           // K : L = K f(γ) φ(Z)
    double ev = 15, evCible = 15;    // EV100 appliqué et visé
    double k = 0;                    // exposition (pré-exposition)
    double facteurArtistique = 1;    // RG-06 : multiplicateur des émissions, halos et projecteurs
    vec3 couleurSoleil{1, 1, 1};     // sRGB linéaire, luminance relative 1
    int sondesCapturees = 0;         // captures traitées depuis le lancement
    bool sondeCaptureActive = false; // la sonde B est une capture (sinon : sonde du ciel)
    vec3 positionSonde{};            // point de la sonde B
    double poidsB = 1;               // part de la sonde B dans le mélange
    double msTraitement = 0;         // dernier traitement CPU d'une sonde (ms)
    vec3 astre{0, 1, 0}; double astreHauteur = 0, astreAzimut = 0, astrePhase = 0;   // WFS-207 : astre calculé (soleil ou lune)
    int moment = 2; double heureUtc = 0, eSol = 0;   // WFS-208 : moment appliqué, heure UTC, éclairement horizontal au sol (lx)
    double luminanceMesuree = 0;     // cd/m², bande 80-98,3 % de l'histogramme (exposition automatique)
    double eLampadaireSol = -1;      // lx sous le premier lampadaire allumé (RG-06 : intensité artistique convertie), −1 sans
};
const Etat& etat();

// RG-01 à RG-06 : passe fp en unités physiques (appelé par Game::render après les ambiances ; dediee = scène dédiée : rien)
void physique(FrameParams& fp, bool dediee, const GameView& g);
// Renderer::beginFrame (accroche) : SH, sondes, exposition, ciel dans u ; liaisons 3 à 5 des deux jeux de l'image
void avantImage(VkCtx& vk, FrameUBO& u, VkDescriptorSet image, VkDescriptorSet reflet);

// irradiance de la sonde mélangée (lx) reçue par une surface de normale n (banc : valeur attendue)
void irradiance(const vec3& n, double E[3]);

// ---- banc (eclairage_banc.cpp) ----
struct Forcage {
    bool actif = false;
    double hauteurSoleil = -100;     // ° ; < −90 : celle de la carte
    double ev = -100;                // EV100 fixe ; < −50 : exposition automatique
    double heureUtc = -100;          // WFS-207 : heure UTC du soleil (lieu et date fictifs) ; < −50 : celle du moment
    bool sansNuages = false;         // WFS-208 : ciel clair (mesures de chromaticité M-03 de WFS-207)
    double azimutSoleil = -1000;     // WFS-207 : azimut forcé (°, du nord vers l'est ; étalon de WFS-151) ; < −500 : celui du soleil calculé
    bool abscisseForcee = false; double abscisse = 0;   // abscisse (unités de la piste) des sondes (tunnels, ponts)
    bool pointForce = false; vec3 point{};             // point des sondes (sinon la voiture)
};
void forcer(const Forcage& f);
// WFS-208 : moment de la journée (atmosphere::Moment : aube, matin, midi, heure dorée, heure bleue, nuit, heure réelle), choisi à
// l'écran Terrain (sinon relu dans la sauvegarde ; < 0 : aucun choix) ; le temps écoulé repart de zéro ; accélération du temps (× 1 par défaut)
void choisirMoment(int moment);
int momentChoisi();
void accelererTemps(double facteur);
void desactiver(bool oui);            // banc avant / après : rendu d'origine (aucune conversion)
void suspendreCaptures(bool oui);     // WFS-222 (RG-03) : baisse automatique : aucune nouvelle capture (sondes en place gardées)
bool sondeCapturePrete();             // au moins une capture traitée depuis la dernière remise à zéro
void remettreAZero();                 // nouvelle scène : sondes oubliées, exposition recalée sur la prochaine mesure

} // namespace eclairage
} // namespace wfs
