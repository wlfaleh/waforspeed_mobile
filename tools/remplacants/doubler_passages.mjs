// WaForSpeed — WFS-397 (RG-06, retour du lead dev sur la PR #393) : doublures de passage de roue. La W212 de Peter_D n'en a pas :
// tant que le pneu touchait presque l'aile (13 à 22 mm), le trou ne se voyait pas ; caisse relevée de 40 mm, on voyait le décor à
// travers le passage. Comme sur la vraie voiture, chaque passage reçoit une coque en plastique noir cintrée au-dessus de la roue,
// dans le matériau du dessous de caisse : rayon = bord de l'aile mesuré + 1 cm (cachée derrière l'aile, vue seulement par
// l'ouverture), largeur du flanc intérieur du pneu jusque sous l'aile, de part et d'autre du sommet de la roue ; deux faces (les
// normales regardent la roue d'un côté, la caisse de l'autre), fermée côté moteur par sa paroi intérieure. Les roues ne bougent
// pas, rien d'autre ne change.
// usage : node doubler_passages.mjs <id> entree.wfm sortie.wfm      (réglages : passages.json)
import fs from 'fs';
import path from 'path';
import { lireWfm, ecrireWfm, roues, jeuPassages, ajouterSommets } from './wfm.mjs';

const [,, id, entree, sortie] = process.argv;
const ICI = path.dirname(new URL(import.meta.url).pathname);
const r = JSON.parse(fs.readFileSync(path.join(ICI, 'passages.json'), 'utf8'))[id];
if (!r) { console.error(`${id} : aucune entrée dans passages.json`); process.exit(2); }
const w = lireWfm(entree);
const m = w.maillages.find((x) => x.mat === r.materiau);
if (!m) { console.error(`${id} : matériau ${r.materiau} absent`); process.exit(1); }
const R4 = roues(w), jeux = jeuPassages(w, R4);
// teinte : uv d'un sommet du dessous de caisse, le plus proche du sommet de la roue (texel du plastique noir voisin)
const uvPres = (p) => { let b = 0, d0 = Infinity; for (const i of m.idx) { const d = Math.hypot(m.pos[i * 3] - p[0], m.pos[i * 3 + 1] - p[1], m.pos[i * 3 + 2] - p[2]); if (d < d0) { d0 = d; b = i; } }
  const o = m.debutSommets + b * 32 + 24; return [w.buf.readFloatLE(o), w.buf.readFloatLE(o + 4)]; };
const N = r.segments || 24, X = r.tranches || 2, A = (r.demiAngle || 100) * Math.PI / 180;
let tri = 0;
R4.forEach((roue, k) => {
  const rl = jeux[k].arche - roue.c[1] + (r.marge ?? 0.01);   // rayon de la doublure
  const sg = Math.sign(roue.c[0]), ax = Math.abs(roue.c[0]);
  const x0 = ax - roue.hx - (r.interieur ?? 0.03), x1 = ax + roue.hx * (r.exterieur ?? 0.6);
  const uv = uvPres([roue.c[0], roue.c[1] + rl, roue.c[2]]);
  const sommets = [];
  for (let a = 0; a <= N; a++) {
    const th = -A + 2 * A * a / N;           // 0 : sommet de la roue ; ± vers l'avant et l'arrière
    const dy = Math.cos(th), dz = Math.sin(th);
    for (let f = 0; f < 2; f++) for (let b = 0; b <= X; b++) {
      const x = sg * (x0 + (x1 - x0) * b / X);
      const n = f ? [0, dy, dz] : [0, -dy, -dz];   // face 0 vers la roue, face 1 vers la caisse
      sommets.push({ p: [x, roue.c[1] + rl * dy, roue.c[2] + rl * dz], n, uv, t: [0, -dz, dy, 1] });
    }
  }
  const base = ajouterSommets(m, sommets), L = 2 * (X + 1);
  const ids = [...m.idx];
  for (let a = 0; a < N; a++) for (let b = 0; b < X; b++) for (let f = 0; f < 2; f++) {
    const i0 = base + a * L + f * (X + 1) + b, i1 = i0 + 1, i2 = i0 + L, i3 = i2 + 1;
    // sens des triangles opposés sur les deux faces (chaque face visible de son côté seulement)
    const face0 = (f === 0) === (sg > 0);
    if (face0) ids.push(i0, i2, i1, i1, i2, i3); else ids.push(i0, i1, i2, i1, i3, i2);
    tri += 2;
  }
  // paroi intérieure du passage (tôle côté moteur), à x0 : du bord de la doublure jusqu'à « bas » sous l'axe de la roue ; sans elle,
  // un regard en biais passe sous la doublure et traverse le compartiment moteur vide
  const yBas = roue.c[1] - (r.bas ?? 0.15), paroi = [];
  for (let a = 0; a <= N; a++) {
    const th = -A + 2 * A * a / N, y = roue.c[1] + rl * Math.cos(th), z = roue.c[2] + rl * Math.sin(th);
    for (let f = 0; f < 2; f++) for (const yy of [y, Math.min(y, Math.max(yBas, y - 2 * rl))]) {
      const yb = Math.max(yy, yBas);
      paroi.push({ p: [sg * x0, yy === y ? y : Math.min(y, yb), z], n: [f ? -sg : sg, 0, 0], uv, t: [0, 0, 1, 1] });
    }
  }
  const bp = ajouterSommets(m, paroi);
  for (let a = 0; a < N; a++) for (let f = 0; f < 2; f++) {
    const j0 = bp + a * 4 + f * 2, j1 = j0 + 1, j2 = j0 + 4, j3 = j2 + 1;
    if ((f === 0) === (sg > 0)) ids.push(j0, j1, j2, j1, j3, j2); else ids.push(j0, j2, j1, j1, j2, j3);
    tri += 2;
  }
  m.idx = Uint32Array.from(ids);
  console.log(`  ${roue.cle} : doublure de rayon ${(rl * 1000).toFixed(0)} mm (bord de l'aile ${((jeux[k].arche - roue.c[1]) * 1000).toFixed(0)} mm), x ${(sg * x0).toFixed(3)} → ${(sg * x1).toFixed(3)} m`);
});
console.log(`  ${tri} triangles ajoutés (matériau ${r.materiau})`);
ecrireWfm(sortie, w);
