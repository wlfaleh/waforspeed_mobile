#include "game.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>   // setenv (WFS-41)
#include <cstring>
#include "../core/jobs.h"
#include "../core/log.h"
#include "../core/platform.h"   // nowSeconds (WFS-74)
#include "../render/model_loader.h"
#include "course_system.h"   // WFS-12 : ralenti après un accident
#include "camera_vue.h"      // WFS-66 : vues poursuite, capot, cockpit

namespace wfs {

static constexpr int CHUNKS_BEHIND = 2, CHUNKS_AHEAD = 22;
static constexpr float WALK_W = 4.0f, CURB_H = 0.15f, GROUND_W = 260.f;

static vec3 hexColor(unsigned h) {
    auto lin = [](float c) { return std::pow(c, 2.2f); };
    return {lin(((h >> 16) & 255) / 255.f), lin(((h >> 8) & 255) / 255.f), lin((h & 255) / 255.f)};
}

bool Game::init(Renderer& r, const char* carId, const char* mapId, void (*progression)(float, const char*)) {
    r_ = &r;
    if (r.gpuLogiciel()) {   // WFS-59 : émulateur de la QA → ombres du soleil coupées (dt et rendu : plafondDt, vk.cpp)
        r.forceNoShadows = true;
        WFS_LOG("mode GPU logiciel : actif (dt max 0,5 s, rendu 25 %%, sans ombres)");
    }
    ecran_ = ecranDemande_ = Ecran::Course; demandeEcran_ = false;   // écran de départ (Menu quand WFS-37 existera)
    lacherCommandes();
    if (progression) progression(0.10f, "Construction de la route");   // WFS-35
    track_.build(mapById(mapId));
    Material m;
    m.flags = MAT_ROAD; m.rough = 0.8f; roadMat_ = r.makeMaterial(m, nullptr, nullptr, nullptr);
    m.flags = MAT_WALK; m.rough = 0.85f; walkMat_ = r.makeMaterial(m, nullptr, nullptr, nullptr);
    m.flags = MAT_GROUND; m.rough = 0.95f; groundMat_ = r.makeMaterial(m, nullptr, nullptr, nullptr);
    m.flags = MAT_SHADOWBLOB | MAT_BLEND; m.color = {0, 0, 0, 0.75f}; shadowMat_ = r.makeMaterial(m, nullptr, nullptr, nullptr);
    {
        // carré 1x1 au sol (uv 0..1), légèrement surélevé
        std::vector<Vertex> v = {{-0.5f, 0.02f, -0.5f, 0, 1, 0, 0, 0}, {0.5f, 0.02f, -0.5f, 0, 1, 0, 1, 0},
                                 {0.5f, 0.02f, 0.5f, 0, 1, 0, 1, 1}, {-0.5f, 0.02f, 0.5f, 0, 1, 0, 0, 1}};
        shadowQuad_ = r.makeMesh(v, {0, 1, 2, 0, 2, 3});
    }
    if (progression) progression(0.20f, "Chargement du modèle 3D…");   // WFS-35
    setCar(carId);
    if (progression && !car_.ok) return false;   // WFS-35 : voiture introuvable, l'écran garde l'étape où l'erreur a eu lieu
    if (progression) progression(0.70f, "Construction de la ville");   // WFS-35
    systemsInit();
    return car_.ok;
}

void Game::setCar(const char* carId) {
    if (car_.ok) { r_->vk().waitIdle(); r_->destroy(car_); }
    carDef_ = &carById(carId);
    paint_ = hexColor(carDef_->paint);
    loadCarModel(*r_, std::string(carDef_->file), car_);
    player_.reset(*carDef_);
    camInit_ = false;
}

void Game::shutdown() {
    if (!r_) return;
    r_->vk().waitIdle();
    systemsShutdown();
    for (auto& [k, c] : chunks_) { r_->destroy(c.road); r_->destroy(c.walk); r_->destroy(c.ground); }
    chunks_.clear();
    for (auto& g : graveyard_) r_->destroy(g.second);
    graveyard_.clear();
    r_->destroy(car_);
    r_->destroy(shadowQuad_);
    r_ = nullptr;
}

// ---------------------------------------------------------------- route par morceaux
// Chaque morceau couvre CH segments (38,4 m) : chaussée, bordures + trottoirs, bas-côtés.
// uv = mètres (x latéral, y distance le long de la route) : les shaders dessinent l'asphalte et les marquages.
void Game::buildChunkCpu(int64_t c, ChunkCpu& out) {
    const int64_t a0 = c * CH;
    const double halfRoad = RW;
    // profils transversaux : (latéral en m, hauteur)
    auto strip = [&](int which, const std::vector<std::pair<float, float>>& prof) {
        auto& V = out.v[which]; auto& I = out.i[which];
        const uint32_t cols = (uint32_t)prof.size(), base = (uint32_t)V.size();
        for (int i = 0; i <= CH; i++) {
            const double s = (double)(a0 + i) * SEG;
            for (uint32_t k = 0; k < cols; k++) {
                TrackPoint p = track_.point(s, prof[k].first);
                V.push_back({(float)p.x, (float)(p.y + prof[k].second), (float)p.z, 0, 1, 0, prof[k].first, (float)((a0 + i) * SEG_M)});
            }
        }
        for (int i = 0; i < CH; i++) for (uint32_t k = 0; k + 1 < cols; k++) {
            uint32_t q = base + i * cols + k;
            I.insert(I.end(), {q, q + cols, q + 1, q + 1, q + cols, q + cols + 1});
        }
    };
    // chaussée : 7 colonnes pour suivre le dévers
    std::vector<std::pair<float, float>> road;
    for (int k = 0; k <= 6; k++) road.push_back({(float)(-halfRoad + 2 * halfRoad * k / 6.0), 0.f});
    strip(0, road);
    // trottoirs (avec la face verticale de la bordure)
    const float R = (float)halfRoad;
    strip(1, {{-R - WALK_W, CURB_H}, {-R - 0.12f, CURB_H}, {-R, CURB_H - 0.02f}, {-R, 0.f}});
    strip(1, {{R, 0.f}, {R, CURB_H - 0.02f}, {R + 0.12f, CURB_H}, {R + WALK_W, CURB_H}});
    // bas-côtés
    strip(2, {{-R - WALK_W - GROUND_W, -6.f}, {-R - WALK_W - 80.f, -0.8f}, {-R - WALK_W - 20.f, 0.f}, {-R - WALK_W, CURB_H - 0.02f}});
    strip(2, {{R + WALK_W, CURB_H - 0.02f}, {R + WALK_W + 20.f, 0.f}, {R + WALK_W + 80.f, -0.8f}, {R + WALK_W + GROUND_W, -6.f}});

    // normales : produit vectoriel des côtés de chaque triangle, moyenné par sommet
    for (int w = 0; w < 3; w++) {
        auto& V = out.v[w]; auto& I = out.i[w];
        for (auto& v : V) { v.nx = v.ny = v.nz = 0; }
        for (size_t t = 0; t + 2 < I.size(); t += 3) {
            Vertex &A = V[I[t]], &B = V[I[t + 1]], &C = V[I[t + 2]];
            vec3 e1{B.px - A.px, B.py - A.py, B.pz - A.pz}, e2{C.px - A.px, C.py - A.py, C.pz - A.pz};
            vec3 n = cross(e1, e2);
            if (n.y < 0 && std::fabs(n.y) > 0.3f * length(n)) n = -n;   // vers le haut pour les surfaces horizontales
            for (Vertex* x : {&A, &B, &C}) { x->nx += n.x; x->ny += n.y; x->nz += n.z; }
        }
        for (auto& v : V) { vec3 n = normalize({v.nx, v.ny, v.nz}); if (length(n) < 0.5f) n = {0, 1, 0}; v.nx = n.x; v.ny = n.y; v.nz = n.z; }
    }
}

void Game::retire(Mesh& m) { if (m.indexCount || m.vb.buf) graveyard_.push_back({frameNo_, m}); m = Mesh{}; }

void Game::streamChunks() {
    const int64_t pc = (int64_t)std::floor(player_.dist / SEG) / CH;
    // morceaux trop loin : libérés quand le GPU ne les utilise plus
    for (auto it = chunks_.begin(); it != chunks_.end();) {
        if (it->first < pc - CHUNKS_BEHIND || it->first > pc + CHUNKS_AHEAD) {
            retire(it->second.road); retire(it->second.walk); retire(it->second.ground);
            it = chunks_.erase(it);
        } else ++it;
    }
    std::vector<int64_t> missing;
    for (int64_t c = pc - CHUNKS_BEHIND; c <= pc + CHUNKS_AHEAD; c++) if (!chunks_.count(c)) missing.push_back(c);
    if (!missing.empty()) {
        // les poses de la piste sont calculées ici (thread principal) ; les threads ne font que les lire
        track_.pose(missing.front() * CH - 1); track_.pose((missing.back() + 1) * CH + 1);
        std::vector<ChunkCpu> cpu(missing.size());
        jobs().parallelFor((int)missing.size(), 1, [&](int b, int e) { for (int i = b; i < e; i++) buildChunkCpu(missing[i], cpu[i]); });
        for (size_t i = 0; i < missing.size(); i++) {
            Chunk& ch = chunks_[missing[i]];
            ch.road = r_->makeMesh(cpu[i].v[0], cpu[i].i[0], true);
            ch.walk = r_->makeMesh(cpu[i].v[1], cpu[i].i[1], true);
            ch.ground = r_->makeMesh(cpu[i].v[2], cpu[i].i[2], true);
            ch.ready = true;
        }
    }
    // destruction différée (3 images plus tard)
    for (auto it = graveyard_.begin(); it != graveyard_.end();) {
        if (frameNo_ > it->first + 3) { r_->destroy(it->second); it = graveyard_.erase(it); } else ++it;
    }
}

// ---------------------------------------------------------------- commandes tactiles
// gauche : glisser pour braquer ; droite : frein et accélérateur ; au-dessus de l'accélérateur : nitro
void Game::onTouch(const Touch& t) {
    if (systemsTouch(t)) return;
    if (ecran_ != Ecran::Course) return;   // hors course : les touchers ne pilotent pas la voiture (WFS-36)
    const float W = (float)r_->width(), H = (float)r_->height();
    const float mg = marges_.gauche, md = marges_.droite;   // zones de toucher décalées des marges (WFS-51)
    if (t.down) {
        bool fresh = t.id != steerId_ && t.id != gasId_ && t.id != brakeId_ && t.id != nitroId_;
        if (fresh) {
            if (t.x < mg + W * 0.45f) { steerId_ = t.id; steerX0_ = steerX_ = t.x; }
            else if (t.x > W * 0.80f - md && t.y < H * 0.45f) nitroId_ = t.id;
            else if (t.x > W * 0.78f - md) gasId_ = t.id;
            else if (t.x > W * 0.55f - md) brakeId_ = t.id;
        }
        if (t.id == steerId_) steerX_ = t.x;
    } else {
        if (t.id == steerId_) steerId_ = -1;
        if (t.id == gasId_) gasId_ = -1;
        if (t.id == brakeId_) brakeId_ = -1;
        if (t.id == nitroId_) nitroId_ = -1;
    }
    // un appui bref (appui et relâchement entre deux images) compte quand même pour au moins une mise à jour
    if (t.down && t.id == gasId_) latchGas_ = true;
    if (t.down && t.id == brakeId_) latchBrake_ = true;
    if (t.down && t.id == nitroId_) latchNitro_ = true;
    input_.steer = steerId_ >= 0 ? clampf((steerX_ - steerX0_) / (W * 0.12f), -1.f, 1.f) : 0.f;
    input_.gas = gasId_ >= 0 || nitroId_ >= 0;
    input_.brake = brakeId_ >= 0;
    input_.nitro = nitroId_ >= 0;
}

void Game::drawControls() {
    const float W = (float)r_->width(), H = (float)r_->height(), u = H / 100.f;
    // marges de sécurité (WFS-51) : à gauche le volant et les jauges, à droite les pédales, en bas tout le monde
    const float mg = marges_.gauche, mh = marges_.haut, md = marges_.droite, mb = marges_.bas;
    if (montrerMarges) {   // banc d'essai : zone interdite marquée (encoche simulée)
        if (mg > 0) r_->uiRect(0, 0, mg, H, {1, 0.1f, 0.2f, 0.35f});
        if (md > 0) r_->uiRect(W - md, 0, md, H, {1, 0.1f, 0.2f, 0.35f});
    }
    // volant virtuel
    if (steerId_ >= 0) {
        r_->uiCircle(steerX0_, H * 0.72f - mb, 11 * u, {1, 1, 1, 0.12f});
        r_->uiCircle(steerX0_ + clampf(steerX_ - steerX0_, -W * 0.12f, W * 0.12f), H * 0.72f - mb, 5 * u, {0, 0.9f, 1, 0.55f});
    } else r_->uiCircle(W * 0.18f + mg, H * 0.72f - mb, 9 * u, {1, 1, 1, 0.08f});
    // pédales
    r_->uiCircle(W * 0.89f - md, H * 0.74f - mb, 11 * u, input_.gas ? vec4{0.1f, 1, 0.45f, 0.6f} : vec4{0.1f, 1, 0.45f, 0.22f});
    r_->uiCircle(W * 0.70f - md, H * 0.80f - mb, 8.5f * u, input_.brake ? vec4{1, 0.2f, 0.25f, 0.6f} : vec4{1, 0.2f, 0.25f, 0.22f});
    r_->uiCircle(W * 0.90f - md, H * 0.32f, 6.5f * u, input_.nitro ? vec4{0.2f, 0.85f, 1, 0.7f} : vec4{0.2f, 0.85f, 1, 0.25f});
    // vitesse, nitro, temps, score : HUD de course (WFS-27, src/ui/hud.cpp ; marges : WFS-52)
}

// ---------------------------------------------------------------- mise à jour
Game::Demandes& Game::demandes() { static Demandes d; return d; }

void Game::update(double dt) {
    if (!demandes().carte.empty() && r_) changerCarte();   // WFS-41 / WFS-74 : entre deux images, jamais au milieu
    dt = courseScaleDt(plafondDt(dt));   // WFS-12 : ralenti ×0,35 après un accident ; WFS-59 : 0,05 s (0,5 s en GPU logiciel)
    appliquerEcran();
    if (ecran_ == Ecran::Pause) { acc_ = 0; return; }   // pause : simulation figée
    const bool course = ecran_ == Ecran::Course;
    DriveInput in = course ? input_ : DriveInput{};     // hors course : aucune commande de conduite
    in.gas = in.gas || latchGas_; in.brake = in.brake || latchBrake_; in.nitro = in.nitro || latchNitro_;
    if (latchNitro_) in.gas = true;
    latchGas_ = latchBrake_ = latchNitro_ = false;
    if (autopilot_ && course) {   // démonstration / tests : plein gaz, reste au milieu de sa voie
        in.gas = true; in.brake = false;
        in.steer = (float)clampf((float)((0.0 - player_.playerX) * 3.0 - player_.heading * 4.0), -1.f, 1.f);
        for (auto& sy : systems_) sy->autopilot(dt, in, view_);   // ex. : éviter le trafic (WFS-08)
    }
    // pas fixes de 1/120 s : la conduite est identique quelle que soit la fréquence d'affichage
    acc_ += dt;
    const double step = 1.0 / 120.0;
    for (int pas = 0; acc_ >= step && pas < gpu_logiciel::PAS_MAX; ++pas) {   // WFS-59 : 60 pas au plus par image
        if (!course) player_.speed = 0;   // hors course : voiture immobile, la scène continue de vivre derrière
        player_.update(step, in, track_, true); for (auto& sy : systems_) sy->collide(step, player_, view_); systemsUpdate(step); acc_ -= step; }
}

mat4 Game::carMatrix(double s, double lat, double yaw) {
    TrackPoint p = track_.point(s, lat);
    return translate({(float)p.x, (float)p.y, (float)p.z}) * rotateY((float)-(p.th + yaw)) * rotateX((float)p.slope);
}

void Game::render(double time) {
    frameNo_++;
    appliquerEcran();
    struct LastT { double& t; double v; ~LastT() { t = v; } } keepTime{lastTime_, time};
    streamChunks();
    const double s = player_.dist, lat = player_.playerX * RW;
    mat4 carM = carMatrix(s, lat, player_.heading + player_.spinYaw);   // + tête-à-queue après un accident
    vec3 carPos = carM.transformPoint({0, 0, 0});
    vec3 fwd = normalize(carM.transformDir({0, 0, -1}));

    // ---------- caméra ----------
    vec3 eye, look;
    if (camMode == 2) {   // vitrine : on tourne autour de la voiture
        float a = orbit;
        eye = carPos + vec3{std::sin(a) * 6.5f, 1.7f, std::cos(a) * 6.5f};
        look = carPos + vec3{0, 0.55f, 0};
        camInit_ = false;
    } else {
        const float back = camMode == 0 ? 5.2f : 8.0f, up = camMode == 0 ? 1.55f : 2.6f;
        // la caméra suit le cap de la route (pas le dérapage) avec un peu de retard
        TrackPoint behind = track_.point(s - back / M, lat * 0.85);
        vec3 eyeT = vec3{(float)behind.x, (float)behind.y + up, (float)behind.z};
        vec3 lookT = carPos + fwd * 4.0f + vec3{0, 0.8f, 0};
        // lissage indépendant de la fréquence d'affichage ; si la caméra est trop loin (téléportation), on la recale
        const float fdt = (float)std::clamp(time - lastTime_, 0.0, 0.1);
        if (!camInit_ || length(camEye_ - eyeT) > 25.f) { camEye_ = eyeT; camLook_ = lookT; camInit_ = true; }
        camEye_ = lerp(camEye_, eyeT, 1.f - std::exp(-fdt * 14.f)); camLook_ = lerp(camLook_, lookT, 1.f - std::exp(-fdt * 20.f));
        eye = camEye_; look = camLook_;
    }
    float aspect = (float)r_->width() / (float)std::max(1u, r_->height());
    float fov = (camMode == 0 ? 58.f : 62.f) + (float)std::min(player_.kmh() / 300.0, 1.0) * 8.f;
    const bool vueInterieure = cameraVue(track_, player_, *carDef_, carM, camMode, ecran_, (float)std::clamp(time - lastTime_, 0.0, 0.1), eye, look, fov);   // WFS-66

    FrameParams fp;
    fp.view = lookAt(eye, look, {0, 1, 0});
    fp.proj = perspectiveVk(fov * PI / 180.f, aspect, 0.1f);
    fp.camPos = eye; fp.time = (float)time;
    fp.sunDir = {-0.45f, 0.75f, -0.5f}; fp.sunIntensity = 3.2f; fp.sunColor = {1.0f, 0.93f, 0.82f};   // soleil du jeu web (envFor, jour)
    fp.focus = carPos; fp.hasFocus = true;   // WFS-07 : la boîte d'ombre suit la voiture
    fp.skyTop = {0.16f, 0.34f, 0.75f}; fp.skyHorizon = {0.66f, 0.76f, 0.88f}; fp.ground = {0.2f, 0.19f, 0.17f};
    fp.fogColor = {0.55f, 0.63f, 0.74f}; fp.fogDensity = 0.0009f;
    fp.exposure = 0.85f; fp.ambient = 0.85f;
    // scène dédiée (WFS-44) : le premier système qui la prend règle fp et choisit ce que Game dessine encore
    bool solPropre = false;   // WFS-14 : ambiance et bas-côtés propres à la carte
    for (auto& sy : systems_) { sy->ambiance(fp, view_); solPropre = solPropre || sy->remplaceSol(view_); }
    SceneDediee sd; sd.cible = look;
    bool dediee = false;
    for (auto& sy : systems_) if (sy->scene(fp, sd, view_)) { dediee = true; break; }
    view_.sceneDediee = dediee;
    if (dediee) { eye = fp.camPos; look = sd.cible; }
    if (!r_->beginFrame(fp)) return;

    const mat4 I = mat4::identity();
    if (!dediee || sd.route) for (auto& [k, c] : chunks_) {
        if (!solPropre) r_->draw(c.ground, groundMat_, I);
        r_->draw(c.walk, walkMat_, I);
        r_->draw(c.road, roadMat_, I, (float)RW);
    }
    systemsRender(eye, look, fp.view, fp.proj, time);
    if ((!dediee || sd.joueur) && !vueInterieure) {   // WFS-66 : vue cockpit sans la voiture du joueur
        // ombre de contact douce sous la voiture (avant les ombres dynamiques F7)
        r_->draw(shadowQuad_, shadowMat_, carM * scale({carDef_->d.W * 1.25f, 1.f, carDef_->d.L * 1.12f}));
        r_->draw(car_, carM, &paint_);
    }
    if (camMode != 2 && ecran_ == Ecran::Course && (!dediee || sd.commandes)) drawControls();   // commandes de conduite : seulement en course
    systemsUi();
    r_->endFrame();
}

// ---------------------------------------------------------------- changement de carte (WFS-41, WFS-74)
// « Rouler » de l'écran Terrain : course neuve sur la carte et le moment choisis. Même carte et même moment : pas de
// reconstruction, la course repart à zéro (Terrain → Course, comme « Rejouer »). Sinon : écran de chargement de la
// plateforme (si branché), partie reconstruite (étapes réelles de init), voiture et peinture du garage gardées.
void Game::changerCarte() {
    Demandes& d = demandes();
    const std::string c = d.carte, v = carDef_->id;
    const bool nuit = d.nuit;
    d.carte.clear();
    lacherCommandes();   // RG-07 : aucune commande tenue pendant le chargement
    const char* m = std::getenv("WFS_MOMENT");
    const bool nuitActuelle = m && std::strcmp(m, "nuit") == 0;
    if (track_.map() && c == track_.map()->id && nuit == nuitActuelle) {   // RG-02
        WFS_LOG("terrain : carte = %s, %s, déjà chargée (pas de reconstruction)", c.c_str(), nuit ? "nuit" : "jour");
        allerA(Ecran::Course);
        return;
    }
    const vec3 p = paint_;
    Renderer& r = *r_;
    const double t0 = nowSeconds();
    setenv("WFS_MOMENT", nuit ? "nuit" : "jour", 1);
    shutdown();
    // après shutdown : le système « texte » vient de libérer la police, l'écran de chargement la recharge (logo et étape lisibles)
    if (d.debutChargement) d.debutChargement();
    const bool ok = init(r, v.c_str(), c.c_str(), d.progression);
    paint_ = p;   // RG-06
    if (d.finChargement) d.finChargement(ok);
    if (ok) WFS_LOG("terrain : carte = %s, %s, chargée en %d ms", c.c_str(), nuit ? "nuit" : "jour", (int)std::lround((nowSeconds() - t0) * 1000));
    else WFS_ERR("terrain : carte = %s, chargement impossible", c.c_str());
}

// ---------------------------------------------------------------- écrans (WFS-36)
// Le changement demandé (Game::allerA ou GameView::allerA) est appliqué ici, au début de update() / render() : jamais au
// milieu d'une image. Les commandes de conduite en cours sont lâchées, puis tous les systèmes sont prévenus.
void Game::appliquerEcran() {
    if (!voitureDemandee_.empty()) { setCar(voitureDemandee_.c_str()); paint_ = peintureDemandee_; voitureDemandee_.clear(); }   // WFS-11
    if (!demandeEcran_) return;
    demandeEcran_ = false;
    if (ecranDemande_ == ecran_) return;
    const Ecran ancien = ecran_;
    ecran_ = ecranDemande_;
    view_.ecran = ecran_;
    lacherCommandes();
    WFS_LOG("Écran : %s → %s", nomEcran(ancien), nomEcran(ecran_));
    for (auto& s : systems_) s->onEcran(ancien, ecran_, view_);
}

// toutes les commandes de conduite relâchées (web : for (const k in input) input[k] = false)
void Game::lacherCommandes() {
    input_ = DriveInput{};
    steerId_ = gasId_ = brakeId_ = nitroId_ = -1;
    latchGas_ = latchBrake_ = latchNitro_ = false;
}

// ---------------------------------------------------------------- systèmes enfichables (WFS-30)
// Tous les systèmes enregistrés par WFS_SYSTEM sont créés au démarrage de la partie, dans l'ordre croissant.
void Game::systemsInit() {
    systemsShutdown();
    auto entries = systemRegistry();
    std::stable_sort(entries.begin(), entries.end(), [](const SystemEntry& a, const SystemEntry& b) { return a.order < b.order; });
    view_ = GameView{};
    view_.track = &track_; view_.player = &player_; view_.car = carDef_; view_.map = track_.map();
    view_.uiWidth = (float)r_->width(); view_.uiHeight = (float)r_->height();
    view_.ecran = ecran_;
    view_.marges = marges_;
    view_.allerA = [this](Ecran e) { allerA(e); };
    view_.choisirVoiture = [this](const char* id, vec3 p) { voitureDemandee_ = id; peintureDemandee_ = p; };   // WFS-11
    for (const SystemEntry& e : entries) {
        systems_.push_back(e.make());
        systems_.back()->init(*r_, view_);
        WFS_LOG("Système : %s (ordre %d)", e.name, e.order);
    }
}

void Game::systemsShutdown() {
    if (!r_) return;
    for (auto it = systems_.rbegin(); it != systems_.rend(); ++it) (*it)->shutdown(*r_);
    systems_.clear();
}

void Game::systemsUpdate(double dt) {
    view_.car = carDef_;
    for (auto& s : systems_) s->update(dt, view_);
}

void Game::systemsRender(const vec3& eye, const vec3& look, const mat4& view, const mat4& proj, double time) {
    view_.camPos = eye; view_.camTarget = look; view_.view = view; view_.proj = proj; view_.time = time;
    view_.uiWidth = (float)r_->width(); view_.uiHeight = (float)r_->height();
    for (auto& s : systems_) s->render(*r_, view_);
}

void Game::systemsUi() { for (auto& s : systems_) s->drawUi(*r_, view_); }

bool Game::systemsTouch(const Touch& t) {
    for (auto& s : systems_) if (s->onTouch(t, view_)) return true;
    return false;
}

} // namespace wfs
