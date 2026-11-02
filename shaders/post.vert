#version 450
// triangle plein écran ; rot = pré-rotation Android (la swapchain est dans l'orientation native de l'écran)
layout(push_constant) uniform P { vec4 rot; vec4 params; } pc;
layout(location = 0) out vec2 vUv;
void main() {
    vec2 p = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2.0 - 1.0;
    vUv = p * 0.5 + 0.5;
    gl_Position = vec4(mat2(pc.rot.xy, pc.rot.zw) * p, 0.0, 1.0);
}
