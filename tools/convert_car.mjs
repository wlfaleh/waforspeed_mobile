// Convertit une voiture du jeu web (glTF JSON compressé meshopt) en fichier binaire .wfm pour le moteur natif.
// usage : node convert_car.mjs entree.json sortie.wfm <longueur_m> <rotateY_deg|auto> <regex_peinture> [regex_feux_arriere]
//
// Le modèle est mis à l'échelle (longueur réelle), posé au sol, centré, avant vers -Z (même convention que le jeu web).
// Les primitives sont aplaties (transformations des nœuds appliquées) et regroupées par matériau : 1 appel de dessin
// par matériau. Les textures WebP sont réencodées en JPEG (opaques) ou PNG (avec transparence), 1024 px max.
//
// Format .wfm (petit-boutiste) :
//   'WFM1' u32 version=1 u32 nTex u32 nMat u32 nMesh f32 bbox[6]
//   textures  : u32 octets, octets (jpg/png)                                   × nTex
//   matériaux : f32 couleur[4] f32 metal f32 rugosite f32 emissif[3] f32 coupureAlpha
//               i32 texCouleur i32 texNormale i32 texMetalRugo u32 drapeaux     × nMat
//   maillages : u32 materiau u32 nSommets u32 nIndices,
//               sommets (pos3 normale3 uv2 = 32 octets), indices u32           × nMesh
//   drapeaux : 1 peinture, 2 transparent, 4 feu arrière, 8 phare, 16 double face, 32 alpha découpé, 64 vitre, 128 roue
import { NodeIO } from '@gltf-transform/core';
import { ALL_EXTENSIONS } from '@gltf-transform/extensions';
import { MeshoptDecoder } from 'meshoptimizer';
import sharp from 'sharp';
import fs from 'fs';

const [,, inp, out, lenArg, rotArg, paintArg, tailArg] = process.argv;
await MeshoptDecoder.ready;
const io = new NodeIO().registerExtensions(ALL_EXTENSIONS).registerDependencies({ 'meshopt.decoder': MeshoptDecoder });
const doc = await io.read(inp);
const root = doc.getRoot();
const L = parseFloat(lenArg);
const rx = {
  paint: paintArg ? new RegExp(paintArg, 'i') : /(car_?paint|paint|body|carros|exterior)/i,
  tail: tailArg ? new RegExp(tailArg, 'i') : /(tail|brake|rear.?light|stop.?light|backlight)/i,
  head: /(head.?light|front.?light|phare)/i,
  glass: /(glass|window|vitre|windshield)/i,
  wheel: /(wheel|tire|tyre|rim|jante|roue)/i,
};

// ---------- matrices ----------
const mul = (a, b) => { const o = new Array(16).fill(0); for (let c = 0; c < 4; c++) for (let r = 0; r < 4; r++) { let s = 0; for (let k = 0; k < 4; k++) s += a[k * 4 + r] * b[c * 4 + k]; o[c * 4 + r] = s; } return o; };
const xf = (m, x, y, z, w) => [m[0] * x + m[4] * y + m[8] * z + m[12] * w, m[1] * x + m[5] * y + m[9] * z + m[13] * w, m[2] * x + m[6] * y + m[10] * z + m[14] * w];

// ---------- collecte des primitives (aplaties) ----------
const prims = [];   // { mat, pos[], nrm[], uv[], idx[], name }
function visit(node, parentM) {
  const m = mul(parentM, node.getWorldMatrix ? node.getMatrix() : node.getMatrix());
  const mesh = node.getMesh();
  if (mesh && !node.getSkin()) {
    for (const p of mesh.listPrimitives()) {
      if (p.getMode() !== 4) continue;   // triangles seulement
      const P = p.getAttribute('POSITION'); if (!P) continue;
      const N = p.getAttribute('NORMAL'), T = p.getAttribute('TEXCOORD_0'), I = p.getIndices();
      const n = P.getCount(), pos = new Float32Array(n * 3), nrm = new Float32Array(n * 3), uv = new Float32Array(n * 2);
      const e = [0, 0, 0];
      // matrice des normales = inverse transposée de la partie 3x3 (approchée par la 3x3 puis normalisation, échelles uniformes)
      for (let i = 0; i < n; i++) {
        P.getElement(i, e); const w = xf(m, e[0], e[1], e[2], 1); pos.set(w, i * 3);
        if (N) { N.getElement(i, e); const v = xf(m, e[0], e[1], e[2], 0); const l = Math.hypot(...v) || 1; nrm.set([v[0] / l, v[1] / l, v[2] / l], i * 3); }
        if (T) { T.getElement(i, e); uv[i * 2] = e[0]; uv[i * 2 + 1] = e[1]; }
      }
      let idx;
      if (I) { idx = new Uint32Array(I.getCount()); for (let i = 0; i < idx.length; i++) idx[i] = I.getScalar(i); }
      else { idx = new Uint32Array(n); for (let i = 0; i < n; i++) idx[i] = i; }
      // déterminant négatif (miroir) : on inverse l'ordre des triangles
      const det = m[0] * (m[5] * m[10] - m[9] * m[6]) - m[4] * (m[1] * m[10] - m[9] * m[2]) + m[8] * (m[1] * m[6] - m[5] * m[2]);
      if (det < 0) for (let t = 0; t < idx.length; t += 3) { const a = idx[t + 1]; idx[t + 1] = idx[t + 2]; idx[t + 2] = a; }
      prims.push({ mat: p.getMaterial(), pos, nrm, uv, idx, hasN: !!N, name: (node.getName() || '') + ' ' + (mesh.getName() || '') });
    }
  }
  for (const c of node.listChildren()) visit(c, m);
}
const I4 = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1];
for (const scene of root.listScenes()) for (const n of scene.listChildren()) visit(n, I4);

// normales manquantes : moyenne des normales des faces
for (const p of prims) if (!p.hasN) {
  p.nrm.fill(0);
  for (let t = 0; t < p.idx.length; t += 3) {
    const [a, b, c] = [p.idx[t], p.idx[t + 1], p.idx[t + 2]];
    const ux = p.pos[b * 3] - p.pos[a * 3], uy = p.pos[b * 3 + 1] - p.pos[a * 3 + 1], uz = p.pos[b * 3 + 2] - p.pos[a * 3 + 2];
    const vx = p.pos[c * 3] - p.pos[a * 3], vy = p.pos[c * 3 + 1] - p.pos[a * 3 + 1], vz = p.pos[c * 3 + 2] - p.pos[a * 3 + 2];
    const nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
    for (const v of [a, b, c]) { p.nrm[v * 3] += nx; p.nrm[v * 3 + 1] += ny; p.nrm[v * 3 + 2] += nz; }
  }
  for (let i = 0; i < p.nrm.length; i += 3) { const l = Math.hypot(p.nrm[i], p.nrm[i + 1], p.nrm[i + 2]) || 1; p.nrm[i] /= l; p.nrm[i + 1] /= l; p.nrm[i + 2] /= l; }
}

// ---------- nettoyage : plans d'ombre plats sous la voiture ----------
const boxOf = (list) => { const b = [1e9, 1e9, 1e9, -1e9, -1e9, -1e9]; for (const p of list) for (const i of p.idx) for (let k = 0; k < 3; k++) { const v = p.pos[i * 3 + k]; if (v < b[k]) b[k] = v; if (v > b[k + 3]) b[k + 3] = v; } return b; };
let list = prims.filter((p) => {
  const b = boxOf([p]), d = [b[3] - b[0], b[4] - b[1], b[5] - b[2]].sort((x, y) => x - y);
  const mn = (p.mat && p.mat.getName()) || '';
  return !(d[2] > 1.5 && d[0] < 0.002 * d[2] && d[1] > 0.4 * d[2] && /plane|shadow|ombre|ground|floor/i.test(p.name + ' ' + mn + ' Plane'));
});

// ---------- orientation, échelle, pose au sol ----------
const rot = (deg) => { const a = deg * Math.PI / 180, c = Math.cos(a), s = Math.sin(a); for (const p of list) for (const arr of [p.pos, p.nrm]) for (let i = 0; i < arr.length; i += 3) { const x = arr[i], z = arr[i + 2]; arr[i] = c * x + s * z; arr[i + 2] = -s * x + c * z; } };
if (rotArg !== 'auto') rot(parseFloat(rotArg) || 0);
let b = boxOf(list);
if (rotArg === 'auto' && b[3] - b[0] > b[5] - b[2]) { rot(90); b = boxOf(list); }
const k = L / Math.max(b[5] - b[2], 1e-3), cx = (b[0] + b[3]) / 2, cz = (b[2] + b[5]) / 2, y0 = b[1];
for (const p of list) for (let i = 0; i < p.pos.length; i += 3) { p.pos[i] = (p.pos[i] - cx) * k; p.pos[i + 1] = (p.pos[i + 1] - y0) * k; p.pos[i + 2] = (p.pos[i + 2] - cz) * k; }
b = boxOf(list);

// ---------- textures ----------
const texIndex = new Map(), texBlobs = [];
async function tex(t, color) {
  if (!t) return -1;
  if (texIndex.has(t)) return texIndex.get(t);
  const img = t.getImage(); if (!img) return -1;
  let s = sharp(Buffer.from(img)), meta = await s.metadata();
  if ((meta.width || 0) > 1024 || (meta.height || 0) > 1024) s = s.resize(1024, 1024, { fit: 'inside' });
  const alpha = color && meta.hasAlpha;
  const buf = alpha ? await s.png({ compressionLevel: 9 }).toBuffer() : await s.jpeg({ quality: 88 }).toBuffer();
  texBlobs.push(buf); texIndex.set(t, texBlobs.length - 1);
  return texBlobs.length - 1;
}

// ---------- matériaux (regroupement des primitives par matériau) ----------
const groups = new Map();   // matériau gltf -> primitives
for (const p of list) { if (!groups.has(p.mat)) groups.set(p.mat, []); groups.get(p.mat).push(p); }
const mats = [], meshes = [];
for (const [m, ps] of groups) {
  const name = (m && m.getName()) || '', names = ps.map((p) => p.name).join(' ');
  let flags = 0;
  const all = (re) => re.test(name) || ps.every((p) => re.test(p.name));
  if (rx.paint.test(name)) flags |= 1;
  else if (all(rx.tail)) flags |= 4;
  else if (all(rx.head)) flags |= 8;
  if (m && m.getAlphaMode() === 'BLEND') flags |= 2;
  if (m && m.getAlphaMode() === 'MASK') flags |= 32;
  if (m && m.getDoubleSided()) flags |= 16;
  if (all(rx.glass)) flags |= 64 | 2;
  if (all(rx.wheel)) flags |= 128;
  const col = m ? m.getBaseColorFactor() : [0.8, 0.8, 0.8, 1];
  const em = m ? m.getEmissiveFactor() : [0, 0, 0];
  mats.push({
    col, metal: m ? m.getMetallicFactor() : 0, rough: m ? m.getRoughnessFactor() : 0.6, em, cut: m ? m.getAlphaCutoff() : 0.5,
    tC: await tex(m && m.getBaseColorTexture(), true), tN: await tex(m && m.getNormalTexture(), false), tMR: await tex(m && m.getMetallicRoughnessTexture(), false), flags, name,
  });
  // fusion des primitives du matériau
  let nv = 0, ni = 0; for (const p of ps) { nv += p.pos.length / 3; ni += p.idx.length; }
  const v = new Float32Array(nv * 8), ix = new Uint32Array(ni); let ov = 0, oi = 0;
  for (const p of ps) {
    const n = p.pos.length / 3;
    for (let i = 0; i < n; i++) { v.set([p.pos[i * 3], p.pos[i * 3 + 1], p.pos[i * 3 + 2], p.nrm[i * 3], p.nrm[i * 3 + 1], p.nrm[i * 3 + 2], p.uv[i * 2], p.uv[i * 2 + 1]], (ov + i) * 8); }
    for (let i = 0; i < p.idx.length; i++) ix[oi + i] = p.idx[i] + ov;
    ov += n; oi += p.idx.length;
  }
  meshes.push({ mat: mats.length - 1, v, ix });
}

// ---------- écriture ----------
const parts = [];
const u32 = (...a) => parts.push(Buffer.from(new Uint32Array(a).buffer));
const i32 = (...a) => parts.push(Buffer.from(new Int32Array(a).buffer));
const f32 = (...a) => parts.push(Buffer.from(new Float32Array(a).buffer));
parts.push(Buffer.from('WFM1')); u32(1, texBlobs.length, mats.length, meshes.length); f32(...b);
for (const t of texBlobs) { u32(t.length); parts.push(t); }
for (const m of mats) { f32(...m.col, m.metal, m.rough, ...m.em, m.cut); i32(m.tC, m.tN, m.tMR); u32(m.flags); }
for (const m of meshes) { u32(m.mat, m.v.length / 8, m.ix.length); parts.push(Buffer.from(m.v.buffer)); parts.push(Buffer.from(m.ix.buffer)); }
fs.writeFileSync(out, Buffer.concat(parts));
const tris = meshes.reduce((s, m) => s + m.ix.length / 3, 0);
console.log(out.split('/').pop(), `${(fs.statSync(out).size / 1e6).toFixed(2)} Mo`, `${mats.length} matériaux`, `${texBlobs.length} textures`, `${Math.round(tris / 1000)}k triangles`,
  `taille ${(b[3] - b[0]).toFixed(2)}×${(b[4] - b[1]).toFixed(2)}×${(b[5] - b[2]).toFixed(2)} m`, 'peinture:', mats.filter((m) => m.flags & 1).map((m) => m.name).join(',') || 'AUCUNE');
