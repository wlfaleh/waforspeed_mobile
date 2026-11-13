// WaForSpeed — WFS-488 (épic E18) : couleur secondaire des voitures. Repère, dans la carrosserie peinte de chaque voiture, le
// PAVILLON (entre les montants, jusqu'aux joints du pare-brise et de la lunette) et les COQUES DE RÉTROVISEURS (glace exclue :
// autre matériau), et l'écrit dans assets/cars/<id>.pt2, à côté du .wfm, sans toucher au .wfm (sommets, indices, TAN1, .lod,
// empreintes de tools/formes.json et de tools/logos.json : tous inchangés ; une voiture sans couleur 2 reste identique au pixel).
// Le moteur (src/render/vehicule_peinture2.cpp) lit ce fichier au chargement : sommets du toit et des rétros marqués dans le même
// maillage (aucun appel de dessin en plus), sommets du bord dédoublés pour une limite nette sur le vrai joint.
//
// Usage (depuis la racine du dépôt) :
//   node tools/peinture2_voitures.mjs --lister <id> [x0 y0 z0 x1 y1 z1]   îlots peints de la voiture (dans la boîte) : repérage
//   node tools/peinture2_voitures.mjs [--verifier] [id …]                 écrit (ou contrôle seulement) les .pt2 de peinture2.json
//
// Îlot = ensemble de triangles d'un maillage peint reliés par des sommets communs (indices) : dans ces modèles, chaque panneau
// de carrosserie (pavillon, porte, capot, coque de rétroviseur) est un îlot, séparé de ses voisins par un vrai joint ou une
// arête vive (sommets dédoublés du modèle d'origine). Choisir des îlots entiers = suivre les vrais joints (RG-02).
//
// peinture2.json : { "<id>": { "note": "…", "zones": [ zone, … ] } } ; zone :
//   { "partie": "toit" | "retro", "maillage": k, "ilots": [x0,y0,z0,x1,y1,z1], "sauf": [[…], …], "triangles": n, "note": "…" }
//     → îlots du maillage k dont TOUTE la boîte est dans « ilots » (et aucune des boîtes « sauf ») ;
//   « triangles » : nombre attendu (contrôle : l'outil s'arrête si le modèle a changé).
//   { …, "triangles_boite": [x0,y0,z0,x1,y1,z1], "nyMin": n } → triangles dont le centre est dans la boîte et dont la face regarde
//     le ciel (|ny| ≥ nyMin) : pour les maillages d'origine faits de triangles séparés (pas d'îlots), comme les toits noirs ;
//   { …, "entier": true } → tout le maillage k. Un maillage peut être une pièce d'origine NON PEINTE (pavillon noir ou carbone pris dans
//     les garnitures : Corsa, M4…) : le moteur la garde telle quelle sans couleur 2 et peint ses îlots choisis en couleur 2 (vernis et
//     finition de la carrosserie) quand le joueur en pose une ; matériau opaque seulement.
//   Voiture sans pavillon distinct (cabriolet, toit ouvrant) : seulement des zones « retro » (RG-03).
//
// Format .pt2 (petit-boutiste) : 'WPT2' u32 version=1 u32 nMaillages, puis par maillage : u32 rang du maillage dans le .wfm,
//   u32 nTriangles, u32 empreinte (FNV-1a 32 des indices puis des positions du maillage), u32 triangles du toit, u32 triangles des
//   rétros, ceil(nTriangles / 8) octets : bit t = triangle t en couleur 2 (bit de poids faible d'abord).
//   Empreinte différente au chargement (modèle reconverti ou retouché après) : fichier ignoré, voiture d'une seule couleur.
import fs from 'fs';
import path from 'path';

const ICI = path.dirname(new URL(import.meta.url).pathname);
const RACINE = path.resolve(ICI, '..');
const MAT_PAINT = 1;

export function lireWfm(chemin) {
  const b = fs.readFileSync(chemin);
  if (b.toString('latin1', 0, 4) !== 'WFM1') throw new Error(`${chemin} : format inconnu`);
  let o = 4;
  const u32 = () => { const v = b.readUInt32LE(o); o += 4; return v; };
  u32();   // version
  const nT = u32(), nM = u32(), nMe = u32();
  o += 24;
  for (let i = 0; i < nT; i++) o += 4 + b.readUInt32LE(o);
  const drapeaux = [];
  for (let i = 0; i < nM; i++) { drapeaux.push(b.readUInt32LE(o + 52)); o += 56; }
  const maillages = [];
  for (let i = 0; i < nMe; i++) {
    const mat = u32(), nv = u32(), ni = u32();
    const brutV = b.subarray(o, o + nv * 32); o += nv * 32;
    const brutI = b.subarray(o, o + ni * 4); o += ni * 4;
    const v = new Float32Array(brutV.buffer.slice(brutV.byteOffset, brutV.byteOffset + brutV.length));
    const ix = new Uint32Array(brutI.buffer.slice(brutI.byteOffset, brutI.byteOffset + brutI.length));
    maillages.push({ mat, nv, v, ix, brutV, brutI, peint: mat < nM && (drapeaux[mat] & MAT_PAINT) !== 0 });
  }
  return { maillages };
}

// FNV-1a 32 bits des indices puis des positions (12 premiers octets de chaque sommet de 32 octets) : même calcul en C++
export function empreinte(m) {
  let h = 0x811c9dc5;
  const pas = o => { h ^= o; h = Math.imul(h, 0x01000193) >>> 0; };
  for (let k = 0; k < m.brutI.length; k++) pas(m.brutI[k]);
  for (let s = 0; s < m.nv; s++) for (let k = 0; k < 12; k++) pas(m.brutV[s * 32 + k]);
  return h >>> 0;
}

// îlots d'un maillage (union-find sur les indices) : pour chaque triangle, son îlot ; pour chaque îlot, boîte et triangles
export function ilots(m) {
  const nt = m.ix.length / 3, par = new Int32Array(m.nv);
  for (let i = 0; i < m.nv; i++) par[i] = i;
  const racine = x => { while (par[x] !== x) { par[x] = par[par[x]]; x = par[x]; } return x; };
  const unir = (a, b) => { a = racine(a); b = racine(b); if (a !== b) par[b] = a; };
  for (let t = 0; t < nt; t++) { unir(m.ix[3 * t], m.ix[3 * t + 1]); unir(m.ix[3 * t], m.ix[3 * t + 2]); }
  const rang = new Map(), liste = [], deTriangle = new Int32Array(nt);
  for (let t = 0; t < nt; t++) {
    const r = racine(m.ix[3 * t]);
    let k = rang.get(r);
    if (k === undefined) { k = liste.length; rang.set(r, k); liste.push({ n: 0, min: [1e9, 1e9, 1e9], max: [-1e9, -1e9, -1e9], ny: 0 }); }
    deTriangle[t] = k;
    const I = liste[k]; I.n++;
    for (let j = 0; j < 3; j++) {
      const s = m.ix[3 * t + j];
      for (let a = 0; a < 3; a++) { I.min[a] = Math.min(I.min[a], m.v[s * 8 + a]); I.max[a] = Math.max(I.max[a], m.v[s * 8 + a]); }
      I.ny += m.v[s * 8 + 4] / 3;
    }
  }
  for (const I of liste) I.ny /= I.n;
  return { liste, deTriangle };
}

const dedans = (I, b) => I.min[0] >= b[0] && I.min[1] >= b[1] && I.min[2] >= b[2] && I.max[0] <= b[3] && I.max[1] <= b[4] && I.max[2] <= b[5];
const f2 = x => x.toFixed(3);

function lister(id, boite) {
  const w = lireWfm(path.join(RACINE, 'assets/cars', `${id}.wfm`));
  w.maillages.forEach((m, k) => {
    if (!m.peint && !process.env.TOUS) return;   // TOUS=1 : aussi les pièces non peintes (toits noirs)
    const { liste } = ilots(m);
    const choisis = liste.map((I, i) => ({ ...I, i })).filter(I => !boite || (I.max[0] >= boite[0] && I.max[1] >= boite[1] && I.max[2] >= boite[2] &&
                                                                               I.min[0] <= boite[3] && I.min[1] <= boite[4] && I.min[2] <= boite[5]));
    console.log(`maillage ${k} (matériau ${m.mat}) : ${m.ix.length / 3} triangles, ${liste.length} îlots, ${choisis.length} dans la boîte`);
    for (const I of choisis.sort((a, b) => b.n - a.n))
      console.log(`  îlot ${I.i} : ${I.n} tri  x ${f2(I.min[0])}..${f2(I.max[0])}  y ${f2(I.min[1])}..${f2(I.max[1])}  z ${f2(I.min[2])}..${f2(I.max[2])}  ny ${I.ny.toFixed(2)}`);
  });
}

// sélection d'une voiture : Map maillage → { bits, toit, retro }
export function selection(id, regles, w) {
  const res = new Map();
  for (const z of regles.zones) {
    const m = w.maillages[z.maillage];
    if (!m) throw new Error(`${id} : maillage ${z.maillage} absent`);
    const nt = m.ix.length / 3;
    let e = res.get(z.maillage);
    if (!e) { e = { bits: new Uint8Array(Math.ceil(nt / 8)), toit: 0, retro: 0, nt, empreinte: empreinte(m) }; res.set(z.maillage, e); }
    const { liste, deTriangle } = ilots(m);
    const garde = liste.map(I => z.entier || (z.ilots && dedans(I, z.ilots) && !(z.sauf || []).some(s => dedans(I, s))));
    // mode « triangles » : centre du triangle dans la boîte, normale de la face vers le haut d'au moins nyMin (maillages faits de
    // triangles séparés, sans îlots : toits noirs d'origine)
    const parTriangle = t => {
      if (!z.triangles_boite) return false;
      const P = [0, 1, 2].map(j => { const q = m.ix[3 * t + j]; return [m.v[q * 8], m.v[q * 8 + 1], m.v[q * 8 + 2]]; });
      const c = [0, 1, 2].map(a => (P[0][a] + P[1][a] + P[2][a]) / 3), b = z.triangles_boite;
      if (c[0] < b[0] || c[1] < b[1] || c[2] < b[2] || c[0] > b[3] || c[1] > b[4] || c[2] > b[5]) return false;
      const u = P[1].map((x, i) => x - P[0][i]), v = P[2].map((x, i) => x - P[0][i]);
      const n = [u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]], l = Math.hypot(...n);
      return l > 0 && Math.abs(n[1]) / l >= (z.nyMin ?? -1);   // |ny| : faces doubles (deux sens) prises ensemble
    };
    let n = 0;
    for (let t = 0; t < nt; t++) {
      if (!(garde[deTriangle[t]] || parTriangle(t)) || (e.bits[t >> 3] & (1 << (t & 7)))) continue;
      e.bits[t >> 3] |= 1 << (t & 7); n++;
    }
    if (z.partie === 'toit') e.toit += n; else if (z.partie === 'retro') e.retro += n; else throw new Error(`${id} : partie inconnue « ${z.partie} »`);
    if (z.triangles !== undefined && z.triangles !== n) throw new Error(`${id} : zone « ${z.note || z.partie} » : ${n} triangles au lieu de ${z.triangles} (modèle changé ?)`);
    z.trouves = n;
  }
  return res;
}

export function ecrirePt2(res) {
  const parts = [Buffer.from('WPT2')];
  const u32 = x => { const b = Buffer.alloc(4); b.writeUInt32LE(x >>> 0); parts.push(b); };
  u32(1); u32(res.size);
  for (const [k, e] of [...res.entries()].sort((a, b) => a[0] - b[0])) { u32(k); u32(e.nt); u32(e.empreinte); u32(e.toit); u32(e.retro); parts.push(Buffer.from(e.bits)); }
  return Buffer.concat(parts);
}

function principal() {
  const args = process.argv.slice(2);
  if (args[0] === '--lister') { lister(args[1], args.length >= 8 ? args.slice(2, 8).map(Number) : null); return; }
  const verifier = args[0] === '--verifier';
  const ids = args.filter(a => !a.startsWith('--'));
  const regles = JSON.parse(fs.readFileSync(path.join(ICI, 'peinture2.json'), 'utf8'));
  let ecarts = 0;
  for (const [id, r] of Object.entries(regles)) {
    if (id.startsWith('_') || (ids.length && !ids.includes(id))) continue;
    const w = lireWfm(path.join(RACINE, 'assets/cars', `${id}.wfm`));
    const res = selection(id, r, w);
    const sortie = path.join(RACINE, 'assets/cars', `${id}.pt2`), b = ecrirePt2(res);
    let toit = 0, retro = 0;
    for (const e of res.values()) { toit += e.toit; retro += e.retro; }
    const pareil = fs.existsSync(sortie) && fs.readFileSync(sortie).equals(b);
    if (verifier) { if (!pareil) { console.log(`${id} : .pt2 absent ou différent`); ecarts++; } else console.log(`${id} : à jour (toit ${toit}, rétros ${retro} triangles)`); continue; }
    fs.writeFileSync(sortie, b);
    console.log(`${id} : toit ${toit} triangles, rétros ${retro} triangles → ${path.relative(RACINE, sortie)} (${b.length} octets)${pareil ? ', inchangé' : ''}`);
  }
  if (ecarts) process.exit(1);
}

if (import.meta.url === `file://${process.argv[1]}`) principal();
