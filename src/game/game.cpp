#include "game.h"
#include <algorithm>
#include "../core/jobs.h"
#include "../core/log.h"
#include "../render/model_loader.h"

namespace wfs {

static constexpr int CHUNKS_BEHIND = 2, CHUNKS_AHEAD = 22;
static constexpr float WALK_W = 4.0f, CURB_H = 0.15f, GROUND_W = 70.f;

static vec3 hexColor(unsigned h) {
    auto lin = [](float c) { return std::pow(c, 2.2f); };
    return {lin(((h >> 16) & 255) / 255.f), lin(((h >> 8) & 255) / 255.f), lin((h & 255) / 255.f)};
}

bool Game::init(Renderer& r, const char* carId, const char* mapId) {
    r_ = &r;
    track_.build(mapById(mapId));
    Material m;
    m.flags = MAT_ROAD; m.rough = 0.8f; roadMat_ = r.makeMaterial(m, nullptr, nullptr, nullptr);
    m.flags = MAT_WALK; m.rough = 0.85f; walkMat_ = r.makeMaterial(m, nullptr, nullptr, nullptr);
    m.flags = MAT_GROUND; m.rough = 0.95f; groundMat_ = r.makeMaterial(m, nullptr, nullptr, nullptr);
    setCar(carId);
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
    for (auto& [k, c] : chunks_) { r_->destroy(c.road); r_->destroy(c.walk); r_->destroy(c.ground); }
    chunks_.clear();
    for (auto& g : graveyard_) r_->destroy(g.second);
    graveyard_.clear();
    r_->destroy(car_);
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
    strip(2, {{-R - WALK_W - GROUND_W, -0.4f}, {-R - WALK_W - 20.f, 0.f}, {-R - WALK_W, CURB_H - 0.02f}});
    strip(2, {{R + WALK_W, CURB_H - 0.02f}, {R + WALK_W + 20.f, 0.f}, {R + WALK_W + GROUND_W, -0.4f}});

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
    const float W = (float)r_->width(), H = (float)r_->height();
    if (t.down) {
        bool fresh = t.id != steerId_ && t.id != gasId_ && t.id != brakeId_ && t.id != nitroId_;
        if (fresh) {
            if (t.x < W * 0.45f) { steerId_ = t.id; steerX0_ = steerX_ = t.x; }
            else if (t.x > W * 0.80f && t.y < H * 0.45f) nitroId_ = t.id;
            else if (t.x > W * 0.78f) gasId_ = t.id;
            else if (t.x > W * 0.55f) brakeId_ = t.id;
        }
        if (t.id == steerId_) steerX_ = t.x;
    } else {
        if (t.id == steerId_) steerId_ = -1;
        if (t.id == gasId_) gasId_ = -1;
        if (t.id == brakeId_) brakeId_ = -1;
        if (t.id == nitroId_) nitroId_ = -1;
    }
    input_.steer = steerId_ >= 0 ? clampf((steerX_ - steerX0_) / (W * 0.12f), -1.f, 1.f) : 0.f;
    input_.gas = gasId_ >= 0 || nitroId_ >= 0;
    input_.brake = brakeId_ >= 0;
    input_.nitro = nitroId_ >= 0;
}

void Game::drawControls() {
    const float W = (float)r_->width(), H = (float)r_->height(), u = H / 100.f;
    // volant virtuel
    if (steerId_ >= 0) {
        r_->uiCircle(steerX0_, H * 0.72f, 11 * u, {1, 1, 1, 0.12f});
        r_->uiCircle(steerX0_ + clampf(steerX_ - steerX0_, -W * 0.12f, W * 0.12f), H * 0.72f, 5 * u, {0, 0.9f, 1, 0.55f});
    } else r_->uiCircle(W * 0.18f, H * 0.72f, 9 * u, {1, 1, 1, 0.08f});
    // pédales
    r_->uiCircle(W * 0.89f, H * 0.74f, 11 * u, input_.gas ? vec4{0.1f, 1, 0.45f, 0.6f} : vec4{0.1f, 1, 0.45f, 0.22f});
    r_->uiCircle(W * 0.70f, H * 0.80f, 8.5f * u, input_.brake ? vec4{1, 0.2f, 0.25f, 0.6f} : vec4{1, 0.2f, 0.25f, 0.22f});
    r_->uiCircle(W * 0.90f, H * 0.32f, 6.5f * u, input_.nitro ? vec4{0.2f, 0.85f, 1, 0.7f} : vec4{0.2f, 0.85f, 1, 0.25f});
    // jauges : vitesse (haut à gauche) et nitro
    float k = (float)clampf((float)(player_.kmh() / 330.0), 0.f, 1.f);
    r_->uiRect(3 * u, 4 * u, 40 * u, 2.2f * u, {0, 0, 0, 0.35f});
    r_->uiRect(3 * u, 4 * u, 40 * u * k, 2.2f * u, {1, 0.42f + 0.4f * (1 - k), 0.1f, 0.9f});
    r_->uiRect(3 * u, 7.5f * u, 25 * u, 1.4f * u, {0, 0, 0, 0.35f});
    r_->uiRect(3 * u, 7.5f * u, 25 * u * (float)player_.nitro, 1.4f * u, {0.2f, 0.85f, 1, 0.9f});
}

// ---------------------------------------------------------------- mise à jour
void Game::update(double dt) {
    dt = std::min(dt, 0.05);
    DriveInput in = input_;
    if (autopilot_) {   // démonstration / tests : plein gaz, reste au milieu de sa voie
        in.gas = true; in.brake = false;
        in.steer = (float)clampf((float)((0.0 - player_.playerX) * 3.0 - player_.heading * 4.0), -1.f, 1.f);
    }
    // pas fixes de 1/120 s : la conduite est identique quelle que soit la fréquence d'affichage
    acc_ += dt;
    const double step = 1.0 / 120.0;
    while (acc_ >= step) { player_.update(step, in, track_, true); acc_ -= step; }
}

mat4 Game::carMatrix(double s, double lat, double yaw) {
    TrackPoint p = track_.point(s, lat);
    return translate({(float)p.x, (float)p.y, (float)p.z}) * rotateY((float)-(p.th + yaw)) * rotateX((float)p.slope);
}

void Game::render(double time) {
    frameNo_++;
    streamChunks();
    const double s = player_.dist, lat = player_.playerX * RW;
    mat4 carM = carMatrix(s, lat, player_.heading);
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
        if (!camInit_) { camEye_ = eyeT; camLook_ = lookT; camInit_ = true; }
        camEye_ = lerp(camEye_, eyeT, 0.25f); camLook_ = lerp(camLook_, lookT, 0.35f);
        eye = camEye_; look = camLook_;
    }
    float aspect = (float)r_->width() / (float)std::max(1u, r_->height());
    float fov = (camMode == 0 ? 58.f : 62.f) + (float)std::min(player_.kmh() / 300.0, 1.0) * 8.f;

    FrameParams fp;
    fp.view = lookAt(eye, look, {0, 1, 0});
    fp.proj = perspectiveVk(fov * PI / 180.f, aspect, 0.1f);
    fp.camPos = eye; fp.time = (float)time;
    fp.sunDir = {-0.45f, 0.42f, -0.55f}; fp.sunIntensity = 3.2f; fp.sunColor = {1.0f, 0.93f, 0.82f};
    fp.skyTop = {0.16f, 0.34f, 0.75f}; fp.skyHorizon = {0.66f, 0.76f, 0.88f}; fp.ground = {0.2f, 0.19f, 0.17f};
    fp.fogColor = {0.62f, 0.7f, 0.8f}; fp.fogDensity = 0.0018f;
    fp.exposure = 1.0f; fp.ambient = 0.9f;
    if (!r_->beginFrame(fp)) return;

    const mat4 I = mat4::identity();
    for (auto& [k, c] : chunks_) {
        r_->draw(c.ground, groundMat_, I);
        r_->draw(c.walk, walkMat_, I);
        r_->draw(c.road, roadMat_, I, (float)RW);
    }
    r_->draw(car_, carM, &paint_);
    if (camMode != 2) drawControls();
    r_->endFrame();
}

} // namespace wfs
