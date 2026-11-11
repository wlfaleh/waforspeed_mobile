// WaForSpeed — WFS-193 (voitures sans marque, phase 3/4) : petites retouches de forme des 20 voitures, appliquées directement aux
// assets/cars/<id>.wfm (les sources glTF ne sont pas dans le dépôt et 4 voitures ne se reconvertissent pas : même méthode que
// tools/effacer_logos.mjs de WFS-192). Données : tools/formes.json. Rejouable : chaque voiture porte l'empreinte de ses sommets
// avant et après ; l'outil s'arrête sans rien écrire si l'état n'est ni l'un ni l'autre.
//
// Usage (depuis la racine du dépôt) :
//   node tools/retoucher_formes.mjs --lister <id>        pièces de la voiture : maillage, matériau, drapeaux, couleur, triangles, boîte
//   node tools/retoucher_formes.mjs --zone <id> x0 y0 z0 x1 y1 z1   sommets de chaque maillage dans la boîte (repérage)
//   node tools/retoucher_formes.mjs [--verifier] [id …]  applique (ou contrôle seulement) les retouches de formes.json
//
// Sortes de retouches (détail plus bas) :
//   - déplacement : déplace les sommets d'UN maillage (un maillage par matériau) qui tombent dans une boîte, sans créer ni retirer
//     de sommet ni de triangle ; « fondu » (m) : les sommets hors de la boîte, à moins de « fondu », suivent en partie ;
//     normales transformées par l'inverse de l'échelle ; le .lod (indices seuls) reste valable et prend la nouvelle forme ;
//   - retrait : vide des triangles (indices confondus) ; aucun sommet retiré ;
//   - découpe (WFS-358) : retire EXACTEMENT la partie d'un maillage dans un contour vu de face (triangles coupés, bord net) ;
//   - bouchon (WFS-358) : retire un ancien logo en creux et recoud la peau (étanche) ;
//   - grille : AJOUTE un matériau (à la fin) et un maillage ;
//   - grille ajustée (WFS-358) : AJOUTE deux matériaux (lames, fond) ; profondeur qui suit la carrosserie, jamais devant elle ;
//   - panneau (WFS-362) : referme une ancienne ouverture par une peau ajoutée au maillage de la carrosserie (même peinture) ;
//   - couture (WFS-363) : referme une ouverture par une peau COUSUE au bord découpé de la carrosserie (mêmes positions : aucun joint) ;
//   - lissage (WFS-364) : efface un petit relief d'une peau (hauteur y ajustée rangée par rangée sur la peau voisine), sans créer de sommet.
//   Après un retrait, une découpe, un bouchon ou une grille, le .lod est régénéré (tools/alleger_voiture.mjs). Dans tous les cas :
//   tangentes (TAN1) des maillages touchés recalculées (tools/tangentes.mjs), boîte d'en-tête recalculée.
//
// formes.json : { "<id>": { "materiaux": nMat, "empreinteAvant": "…", "empreinteApres": "…", "retouches": [ … ] } }
// Retouches (dans l'ordre) :
//   déplacement  { "materiau": m, "boite": [...], "pivot": [x,y,z] | "boite", "echelle": [sx,sy,sz], "deplacer": [dx,dy,dz], "fondu": f, "sommets": n }
//   retrait      { "retirer": true, "materiau": m, "boite": [...], "triangles": n } : triangles dont le centre est dans la boîte
//                vidés (indices confondus, comme tools/plaques_voitures.mjs), aucun sommet retiré (accord du PO, WFS-193 (A))
//   grille       { "grille": [x0,y0,z0,x1,y1,z1], "barres": n, "galbe": g, "trapeze": t, "cadre": c, "couleur": "#rrggbb",
//                  "metal": m, "rugosite": r } : grille générique à la place d'une calandre de marque (fond, cadre en relief,
//                barres fermées, galbe du nez, trapèze) ; un matériau ajouté à la FIN (rang des matériaux inchangé,
//                src/render/correctifs_voitures.h) et un maillage.
//   découpe      { "decouper": true, "materiau": m, "contour": [[x,y],…] | { "rectangle": [x0,y0,x1,y1], "rayons": [bas, haut], "pas": n },
//                  "z": [z0, z1], "triangles": n } : contour convexe vu de face (rayons : coins arrondis) ; n = triangles touchés
//   bouchon      { "boucher": true, "materiau": m, "centre": [x,y], "rayons": [rx,ry], "seuil": s, "z": [z0,z1], "triangles": n }
//   grille ajustée { "grilleAjustee": { "contour", "z", "ignorer", "recul", "fond", "lames", "lame": {…}, couleurs }, … } (voir plus bas)
//   panneau      { "panneau": { "materiau", "contour", "z", "ignorer", "recul", "debord", "bombe" }, "triangles": n } (voir plus bas)
//   couture      { "couture": { "materiau", "contour", "z", "ignorer", "bande", "baseFace" }, "triangles": n } (WFS-363, voir plus bas)
//   lissage      { "lisser": { "materiau", "boite", "appui", "pas", "fondu" }, "sommets": n } (WFS-364, voir plus bas)
// Chaque retouche porte une « note ».
// « sommets » : nombre de sommets dans la boîte (contrôle à la 1re application). Repère du .wfm : avant vers −Z, x à droite, y en haut.
import fs from 'fs';
import crypto from 'crypto';
import path from 'path';
import { tangentes } from './tangentes.mjs';
import { execFileSync } from 'child_process';

const ICI = path.dirname(new URL(import.meta.url).pathname);
const RACINE = path.resolve(ICI, '..');
const FORMES = JSON.parse(fs.readFileSync(path.join(ICI, 'formes.json'), 'utf8'));

// ---------- lecture / écriture d'un .wfm (même format que tools/effacer_logos.mjs) ----------
function lire(b) {
  let o = 0;
  const u32 = () => { const v = b.readUInt32LE(o); o += 4; return v; };
  if (b.toString('latin1', 0, 4) !== 'WFM1') throw new Error('format inconnu');
  o = 4;
  const version = u32(), nT = u32(), nM = u32(), nMe = u32();
  const boite = Buffer.from(b.subarray(o, o + 24)); o += 24;
  const textures = [];
  for (let i = 0; i < nT; i++) { const n = u32(); textures.push(Buffer.from(b.subarray(o, o + n))); o += n; }
  const materiaux = Buffer.from(b.subarray(o, o + 56 * nM)); o += 56 * nM;
  const maillages = [];
  for (let i = 0; i < nMe; i++) {
    const mat = u32(), nv = u32(), ni = u32();
    const v = new Float32Array(b.buffer.slice(b.byteOffset + o, b.byteOffset + o + nv * 32)); o += nv * 32;
    const ix = new Uint32Array(b.buffer.slice(b.byteOffset + o, b.byteOffset + o + ni * 4)); o += ni * 4;
    maillages.push({ mat, nv, v, ix });
  }
  let tan = null;
  if (version >= 2 && o + 4 <= b.length && b.toString('latin1', o, o + 4) === 'TAN1') {
    let p = o + 4; tan = [];
    for (const m of maillages) { tan.push(Buffer.from(b.subarray(p, p + m.nv * 4))); p += m.nv * 4; }
    if (p !== b.length) throw new Error(`section TAN1 de ${b.length - o} octets inattendue`);
  } else if (o !== b.length) throw new Error(`${b.length - o} octets inattendus après les maillages`);
  return { version, boite, textures, nM, materiaux, maillages, tan };
}
function ecrire(w) {
  const parts = [Buffer.from('WFM1')];
  const u32 = (...a) => parts.push(Buffer.from(new Uint32Array(a).buffer));
  u32(w.version, w.textures.length, w.nM, w.maillages.length); parts.push(Buffer.from(w.boite));
  for (const t of w.textures) { u32(t.length); parts.push(t); }
  parts.push(Buffer.from(w.materiaux));
  for (const m of w.maillages) { u32(m.mat, m.nv, m.ix.length); parts.push(Buffer.from(m.v.buffer)); parts.push(Buffer.from(m.ix.buffer)); }
  if (w.tan) { parts.push(Buffer.from('TAN1')); for (const t of w.tan) parts.push(Buffer.from(t)); }
  return Buffer.concat(parts);
}
// empreinte des sommets de toute la voiture (positions et normales) : état avant / après
const empreinte = (w) => { const h = crypto.createHash('sha256'); h.update(Buffer.from(w.materiaux)); for (const m of w.maillages) { h.update(Buffer.from(m.v.buffer)); h.update(Buffer.from(m.ix.buffer)); } return h.digest('hex').slice(0, 16); };
const DRAPEAUX = ['peinture', 'transparent', 'feu-arr', 'phare', 'double', 'decoupe', 'vitre', 'roue'];

function lister(id) {
  const w = lire(fs.readFileSync(path.join(RACINE, 'assets/cars', id + '.wfm')));
  console.log(`${id} : ${w.textures.length} textures, ${w.nM} matériaux, ${w.maillages.length} maillages, empreinte ${empreinte(w)}`);
  w.maillages.forEach((m, k) => {
    const mo = m.mat * 56, f = (i) => w.materiaux.readFloatLE(mo + 4 * i);
    const dr = w.materiaux.readUInt32LE(mo + 52), tc = w.materiaux.readInt32LE(mo + 40);
    const col = [0, 1, 2].map((i) => Math.round(Math.min(1, Math.max(0, f(i))) * 255).toString(16).padStart(2, '0')).join('');
    const bb = [1e9, 1e9, 1e9, -1e9, -1e9, -1e9];
    for (let i = 0; i < m.nv; i++) for (let c = 0; c < 3; c++) { const x = m.v[i * 8 + c]; bb[c] = Math.min(bb[c], x); bb[c + 3] = Math.max(bb[c + 3], x); }
    const noms = DRAPEAUX.filter((_, i) => dr & (1 << i)).join(',');
    console.log(`  maillage ${String(k).padStart(2)} mat ${String(m.mat).padStart(2)} #${col} tex ${String(tc).padStart(2)} ${noms.padEnd(22)} ${String(m.ix.length / 3).padStart(6)} tri  ` +
      `x ${bb[0].toFixed(3)}..${bb[3].toFixed(3)} y ${bb[1].toFixed(3)}..${bb[4].toFixed(3)} z ${bb[2].toFixed(3)}..${bb[5].toFixed(3)}`);
  });
}

// poids d'un point pour une boîte avec fondu : 1 dedans, décroît linéairement jusqu'à 0 à « fondu » des faces
function poids(p, b, fondu) {
  let d = 0;
  for (let c = 0; c < 3; c++) d = Math.max(d, b[c] - p[c], p[c] - b[c + 3]);
  if (d <= 0) return 1;
  return fondu > 0 && d < fondu ? 1 - d / fondu : 0;
}

function appliquer(w, r) {
  const k = w.maillages.findIndex((m) => m.mat === r.materiau);
  if (k < 0 || w.maillages.filter((m) => m.mat === r.materiau).length !== 1) throw new Error(`matériau ${r.materiau} : un maillage attendu`);
  const m = w.maillages[k], b = r.boite;
  const piv = r.pivot === 'boite' || !r.pivot ? [(b[0] + b[3]) / 2, (b[1] + b[4]) / 2, (b[2] + b[5]) / 2] : r.pivot;
  const s = r.echelle || [1, 1, 1], d = r.deplacer || [0, 0, 0], fondu = r.fondu || 0;
  let dedans = 0;
  for (let i = 0; i < m.nv; i++) {
    const o = i * 8, p = [m.v[o], m.v[o + 1], m.v[o + 2]];
    const a = poids(p, b, fondu);
    if (a <= 0) continue;
    if (a === 1) dedans++;
    for (let c = 0; c < 3; c++) m.v[o + c] = p[c] + a * ((piv[c] + s[c] * (p[c] - piv[c]) + d[c]) - p[c]);
    // normale : inverse de l'échelle (mélangée selon le poids), renormée
    const n = [0, 1, 2].map((c) => m.v[o + 3 + c] / (1 + a * (s[c] - 1)));
    const l = Math.hypot(...n);
    if (l > 1e-12) for (let c = 0; c < 3; c++) m.v[o + 3 + c] = n[c] / l;
  }
  return { k, dedans };
}

function retirer(w, r) {
  const ks = w.maillages.map((m, k) => (m.mat === r.materiau ? k : -1)).filter((k) => k >= 0);
  if (ks.length !== 1) throw new Error(`matériau ${r.materiau} : un maillage attendu`);
  const m = w.maillages[ks[0]], b = r.boite; let n = 0;
  for (let t = 0; t + 2 < m.ix.length; t += 3) {
    if (m.ix[t] === m.ix[t + 1] && m.ix[t] === m.ix[t + 2]) continue;   // déjà vide
    const c = [0, 1, 2].map((q) => (m.v[m.ix[t] * 8 + q] + m.v[m.ix[t + 1] * 8 + q] + m.v[m.ix[t + 2] * 8 + q]) / 3);
    if (poids(c, b, 0) < 1) continue;
    m.ix[t + 1] = m.ix[t + 2] = m.ix[t]; n++;
  }
  return { k: ks[0], n };
}

// lissage (WFS-364, RG-SUIVI-404 : petit relief au centre du capot de la Corsa) : les sommets d'UN maillage dont (x, z) tombent dans
// la boîte prennent la hauteur (y) de la peau voisine, ajustée rangée par rangée : pour chaque sommet, y = c0 + c1·x + c2·x² + c3·Δz
// est ajusté (moindres carrés) sur les sommets du même maillage situés de part et d'autre de la boîte en x (bande « appui » m), à
// moins de « pas » m en z (Δz = z − z du sommet) ; la courbure d'avant en arrière du capot est donc gardée exactement, seule la
// bosse disparaît. Normale = celle de la surface ajustée (−∂y/∂x, 1, −∂y/∂z), dans le sens de l'ancienne. « fondu » (m) : en z,
// au-delà de la boîte, les sommets suivent en partie (poids linéaire). Aucun sommet ni triangle créé ou retiré : le .lod reste valable.
// { "lisser": { "materiau": m, "boite": [x0, y0, z0, x1, y1, z1], "appui": a, "pas": p, "fondu": f }, "note", "sommets": n }
function lisser(w, r) {
  const L = r.lisser;
  const ks = w.maillages.map((m, k) => (m.mat === L.materiau ? k : -1)).filter((k) => k >= 0);
  if (ks.length !== 1) throw new Error(`matériau ${L.materiau} : un maillage attendu`);
  const m = w.maillages[ks[0]], [x0, y0, z0, x1, y1, z1] = L.boite, ap = L.appui ?? 0.06, pas = L.pas ?? 0.008, fondu = L.fondu ?? 0;
  const v0 = Float32Array.from(m.v);   // positions d'origine (l'appui est lu avant toute écriture)
  // appui : la peau d'origine échantillonnée vue de dessus (cellules de 5 mm, y le plus haut dans [y0, y1]), dans les deux bandes
  // de part et d'autre de la boîte : un capot maillé en grands triangles n'a pas assez de sommets dans la bande
  const c5 = 0.005, gx0 = x0 - ap, gz0 = z0 - fondu - pas, NX = Math.ceil((x1 + ap - gx0) / c5) + 1, NZ = Math.ceil((z1 + fondu + pas - gz0) / c5) + 1;
  const Y = new Float64Array(NX * NZ).fill(-Infinity);
  for (let t = 0; t + 2 < m.ix.length; t += 3) {
    const c = [m.ix[t], m.ix[t + 1], m.ix[t + 2]]; if (c[0] === c[1] && c[1] === c[2]) continue;
    const P = c.map((i) => [(v0[i * 8] - gx0) / c5, (v0[i * 8 + 2] - gz0) / c5, v0[i * 8 + 1]]);
    if (P.some((p) => p[2] < y0 || p[2] > y1)) continue;
    const d = (P[1][0] - P[0][0]) * (P[2][1] - P[0][1]) - (P[2][0] - P[0][0]) * (P[1][1] - P[0][1]);
    if (Math.abs(d) < 1e-12) continue;
    const i0 = Math.max(0, Math.ceil(Math.min(P[0][0], P[1][0], P[2][0]))), i1 = Math.min(NX - 1, Math.floor(Math.max(P[0][0], P[1][0], P[2][0])));
    const j0 = Math.max(0, Math.ceil(Math.min(P[0][1], P[1][1], P[2][1]))), j1 = Math.min(NZ - 1, Math.floor(Math.max(P[0][1], P[1][1], P[2][1])));
    for (let j = j0; j <= j1; j++) for (let i = i0; i <= i1; i++) {
      const l1 = ((P[1][0] - i) * (P[2][1] - j) - (P[2][0] - i) * (P[1][1] - j)) / d, l2 = ((P[2][0] - i) * (P[0][1] - j) - (P[0][0] - i) * (P[2][1] - j)) / d, l3 = 1 - l1 - l2;
      if (l1 < -1e-9 || l2 < -1e-9 || l3 < -1e-9) continue;
      const y = l1 * P[0][2] + l2 * P[1][2] + l3 * P[2][2]; if (y > Y[j * NX + i]) Y[j * NX + i] = y;
    }
  }
  const appui = [];
  for (let j = 0; j < NZ; j++) for (let i = 0; i < NX; i++) {
    const x = gx0 + i * c5, z = gz0 + j * c5, y = Y[j * NX + i];
    if (Number.isFinite(y) && (x < x0 || x > x1)) appui.push([x, y, z]);
  }
  // appui rangé par z croissant (boucle j extérieure) : recherche du premier point à z ≥ zmin par dichotomie
  const premier = (z) => { let lo = 0, hi = appui.length; while (lo < hi) { const mi = (lo + hi) >> 1; if (appui[mi][2] < z) lo = mi + 1; else hi = mi; } return lo; };
  let dedans = 0, manque = 0;
  for (let i = 0; i < m.nv; i++) {
    const o = i * 8, x = v0[o], y = v0[o + 1], z = v0[o + 2];
    if (x < x0 || x > x1 || y < y0 || y > y1) continue;
    const dz = Math.max(z0 - z, z - z1, 0);
    if (dz > fondu || (dz > 0 && fondu <= 0)) continue;
    const a = dz > 0 ? 1 - dz / fondu : 1;
    // ajustement de la rangée : base (1, x, x², Δz), au moins 3 points de chaque côté
    const A = [[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]], B = [0, 0, 0, 0]; let ng = 0, nd = 0;
    for (let j = premier(z - pas); j < appui.length && appui[j][2] <= z + pas; j++) {
      const [px, py, pz] = appui[j], f = [1, px, px * px, pz - z];
      if (px < x0) ng++; else nd++;
      for (let p = 0; p < 4; p++) { B[p] += f[p] * py; for (let q = 0; q < 4; q++) A[p][q] += f[p] * f[q]; }
    }
    if (ng < 3 || nd < 3) { manque++; continue; }
    const c = resoudre(A, B);
    const yf = c[0] + c[1] * x + c[2] * x * x;
    m.v[o + 1] = y + a * (yf - y);
    let n = unite([-(c[1] + 2 * c[2] * x), 1, -c[3]]);
    if (n[0] * v0[o + 3] + n[1] * v0[o + 4] + n[2] * v0[o + 5] < 0) n = n.map((q) => -q);
    const nb = unite([0, 1, 2].map((q) => v0[o + 3 + q] + a * (n[q] - v0[o + 3 + q])));
    for (let q = 0; q < 3; q++) m.v[o + 3 + q] = nb[q];
    if (a === 1) dedans++;
  }
  return { k: ks[0], dedans, manque };
}

// grille générique (WFS-193, revue du lead dev sur #308) : fond, cadre en relief et barres horizontales fermées, qui suivent le galbe
// du nez. Boîte [x0,y0,z0,x1,y1,z1] : z0 = avant des barres et du cadre, z1 = fond (au centre) ; « galbe » (m) : recul du fond, des
// barres et du cadre aux bords gauche et droit (z + galbe × u², u = −1..1 en largeur) ; « trapeze » : largeur du bas / largeur du
// haut (1 : rectangle) ; « cadre » : largeur du cadre (m) ; « barres » : nombre de barres. Surfaces découpées en 16 colonnes.
function grille(w, r) {
  const [x0, y0, z0, x1, y1, z1] = r.grille;
  const NX = 16, cx = (x0 + x1) / 2, hwH = (x1 - x0) / 2, hwB = hwH * (r.trapeze ?? 1), g = r.galbe ?? 0, ca = r.cadre ?? 0.015;
  const V = [], I = [];
  const hw = (y) => hwB + (hwH - hwB) * ((y - y0) / (y1 - y0));
  const zg = (x, z) => z + g * Math.pow((x - cx) / hwH, 2);
  const nf = (x) => { const d = 2 * g * (x - cx) / (hwH * hwH), l = Math.hypot(d, 1); return [d / l, 0, -1 / l]; };   // normale de face (vers −z)
  const s = (x, y, z, n) => { V.push(x, y, z, n[0], n[1], n[2], (x - x0) / (x1 - x0), (y - y0) / (y1 - y0)); return V.length / 8 - 1; };
  const quad = (a, b, c, d) => I.push(a, b, c, a, c, d);
  // bande entre deux lignes horizontales (y bas → y haut) à la profondeur z (avant), insettées de « ins » sur chaque bord
  const face = (ya, yb, z, ins) => {
    for (let k = 0; k < NX; k++) {
      const t0 = k / NX, t1 = (k + 1) / NX;
      const xa0 = cx - hw(ya) + ins + t0 * 2 * (hw(ya) - ins), xa1 = cx - hw(ya) + ins + t1 * 2 * (hw(ya) - ins);
      const xb0 = cx - hw(yb) + ins + t0 * 2 * (hw(yb) - ins), xb1 = cx - hw(yb) + ins + t1 * 2 * (hw(yb) - ins);
      quad(s(xa0, ya, zg(xa0, z), nf(xa0)), s(xb0, yb, zg(xb0, z), nf(xb0)), s(xb1, yb, zg(xb1, z), nf(xb1)), s(xa1, ya, zg(xa1, z), nf(xa1)));
    }
  };
  // côté horizontal d'un relief (dessus n = +y, dessous n = −y) entre z0 et z1, le long de y
  const tranche = (y, ins, haut) => {
    const n = haut ? [0, 1, 0] : [0, -1, 0];
    for (let k = 0; k < NX; k++) {
      const xa = cx - hw(y) + ins + (k / NX) * 2 * (hw(y) - ins), xb = cx - hw(y) + ins + ((k + 1) / NX) * 2 * (hw(y) - ins);
      const a = s(xa, y, zg(xa, z0), n), b = s(xb, y, zg(xb, z0), n), c = s(xb, y, zg(xb, z1), n), d = s(xa, y, zg(xa, z1), n);
      if (haut) quad(a, d, c, b); else quad(a, b, c, d);
    }
  };
  // bout fermé d'un relief (côté gauche ou droit) entre ya et yb, à x = bord insetté
  const bout = (ya, yb, ins, gauche) => {
    const xa = gauche ? cx - hw(ya) + ins : cx + hw(ya) - ins, xb = gauche ? cx - hw(yb) + ins : cx + hw(yb) - ins;
    const n = gauche ? [-1, 0, 0] : [1, 0, 0];
    const a = s(xa, ya, zg(xa, z0), n), b = s(xb, yb, zg(xb, z0), n), c = s(xb, yb, zg(xb, z1), n), d = s(xa, ya, zg(xa, z1), n);
    if (gauche) quad(a, d, c, b); else quad(a, b, c, d);
  };
  // fond (à z1, un peu en retrait derrière le cadre)
  face(y0, y1, z1 + 0.002, 0);
  // cadre : bandes haute et basse (face avant + tranches), montants gauche et droit (face avant + côtés)
  face(y1 - ca, y1, z0, 0); tranche(y1 - ca, ca, false);
  face(y0, y0 + ca, z0, 0); tranche(y0 + ca, ca, true);
  for (const gauche of [true, false]) {
    const xs = (y, ins) => (gauche ? cx - hw(y) + ins : cx + hw(y) - ins);
    const n0 = nf(xs(y0, 0));
    quad(s(xs(y0, 0), y0, zg(xs(y0, 0), z0), n0), s(xs(y1, 0), y1, zg(xs(y1, 0), z0), n0), s(xs(y1, ca), y1, zg(xs(y1, ca), z0), n0), s(xs(y0, ca), y0, zg(xs(y0, ca), z0), n0));
    bout(y0, y1, 0, gauche); bout(y0, y1, ca, !gauche);
  }
  // barres : n pavés horizontaux fermés (face avant, dessus, dessous, deux bouts), épaisseur 40 % du pas, entre les montants du cadre
  const n = r.barres || 0, pas = (y1 - y0 - 2 * ca) / (n + 1), e = pas * 0.4;
  for (let k = 1; k <= n; k++) {
    const yc = y0 + ca + k * pas, ya = yc - e / 2, yb = yc + e / 2;
    face(ya, yb, z0, ca); tranche(yb, ca, true); tranche(ya, ca, false); bout(ya, yb, ca, true); bout(ya, yb, ca, false);
  }
  // matériau ajouté à la fin : couleur, métal, rugosité, pas de texture, double face (16)
  const col = (r.couleur || '#1a1a1a').match(/[0-9a-f]{2}/gi).map((h) => Math.pow(parseInt(h, 16) / 255, 2.2));
  const mat = Buffer.alloc(56);
  [col[0], col[1], col[2], 1, r.metal ?? 0.6, r.rugosite ?? 0.45, 0, 0, 0, 0.5].forEach((x, i) => mat.writeFloatLE(x, 4 * i));
  mat.writeInt32LE(-1, 40); mat.writeInt32LE(-1, 44); mat.writeInt32LE(-1, 48); mat.writeUInt32LE(16, 52);
  w.materiaux = Buffer.concat([w.materiaux, mat]); w.nM++;
  const m = { mat: w.nM - 1, nv: V.length / 8, v: new Float32Array(V), ix: new Uint32Array(I) };
  w.maillages.push(m); if (w.tan) w.tan.push(Buffer.alloc(m.nv * 4));
  return { k: w.maillages.length - 1, n: I.length / 3 };
}

// ---------- WFS-358 : découpe exacte, bouchon, grille ajustée ----------
const sous = (a, b) => [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
const vect = (a, b) => [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
const unite = (a) => { const l = Math.hypot(...a); return l > 1e-20 ? a.map((x) => x / l) : [0, 0, -1]; };
// contour convexe (vue de face, [[x, y], …]) remis dans le sens trigonométrique
function contourDirect(c) {
  let s = 0; for (let i = 0; i < c.length; i++) { const a = c[i], b = c[(i + 1) % c.length]; s += a[0] * b[1] - b[0] * a[1]; }
  return s >= 0 ? c : [...c].reverse();
}
// contour à coins arrondis : { "rectangle": [x0, y0, x1, y1], "rayons": [bas, haut], "pas": n } → polygone convexe
function contourDe(r) {
  if (Array.isArray(r)) return contourDirect(r);
  const [x0, y0, x1, y1] = r.rectangle, [rb, rh] = r.rayons || [0, 0], n = r.pas || 6, P = [];
  const coin = (cx, cy, ray, a0) => { if (ray <= 0) { P.push([cx, cy]); return; } for (let k = 0; k <= n; k++) { const a = a0 + (k / n) * Math.PI / 2; P.push([cx + ray * Math.cos(a), cy + ray * Math.sin(a)]); } };
  coin(x0 + rb, y0 + rb, rb, Math.PI); coin(x1 - rb, y0 + rb, rb, -Math.PI / 2); coin(x1 - rh, y1 - rh, rh, 0); coin(x0 + rh, y1 - rh, rh, Math.PI / 2);
  return contourDirect(P);
}
// résolution de A·x = b (Gauss avec pivot partiel), petits systèmes
function resoudre(A, b) {
  const n = b.length, M = A.map((r, i) => [...r, b[i]]);
  for (let c = 0; c < n; c++) {
    let p = c; for (let r = c + 1; r < n; r++) if (Math.abs(M[r][c]) > Math.abs(M[p][c])) p = r;
    [M[c], M[p]] = [M[p], M[c]];
    if (Math.abs(M[c][c]) < 1e-18) throw new Error('système singulier');
    for (let r = 0; r < n; r++) if (r !== c) { const f = M[r][c] / M[c][c]; for (let k = c; k <= n; k++) M[r][k] -= f * M[c][k]; }
  }
  return M.map((r, i) => r[n] / r[i]);
}
// décalage d'un contour convexe (sens trigonométrique) : chaque côté poussé de d (d > 0 : vers l'extérieur), sommets à l'onglet
function decaler(P, d) {
  const n = P.length, N = [];
  for (let i = 0; i < n; i++) { const a = P[i], b = P[(i + 1) % n], l = Math.hypot(b[0] - a[0], b[1] - a[1]) || 1; N.push([(b[1] - a[1]) / l, -(b[0] - a[0]) / l]); }
  return P.map((p, i) => {
    const n1 = N[(i + n - 1) % n], n2 = N[i], c = 1 + n1[0] * n2[0] + n1[1] * n2[1];
    return c < 1e-6 ? [p[0] + d * n2[0], p[1] + d * n2[1]] : [p[0] + d * (n1[0] + n2[0]) / c, p[1] + d * (n1[1] + n2[1]) / c];
  });
}
// triangle ajouté dans le sens qui rend sa normale géométrique égale à la normale n de ses sommets (comme la carrosserie d'origine)
function triOriente(V, I, a, b, c, n) {
  const p = (i) => [V[i * 8], V[i * 8 + 1], V[i * 8 + 2]], g = vect(sous(p(b), p(a)), sous(p(c), p(a)));
  if (g[0] * n[0] + g[1] * n[1] + g[2] * n[2] >= 0) I.push(a, b, c); else I.push(a, c, b);
}
const dedansContour = (P, x, y) => { for (let i = 0; i < P.length; i++) { const a = P[i], b = P[(i + 1) % P.length]; if ((b[0] - a[0]) * (y - a[1]) - (b[1] - a[1]) * (x - a[0]) < 0) return false; } return true; };

// découpe EXACTE (WFS-358, cause des dents de scie et des trous de WFS-193 : le retrait « par centre de triangle » laissait un bord
// en escalier) : la partie des triangles d'UN maillage qui tombe dans le prisme { contour convexe vu de face } × [z0, z1] est
// retirée ; les triangles à cheval sont coupés le long du contour (sommets ajoutés, attributs interpolés) : bord net.
// { "decouper": true, "materiau": m, "contour": … , "z": [z0, z1], "triangles": n } (n : triangles touchés, contrôle)
function decouper(w, r) {
  const ks = w.maillages.map((m, k) => (m.mat === r.materiau ? k : -1)).filter((k) => k >= 0);
  if (ks.length !== 1) throw new Error(`matériau ${r.materiau} : un maillage attendu`);
  const m = w.maillages[ks[0]], P = contourDe(r.contour), [z0, z1] = r.z;
  const V = Array.from(m.v), I = Array.from(m.ix);
  const bx = [Math.min(...P.map((p) => p[0])), Math.max(...P.map((p) => p[0])), Math.min(...P.map((p) => p[1])), Math.max(...P.map((p) => p[1]))];
  const sommet = (i) => V.slice(i * 8, i * 8 + 8);
  const melange = (a, b, t) => { const s = a.map((x, q) => x + t * (b[q] - x)); const n = unite([s[3], s[4], s[5]]); s[3] = n[0]; s[4] = n[1]; s[5] = n[2]; return s; };
  // coupe d'un polygone (sommets à 8 attributs) par le demi-plan f ≥ 0 (garder = true) ou f < 0
  const couper = (poly, f, garder) => {
    const out = [];
    for (let i = 0; i < poly.length; i++) {
      const a = poly[i], b = poly[(i + 1) % poly.length], fa = f(a), fb = f(b);
      const ina = garder ? fa >= 0 : fa < 0, inb = garder ? fb >= 0 : fb < 0;
      if (ina) out.push(a);
      if (ina !== inb) out.push(melange(a, b, fa / (fa - fb)));
    }
    return out;
  };
  const aire = (p) => { let s = 0; for (let i = 1; i + 1 < p.length; i++) s += Math.abs((p[i][0] - p[0][0]) * (p[i + 1][1] - p[0][1]) - (p[i + 1][0] - p[0][0]) * (p[i][1] - p[0][1])); return s / 2; };
  let n = 0;
  const nt = I.length;
  for (let t = 0; t < nt; t += 3) {
    const c = [I[t], I[t + 1], I[t + 2]];
    if (c[0] === c[1] && c[1] === c[2]) continue;
    const T = c.map(sommet);
    const zc = (T[0][2] + T[1][2] + T[2][2]) / 3;
    if (zc < z0 || zc > z1) continue;
    if (Math.max(...T.map((p) => p[0])) <= bx[0] || Math.min(...T.map((p) => p[0])) >= bx[1] || Math.max(...T.map((p) => p[1])) <= bx[2] || Math.min(...T.map((p) => p[1])) >= bx[3]) continue;
    // triangle entier dans le contour, ou vu par la tranche (pièce verticale, surface vue de face nulle) avec son centre dedans :
    // retiré en entier (sans cela, les ailettes verticales de l'ancienne calandre restaient)
    const dedansT = T.filter((p) => dedansContour(P, p[0], p[1])).length;
    if (dedansT === 3 || (aire(T) <= 1e-9 && dedansContour(P, (T[0][0] + T[1][0] + T[2][0]) / 3, (T[0][1] + T[1][1] + T[2][1]) / 3))) { n++; I[t + 1] = I[t + 2] = I[t]; continue; }
    // T ∖ contour = ∪ (T ∩ H0 ∩ … ∩ H(i−1) ∩ ¬Hi) ; ce qui reste à la fin (T ∩ contour) est retiré
    let reste = T; const morceaux = [];
    for (let i = 0; i < P.length && reste.length >= 3; i++) {
      const a = P[i], b = P[(i + 1) % P.length];
      const f = (p) => (b[0] - a[0]) * (p[1] - a[1]) - (b[1] - a[1]) * (p[0] - a[0]);
      const dehors = couper(reste, f, false);
      if (dehors.length >= 3 && aire(dehors) > 1e-12) morceaux.push(dehors);
      reste = couper(reste, f, true);
    }
    if (reste.length < 3 || aire(reste) <= 1e-12) continue;   // rien du triangle dans le contour : inchangé
    n++;
    I[t + 1] = I[t + 2] = I[t];   // triangle vidé (comme le retrait), remplacé par ses morceaux hors du contour
    for (const p of morceaux) {
      const base = V.length / 8; for (const s of p) V.push(...s);
      for (let i = 1; i + 1 < p.length; i++) I.push(base, base + i, base + i + 1);
    }
  }
  m.v = new Float32Array(V); m.nv = V.length / 8; m.ix = new Uint32Array(I);
  if (w.tan) w.tan[ks[0]] = Buffer.alloc(m.nv * 4);
  return { k: ks[0], n };
}

// carte de profondeur vue de face (regard vers +z) des maillages de la voiture : z le plus en avant par cellule de « pas » m,
// seulement pour les triangles dont le z est dans [z0, z1] ; matériaux de « ignorer » laissés de côté
// dos = true (WFS-362) : z le plus EN ARRIÈRE au lieu du plus en avant (dos d'une pièce, ex. la plaque) ; cellule vide : Infinity
function profondeurFace(w, bx, pas, z0, z1, ignorer, dos = false) {
  const W = Math.ceil((bx[1] - bx[0]) / pas) + 1, H = Math.ceil((bx[3] - bx[2]) / pas) + 1;
  const Z = new Float64Array(W * H).fill(dos ? -Infinity : Infinity);
  for (const m of w.maillages) {
    if (ignorer.includes(m.mat)) continue;
    for (let t = 0; t + 2 < m.ix.length; t += 3) {
      const c = [m.ix[t], m.ix[t + 1], m.ix[t + 2]]; if (c[0] === c[1] && c[1] === c[2]) continue;
      const p = c.map((i) => [(m.v[i * 8] - bx[0]) / pas, (m.v[i * 8 + 1] - bx[2]) / pas, m.v[i * 8 + 2]]);
      if (Math.max(p[0][2], p[1][2], p[2][2]) < z0 || Math.min(p[0][2], p[1][2], p[2][2]) > z1) continue;
      const d = (p[1][0] - p[0][0]) * (p[2][1] - p[0][1]) - (p[2][0] - p[0][0]) * (p[1][1] - p[0][1]);
      if (Math.abs(d) < 1e-12) continue;
      const i0 = Math.max(0, Math.ceil(Math.min(p[0][0], p[1][0], p[2][0]))), i1 = Math.min(W - 1, Math.floor(Math.max(p[0][0], p[1][0], p[2][0])));
      const j0 = Math.max(0, Math.ceil(Math.min(p[0][1], p[1][1], p[2][1]))), j1 = Math.min(H - 1, Math.floor(Math.max(p[0][1], p[1][1], p[2][1])));
      for (let j = j0; j <= j1; j++) for (let i = i0; i <= i1; i++) {
        const l1 = ((p[1][0] - i) * (p[2][1] - j) - (p[2][0] - i) * (p[1][1] - j)) / d;
        const l2 = ((p[2][0] - i) * (p[0][1] - j) - (p[0][0] - i) * (p[2][1] - j)) / d, l3 = 1 - l1 - l2;
        if (l1 < -1e-9 || l2 < -1e-9 || l3 < -1e-9) continue;
        const z = l1 * p[0][2] + l2 * p[1][2] + l3 * p[2][2];
        if (z >= z0 && z <= z1 && (dos ? z > Z[j * W + i] : z < Z[j * W + i])) Z[j * W + i] = z;
      }
    }
  }
  return { W, H, Z, at: (x, y) => { const i = Math.round((x - bx[0]) / pas), j = Math.round((y - bx[2]) / pas); return i < 0 || j < 0 || i >= W || j >= H || !Number.isFinite(Z[j * W + i]) ? Infinity : Z[j * W + i]; } };
}

// bouchon (WFS-358, contour de l'ancien logo rond du capot, demande du lead dev) : les triangles du logo (biseau, creux et bord
// du trou laissé par le logo retiré) d'UN maillage, dans l'ellipse de centre « centre », de rayons « rayons » × « seuil », sont
// retirés, puis le trou est RECOUSU dans la peau du capot à partir des sommets de son bord (étanche, aucune pièce rapportée).
// { "boucher": true, "materiau": m, "centre": [x, y], "rayons": [rx, ry], "seuil": s, "z": [z0, z1], "triangles": n }
// Essais écartés (journal) : plan ou quadrique ajustés + pièce posée devant (disque plus clair), pièce posée derrière (points
// sombres sur le bord du trou en lumière rasante), zone recousue plus large (elle débordait sur la pointe repliée).
function boucher(w, r) {
  const ks = w.maillages.map((m, k) => (m.mat === r.materiau ? k : -1)).filter((k) => k >= 0);
  if (ks.length !== 1) throw new Error(`matériau ${r.materiau} : un maillage attendu`);
  const m = w.maillages[ks[0]], [cx, cy] = r.centre, [rx, ry] = r.rayons, [z0, z1] = r.z;
  const e = (x, y) => Math.hypot((x - cx) / rx, (y - cy) / ry);
  // 1) triangles du logo retirés : ceux dont les 3 sommets sont dans l'ellipse (biseau, creux, bords du trou)
  const V = Array.from(m.v), I = Array.from(m.ix);
  const seuil = r.seuil ?? 1.2;   // X5 : au-delà du biseau (sommets entre 0,97 et 1,02), avant le capot (1,33)
  const dedansE = (i) => V[i * 8 + 2] >= z0 && V[i * 8 + 2] <= z1 && e(V[i * 8], V[i * 8 + 1]) < seuil;
  const cle = (i) => `${Math.round(V[i * 8] * 1e5)},${Math.round(V[i * 8 + 1] * 1e5)},${Math.round(V[i * 8 + 2] * 1e5)}`;
  let dedans = 0;
  const gardes = new Map();   // arêtes des triangles gardés : clé → [a, b] dans le sens du triangle
  for (let t = 0; t + 2 < I.length; t += 3) {
    const c = [I[t], I[t + 1], I[t + 2]]; if (c[0] === c[1] && c[1] === c[2]) continue;
    if (c.every(dedansE)) { I[t + 1] = I[t + 2] = I[t]; dedans++; continue; }
    for (let q = 0; q < 3; q++) { const a = c[q], b = c[(q + 1) % 3]; gardes.set(cle(a) + '>' + cle(b), [a, b]); }
  }
  // 2) bord du trou : arêtes gardées dont les deux bouts sont dans l'ellipse et dont l'arête inverse n'est portée par aucun
  // triangle gardé ; parcourues en sens inverse (le remplissage tourne dans le même sens que ses voisins)
  const suivant = new Map();
  for (const [k, [a, b]] of gardes) {
    if (!dedansE(a) || !dedansE(b)) continue;
    const ka = cle(a), kb = cle(b); if (ka === kb || gardes.has(kb + '>' + ka)) continue;
    suivant.set(kb, { de: b, vers: a });
  }
  // 3) chaque boucle fermée est recousue : couronnes du bord vers le centre ; sommets du bord inchangés (position et normale : les
  // grands triangles voisins gardent leur éclairage) ; à l'intérieur, hauteur et normale interpolées entre le bord et le centre
  // (membrane tendue : pas de bosse ni de pli contre les facettes voisines)
  let nTri = 0;
  const vus = new Set(), C = 6;
  for (const [k0, d0] of suivant) {
    if (vus.has(k0)) continue;
    const boucle = []; let k = k0, d = d0;
    while (d && !vus.has(k)) { vus.add(k); boucle.push(d.de); k = cle(d.vers); d = suivant.get(k); }
    if (k !== k0 || boucle.length < 3) continue;
    const L = boucle.length;
    const gx = boucle.reduce((s, i) => s + V[i * 8], 0) / L, gy = boucle.reduce((s, i) => s + V[i * 8 + 1], 0) / L;
    const nC = unite(boucle.reduce((n, i) => n.map((c, q) => c + V[i * 8 + 3 + q]), [0, 0, 0]));
    const zC = boucle.reduce((s, i) => s + V[i * 8 + 2], 0) / L;   // membrane tendue entre les sommets du bord
    const anneaux = [boucle];   // anneaux[0] = bord ; anneaux[r] = r-ième couronne vers le centre
    for (let r = 1; r < C; r++) {
      const f = 1 - r / C, ring = [];
      for (const i of boucle) {
        const x = gx + f * (V[i * 8] - gx), y = gy + f * (V[i * 8 + 1] - gy);
        const z = f * V[i * 8 + 2] + (1 - f) * zC;
        const n = unite([0, 1, 2].map((q) => f * V[i * 8 + 3 + q] + (1 - f) * nC[q]));
        ring.push(V.length / 8); V.push(x, y, z, n[0], n[1], n[2], V[i * 8 + 6], V[i * 8 + 7]);
      }
      anneaux.push(ring);
    }
    const centre = V.length / 8; V.push(gx, gy, zC, nC[0], nC[1], nC[2], V[boucle[0] * 8 + 6], V[boucle[0] * 8 + 7]);
    for (let r = 0; r + 1 < C; r++) for (let q = 0; q < L; q++) {
      const a = anneaux[r][q], b = anneaux[r][(q + 1) % L], c = anneaux[r + 1][(q + 1) % L], dd = anneaux[r + 1][q];
      I.push(a, b, c, a, c, dd); nTri += 2;
    }
    for (let q = 0; q < L; q++) { I.push(anneaux[C - 1][q], anneaux[C - 1][(q + 1) % L], centre); nTri++; }
  }
  m.v = new Float32Array(V); m.nv = V.length / 8; m.ix = new Uint32Array(I);
  if (w.tan) w.tan[ks[0]] = Buffer.alloc(m.nv * 4);
  return { k: ks[0], retires: dedans, n: nTri };
}

// surface d'une ouverture (WFS-358, factorisée par WFS-362) : contour P, carrosserie vue de face dans [z0, z1] (matériaux « ignorer »
// laissés de côté) ; sur les nœuds de 1 cm où il y a de la carrosserie autour (± 1 cm), surface = carrosserie la plus profonde
// + « recul » ; ailleurs, interpolée (équation de Laplace) : galbe naturel du nez. Rend la boîte, la carte, surf(x, y) et pente(x, y).
// mode « profond » (grille) : carrosserie la plus profonde à ± 1 cm ; mode « proche » (panneau, WFS-362) : carrosserie de la cellule
// même (sinon la plus profonde à ± 1 cm) : le panneau suit la vraie peau au raccord au lieu de plonger dans les creux voisins ;
// mode « exact » (WFS-363, couture) : carrosserie de la cellule seulement (la plus profonde à ± 1 cm tirait le haut de la peau de
// la Giulia vers l'arrondi du capot : creux sous la couture).
// base (WFS-362, option) : surface lisse f(x, y) ; seul l'écart à f est interpolé : la pente et le galbe de f passent dans l'ouverture.
// plancher (WFS-362, option) : z minimal permis en (x, y) (−Infinity : aucun) ; la surface reste derrière (ex. derrière la plaque).
// biharmonique (WFS-363, option, couture) : après Laplace, l'écart est lissé par l'équation biharmonique (∇⁴ = 0, stencil à 13 points) :
// la bande de carrosserie connue autour du contour impose aussi la PENTE au raccord (Laplace ne raccorde que la hauteur : pli au
// bord, ex. haut de l'écusson de la Giulia, dans l'arrondi vers le capot) ; nœud sans ses 12 voisins (bord de la grille) : Laplace.
function surfaceOuverture(w, P, z0, z1, ignorer, recul, mode = 'profond', base = null, plancher = null, biharmonique = false) {
  const bx = [Math.min(...P.map((p) => p[0])), Math.max(...P.map((p) => p[0])), Math.min(...P.map((p) => p[1])), Math.max(...P.map((p) => p[1]))];
  const pas = 0.005, carte = profondeurFace(w, bx, pas, z0, z1, ignorer || []);
  // surface S sur une grille de nœuds de 1 cm : connue (carrosserie + recul) ou interpolée
  const h = 0.01, NX = Math.ceil((bx[1] - bx[0]) / h) + 1, NY = Math.ceil((bx[3] - bx[2]) / h) + 1;
  const S = new Float64Array(NX * NY), connu = new Uint8Array(NX * NY);
  let somme = 0, nb = 0;
  for (let j = 0; j < NY; j++) for (let i = 0; i < NX; i++) {
    const x = bx[0] + i * h, y = bx[2] + j * h;
    let z = -Infinity;   // carrosserie la plus PROFONDE autour du nœud (± 1 cm) : la grille reste derrière chacune des surfaces qu'elle croise
    for (let dj = -2; dj <= 2; dj++) for (let di = -2; di <= 2; di++) { const zc = carte.at(x + di * pas, y + dj * pas); if (Number.isFinite(zc)) z = Math.max(z, zc); }
    if (mode === 'proche' && Number.isFinite(carte.at(x, y))) z = carte.at(x, y);
    if (mode === 'exact') z = carte.at(x, y);   // WFS-363 (couture) : sans voisinage, sinon le nœud prend la peau la plus en arrière
    if (Number.isFinite(z)) { S[j * NX + i] = z + recul - (base ? base(x, y) : 0); connu[j * NX + i] = 1; somme += S[j * NX + i]; nb++; }
  }
  const Cmin = new Float64Array(NX * NY).fill(-Infinity);
  if (plancher) for (let j = 0; j < NY; j++) for (let i = 0; i < NX; i++) { const x = bx[0] + i * h, y = bx[2] + j * h, k = j * NX + i; Cmin[k] = plancher(x, y) - (base ? base(x, y) : 0); if (connu[k]) S[k] = Math.max(S[k], Cmin[k]); }
  if (!nb) throw new Error('ouverture : aucune carrosserie autour du contour');
  for (let k = 0; k < S.length; k++) if (!connu[k]) S[k] = somme / nb;   // départ des itérations : moyenne des nœuds connus
  for (let it = 0; it < 4000; it++) for (let j = 0; j < NY; j++) for (let i = 0; i < NX; i++) {
    const k = j * NX + i; if (connu[k]) continue;
    let s = 0, c = 0;
    if (i > 0) { s += S[k - 1]; c++; } if (i < NX - 1) { s += S[k + 1]; c++; } if (j > 0) { s += S[k - NX]; c++; } if (j < NY - 1) { s += S[k + NX]; c++; }
    S[k] = plancher ? Math.max(s / c, Cmin[k]) : s / c;
  }
  if (biharmonique) for (let it = 0; it < 6000; it++) for (let j = 0; j < NY; j++) for (let i = 0; i < NX; i++) {
    const k = j * NX + i; if (connu[k]) continue;
    if (i >= 2 && i < NX - 2 && j >= 2 && j < NY - 2) {
      const a4 = S[k - 1] + S[k + 1] + S[k - NX] + S[k + NX], ad = S[k - NX - 1] + S[k - NX + 1] + S[k + NX - 1] + S[k + NX + 1];
      const a2 = S[k - 2] + S[k + 2] + S[k - 2 * NX] + S[k + 2 * NX];
      S[k] = (8 * a4 - 2 * ad - a2) / 20;
    } else {
      let s = 0, c = 0;
      if (i > 0) { s += S[k - 1]; c++; } if (i < NX - 1) { s += S[k + 1]; c++; } if (j > 0) { s += S[k - NX]; c++; } if (j < NY - 1) { s += S[k + NX]; c++; }
      S[k] = s / c;
    }
  }
  const surf = (x, y) => {   // interpolation bilinéaire de S
    const fx = Math.min(NX - 1.0001, Math.max(0, (x - bx[0]) / h)), fy = Math.min(NY - 1.0001, Math.max(0, (y - bx[2]) / h));
    const i = Math.floor(fx), j = Math.floor(fy), u = fx - i, v = fy - j, k = j * NX + i;
    return (base ? base(x, y) : 0) + (1 - u) * (1 - v) * S[k] + u * (1 - v) * S[k + 1] + (1 - u) * v * S[k + NX] + u * v * S[k + NX + 1];
  };
  const pente = (x, y) => [(surf(x + 0.005, y) - surf(x - 0.005, y)) / 0.01, (surf(x, y + 0.005) - surf(x, y - 0.005)) / 0.01];
  return { bx, carte, surf, pente };
}

// grille ajustée (WFS-358, RG-03 / RG-05) : grille d'un seul tenant dont la profondeur SUIT la carrosserie : partout où la
// carrosserie passe devant (bords de l'ouverture, cachés par le contour élargi), la grille est « recul » m derrière elle ; dans
// l'ouverture, la surface est interpolée (équation de Laplace) entre ces bords : galbe naturel du nez, aucun triangle de la
// grille devant la carrosserie (contrôlé à la fin). Lames horizontales à profil chanfreiné (6 faces, inclinées vers le bas comme
// des ailettes de série) et fond noir mat « fond » m plus loin. Deux matériaux ajoutés à la fin : lames, puis fond.
// { "grilleAjustee": { "contour": …, "z": [z0, z1], "ignorer": [mats], "recul": m, "fond": m, "lames": n,
//   "lame": { "hauteur": m, "profondeur": m, "chanfrein": m, "pente": m }, "couleur", "metal", "rugosite", "couleurFond" } }
//   options WFS-362 : "joue" (paroi depuis le bord de la carrosserie), "rebord": { "largeur", "avance" }, "derriere": [mats] (pièces
//   devant lesquelles la grille ne passe jamais, ex. la plaque : surface tenue derrière leur dos, contrôlé)
function grilleAjustee(w, r) {
  const g = r.grilleAjustee, P = contourDe(g.contour), [z0, z1] = g.z;
  // « derriere » (WFS-362, revue #323 : une lame barrait la plaque) : dos des pièces listées, élargi de 1,5 cm, + 8 mm de marge
  // (rebord et chanfreins compris) ; la surface de la grille reste derrière, et le contrôle final s'arrête sinon.
  let dosDe = null, plancher = null;
  if (g.derriere) {
    const bxP = [Math.min(...P.map((p) => p[0])), Math.max(...P.map((p) => p[0])), Math.min(...P.map((p) => p[1])), Math.max(...P.map((p) => p[1]))];
    const autres = [...new Set(w.maillages.map((m) => m.mat))].filter((m) => !g.derriere.includes(m));
    dosDe = profondeurFace(w, bxP, 0.005, -1e9, 1e9, autres, true);
    plancher = (x, y) => { let z = -Infinity; for (let dj = -3; dj <= 3; dj++) for (let di = -3; di <= 3; di++) { const d = dosDe.at(x + di * 0.005, y + dj * 0.005); if (Number.isFinite(d)) z = Math.max(z, d + 0.008); } return z; };
  }
  const { bx, carte, surf, pente } = surfaceOuverture(w, P, z0, z1, g.ignorer, g.recul, 'profond', null, plancher);   // sans option : comme la X5
  // surface lissée sur ± 1 cm (rebord et paroi : la carte au pas de 5 mm donnait des bords en escalier, revue #323)
  const lisse = (x, y) => { let s = 0; for (let dj = -1; dj <= 1; dj++) for (let di = -1; di <= 1; di++) s += surf(x + di * 0.01, y + dj * 0.01); return s / 9; };
  const ajouter = (V, I, couleur, metal, rugosite) => {
    const col = (couleur || '#1a1a1a').match(/[0-9a-f]{2}/gi).map((x) => Math.pow(parseInt(x, 16) / 255, 2.2));
    const mat = Buffer.alloc(56);
    [col[0], col[1], col[2], 1, metal, rugosite, 0, 0, 0, 0.5].forEach((x, i) => mat.writeFloatLE(x, 4 * i));
    mat.writeInt32LE(-1, 40); mat.writeInt32LE(-1, 44); mat.writeInt32LE(-1, 48); mat.writeUInt32LE(16, 52);   // double face
    w.materiaux = Buffer.concat([w.materiaux, mat]); w.nM++;
    const m = { mat: w.nM - 1, nv: V.length / 8, v: new Float32Array(V), ix: new Uint32Array(I) };
    w.maillages.push(m); if (w.tan) w.tan.push(Buffer.alloc(m.nv * 4));
    return w.maillages.length - 1;
  };
  // largeur du contour à la hauteur y : [xa, xb]
  const largeur = (y) => {
    let xa = Infinity, xb = -Infinity;
    for (let i = 0; i < P.length; i++) {
      const a = P[i], b = P[(i + 1) % P.length];
      if ((a[1] - y) * (b[1] - y) > 0 || a[1] === b[1]) { if (a[1] === y) { xa = Math.min(xa, a[0]); xb = Math.max(xb, a[0]); } continue; }
      const x = a[0] + (b[0] - a[0]) * (y - a[1]) / (b[1] - a[1]); xa = Math.min(xa, x); xb = Math.max(xb, x);
    }
    return [xa, xb];
  };
  // ---- fond : surface S + fond, cellules de 1 cm dans le contour ----
  const Vf = [], If = [];
  const pf = (x, y) => { const [dx, dy] = pente(x, y), n = unite([dx, dy, -1]); Vf.push(x, y, surf(x, y) + g.fond, n[0], n[1], n[2], 0, 0); return Vf.length / 8 - 1; };
  // grille de sommets partagés : 2 cm en hauteur, 32 colonnes entre les bords du contour (la surface est lisse)
  const hf = 0.02, NYf = Math.ceil((bx[3] - bx[2]) / hf), C = 32, lignes = [];
  for (let j = 0; j <= NYf; j++) {
    const y = Math.min(bx[3], bx[2] + j * hf), [a0, a1] = largeur(y);
    lignes.push(Number.isFinite(a0) ? Array.from({ length: C + 1 }, (_, k) => pf(a0 + (k / C) * (a1 - a0), y)) : null);
  }
  for (let j = 0; j < NYf; j++) {
    const A = lignes[j], B = lignes[j + 1]; if (!A || !B) continue;
    for (let k = 0; k < C; k++) If.push(A[k], A[k + 1], B[k + 1], A[k], B[k + 1], B[k]);
  }
  // ---- conduit : paroi noire tout autour du contour, du bord avant des lames jusqu'au fond (comme le conduit d'air en plastique
  // d'une vraie calandre) : derrière le bord de l'ouverture, on voit cette paroi, plus l'intérieur clair de la voiture (bouts des
  // lames « en peigne » sur fond clair, défaut (2) de RG-04) ----
  const gx = P.reduce((a, p) => a + p[0], 0) / P.length, gy = P.reduce((a, p) => a + p[1], 0) / P.length;
  for (let i = 0; i < P.length; i++) {
    const a = P[i], b = P[(i + 1) % P.length], lg = Math.hypot(b[0] - a[0], b[1] - a[1]), nseg = Math.max(1, Math.ceil(lg / 0.01));
    let n = unite([-(b[1] - a[1]), b[0] - a[0], 0]);   // vers l'intérieur (contour dans le sens trigonométrique)
    if (n[0] * (gx - a[0]) + n[1] * (gy - a[1]) < 0) n = n.map((c) => -c);
    for (let k = 0; k < nseg; k++) {
      const t0 = k / nseg, t1 = (k + 1) / nseg, x0 = a[0] + t0 * (b[0] - a[0]), y0 = a[1] + t0 * (b[1] - a[1]), x1 = a[0] + t1 * (b[0] - a[0]), y1 = a[1] + t1 * (b[1] - a[1]);
      const q = Vf.length / 8;
      // « joue » (WFS-362) : la paroi part du bord de la carrosserie (2 mm derrière elle) et non 1,5 cm plus loin : plus de jour entre
      // le bord découpé et la paroi, par lequel on voyait de 3/4 l'intérieur clair (éclats). Sans l'option : comme la X5.
      // la peau du bord est moyennée sur ± 5 mm (sinon le haut de la paroi suivait les cellules de 5 mm : montant en zigzag)
      const avant = (x, y) => {
        if (!g.joue) return surf(x, y);
        let s = 0, c = 0; for (let dj = -1; dj <= 1; dj++) for (let di = -1; di <= 1; di++) { const zc = carte.at(x + di * 0.005, y + dj * 0.005); if (Number.isFinite(zc)) { s += zc; c++; } }
        return c ? Math.min(lisse(x, y), s / c + 0.004) : lisse(x, y);
      };
      Vf.push(x0, y0, avant(x0, y0), ...n, 0, 0, x1, y1, avant(x1, y1), ...n, 0, 0, x1, y1, surf(x1, y1) + g.fond + 0.005, ...n, 0, 0, x0, y0, surf(x0, y0) + g.fond + 0.005, ...n, 0, 0);
      If.push(q, q + 1, q + 2, q, q + 2, q + 3);
    }
  }
  // ---- lames : profil (dy, dz) autour du bord avant, de haut en bas, extrudé le long de x, sur 48 colonnes ----
  const L = g.lame, hl = L.hauteur, pr = L.profondeur, ch = L.chanfrein, pt = L.pente;
  const profil = [[hl / 2 - ch, 0], [hl / 2, ch], [hl / 2 - pt, pr], [-hl / 2 - pt, pr], [-hl / 2, ch], [-hl / 2 + ch, 0]];
  const Vl = [], Il = [];
  const yb0 = bx[2], yb1 = bx[3], nl = g.lames, pasL = (yb1 - yb0) / nl;
  for (let k = 0; k < nl; k++) {
    const yc = yb0 + (k + 0.5) * pasL;
    const [xa, xb] = largeur(Math.min(yb1, Math.max(yb0, yc))); if (!Number.isFinite(xa)) continue;
    const COL = 48, xs = Array.from({ length: COL + 1 }, (_, c) => xa + (c / COL) * (xb - xa));
    const pos = (x, q) => [x, yc + profil[q][0], surf(x, yc + profil[q][0]) + profil[q][1]];
    for (let q = 0; q < profil.length; q++) {   // une bande par côté du profil, normales de la face (arêtes vives des chanfreins)
      const q2 = (q + 1) % profil.length, base = Vl.length / 8;
      for (const x of xs) {
        const a = pos(x, q), b = pos(x, q2), tx = sous(pos(x + 0.002, q), pos(x - 0.002, q));
        let n = unite(vect(tx, sous(b, a)));
        // dehors du profil : du côté opposé au centre de la lame
        const mil = [(a[0] + b[0]) / 2, (a[1] + b[1]) / 2, (a[2] + b[2]) / 2], ctr = [x, yc - pt / 2, surf(x, yc) + pr / 2];
        if (n[0] * (mil[0] - ctr[0]) + n[1] * (mil[1] - ctr[1]) + n[2] * (mil[2] - ctr[2]) < 0) n = n.map((c) => -c);
        Vl.push(...a, ...n, 0, 0, ...b, ...n, 0, 0);
      }
      for (let c = 0; c < COL; c++) { const i0 = base + 2 * c; Il.push(i0, i0 + 2, i0 + 3, i0, i0 + 3, i0 + 1); }
    }
    for (const [x, sens] of [[xa, -1], [xb, 1]]) {   // bouts fermés (cachés sous la carrosserie)
      const base = Vl.length / 8;
      for (let q = 0; q < profil.length; q++) Vl.push(...pos(x, q), sens, 0, 0, 0, 0);
      for (let q = 1; q + 1 < profil.length; q++) Il.push(base, base + q, base + q + 1);
    }
  }
  // ---- rebord (WFS-362, option) : bande de « largeur » m tout autour, « avance » m devant les lames (toujours derrière la carrosserie),
  // puis paroi intérieure jusqu'au dos des lames : les bouts des lames finissent contre un rebord, plus « en peigne » ----
  if (g.rebord) {
    const lr = g.rebord.largeur ?? 0.012, av = g.rebord.avance ?? 0.004, Pin = decaler(P, -lr);
    for (let i = 0; i < P.length; i++) {
      const a = P[i], b = P[(i + 1) % P.length], ai = Pin[i], bi = Pin[(i + 1) % P.length];
      const nseg = Math.max(1, Math.ceil(Math.hypot(b[0] - a[0], b[1] - a[1]) / 0.01));
      let nin = unite([-(b[1] - a[1]), b[0] - a[0], 0]);
      if (nin[0] * (gx - a[0]) + nin[1] * (gy - a[1]) < 0) nin = nin.map((c) => -c);
      for (let k = 0; k < nseg; k++) {
        const t0 = k / nseg, t1 = (k + 1) / nseg, L = (u, v, t) => [u[0] + t * (v[0] - u[0]), u[1] + t * (v[1] - u[1])];
        const o0 = L(a, b, t0), o1 = L(a, b, t1), i0 = L(ai, bi, t0), i1 = L(ai, bi, t1);
        const face = (q) => { const [dx, dy] = pente(q[0], q[1]); return unite([dx, dy, -1]); };
        const base = Vl.length / 8;
        for (const q of [o0, o1, i1, i0]) Vl.push(q[0], q[1], lisse(q[0], q[1]) - av, ...face(q), 0, 0);
        triOriente(Vl, Il, base, base + 1, base + 2, face(o0)); triOriente(Vl, Il, base, base + 2, base + 3, face(o0));
        const b2 = Vl.length / 8;   // paroi intérieure du rebord, tournée vers l'ouverture
        for (const [q, dz] of [[i0, -av], [i1, -av], [i1, pr + 0.004], [i0, pr + 0.004]]) Vl.push(q[0], q[1], lisse(q[0], q[1]) + dz, ...nin, 0, 0);
        triOriente(Vl, Il, b2, b2 + 1, b2 + 2, nin); triOriente(Vl, Il, b2, b2 + 2, b2 + 3, nin);
      }
    }
  }
  const kl = ajouter(Vl, Il, g.couleur, g.metal ?? 0.7, g.rugosite ?? 0.3);
  const kf = ajouter(Vf, If, g.couleurFond || '#0b0b0b', 0, 0.75);
  // contrôle RG-05 : aucun point de la grille devant la carrosserie (carte faite avant l'ajout)
  let devant = 0;
  for (const m of [w.maillages[kl], w.maillages[kf]]) for (let i = 0; i < m.nv; i++) {
    const o = i * 8, zc = carte.at(m.v[o], m.v[o + 1]);
    if (Number.isFinite(zc) && m.v[o + 2] < zc - 0.0005) devant++;
  }
  // contrôle « derriere » : aucun point de la grille devant le dos des pièces listées (ex. la plaque)
  let devantPiece = 0;
  if (dosDe) for (const m of [w.maillages[kl], w.maillages[kf]]) for (let i = 0; i < m.nv; i++) {
    const o = i * 8, d = dosDe.at(m.v[o], m.v[o + 1]);
    if (Number.isFinite(d) && m.v[o + 2] < d + 0.001) devantPiece++;
  }
  return { ks: [kl, kf], n: (Il.length + If.length) / 3, devant, devantPiece };
}

// base d'une peau (WFS-362, factorisée par WFS-363) : surface de degré 2 ajustée (moindres carrés) sur la PEINTURE seule (matériau
// « mat »), dans une bande de « bande » m autour du contour P0 : elle donne à la peau l'inclinaison et le galbe du nez voisin (sans
// elle, la surface interpolée restait presque verticale : panneau plus sombre que la carrosserie, revue #323) ; l'écart à cette base
// est ensuite interpolé jusqu'aux bords exacts. « baseFace » (WFS-363, facultatif) : seuls les points dont la pente vue de face
// (|∇z| lue sur ± 1 cm) reste sous cette valeur comptent : ni le rebord de l'ancienne ouverture ni l'arrondi vers le capot (Giulia :
// sans cela, la base bombait la peau, bas tourné vers le sol, zone sombre).
function baseAjustee(w, mat, P0, bande, z0, z1, baseFace) {
  const Pb = decaler(P0, bande);
  const bb = [Math.min(...Pb.map((q) => q[0])), Math.max(...Pb.map((q) => q[0])), Math.min(...Pb.map((q) => q[1])), Math.max(...Pb.map((q) => q[1]))];
  const autres = [...new Set(w.maillages.map((m) => m.mat))].filter((m) => m !== mat);
  const peau = profondeurFace(w, bb, 0.005, z0, z1, autres);
  const A = Array.from({ length: 6 }, () => new Float64Array(6)), Bv = new Float64Array(6);
  let nbase = 0;
  for (let y = bb[2]; y <= bb[3]; y += 0.005) for (let x = bb[0]; x <= bb[1]; x += 0.005) {
    if (dedansContour(P0, x, y) || !dedansContour(Pb, x, y)) continue;
    const z = peau.at(x, y); if (!Number.isFinite(z)) continue;
    if (baseFace !== undefined) {
      const zx0 = peau.at(x - 0.01, y), zx1 = peau.at(x + 0.01, y), zy0 = peau.at(x, y - 0.01), zy1 = peau.at(x, y + 0.01);
      if (![zx0, zx1, zy0, zy1].every(Number.isFinite) || Math.hypot((zx1 - zx0) / 0.02, (zy1 - zy0) / 0.02) > baseFace) continue;
    }
    const f = [1, x, y, x * x, x * y, y * y];
    for (let a = 0; a < 6; a++) { Bv[a] += f[a] * z; for (let b = 0; b < 6; b++) A[a][b] += f[a] * f[b]; }
    nbase++;
  }
  if (nbase < 50) throw new Error(`base : ${nbase} points de peinture autour du contour, trop peu`);
  const c = resoudre(A, Bv);
  return (x, y) => c[0] + c[1] * x + c[2] * y + c[3] * x * x + c[4] * x * y + c[5] * y * y;
}

// panneau (WFS-362, skill artiste-3d § 5 : « fermer l'ouverture par une pièce de carrosserie ») : une ancienne ouverture de marque
// (déjà découpée) est refermée par une peau AJOUTÉE AU MAILLAGE de la carrosserie (même matériau : même peinture, même vernis).
// - Surface : base de degré 2 ajustée sur la peinture voisine, plus l'écart interpolé (Laplace) jusqu'à la peau du bord (mode « proche ») ;
//   « recul » m derrière elle, jamais devant (contrôlé). Le contour doit suivre des lignes du nez (joint du capot, bords des phares),
//   au-delà du rebord de l'ancienne ouverture : sinon ce rebord redessine la calandre (revue du lead dev, #323).
// - Débord : le contour est décalé de « debord » m vers l'extérieur (vrai décalage des côtés, sommets à l'onglet) : le panneau passe
//   sous la carrosserie voisine, aucun jour au raccord.
// - Bombé (m) : φ = bosse douce (∇²φ = −1, nulle sur le contour d'origine, normalisée, élevée au carré : pente nulle au bord) ;
//   la surface avance de « bombe » × φ.
// - Normales : celles de la surface finale (différences finies), lisses ; ne plus prendre celles de la carrosserie voisine (parois
//   du rebord tournées de côté : cratère et vagues dans les reflets, revue #323).
// { "panneau": { "materiau": m, "contour": …, "z": [z0, z1], "ignorer": [mats], "recul": m, "debord": m, "bombe": m, "bande": m }, "note", "triangles": n }
function panneau(w, r) {
  const p = r.panneau;
  const ks = w.maillages.map((m, k) => (m.mat === p.materiau ? k : -1)).filter((k) => k >= 0);
  if (ks.length !== 1) throw new Error(`matériau ${p.materiau} : un maillage attendu`);
  const P0 = contourDe(p.contour), [z0, z1] = p.z, P = decaler(P0, p.debord ?? 0.006);
  const base = baseAjustee(w, p.materiau, P0, p.bande ?? 0.03, z0, z1);
  // surface calculée sur le contour élargi de 2,5 cm de plus : la pente lue à ± 1,5 cm au bord du panneau reste dans la grille
  // (revue #323 : bornée à la boîte, elle faussait les normales du bord : pli contre le phare)
  const { carte, surf } = surfaceOuverture(w, decaler(P0, (p.debord ?? 0.006) + 0.025), z0, z1, p.ignorer, p.recul ?? 0.002, 'proche', base);
  const bx = [Math.min(...P.map((q) => q[0])), Math.max(...P.map((q) => q[0])), Math.min(...P.map((q) => q[1])), Math.max(...P.map((q) => q[1]))];
  // bosse φ sur une grille de 1 cm
  const bombe = p.bombe ?? 0, hn = 0.01, NX = Math.ceil((bx[1] - bx[0]) / hn) + 1, NY = Math.ceil((bx[3] - bx[2]) / hn) + 1;
  const Ph = new Float64Array(NX * NY), dedans = new Uint8Array(NX * NY);
  for (let j = 0; j < NY; j++) for (let i = 0; i < NX; i++) if (dedansContour(P0, bx[0] + i * hn, bx[2] + j * hn)) dedans[j * NX + i] = 1;
  if (bombe > 0) {
    for (let it = 0; it < 4000; it++) for (let j = 1; j < NY - 1; j++) for (let i = 1; i < NX - 1; i++) {
      const k = j * NX + i; if (dedans[k]) Ph[k] = (Ph[k - 1] + Ph[k + 1] + Ph[k - NX] + Ph[k + NX] + hn * hn) / 4;
    }
    let mx = 0; for (const v of Ph) mx = Math.max(mx, v);
    if (mx > 0) for (let k = 0; k < Ph.length; k++) Ph[k] = (Ph[k] / mx) ** 2;
  }
  const phi = (x, y) => {
    const fx = Math.min(NX - 1.0001, Math.max(0, (x - bx[0]) / hn)), fy = Math.min(NY - 1.0001, Math.max(0, (y - bx[2]) / hn));
    const i = Math.floor(fx), j = Math.floor(fy), u = fx - i, v = fy - j, k = j * NX + i;
    return (1 - u) * (1 - v) * Ph[k] + u * (1 - v) * Ph[k + 1] + (1 - u) * v * Ph[k + NX] + u * v * Ph[k + NX + 1];
  };
  const hauteur = (x, y) => surf(x, y) - bombe * phi(x, y);
  // pente lue sur ± 1,5 cm (plus que la maille de 1 cm de la surface bilinéaire : sinon des ondulations se lisent dans les reflets)
  const e = 0.015, normale = (x, y) => unite([(hauteur(x + e, y) - hauteur(x - e, y)) / (2 * e), (hauteur(x, y + e) - hauteur(x, y - e)) / (2 * e), -1]);
  const largeur = (y) => {
    let xa = Infinity, xb = -Infinity;
    for (let i = 0; i < P.length; i++) {
      const a = P[i], b = P[(i + 1) % P.length];
      if ((a[1] - y) * (b[1] - y) > 0 || a[1] === b[1]) { if (a[1] === y) { xa = Math.min(xa, a[0]); xb = Math.max(xb, a[0]); } continue; }
      const x = a[0] + (b[0] - a[0]) * (y - a[1]) / (b[1] - a[1]); xa = Math.min(xa, x); xb = Math.max(xb, x);
    }
    return [xa, xb];
  };
  const m = w.maillages[ks[0]], V = Array.from(m.v), I = Array.from(m.ix), premier = m.nv;
  const h = 0.01, NYp = Math.max(1, Math.ceil((bx[3] - bx[2]) / h)), C = Math.max(2, Math.ceil((bx[1] - bx[0]) / h)), lignes = [];
  for (let j = 0; j <= NYp; j++) {
    const y = Math.min(bx[3], bx[2] + j * h), [a0, a1] = largeur(y);
    if (!Number.isFinite(a0)) { lignes.push(null); continue; }
    lignes.push(Array.from({ length: C + 1 }, (_, k) => {
      const x = a0 + (k / C) * (a1 - a0), n = normale(x, y);
      V.push(x, y, hauteur(x, y), n[0], n[1], n[2], 0, 0); return V.length / 8 - 1;
    }));
  }
  let n = 0;
  for (let j = 0; j < NYp; j++) {
    const A = lignes[j], B = lignes[j + 1]; if (!A || !B) continue;
    // sens des sommets : normale géométrique = normale enregistrée (vers l'avant, −z), comme la carrosserie d'origine
    for (let k = 0; k < C; k++) { I.push(A[k], B[k + 1], A[k + 1], A[k], B[k], B[k + 1]); n += 2; }
  }
  // contrôle : aucun sommet du panneau devant la carrosserie (carte faite avant l'ajout)
  let devant = 0;
  for (let i = premier; i < V.length / 8; i++) { const zc = carte.at(V[i * 8], V[i * 8 + 1]); if (Number.isFinite(zc) && V[i * 8 + 2] < zc - 0.0005) devant++; }
  m.v = new Float32Array(V); m.nv = V.length / 8; m.ix = new Uint32Array(I);
  if (w.tan) w.tan[ks[0]] = Buffer.alloc(m.nv * 4);
  return { k: ks[0], n, devant };
}

// triangulation de Delaunay (Bowyer-Watson) de points 2D [[x, y], …] : rend des triangles [a, b, c] (indices des points)
function delaunay(P) {
  const xs = P.map((p) => p[0]), ys = P.map((p) => p[1]);
  const mx = Math.min(...xs), Mx = Math.max(...xs), my = Math.min(...ys), My = Math.max(...ys), d = Math.max(Mx - mx, My - my) * 20;
  const Q = P.concat([[mx - d, my - d], [Mx + d, my - d], [(mx + Mx) / 2, My + d]]), n = P.length;
  const cercle = (a, b, c) => {
    const [ax, ay] = Q[a], [bx, by] = Q[b], [cx, cy] = Q[c], D = 2 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by));
    const ux = ((ax * ax + ay * ay) * (by - cy) + (bx * bx + by * by) * (cy - ay) + (cx * cx + cy * cy) * (ay - by)) / D;
    const uy = ((ax * ax + ay * ay) * (cx - bx) + (bx * bx + by * by) * (ax - cx) + (cx * cx + cy * cy) * (bx - ax)) / D;
    return [ux, uy, (ax - ux) ** 2 + (ay - uy) ** 2];
  };
  let T = [[n, n + 1, n + 2, cercle(n, n + 1, n + 2)]];
  for (let i = 0; i < n; i++) {
    const [x, y] = Q[i], mauvais = [], gardes = [];
    for (const t of T) ((x - t[3][0]) ** 2 + (y - t[3][1]) ** 2 < t[3][2] * (1 - 1e-12) ? mauvais : gardes).push(t);
    const aretes = new Map();
    for (const t of mauvais) for (const [a, b] of [[t[0], t[1]], [t[1], t[2]], [t[2], t[0]]]) { const k = a < b ? a + ',' + b : b + ',' + a; aretes.set(k, aretes.has(k) ? null : [a, b]); }
    for (const e of aretes.values()) if (e) gardes.push([e[0], e[1], i, cercle(e[0], e[1], i)]);
    T = gardes;
  }
  return T.filter((t) => t[0] < n && t[1] < n && t[2] < n).map((t) => [t[0], t[1], t[2]]);
}

// couture (WFS-363, skill artiste-3d § 5 : « fermer l'ouverture par une pièce de carrosserie », sans joint) : une ouverture déjà
// découpée sur le contour C (convexe) est refermée par une peau ajoutée au maillage de la carrosserie, COUSUE à son bord :
// - Bord : les sommets du bord découpé de la carrosserie qui sont sur C (à 1,5 mm près) sont repris tels quels (même position, même
//   normale) : le moteur les soude au chargement (positions au 1/10 de mm, correctifs_voitures.cpp) ; aucun joint ni saut
//   d'éclairage ne dessine plus l'ancienne ouverture (Giulia, revue de M4 #323 (2)). Là où C ne longe aucune carrosserie (ex. le
//   bas, au-dessus d'une grille), des points sont ajoutés sur C tous les 1 cm : bord libre, caché par la paroi de la grille.
// - Surface : base de degré 2 ajustée sur la face voisine (baseAjustee, « baseFace » conseillé), plus l'écart interpolé (Laplace puis
//   biharmonique : hauteur ET pente raccordées à la bande de carrosserie autour, mode « exact ») ; l'écart entre cette surface et le bord réel (moyenne pondérée par 1 / d²) est fondu sur 3 cm vers l'intérieur.
// - Maillage : triangulation de Delaunay des points du bord et d'une grille de 1 cm à l'intérieur (triangles bien formés : des
//   couronnes concentriques donnaient des triangles très allongés, stries en étoile une fois les normales recalculées).
// - Normales : surface (différences finies sur ± 1,5 cm) ; bord : celles de la carrosserie.
// { "couture": { "materiau": m, "contour": …, "z": [z0, z1], "ignorer": [mats], "bande": m, "baseFace": p }, "note", "triangles": n }
function couture(w, r) {
  const p = r.couture;
  const ks = w.maillages.map((m, k) => (m.mat === p.materiau ? k : -1)).filter((k) => k >= 0);
  if (ks.length !== 1) throw new Error(`matériau ${p.materiau} : un maillage attendu`);
  const m = w.maillages[ks[0]], C = contourDe(p.contour), [z0, z1] = p.z;
  // position le long de C (abscisse curviligne depuis le sommet 0) et distance à C
  const lg = C.map((a, i) => Math.hypot(C[(i + 1) % C.length][0] - a[0], C[(i + 1) % C.length][1] - a[1]));
  const perim = lg.reduce((s, x) => s + x, 0);
  const surC = (x, y) => {
    let best = Infinity, sb = 0, acc = 0;
    for (let i = 0; i < C.length; i++) {
      const a = C[i], b = C[(i + 1) % C.length], dx = b[0] - a[0], dy = b[1] - a[1];
      const t = Math.max(0, Math.min(1, ((x - a[0]) * dx + (y - a[1]) * dy) / (dx * dx + dy * dy)));
      const d = Math.hypot(a[0] + t * dx - x, a[1] + t * dy - y);
      if (d < best) { best = d; sb = acc + t * lg[i]; }
      acc += lg[i];
    }
    return { d: best, s: sb };
  };
  const pointC = (s) => {   // point de C à l'abscisse s
    s = ((s % perim) + perim) % perim;
    for (let i = 0; i < C.length; i++) { if (s <= lg[i] || i === C.length - 1) { const a = C[i], b = C[(i + 1) % C.length], t = lg[i] ? Math.min(1, s / lg[i]) : 0; return [a[0] + t * (b[0] - a[0]), a[1] + t * (b[1] - a[1])]; } s -= lg[i]; }
  };
  // 1) bord découpé : arêtes portées par un seul triangle, deux bouts sur C, dans [z0, z1]
  const cle = (a, b) => (a < b ? a + ',' + b : b + ',' + a), nb = new Map();
  for (let t = 0; t + 2 < m.ix.length; t += 3) {
    const c = [m.ix[t], m.ix[t + 1], m.ix[t + 2]]; if (c[0] === c[1] && c[1] === c[2]) continue;
    for (let q = 0; q < 3; q++) { const k = cle(c[q], c[(q + 1) % 3]); nb.set(k, (nb.get(k) || 0) + 1); }
  }
  const pos = (i) => [m.v[i * 8], m.v[i * 8 + 1], m.v[i * 8 + 2]];
  const prend = (i) => { const q = pos(i); return q[2] >= z0 && q[2] <= z1 && surC(q[0], q[1]).d < 0.0015; };
  const bord = new Map(), liens = new Set();   // sommets du bord (par position au 1/10 de mm), arêtes du bord entre eux
  const id = (i) => { const q = pos(i); return q.map((x) => Math.round(x * 1e4)).join(','); };
  for (const [k, n] of nb) {
    if (n !== 1) continue;
    const [a, b] = k.split(',').map(Number);
    if (!prend(a) || !prend(b)) continue;
    for (const i of [a, b]) if (!bord.has(id(i))) { const q = pos(i); bord.set(id(i), { i, x: q[0], y: q[1], z: q[2], s: surC(q[0], q[1]).s }); }
    liens.add(cle(id(a), id(b)) );
  }
  const pts = [...bord.values()].sort((a, b) => a.s - b.s);
  if (pts.length < 8) throw new Error(`couture : ${pts.length} sommets de bord sur le contour, trop peu`);
  // 2) surface : base + écart interpolé (comme le panneau, sans recul ni débord)
  const base = baseAjustee(w, p.materiau, C, p.bande ?? 0.03, z0, z1, p.baseFace);
  const { carte, surf } = surfaceOuverture(w, decaler(C, 0.025), z0, z1, p.ignorer, 0, 'exact', base, null, true);
  // 3) boucle du bord : sommets cousus, et points libres sur C dans les trous de plus de 1 cm sans arête de carrosserie
  const boucle = [];
  for (let k = 0; k < pts.length; k++) {
    const a = pts[k], b = pts[(k + 1) % pts.length];
    boucle.push({ x: a.x, y: a.y, z: a.z, n: [m.v[a.i * 8 + 3], m.v[a.i * 8 + 4], m.v[a.i * 8 + 5]], cousu: true });
    let ds = b.s - a.s; if (k === pts.length - 1) ds += perim;
    if (liens.has(cle(id(a.i), id(b.i))) || ds <= 0.01) continue;
    const n = Math.ceil(ds / 0.01);
    // point libre : sur la peau de la carrosserie s'il y en a là (trou de couture), sinon sur la surface
    for (let q = 1; q < n; q++) { const [x, y] = pointC(a.s + (ds * q) / n), zc = carte.at(x, y); boucle.push({ x, y, z: Number.isFinite(zc) ? zc : surf(x, y), cousu: false }); }
  }
  // 4) points intérieurs : grille de 1 cm à plus de 7 mm du contour ; écart au bord réel fondu sur 3 cm (le plus proche du bord)
  const e = 0.015, normaleSurf = (x, y) => unite([(surf(x + e, y) - surf(x - e, y)) / (2 * e), (surf(x, y + e) - surf(x, y - e)) / (2 * e), -1]);
  const ecart = boucle.map((b) => b.z - surf(b.x, b.y));
  const P2 = boucle.map((b) => [b.x, b.y]);
  const bxC = [Math.min(...C.map((q) => q[0])), Math.max(...C.map((q) => q[0])), Math.min(...C.map((q) => q[1])), Math.max(...C.map((q) => q[1]))];
  for (let y = Math.ceil(bxC[2] / 0.01) * 0.01; y < bxC[3]; y += 0.01) for (let x = Math.ceil(bxC[0] / 0.01) * 0.01; x < bxC[1]; x += 0.01)
    if (dedansContour(C, x, y) && surC(x, y).d > 0.007) P2.push([x, y]);
  // 5) triangulation de Delaunay (Bowyer-Watson) des points du bord et de l'intérieur ; triangles hors de C retirés
  const tris = delaunay(P2).filter(([a, b, c]) => dedansContour(decaler(C, 0.0005), (P2[a][0] + P2[b][0] + P2[c][0]) / 3, (P2[a][1] + P2[b][1] + P2[c][1]) / 3));
  const V = Array.from(m.v), I = Array.from(m.ix), premier = m.nv, N = boucle.length, idx = [];
  for (let k = 0; k < P2.length; k++) {
    const [x, y] = P2[k];
    if (k < N) { const b = boucle[k], n = b.cousu ? b.n : normaleSurf(x, y); V.push(x, y, b.z, n[0], n[1], n[2], 0, 0); idx.push(V.length / 8 - 1); continue; }
    // écart moyen du bord, pondéré par 1 / d² (continu : le point le plus proche seul donnait des marches, bord en dents de scie)
    let dmin = Infinity, se = 0, sp = 0;
    for (let q = 0; q < N; q++) { const d2 = (P2[q][0] - x) ** 2 + (P2[q][1] - y) ** 2; dmin = Math.min(dmin, Math.sqrt(d2)); se += ecart[q] / d2; sp += 1 / d2; }
    const ec = se / sp, f = Math.max(0, 1 - dmin / 0.03), n = normaleSurf(x, y);
    V.push(x, y, surf(x, y) + f * f * ec, n[0], n[1], n[2], 0, 0); idx.push(V.length / 8 - 1);
  }
  const avant = [0, 0, -1];
  for (const [a, b, c] of tris) triOriente(V, I, idx[a], idx[b], idx[c], avant);
  const n = tris.length;
  // contrôle : aucun sommet de la peau devant la carrosserie (carte faite avant l'ajout), sauf le bord cousu, qui EST la carrosserie
  let devant = 0;
  const coususIdx = new Set(idx.slice(0, N).filter((_, i) => boucle[i].cousu));
  for (let i = premier; i < V.length / 8; i++) { if (coususIdx.has(i)) continue; const zc = carte.at(V[i * 8], V[i * 8 + 1]); if (Number.isFinite(zc) && V[i * 8 + 2] < zc - 0.0005) devant++; }
  m.v = new Float32Array(V); m.nv = V.length / 8; m.ix = new Uint32Array(I);
  if (w.tan) w.tan[ks[0]] = Buffer.alloc(m.nv * 4);
  return { k: ks[0], n, devant, cousus: pts.length, libres: N - pts.length };
}

function traiter(id, verifier) {
  const cfg = FORMES[id];
  if (!cfg) { console.log(`${id} : aucune retouche de forme (absente de formes.json)`); return true; }   // chaîne convertir_voitures.sh
  const chemin = path.join(RACINE, 'assets/cars', id + '.wfm');
  const w = lire(fs.readFileSync(chemin));
  const ajoutes = cfg.retouches.reduce((s, r) => s + (r.grille ? 1 : r.grilleAjustee ? 2 : 0), 0);   // matériaux ajoutés à la fin par les grilles
  if (w.nM !== cfg.materiaux && w.nM !== cfg.materiaux + ajoutes) throw new Error(`${id} : ${w.nM} matériaux, ${cfg.materiaux} attendus`);
  const e = empreinte(w);
  if (cfg.empreinteApres && e === cfg.empreinteApres) { console.log(`${id} : déjà retouchée (${e})`); return true; }
  if (e !== cfg.empreinteAvant) { console.error(`${id} : empreinte ${e}, ni « avant » (${cfg.empreinteAvant}) ni « après » (${cfg.empreinteApres}) : rien écrit`); return false; }
  const touches = new Set(); const journal = [];
  let structure = false;   // triangles retirés ou pièce ajoutée : .lod à régénérer
  for (const r of cfg.retouches) {
    if (r.grille) { const { k, n } = grille(w, r); touches.add(k); structure = true; journal.push(`${r.note} (${n} triangles ajoutés)`); continue; }
    if (r.grilleAjustee) {
      const { ks, n, devant, devantPiece } = grilleAjustee(w, r);
      if (devant) { console.error(`${id} : « ${r.note} » : ${devant} sommets de la grille devant la carrosserie (RG-05) : rien écrit`); return false; }
      if (devantPiece) { console.error(`${id} : « ${r.note} » : ${devantPiece} sommets de la grille devant la plaque (« derriere ») : rien écrit`); return false; }
      ks.forEach((k) => touches.add(k)); structure = true; journal.push(`${r.note} (${n} triangles ajoutés, aucun devant la carrosserie${r.grilleAjustee.derriere ? ' ni devant la plaque' : ''})`); continue;
    }
    if (r.panneau) {
      const { k, n, devant } = panneau(w, r);
      if (devant) { console.error(`${id} : « ${r.note} » : ${devant} sommets du panneau devant la carrosserie : rien écrit`); return false; }
      if (r.triangles !== undefined && n !== r.triangles) { console.error(`${id} : « ${r.note} » : ${n} triangles, ${r.triangles} attendus : rien écrit`); return false; }
      touches.add(k); structure = true; journal.push(`${r.note} (${n} triangles ajoutés à la carrosserie, aucun devant)`); continue;
    }
    if (r.couture) {
      const { k, n, devant, cousus, libres } = couture(w, r);
      if (devant) { console.error(`${id} : « ${r.note} » : ${devant} sommets de la peau devant la carrosserie : rien écrit`); return false; }
      if (r.triangles !== undefined && n !== r.triangles) { console.error(`${id} : « ${r.note} » : ${n} triangles, ${r.triangles} attendus : rien écrit`); return false; }
      touches.add(k); structure = true; journal.push(`${r.note} (${n} triangles ajoutés à la carrosserie, ${cousus} sommets cousus au bord, ${libres} points de bord libre, aucun devant)`); continue;
    }
    if (r.decouper) {
      const { k, n } = decouper(w, r);
      if (r.triangles !== undefined && n !== r.triangles) { console.error(`${id} : « ${r.note} » : ${n} triangles coupés, ${r.triangles} attendus : rien écrit`); return false; }
      touches.add(k); structure = true; journal.push(`${r.note} (${n} triangles coupés sur le contour)`); continue;
    }
    if (r.boucher) {
      const { k, retires, n } = boucher(w, r);
      if (r.triangles !== undefined && retires !== r.triangles) { console.error(`${id} : « ${r.note} » : ${retires} triangles du logo, ${r.triangles} attendus : rien écrit`); return false; }
      if (!n) { console.error(`${id} : « ${r.note} » : trou non refermé : rien écrit`); return false; }
      touches.add(k); structure = true; journal.push(`${r.note} (${retires} triangles du logo retirés, trou recousu par ${n} triangles)`); continue;
    }
    if (r.lisser) {
      const { k, dedans, manque } = lisser(w, r);
      if (manque) { console.error(`${id} : « ${r.note} » : ${manque} sommets sans appui des deux côtés : rien écrit`); return false; }
      if (r.sommets !== undefined && dedans !== r.sommets) { console.error(`${id} : « ${r.note} » : ${dedans} sommets lissés, ${r.sommets} attendus : rien écrit`); return false; }
      touches.add(k); journal.push(`${r.note} (${dedans} sommets lissés)`); continue;
    }
    if (r.retirer) {
      const { k, n } = retirer(w, r);
      if (r.triangles !== undefined && n !== r.triangles) { console.error(`${id} : « ${r.note} » : ${n} triangles retirés, ${r.triangles} attendus : rien écrit`); return false; }
      touches.add(k); structure = true; journal.push(`${r.note} (${n} triangles retirés)`); continue;
    }
    const { k, dedans } = appliquer(w, r);
    if (r.sommets !== undefined && dedans !== r.sommets) { console.error(`${id} : « ${r.note} » : ${dedans} sommets dans la boîte, ${r.sommets} attendus : rien écrit`); return false; }
    touches.add(k); journal.push(`${r.note} (${dedans} sommets)`);
  }
  const apres = empreinte(w);
  if (cfg.empreinteApres && apres !== cfg.empreinteApres) { console.error(`${id} : résultat ${apres}, ${cfg.empreinteApres} attendu : rien écrit`); return false; }
  if (verifier) { console.log(`${id} : à retoucher, résultat ${apres} : ${journal.join(' ; ')}`); return true; }
  if (w.tan) for (const k of touches) w.tan[k] = Buffer.from(tangentes(w.maillages[k].v, 8, w.maillages[k].ix).octets.buffer);
  const bb = [1e9, 1e9, 1e9, -1e9, -1e9, -1e9];
  for (const m of w.maillages) for (const i of m.ix) for (let c = 0; c < 3; c++) { const x = m.v[i * 8 + c]; bb[c] = Math.min(bb[c], x); bb[c + 3] = Math.max(bb[c + 3], x); }
  w.boite = Buffer.from(new Float32Array(bb).buffer);
  fs.writeFileSync(chemin, ecrire(w));
  if (structure) execFileSync('node', [path.join(ICI, 'alleger_voiture.mjs'), chemin, chemin.replace(/\.wfm$/, '.lod'), '0.3'], { stdio: 'inherit' });   // indices changés : niveaux de détail refaits
  console.log(`${id} : retouchée, empreinte ${apres}${cfg.empreinteApres ? '' : ' (à noter dans formes.json : empreinteApres)'} : ${journal.join(' ; ')}`);
  return true;
}

function zone(id, b) {
  const w = lire(fs.readFileSync(path.join(RACINE, 'assets/cars', id + '.wfm')));
  w.maillages.forEach((m, k) => {
    let n = 0; const bb = [1e9, 1e9, 1e9, -1e9, -1e9, -1e9];
    for (let i = 0; i < m.nv; i++) {
      const p = [m.v[i * 8], m.v[i * 8 + 1], m.v[i * 8 + 2]];
      if (poids(p, b, 0) < 1) continue;
      n++; for (let c = 0; c < 3; c++) { bb[c] = Math.min(bb[c], p[c]); bb[c + 3] = Math.max(bb[c + 3], p[c]); }
    }
    if (n) console.log(`  mat ${String(m.mat).padStart(2)} : ${String(n).padStart(6)} sommets  x ${bb[0].toFixed(3)}..${bb[3].toFixed(3)} y ${bb[1].toFixed(3)}..${bb[4].toFixed(3)} z ${bb[2].toFixed(3)}..${bb[5].toFixed(3)}`);
  });
}

const args = process.argv.slice(2);
if (args[0] === '--zone') { zone(args[1], args.slice(2, 8).map(Number)); process.exit(0); }
if (args[0] === '--lister') { for (const id of args.slice(1)) lister(id); process.exit(0); }
const verifier = args[0] === '--verifier';
const ids = args.filter((a) => !a.startsWith('--'));
let ok = true;
for (const id of ids.length ? ids : Object.keys(FORMES)) ok = traiter(id, verifier) && ok;
process.exit(ok ? 0 : 1);
