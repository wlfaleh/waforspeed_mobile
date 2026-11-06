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
  elantra: { plaque: 10, cadre: -1, numero: 'GN-623-YB', dep: '35' },
  mustang: { plaque: 18, cadre: -1, numero: 'FM-509-GT', dep: '33' },
  m4: { plaque: 35, cadre: -1, numero: 'EB-944-KM', dep: '92' },
  sf90: { plaque: 9, cadre: -1, numero: 'GF-190-XS', dep: '06' },
  model3: { plaque: 24, cadre: -1, numero: 'EY-317-TL', dep: '92' },   // WFS-158 (RG-D) : nouveau modèle, plaque arrière seule
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
for (const id of ids) {
  const cfg = VOITURES[id];
  const chemin = new URL(`assets/cars/${id}.wfm`, RACINE);
  const f = lire(chemin);
  const mp = f.maillages[cfg.plaque], journal = [];
  if (!mp || mp.mat !== cfg.plaque) throw new Error(`${id} : maillage ${cfg.plaque} introuvable`);
  const touches = new Set([cfg.plaque]);
  let change = false;
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
  // image de la plaque, ajoutée à la fin (ou déjà là : dernière texture, posée par cet outil)
  const img = await sharp(await plaque(cfg.numero, cfg.dep)).resize(1024, 256, { fit: 'fill' }).jpeg({ quality: 90 }).toBuffer();
  const mat = mp.mat, M = f.mats[mat];
  const tC = M.readInt32LE(40);
  if (!(tC >= 0 && tC === f.textures.length - 1 && Buffer.compare(f.textures[tC], img) === 0)) {
    f.textures.push(img);
    M.writeInt32LE(f.textures.length - 1, 40); change = true;
  }
  // matériau : blanc opaque, sans normales, satiné (couleur 0-3, métal 4, rugosité 5, émissif 6-8, coupure 9, textures 10-12, drapeaux)
  const voulu = [1, 1, 1, 1, 0, 0.45];
  for (let q = 0; q < 6; q++) if (M.readFloatLE(q * 4) !== Math.fround(voulu[q])) { M.writeFloatLE(voulu[q], q * 4); change = true; }
  if (M.readInt32LE(44) !== -1) { M.writeInt32LE(-1, 44); change = true; }
  const dr = M.readUInt32LE(52), dr2 = dr & ~(2 | 32 | 64);   // ni transparent, ni découpé, ni vitre
  if (dr2 !== dr) { M.writeUInt32LE(dr2, 52); change = true; }
  if (!change) { console.log(`${id} : déjà au format, inchangée`); continue; }
  if (f.tan) for (const k of touches) f.tan[k] = Buffer.from(tangentes(f.maillages[k].v, 8, f.maillages[k].ix).octets.buffer);
  // boîte
  const bb = [1e9, 1e9, 1e9, -1e9, -1e9, -1e9];
  for (const m of f.maillages) for (const i of m.ix) for (let c = 0; c < 3; c++) { const x = m.v[i * 8 + c]; bb[c] = Math.min(bb[c], x); bb[c + 3] = Math.max(bb[c + 3], x); }
  f.boite = bb;
  ecrire(chemin, f);
  console.log(`${id} : ${journal.join(', ')} ; ${cfg.numero} (${cfg.dep}) ; texture ${f.textures.length - 1}`);
}
