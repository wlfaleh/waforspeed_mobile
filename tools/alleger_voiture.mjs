// WaForSpeed — version allégée d'une voiture .wfm (WFS-134, RG-11 / RG-12) : fichier <id>.lod à côté du .wfm, qui ne
// contient que de nouveaux indices par maillage (mêmes sommets, matériaux et textures que le modèle complet : même peinture,
// mêmes feux, même silhouette, rien en double dans l'APK ni sur le GPU) ; triangles simplifiés par meshoptimizer (bords
// verrouillés), au moins 30 % gardés (RG-12 : ≥ 25 %). Les maillages de moins de 300 triangles restent tels quels.
// usage : node alleger_voiture.mjs entree.wfm sortie.lod [part des triangles gardés, défaut 0.3]
// Format .lod (petit-boutiste) : 'WFL1' u32 nMaillages, puis par maillage : u32 nIndices, indices u32.
import { MeshoptSimplifier } from 'meshoptimizer';
import fs from 'fs';

const [,, inp, out, partArg] = process.argv;
const PART = Math.min(1, Math.max(0.05, parseFloat(partArg || '0.3')));
await MeshoptSimplifier.ready;
const buf = fs.readFileSync(inp);
let p = 0;
const u32 = () => { const v = buf.readUInt32LE(p); p += 4; return v; };
if (buf.toString('latin1', 0, 4) !== 'WFM1') throw new Error('format inconnu');
p = 4; u32();
const nTex = u32(), nMat = u32(), nMesh = u32();
p += 24;
for (let i = 0; i < nTex; i++) { const n = u32(); p += n; }
p += nMat * (4 * 13 + 4);   // matériaux : 10 f32 + 3 i32 + 1 u32
const tete = Buffer.alloc(8); tete.write('WFL1', 0, 'latin1'); tete.writeUInt32LE(nMesh, 4);
const morceaux = [tete];
let avant = 0, apres = 0;
for (let i = 0; i < nMesh; i++) {
  u32(); const nv = u32(), ni = u32();
  const v = new Float32Array(buf.buffer.slice(buf.byteOffset + p, buf.byteOffset + p + nv * 32)); p += nv * 32;
  let idx = new Uint32Array(buf.buffer.slice(buf.byteOffset + p, buf.byteOffset + p + ni * 4)); p += ni * 4;
  avant += ni / 3;
  if (ni / 3 >= 300) {
    const cible = Math.max(3, Math.floor(ni * PART / 3) * 3);
    const [res] = MeshoptSimplifier.simplify(idx, v, 8, cible, 0.02, ['LockBorder']);
    if (res.length >= 3) idx = res;
  }
  apres += idx.length / 3;
  const h = Buffer.alloc(4); h.writeUInt32LE(idx.length, 0);
  morceaux.push(h, Buffer.from(new Uint32Array(idx).buffer));
}
fs.writeFileSync(out, Buffer.concat(morceaux));
console.log(`${out.split('/').pop()} : ${avant} → ${apres} triangles (${(100 * apres / avant).toFixed(0)} %)`);
