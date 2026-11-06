// WaForSpeed — WFS-149 (RG-04) : tangentes des sommets pour tools/convert_car.mjs. MÊME calcul que le moteur
// (src/render/mesh.cpp, voir mesh.h) : celui de MikkTSpace (M. Mikkelsen 2008, mikktspace.c) pour un maillage découpé aux
// coutures d'uv : par coin, ∂p/∂u et ∂p/∂v projetées dans le plan de la normale du sommet, normalisées, pondérées par
// l'angle du coin, sommées par sommet, puis Gram-Schmidt ; signe = orientation de la bitangente (bitangente = w · N × T).
// Sortie : 4 octets signés par sommet (x, y, z, w × 127), comme la section 'TAN1' d'un .wfm version 2.
// v : Float32Array de sommets (stride flottants : pos3 normale3 uv2 en tête), ix : indices des triangles.
export function tangentes(v, stride, ix) {
  const n = v.length / stride, T = new Float64Array(n * 3), B = new Float64Array(n * 3), out = new Int8Array(n * 4);
  const P = (i) => [v[i * stride], v[i * stride + 1], v[i * stride + 2]];
  const N = (i) => { const a = [v[i * stride + 3], v[i * stride + 4], v[i * stride + 5]], l = Math.hypot(...a); return l > 0.5 ? a.map((x) => x / l) : [0, 1, 0]; };
  const sub = (a, b) => [a[0] - b[0], a[1] - b[1], a[2] - b[2]], dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  const cross = (a, b) => [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
  const unit = (a) => { const l = Math.hypot(...a); return l > 1e-30 ? a.map((x) => x / l) : [0, 0, 0]; };
  const plan = (a, nn) => { const d = dot(nn, a); return unit([a[0] - nn[0] * d, a[1] - nn[1] * d, a[2] - nn[2] * d]); };
  for (let t = 0; t + 2 < ix.length; t += 3) {
    const c = [ix[t], ix[t + 1], ix[t + 2]];
    if (c.some((k) => k >= n)) continue;
    const p0 = P(c[0]), e1 = sub(P(c[1]), p0), e2 = sub(P(c[2]), p0);
    const uv = (k) => [v[k * stride + 6], v[k * stride + 7]];
    const s1 = uv(c[1])[0] - uv(c[0])[0], t1 = uv(c[1])[1] - uv(c[0])[1], s2 = uv(c[2])[0] - uv(c[0])[0], t2 = uv(c[2])[1] - uv(c[0])[1];
    const r = s1 * t2 - s2 * t1;
    if (Math.abs(r) < 1e-20 || Math.hypot(...cross(e1, e2)) < 1e-20) continue;
    const sdir = [0, 1, 2].map((k) => (e1[k] * t2 - e2[k] * t1) / r), tdir = [0, 1, 2].map((k) => (e2[k] * s1 - e1[k] * s2) / r);
    for (let k = 0; k < 3; k++) {
      const i = c[k], a = unit(sub(P(c[(k + 1) % 3]), P(i))), b = unit(sub(P(c[(k + 2) % 3]), P(i)));
      const angle = Math.acos(Math.max(-1, Math.min(1, dot(a, b)))), nn = N(i), ts = plan(sdir, nn), bs = plan(tdir, nn);
      for (let q = 0; q < 3; q++) { T[i * 3 + q] += ts[q] * angle; B[i * 3 + q] += bs[q] * angle; }
    }
  }
  const octet = (x) => { const c = Math.max(-1, Math.min(1, x)); return c >= 0 ? Math.trunc(c * 127 + 0.5) : -Math.trunc(-c * 127 + 0.5); };
  let sans = 0;
  for (let i = 0; i < n; i++) {
    const nn = N(i);
    let t = plan([T[i * 3], T[i * 3 + 1], T[i * 3 + 2]], nn), w = 1;
    if (Math.hypot(...t) < 0.5) { t = plan(Math.abs(nn[0]) < 0.9 ? [1, 0, 0] : [0, 0, 1], nn); sans++; }
    else if (dot(cross(nn, t), [B[i * 3], B[i * 3 + 1], B[i * 3 + 2]]) < 0) w = -1;
    out.set([octet(t[0]), octet(t[1]), octet(t[2]), w > 0 ? 127 : -127], i * 4);
  }
  return { octets: out, sans };
}
