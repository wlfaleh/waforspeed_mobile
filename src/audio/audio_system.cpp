// WaForSpeed — module « son » (WFS-10) : son du moteur de la voiture du joueur, branché sur le moteur du jeu
// par les systèmes enfichables (WFS-30). Le régime est calculé à chaque pas de conduite (thread du jeu) ;
// le son est fabriqué sur le thread audio (AAudio). Sur le banc d'essai PC, il est calculé hors ligne et
// enregistré (moteur.wav, moteur-courbe.png) pour la QA.
#include "audio_system.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include "../../third_party/stb/stb_image_write.h"
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/system.h"
#include "../ui/ecrans/reglages.h"   // WFS-68 : réglage du son
#include "../game/cars.h"
#include "../game/game.h"
#include "../game/player.h"
#include "audio_out.h"
#include "engine_synth.h"
#include "sfx.h"   // WFS-28 : bruitages
#include "musique.h"   // WFS-76 : musique et ambiance de la carte
#include "../game/course_system.h"
#include <cstdlib>
#include "../game/traffic.h"           // WFS-121 : lecture seule (M, MAX_SPEED, TrafficCar)
#include "../game/traffic_system.h"    // WFS-121 : activeTraffic(), activeTrafficStats() (lecture seule)
#include "trafic_sons.h"               // WFS-121 : passages, klaxon, rumeur
#include "garage_son.h"                // WFS-127 : showroom sonore
#include "../ui/garage.h"              // WFS-127 : voiture affichée au garage (lecture seule)
#include "ambiance_lieux.h"            // WFS-261 : son des lieux (sources placées, tunnels, stéréo)
#include "../game/trace_cartes.h"      // WFS-261 : section sous la voiture (tunnel), lecture seule
#include "../world/enseignes.h"        // WFS-261 : clients attablés des terrasses (vie::clientsTerrasses), lecture seule
#include "voitures_sons.h"             // WFS-272 : son des autres voitures (adversaires, police, trafic)
#include "parois_sons.h"               // WFS-275 : écho des murs, sons masqués derrière les immeubles
#include "../world/ville_quartiers.h"  // WFS-275 : plan de la Ville (façades, immeubles), lecture seule
#include "../game/course5.h"           // WFS-272 : adversaires de la Course à 5 (lecture seule)
#include "../game/duel_system.h"       // WFS-272 : adversaires du Duel (duelSession().ghosts(), lecture seule)
#include "../game/poursuite.h"         // WFS-272 : police de la Poursuite (lecture seule)
#include "pluie_sons.h"                // WFS-288 : son de la pluie (averse, tambourinage, essuie-glaces)
#include "../world/meteo.h"            // WFS-288 : meteo::pluieActive (lecture seule)
#include "../game/camera_vue.h"        // WFS-288 : vue choisie (vueCamera, VUES), lecture seule

namespace wfs {

static const char* SAMPLES[] = {"audio/sport.wav", "audio/v8.wav", "audio/citadine.wav", "audio/thermique.wav", "audio/gros.wav"};
constexpr int SAMPLE_COUNT = sizeof(SAMPLES) / sizeof(SAMPLES[0]);
constexpr int OUT_RATE = 48000;

struct BenchRec { bool on = false; std::vector<float> pcm; std::vector<EngineFrame> frames; double acc = 0, t = 0; double e2Moteur = 0, e2Sfx = 0; };   // WFS-174 : énergies moteur / bruitages
static BenchRec g_rec;

class AudioSystem : public System {
public:
    const char* name() const override { return "son"; }

    void init(Renderer&, const GameView& g) override {
        for (int i = 0; i < SAMPLE_COUNT; i++) {
            std::vector<uint8_t> w;
            synths_[i] = std::make_unique<EngineSynth>();
            if (!readAsset(SAMPLES[i], w) || !synths_[i]->loadWav(w)) WFS_ERR("son : %s illisible", SAMPLES[i]);
            synths_[i]->setOutputRate(OUT_RATE);
        }
        active_ = -1;
        // WFS-261 : tampons du rappel et son des lieux prêts AVANT l'ouverture du flux (aucune allocation dans le rappel)
        const char* mo = std::getenv("WFS_MOMENT");
        const bool nuit = mo && std::strcmp(mo, "nuit") == 0;
        for (auto* b : {&blocMono_, &blocG_, &blocD_, &blocBus_}) b->assign(BLOC, 0.f);
        lieux_.build((float)OUT_RATE, nuit); tunnel_.build((float)OUT_RATE);
        voitures_.build((float)OUT_RATE); attribution_.reset(); candidats_.reserve(128);   // WFS-272
        echos_.build((float)OUT_RATE);   // WFS-275
        pluie_.build((float)OUT_RATE);   // WFS-288
        const int rate = audioOutStart([this](float* out, int n) { remplir(out, n); });
        if (rate && rate != OUT_RATE) for (auto& s : synths_) s->setOutputRate((float)rate);
        if (rate && rate != OUT_RATE) { lieux_.build((float)rate, nuit); tunnel_.build((float)rate); voitures_.build((float)rate); echos_.build((float)rate); pluie_.build((float)rate); }   // WFS-261, WFS-272, WFS-275, WFS-288
        sfx_.build((float)(rate ? rate : OUT_RATE));   // WFS-28 : bruitages calculés une fois, à la fréquence du flux
        // WFS-76 : musique (une fois par lancement) et ambiance de la carte de cette partie (moment : WFS_MOMENT, comme la plage)
        musique().build((float)(rate ? rate : OUT_RATE));
        ambiance_.build((float)(rate ? rate : OUT_RATE), g.map ? g.map->id : "ville", nuit);
        vMusique_ = -1; ambActive_ = -1;
        initLieux(g);   // WFS-261
        decider(g);
    }
    // WFS-76 : l'application dessine encore (sinon fondu vers le silence) ; décision aussi à chaque image : en pause (plus
    // d'update), un réglage « Musique » change le son tout de suite
    void render(Renderer&, const GameView& g) override { vivant_ = nowSeconds(); decider(g); }
    void onEcran(Ecran, Ecran, const GameView& g) override {   // WFS-76 : pause (plus d'update), menus
        decider(g);
        if (g.ecran != Ecran::Course && rumeur_ > 0) { sfx_.setRumeur(0); rumeur_ = 0; traficSons_.reset(); WFS_LOG("rumeur 0.00"); }   // WFS-121 : aucun son coincé
    }

    void shutdown(Renderer&) override { audioOutStop(); }

    // à chaque pas de conduite : régime, rapport, charge → cibles du son
    void update(double dt, const GameView& g) override {
        const Player& pl = *g.player;
        // WFS-127 (RG-01 / RG-02) : au garage, la voix de la voiture affichée ; ailleurs, celle de la voiture retenue
        const char* montree = g.ecran == Ecran::Garage ? ecrans::garageVoitureAffichee() : nullptr;
        const EngineVoiceDef& voice = engineVoiceFor(montree ? montree : g.car->id);
        int idx = 0;
        for (int i = 0; i < SAMPLE_COUNT; i++) if (std::strcmp(SAMPLES[i], voice.sample) == 0) idx = i;
        // pédale d'accélérateur : déduite de la conduite (le module ne lit que l'état du joueur) :
        // nitro, ou la voiture ne ralentit pas comme au lever de pied (frottements + frein moteur ≥ 1,2 m/s²)
        const double v = std::abs(pl.speed) * M;
        const bool throttle = pl.nitroOn || (!pl.braking && (pl.lonAcc > 0.4 || (v > 0.5 && pl.lonAcc > -0.6)));
        const double top = MAX_SPEED * pl.perf.top * (pl.nitroOn ? pl.perf.nitroMult : 1);
        EngineSynth& s = *synths_[idx];
        if (pl.modeleMonde() && !montree) {   // WFS-255 (RG-03) : régime et rapport réels de la conduite réaliste
            const double coupure = physique::params(*pl.car).wMax * 60 / (2 * 3.14159265358979);
            model_.updateReel(dt, pl.monde.regime, coupure, pl.monde.rapport, pl.speed, top, throttle, pl.nitroOn, true, voice.pitch, false, s.targets);
        } else model_.update(dt, pl.speed, top, MAX_SPEED, throttle, pl.nitroOn, true, voice.pitch, false, s.targets);
        {   // WFS-120 : pétarades, turbo, rupteur (en course seulement, aucune pour la Model 3 électrique)
            const Course* c = activeCourse();
            const bool enCourse = g.ecran == Ecran::Course && !g.sceneDediee && c && c->state == RaceState::Play;
            effets_.update(dt, model_.rpm, model_.gear, (float)(v / std::max(top * M, 1e-6)), throttle, (float)pl.kmh(), enCourse,
                           std::strcmp(g.car->id, "model3") == 0);
            for (int k = 0; k < effets_.claquements; k++) sfx_.play(SfxId::Petarade, 5.f);   // claquement court : crête 0,03 → audible sur le moteur
            if (effets_.turbo) sfx_.play(SfxId::Turbo);
            if (effets_.rupteurDebut) WFS_LOG("SFX rupteur");
            if (effets_.couper) { s.targets.hiGain = 0; s.targets.hiTau = 0.006f; }
        }
        showroom(dt, g, montree, voice, s);   // WFS-127
        s.targets.lastUpdate = nowSeconds();
        for (int i = 0; i < SAMPLE_COUNT; i++) if (i != idx) { synths_[i]->targets.loGain = 0; synths_[i]->targets.hiGain = 0; }
        active_ = idx;
        decider(g);   // WFS-76
        sfx_.lastUpdate = nowSeconds(); events_.update(dt, g, sfx_);   // WFS-28 : un son par événement de la partie
        sonsTrafic(dt, g);   // WFS-121
        preparerParois(g);   // WFS-275 : immeubles proches (masques), avant les sources
        sonsLieux(g);        // WFS-261
        sonsVoitures(dt, g); // WFS-272
        sonsParois(dt, g);   // WFS-275
        sonsPluie(dt, g);    // WFS-288
        if (g_rec.on) {   // banc d'essai : le son de ce pas est calculé tout de suite et gardé
            g_rec.t += dt; g_rec.acc += dt * OUT_RATE;
            const int n = (int)g_rec.acc; g_rec.acc -= n;
            const size_t at = g_rec.pcm.size(); g_rec.pcm.resize(at + (size_t)n);
            for (auto& x : synths_) x->targets.lastUpdate = 1e9;   // hors ligne : pas de « pause » détectée
            renderInto(g_rec.pcm.data() + at, n, 0);
            for (int k = 0; k < n; k++) g_rec.e2Moteur += (double)g_rec.pcm[at + k] * g_rec.pcm[at + k];   // WFS-174 (RG-08) : moteur seul
            sfx_.lastUpdate = 1e9; sfx_.mix(g_rec.pcm.data() + at, n, 1e9);   // WFS-120 (RG-07) : bruitages dans moteur.wav
            g_rec.frames.push_back({(float)g_rec.t, model_.rpm, model_.gear, model_.load, (float)pl.kmh(), throttle});
        }
    }

private:
    // WFS-121 : sons du trafic. RG-01 : trafic lu par activeTraffic() (lecture seule) ; RG-06 : écran Course, course en jeu
    // (ou conduite libre, sans course) ; duel et course à 5 : le trafic est vide → aucun son
    void sonsTrafic(double dt, const GameView& g) {
        const Traffic* tr = activeTraffic();
        const TrafficStats* st = activeTrafficStats();
        const Course* c = activeCourse();
        const bool actif = tr && st && g.track && g.ecran == Ecran::Course && !g.sceneDediee && (!c || c->state == RaceState::Play);
        const Player& pl = *g.player;
        voituresSon_.resize(actif ? tr->cars.size() : 0);
        if (actif) for (size_t i = 0; i < tr->cars.size(); i++) {
            const TrafficCar& tc = tr->cars[i];
            VoitureSon& v = voituresSon_[i];
            v.dzM = g.track->relZ(tc.z - (pl.position + PLAYER_Z)) * M;
            v.lat = std::abs(tc.offset - pl.playerX);
            v.vRelKmh = std::abs(pl.speed - tc.speed) / MAX_SPEED * 260.0;   // même conversion que Player::kmh()
            v.longueurM = tc.len * M; v.accidentee = tc.crashed;
        }
        const SonsDuPas s = traficSons_.update(dt, actif, voituresSon_, pl.kmh(), webCarLength(pl.car->id, pl.car->d.L) * M,
                                              st ? st->nearMisses : 0);
        // WFS-272 (RG-06) : passage et klaxon joués avec le gauche / droite de leur voiture (même tampon, même gain) ; le banc
        // hors ligne (moteur.wav, mono) les garde dans SfxMixer, comme avant
        if (s.passages) { if (g_rec.on) sfx_.play(SfxId::Passage, s.gainPassage); else { voitures_.jouer(&sfx_.tampon(SfxId::Passage), s.gainPassage, panTrafic(g, s.voiturePassage)); WFS_LOG("SFX %s", sfxNom(SfxId::Passage)); } }
        if (s.klaxon) { if (g_rec.on) sfx_.play(SfxId::Klaxon); else { voitures_.jouer(&sfx_.tampon(SfxId::Klaxon), 1.f, panTrafic(g, s.voitureKlaxon)); WFS_LOG("SFX %s", sfxNom(SfxId::Klaxon)); } }
        if (s.rumeur != rumeur_) { sfx_.setRumeur(s.rumeur); rumeur_ = s.rumeur; }
        if (s.journalRumeur) WFS_LOG("rumeur %.2f", s.rumeur);
    }
    TraficSons traficSons_; std::vector<VoitureSon> voituresSon_; float rumeur_ = 0;
    // WFS-76 (RG-02 à RG-06) : musique « course » de « GO » à la fin de la course, « menu » partout ailleurs (menus, garage,
    // terrain, réglages, pause, décompte, écran de fin) ; ambiance de la carte sur l'écran Course seulement ; journal (RG-08)
    void decider(const GameView& g) {
        const Course* c = activeCourse();
        Variante v = g.ecran == Ecran::Course && !g.sceneDediee && c && c->state == RaceState::Play ? Variante::Course : Variante::Menu;
        if (!ecrans::reglages().musique) v = Variante::Silence;
        if ((int)v != vMusique_) {
            vMusique_ = (int)v; musique().cible(v);
            WFS_LOG("MUSIQUE %s", v == Variante::Course ? "course" : v == Variante::Menu ? "menu" : "coupée");
        }
        const int amb = g.ecran == Ecran::Course && !g.sceneDediee ? 1 : 0;
        lieuxActifs_ = amb; lieux_.active(amb);   // WFS-261 : sources placées et écho comme l'ambiance (écran Course seulement)
        pluie_.active(amb);                       // WFS-288 (RG-02) : la pluie aussi
        if (!amb) tunnel_.dedans(false);
        if (amb != ambActive_) {
            ambActive_ = amb; ambiance_.active(amb);
            if (!amb) WFS_LOG("AMBIANCE coupée");
            else if (ambiance_.nuit()) WFS_LOG("AMBIANCE nuit (%s)", ambiance_.carte().c_str());
            else WFS_LOG("AMBIANCE %s", ambiance_.carte().c_str());
        }
    }
    Ambiance ambiance_;
    int vMusique_ = -1, ambActive_ = -1;
    // WFS-261 (RG-01) : rappel audio. Le son d'avant (moteur, bruitages, musique, ambiance) est calculé en mono comme avant, par
    // blocs de BLOC trames ; l'écho du tunnel (moteur, vent, trafic : avant musique et ambiance) et les sources placées
    // s'ajoutent à gauche et à droite ; le mono est recopié à l'égal dans les deux canaux. Aucune allocation, aucun verrou.
    static constexpr int BLOC = 1024;
    std::vector<float> blocMono_, blocG_, blocD_, blocBus_;
    void remplir(float* out, int n) {
        const double now = nowSeconds();
        const int canaux = audioOutCanaux();
        // WFS-76 : application en arrière-plan (plus d'images) : fondu vers le silence (musique et ambiance)
        gVie_ += ((now - vivant_.load() < 0.5 ? 1.f : 0.f) - gVie_) * 0.15f;
        const float master = sfx_.master * gVie_;
        for (int fait = 0; fait < n;) {
            const int k = std::min(n - fait, BLOC);
            float* m = blocMono_.data(); float* G = blocG_.data(); float* D = blocD_.data(); float* bus = blocBus_.data();
            std::fill(G, G + k, 0.f); std::fill(D, D + k, 0.f);
            renderInto(m, k, now); sfx_.mix(m, k, now, G, D);   // WFS-272 : la sirène sort à gauche / à droite (G, D)
            for (int i = 0; i < k; i++) bus[i] = m[i] + 0.5f * (G[i] + D[i]);   // écho : moteur, bruitages, sirène
            voitures_.mix(G, D, bus, k, sfx_.master, now - sfx_.lastUpdate.load() < 0.25);   // WFS-272 : autres voitures, passage, klaxon
            tunnel_.process(bus, G, D, k);   // RG-05 (WFS-261) ; RG-07 (WFS-272) : les voitures passent dans l'écho
            echos_.process(bus, G, D, k);    // WFS-275 : écho des murs (même bus) ; sans mur, rien n'est ajouté
            musique().mix(m, k, master); ambiance_.mix(m, k, master);
            lieux_.mix(G, D, k, master);   // RG-02 à RG-04
            pluie_.mix(G, D, k, master);   // WFS-288 : hors de l'écho (dehors) ; muette hors pluie : rien n'est touché
            lieux::entrelacer(m, G, D, out + (size_t)fait * (size_t)canaux, k, canaux);
            fait += k;
        }
        if (!ecrans::reglages().son) std::fill(out, out + (size_t)n * (size_t)canaux, 0.f);   // WFS-68 : réglage « Son : Coupé »
    }
    // WFS-261 (RG-02) : sources de la carte : celles des cartes (registre lieux::, rempli à leur init, avant ce système) et les
    // terrasses de la Ville (clients attablés, regroupés par terrasse) ; journal du banc : nombre de sources par type
    lieux::SonsDesLieux lieux_; lieux::Tunnel tunnel_;
    std::string carte_; bool lieuxActifs_ = false, tunnelLog_ = false, aPortee_[lieux::NB_TYPES] = {};
    void initLieux(const GameView& g) {
        carte_ = g.map ? g.map->id : ""; tunnelLog_ = false;
        if (!g.map || !g.track) return;
        if (carte_ == "ville") {
            std::vector<lieux::Attable> cl;
            for (const vie::Client& c : vie::clientsTerrasses(*g.map, (int64_t)g.track->segmentCount())) cl.push_back({c.s, c.lat});
            lieux::poser("ville", lieux::TERRASSE, lieux::regrouperTerrasses(cl, (int64_t)g.track->segmentCount()));
        }
        immeubles_.clear();   // WFS-275 : Ville : façades (écho) et immeubles (masques) lus dans le plan ; autres cartes : accroches
        if (carte_ == "ville") {
            const ville::Plan& P = ville::adapter(*g.map, *g.track);
            parois::depuisVille(P, (int64_t)g.track->segmentCount());
            immeubles_ = parois::immeublesVille(P, (int64_t)g.track->segmentCount());
        } else {
            immeubles_ = parois::obstacles(carte_);   // WFS-278 (RG-03) : roche, maisons, garages… déclarés par les cartes
        }
        {
            int nG = 0, nD = 0;
            for (const parois::Cotes& c : parois::lire(carte_)) { nG += c[0].d > 0; nD += c[1].d > 0; }
            WFS_LOG("PAROIS %s : %d segments avec une paroi à gauche, %d à droite, %zu immeubles (masques)", carte_.c_str(), nG, nD, immeubles_.size());
            float pMin = 1e9f, pMax = 0, hMin = 1e9f, hMax = 0; double pSom = 0;   // WFS-278 : profondeur et hauteur des obstacles (journal)
            for (const parois::Immeuble& im : immeubles_) {
                pMin = std::min(pMin, 2 * im.demiProf); pMax = std::max(pMax, 2 * im.demiProf); pSom += 2 * im.demiProf;
                hMin = std::min(hMin, im.h); hMax = std::max(hMax, im.h);
            }
            if (!immeubles_.empty())
                WFS_LOG("PAROIS %s (WFS-278) : obstacles : profondeur %.1f à %.1f m (moyenne %.1f), hauteur %.1f à %.1f m", carte_.c_str(), pMin, pMax,
                        pSom / (double)immeubles_.size(), hMin, hMax);
        }
        suiviParois_.reset(); for (float& m : masqueLieu_) m = 0; for (auto& x : masqueId_) x = 0;
        char txt[160]; int k = 0;
        for (int t = 0; t < lieux::NB_TYPES; t++)
            k += std::snprintf(txt + k, sizeof txt - (size_t)k, "%s%s %zu", t ? ", " : "", lieux::nomType(t), lieux::lire(carte_, (lieux::Type)t).size());
        WFS_LOG("LIEUX %s : %s", carte_.c_str(), txt);
    }
    // WFS-261 (RG-02 à RG-05) : à chaque pas, la source la plus proche de chaque type (copie du tour du joueur), vue depuis la
    // tête du conducteur (position et cap lissés, comme la caméra) ; tunnel : section du tracé sous la voiture
    void sonsLieux(const GameView& g) {
        if (!lieuxActifs_ || !g.track || !g.player) {
            for (int t = 0; t < lieux::NB_TYPES; t++) { lieux_.cible(t, {}); aPortee_[t] = false; }
            tunnel_.dedans(false);
            return;
        }
        const Player& pl = *g.player; Track& t = *g.track;
        const TrackPoint P = t.pointLisse(pl.dist, pl.playerX * RW);
        const double cap = P.th + pl.heading;
        const vec3 ici{(float)P.x, (float)P.y + 1.2f, (float)P.z};
        const float rx = (float)std::cos(cap), rz = (float)std::sin(cap), fx = (float)std::sin(cap), fz = (float)-std::cos(cap);
        const int64_t aP = (int64_t)std::floor(pl.dist / SEG);
        for (int ty = 0; ty < lieux::NB_TYPES; ty++) {
            const std::vector<lieux::Lieu>& L = lieux::lire(carte_, (lieux::Type)ty);
            float best = 1e30f; vec3 q{}; int var = 0;
            for (const lieux::Lieu& l : L) {
                const vec3 p = lieux::positionMonde(t, l, aP), d = p - ici;
                const float d2 = dot(d, d);
                if (d2 < best) { best = d2; q = p; var = l.variante; }
            }
            const float dist = std::sqrt(best);
            const bool porte = !L.empty() && dist < lieux::FICHES[ty].portee;
            const vec3 d = q - ici;
            const float az = std::atan2(d.x * rx + d.z * rz, d.x * fx + d.z * fz);   // 0 devant, + à droite
            if (porte != aPortee_[ty]) {   // journal du banc : une ligne quand une source entre ou sort de sa portée
                aPortee_[ty] = porte;
                if (porte) WFS_LOG("LIEU %s à portée : %.0f m, %.0f° (%s)", lieux::nomType(ty), dist, az * 57.2958f, az > 0 ? "droite" : "gauche");
                else WFS_LOG("LIEU %s hors de portée", lieux::nomType(ty));
            }
            lieux::Gains gs = porte ? lieux::spatialiser(dist, az, lieux::FICHES[ty]) : lieux::Gains{};
            masquer(gs.gG, gs.gD, gs.coupure, masqueLieu_[ty], porte ? parois::masqueDb(ici_, q, proches_) : 0.f);   // WFS-275 (RG-06)
            lieux_.cible(ty, gs, var);
        }
        const trace::Section* s = trace::sectionA(t, pl.dist);
        const bool tun = s && s->type == trace::Type::Tunnel;
        if (tun != tunnelLog_) { tunnelLog_ = tun; WFS_LOG("TUNNEL %s", tun ? "entrée (écho)" : "sortie"); }
        tunnel_.dedans(tun);
    }
    // WFS-272 (RG-01, RG-02, RG-05) : voix des autres voitures. Candidats du pas (adversaires de la Course à 5 et du Duel, police
    // mobile, trafic visible), 8 voix au plus (Attribution), cibles calculées depuis la tête du conducteur ; RG-06 : sirène placée
    voitures::SonsVoitures voitures_; voitures::Attribution attribution_;
    std::vector<voitures::Candidat> candidats_;
    vec3 ici_{}, vIci_{}, avant_{0, 0, -1}, droite_{1, 0, 0};
    int voixLog_ = -1; double horlogeVoix_ = 1;
    int masquesLog_ = -1; double horlogeMasques_ = 1;   // WFS-278 : journal des voix cachées
    // voiture du trafic → fiche : même choix que l'affichage (traffic_system.cpp, nativeModelFor) : le même modèle s'il existe
    // parmi les TRAFIC_MODELES, sinon celui de longueur la plus proche
    static const CarDef& voitureDuTrafic(int type) {
        const WebCarDims& w = WEB_CARS[std::clamp(type, 0, WEB_CAR_COUNT - 1)];
        int best = 0; double bd = 1e9;
        for (int i = 0; i < TRAFIC_MODELES; i++) {
            if (std::strcmp(CARS[i].id, w.id) == 0) return CARS[i];
            const double d = std::abs(CARS[i].d.L - w.L);
            if (d < bd) { bd = d; best = i; }
        }
        return CARS[best];
    }
    void ecoute(const GameView& g) {   // tête du conducteur, vitesse et axes de sa voiture
        const Player& pl = *g.player;
        const TrackPoint P = g.track->pointLisse(pl.dist, pl.playerX * RW);
        const double cap = P.th + pl.heading;
        ici_ = {(float)P.x, (float)P.y + 1.2f, (float)P.z};
        avant_ = {(float)std::sin(cap), 0, (float)-std::cos(cap)}; droite_ = {(float)std::cos(cap), 0, (float)std::sin(cap)};
        vIci_ = avant_ * (float)(pl.speed * M);
    }
    voitures::Pan panTrafic(const GameView& g, int i) {   // RG-06 : gauche / droite de la voiture du trafic i (−1 : au centre)
        const Traffic* tr = activeTraffic();
        if (!tr || !g.track || !g.player || i < 0 || i >= (int)tr->cars.size()) return {};
        ecoute(g);
        const TrafficCar& c = tr->cars[(size_t)i];
        const TrackPoint q = g.track->pointLisse(g.player->dist + g.track->relZ(c.z - g.player->position), c.offset * RW);
        const vec3 d = vec3{(float)q.x, (float)q.y + 0.6f, (float)q.z} - ici_;
        return voitures::panExistant(std::atan2(dot(d, droite_), dot(d, avant_)));
    }
    // WFS-275 (RG-02 à RG-07) : écho des murs et masques
    parois::Echos echos_;
    std::vector<parois::Immeuble> immeubles_;            // obstacles des masques (repère de la route) : Ville (plan), autres cartes (WFS-278)
    std::vector<parois::ImmeubleMonde> proches_;         // immeubles à moins de 150 m, dans le monde (tour du joueur)
    parois::Suivi suiviParois_;                          // présence des parois (fondus des trouées), tunnel
    float masqueLieu_[lieux::NB_TYPES] = {}, masqueVoix_[voitures::VOIX] = {};
    uint64_t masqueId_[voitures::VOIX] = {};
    int parLog_ = -1;
    static void masquer(float& gG, float& gD, float& coupure, float& m, float cible) { parois::appliquerMasque(gG, gD, coupure, m, cible, 1.f / 120); }
    void preparerParois(const GameView& g) {
        proches_.clear();
        if (immeubles_.empty() || !g.track || !g.player) return;
        ecoute(g);
        Track& t = *g.track;
        const int64_t aP = (int64_t)std::floor(g.player->dist / SEG);
        const int64_t L = (int64_t)t.segmentCount();
        for (const parois::Immeuble& im : immeubles_) {
            const vec3 c = lieux::positionMonde(t, im.centre, aP);
            if (dot(c - ici_, c - ici_) > 150.f * 150.f) continue;
            const int64_t k = (int64_t)std::floor((double)(aP - im.centre.seg) / (double)L + 0.5);
            const double th = t.pose(im.centre.seg + k * L).th;
            proches_.push_back({c, {(float)std::cos(th), 0, (float)std::sin(th)}, {(float)std::sin(th), 0, (float)-std::cos(th)}, im.demiProf, im.demiLong, im.h});
        }
    }
    void sonsParois(double dt, const GameView& g) {
        const std::vector<parois::Cotes>& P = parois::lire(carte_);
        float a = 0, b = 0, alG = parois::ALPHA, alD = parois::ALPHA;   // distances de la tête aux parois (m, 0 : aucune), absorptions
        if (lieuxActifs_ && g.track && g.player && !P.empty()) {
            const int64_t n = (int64_t)P.size(), s = (((int64_t)std::floor(g.player->dist / SEG) % n) + n) % n;
            const float x = (float)(g.player->playerX * RW);   // + à droite
            const parois::Cotes& c = P[(size_t)s];
            if (c[0].d > 0) { a = std::max(0.5f, c[0].d + x); alG = c[0].alpha; }
            if (c[1].d > 0) { b = std::max(0.5f, c[1].d - x); alD = c[1].alpha; }
            if (a > parois::PORTEE) a = 0;
            if (b > parois::PORTEE) b = 0;
        }
        parois::Reflexion r[6]; float gG[6], gD[6];
        const int nr = suiviParois_.pas((float)dt, a, b, tunnelLog_, r, gG, gD, alG, alD);   // RG-05 : trouées, tunnel ; WFS-278 : matériaux
        for (int i = 0; i < parois::PRISES; i++) echos_.cible(i, i < nr ? r[i].delai : 0.01f, i < nr ? gG[i] : 0.f, i < nr ? gD[i] : 0.f);
        const int etat = (a > 0) + 2 * (b > 0);   // journal du banc : une ligne quand les côtés changent, au plus une par seconde
        horlogeParois_ += dt;
        if (etat != parLog_ && horlogeParois_ >= 1) { horlogeParois_ = 0; parLog_ = etat; WFS_LOG("PAROIS gauche %.0f m, droite %.0f m", a, b); }
    }
    double horlogeParois_ = 1;
    // WFS-288 : pluie de la Ville de nuit (même expression que sfx.cpp) ; essuie-glaces avancés à chaque pas avec les mêmes entrées
    // que l'image (camera_vue.cpp : dt, vitesse, pluie ; rien si ni joueur ni voiture) ; entendus en vue Cockpit (intérieur du modèle)
    pluie::SonsPluie pluie_; pluie::Surfaces surfacesPluie_; const CarDef* carPluie_ = nullptr; int pluieLog_ = -1;
    void sonsPluie(double dt, const GameView& g) {
        if (!g.player || !g.car) return;
        const char* mo = std::getenv("WFS_MOMENT");
        const bool pluie = meteo::pluieActive(g.map ? g.map->id : nullptr, mo && std::strcmp(mo, "nuit") == 0);
        const bool cockpit = VUES[(int)vueCamera()].cockpit, balais = cockpit && cockpit::interieurPour(g.car->id) != nullptr;
        if (carPluie_ != g.car) { carPluie_ = g.car; surfacesPluie_ = pluie::surfaces(g.car->d.L, g.car->d.W, g.car->d.H, g.car->d.wb); }
        pluie_.pas((float)dt, g.player->kmh(), pluie, cockpit, balais, tunnelLog_, surfacesPluie_);
        const int etat = pluie && lieuxActifs_ ? 1 + cockpit : 0;   // journal du banc : une ligne quand la pluie ou la vue change
        if (etat != pluieLog_) { pluieLog_ = etat; WFS_LOG("PLUIE %s", etat == 0 ? "muette" : etat == 1 ? "vue extérieure" : "vue Cockpit"); }
    }
    void sonsVoitures(double dt, const GameView& g) {
        candidats_.clear();
        const bool actif = lieuxActifs_ && g.track && g.player;
        if (actif) {
            const Player& pl = *g.player; Track& t = *g.track;
            ecoute(g);
            auto ajouter = [&](uint64_t id, double rd, double x, double v, const CarDef& car, uint8_t genre) {
                if (std::abs(rd - pl.dist) * M > 600) return;   // loin le long de la route (la portée est de 200 m au plus)
                const TrackPoint q = t.pointLisse(rd, x * RW);
                voitures::Candidat c; c.id = id; c.car = &car; c.genre = genre;
                c.p = {(float)q.x, (float)q.y + 0.6f, (float)q.z};
                c.v = vec3{(float)std::sin(q.th), 0, (float)-std::cos(q.th)} * (float)(v * M);
                c.kmh = (float)(std::abs(v) * M * 3.6);
                candidats_.push_back(c);
            };
            if (const auto* os = course5Ordinateurs())
                for (size_t i = 0; i < os->size(); i++) { const auto& o = (*os)[i]; ajouter((1ull << 56) | i, o.g.rd, o.g.x, o.g.v, carById(o.car.c_str()), voitures::ADVERSAIRE); }
            for (const auto& [nom, gh] : duelSession().ghosts())
                if (!gh.done) ajouter((2ull << 56) | (std::hash<std::string>{}(nom) & 0xFFFFFFFFFFFFFFull), gh.rd, gh.x, gh.v, carById(gh.car.c_str()), voitures::ADVERSAIRE);
            const char* pc = "p208";
            if (const auto* ps = poursuiteVoitures(&pc))
                for (size_t i = 0; i < ps->size(); i++) if (!(*ps)[i].fixe) ajouter((3ull << 56) | i, (*ps)[i].rd, (*ps)[i].x, (*ps)[i].v, carById(pc), voitures::POLICE);
            if (const Traffic* tr = activeTraffic())
                for (size_t i = 0; i < tr->cars.size(); i++) {
                    const TrafficCar& c = tr->cars[i];
                    if (!c.cache) ajouter((4ull << 56) | i, pl.dist + t.relZ(c.z - pl.position), c.offset, c.speed, voitureDuTrafic(c.type), voitures::TRAFIC);
                }
        }
        attribution_.update((float)dt, candidats_, ici_);
        int caches = 0; float plusFort = 0;   // WFS-278 : journal des voix cachées
        for (int v = 0; v < voitures::VOIX; v++) {
            voitures::Cible c = voitures::calculerCible(attribution_.voix(v), (float)dt, ici_, vIci_, avant_, droite_);
            const voitures::EtatVoix& e = attribution_.voix(v);   // WFS-275 (RG-06) : voiture cachée derrière un immeuble de la Ville
            const float cibleMasque = e.occupee ? parois::masqueDb(ici_, e.c.p, proches_) : 0.f;
            if (masqueId_[v] != e.id) { masqueId_[v] = e.id; masqueVoix_[v] = cibleMasque; }   // nouvelle voix : elle entre déjà en fondu
            masquer(c.gG, c.gD, c.coupure, masqueVoix_[v], cibleMasque);
            voitures_.cible(v, c);
            if (cibleMasque > 0) { caches++; plusFort = std::max(plusFort, cibleMasque); }
        }
        horlogeMasques_ += dt;   // au plus une ligne par seconde, quand le nombre de voix cachées change
        if (caches != masquesLog_ && horlogeMasques_ >= 1) { horlogeMasques_ = 0; masquesLog_ = caches; WFS_LOG("MASQUES %d voix cachée(s), la plus masquée %.1f dB", caches, plusFort); }
        horlogeVoix_ += dt;   // journal : au plus une ligne par seconde, quand le nombre de voix a changé
        if (attribution_.actives() != voixLog_ && horlogeVoix_ >= 1) { horlogeVoix_ = 0; voixLog_ = attribution_.actives(); WFS_LOG("VOITURES %d voix (%zu voitures autour)", voixLog_, candidats_.size()); }
        // RG-06 : sirène à gauche / à droite de la police mobile la plus proche ; sans police : au centre (comme avant)
        float best = 1e30f; voitures::Pan pan;
        for (const voitures::Candidat& c : candidats_)
            if (c.genre == voitures::POLICE) {
                const vec3 d = c.p - ici_;
                if (dot(d, d) < best) { best = dot(d, d); pan = voitures::panExistant(std::atan2(dot(d, droite_), dot(d, avant_))); }
            }
        sfx_.setPanSirene(pan.g, pan.d);
    }
    std::atomic<double> vivant_{0};
    float gVie_ = 0;   // thread audio seulement
    void renderInto(float* out, int n, double now) {
        const int a = active_;
        if (a < 0) { std::fill(out, out + n, 0.f); return; }
        synths_[a]->render(out, n, now);
    }
    std::unique_ptr<EngineSynth> synths_[SAMPLE_COUNT];
    std::atomic<int> active_{-1};
    EngineModel model_;
    EffetsMoteur effets_;   // WFS-120
    // WFS-127 : showroom sonore. Au garage, à l'ouverture et à chaque voiture montrée : démarreur (RG-03, pas pour la
    // model3), puis coup de gaz et ralenti (RG-04) ; un changement rapide repart de zéro (RG-05) ; à la sortie, la voiture
    // retenue reprend, sans démarreur (RG-06) ; journal du banc (RG-08)
    std::string montree_;
    float tGarage_ = 0, depuisDemarreur_ = 99;
    void showroom(double dt, const GameView& g, const char* montree, const EngineVoiceDef& voice, EngineSynth& s) {
        depuisDemarreur_ += (float)dt;
        if (!montree) {
            if (!montree_.empty()) { WFS_LOG("MOTEUR course %s (%s)", g.car->id, engineVoiceFor(g.car->id).sample); montree_.clear(); }
            return;
        }
        const bool electrique = std::strcmp(montree, "model3") == 0;
        if (montree_ != montree) {   // ouverture du garage ou autre voiture affichée
            montree_ = montree; tGarage_ = 0;
            WFS_LOG("MOTEUR garage %s (%s)", montree, voice.sample);
            if (!electrique && depuisDemarreur_ >= garageson::ECART_DEMARREUR) { sfx_.play(SfxId::Demarreur, 4.f); depuisDemarreur_ = 0; }
        }
        tGarage_ += (float)dt;
        const float t = electrique ? -1.f : tGarage_ - garageson::DEMARREUR;   // le coup de gaz part après le démarreur
        const garageson::Cibles c = garageson::cibles(garageson::regimeGarage(t), garageson::chargeGarage(t), voice.pitch);
        s.targets.rate = c.rate; s.targets.loGain = c.loGain; s.targets.loFreq = c.loFreq;
        s.targets.hiGain = c.hiGain; s.targets.hiFreq = c.hiFreq; s.targets.hiTau = c.hiTau;
        model_.rpm = garageson::regimeGarage(t);   // courbe du banc (moteur-courbe.png) cohérente avec ce qu'on entend
    }
    SfxMixer sfx_; SfxEvenements events_;   // WFS-28 : bruitages (mélangés au moteur dans le rappel audio)
};
WFS_SYSTEM(AudioSystem, 300)

// ---------------------------------------------------------------- banc d'essai
static void writeWav(const std::string& path, const std::vector<float>& pcm, int rate) {
    std::ofstream f(path, std::ios::binary);
    auto u32 = [&](uint32_t x) { f.write((const char*)&x, 4); };
    auto u16 = [&](uint16_t x) { f.write((const char*)&x, 2); };
    const uint32_t bytes = (uint32_t)pcm.size() * 2;
    f.write("RIFF", 4); u32(36 + bytes); f.write("WAVEfmt ", 8); u32(16); u16(1); u16(1); u32(rate); u32(rate * 2); u16(2); u16(16);
    f.write("data", 4); u32(bytes);
    for (float x : pcm) { const int16_t s = (int16_t)std::lround(std::clamp(x, -1.f, 1.f) * 32767); f.write((const char*)&s, 2); }
}

// courbe : forme d'onde (gris), régime (orange), rapport (cyan, en marches), vitesse (blanc), pédale (vert, en bas)
static void writeCurve(const std::string& path, const std::vector<float>& pcm, const std::vector<EngineFrame>& fr) {
    const int W = 1200, H = 520;
    std::vector<uint8_t> img((size_t)W * H * 3, 18);
    auto px = [&](int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        if (x < 0 || y < 0 || x >= W || y >= H) return;
        uint8_t* p = &img[((size_t)y * W + x) * 3]; p[0] = r; p[1] = g; p[2] = b;
    };
    // quadrillage : une ligne par seconde
    const float T = fr.empty() ? 1 : fr.back().t;
    for (int s = 0; s <= (int)T; s++) { const int x = (int)(s / T * (W - 1)); for (int y = 0; y < H; y += 3) px(x, y, 50, 50, 60); }
    // forme d'onde (enveloppe) : bande du haut
    const int wy = 110, wh = 90;
    for (int x = 0; x < W; x++) {
        const size_t a = pcm.size() * x / W, b = std::max(a + 1, pcm.size() * (x + 1) / W);
        float lo = 0, hi = 0;
        for (size_t i = a; i < b && i < pcm.size(); i++) { lo = std::min(lo, pcm[i]); hi = std::max(hi, pcm[i]); }
        for (int y = (int)(wy - hi * wh * 2.5f); y <= (int)(wy - lo * wh * 2.5f); y++) px(x, y, 150, 150, 160);
    }
    // courbes
    auto plot = [&](float t, float v01, int y0, int h, uint8_t r, uint8_t g, uint8_t b) {
        const int x = (int)(t / T * (W - 1)), y = y0 - (int)(std::clamp(v01, 0.f, 1.1f) * h);
        for (int d = -1; d <= 1; d++) { px(x, y + d, r, g, b); px(x + d, y, r, g, b); }
    };
    for (const auto& f : fr) {
        plot(f.t, f.rpm / 1.05f, 470, 230, 255, 120, 20);        // régime
        plot(f.t, (f.gear - 1) / 4.f, 470, 230, 0, 229, 255);    // rapport 1 à 5
        plot(f.t, f.kmh / 300.f, 470, 230, 240, 240, 240);       // vitesse (0-300 km/h)
        if (f.throttle) for (int y = 495; y < 510; y++) px((int)(f.t / T * (W - 1)), y, 60, 220, 90);   // pédale
    }
    stbi_write_png(path.c_str(), W, H, 3, img.data(), W * 3);
}

// WFS-131 (RG-09) : sirene.wav, écoutée par le QA : une police arrive de 150 m (2 s), colle 2 s à 10 m (yelp), double
// le joueur et s'éloigne (Doppler : plus aiguë en approchant, plus grave après) ; mixeur à part, mêmes décisions que la partie.
// WFS-184 (ANO-95) : puis deux fins de poursuite : police à 5 m → « ARRÊTÉ » (t = 8 s), police à 14 m → « ÉCHAPPÉ » (t = 11 s)
static void benchSirene(const std::string& out) {
    SfxMixer m; m.build((float)OUT_RATE); m.lastUpdate = 1e9;   // hors ligne : pas de « pause » détectée
    SireneSon dec; std::vector<float> pcm; double acc = 0;
    constexpr double DT = 1.0 / 120, DX = 3.5;   // pas du jeu ; la police roule dans la file voisine (m)
    int seconde = -1;
    // WFS-184 : fins de poursuite (début de la police collée, instant de la fin, distance, jingle)
    struct FinBanc { double debut, fin, distanceM; SfxId jingle; const char* nom; };
    const FinBanc fins[] = {{6, 8, 5, SfxId::PoursuiteArrete, "ARRÊTÉ"}, {9, 11, 14, SfxId::PoursuiteEchappe, "ÉCHAPPÉ"}};
    for (int i = 0; i < 13 * 120; i++) {
        const double t = i * DT;
        double dz, v;   // écart police − joueur le long de la route (m, < 0 : derrière) ; vitesse de rapprochement (m/s)
        if (t < 2) { dz = -150 + 70 * t; v = 70; }                 // arrive de 150 m à 10 m
        else if (t < 4) { dz = -10; v = 0; }                         // colle à 10 m
        else { dz = -10 + 35 * (t - 4); v = dz < 0 ? 35 : -35; }     // double à 35 m/s, puis s'éloigne
        bool enJeu = t < 6, fin = false, appel = t < 6;   // de 12 à 13 s : plus de poursuite (aucun appel)
        for (const FinBanc& f : fins) if (t >= f.debut && t < f.debut + 3) {
            dz = -f.distanceM; v = 0; enJeu = t < f.fin; fin = !enJeu; appel = true;
            if (i == (int)std::lround(f.fin / DT)) {   // même ordre que la partie : jingle demandé au pas où la course finit
                m.play(f.jingle);
                WFS_LOG("son (WFS-184) : sirene.wav t=%.0fs : fin de poursuite « %s », police à %.0f m", f.fin, f.nom, f.distanceM);
            }
        }
        const CiblesSirene c = dec.update(DT, appel, appel ? std::hypot(dz, DX) : sirene::AUCUNE, v, enJeu, fin);
        m.setSirene(c.gain, c.yelp, c.doppler, c.fin);
        if ((int)t != seconde) {
            seconde = (int)t;
            WFS_LOG("son (WFS-131) : sirene.wav t=%ds : %s gain %.2f doppler %.2f police %.0f m", seconde, c.yelp ? "yelp" : "wail", c.gain, c.doppler, c.distanceM);
        }
        acc += DT * OUT_RATE; const int n = (int)acc; acc -= n;
        const size_t at = pcm.size(); pcm.resize(at + (size_t)n, 0.f);
        m.mix(pcm.data() + at, n, 1e9);
    }
    writeWav(out + "/sirene.wav", pcm, OUT_RATE);
    float crete = 0, saut = 0;
    for (size_t i = 1; i < pcm.size(); i++) { crete = std::max(crete, std::abs(pcm[i])); saut = std::max(saut, std::abs(pcm[i] - pcm[i - 1])); }
    WFS_LOG("son (WFS-131) : sirene.wav : %.1f s, crête %.2f, plus grand saut %.3f", pcm.size() / (double)OUT_RATE, crete, saut);
}

void benchAudioScene(Game& g, Renderer& r, const std::string& out) {
    // scénario : ralenti 2 s, plein gaz 14 s (passages de rapports), lever de pied 4 s, freinage 3 s, gaz 5 s
    g.setAutopilot(false);
    g.init(r, "golf", "ville");
    g_rec = BenchRec{}; g_rec.on = true;
    const float W = (float)r.width(), H = (float)r.height();
    const Touch gasDown{7, W * 0.89f, H * 0.74f, true}, gasUp{7, W * 0.89f, H * 0.74f, false};
    const Touch brDown{8, W * 0.70f, H * 0.80f, true}, brUp{8, W * 0.70f, H * 0.80f, false};
    struct Phase { double until; int what; };   // 0 rien, 1 gaz, 2 frein
    const Phase phases[] = {{2, 0}, {16, 1}, {20, 0}, {23, 2}, {28, 1}};
    int cur = -1;
    g.onTouch({9, W * 0.18f, H * 0.72f, true});   // point de départ du volant
    for (int i = 0; i < 28 * 60; i++) {
        const double t = i / 60.0;
        int p = 0; while (p < 4 && t >= phases[p].until) p++;
        if (phases[p].what != cur) {
            if (cur == 1) g.onTouch(gasUp);
            if (cur == 2) g.onTouch(brUp);
            cur = phases[p].what;
            if (cur == 1) g.onTouch(gasDown);
            if (cur == 2) g.onTouch(brDown);
        }
        // volant : reste au milieu de la route (sinon la voiture dérive dans les virages et sort de la route)
        const Player& pl = g.player();
        const float st = clampf((float)((0.0 - pl.playerX) * 3 - pl.heading * 4), -1.f, 1.f);
        g.onTouch({9, W * 0.18f + st * W * 0.12f, H * 0.72f, true});
        g.update(1.0 / 60);
    }
    g.onTouch({9, W * 0.18f, H * 0.72f, false});
    g_rec.on = false;
    writeWav(out + "/moteur.wav", g_rec.pcm, OUT_RATE);
    writeCurve(out + "/moteur-courbe.png", g_rec.pcm, g_rec.frames);
    // une ligne par seconde (journal de la CI) + crête / craquements : saut d'un échantillon à l'autre
    float peak = 0, jump = 0;
    for (size_t i = 1; i < g_rec.pcm.size(); i++) { peak = std::max(peak, std::abs(g_rec.pcm[i])); jump = std::max(jump, std::abs(g_rec.pcm[i] - g_rec.pcm[i - 1])); }
    int lastS = -1;
    for (const auto& f : g_rec.frames) if ((int)f.t != lastS) {
        lastS = (int)f.t;
        WFS_LOG("son t=%2ds : %3.0f km/h, rapport %d, régime %.2f, charge %.2f, pédale %s", lastS, f.kmh, f.gear, f.rpm, f.load, f.throttle ? "oui" : "non");
    }
    WFS_LOG("son : %.1f s enregistrées (%zu échantillons), crête %.2f, plus grand saut %.3f", g_rec.pcm.size() / (double)OUT_RATE, g_rec.pcm.size(), peak, jump);
    {   // WFS-174 (RG-08) : le moteur reste au-dessus des bruitages (sons de la route compris) ; énergie des bruitages = total − moteur
        double e2Total = 0; for (float x : g_rec.pcm) e2Total += (double)x * x;
        const double nEch = std::max<double>(1, (double)g_rec.pcm.size());
        const double rmsM = std::sqrt(g_rec.e2Moteur / nEch), rmsS = std::sqrt(std::max(0.0, e2Total - g_rec.e2Moteur) / nEch);
        WFS_LOG("son : RMS moteur %.3f, RMS bruitages (route comprise) %.3f → moteur %s", rmsM, rmsS, rmsM >= rmsS ? "au-dessus" : "EN DESSOUS");
    }
    {   // WFS-121 (critère « coller ») : le joueur se place 8 m derrière la voiture la plus entourée, à sa vitesse, et la
        // suit 3 s dans sa file en dosant l'accélérateur (écart pare-chocs à pare-chocs gardé vers 3 m) → « SFX klaxon »
        trafficBenchPlace(8, 0, 0, false);
        g.update(1.0 / 60);
        const int idx = trafficBenchCar();
        const Traffic* tr = activeTraffic();
        bool gaz = false; double ecartMin = 1e9, ecartMax = 0;
        for (int i = 0; tr && idx >= 0 && idx < (int)tr->cars.size() && i < 3 * 60; i++) {
            const Player& pl = g.player(); const TrafficCar& c = tr->cars[(size_t)idx];
            const double ecart = c.prevDz * M - (c.len * M + webCarLength(pl.car->id, pl.car->d.L) * M) / 2;
            ecartMin = std::min(ecartMin, ecart); ecartMax = std::max(ecartMax, ecart);
            const bool veut = ecart > 3 || pl.speed < c.speed * 0.97;
            if (veut != gaz) { g.onTouch(veut ? gasDown : gasUp); gaz = veut; }
            const float st = clampf((float)((c.offset - pl.playerX) * 3 - pl.heading * 4), -1.f, 1.f);
            g.onTouch({9, W * 0.18f + st * W * 0.12f, H * 0.72f, true});
            g.update(1.0 / 60);
        }
        if (gaz) g.onTouch(gasUp);
        g.onTouch({9, W * 0.18f, H * 0.72f, false});
        WFS_LOG("son (WFS-121) : collage 3 s derrière la voiture %d, écart pare-chocs %.1f à %.1f m", idx, ecartMin, ecartMax);
    }
    g.shutdown();
    benchMusique(out);   // WFS-76 : musique-menu / musique-course / ambiance-<carte> (.wav + .png)
    benchSirene(out);    // WFS-131 : sirene.wav
    lieux::benchLieux(out, g_rec.pcm);   // WFS-261 : lieu-<source>.wav / .png (stéréo), lieu-tunnel, coût
    voitures::benchVoitures(out);        // WFS-272 : voitures-course5, voitures-trafic, voitures-police (stéréo, hauteur), coût
    parois::benchParois(out, g_rec.pcm); // WFS-275 et WFS-278 : parois-*, masque-* (ville, corniche, campagne, stands), coût
    pluie::benchPluie(out, g_rec.pcm);   // WFS-288 : pluie-arriere, pluie-cockpit, pluie-tunnel, coût
}

} // namespace wfs
