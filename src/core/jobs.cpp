#include "jobs.h"
#include "log.h"

namespace wfs {

Jobs& jobs() { static Jobs j; return j; }

void Jobs::start(int workers) {
    if (!threads_.empty()) return;
    if (workers < 0) {
        unsigned hc = std::thread::hardware_concurrency();
        workers = hc > 1 ? static_cast<int>(hc) - 1 : 1;
        if (workers > 7) workers = 7;
    }
    quit_ = false;
    for (int i = 0; i < workers; i++) threads_.emplace_back([this] { loop(); });
    WFS_LOG("Système de tâches : %d threads de travail", workers);
}

void Jobs::stop() {
    { std::lock_guard<std::mutex> l(m_); quit_ = true; }
    cv_.notify_all();
    for (auto& t : threads_) if (t.joinable()) t.join();
    threads_.clear();
}

void Jobs::run(std::function<void()> fn, Counter* c) {
    if (c) c->pending.fetch_add(1, std::memory_order_relaxed);
    if (threads_.empty()) { fn(); if (c) c->pending.fetch_sub(1, std::memory_order_release); return; }
    { std::lock_guard<std::mutex> l(m_); queue_.push_back({std::move(fn), c}); }
    cv_.notify_one();
}

bool Jobs::tryRunOne() {
    Job j;
    { std::lock_guard<std::mutex> l(m_); if (queue_.empty()) return false; j = std::move(queue_.front()); queue_.pop_front(); }
    j.fn();
    if (j.c) j.c->pending.fetch_sub(1, std::memory_order_release);
    return true;
}

void Jobs::loop() {
    for (;;) {
        Job j;
        {
            std::unique_lock<std::mutex> l(m_);
            cv_.wait(l, [this] { return quit_ || !queue_.empty(); });
            if (quit_ && queue_.empty()) return;
            j = std::move(queue_.front()); queue_.pop_front();
        }
        j.fn();
        if (j.c) j.c->pending.fetch_sub(1, std::memory_order_release);
    }
}

void Jobs::wait(Counter& c) {
    while (c.pending.load(std::memory_order_acquire) > 0) {
        if (!tryRunOne()) std::this_thread::yield();
    }
}

void Jobs::parallelFor(int n, int grain, const std::function<void(int, int)>& body) {
    if (n <= 0) return;
    if (grain < 1) grain = 1;
    Counter c;
    for (int b = 0; b < n; b += grain) {
        int e = b + grain < n ? b + grain : n;
        run([&body, b, e] { body(b, e); }, &c);
    }
    wait(c);
}

} // namespace wfs
