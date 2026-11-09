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
// Trois sortes de retouches (détail plus bas) :
//   - déplacement : déplace les sommets d'UN maillage (un maillage par matériau) qui tombent dans une boîte, sans créer ni retirer
//     de sommet ni de triangle ; « fondu » (m) : les sommets hors de la boîte, à moins de « fondu », suivent en partie ;
//     normales transformées par l'inverse de l'échelle ; le .lod (indices seuls) reste valable et prend la nouvelle forme ;
//   - retrait : vide des triangles (indices confondus) ; aucun sommet retiré ;
//   - grille : AJOUTE un matériau (à la fin) et un maillage.
//   Après un retrait ou une grille, le .lod est régénéré (tools/alleger_voiture.mjs). Dans tous les cas : tangentes (TAN1) des
//   maillages touchés recalculées (tools/tangentes.mjs), boîte d'en-tête recalculée.
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

// grille générique (WFS-193, revue du lead dev sur #308) : fond, cadre en relief et barres horizontales fermées, qui suivent le galbe
// du nez. Boîte [x0,y0,z0,x1,y1,z1] : z0 = avant des barres et du cadre, z1 = fond (au centre) ; « galbe » (m) : recul du fond, des
// barres et du cadre aux bords gauche et droit (z + galbe × u², u = −1..1 en largeur) ; « trapeze » : largeur du bas / largeur du
// haut (1 : rectangle) ; « cadre » : largeur du cadre (m) ; « galbeCadre » : galbe de la face avant du cadre (par défaut celui des barres) ; « barres » : nombre de barres. Surfaces découpées en 16 colonnes.
function grille(w, r) {
  const [x0, y0, z0, x1, y1, z1] = r.grille;
  const NX = 16, cx = (x0 + x1) / 2, hwH = (x1 - x0) / 2, hwB = hwH * (r.trapeze ?? 1), g = r.galbe ?? 0, ca = r.cadre ?? 0.015;
  const gc = r.galbeCadre ?? g;   // galbe de la face avant du cadre (moins que les barres : le cadre couvre la jonction avec la carrosserie)
  const V = [], I = [];
  const hw = (y) => hwB + (hwH - hwB) * ((y - y0) / (y1 - y0));
  const zg = (x, z, gg = g) => z + gg * Math.pow((x - cx) / hwH, 2);
  const nf = (x, gg = g) => { const d = 2 * gg * (x - cx) / (hwH * hwH), l = Math.hypot(d, 1); return [d / l, 0, -1 / l]; };   // normale de face (vers −z)
  const s = (x, y, z, n) => { V.push(x, y, z, n[0], n[1], n[2], (x - x0) / (x1 - x0), (y - y0) / (y1 - y0)); return V.length / 8 - 1; };
  const quad = (a, b, c, d) => I.push(a, b, c, a, c, d);
  // bande entre deux lignes horizontales (y bas → y haut) à la profondeur z (avant), insettées de « ins » sur chaque bord
  const face = (ya, yb, z, ins, gg = g) => {
    for (let k = 0; k < NX; k++) {
      const t0 = k / NX, t1 = (k + 1) / NX;
      const xa0 = cx - hw(ya) + ins + t0 * 2 * (hw(ya) - ins), xa1 = cx - hw(ya) + ins + t1 * 2 * (hw(ya) - ins);
      const xb0 = cx - hw(yb) + ins + t0 * 2 * (hw(yb) - ins), xb1 = cx - hw(yb) + ins + t1 * 2 * (hw(yb) - ins);
      quad(s(xa0, ya, zg(xa0, z, gg), nf(xa0, gg)), s(xb0, yb, zg(xb0, z, gg), nf(xb0, gg)), s(xb1, yb, zg(xb1, z, gg), nf(xb1, gg)), s(xa1, ya, zg(xa1, z, gg), nf(xa1, gg)));
    }
  };
  // côté horizontal d'un relief (dessus n = +y, dessous n = −y) entre z0 et z1, le long de y
  const tranche = (y, ins, haut, gg = g) => {   // gg : galbe du bord avant
    const n = haut ? [0, 1, 0] : [0, -1, 0];
    for (let k = 0; k < NX; k++) {
      const xa = cx - hw(y) + ins + (k / NX) * 2 * (hw(y) - ins), xb = cx - hw(y) + ins + ((k + 1) / NX) * 2 * (hw(y) - ins);
      const a = s(xa, y, zg(xa, z0, gg), n), b = s(xb, y, zg(xb, z0, gg), n), c = s(xb, y, zg(xb, z1), n), d = s(xa, y, zg(xa, z1), n);
      if (haut) quad(a, d, c, b); else quad(a, b, c, d);
    }
  };
  // bout fermé d'un relief (côté gauche ou droit) entre ya et yb, à x = bord insetté
  const bout = (ya, yb, ins, gauche, gg = g) => {
    const xa = gauche ? cx - hw(ya) + ins : cx + hw(ya) - ins, xb = gauche ? cx - hw(yb) + ins : cx + hw(yb) - ins;
    const n = gauche ? [-1, 0, 0] : [1, 0, 0];
    const a = s(xa, ya, zg(xa, z0, gg), n), b = s(xb, yb, zg(xb, z0, gg), n), c = s(xb, yb, zg(xb, z1), n), d = s(xa, ya, zg(xa, z1), n);
    if (gauche) quad(a, d, c, b); else quad(a, b, c, d);
  };
  // fond (à z1, un peu en retrait derrière le cadre)
  face(y0, y1, z1 + 0.002, 0);
  // cadre : bandes haute et basse (face avant + tranches), montants gauche et droit (face avant + côtés)
  face(y1 - ca, y1, z0, 0, gc); tranche(y1 - ca, ca, false, gc);
  face(y0, y0 + ca, z0, 0, gc); tranche(y0 + ca, ca, true, gc);
  for (const gauche of [true, false]) {
    const xs = (y, ins) => (gauche ? cx - hw(y) + ins : cx + hw(y) - ins);
    const n0 = nf(xs(y0, 0), gc);
    quad(s(xs(y0, 0), y0, zg(xs(y0, 0), z0, gc), n0), s(xs(y1, 0), y1, zg(xs(y1, 0), z0, gc), n0), s(xs(y1, ca), y1, zg(xs(y1, ca), z0, gc), n0), s(xs(y0, ca), y0, zg(xs(y0, ca), z0, gc), n0));
    bout(y0, y1, 0, gauche, gc); bout(y0, y1, ca, !gauche, gc);
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

function traiter(id, verifier) {
  const cfg = FORMES[id];
  if (!cfg) { console.log(`${id} : aucune retouche de forme (absente de formes.json)`); return true; }   // chaîne convertir_voitures.sh
  const chemin = path.join(RACINE, 'assets/cars', id + '.wfm');
  const w = lire(fs.readFileSync(chemin));
  const ajoutes = cfg.retouches.filter((r) => r.grille).length;   // matériaux ajoutés à la fin par les grilles
  if (w.nM !== cfg.materiaux && w.nM !== cfg.materiaux + ajoutes) throw new Error(`${id} : ${w.nM} matériaux, ${cfg.materiaux} attendus`);
  const e = empreinte(w);
  if (cfg.empreinteApres && e === cfg.empreinteApres) { console.log(`${id} : déjà retouchée (${e})`); return true; }
  if (e !== cfg.empreinteAvant) { console.error(`${id} : empreinte ${e}, ni « avant » (${cfg.empreinteAvant}) ni « après » (${cfg.empreinteApres}) : rien écrit`); return false; }
  const touches = new Set(); const journal = [];
  let structure = false;   // triangles retirés ou pièce ajoutée : .lod à régénérer
  for (const r of cfg.retouches) {
    if (r.grille) { const { k, n } = grille(w, r); touches.add(k); structure = true; journal.push(`${r.note} (${n} triangles ajoutés)`); continue; }
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
