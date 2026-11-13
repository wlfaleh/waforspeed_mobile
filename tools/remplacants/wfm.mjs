// WaForSpeed — WFS-397 : lecture / écriture minimale d'un .wfm (format décrit dans tools/convert_car.mjs) pour les retouches de
// géométrie des remplaçants : seules les positions des sommets et la boîte de l'en-tête sont réécrites, tout le reste (textures,
// matériaux, normales, uv, indices, section TAN1) est recopié octet pour octet.
import fs from 'fs';

export const ROUE = 128, PEINTURE = 1;

export function lireWfm(chemin) {
  const buf = fs.readFileSync(chemin);
  if (buf.toString('latin1', 0, 4) !== 'WFM1') throw new Error(`${chemin} : pas un .wfm`);
  const u32 = (o) => buf.readUInt32LE(o);
  const nT = u32(8), nM = u32(12), nMe = u32(16);
  let o = 44;
  for (let t = 0; t < nT; t++) o += 4 + u32(o);
  const drapeauxMat = [];
  for (let m = 0; m < nM; m++) { drapeauxMat.push(u32(o + 52)); o += 56; }
  const debutMaillages = o, maillages = [];
  for (let k = 0; k < nMe; k++) {
    const mat = u32(o), nv = u32(o + 4), ni = u32(o + 8); o += 12;
    const pos = new Float32Array(nv * 3);
    for (let i = 0; i < nv; i++) for (let q = 0; q < 3; q++) pos[i * 3 + q] = buf.readFloatLE(o + i * 32 + q * 4);
    const debutSommets = o; o += nv * 32;
    const idx = new Uint32Array(ni);
    for (let i = 0; i < ni; i++) idx[i] = u32(o + i * 4);
    o += ni * 4;
    maillages.push({ mat, drapeaux: drapeauxMat[mat], nv, pos, idx, debutSommets });
  }
  if (o > buf.length) throw new Error(`${chemin} : fichier tronqué`);
  return { buf, maillages, debutMaillages, finMaillages: o };
}

export function ecrireWfm(chemin, w) {
  // en-tête, textures et matériaux recopiés ; maillages réécrits (positions, indices éventuellement réduits par retirerPieces) ;
  // section TAN1 recopiée : le nombre de sommets de chaque maillage ne change jamais
  const morceaux = [Buffer.from(w.buf.subarray(0, w.debutMaillages))];
  const bb = [Infinity, Infinity, Infinity, -Infinity, -Infinity, -Infinity];
  for (const m of w.maillages) {
    const tete = Buffer.alloc(12); tete.writeUInt32LE(m.mat, 0); tete.writeUInt32LE(m.nv, 4); tete.writeUInt32LE(m.idx.length, 8);
    const s = Buffer.from(w.buf.subarray(m.debutSommets, m.debutSommets + m.nv * 32));
    for (let i = 0; i < m.nv; i++) for (let q = 0; q < 3; q++) s.writeFloatLE(m.pos[i * 3 + q], i * 32 + q * 4);
    const ix = Buffer.alloc(m.idx.length * 4);
    m.idx.forEach((v, i) => ix.writeUInt32LE(v, i * 4));
    for (const i of m.idx) for (let q = 0; q < 3; q++) { const v = m.pos[i * 3 + q]; bb[q] = Math.min(bb[q], v); bb[q + 3] = Math.max(bb[q + 3], v); }
    morceaux.push(tete, s, ix);
  }
  morceaux.push(Buffer.from(w.buf.subarray(w.finMaillages)));
  const sortie = Buffer.concat(morceaux);
  for (let q = 0; q < 6; q++) sortie.writeFloatLE(bb[q], 20 + q * 4);
  fs.writeFileSync(chemin, sortie);
}

// pièces d'un maillage : composantes connexes des triangles, sommets soudés par position (au 1/2 mm) ; pour chaque pièce, ses
// triangles (rangs) et sa boîte
export function pieces(m) {
  const cle = (i) => `${Math.round(m.pos[i * 3] * 2000)},${Math.round(m.pos[i * 3 + 1] * 2000)},${Math.round(m.pos[i * 3 + 2] * 2000)}`;
  const premier = new Map(), rep = new Int32Array(m.nv);
  for (let i = 0; i < m.nv; i++) { const k = cle(i); if (!premier.has(k)) premier.set(k, i); rep[i] = premier.get(k); }
  const parent = new Int32Array(m.nv).map((_, i) => i);
  const racine = (i) => { while (parent[i] !== i) { parent[i] = parent[parent[i]]; i = parent[i]; } return i; };
  for (let t = 0; t < m.idx.length; t += 3) for (let j = 1; j < 3; j++) {
    const a = racine(rep[m.idx[t]]), b = racine(rep[m.idx[t + j]]); if (a !== b) parent[a] = b;
  }
  const g = new Map();
  for (let t = 0; t < m.idx.length; t += 3) {
    const c = racine(rep[m.idx[t]]);
    const e = g.get(c) || { triangles: [], boite: [Infinity, Infinity, Infinity, -Infinity, -Infinity, -Infinity] };
    e.triangles.push(t / 3);
    for (let j = 0; j < 3; j++) for (let q = 0; q < 3; q++) { const v = m.pos[m.idx[t + j] * 3 + q]; e.boite[q] = Math.min(e.boite[q], v); e.boite[q + 3] = Math.max(e.boite[q + 3], v); }
    g.set(c, e);
  }
  return [...g.values()];
}

// retire d'un maillage les triangles donnés (rangs) ; les sommets restent (inutilisés), la section TAN1 reste donc valable
export function retirerTriangles(m, rangs) {
  const r = new Set(rangs), idx = [];
  for (let t = 0; t < m.idx.length / 3; t++) if (!r.has(t)) idx.push(m.idx[t * 3], m.idx[t * 3 + 1], m.idx[t * 3 + 2]);
  m.idx = Uint32Array.from(idx);
}

// les 4 roues : maillages marqués « roue » (drapeau 128), regroupés par côté (x) et par essieu (z) ; centre = milieu de la boîte,
// rayon = demi-hauteur, demi-largeur + 1 cm (comme le moteur)
export function roues(w) {
  const b = new Map();
  for (const m of w.maillages) if (m.drapeaux & ROUE) for (const i of m.idx) {
    const x = m.pos[i * 3], y = m.pos[i * 3 + 1], z = m.pos[i * 3 + 2];
    const cle = `${x > 0 ? 'D' : 'G'}${z < 0 ? 'AV' : 'AR'}`;
    const r = b.get(cle) || { cle, min: [Infinity, Infinity, Infinity], max: [-Infinity, -Infinity, -Infinity] };
    [x, y, z].forEach((v, q) => { r.min[q] = Math.min(r.min[q], v); r.max[q] = Math.max(r.max[q], v); });
    b.set(cle, r);
  }
  if (b.size !== 4) throw new Error(`4 roues attendues, ${b.size} trouvées`);
  return [...b.values()].map((r) => ({
    cle: r.cle, avant: r.cle.endsWith('AV'),
    c: [0, 1, 2].map((q) => (r.min[q] + r.max[q]) / 2),
    R: (r.max[1] - r.min[1]) / 2, hx: (r.max[0] - r.min[0]) / 2 + 0.01,
  }));
}

// sommets qui suivent les roues : maillages de roue, et triangles des autres maillages (sauf la peinture) entièrement dans le
// cylindre d'une roue (disques, étriers, moyeux), comme tools/corriger_voitures.cpp
export function sommetsDesRoues(w, R4) {
  const dedans = (p) => R4.some((r) => Math.abs(p[0] - r.c[0]) <= r.hx && Math.hypot(p[1] - r.c[1], p[2] - r.c[2]) <= r.R * 0.97);
  return w.maillages.map((m) => {
    const fixe = new Uint8Array(m.nv);
    if (m.drapeaux & ROUE) { fixe.fill(1); return fixe; }
    if (m.drapeaux & PEINTURE) return fixe;
    const P = (i) => [m.pos[i * 3], m.pos[i * 3 + 1], m.pos[i * 3 + 2]];
    for (let t = 0; t < m.idx.length; t += 3) {
      const a = m.idx[t], b = m.idx[t + 1], c = m.idx[t + 2];
      if (dedans(P(a)) && dedans(P(b)) && dedans(P(c))) fixe[a] = fixe[b] = fixe[c] = 1;
    }
    return fixe;
  });
}

// jeu entre le haut du pneu et le bord du passage de roue (peinture la plus basse au-dessus de la roue, à ±4 cm de son axe,
// dans la moitié extérieure de la roue), en m
export function jeuPassages(w, R4) {
  return R4.map((r) => {
    let bas = Infinity;
    for (const m of w.maillages) if (m.drapeaux & PEINTURE) for (let i = 0; i < m.nv; i++) {
      const x = m.pos[i * 3], y = m.pos[i * 3 + 1], z = m.pos[i * 3 + 2];
      if (Math.abs(z - r.c[2]) < 0.04 && y > r.c[1] && Math.sign(x) === Math.sign(r.c[0]) && Math.abs(x) > Math.abs(r.c[0]) - 0.02) bas = Math.min(bas, y);
    }
    return { cle: r.cle, jeu: bas - (r.c[1] + r.R), arche: bas, hautPneu: r.c[1] + r.R };
  });
}
