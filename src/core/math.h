// WaForSpeed — mathématiques 3D (vecteurs, matrices 4x4 colonne-majeure comme GLSL).
// Repère du monde : y vers le haut, la route avance vers -z (même convention que le jeu web).
#pragma once
#include <cmath>
#include <cstdint>

namespace wfs {

constexpr float PI = 3.14159265358979f;

inline float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline float signf(float v) { return v > 0 ? 1.f : (v < 0 ? -1.f : 0.f); }

struct vec2 { float x = 0, y = 0; };
struct vec3 {
    float x = 0, y = 0, z = 0;
    vec3() = default;
    constexpr vec3(float a, float b, float c) : x(a), y(b), z(c) {}
    vec3 operator+(const vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    vec3 operator-(const vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    vec3 operator-() const { return {-x, -y, -z}; }
    vec3& operator+=(const vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
};
inline float dot(const vec3& a, const vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline vec3 cross(const vec3& a, const vec3& b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
inline float length(const vec3& v) { return std::sqrt(dot(v, v)); }
inline vec3 normalize(const vec3& v) { float l = length(v); return l > 1e-8f ? v * (1.f / l) : vec3{0, 0, 0}; }
inline vec3 lerp(const vec3& a, const vec3& b, float t) { return a + (b - a) * t; }

struct vec4 { float x = 0, y = 0, z = 0, w = 0; };

struct mat4 {
    float m[16];   // colonne-majeure : m[col*4 + row]
    static mat4 identity() { mat4 r{}; for (int i = 0; i < 16; i++) r.m[i] = (i % 5 == 0) ? 1.f : 0.f; return r; }
    float& at(int row, int col) { return m[col * 4 + row]; }
    float at(int row, int col) const { return m[col * 4 + row]; }
    mat4 operator*(const mat4& b) const {
        mat4 r{};
        for (int c = 0; c < 4; c++) for (int rr = 0; rr < 4; rr++) {
            float s = 0; for (int k = 0; k < 4; k++) s += at(rr, k) * b.at(k, c);
            r.at(rr, c) = s;
        }
        return r;
    }
    vec3 transformPoint(const vec3& p) const {
        return {at(0, 0) * p.x + at(0, 1) * p.y + at(0, 2) * p.z + at(0, 3),
                at(1, 0) * p.x + at(1, 1) * p.y + at(1, 2) * p.z + at(1, 3),
                at(2, 0) * p.x + at(2, 1) * p.y + at(2, 2) * p.z + at(2, 3)};
    }
    vec3 transformDir(const vec3& p) const {
        return {at(0, 0) * p.x + at(0, 1) * p.y + at(0, 2) * p.z,
                at(1, 0) * p.x + at(1, 1) * p.y + at(1, 2) * p.z,
                at(2, 0) * p.x + at(2, 1) * p.y + at(2, 2) * p.z};
    }
};

inline mat4 translate(const vec3& t) { mat4 r = mat4::identity(); r.at(0, 3) = t.x; r.at(1, 3) = t.y; r.at(2, 3) = t.z; return r; }
inline mat4 scale(const vec3& s) { mat4 r = mat4::identity(); r.at(0, 0) = s.x; r.at(1, 1) = s.y; r.at(2, 2) = s.z; return r; }
inline mat4 rotateX(float a) { mat4 r = mat4::identity(); float c = std::cos(a), s = std::sin(a); r.at(1, 1) = c; r.at(1, 2) = -s; r.at(2, 1) = s; r.at(2, 2) = c; return r; }
inline mat4 rotateY(float a) { mat4 r = mat4::identity(); float c = std::cos(a), s = std::sin(a); r.at(0, 0) = c; r.at(0, 2) = s; r.at(2, 0) = -s; r.at(2, 2) = c; return r; }
inline mat4 rotateZ(float a) { mat4 r = mat4::identity(); float c = std::cos(a), s = std::sin(a); r.at(0, 0) = c; r.at(0, 1) = -s; r.at(1, 0) = s; r.at(1, 1) = c; return r; }

// caméra : matrice de vue (regarde de eye vers target)
inline mat4 lookAt(const vec3& eye, const vec3& target, const vec3& up) {
    vec3 f = normalize(target - eye), s = normalize(cross(f, up)), u = cross(s, f);
    mat4 r = mat4::identity();
    r.at(0, 0) = s.x; r.at(0, 1) = s.y; r.at(0, 2) = s.z; r.at(0, 3) = -dot(s, eye);
    r.at(1, 0) = u.x; r.at(1, 1) = u.y; r.at(1, 2) = u.z; r.at(1, 3) = -dot(u, eye);
    r.at(2, 0) = -f.x; r.at(2, 1) = -f.y; r.at(2, 2) = -f.z; r.at(2, 3) = dot(f, eye);
    return r;
}
// perspective Vulkan : y de l'écran vers le bas, profondeur inversée (1 près, 0 loin) pour la précision
inline mat4 perspectiveVk(float fovyRad, float aspect, float zNear) {
    float f = 1.f / std::tan(fovyRad * 0.5f);
    mat4 r{};
    r.at(0, 0) = f / aspect;
    r.at(1, 1) = -f;
    r.at(2, 2) = 0.f; r.at(2, 3) = zNear;   // profondeur inversée, plan lointain à l'infini
    r.at(3, 2) = -1.f;
    return r;
}

inline mat4 inverseAffine(const mat4& a) {
    // inverse d'une matrice rotation+translation (vue de caméra)
    mat4 r = mat4::identity();
    for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) r.at(i, j) = a.at(j, i);
    vec3 t{a.at(0, 3), a.at(1, 3), a.at(2, 3)};
    r.at(0, 3) = -(r.at(0, 0) * t.x + r.at(0, 1) * t.y + r.at(0, 2) * t.z);
    r.at(1, 3) = -(r.at(1, 0) * t.x + r.at(1, 1) * t.y + r.at(1, 2) * t.z);
    r.at(2, 3) = -(r.at(2, 0) * t.x + r.at(2, 1) * t.y + r.at(2, 2) * t.z);
    return r;
}

// inverse générale 4x4 (cofacteurs)
inline mat4 inverse(const mat4& A) {
    const float* m = A.m; float inv[16];
    inv[0] = m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15] + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
    inv[4] = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15] - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
    inv[8] = m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15] + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
    inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14] - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
    inv[1] = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15] - m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
    inv[5] = m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15] + m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
    inv[9] = -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15] - m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
    inv[13] = m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14] + m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
    inv[2] = m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15] + m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6];
    inv[6] = -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15] - m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6];
    inv[10] = m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15] + m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5];
    inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14] - m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5];
    inv[3] = -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11] - m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6];
    inv[7] = m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11] + m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6];
    inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11] - m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5];
    inv[15] = m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10] + m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5];
    float det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
    mat4 r{}; float id = std::fabs(det) > 1e-20f ? 1.f / det : 0.f;
    for (int i = 0; i < 16; i++) r.m[i] = inv[i] * id;
    return r;
}

// générateur pseudo-aléatoire identique au jeu web (mulberry32) : mêmes cartes à partir de la même graine
struct Mulberry32 {
    uint32_t a;
    explicit Mulberry32(uint32_t seed) : a(seed) {}
    double next() {
        a += 0x6D2B79F5u;
        uint32_t t = a;
        t = (t ^ (t >> 15)) * (1u | t);
        t = (t + ((t ^ (t >> 7)) * (61u | t))) ^ t;
        return static_cast<double>(t ^ (t >> 14)) / 4294967296.0;
    }
};

} // namespace wfs
