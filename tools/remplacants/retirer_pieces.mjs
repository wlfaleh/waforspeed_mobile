// WaForSpeed — WFS-397 (RG-05) : voitures sans marque. Retire d'un remplaçant les pièces de logo listées dans pieces.json
// (étoile de la calandre, du capot, …) : pièce = triangles connexes d'un matériau, entièrement dans la boîte donnée, avec le nombre
// de triangles attendu (contrôle : si le modèle a changé, l'outil s'arrête sans rien écrire). Rien d'autre ne bouge.
// Les outils existants (effacer_logos.mjs, logos.json) ne traitent pas assets/cars/remplacants/ : d'où ce fichier à part.
// Action « prolonger », mode « bouts » : seuls les sommets à |x| < seuil vont à x = 0 (lames peintes dont les triangles vont
// d'un bout à l'autre : les allonger ne change pas leur profil).
// Action « prolonger » : la pièce n'est pas retirée, elle est allongée en x jusqu'au milieu (x = 0) : les barres de calandre
// coupées par l'anneau du logo se rejoignent au milieu (grille continue, rien de posé devant la carrosserie : leçon de la X5).
// Action « cercle » : seuls les triangles de la pièce entièrement dans le cercle (plan x, y) de centre « centre » et de rayon
// « rayon » sont retirés (anneau de logo modelé dans la même pièce peinte que les bords de la calandre).
// usage : node retirer_pieces.mjs <id> entree.wfm sortie.wfm
import fs from 'fs';
import path from 'path';
import { lireWfm, ecrireWfm, pieces, retirerTriangles } from './wfm.mjs';

const [,, id, entree, sortie] = process.argv;
const ICI = path.dirname(new URL(import.meta.url).pathname);
const liste = JSON.parse(fs.readFileSync(path.join(ICI, 'pieces.json'), 'utf8'))[id];
if (!liste) { console.error(`${id} : aucune entrée dans pieces.json`); process.exit(2); }
const w = lireWfm(entree);
const dans = (b, B) => b[0] >= B[0] && b[1] >= B[1] && b[2] >= B[2] && b[3] <= B[3] && b[4] <= B[4] && b[5] <= B[5];
const parMaillage = new Map();
let erreurs = 0;
for (const p of liste) {
  const k = w.maillages.findIndex((m) => m.mat === p.materiau);
  const trouvees = k < 0 ? [] : pieces(w.maillages[k]).filter((x) => dans(x.boite, p.boite) && x.triangles.length === p.triangles);
  if (trouvees.length !== (p.nombre || 1)) { console.error(`  ${p.nom} : ${trouvees.length} pièce(s) trouvée(s) au lieu de ${p.nombre || 1}`); erreurs++; continue; }
  if (p.action === 'prolonger') {
    // la barre est allongée en x de son bout extérieur jusqu'au milieu (x = 0) : x' = xExt + (x − xExt) · |xExt| / |xExt − xSeuil|
    // pour |x| ≥ seuil, sommets de la pointe arrondie (|x| < seuil) amenés au milieu, cachés dans la section de l'autre moitié ;
    // l'axe de la barre (y, z moyens par tranche de 1 cm) suit la calandre : chaque sommet garde son écart à l'axe d'origine,
    // posé sur l'axe d'origine au nouvel x (la barre reste sur la courbe de la grille, sans être aplatie ni déformée)
    const m = w.maillages[k], vus = new Set();
    for (const x of trouvees) for (const t of x.triangles) for (let j = 0; j < 3; j++) vus.add(m.idx[t * 3 + j]);
    const cotes = [-1, 1];
    let n = 0;
    for (const sg of cotes) {
      const ids = [...vus].filter((i) => Math.sign(m.pos[i * 3]) === sg);
      if (!ids.length) continue;
      const xExt = Math.max(...ids.map((i) => Math.abs(m.pos[i * 3])));
      const tranches = new Map();
      for (const i of ids) { const c = Math.round(Math.abs(m.pos[i * 3]) * 100); const e = tranches.get(c) || [0, 0, 0]; e[0] += m.pos[i * 3 + 1]; e[1] += m.pos[i * 3 + 2]; e[2]++; tranches.set(c, e); }
      const cles = [...tranches.keys()].sort((a, b) => a - b);
      const axe = (ax) => {   // axe (y, z) à |x| = ax, interpolé entre tranches, prolongé à plat au-delà
        const c = ax * 100; let a = cles[0], b = cles[cles.length - 1];
        for (const q of cles) { if (q <= c) a = q; if (q >= c) { b = q; break; } }
        const ea = tranches.get(a), eb = tranches.get(b), t = b === a ? 0 : Math.min(1, Math.max(0, (c - a) / (b - a)));
        return [ea[0] / ea[2] * (1 - t) + eb[0] / eb[2] * t, ea[1] / ea[2] * (1 - t) + eb[1] / eb[2] * t];
      };
      const k2 = p.mode === 'bouts' ? 0 : xExt / (xExt - p.seuil);   // « bouts » : seuls les sommets à |x| < seuil vont au milieu
      for (const i of ids) {
        const ax = Math.abs(m.pos[i * 3]), nx = ax < p.seuil ? 0 : p.mode === 'bouts' ? ax : xExt + (ax - xExt) * k2;
        if (p.mode === 'bouts' && ax >= p.seuil) continue;
        const [y0, z0] = axe(ax), [y1, z1] = axe(Math.max(nx, p.seuil));
        m.pos[i * 3] = sg * nx; m.pos[i * 3 + 1] += y1 - y0; m.pos[i * 3 + 2] += z1 - z0; n++;
      }
    }
    console.log(`  ${p.nom} : ${n} sommets déplacés, barre allongée jusqu'au milieu (matériau ${p.materiau})`);
    continue;
  }
  const m = w.maillages[k];
  const garde = (t) => p.action !== 'cercle' || [0, 1, 2].every((j) => { const i = m.idx[t * 3 + j]; return Math.hypot(m.pos[i * 3] - p.centre[0], m.pos[i * 3 + 1] - p.centre[1]) <= p.rayon; });
  const r = parMaillage.get(k) || []; let n = 0;
  for (const x of trouvees) for (const t of x.triangles) if (garde(t)) { r.push(t); n++; }
  parMaillage.set(k, r);
  console.log(`  ${p.nom} : ${n} triangles retirés (matériau ${p.materiau})`);
}
if (erreurs) { console.error(`${id} : modèle inattendu, fichier non écrit`); process.exit(1); }
for (const [k, r] of parMaillage) retirerTriangles(w.maillages[k], r);
ecrireWfm(sortie, w);
