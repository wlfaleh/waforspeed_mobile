// WaForSpeed — versions allégées d'une voiture .wfm (WFS-134, RG-11 / RG-12 ; WFS-158, RG-04 : 4 niveaux de détail) :
// fichier <id>.lod à côté du .wfm, qui ne contient que de nouveaux indices par maillage (mêmes sommets, matériaux et
// textures que le modèle complet : même peinture, mêmes feux, même silhouette, rien en double dans l'APK ni sur le GPU).
//   niveau 0 : le modèle complet (.wfm) ;
//   niveau 1 : 30 % des triangles visés pour les maillages de 300 triangles et plus (RG-12 de WFS-134 : ≥ 25 %),
//              meshoptimizer bords verrouillés (aucun trou entre deux pièces de carrosserie) ;
//   niveaux 2 et 3 : depuis le niveau précédent, 40 % puis 35 % visés, bords libres.
// meshoptimizer prend les normales en attributs (simplifyWithAttributes) : une arête qui ferait tourner les normales de la
// carrosserie compte comme un écart de plusieurs centimètres, la peinture garde ses reflets (sans cela : plaques grises ou
// argentées sur la Model 3, le X5, la SF90, l'Abarth). L'erreur permise suit la taille de la voiture à l'écran (seuils de
// src/render/vehicule_lod.h : 60, 36 et 22 pixels pour sa longueur, à l'entrée des niveaux 1, 2 et 3) : un demi-pixel, en
// mètres. Pièces que meshoptimizer ne sait pas réduire (« soupes » de triangles, faces doublées des deux côtés, pièces à
// facettes : sommets verrouillés partout) : regroupement des sommets sur une grille (« vertex clustering »), cellules d'un
// demi-pixel au plus (plus fines au niveau 1 pour garder la part visée), séparées selon le sens de la normale.
// Les maillages des feux (MAT_TAIL, MAT_HEAD), des vitres et des pièces transparentes gardent le niveau 1 à toute distance.
// usage : node alleger_voiture.mjs entree.wfm sortie.lod [part des triangles gardés au niveau 1, défaut 0.3]
// Format .lod (petit-boutiste) :
//   'WFL2' u32 nMaillages u32 nNiveaux, puis pour chaque niveau (1, 2, 3) et chaque maillage : u32 nIndices,
//   u32 octets par indice (2 si le maillage a moins de 65 536 sommets, sinon 4), indices, zéros jusqu'au multiple de 4.
//   (l'ancien 'WFL1' u32 nMaillages, puis u32 nIndices + indices u32 par maillage, reste lu : un seul niveau)
import { MeshoptSimplifier } from 'meshoptimizer';
import fs from 'fs';

const [,, inp, out, partArg] = process.argv;
if (!inp || !out) { console.error('usage : node alleger_voiture.mjs entree.wfm sortie.lod [part niveau 1]'); process.exit(1); }
const PART = Math.min(1, Math.max(0.05, parseFloat(partArg || '0.3')));
const MAT_TAIL = 4, MAT_HEAD = 8, MAT_GLASS = 64, MAT_TRANSPARENT = 2;
// niveaux 2 et 3 : part visée du niveau précédent (meshoptimizer), maillages simplifiés à partir de N triangles, seuil en
// pixels de la longueur de la voiture à l'écran à l'entrée du niveau (les mêmes que vehicule_lod.h)
const NIVEAUX = [{ part: 0.40, min: 60, seuilPx: 36 }, { part: 0.35, min: 30, seuilPx: 22 }];

// Regroupement des sommets sur une grille de côté h (m) : chaque sommet va au représentant de sa cellule (le sommet le plus
// proche du centre de gravité de la cellule) ; triangles dégénérés et doublons retirés. Parmi les sommets à la position du
// représentant (copies d'un même point : normales ou UV différentes), chaque coin prend celui dont la normale suit le mieux
// la face du nouveau triangle (pièces à facettes : la bonne facette). Résultat : indices de sommets existants.
function regrouper(v, idx, h) {
  const cellules = new Map(), cleDe = new Map();
  // cellule = case de la grille + sens dominant de la normale (6 sens) : deux couches proches et opposées (coque extérieure et
  // intérieure d'un panneau, dessus et dessous d'un becquet) ne se mélangent pas
  const sens = i => {
    const nx = v[i * 8 + 3], ny = v[i * 8 + 4], nz = v[i * 8 + 5], ax = Math.abs(nx), ay = Math.abs(ny), az = Math.abs(nz);
    return ax >= ay && ax >= az ? (nx >= 0 ? 0 : 1) : ay >= az ? (ny >= 0 ? 2 : 3) : (nz >= 0 ? 4 : 5);
  };
  const cle = i => `${Math.floor(v[i * 8] / h)},${Math.floor(v[i * 8 + 1] / h)},${Math.floor(v[i * 8 + 2] / h)},${sens(i)}`;
  for (const i of idx) {
    if (cleDe.has(i)) continue;
    const k = cle(i); cleDe.set(i, k);
    let c = cellules.get(k);
    if (!c) { c = { s: [0, 0, 0], n: 0, sommets: [] }; cellules.set(k, c); }
    c.s[0] += v[i * 8]; c.s[1] += v[i * 8 + 1]; c.s[2] += v[i * 8 + 2]; c.n++; c.sommets.push(i);
  }
  for (const c of cellules.values()) {
    const g = c.s.map(x => x / c.n);
    let meilleur = Infinity;
    for (const i of c.sommets) {
      const d = (v[i * 8] - g[0]) ** 2 + (v[i * 8 + 1] - g[1]) ** 2 + (v[i * 8 + 2] - g[2]) ** 2;
      if (d < meilleur) { meilleur = d; c.pos = [v[i * 8], v[i * 8 + 1], v[i * 8 + 2]]; }
    }
    c.copies = c.sommets.filter(i => v[i * 8] === c.pos[0] && v[i * 8 + 1] === c.pos[1] && v[i * 8 + 2] === c.pos[2]);
  }
  const vus = new Set(), sortie = [];
  for (let t = 0; t + 2 < idx.length; t += 3) {
    const k = [cleDe.get(idx[t]), cleDe.get(idx[t + 1]), cleDe.get(idx[t + 2])];
    if (k[0] === k[1] || k[1] === k[2] || k[0] === k[2]) continue;
    let C = k.map(x => cellules.get(x));
    const a = C[1].pos.map((x, j) => x - C[0].pos[j]), b = C[2].pos.map((x, j) => x - C[0].pos[j]);
    let n = [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
    if (Math.hypot(n[0], n[1], n[2]) < 1e-12) continue;
    // triangle retourné par le regroupement (sa face va contre les normales de ses sommets d'origine) : remis dans le bon
    // sens, sinon il disparaît (pièce à une face) ou montre l'envers (trou sombre, reflet du ciel)
    let ref = 0;
    for (let j = 0; j < 3; j++) { const o = idx[t + j] * 8; ref += v[o + 3] * n[0] + v[o + 4] * n[1] + v[o + 5] * n[2]; }
    if (ref < 0) { [k[1], k[2]] = [k[2], k[1]]; C = k.map(x => cellules.get(x)); n = n.map(x => -x); }
    const r = k[0] < k[1] && k[0] < k[2] ? 0 : k[1] < k[2] ? 1 : 2;   // même triangle, même sens : une seule fois
    const sig = `${k[r]}|${k[(r + 1) % 3]}|${k[(r + 2) % 3]}`;
    if (vus.has(sig)) continue;
    vus.add(sig);
    for (let j = 0; j < 3; j++) {
      let choix = C[j].copies[0], score = -Infinity;
      for (const c of C[j].copies) {
        const s2 = v[c * 8 + 3] * n[0] + v[c * 8 + 4] * n[1] + v[c * 8 + 5] * n[2];
        if (s2 > score) { score = s2; choix = c; }
      }
      sortie.push(choix);
    }
  }
  return new Uint32Array(sortie);
}
await MeshoptSimplifier.ready;
MeshoptSimplifier.useExperimentalFeatures = true;   // simplifyWithAttributes (normales)
const buf = fs.readFileSync(inp);
let p = 0;
const u32 = () => { const v = buf.readUInt32LE(p); p += 4; return v; };
if (buf.toString('latin1', 0, 4) !== 'WFM1') throw new Error('format inconnu');
p = 4; u32();
const nTex = u32(), nMat = u32(), nMesh = u32();
const bb = [0, 1, 2, 3, 4, 5].map(k => buf.readFloatLE(20 + 4 * k));
const longueur = Math.max(bb[3] - bb[0], bb[4] - bb[1], bb[5] - bb[2]);
p += 24;
for (let i = 0; i < nTex; i++) { const n = u32(); p += n; }
const drapeaux = [];
for (let i = 0; i < nMat; i++) { drapeaux.push(buf.readUInt32LE(p + 52)); p += 56; }   // matériaux : 10 f32 + 3 i32 + 1 u32
const maillages = [];
for (let i = 0; i < nMesh; i++) {
  const mat = u32(), nv = u32(), ni = u32();
  const v = new Float32Array(buf.buffer.slice(buf.byteOffset + p, buf.byteOffset + p + nv * 32)); p += nv * 32;
  const idx = new Uint32Array(buf.buffer.slice(buf.byteOffset + p, buf.byteOffset + p + ni * 4)); p += ni * 4;
  maillages.push({ mat, nv, v, idx });
}
// meshoptimizer, normales en attributs (poids POIDS_NORMALE : une normale qui tourne d'un radian compte comme un écart de
// 40 cm environ, réglé sur les vitrines des 20 voitures) ; erreur absolue en mètres ; bords verrouillés ou libres
const POIDS_NORMALE = 0.4;
function simplifierNormales(v, idx, part, erreur, verrou = false) {
  const nv = v.length / 8, N = new Float32Array(nv * 3);
  for (let k = 0; k < nv; k++) { N[k * 3] = v[k * 8 + 3]; N[k * 3 + 1] = v[k * 8 + 4]; N[k * 3 + 2] = v[k * 8 + 5]; }
  const cible = Math.max(3, Math.floor(idx.length * part / 3) * 3);
  const [res] = MeshoptSimplifier.simplifyWithAttributes(idx, v, 8, N, 3, [POIDS_NORMALE, POIDS_NORMALE, POIDS_NORMALE], null, cible, erreur, verrou ? ['ErrorAbsolute', 'LockBorder'] : ['ErrorAbsolute']);
  return res.length >= 3 ? res : idx;
}

// niveau 1 : meshoptimizer bords verrouillés ; un maillage qu'il ne réduit pas sous la moitié (coques qui se touchent :
// bords partout) essaie les bords libres, puis, si meshoptimizer n'y arrive toujours pas (moins de 10 % retirés), la grille
// (la plus grande cellule, d'un demi-pixel au plus, qui garde la part visée) ; le plus léger qui garde 25 % au moins (RG-12)
const erreur1 = 0.5 * longueur / 60;   // demi-pixel au 1er seuil (60 px)
const niveau1 = maillages.map(({ mat, v, idx }, i) => {
  if (idx.length / 3 < 300) return idx;
  const cible = Math.max(3, Math.floor(idx.length * PART / 3) * 3);
  const verrou = simplifierNormales(v, idx, PART, erreur1, true);
  let meilleur = verrou, methode = 'bords verrouillés';
  if (meilleur.length > 0.5 * idx.length) {
    const libre = simplifierNormales(v, idx, PART, erreur1);
    let grille = idx;
    if (libre.length > 0.9 * idx.length) {
      grille = regrouper(v, idx, erreur1);
      if (grille.length < cible) {
        let bas = erreur1 / 32, haut = erreur1;
        for (let k = 0; k < 8; k++) {
          const h = Math.sqrt(bas * haut), g = regrouper(v, idx, h);
          if (g.length >= cible) { grille = g; bas = h; } else haut = h;
        }
      }
    }
    for (const [nom, c] of [['bords libres', libre], ['grille', grille]]) if (c.length >= 0.25 * idx.length && c.length < meilleur.length) { meilleur = c; methode = nom; }
  }
  if (process.env.WFS_DETAIL) console.log(`  niveau 1, maillage ${i} (matériau ${mat}, drapeaux ${drapeaux[mat]}) : ${idx.length / 3} → ${meilleur.length / 3} (${methode})`);
  return meilleur;
});
const niveaux = [niveau1];
for (const { part, min, seuilPx } of NIVEAUX) {
  // demi-pixel de la longueur de la voiture au seuil d'entrée du niveau, en mètres
  const erreur = 0.5 * longueur / seuilPx;
  const prec = niveaux[niveaux.length - 1];
  niveaux.push(maillages.map(({ mat, v }, i) => {
    const idx = prec[i];
    const f = mat < nMat ? drapeaux[mat] : 0;
    if (idx.length / 3 < min || (f & (MAT_TAIL | MAT_HEAD | MAT_GLASS | MAT_TRANSPARENT))) return idx;
    const res = simplifierNormales(v, idx, part, erreur);
    if (res.length <= 0.6 * idx.length) return res;   // meshoptimizer y arrive : sa forme est la plus propre (sinon la grille)
    const grille = regrouper(v, idx, erreur);
    const meilleur = grille.length >= 3 && grille.length < res.length ? grille : res;
    return meilleur.length >= 3 && meilleur.length < idx.length ? meilleur : idx;
  }));
}
const tete = Buffer.alloc(12); tete.write('WFL2', 0, 'latin1'); tete.writeUInt32LE(nMesh, 4); tete.writeUInt32LE(niveaux.length, 8);
const morceaux = [tete];
const complet = maillages.reduce((s, m) => s + m.idx.length / 3, 0);
const bilan = [];
for (const niv of niveaux) {
  let tri = 0;
  niv.forEach((idx, i) => {
    const court = maillages[i].nv < 65536, octets = court ? 2 : 4;
    const h = Buffer.alloc(8); h.writeUInt32LE(idx.length, 0); h.writeUInt32LE(octets, 4);
    const corps = Buffer.from((court ? new Uint16Array(idx) : new Uint32Array(idx)).buffer);
    morceaux.push(h, corps, Buffer.alloc((4 - corps.length % 4) % 4));
    tri += idx.length / 3;
  });
  bilan.push(`${tri} (${(100 * tri / complet).toFixed(1)} %)`);
}
fs.writeFileSync(out, Buffer.concat(morceaux));
console.log(`${out.split('/').pop()} : ${complet} triangles → niveaux 1 à 3 : ${bilan.join(', ')}`);
