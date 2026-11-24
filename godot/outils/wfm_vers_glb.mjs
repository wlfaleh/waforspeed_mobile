// WaForSpeed — prototype Godot (WFS-610) : convertit une voiture du jeu actuel (.wfm, format de tools/convert_car.mjs) en
// glTF binaire (.glb) lisible par Godot, sans toucher au fichier d'origine.
//   node godot/outils/wfm_vers_glb.mjs assets/cars/golf.wfm godot/projet/voitures/golf.glb
//
// Format .wfm (petit-boutiste) : 'WFM1' u32 version u32 nTex u32 nMat u32 nMesh f32 bbox[6] ; textures (u32 octets, jpg/png) ;
// matériaux (f32 couleur[4] metal rugosite emissif[3] coupureAlpha, i32 texCouleur texNormale texMetalRugo, u32 drapeaux) ;
// maillages (u32 materiau nSommets nIndices, sommets pos3 normale3 uv2, indices u32) ; puis 'TAN1' (tangentes, ignorées ici :
// Godot les recalcule). Drapeaux : 1 peinture, 2 transparent, 4 feu arrière, 8 phare, 16 double face, 32 alpha découpé,
// 64 vitre, 128 roue. Repère : mètres, Y en haut, avant vers −Z (comme Godot).
// Sortie : un nœud « carrosserie » (tout sauf les roues) et quatre nœuds « roue_av_g », « roue_av_d », « roue_ar_g »,
// « roue_ar_d » dont l'origine est le centre de la roue (pour la faire tourner et braquer dans Godot). Les matériaux de
// peinture s'appellent « peinture » (la couleur se change dans Godot).
import fs from 'node:fs';

const [, , entree, sortie] = process.argv;
if (!entree || !sortie) { console.error('usage : node wfm_vers_glb.mjs entree.wfm sortie.glb'); process.exit(2); }
const b = fs.readFileSync(entree);
let o = 0;
const u32 = () => { const v = b.readUInt32LE(o); o += 4; return v; };
const i32 = () => { const v = b.readInt32LE(o); o += 4; return v; };
const f32 = () => { const v = b.readFloatLE(o); o += 4; return v; };
if (b.toString('latin1', 0, 4) !== 'WFM1') throw new Error('pas un fichier WFM1');
o = 4;
const version = u32(), nTex = u32(), nMat = u32(), nMesh = u32();
for (let i = 0; i < 6; i++) f32();
const textures = [];
for (let i = 0; i < nTex; i++) { const n = u32(); textures.push(b.subarray(o, o + n)); o += n; }
const mats = [];
for (let i = 0; i < nMat; i++) {
    const couleur = [f32(), f32(), f32(), f32()], metal = f32(), rugo = f32(), emissif = [f32(), f32(), f32()], coupure = f32();
    const texC = i32(), texN = i32(), texMR = i32(), drapeaux = u32();
    mats.push({ couleur, metal, rugo, emissif, coupure, texC, texN, texMR, drapeaux });
}
const meshes = [];
for (let i = 0; i < nMesh; i++) {
    const mat = u32(), nv = u32(), ni = u32();
    const v = new Float32Array(b.buffer.slice(b.byteOffset + o, b.byteOffset + o + nv * 32)); o += nv * 32;
    const idx = new Uint32Array(b.buffer.slice(b.byteOffset + o, b.byteOffset + o + ni * 4)); o += ni * 4;
    meshes.push({ mat, v, idx });
}

// ---- découpe des roues (drapeau 128) en quatre, par quart (x gauche/droite, z avant/arrière)
const groupes = { carrosserie: [] };   // nom → [{ mat, positions[], normales[], uvs[], indices[] }]
const ROUES = ['roue_av_g', 'roue_av_d', 'roue_ar_g', 'roue_ar_d'];
for (const r of ROUES) groupes[r] = [];
function ajouterTri(nom, mat, m, tri) {
    let p = groupes[nom].find(x => x.mat === mat);
    if (!p) { p = { mat, pos: [], nor: [], uv: [], idx: [], carte: new Map() }; groupes[nom].push(p); }
    for (const s of tri) {
        let k = p.carte.get(s);
        if (k === undefined) {
            k = p.pos.length / 3; p.carte.set(s, k);
            const a = s * 8;
            p.pos.push(m.v[a], m.v[a + 1], m.v[a + 2]); p.nor.push(m.v[a + 3], m.v[a + 4], m.v[a + 5]); p.uv.push(m.v[a + 6], m.v[a + 7]);
        }
        p.idx.push(k);
    }
}
for (const m of meshes) {
    const roue = (mats[m.mat].drapeaux & 128) !== 0;
    for (let t = 0; t < m.idx.length; t += 3) {
        const tri = [m.idx[t], m.idx[t + 1], m.idx[t + 2]];
        if (!roue) { ajouterTri('carrosserie', m.mat, m, tri); continue; }
        let cx = 0, cz = 0;
        for (const s of tri) { cx += m.v[s * 8]; cz += m.v[s * 8 + 2]; }
        const nom = (cz < 0 ? 'roue_av_' : 'roue_ar_') + (cx < 0 ? 'g' : 'd');
        ajouterTri(nom, m.mat, m, tri);
    }
}
// centre de chaque roue : milieu de sa boîte ; ses sommets sont exprimés par rapport à ce centre
const centres = {};
for (const r of ROUES) {
    const mn = [Infinity, Infinity, Infinity], mx = [-Infinity, -Infinity, -Infinity];
    for (const p of groupes[r]) for (let i = 0; i < p.pos.length; i += 3) for (let k = 0; k < 3; k++) { mn[k] = Math.min(mn[k], p.pos[i + k]); mx[k] = Math.max(mx[k], p.pos[i + k]); }
    if (!isFinite(mn[0])) continue;
    const c = [0, 1, 2].map(k => (mn[k] + mx[k]) / 2);
    centres[r] = c;
    for (const p of groupes[r]) for (let i = 0; i < p.pos.length; i += 3) for (let k = 0; k < 3; k++) p.pos[i + k] -= c[k];
}

// ---- écriture du glTF binaire
const morceaux = []; let longueur = 0;
const gltf = { asset: { version: '2.0', generator: 'WaForSpeed wfm_vers_glb (WFS-610)' }, scene: 0, scenes: [{ nodes: [] }], nodes: [], meshes: [],
               materials: [], textures: [], images: [], samplers: [{ magFilter: 9729, minFilter: 9987, wrapS: 10497, wrapT: 10497 }],
               accessors: [], bufferViews: [], buffers: [] };
function vue(buf, cible) {
    while (longueur % 4) { morceaux.push(Buffer.alloc(1)); longueur++; }
    const bv = { buffer: 0, byteOffset: longueur, byteLength: buf.length };
    if (cible) bv.target = cible;
    morceaux.push(buf); longueur += buf.length;
    gltf.bufferViews.push(bv); return gltf.bufferViews.length - 1;
}
function accesseur(tab, type, composant, cible, avecBornes) {
    const buf = Buffer.from(tab.buffer, tab.byteOffset, tab.byteLength);
    const a = { bufferView: vue(buf, cible), componentType: composant, count: tab.length / { SCALAR: 1, VEC2: 2, VEC3: 3 }[type], type };
    if (avecBornes) {
        const mn = [Infinity, Infinity, Infinity], mx = [-Infinity, -Infinity, -Infinity];
        for (let i = 0; i < tab.length; i += 3) for (let k = 0; k < 3; k++) { mn[k] = Math.min(mn[k], tab[i + k]); mx[k] = Math.max(mx[k], tab[i + k]); }
        a.min = mn; a.max = mx;
    }
    gltf.accessors.push(a); return gltf.accessors.length - 1;
}
for (const t of textures) {
    const png = t[0] === 0x89 && t[1] === 0x50;
    gltf.images.push({ bufferView: vue(Buffer.from(t)), mimeType: png ? 'image/png' : 'image/jpeg' });
    gltf.textures.push({ source: gltf.images.length - 1, sampler: 0 });
}
for (const m of mats) {
    const d = m.drapeaux;
    const nom = d & 1 ? 'peinture' : d & 64 ? 'vitre' : d & 4 ? 'feu_arriere' : d & 8 ? 'phare' : d & 128 ? 'roue' : 'matiere';
    const mat = { name: nom, pbrMetallicRoughness: { baseColorFactor: m.couleur, metallicFactor: m.metal, roughnessFactor: m.rugo } };
    if (m.texC >= 0) mat.pbrMetallicRoughness.baseColorTexture = { index: m.texC };
    if (m.texMR >= 0) mat.pbrMetallicRoughness.metallicRoughnessTexture = { index: m.texMR };
    if (m.texN >= 0) mat.normalTexture = { index: m.texN };
    if (m.emissif.some(x => x > 0)) mat.emissiveFactor = m.emissif.map(x => Math.min(1, x));
    if (d & 2 || d & 64) mat.alphaMode = 'BLEND';
    else if (d & 32) { mat.alphaMode = 'MASK'; mat.alphaCutoff = m.coupure > 0 ? m.coupure : 0.5; }
    if (d & 16 || d & 64) mat.doubleSided = true;
    gltf.materials.push(mat);
}
for (const nom of ['carrosserie', ...ROUES]) {
    const parts = groupes[nom];
    if (!parts.length) continue;
    const prims = parts.map(p => ({
        attributes: {
            POSITION: accesseur(new Float32Array(p.pos), 'VEC3', 5126, 34962, true),
            NORMAL: accesseur(new Float32Array(p.nor), 'VEC3', 5126, 34962),
            TEXCOORD_0: accesseur(new Float32Array(p.uv), 'VEC2', 5126, 34962),
        },
        indices: accesseur(new Uint32Array(p.idx), 'SCALAR', 5125, 34963),
        material: p.mat,
    }));
    gltf.meshes.push({ name: nom, primitives: prims });
    const noeud = { name: nom, mesh: gltf.meshes.length - 1 };
    if (centres[nom]) noeud.translation = centres[nom];
    gltf.nodes.push(noeud); gltf.scenes[0].nodes.push(gltf.nodes.length - 1);
}
while (longueur % 4) { morceaux.push(Buffer.alloc(1)); longueur++; }
gltf.buffers.push({ byteLength: longueur });
if (!gltf.textures.length) { delete gltf.textures; delete gltf.images; delete gltf.samplers; }
let json = Buffer.from(JSON.stringify(gltf));
while (json.length % 4) json = Buffer.concat([json, Buffer.from(' ')]);
const bin = Buffer.concat(morceaux);
const tete = Buffer.alloc(12); tete.write('glTF', 0, 'latin1'); tete.writeUInt32LE(2, 4); tete.writeUInt32LE(12 + 8 + json.length + 8 + bin.length, 8);
const cj = Buffer.alloc(8); cj.writeUInt32LE(json.length, 0); cj.write('JSON', 4, 'latin1');
const cb = Buffer.alloc(8); cb.writeUInt32LE(bin.length, 0); cb.write('BIN\0', 4, 'latin1');
fs.writeFileSync(sortie, Buffer.concat([tete, cj, json, cb, bin]));
const nr = ROUES.filter(r => centres[r]).length;
console.log(`${entree} → ${sortie} : version ${version}, ${nTex} textures, ${nMat} matériaux, ${nMesh} maillages, ${nr} roues séparées`);
