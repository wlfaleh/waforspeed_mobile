// Convertit une voiture du jeu web (glTF JSON compressé meshopt) en fichier binaire .wfm pour le moteur natif.
// usage : node convert_car.mjs entree.json sortie.wfm <longueur_m> <rotateY_deg|auto> <regex_peinture> [regex_feux_arriere]
//
// Le modèle est mis à l'échelle (longueur réelle), posé au sol, centré, avant vers -Z (même convention que le jeu web).
// Les primitives sont aplaties (transformations des nœuds appliquées) et regroupées par matériau : 1 appel de dessin
// par matériau. Les textures WebP sont réencodées en JPEG (opaques) ou PNG (avec transparence), 1024 px max.
//
// Format .wfm (petit-boutiste) :
//   'WFM1' u32 version=2 u32 nTex u32 nMat u32 nMesh f32 bbox[6]   (WFS-149 : version 2 = version 1 + section 'TAN1')
//   textures  : u32 octets, octets (jpg/png)                                   × nTex
//   matériaux : f32 couleur[4] f32 metal f32 rugosite f32 emissif[3] f32 coupureAlpha
//               i32 texCouleur i32 texNormale i32 texMetalRugo u32 drapeaux     × nMat
//   maillages : u32 materiau u32 nSommets u32 nIndices,
//               sommets (pos3 normale3 uv2 = 32 octets), indices u32           × nMesh
//   drapeaux : 1 peinture, 2 transparent, 4 feu arrière, 8 phare, 16 double face, 32 alpha découpé, 64 vitre, 128 roue
//   WFS-149 (RG-04) : 'TAN1' puis, maillage par maillage, nSommets × 4 octets signés (tangente x y z, signe w, × 127),
//               calculés par tools/tangentes.mjs (même calcul que le moteur, src/render/mesh.cpp, type MikkTSpace). Les
//               lecteurs de la version 1 lisent la version 2 sans changement (section après tout le reste) ; un .wfm
//               version 1 reçoit ses tangentes au chargement (model_loader.cpp).
//
// WFS-77 : peinture unie métallisée (repaintMat du jeu web : métal 0,45, rugosité 0,32, ni texture de couleur, ni carte de
// normales, de métal ou de rugosité, sans émission) : les motifs des modèles (camouflage, paillettes étirées) disparaissent.
// WFS_ROUES=forme : pièces en forme de roue (rondes, posées au sol, sur un côté) regroupées à part avec le drapeau 128,
// pour les modèles dont les roues n'ont pas de nom reconnaissable et partagent leur matériau avec la carrosserie (GT86).
// WFS-232 : WFS_OPAQUE=<regex matériau> : matériaux BLEND traités en alpha découpé (surfaces opaques mal exportées).
// WFS-158 (RG-A, M-04) : WFS_PLAQUE=<regex matériau> : plaques avant et arrière remises au format européen 520 × 110 mm ;
// WFS_TEXTURES=« matériau|couleur=fichier[|normale=fichier] ; … » : images du dépôt à la place des textures d'un matériau
// (plaques au nom de l'auteur du modèle, remplacées par les plaques neutres de tools/plaques_neutres.mjs).
import { NodeIO } from '@gltf-transform/core';
import { ALL_EXTENSIONS } from '@gltf-transform/extensions';
import { MeshoptDecoder } from 'meshoptimizer';
import sharp from 'sharp';
import fs from 'fs';
import { tangentes } from './tangentes.mjs';   // WFS-149 (RG-04)

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
// WFS-134 (RG-11) : WFS_EXCLURE=<regex> retire des pièces par nom de matériau (LOD du jeu web : silhouette « wfs_far »)
if (process.env.WFS_EXCLURE) { const rx = new RegExp(process.env.WFS_EXCLURE); list = list.filter((p) => !rx.test((p.mat && p.mat.getName()) || '')); }

// ---------- orientation, échelle, pose au sol ----------
const rot = (deg) => { const a = deg * Math.PI / 180, c = Math.cos(a), s = Math.sin(a); for (const p of list) for (const arr of [p.pos, p.nrm]) for (let i = 0; i < arr.length; i += 3) { const x = arr[i], z = arr[i + 2]; arr[i] = c * x + s * z; arr[i + 2] = -s * x + c * z; } };
if (rotArg !== 'auto') rot(parseFloat(rotArg) || 0);
let b = boxOf(list);
if (rotArg === 'auto' && b[3] - b[0] > b[5] - b[2]) { rot(90); b = boxOf(list); }
const k = L / Math.max(b[5] - b[2], 1e-3), cx = (b[0] + b[3]) / 2, cz = (b[2] + b[5]) / 2, y0 = b[1];
for (const p of list) for (let i = 0; i < p.pos.length; i += 3) { p.pos[i] = (p.pos[i] - cx) * k; p.pos[i + 1] = (p.pos[i + 1] - y0) * k; p.pos[i + 2] = (p.pos[i + 2] - cz) * k; }
b = boxOf(list);

// ---------- WFS-158 (RG-A, M-04) : plaques au format européen 520 × 110 mm ----------
// Chaque plaque (avant : z < 0, arrière : z > 0) est étirée dans son propre plan, autour de son centre : largeur selon x,
// hauteur selon la verticale du plan ; épaisseur, inclinaison et normales inchangées. Seules les pièces du matériau désigné
// bougent (une plaque est une pièce rapportée : ce n'est pas une échelle par axe de la voiture, interdite par RG-01).
const PLAQUE_L = 0.520, PLAQUE_H = 0.110;
const plaquesMesurees = [];
if (process.env.WFS_PLAQUE) {
  const rxP = new RegExp(process.env.WFS_PLAQUE);
  const dot = (a, c) => a[0] * c[0] + a[1] * c[1] + a[2] * c[2];
  const unit = (a) => { const l = Math.hypot(a[0], a[1], a[2]) || 1; return [a[0] / l, a[1] / l, a[2] / l]; };
  for (const cote of [-1, 1]) {
    const pts = [];
    for (const p of list) if (rxP.test((p.mat && p.mat.getName()) || '')) for (let i = 0; i < p.pos.length / 3; i++) if (Math.sign(p.pos[i * 3 + 2]) === cote) pts.push([p, i]);
    if (!pts.length) continue;
    // normale du plan : faces avant et arrière de la plaque (|nz| > 0,5), retournées vers l'extérieur de la voiture
    let n = [0, 0, 0];
    for (const [p, i] of pts) { const nz = p.nrm[i * 3 + 2]; if (Math.abs(nz) > 0.5) { const sg = Math.sign(nz) === cote ? 1 : -1; for (let c = 0; c < 3; c++) n[c] += sg * p.nrm[i * 3 + c]; } }
    n = unit(n);
    const X = unit([1 - n[0] * n[0], -n[0] * n[1], -n[0] * n[2]]);                       // x projeté dans le plan
    let U = unit([n[1] * X[2] - n[2] * X[1], n[2] * X[0] - n[0] * X[2], n[0] * X[1] - n[1] * X[0]]);
    if (U[1] < 0) U = [-U[0], -U[1], -U[2]];                                                   // verticale du plan, vers le haut
    const q = pts.map(([p, i]) => [p.pos[i * 3], p.pos[i * 3 + 1], p.pos[i * 3 + 2]]);
    const lo = [0, 1, 2].map((c) => Math.min(...q.map((v) => v[c]))), hi = [0, 1, 2].map((c) => Math.max(...q.map((v) => v[c])));
    const ctr = [0, 1, 2].map((c) => (lo[c] + hi[c]) / 2);
    const a = q.map((v) => dot([v[0] - ctr[0], v[1] - ctr[1], v[2] - ctr[2]], X)), h = q.map((v) => dot([v[0] - ctr[0], v[1] - ctr[1], v[2] - ctr[2]], U));
    const largeur = Math.max(...a) - Math.min(...a), hauteur = Math.max(...h) - Math.min(...h);
    const sx = PLAQUE_L / largeur, sy = PLAQUE_H / hauteur, a0 = (Math.max(...a) + Math.min(...a)) / 2, h0 = (Math.max(...h) + Math.min(...h)) / 2;
    pts.forEach(([p, i], j) => {
      const d = [q[j][0] - ctr[0], q[j][1] - ctr[1], q[j][2] - ctr[2]], e = dot(d, n);
      const na = a0 + (a[j] - a0) * sx, nh = h0 + (h[j] - h0) * sy;
      for (let c = 0; c < 3; c++) p.pos[i * 3 + c] = ctr[c] + X[c] * na + U[c] * nh + n[c] * e;
    });
    plaquesMesurees.push(`${cote < 0 ? 'avant' : 'arrière'} ${(largeur * 1000).toFixed(0)}×${(hauteur * 1000).toFixed(0)} → ${PLAQUE_L * 1000}×${PLAQUE_H * 1000} mm`);
  }
  b = boxOf(list);
}

// ---------- WFS-77 : roues reconnues à leur forme (looksLikeWheel du jeu web, en mètres, après la mise à l'échelle) ----------
let nRouesForme = 0;
if (process.env.WFS_ROUES === 'forme') for (const p of list) {
  const bb = boxOf([p]), sx = bb[3] - bb[0], sy = bb[4] - bb[1], sz = bb[5] - bb[2], cxp = (bb[0] + bb[3]) / 2;
  p.roue = bb[1] < 0.1 && sy > 0.35 && sy < 0.9 && Math.abs(sy - sz) < 0.05 && sx < 0.5 && Math.abs(cxp) > 0.3;
  if (p.roue) nRouesForme++;
}

// ---------- textures ----------
// WFS-158 (RG-A) : WFS_TEXTURES : une image du dépôt (chemin depuis tools/) prend la place de la texture du matériau, au même
// rang : le nombre et l'ordre des textures et des matériaux ne changent pas (src/render/correctifs_voitures.h les désigne par rang)
const remplacees = new Map();
for (const r of (process.env.WFS_TEXTURES || '').split(';').map((x) => x.trim()).filter(Boolean)) {
  const [mat, ...ops] = r.split('|'), o = {};
  for (const op of ops) { const [cle, f] = op.split('='); o[cle] = f; }
  remplacees.set(mat, o);
}
const texIndex = new Map(), texBlobs = [];
async function tex(t, color, fichier) {
  if (!t) return -1;
  const cle = fichier ? `fichier:${fichier}` : t;
  if (texIndex.has(cle)) return texIndex.get(cle);
  const img = fichier ? fs.readFileSync(fichier) : t.getImage(); if (!img) return -1;
  let s = sharp(Buffer.from(img)), meta = await s.metadata();
  if ((meta.width || 0) > 1024 || (meta.height || 0) > 1024) s = s.resize(1024, 1024, { fit: 'inside' });
  const alpha = color && meta.hasAlpha;
  const buf = alpha ? await s.png({ compressionLevel: 9 }).toBuffer() : await s.jpeg({ quality: 88 }).toBuffer();
  texBlobs.push(buf); texIndex.set(cle, texBlobs.length - 1);
  return texBlobs.length - 1;
}

// ---------- matériaux (regroupement des primitives par matériau) ----------
const groups = new Map();   // clé (matériau gltf, ou matériau + « roue » pour WFS_ROUES=forme) -> primitives
const cleRoue = new Map();
for (const p of list) {
  let k = p.mat;
  if (p.roue) { if (!cleRoue.has(p.mat)) cleRoue.set(p.mat, { roue: true, mat: p.mat }); k = cleRoue.get(p.mat); }
  if (!groups.has(k)) groups.set(k, []); groups.get(k).push(p);
}
const mats = [], meshes = [];
for (const [cle, ps] of groups) {
  const m = cle && cle.roue ? cle.mat : cle, estRoue = !!(cle && cle.roue);
  const name = ((m && m.getName()) || '') + (estRoue ? ' (roue)' : ''), names = ps.map((p) => p.name).join(' ');
  let flags = 0;
  const all = (re) => re.test(name) || ps.every((p) => re.test(p.name));
  if (rx.paint.test(name) && !estRoue) flags |= 1;
  else if (all(rx.tail)) flags |= 4;
  else if (all(rx.head)) flags |= 8;
  // WFS-232 : WFS_OPAQUE=<regex matériau> : surfaces opaques exportées en BLEND par Sketchfab (carénages, moteur des deux-roues) :
  // alpha découpé (bords nets des autocollants) au lieu de transparent (file triée, jamais allégé par alleger_voiture.mjs)
  const opaque = process.env.WFS_OPAQUE && new RegExp(process.env.WFS_OPAQUE, 'i').test(name);
  if (m && m.getAlphaMode() === 'BLEND' && !opaque) flags |= 2;
  if (m && (m.getAlphaMode() === 'MASK' || (opaque && m.getAlphaMode() === 'BLEND'))) flags |= 32;
  if (m && m.getDoubleSided()) flags |= 16;
  if (all(rx.glass)) flags |= 64 | 2;
  if (all(rx.wheel) || estRoue) flags |= 128;
  const col = m ? m.getBaseColorFactor() : [0.8, 0.8, 0.8, 1];
  const em = m ? m.getEmissiveFactor() : [0, 0, 0];
  const unie = (flags & 1) !== 0;   // WFS-77 : peinture unie métallisée (repaintMat)
  const rem = (!estRoue && remplacees.get((m && m.getName()) || '')) || {};   // WFS-158 (RG-A)
  // WFS-77 : feu arrière blanc sans texture (le moteur ne le teinte pas) → rouge des feux du jeu web (0xff2040), lueur faible
  const feuBlanc = (flags & 4) && !(m && m.getBaseColorTexture()) && Math.min(col[0], col[1], col[2]) > 0.8;
  if (feuBlanc) { col[0] = 1; col[1] = 0.0123; col[2] = 0.0513; em[0] = 0.4; em[1] = 0.005; em[2] = 0.02; }
  mats.push({
    col, metal: unie ? 0.45 : m ? m.getMetallicFactor() : 0, rough: unie ? 0.32 : m ? m.getRoughnessFactor() : 0.6, em: unie ? [0, 0, 0] : em,
    cut: m ? m.getAlphaCutoff() : 0.5,
    tC: unie ? -1 : await tex(m && m.getBaseColorTexture(), true, rem.couleur), tN: unie ? -1 : await tex(m && m.getNormalTexture(), false, rem.normale),
    tMR: unie ? -1 : await tex(m && m.getMetallicRoughnessTexture(), false), flags, name,
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
parts.push(Buffer.from('WFM1')); u32(2, texBlobs.length, mats.length, meshes.length); f32(...b);   // WFS-149 : version 2
for (const t of texBlobs) { u32(t.length); parts.push(t); }
for (const m of mats) { f32(...m.col, m.metal, m.rough, ...m.em, m.cut); i32(m.tC, m.tN, m.tMR); u32(m.flags); }
for (const m of meshes) { u32(m.mat, m.v.length / 8, m.ix.length); parts.push(Buffer.from(m.v.buffer)); parts.push(Buffer.from(m.ix.buffer)); }
parts.push(Buffer.from('TAN1'));   // WFS-149 (RG-04) : tangentes, après les normales et l'orientation finales
let sansUv = 0;
for (const m of meshes) { const t = tangentes(m.v, 8, m.ix); sansUv += t.sans; parts.push(Buffer.from(t.octets.buffer)); }
fs.writeFileSync(out, Buffer.concat(parts));
const tris = meshes.reduce((s, m) => s + m.ix.length / 3, 0);
console.log(out.split('/').pop(), `${(fs.statSync(out).size / 1e6).toFixed(2)} Mo`, `${mats.length} matériaux`, `${texBlobs.length} textures`, `${Math.round(tris / 1000)}k triangles`,
  `taille ${(b[3] - b[0]).toFixed(2)}×${(b[4] - b[1]).toFixed(2)}×${(b[5] - b[2]).toFixed(2)} m`, 'peinture:', mats.filter((m) => m.flags & 1).map((m) => m.name).join(',') || 'AUCUNE',
  ...(process.env.WFS_ROUES === 'forme' ? ['roues (forme):', `${nRouesForme} pièces`] : []), `tangentes : ${sansUv} sommets sans uv`,
  ...(plaquesMesurees.length ? ['plaques :', plaquesMesurees.join(', ')] : []), ...(remplacees.size ? ['textures remplacées :', [...remplacees.keys()].join(', ')] : []));
