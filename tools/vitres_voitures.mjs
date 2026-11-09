// WaForSpeed — WFS-215 (ANO-107, ANO-110, RG-04) : vitres avant claires du Range Rover Sport et de l'A 45, verre sombre (non
// laiteux) de l'A 45, plaque arrière de la Model 3 sortie de la carrosserie. Retouche directement assets/cars/<id>.wfm.
//
// Pourquoi un matériau à part pour les vitres avant : le shader des voitures (shaders/vehicule.glsl, fonction vitre) rend toute
// pièce F_GLASS (drapeau 64) avec une opacité d'au moins 0,55 + Fresnel, quel que soit le matériau : au plus ≈ 45 % de lumière
// transmise. Le pare-brise et les vitres latérales avant doivent en laisser passer au moins 75 % et 70 % (règlement ONU n° 43,
// transmission régulière de la lumière) : leurs triangles passent dans un nouveau matériau transparent ordinaire (F_BLEND 2 +
// double face 16, sans F_GLASS), opacité 0,25 = 75 % transmis, rugosité 0,05 (reflets du ciel par l'éclairage d'image) ; le reste
// du verre (lunette, vitres arrière, optiques) garde son matériau F_GLASS.
//  - Range Rover Sport : verre (matériau 12) noir et métallique : seules les vitres avant changent ; vitres arrière sombres
//    (verre surteinté d'origine, transmission non sourcée : « à confirmer »).
//  - A 45 : verre (matériau 13) gris 0,91 : voile laiteux (terme albédo × 0,08 × ciel du shader) ; albédo ramené à un gris-vert
//    sombre (0,05 / 0,06 / 0,06) pour le verre qui reste F_GLASS (vitres arrière, lunette, optiques) ; vitres avant comme ci-dessus.
//  - Model 3 : la plaque arrière (matériau 24) était une surface courbe de 148 petits triangles (hauteur médiane 1,6 mm) : prise
//    pour un fil très fin par le chargeur (WFS-42), elle était dessinée en couverture partielle, la peinture grise au travers
//    (texte gris clair sur fond gris, ANO-107) ; refaite en rectangle plat et vertical de 2 triangles, devant la peinture.
// Le nouveau matériau est ajouté à la fin : les rangs des matériaux existants ne changent pas (src/render/correctifs_voitures.h
// et tools/logos.json les désignent par rang ; logos.json porte le nouveau nombre de matériaux, 22 et 26). Lancé par
// tools/convertir_voitures.sh juste après plaques_voitures.mjs (avant effacer_logos.mjs, qui contrôle ce nombre) ; les .lod sont
// régénérés à la fin du script (alleger_voiture.mjs). Rejouable : une voiture déjà retouchée n'est plus modifiée.
// Lecture / écriture du .wfm : mêmes fonctions que tools/plaques_voitures.mjs (recopiées : ce script-là s'exécute à l'import).
// usage (depuis tools/) : node vitres_voitures.mjs [id …]   (sans id : rrsport, a45, model3) ; WFS_RACINE=<dossier> : essai sur une copie
import fs from 'fs';

const RACINE = process.env.WFS_RACINE ? new URL(`file://${process.env.WFS_RACINE.replace(/\/?$/, '/')}`) : new URL('../', import.meta.url);
// zone des vitres avant : centre du triangle au-dessus de la ceinture (y), entre le bas du pare-brise et le montant central (z,
// avant de la voiture vers z < 0), relevée sur les maillages (répartition des triangles du verre par position et orientation)
const VOITURES = {
  rrsport: { verre: 12, avant: { yMin: 0.95, zMin: -1.4, zMax: 0.4 } },
  a45: { verre: 13, avant: { yMin: 0.95, zMin: -1.25, zMax: 0.2 }, albedoVerre: [0.05, 0.06, 0.06] },
  model3: { plaqueArriere: 24 },
};
// vitres avant : gris-vert très léger (verre clair), opacité 0,25, métal 0, rugosité 0,05, sans texture, transparent double face
const VITRE_AVANT = { couleur: [0.05, 0.06, 0.06, 0.25], metal: 0, rugosite: 0.05, drapeaux: 2 | 16 };

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
// matériau (56 octets) : couleur 0-3, métal 4, rugosité 5, émissif 6-8, coupure 9 (flottants), textures 10-12 (entiers), drapeaux 13
function materiau({ couleur, metal, rugosite, drapeaux }) {
  const M = Buffer.alloc(56);
  couleur.forEach((x, i) => M.writeFloatLE(x, 4 * i));
  M.writeFloatLE(metal, 16); M.writeFloatLE(rugosite, 20);
  M.writeInt32LE(-1, 40); M.writeInt32LE(-1, 44); M.writeInt32LE(-1, 48); M.writeUInt32LE(drapeaux, 52);
  return M;
}
const centre = (m, t, a) => (m.v[m.ix[t] * 8 + a] + m.v[m.ix[t + 1] * 8 + a] + m.v[m.ix[t + 2] * 8 + a]) / 3;

// vitres avant du matériau de verre `verre` → nouveau maillage du matériau ajouté à la fin
function vitresAvant(f, cfg, journal) {
  const nouveau = materiau(VITRE_AVANT);
  const k = f.maillages.findIndex((m) => m.mat === cfg.verre);
  if (k < 0) throw new Error(`aucun maillage du matériau ${cfg.verre}`);
  const m = f.maillages[k], Z = cfg.avant;
  const avant = [], reste = [];
  for (let t = 0; t + 2 < m.ix.length; t += 3) {
    const y = centre(m, t, 1), z = centre(m, t, 2);
    (y > Z.yMin && z > Z.zMin && z < Z.zMax ? avant : reste).push(m.ix[t], m.ix[t + 1], m.ix[t + 2]);
  }
  const deja = f.mats.length > 0 && f.mats[f.mats.length - 1].equals(nouveau) && f.maillages.some((x) => x.mat === f.mats.length - 1);
  if (deja) { if (avant.length) throw new Error(`vitres avant déjà séparées, mais ${avant.length / 3} triangles encore dans le matériau ${cfg.verre}`); journal.push('vitres avant : déjà faites'); return false; }
  if (!avant.length) throw new Error(`aucun triangle de vitre avant dans le matériau ${cfg.verre}`);
  // nouveau maillage : seuls les sommets utilisés (mêmes attributs, même tangente)
  const ancien = new Map(), sv = [], st = [];
  const tan = f.tan ? f.tan[k] : null;
  const ix = new Uint32Array(avant.map((i) => {
    if (!ancien.has(i)) { ancien.set(i, ancien.size); for (let c = 0; c < 8; c++) sv.push(m.v[i * 8 + c]); if (tan) st.push(tan.subarray(i * 4, i * 4 + 4)); }
    return ancien.get(i);
  }));
  m.ix = new Uint32Array(reste);   // le maillage d'origine garde ses sommets (les inutilisés ne sont pas dessinés)
  f.mats.push(nouveau);
  f.maillages.push({ mat: f.mats.length - 1, v: new Float32Array(sv), ix });
  if (f.tan) f.tan.push(Buffer.concat(st));
  journal.push(`vitres avant : ${avant.length / 3} triangles du matériau ${cfg.verre} → matériau ${f.mats.length - 1} (transparent, opacité ${VITRE_AVANT.couleur[3]}, 75 % transmis), ${reste.length / 3} restent en verre`);
  return true;
}
// albédo du verre resté F_GLASS (A 45 : plus de voile laiteux)
function albedoVerre(f, cfg, journal) {
  const M = f.mats[cfg.verre];
  const a = [0, 1, 2].map((q) => M.readFloatLE(4 * q));
  if (a.every((x, q) => x === Math.fround(cfg.albedoVerre[q]))) { journal.push('verre sombre : déjà fait'); return false; }
  cfg.albedoVerre.forEach((x, q) => M.writeFloatLE(x, 4 * q));
  journal.push(`verre (matériau ${cfg.verre}) : albédo ${a.map((x) => x.toFixed(2)).join(' / ')} → ${cfg.albedoVerre.join(' / ')}`);
  return true;
}
// plaque arrière refaite en rectangle plat et vertical de 2 triangles (comme une vraie plaque et les plaques des autres voitures),
// 4 mm devant la peinture opaque la plus en avant sous elle ; normale vers l'arrière (0, 0, 1) ; coordonnées de texture reprises de
// la plaque d'origine (u et v ajustés en ligne droite sur x et y : la plaque de plaques_voitures.mjs est texturée à plat) ;
// tangente du premier sommet d'origine. Raison : 148 petits triangles d'une surface courbe, de hauteur médiane 1,6 mm : le
// chargeur (model_loader.cpp, WFS-42) la prenait pour un fil très fin, dessiné en couverture partielle (peinture visible au travers)
function plaqueDevant(f, mat, journal) {
  const k = f.maillages.findIndex((m) => m.mat === mat);
  if (k < 0) throw new Error(`aucun maillage du matériau ${mat}`);
  const p = f.maillages[k], nv = p.v.length / 8;
  let xmin = 9, xmax = -9, ymin = 9, ymax = -9, zmin = 9, zmax = -9, n = [0, 0, 0];
  for (let i = 0; i < nv; i++) {
    const [x, y, z] = [p.v[i * 8], p.v[i * 8 + 1], p.v[i * 8 + 2]];
    xmin = Math.min(xmin, x); xmax = Math.max(xmax, x); ymin = Math.min(ymin, y); ymax = Math.max(ymax, y);
    zmin = Math.min(zmin, z); zmax = Math.max(zmax, z); for (let c = 0; c < 3; c++) n[c] += p.v[i * 8 + 3 + c];
  }
  if (nv === 4 && p.ix.length === 6 && zmin === zmax && p.v[5] === 1) { journal.push(`plaque arrière : déjà un rectangle plat (z ${zmax.toFixed(3)} m)`); return false; }
  if (n[2] / Math.hypot(...n) < 0.7) throw new Error(`plaque (matériau ${mat}) : normale moyenne ${n.map((c) => (c / Math.hypot(...n)).toFixed(2)).join(' ')} (arrière attendu)`);
  let zPeinture = -9;   // sommets des autres pièces opaques sous la plaque (dans sa largeur et sa hauteur, à ± 5 cm en profondeur)
  f.maillages.forEach((m, j) => {
    if (j === k || (f.mats[m.mat].readUInt32LE(52) & (2 | 64))) return;
    for (const i of new Set(m.ix)) {
      const [x, y, z] = [m.v[i * 8], m.v[i * 8 + 1], m.v[i * 8 + 2]];
      if (x > xmin + 0.01 && x < xmax - 0.01 && y > ymin + 0.01 && y < ymax - 0.01 && z > zmin - 0.05 && z < zmax + 0.05) zPeinture = Math.max(zPeinture, z);
    }
  });
  const plat = Math.max(zPeinture + 0.004, zmax);
  // droites des moindres carrés u = a + b x et v = c + d y
  const droite = (axe, uv) => {
    let sx = 0, sy = 0, sxx = 0, sxy = 0;
    for (let i = 0; i < nv; i++) { const x = p.v[i * 8 + axe], y = p.v[i * 8 + uv]; sx += x; sy += y; sxx += x * x; sxy += x * y; }
    const b = (nv * sxy - sx * sy) / (nv * sxx - sx * sx); return [(sy - b * sx) / nv, b];
  };
  const [ua, ub] = droite(0, 6), [va, vb] = droite(1, 7);
  const coin = (x, y) => [x, y, plat, 0, 0, 1, ua + ub * x, va + vb * y];
  const v = new Float32Array([...coin(xmin, ymin), ...coin(xmax, ymin), ...coin(xmax, ymax), ...coin(xmin, ymax)]);
  if (f.tan) { const t0 = f.tan[k].subarray(0, 4); f.tan[k] = Buffer.concat([t0, t0, t0, t0]); }
  p.v = v; p.ix = new Uint32Array([0, 1, 2, 0, 2, 3]);
  journal.push(`plaque arrière (matériau ${mat}) : ${nv} sommets sur une surface courbe (z ${zmin.toFixed(3)} à ${zmax.toFixed(3)} m, peinture à ${zPeinture.toFixed(3)} m) → rectangle plat de ${((xmax - xmin) * 1000).toFixed(0)} × ${((ymax - ymin) * 1000).toFixed(0)} mm à z ${plat.toFixed(3)} m (u ${(ua + ub * xmin).toFixed(2)} à ${(ua + ub * xmax).toFixed(2)}, v ${(va + vb * ymin).toFixed(2)} à ${(va + vb * ymax).toFixed(2)})`);
  return true;
}

const ids = process.argv.slice(2).length ? process.argv.slice(2).filter((id) => VOITURES[id]) : Object.keys(VOITURES);
for (const id of ids) {
  const cfg = VOITURES[id], chemin = new URL(`assets/cars/${id}.wfm`, RACINE);
  const f = lire(chemin), journal = [];
  let change = false;
  if (cfg.verre !== undefined) change = vitresAvant(f, cfg, journal) || change;
  if (cfg.albedoVerre) change = albedoVerre(f, cfg, journal) || change;
  if (cfg.plaqueArriere !== undefined) change = plaqueDevant(f, cfg.plaqueArriere, journal) || change;
  if (change) ecrire(chemin, f);
  console.log(`${id} : ${journal.join(' ; ')}${change ? '' : ' (rien à écrire)'}`);
}
