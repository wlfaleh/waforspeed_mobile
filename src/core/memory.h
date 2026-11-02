// WaForSpeed — mémoire : arène linéaire (allocations par image, remise à zéro en bloc)
// et réserve d'objets de taille fixe (pas d'allocation système pendant la course).
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>
#include "log.h"

namespace wfs {

// Arène : on avance un pointeur ; reset() libère tout d'un coup. Idéale pour les données d'une image.
class Arena {
public:
    explicit Arena(size_t bytes = 0) { if (bytes) init(bytes); }
    ~Arena() { std::free(base_); }
    Arena(const Arena&) = delete; Arena& operator=(const Arena&) = delete;
    void init(size_t bytes) { std::free(base_); base_ = static_cast<uint8_t*>(std::malloc(bytes)); cap_ = bytes; off_ = 0; peak_ = 0; WFS_CHECK(base_, "Arena: plus de mémoire (%zu octets)", bytes); }
    void* alloc(size_t bytes, size_t align = 16) {
        size_t p = (off_ + align - 1) & ~(align - 1);
        WFS_CHECK(p + bytes <= cap_, "Arena pleine (%zu / %zu octets)", p + bytes, cap_);
        off_ = p + bytes; if (off_ > peak_) peak_ = off_;
        return base_ + p;
    }
    template <class T> T* make(size_t n = 1) { T* t = static_cast<T*>(alloc(sizeof(T) * n, alignof(T))); for (size_t i = 0; i < n; i++) new (t + i) T(); return t; }
    void reset() { off_ = 0; }
    size_t used() const { return off_; }
    size_t peak() const { return peak_; }
    size_t capacity() const { return cap_; }
private:
    uint8_t* base_ = nullptr; size_t cap_ = 0, off_ = 0, peak_ = 0;
};

// Réserve d'objets T : capacité fixe, allocation/libération en O(1) via une liste libre.
template <class T>
class Pool {
public:
    explicit Pool(size_t capacity = 0) { if (capacity) init(capacity); }
    void init(size_t capacity) {
        slots_.resize(capacity); free_.clear(); free_.reserve(capacity);
        for (size_t i = capacity; i-- > 0;) free_.push_back(static_cast<uint32_t>(i));
        alive_.assign(capacity, 0);
    }
    template <class... A> T* create(A&&... a) {
        if (free_.empty()) return nullptr;
        uint32_t i = free_.back(); free_.pop_back(); alive_[i] = 1;
        return new (&slots_[i].storage) T(static_cast<A&&>(a)...);
    }
    void destroy(T* t) {
        size_t i = reinterpret_cast<Slot*>(t) - slots_.data();
        if (i >= slots_.size() || !alive_[i]) return;
        t->~T(); alive_[i] = 0; free_.push_back(static_cast<uint32_t>(i));
    }
    template <class F> void forEach(F&& f) { for (size_t i = 0; i < slots_.size(); i++) if (alive_[i]) f(*reinterpret_cast<T*>(&slots_[i].storage)); }
    size_t size() const { return slots_.size() - free_.size(); }
    size_t capacity() const { return slots_.size(); }
private:
    struct Slot { alignas(T) unsigned char storage[sizeof(T)]; };
    std::vector<Slot> slots_; std::vector<uint32_t> free_; std::vector<uint8_t> alive_;
};

} // namespace wfs
