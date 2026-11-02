// Vérifie le mécanisme des systèmes enfichables (WFS-30) : enregistrement automatique et ordre d'appel.
#include <algorithm>
#include <cstdio>
#include <string>
#include "../src/core/system.h"
using namespace wfs;
static std::string trace;
struct Deux : System { const char* name() const override { return "deux"; } void update(double, const GameView&) override { trace += "2"; } };
struct Un : System { const char* name() const override { return "un"; } void update(double, const GameView&) override { trace += "1"; } };
WFS_SYSTEM(Deux, 200)
WFS_SYSTEM(Un, 100)
int main() {
    auto e = systemRegistry();
    std::stable_sort(e.begin(), e.end(), [](const SystemEntry& a, const SystemEntry& b) { return a.order < b.order; });
    GameView v;
    for (auto& x : e) x.make()->update(0.01, v);
    std::printf("systèmes : %zu, ordre : %s\n", e.size(), trace.c_str());
    return (e.size() == 2 && trace == "12") ? 0 : 1;
}
