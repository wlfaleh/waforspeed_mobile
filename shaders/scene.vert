#version 450
#include "common.glsl"
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNrm;
layout(location = 2) in vec2 inUv;
layout(location = 3) in vec4 inTan;   // WFS-149 (RG-04) : tangente (xyz, signe de la bitangente), nulle = sans tangente
layout(location = 4) in vec4 inI0;   // WFS-273 (RG-01) : exemplaire (binding 1) : lignes 0 à 2 de sa matrice monde
layout(location = 5) in vec4 inI1;
layout(location = 6) in vec4 inI2;
layout(location = 7) in vec4 inIx;   // x : facteur de la normale (teinte MAT_VTINT ÷ échelle) ; dessins ordinaires : identité, 1
layout(push_constant) uniform P { mat4 model; vec4 color; vec4 params; vec4 emissive; } pc;
layout(location = 0) out vec3 vWorld;
layout(location = 1) out vec3 vNrm;
layout(location = 2) out vec2 vUv;
layout(location = 3) out vec4 vTan;
void main() {
    const mat4 inst = mat4(vec4(inI0.x, inI1.x, inI2.x, 0.0), vec4(inI0.y, inI1.y, inI2.y, 0.0), vec4(inI0.z, inI1.z, inI2.z, 0.0),
                           vec4(inI0.w, inI1.w, inI2.w, 1.0));
    const mat4 M = pc.model * inst;
    vec4 w = M * vec4(inPos, 1.0);
    vWorld = w.xyz;
    vNrm = mat3(M) * inNrm * inIx.x;
    vUv = inUv;
    vTan = vec4(mat3(M) * inTan.xyz, inTan.w);
    gl_Position = F.viewProj * w;
}
