// WaForSpeed — système de tâches multi-thread.
// Un thread par cœur (moins le thread principal). Les tâches sont des fonctions courtes ;
// un Counter permet d'attendre la fin d'un groupe. parallelFor découpe une boucle sur tous les cœurs.
#pragma once
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace wfs {

struct Counter { std::atomic<int> pending{0}; };

class Jobs {
public:
    void start(int workers = -1);
    void stop();
    ~Jobs() { stop(); }
    // lance une tâche ; le compteur (facultatif) est décrémenté quand elle se termine
    void run(std::function<void()> fn, Counter* c = nullptr);
    // attend la fin du groupe ; le thread appelant aide à exécuter les tâches en attendant
    void wait(Counter& c);
    // exécute body(i) pour i dans [0, n) en parallèle, par paquets de `grain`
    void parallelFor(int n, int grain, const std::function<void(int begin, int end)>& body);
    int workerCount() const { return static_cast<int>(threads_.size()); }
private:
    struct Job { std::function<void()> fn; Counter* c; };
    bool tryRunOne();
    void loop();
    std::vector<std::thread> threads_;
    std::deque<Job> queue_;
    std::mutex m_;
    std::condition_variable cv_;
    bool quit_ = false;
};

Jobs& jobs();

} // namespace wfs
