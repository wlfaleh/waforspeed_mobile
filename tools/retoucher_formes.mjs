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
// Une retouche ne crée ni ne retire aucun sommet ni aucun triangle : elle déplace les sommets d'UN maillage (un maillage par
// matériau) qui tombent dans une boîte, autour d'un pivot : p' = pivot + echelle × (p − pivot) + deplacer. « fondu » (m) : les
// sommets hors de la boîte, à moins de « fondu » de ses faces, suivent en partie (poids linéaire), pour ne pas déchirer la pièce.
// Normales : transformées par l'inverse de l'échelle puis renormées ; tangentes (section TAN1) recalculées (tools/tangentes.mjs) ;
// boîte d'en-tête recalculée. Le .lod (indices seuls, sommets partagés) reste valable : il prend la nouvelle forme tel quel.
//
// formes.json : { "<id>": { "materiaux": nMat, "empreinteAvant": "…", "empreinteApres": "…",
//                 "retouches": [ { "materiau": m, "boite": [x0,y0,z0,x1,y1,z1], "pivot": [x,y,z] | "boite",
//                                  "echelle": [sx,sy,sz], "deplacer": [dx,dy,dz], "fondu": f, "sommets": n, "note": "…" } ] } }
// « sommets » : nombre de sommets dans la boîte (contrôle à la 1re application). Repère du .wfm : avant vers −Z, x à droite, y en haut.
import fs from 'fs';
import crypto from 'crypto';
import path from 'path';
import { tangentes } from './tangentes.mjs';

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
const empreinte = (w) => { const h = crypto.createHash('sha256'); for (const m of w.maillages) h.update(Buffer.from(m.v.buffer)); return h.digest('hex').slice(0, 16); };
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

function traiter(id, verifier) {
  const cfg = FORMES[id];
  const chemin = path.join(RACINE, 'assets/cars', id + '.wfm');
  const w = lire(fs.readFileSync(chemin));
  if (w.nM !== cfg.materiaux) throw new Error(`${id} : ${w.nM} matériaux, ${cfg.materiaux} attendus`);
  const e = empreinte(w);
  if (cfg.empreinteApres && e === cfg.empreinteApres) { console.log(`${id} : déjà retouchée (${e})`); return true; }
  if (e !== cfg.empreinteAvant) { console.error(`${id} : empreinte ${e}, ni « avant » (${cfg.empreinteAvant}) ni « après » (${cfg.empreinteApres}) : rien écrit`); return false; }
  const touches = new Set(); const journal = [];
  for (const r of cfg.retouches) {
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
