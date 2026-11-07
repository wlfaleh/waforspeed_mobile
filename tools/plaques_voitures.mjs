// WaForSpeed — WFS-158 (ST-3, RG-03, M-04) : plaques d'immatriculation européennes de 520 × 110 mm sur les voitures dont la
// plaque est une pièce à part (son propre matériau dans le .wfm) : plaques blanches vides (208, Abarth, Giulia, Compass,
// Corsa), plaque allemande de la 911, plaques « ASSETTO CORSA » de la S3 (nom d'un jeu du commerce). Sur le .wfm lui-même
// (les 4 premières voitures ne se reconvertissent pas à l'identique) :
//   - chaque plaque (avant : z < 0, arrière : z > 0) est remise à 520 × 110 mm dans son plan, autour de son centre (même
//     calcul que WFS_PLAQUE de convert_car.mjs) ; son cadre noir, s'il en a un, à 550 × 140 mm (15 mm de bord) ;
//   - coordonnées de texture refaites à plat (u de gauche à droite et v de haut en bas, vus de l'extérieur), et image d'une
//     plaque française neutre (tools/plaques_neutres.mjs) ajoutée à la fin des textures : les rangs des textures et des
//     matériaux existants ne changent pas (src/render/correctifs_voitures.h les désigne par rang) ;
//   - matériau de la plaque : blanc, opaque, sans carte de normales (les lettres en relief d'origine disparaissent),
//     rugosité 0,45 (film rétroréfléchissant satiné).
// Les indices ne changent pas (les .lod restent valables) ; tangentes (section TAN1) recalculées pour les maillages touchés.
// Rejouable : une plaque déjà au format et déjà texturée par cet outil n'est plus modifiée.
// usage : node plaques_voitures.mjs [id …]   (sans id : toutes les voitures du tableau) ; WFS_RACINE=<dossier> : essai sur une copie
import fs from 'fs';
import sharp from 'sharp';
import { plaque } from './plaques_neutres.mjs';
import { tangentes } from './tangentes.mjs';

const RACINE = process.env.WFS_RACINE ? new URL(`file://${process.env.WFS_RACINE.replace(/\/?$/, '/')}`) : new URL('../', import.meta.url);   // WFS_RACINE : essai sur une copie
const L = 0.520, H = 0.110, BORD = 0.015, TOL = 0.01;
// voiture : maillage de la plaque (= rang de son matériau), maillage du cadre (ou -1), numéro inventé AA-123-AA et département
const VOITURES = {
  p208: { plaque: 16, cadre: 15, numero: 'DZ-418-HP', dep: '75' },
  abarth: { plaque: 56, cadre: 55, numero: 'BL-592-NE', dep: '06' },
  giulia: { plaque: 65, cadre: 64, numero: 'GA-731-RC', dep: '13' },
  compass: { plaque: 14, cadre: 13, numero: 'EK-266-LV', dep: '31' },
  corsa: { plaque: 0, cadre: -1, numero: 'FC-850-MS', dep: '44' },
  p911: { plaque: 11, cadre: -1, numero: 'HD-097-TZ', dep: '67' },
  s3: { plaque: 18, cadre: -1, numero: 'CW-374-JN', dep: '59' },
  // plaques arrière américaines ou de concession (« ELANTRA N », « FORD PERFORMANCE », Texas) et italienne de la SF90
  // + plaque avant ajoutée (le modèle n'en a pas ; hauteur du centre choisie sur la surface la plus plate du bouclier)
  elantra: { plaque: 10, cadre: -1, numero: 'GN-623-YB', dep: '35', ajouts: [{ cote: -1, y0: 0.44 }] },
  mustang: { plaque: 18, cadre: -1, numero: 'FM-509-GT', dep: '33', ajouts: [{ cote: -1, y0: 0.44 }] },
  m4: { plaque: 35, cadre: -1, numero: 'EB-944-KM', dep: '92', ajouts: [{ cote: -1, y0: 0.37 }] },
  sf90: { plaque: 9, cadre: -1, numero: 'GF-190-XS', dep: '06', ajouts: [{ cote: -1, y0: 0.43 }] },
  model3: { plaque: 24, cadre: -1, numero: 'EY-317-TL', dep: '92', ajouts: [{ cote: -1, y0: 0.44 }] },   // WFS-158 (RG-D) : nouveau modèle
  // aucune plaque à part : plaques ajoutées sur les porte-plaques noirs (Golf), sur les plaques de concession « T-Cross » et
  // « 途铠 » (masquées : atlas du modèle, matériau 10), dans le logement du hayon (X5), sur le porte-plaque gris (GT86)
  golf: { plaque: -1, cadre: -1, numero: 'BJ-276-WC', dep: '38', ajouts: [{ cote: -1, y0: 0.471 }, { cote: 1, y0: 0.50 }] },
  gallardo: { plaque: -1, cadre: -1, numero: 'DL-865-ZP', dep: '83', ajouts: [{ cote: -1, y0: 0.36 }, { cote: 1, y0: 0.37 }] },
  // T-Cross : version chinoise à empattement long (4,218 m, WFS-158) ; positions mesurées à cette taille
  tcross: { plaque: -1, cadre: -1, numero: 'FT-148-CX', dep: '57', ajouts: [{ cote: -1, y0: 0.485 }, { cote: 1, y0: 0.823 }],
            masquer: { maillage: 10, boites: [[-0.246, 0.246, 0.40, 0.57, -2.135, -2.073], [-0.257, 0.257, 0.739, 0.908, 1.970, 2.032]] } },
  x5: { plaque: -1, cadre: -1, numero: 'HB-530-RV', dep: '78', ajouts: [{ cote: -1, y0: 0.56 }, { cote: 1, y0: 0.70 }] },
  gt86: { plaque: -1, cadre: -1, numero: 'CG-861-PZ', dep: '69', ajouts: [{ cote: -1, y0: 0.54 }, { cote: 1, y0: 0.548 }] },
};

function lire(chemin) {
  const b = fs.readFileSync(chemin);
  let o = 4;
  const u32 = () => { const x = b.readUInt32LE(o); o += 4; return x; };
  const f = { version: u32() };
  const nT = u32(), nM = u32(), nMe = u32();
  f.boite = Array.from({ length: 6 }, (_, i) => b.readFloatLE(o + 4 * i)); o += 24;
  f.textures = []; for (let t = 0; t < nT; t++) { const n = u32(); f.textures.push(b.subarray(o, o + n)); o += n; }
  f.mats = []; for (let i = 0; i < nM; i++) { f.mats.push(Buffer.from(b.subarray(o, o + 56))); o += 56; }
  f.maillages = [];
  for (let k = 0; k < nMe; k++) {
    const mat = u32(), nv = u32(), ni = u32();
    const v = new Float32Array(b.buffer.slice(b.byteOffset + o, b.byteOffset + o + nv * 32)); o += nv * 32;
    const ix = new Uint32Array(b.buffer.slice(b.byteOffset + o, b.byteOffset + o + ni * 4)); o += ni * 4;
    f.maillages.push({ mat, v, ix });
  }
  f.tan = null;
  if (f.version >= 2 && b.toString('latin1', o, o + 4) === 'TAN1') {
    o += 4; f.tan = [];
    for (const m of f.maillages) { const n = m.v.length / 8; f.tan.push(b.subarray(o, o + n * 4)); o += n * 4; }
  }
  if (o !== b.length) throw new Error(`${chemin} : ${b.length - o} octets inattendus en fin de fichier`);
  return f;
}
function ecrire(chemin, f) {
  const parts = [], u32 = (...a) => parts.push(Buffer.from(new Uint32Array(a).buffer));
  parts.push(Buffer.from('WFM1')); u32(f.version, f.textures.length, f.mats.length, f.maillages.length);
  parts.push(Buffer.from(new Float32Array(f.boite).buffer));
  for (const t of f.textures) { u32(t.length); parts.push(t); }
  for (const m of f.mats) parts.push(m);
  for (const m of f.maillages) { u32(m.mat, m.v.length / 8, m.ix.length); parts.push(Buffer.from(m.v.buffer)); parts.push(Buffer.from(m.ix.buffer)); }
  if (f.tan) { parts.push(Buffer.from('TAN1')); for (const t of f.tan) parts.push(t); }
  fs.writeFileSync(chemin, Buffer.concat(parts));
}

const dot = (a, c) => a[0] * c[0] + a[1] * c[1] + a[2] * c[2];
const unit = (a) => { const l = Math.hypot(a[0], a[1], a[2]) || 1; return [a[0] / l, a[1] / l, a[2] / l]; };
// repère d'une plaque (côté −1 avant, +1 arrière) : normale vers l'extérieur, X (x projeté dans le plan), U (verticale du plan)
function repere(m, sommets, cote) {
  let n = [0, 0, 0];
  for (const i of sommets) { const nz = m.v[i * 8 + 5]; if (Math.abs(nz) > 0.5) { const sg = Math.sign(nz) === cote ? 1 : -1; for (let c = 0; c < 3; c++) n[c] += sg * m.v[i * 8 + 3 + c]; } }
  if (Math.hypot(...n) < 0.5) {   // normales des sommets inutilisables (SF90) : normales des faces de la plaque
    const dans = new Set(sommets); n = [0, 0, 0];
    for (let t = 0; t + 2 < m.ix.length; t += 3) {
      const c = [m.ix[t], m.ix[t + 1], m.ix[t + 2]]; if (!c.every((i) => dans.has(i))) continue;
      const p = c.map((i) => [m.v[i * 8], m.v[i * 8 + 1], m.v[i * 8 + 2]]);
      const e1 = [p[1][0] - p[0][0], p[1][1] - p[0][1], p[1][2] - p[0][2]], e2 = [p[2][0] - p[0][0], p[2][1] - p[0][1], p[2][2] - p[0][2]];
      const f = [e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]];
      if (Math.abs(f[2]) < 0.5 * Math.hypot(...f)) continue;
      const sg = Math.sign(f[2]) === cote ? 1 : -1; for (let q = 0; q < 3; q++) n[q] += sg * f[q];
    }
  }
  n = unit(n);
  const X = unit([1 - n[0] * n[0], -n[0] * n[1], -n[0] * n[2]]);
  let U = unit([n[1] * X[2] - n[2] * X[1], n[2] * X[0] - n[0] * X[2], n[0] * X[1] - n[1] * X[0]]);
  if (U[1] < 0) U = [-U[0], -U[1], -U[2]];
  const P = sommets.map((i) => [m.v[i * 8], m.v[i * 8 + 1], m.v[i * 8 + 2]]);
  const ctr = [0, 1, 2].map((c) => (Math.min(...P.map((p) => p[c])) + Math.max(...P.map((p) => p[c]))) / 2);
  const a = P.map((p) => dot([p[0] - ctr[0], p[1] - ctr[1], p[2] - ctr[2]], X)), h = P.map((p) => dot([p[0] - ctr[0], p[1] - ctr[1], p[2] - ctr[2]], U));
  const r = { n, X, U, ctr, P, a, h, aMin: Math.min(...a), aMax: Math.max(...a), hMin: Math.min(...h), hMax: Math.max(...h) };
  r.largeur = r.aMax - r.aMin; r.hauteur = r.hMax - r.hMin;
  return r;
}
// étire les sommets dans le plan du repère pour donner largeur × hauteur (autour du centre de la boîte du plan)
function etirer(m, sommets, r, largeur, hauteur) {
  const sx = largeur / r.largeur, sy = hauteur / r.hauteur, a0 = (r.aMin + r.aMax) / 2, h0 = (r.hMin + r.hMax) / 2;
  sommets.forEach((i, j) => {
    const d = [r.P[j][0] - r.ctr[0], r.P[j][1] - r.ctr[1], r.P[j][2] - r.ctr[2]], e = dot(d, r.n);
    const na = a0 + (r.a[j] - a0) * sx, nh = h0 + (r.h[j] - h0) * sy;
    for (let c = 0; c < 3; c++) m.v[i * 8 + c] = r.ctr[c] + r.X[c] * na + r.U[c] * nh + r.n[c] * e;
  });
}
const cotes = (m) => {   // sommets de la plaque avant (z < 0) et arrière (z > 0)
  const s = { '-1': [], 1: [] };
  for (let i = 0; i < m.v.length / 8; i++) if (m.v[i * 8 + 2] !== 0) s[Math.sign(m.v[i * 8 + 2])].push(i);
  return s;
};

// avec une liste d'id (tools/convertir_voitures.sh) : seules celles du tableau ; les autres n'ont pas de plaque à part
const ids = process.argv.slice(2).length ? process.argv.slice(2).filter((id) => VOITURES[id]) : Object.keys(VOITURES);
// ---------- WFS-158 (ST-3) : plaque ajoutée là où le modèle n'en a pas (avant de l'Elantra, de la Golf…) ----------
// Rayons lancés vers la voiture sur une grille de 11 × 5 points du rectangle de la plaque (centre x0, y0) : la surface la plus en
// avant de la caisse (vitres et pièces transparentes exceptées) ; plan ajusté sur ces points (moindres carrés : z = a + b x + c y),
// avancé pour passer 3 mm devant le point le plus en avant ; rectangle de 520 × 110 mm dans ce plan, double face, image de la plaque.
function poserPlaque(f, cote, x0, y0, journal) {
  const xs = [], hits = [];
  for (let i = 0; i < 11; i++) for (let j = 0; j < 5; j++) xs.push([x0 - L / 2 + 0.01 + (L - 0.02) * i / 10, y0 - H / 2 + 0.01 + (H - 0.02) * j / 4]);
  const cand = [];
  f.maillages.forEach((m) => {
    const dr = f.mats[m.mat].readUInt32LE(52);
    if (dr & (2 | 64)) return;   // vitres, pièces transparentes
    for (let t = 0; t + 2 < m.ix.length; t += 3) {
      const P = [0, 1, 2].map((q) => [m.v[m.ix[t + q] * 8], m.v[m.ix[t + q] * 8 + 1], m.v[m.ix[t + q] * 8 + 2]]);
      if (Math.max(...P.map((p) => p[0])) < x0 - L / 2 || Math.min(...P.map((p) => p[0])) > x0 + L / 2) continue;
      if (Math.max(...P.map((p) => p[1])) < y0 - H / 2 || Math.min(...P.map((p) => p[1])) > y0 + H / 2) continue;
      if (cote < 0 ? Math.min(...P.map((p) => p[2])) > 0 : Math.max(...P.map((p) => p[2])) < 0) continue;
      cand.push(P);
    }
  });
  for (const [x, y] of xs) {
    let best = null;
    for (const P of cand) {   // point dans le triangle projeté sur (x, y), z par les coordonnées barycentriques
      const d = (P[1][1] - P[2][1]) * (P[0][0] - P[2][0]) + (P[2][0] - P[1][0]) * (P[0][1] - P[2][1]);
      if (Math.abs(d) < 1e-12) continue;
      const l0 = ((P[1][1] - P[2][1]) * (x - P[2][0]) + (P[2][0] - P[1][0]) * (y - P[2][1])) / d;
      const l1 = ((P[2][1] - P[0][1]) * (x - P[2][0]) + (P[0][0] - P[2][0]) * (y - P[2][1])) / d, l2 = 1 - l0 - l1;
      if (l0 < -1e-6 || l1 < -1e-6 || l2 < -1e-6) continue;
      const z = l0 * P[0][2] + l1 * P[1][2] + l2 * P[2][2];
      if (best === null || (cote < 0 ? z < best : z > best)) best = z;
    }
    if (best !== null) hits.push([x, y, best]);
  }
  if (hits.length < 30) { journal.push(`plaque ${cote < 0 ? 'avant' : 'arrière'} NON posée (${hits.length} / 55 points sur la caisse)`); return null; }
  // surface « portante » : points à moins de 4 cm du plus en avant (les trous de calandre plus profonds ne comptent pas pour le plan)
  const front = cote < 0 ? Math.min(...hits.map((h) => h[2])) : Math.max(...hits.map((h) => h[2]));
  const port = hits.filter((h) => Math.abs(h[2] - front) <= 0.04);
  // moindres carrés z = a + b (x − x0) + c (y − y0)
  let S = [[0, 0, 0], [0, 0, 0], [0, 0, 0]], R = [0, 0, 0];
  for (const [x, y, z] of port) { const g = [1, x - x0, y - y0]; for (let i = 0; i < 3; i++) { R[i] += g[i] * z; for (let j = 0; j < 3; j++) S[i][j] += g[i] * g[j]; } }
  const det3 = (M) => M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1]) - M[0][1] * (M[1][0] * M[2][2] - M[1][2] * M[2][0]) + M[0][2] * (M[1][0] * M[2][1] - M[1][1] * M[2][0]);
  const D = det3(S), sol = [0, 1, 2].map((k) => det3(S.map((row, i) => row.map((v, j) => (j === k ? R[i] : v)))) / D);
  let [a, b, c] = sol;
  // plan avancé : 3 mm devant le point le plus en avant de toute la grille
  const ecart = hits.map(([x, y, z]) => z - (a + b * (x - x0) + c * (y - y0)));
  a += cote < 0 ? Math.min(...ecart) - 0.003 : Math.max(...ecart) + 0.003;
  const vide = Math.max(...port.map(([x, y, z]) => Math.abs(z - (a + b * (x - x0) + c * (y - y0)))));
  // repère du plan : normale vers l'extérieur, X (x dans le plan), U (verticale du plan)
  const n = unit(cote < 0 ? [b, c, -1] : [-b, -c, 1]);
  const X = unit([1, 0, b]);
  let U = unit([n[1] * X[2] - n[2] * X[1], n[2] * X[0] - n[0] * X[2], n[0] * X[1] - n[1] * X[0]]); if (U[1] < 0) U = U.map((q) => -q);
  const P0 = [x0, y0, a];
  const v = new Float32Array(4 * 8), ix = new Uint32Array([0, 1, 2, 0, 2, 3]);
  [[-1, 1], [1, 1], [1, -1], [-1, -1]].forEach(([sx, sy], k) => {
    for (let q = 0; q < 3; q++) v[k * 8 + q] = P0[q] + X[q] * sx * L / 2 + U[q] * sy * H / 2;
    for (let q = 0; q < 3; q++) v[k * 8 + 3 + q] = n[q];
    // vu de l'extérieur : la gauche est +x à l'avant, −x à l'arrière ; v = 0 en haut
    v[k * 8 + 6] = cote < 0 ? (1 - sx) / 2 : (1 + sx) / 2; v[k * 8 + 7] = (1 - sy) / 2;
  });
  journal.push(`plaque ${cote < 0 ? 'avant' : 'arrière'} posée (centre x ${x0.toFixed(2)} y ${y0.toFixed(2)} z ${a.toFixed(3)}, inclinaison ${(Math.atan(c) * 180 / Math.PI).toFixed(1)}°, plus grand vide sous la plaque ${(vide * 1000).toFixed(0)} mm)`);
  return { v, ix };
}
const matPlaque = (tex) => {   // blanc opaque, satiné, double face (couleur, métal, rugosité, émissif, coupure, textures, drapeaux)
  const M = Buffer.alloc(56);
  [1, 1, 1, 1, 0, 0.45, 0, 0, 0, 0.5].forEach((x, q) => M.writeFloatLE(x, q * 4));
  M.writeInt32LE(tex, 40); M.writeInt32LE(-1, 44); M.writeInt32LE(-1, 48); M.writeUInt32LE(16, 52);
  return M;
};

for (const id of ids) {
  const cfg = VOITURES[id];
  const chemin = new URL(`assets/cars/${id}.wfm`, RACINE);
  const f = lire(chemin);
  const journal = [], touches = new Set();
  let change = false;
  const img = await sharp(await plaque(cfg.numero, cfg.dep)).resize(1024, 256, { fit: 'fill' }).jpeg({ quality: 90 }).toBuffer();
  let tImg = f.textures.findIndex((t) => Buffer.compare(t, img) === 0);
  const texture = () => { if (tImg < 0) { f.textures.push(img); tImg = f.textures.length - 1; change = true; } return tImg; };
  if (cfg.plaque >= 0) {
  const mp = f.maillages[cfg.plaque];
  if (!mp || mp.mat !== cfg.plaque) throw new Error(`${id} : maillage ${cfg.plaque} introuvable`);
  touches.add(cfg.plaque);
  const sp = cotes(mp), sc = cfg.cadre >= 0 ? cotes(f.maillages[cfg.cadre]) : null;
  for (const cote of [-1, 1]) {
    const som = sp[cote];
    if (!som.length) continue;
    const r = repere(mp, som, cote);
    const avant = `${(r.largeur * 1000).toFixed(0)}×${(r.hauteur * 1000).toFixed(0)}`;
    const auFormat = Math.abs(r.largeur / L - 1) <= TOL && Math.abs(r.hauteur / H - 1) <= TOL;
    if (!auFormat) { etirer(mp, som, r, L, H); change = true; }
    // cadre : 15 mm autour de la plaque s'il est plus petit
    if (sc && sc[cote].length) {
      const mc = f.maillages[cfg.cadre], rc = repere(mc, sc[cote], cote);
      if (rc.largeur < L + 2 * BORD - 0.001 || rc.hauteur < H + 2 * BORD - 0.001) {
        etirer(mc, sc[cote], rc, Math.max(rc.largeur, L + 2 * BORD), Math.max(rc.hauteur, H + 2 * BORD)); touches.add(cfg.cadre); change = true;
        journal.push(`cadre ${cote < 0 ? 'avant' : 'arrière'} ${(rc.largeur * 1000).toFixed(0)}×${(rc.hauteur * 1000).toFixed(0)} → ${(Math.max(rc.largeur, L + 2 * BORD) * 1000).toFixed(0)}×${(Math.max(rc.hauteur, H + 2 * BORD) * 1000).toFixed(0)} mm`);
      }
    }
    // plaque posée sur la caisse : une pièce d'un autre matériau qui la traverse dans son rectangle (bouclier bombé de la Corsa)
    // la ferait disparaître en partie ; la plaque (et son cadre) avance alors devant elle, de 3 mm, 5 cm au plus
    {
      const r1 = repere(mp, som, cote);
      const face = Math.max(...som.map((i) => dot([mp.v[i * 8] - r1.ctr[0], mp.v[i * 8 + 1] - r1.ctr[1], mp.v[i * 8 + 2] - r1.ctr[2]], r1.n)));
      const a0 = (r1.aMin + r1.aMax) / 2, h0 = (r1.hMin + r1.hMax) / 2;
      // triangles tournés vers l'extérieur, dans le rectangle de la plaque : on retient la position où leur surface cumulée
      // (du plus en avant au moins en avant) atteint 5 % de la plaque (une vis ou un crochet de remorquage ne compte pas)
      const tris = [];
      f.maillages.forEach((m, k) => {
        if (k === cfg.plaque || k === cfg.cadre) return;
        for (let t = 0; t + 2 < m.ix.length; t += 3) {
          const P = [0, 1, 2].map((j) => [m.v[m.ix[t + j] * 8], m.v[m.ix[t + j] * 8 + 1], m.v[m.ix[t + j] * 8 + 2]]);
          const c = [0, 1, 2].map((q) => (P[0][q] + P[1][q] + P[2][q]) / 3), d = [c[0] - r1.ctr[0], c[1] - r1.ctr[1], c[2] - r1.ctr[2]];
          if (Math.abs(dot(d, r1.X) - a0) > L / 2 - 0.005 || Math.abs(dot(d, r1.U) - h0) > H / 2 - 0.005) continue;
          const e1 = [P[1][0] - P[0][0], P[1][1] - P[0][1], P[1][2] - P[0][2]], e2 = [P[2][0] - P[0][0], P[2][1] - P[0][1], P[2][2] - P[0][2]];
          const nn = [e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]], aire = Math.hypot(...nn) / 2;
          if (aire < 1e-9 || Math.abs(dot(unit(nn), r1.n)) < 0.3) continue;
          const e = Math.max(...P.map((p) => dot([p[0] - r1.ctr[0], p[1] - r1.ctr[1], p[2] - r1.ctr[2]], r1.n)));
          if (e < face - 0.10 || e > face + 0.10) continue;
          tris.push([e, aire * Math.abs(dot(unit(nn), r1.n))]);
        }
      });
      tris.sort((x, y) => y[0] - x[0]);
      let devant = -1e9, cumul = 0;
      for (const [e, a] of tris) { cumul += a; if (cumul >= 0.05 * L * H) { devant = e; break; } }
      const recul = devant + 0.003 - face;
      if (recul > 0.0005 && recul <= 0.05) {
        const bouger = (m, liste) => { for (const i of liste) for (let c = 0; c < 3; c++) m.v[i * 8 + c] += r1.n[c] * recul; };
        bouger(mp, som); if (sc && sc[cote].length) { bouger(f.maillages[cfg.cadre], sc[cote]); touches.add(cfg.cadre); }
        change = true; journal.push(`plaque ${cote < 0 ? 'avant' : 'arrière'} avancée de ${(recul * 1000).toFixed(0)} mm (traversée par la caisse)`);
      } else if (recul > 0.05) journal.push(`plaque ${cote < 0 ? 'avant' : 'arrière'} traversée de ${(recul * 1000).toFixed(0)} mm : NON corrigée (plus de 5 cm)`);
    }
    // coordonnées de texture à plat : vu de l'extérieur, la droite est −x à l'avant, +x à l'arrière
    const r2 = repere(mp, som, cote);
    som.forEach((i, j) => {
      let u = (r2.a[j] - r2.aMin) / r2.largeur; if (cote < 0) u = 1 - u;
      const v = (r2.hMax - r2.h[j]) / r2.hauteur;
      if (Math.abs(mp.v[i * 8 + 6] - u) > 1e-6 || Math.abs(mp.v[i * 8 + 7] - v) > 1e-6) change = true;
      mp.v[i * 8 + 6] = u; mp.v[i * 8 + 7] = v;
    });
    journal.push(`plaque ${cote < 0 ? 'avant' : 'arrière'} ${avant} → ${(r2.largeur * 1000).toFixed(0)}×${(r2.hauteur * 1000).toFixed(0)} mm`);
  }
  // image de la plaque, ajoutée à la fin des textures (ou déjà là, posée par cet outil)
  const mat = mp.mat, M = f.mats[mat];
  const tP = texture();
  if (M.readInt32LE(40) !== tP) { M.writeInt32LE(tP, 40); change = true; }
  // matériau : blanc opaque, sans normales, satiné (couleur 0-3, métal 4, rugosité 5, émissif 6-8, coupure 9, textures 10-12, drapeaux)
  const voulu = [1, 1, 1, 1, 0, 0.45];
  for (let q = 0; q < 6; q++) if (M.readFloatLE(q * 4) !== Math.fround(voulu[q])) { M.writeFloatLE(voulu[q], q * 4); change = true; }
  if (M.readInt32LE(44) !== -1) { M.writeInt32LE(-1, 44); change = true; }
  const dr = M.readUInt32LE(52), dr2 = dr & ~(2 | 32 | 64);   // ni transparent, ni découpé, ni vitre
  if (dr2 !== dr) { M.writeUInt32LE(dr2, 52); change = true; }
  }
  // pièces masquées (plaques de concession d'un atlas partagé) : triangles dont le centre est dans une boîte, rendus vides
  if (cfg.masquer) {
    const m = f.maillages[cfg.masquer.maillage]; let n = 0;
    for (let t = 0; t + 2 < m.ix.length; t += 3) {
      if (m.ix[t] === m.ix[t + 1] && m.ix[t] === m.ix[t + 2]) continue;
      const c = [0, 1, 2].map((q) => (m.v[m.ix[t] * 8 + q] + m.v[m.ix[t + 1] * 8 + q] + m.v[m.ix[t + 2] * 8 + q]) / 3);
      if (cfg.masquer.boites.some((B) => c[0] >= B[0] && c[0] <= B[1] && c[1] >= B[2] && c[1] <= B[3] && c[2] >= B[4] && c[2] <= B[5])) {
        m.ix[t + 1] = m.ix[t]; m.ix[t + 2] = m.ix[t]; n++;
      }
    }
    if (n) { change = true; journal.push(`${n} triangles de plaques de concession masqués`); }
  }
  // plaques ajoutées (cfg.ajouts : côté −1 avant / +1 arrière, centre x0, y0) : une seule fois (maillage de 4 sommets avec l'image)
  for (const aj of cfg.ajouts || []) {
    const deja = f.maillages.some((m) => m.v.length === 32 && f.mats[m.mat].readInt32LE(40) === tImg && tImg >= 0 && Math.sign(m.v[2]) === aj.cote);
    if (deja) continue;
    const q = poserPlaque(f, aj.cote, aj.x0 || 0, aj.y0, journal);
    if (!q) { process.exitCode = 1; continue; }
    const t = texture();
    f.mats.push(matPlaque(t));
    f.maillages.push({ mat: f.mats.length - 1, v: q.v, ix: q.ix });
    if (f.tan) f.tan.push(null);
    touches.add(f.maillages.length - 1); change = true;
  }
  if (!change) { console.log(`${id} : déjà au format, inchangée`); continue; }
  if (f.tan) for (const k of touches) f.tan[k] = Buffer.from(tangentes(f.maillages[k].v, 8, f.maillages[k].ix).octets.buffer);
  // boîte
  const bb = [1e9, 1e9, 1e9, -1e9, -1e9, -1e9];
  for (const m of f.maillages) for (const i of m.ix) for (let c = 0; c < 3; c++) { const x = m.v[i * 8 + c]; bb[c] = Math.min(bb[c], x); bb[c + 3] = Math.max(bb[c + 3], x); }
  f.boite = bb;
  ecrire(chemin, f);
  console.log(`${id} : ${journal.join(', ')} ; ${cfg.numero} (${cfg.dep}) ; texture ${tImg}`);
}
