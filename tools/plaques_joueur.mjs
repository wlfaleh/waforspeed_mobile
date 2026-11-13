// WaForSpeed — WFS-483 (RG-04) : place des plaques 3D de la voiture du joueur, relevée sur la plaque d'origine de chaque .wfm.
// Les plaques d'origine sont toutes au format 520 × 110 mm depuis WFS-158 (tools/plaques_voitures.mjs, tools/plaques_neutres.mjs) :
// un maillage à part dont le matériau porte l'image de la plaque (dernière texture du fichier, ou l'image de tools/plaques/
// pour la V60, l'A 45 et le Range Rover Sport). Pour chaque côté (avant : z < 0, arrière : z > 0) :
//   - normale : vecteur propre de la plus petite valeur propre de la covariance des sommets (plan ajusté), tourné vers l'extérieur ;
//   - centre : milieu du rectangle de la plaque dans son plan, posé sur le sommet le plus en avant le long de la normale
//     (le porte-plaque 3D est posé devant toute la plaque d'origine, qu'il recouvre : 540 × 130 mm contre 520 × 110 mm) ;
//   - jour entre le fond du porte-plaque (plan de la plaque + 0,8 mm) et la caisse, en 64 points répartis sur le contour du
//     porte-plaque (≈ 21 mm entre deux) : rayon lancé le long de −normale sur les pièces opaques. Le porte-plaque 3D descend jusqu'à la caisse en suivant ces jours (support adaptateur, comme sur une vraie voiture
//     au pare-chocs bombé) : il ne flotte jamais (RG-04, RG-08). Jour borné à 60 mm (au-delà : calandre ouverte, rien derrière) ;
//   - contrôle : largeur et hauteur de la plaque d'origine (520 × 110 mm ± 2 %), écrit sur la sortie d'erreur.
// Sortie : les lignes du tableau de src/render/vehicule_plaque_defs.cpp (à coller), une par voiture.
// usage : node tools/plaques_joueur.mjs [id …]   (sans id : toutes les voitures de assets/cars/)
import fs from 'fs';

const RACINE = new URL('../', import.meta.url);
const ATLAS = { v60: 'tools/plaques/v60.png', a45: 'tools/plaques/a45.png', rrsport: 'tools/plaques/rrsport.png' };

function lire(chemin) {
  const b = fs.readFileSync(chemin);
  let o = 4;
  const u32 = () => { const x = b.readUInt32LE(o); o += 4; return x; };
  const f = { version: u32() };
  const nT = u32(), nM = u32(), nMe = u32();
  o += 24;
  f.textures = []; for (let t = 0; t < nT; t++) { const n = u32(); f.textures.push(b.subarray(o, o + n)); o += n; }
  f.mats = []; for (let i = 0; i < nM; i++) { f.mats.push(b.subarray(o, o + 56)); o += 56; }
  f.maillages = [];
  for (let k = 0; k < nMe; k++) {
    const mat = u32(), nv = u32(), ni = u32();
    const v = new Float32Array(b.buffer.slice(b.byteOffset + o, b.byteOffset + o + nv * 32)); o += nv * 32;
    const ix = new Uint32Array(b.buffer.slice(b.byteOffset + o, b.byteOffset + o + ni * 4)); o += ni * 4;
    f.maillages.push({ mat, v, ix });
  }
  return f;
}
const dot = (a, c) => a[0] * c[0] + a[1] * c[1] + a[2] * c[2];
const sub = (a, c) => [a[0] - c[0], a[1] - c[1], a[2] - c[2]];
const cross = (a, c) => [a[1] * c[2] - a[2] * c[1], a[2] * c[0] - a[0] * c[2], a[0] * c[1] - a[1] * c[0]];
const unit = (a) => { const l = Math.hypot(a[0], a[1], a[2]) || 1; return [a[0] / l, a[1] / l, a[2] / l]; };

// plus petit vecteur propre d'une matrice symétrique 3 × 3 (itérations de la puissance sur (trace·I − C))
function normalePlan(P) {
  const m = [0, 1, 2].map((c) => P.reduce((s, p) => s + p[c], 0) / P.length);
  const C = [[0, 0, 0], [0, 0, 0], [0, 0, 0]];
  for (const p of P) { const d = sub(p, m); for (let i = 0; i < 3; i++) for (let j = 0; j < 3; j++) C[i][j] += d[i] * d[j]; }
  const tr = C[0][0] + C[1][1] + C[2][2];
  const A = C.map((row, i) => row.map((x, j) => (i === j ? tr - x : -x)));
  let v = [0.1, 0.2, 1];
  for (let it = 0; it < 200; it++) v = unit([0, 1, 2].map((i) => A[i][0] * v[0] + A[i][1] * v[1] + A[i][2] * v[2]));
  return v;
}

function placer(f, id) {
  let tp = f.textures.length - 1;
  if (ATLAS[id]) { const p = fs.readFileSync(new URL(ATLAS[id], RACINE)); tp = f.textures.findIndex((t) => Buffer.compare(t, p) === 0); }
  const res = {};
  for (const cote of [-1, 1]) {
    const P = [];
    f.maillages.forEach((m) => {
      if (f.mats[m.mat].readInt32LE(40) !== tp) return;
      for (let t = 0; t + 2 < m.ix.length; t += 3) {
        const c = [m.ix[t], m.ix[t + 1], m.ix[t + 2]];
        if (c[0] === c[1] || c[1] === c[2]) continue;   // triangle masqué (dégénéré)
        const z = (m.v[c[0] * 8 + 2] + m.v[c[1] * 8 + 2] + m.v[c[2] * 8 + 2]) / 3;
        if (Math.sign(z) !== cote) continue;
        for (const i of c) P.push([m.v[i * 8], m.v[i * 8 + 1], m.v[i * 8 + 2]]);
      }
    });
    if (P.length < 3) { res[cote] = null; continue; }
    let n = normalePlan(P); if (Math.sign(n[2]) !== cote) n = n.map((x) => -x);
    let U = unit(sub([0, 1, 0], n.map((x) => x * n[1])));
    const X = cross(U, n);
    const a = P.map((p) => dot(p, X)), h = P.map((p) => dot(p, U)), e = P.map((p) => dot(p, n));
    const ac = (Math.min(...a) + Math.max(...a)) / 2, hc = (Math.min(...h) + Math.max(...h)) / 2, ef = Math.max(...e);
    const centre = [0, 1, 2].map((c) => X[c] * ac + U[c] * hc + n[c] * ef);
    res[cote] = { centre, n, X, U, largeur: Math.max(...a) - Math.min(...a), hauteur: Math.max(...h) - Math.min(...h) };
  }
  return res;
}

// contour du porte-plaque (540 × 130 mm, rayon 9 mm, sens trigonométrique vu de l'extérieur, départ en haut du côté droit) :
// JOURS points à intervalles égaux de l'abscisse curviligne (même calcul que pointContour, src/render/vehicule_plaque_geo.cpp)
export const JOURS = 64, JOUR_MAX = 0.060;
export function pointsContour(n = JOURS) {
  const a = 0.270, b = 0.065, r = 0.009, seg = 64, c = [];
  const cx = [a - r, -a + r, -a + r, a - r], cy = [b - r, b - r, -b + r, -b + r];
  for (let k = 0; k < 4; k++) for (let i = 0; i <= seg; i++) { const t = (k + i / seg) * Math.PI / 2; c.push([cx[k] + r * Math.cos(t), cy[k] + r * Math.sin(t)]); }
  const s = [0]; for (let i = 1; i < c.length; i++) s.push(s[i - 1] + Math.hypot(c[i][0] - c[i - 1][0], c[i][1] - c[i - 1][1]));
  const total = s[s.length - 1] + Math.hypot(c[0][0] - c[c.length - 1][0], c[0][1] - c[c.length - 1][1]);
  const out = [];
  for (let k = 0; k < n; k++) {
    const x = k / n; let i = 1; while (i < s.length && s[i] / total <= x) i++;
    const i0 = i - 1, i1 = i % c.length, s1 = i < s.length ? s[i] / total : 1, f = (x - s[i0] / total) / Math.max(1e-9, s1 - s[i0] / total);
    out.push([c[i0][0] + (c[i1][0] - c[i0][0]) * f, c[i0][1] + (c[i1][1] - c[i0][1]) * f]);
  }
  return out;
}
// jour entre le fond du porte-plaque (plan de la plaque + 0,8 mm) et la caisse, le long de −normale (pièces opaques seulement),
// aux points du contour, en m, borné à JOUR_MAX
export function jours(f, p) {
  const pts = pointsContour();
  const tris = [];
  f.maillages.forEach((m) => {
    const dr = f.mats[m.mat].readUInt32LE(52);
    if (dr & (2 | 64)) return;
    for (let t = 0; t + 2 < m.ix.length; t += 3) {
      const T = [0, 1, 2].map((q) => [m.v[m.ix[t + q] * 8], m.v[m.ix[t + q] * 8 + 1], m.v[m.ix[t + q] * 8 + 2]]);
      // triangle proche de la plaque (boîte à 0,4 m du centre ; un grand triangle compte même si son centre est loin)
      const lo = [0, 1, 2].map((c) => Math.min(T[0][c], T[1][c], T[2][c])), hi = [0, 1, 2].map((c) => Math.max(T[0][c], T[1][c], T[2][c]));
      if ([0, 1, 2].every((c) => lo[c] <= p.centre[c] + 0.4 && hi[c] >= p.centre[c] - 0.4)) tris.push(T);
    }
  });
  return pts.map(([x, y]) => {
    const o = [0, 1, 2].map((c) => p.centre[c] + p.X[c] * x + p.U[c] * y + p.n[c] * 0.0008), d = p.n.map((q) => -q);
    let best = Infinity;
    for (const T of tris) {   // Möller-Trumbore, des deux côtés
      const e1 = sub(T[1], T[0]), e2 = sub(T[2], T[0]), q = cross(d, e2), det = dot(e1, q);
      if (Math.abs(det) < 1e-12) continue;
      const s = sub(o, T[0]), u = dot(s, q) / det; if (u < 0 || u > 1) continue;
      const r = cross(s, e1), v = dot(d, r) / det; if (v < 0 || u + v > 1) continue;
      const tt = dot(e2, r) / det; if (tt > -0.002 && tt < best) best = tt;
    }
    return Math.max(0, Math.min(best, JOUR_MAX));
  });
}

if (process.argv[1] && import.meta.url.endsWith(process.argv[1].split('/').pop())) principal();
function principal() {
const ids = process.argv.slice(2).length ? process.argv.slice(2)
  : fs.readdirSync(new URL('assets/cars/', RACINE)).filter((n) => n.endsWith('.wfm')).map((n) => n.slice(0, -4)).sort();
const f3 = (v) => v.map((x) => `${x.toFixed(4)}f`).join(', ');
let echec = 0;
for (const id of ids) {
  const f = lire(new URL(`assets/cars/${id}.wfm`, RACINE));
  const r = placer(f, id);
  if (!r[-1] || !r[1]) { console.error(`// ${id} : plaque ${!r[-1] ? 'avant' : 'arrière'} introuvable`); echec = 1; continue; }
  for (const c of [-1, 1]) {
    const p = r[c];
    p.jours = jours(f, p).map((x) => Math.round(x * 10000) / 10);   // mm, au dixième
    const ok = Math.abs(p.largeur / 0.52 - 1) <= 0.03 && Math.abs(p.hauteur / 0.11 - 1) <= 0.06;   // A 45, Abarth, Giulia : 105 à 112 mm
    if (!ok) echec = 1;
    console.error(`// ${id} ${c < 0 ? 'avant  ' : 'arrière'} : ${(p.largeur * 1000).toFixed(0)} × ${(p.hauteur * 1000).toFixed(0)} mm${ok ? '' : ' (HORS FORMAT)'}, ` +
      `jours ${p.jours.join(' ')} mm`);
  }
  const pose = (p) => `{{${f3(p.centre)}}, {${f3(p.n)}}, {${p.jours.map((x) => `${x.toFixed(1)}f`).join(', ')}}}`;
  console.log(`    {"${id}", ${pose(r[-1])},\n              ${pose(r[1])}},`);
}
process.exitCode = echec;
}
