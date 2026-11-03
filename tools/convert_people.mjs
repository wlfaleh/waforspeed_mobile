// Convertit un personnage du jeu web (glTF JSON autonome, models/people/*.json du dépôt wlfaleh/waaps) et la
// bibliothèque d'animations (animations.json) en un fichier binaire .wfp pour le moteur natif (WFS-63).
// usage : node convert_people.mjs <personnage.json> <animations.json> <sortie.wfp>
//
// Rien n'est créé : maillage skinné, matériau (atlas couleur + normales), squelette, coiffures, niveaux de détail et
// clips sont ceux du jeu web (Quaternius, CC0 — voir models/people/license.txt, copié dans assets/licences/).
// Les textures WebP sont réencodées en JPEG (couleur) et PNG (normales), 1024 px max (comme le jeu web sur mobile).
//
// Format .wfp (petit-boutiste) :
//   'WFP1' u32 version=1
//   u32 nTex, textures : u32 octets, octets (jpg/png)                    × nTex (0 = couleur, 1 = normales)
//   f32 rugosite f32 metal f32 hauteur(m)
//   u32 nSommets u32 nIndices u32 nPlages u32 nOs u32 nClips
//   sommets  : pos3 normale3 uv2 (f32), palette u8[4], poids f32[4]  = 52 octets  × nSommets
//   indices  : u32                                                            × nIndices
//   plages   : char nom[32] u32 premier u32 nombre (« corps-<lod> », « <coiffure>-<lod> »)   × nPlages
//   os       : char nom[32] i32 parent (-1 = racine) f32 t[3] r[4] s[3] (pose de repos locale) f32 bindInverse[16]
//              (ordre du skin glTF ; un parent est toujours avant ses enfants)                                 × nOs
//   palette  : u32 nPalette, u32 os[nPalette] : os qui portent des poids ; les sommets indexent cette palette
//              (≤ 64 entrées, limite du renderer — 62 sur 65 os pour les personnages du jeu web)
//   clips    : char nom[32] f32 duree u32 nCanaux,
//              canaux : u32 os u32 chemin (0 translation, 1 rotation, 2 échelle) u32 nCles f32 temps[n] f32 valeurs[n×3|4]
import sharp from 'sharp';
import fs from 'fs';

const [,, inChar, inAnim, out] = process.argv;
if (!out) { console.error('usage : node convert_people.mjs <personnage.json> <animations.json> <sortie.wfp>'); process.exit(1); }

// ---------- lecture glTF JSON (tampons en data URI) ----------
function charger(path) {
  const g = JSON.parse(fs.readFileSync(path, 'utf8'));
  g._bufs = g.buffers.map((b) => Buffer.from(b.uri.slice(b.uri.indexOf(',') + 1), 'base64'));
  return g;
}
const COMP = { 5120: [1, 'getInt8'], 5121: [1, 'getUint8'], 5122: [2, 'getInt16'], 5123: [2, 'getUint16'], 5125: [4, 'getUint32'], 5126: [4, 'getFloat32'] };
const NB = { SCALAR: 1, VEC2: 2, VEC3: 3, VEC4: 4, MAT4: 16 };
function lire(g, i) {
  const a = g.accessors[i], v = g.bufferViews[a.bufferView], buf = g._bufs[v.buffer];
  const [sz, fn] = COMP[a.componentType], n = NB[a.type], stride = v.byteStride || sz * n;
  const dv = new DataView(buf.buffer, buf.byteOffset + (v.byteOffset || 0) + (a.byteOffset || 0));
  const res = new Array(a.count * n);
  for (let e = 0; e < a.count; e++) for (let c = 0; c < n; c++) res[e * n + c] = dv[fn](e * stride + c * sz, true);
  return res;
}

const ch = charger(inChar), an = charger(inAnim);
const mesh = ch.meshes[0], prim = mesh.primitives[0], skin = ch.skins[0];
const A = prim.attributes;
const pos = lire(ch, A.POSITION), nrm = lire(ch, A.NORMAL), uv = lire(ch, A.TEXCOORD_0);
const jts = lire(ch, A.JOINTS_0), wts = lire(ch, A.WEIGHTS_0), idx = lire(ch, prim.indices);
const nV = pos.length / 3;

// ---------- squelette : ordre des os = ordre de skin.joints (indices des sommets), parents avant enfants ----------
const parentDe = new Map();
ch.nodes.forEach((n, i) => (n.children || []).forEach((c) => parentDe.set(c, i)));
const jointIdx = new Map(skin.joints.map((n, i) => [n, i]));
const ib = lire(ch, skin.inverseBindMatrices);
const os = skin.joints.map((n, i) => {
  const nd = ch.nodes[n];
  let p = parentDe.get(n);
  while (p !== undefined && !jointIdx.has(p)) p = parentDe.get(p);   // parent dans le squelette (l'Armature n'a pas de transformation)
  return { nom: nd.name, parent: p === undefined ? -1 : jointIdx.get(p), t: nd.translation || [0, 0, 0], r: nd.rotation || [0, 0, 0, 1],
           s: nd.scale || [1, 1, 1], inv: ib.slice(i * 16, i * 16 + 16) };
});
os.forEach((o, i) => { if (o.parent >= i) throw new Error(`os ${o.nom} : parent après l'enfant (ordre non pris en charge)`); });
for (const n of [ch.scenes[0].nodes[0]]) {   // l'Armature et le nœud du maillage ne doivent pas porter de transformation
  const nd = ch.nodes[n];
  if (nd.translation || nd.rotation || nd.scale || nd.matrix) throw new Error('Armature transformée : non pris en charge');
}
const utilises = new Set();
for (let v = 0; v < nV; v++) for (let k = 0; k < 4; k++) if (wts[v * 4 + k] > 0) utilises.add(jts[v * 4 + k]);
const palette = [...utilises].sort((a, b) => a - b), versPalette = new Map(palette.map((o, i) => [o, i]));
if (palette.length > 64) throw new Error(`${palette.length} os avec des poids : plus de 64 (limite du renderer)`);

// ---------- plages d'indices : corps et coiffures par niveau de détail (extras du jeu web) ----------
const ex = mesh.extras || {};
const plages = [];
(ex.lods || [{ base: [0, ex.baseCount || idx.length], hairs: ex.hairRanges || [] }]).forEach((l, li) => {
  plages.push([`corps-${li}`, l.base[0], l.base[1]]);
  l.hairs.forEach(([s, n], hi) => plages.push([`${(ex.hairRanges && ex.hairRanges[hi] && ex.hairRanges[hi][2]) || 'coiffure' + hi}-${li}`, s, n]));
});

// ---------- matériau et textures ----------
const mat = ch.materials[prim.material], pbr = mat.pbrMetallicRoughness || {};
async function texture(ti, jpeg) {
  const img = ch.images[ch.textures[ti].source];
  const data = Buffer.from(img.uri.slice(img.uri.indexOf(',') + 1), 'base64');
  let s = sharp(data).resize({ width: 1024, height: 1024, fit: 'inside', withoutEnlargement: true });
  return jpeg ? s.jpeg({ quality: 90 }).toBuffer() : s.png().toBuffer();
}
const texs = [await texture(pbr.baseColorTexture.index, true), await texture(mat.normalTexture.index, false)];

// ---------- clips (animations.json) : cibles retrouvées par le nom de l'os ----------
const parNom = new Map(os.map((o, i) => [o.nom, i]));
const CHEMIN = { translation: 0, rotation: 1, scale: 2 };
const clips = an.animations.map((a) => {
  let duree = 0;
  const canaux = [];
  for (const c of a.channels) {
    const nom = an.nodes[c.target.node].name, o = parNom.get(nom), chemin = CHEMIN[c.target.path];
    if (o === undefined || chemin === undefined) continue;
    const s = a.samplers[c.sampler];
    if ((s.interpolation || 'LINEAR') !== 'LINEAR') throw new Error(`${a.name} : interpolation ${s.interpolation} non prise en charge`);
    const temps = lire(an, s.input), val = lire(an, s.output);
    duree = Math.max(duree, temps[temps.length - 1]);
    canaux.push({ o, chemin, temps, val });
  }
  return { nom: a.name, duree, canaux };
});

// ---------- écriture ----------
const parts = [];
const u32 = (v) => { const b = Buffer.alloc(4); b.writeUInt32LE(v >>> 0); parts.push(b); };
const i32 = (v) => { const b = Buffer.alloc(4); b.writeInt32LE(v); parts.push(b); };
const f32s = (a) => { const b = Buffer.alloc(a.length * 4); a.forEach((v, i) => b.writeFloatLE(v, i * 4)); parts.push(b); };
const nom32 = (s) => { const b = Buffer.alloc(32); b.write(s.slice(0, 31), 'utf8'); parts.push(b); };
parts.push(Buffer.from('WFP1')); u32(1);
u32(texs.length); for (const t of texs) { u32(t.length); parts.push(t); }
f32s([pbr.roughnessFactor ?? 1, pbr.metallicFactor ?? 0, ex.height || 1.6]);
u32(nV); u32(idx.length); u32(plages.length); u32(os.length); u32(clips.length);
const vb = Buffer.alloc(nV * 52);
for (let v = 0; v < nV; v++) {
  const o = v * 52;
  for (let k = 0; k < 3; k++) { vb.writeFloatLE(pos[v * 3 + k], o + k * 4); vb.writeFloatLE(nrm[v * 3 + k], o + 12 + k * 4); }
  vb.writeFloatLE(uv[v * 2], o + 24); vb.writeFloatLE(uv[v * 2 + 1], o + 28);
  let somme = 0; for (let k = 0; k < 4; k++) somme += wts[v * 4 + k];
  for (let k = 0; k < 4; k++) {
    const w = somme > 0 ? wts[v * 4 + k] / somme : (k ? 0 : 1);
    vb.writeUInt8(w > 0 ? versPalette.get(jts[v * 4 + k]) : 0, o + 32 + k); vb.writeFloatLE(w, o + 36 + k * 4);
  }
}
parts.push(vb);
const ibuf = Buffer.alloc(idx.length * 4); idx.forEach((v, i) => ibuf.writeUInt32LE(v, i * 4)); parts.push(ibuf);
for (const [n, s, c] of plages) { nom32(n); u32(s); u32(c); }
for (const o of os) { nom32(o.nom); i32(o.parent); f32s([...o.t, ...o.r, ...o.s, ...o.inv]); }
u32(palette.length); for (const o of palette) u32(o);
for (const c of clips) {
  nom32(c.nom); f32s([c.duree]); u32(c.canaux.length);
  for (const k of c.canaux) { u32(k.o); u32(k.chemin); u32(k.temps.length); f32s(k.temps); f32s(k.val); }
}
fs.writeFileSync(out, Buffer.concat(parts));
console.log(`${out} : ${nV} sommets, ${idx.length} indices, ${plages.length} plages, ${os.length} os (${utilises.size} avec des poids), ` +
            `${clips.length} clips (${clips.map((c) => c.nom).join(', ')}), textures ${texs.map((t) => t.length).join(' + ')} octets`);
