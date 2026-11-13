// WaForSpeed — WFS-397 (RG-01, RG-02) : relève la caisse d'un remplaçant par rapport à ses roues, roues fixes.
// Les modèles libres retenus (Classe E W212 de Peter_D, VW Virtus GT de BHP3D) sont posés « bas » : le pneu touche presque l'aile
// (W212 : 22 mm de jeu à l'avant, 13 mm à l'arrière), d'où une hauteur hors tout 2,6 à 2,7 % sous la fiche. On remonte toute la
// caisse (carrosserie, vitres, habitacle, dessous) de « mm » millimètres ; les roues, et les pièces entièrement dans le cylindre
// d'une roue (disques, étriers, moyeux), ne bougent pas : rien n'est étiré, longueur, largeur, empattement et voies ne changent pas.
// Les pièces qui ne sont qu'en partie dans le cylindre d'une roue (morceau d'étrier, pare-boue) suivent la caisse ou la roue
// selon où est la plus grande partie de leurs sommets, pour qu'aucun triangle ne soit étiré entre les deux.
// usage : node relever_caisse.mjs entree.wfm sortie.wfm <mm> [--essai]
import { lireWfm, ecrireWfm, roues, sommetsDesRoues, jeuPassages, estRoue, PEINTURE } from './wfm.mjs';

const [,, entree, sortie, mmArg, ...opts] = process.argv;
const essai = opts.includes('--essai');
const d = parseFloat(mmArg) / 1000;
if (!entree || !sortie || !Number.isFinite(d)) { console.error('usage : node relever_caisse.mjs entree.wfm sortie.wfm <mm> [--essai]'); process.exit(2); }
const w = lireWfm(entree);
const R4 = roues(w);
const fixe = sommetsDesRoues(w, R4);

// pièces (composantes connexes par les indices) des maillages ni roue ni peinture : toute la pièce suit la majorité
let pieces = 0;
w.maillages.forEach((m, k) => {
  if (estRoue(m) || (m.drapeaux & PEINTURE)) return;
  const parent = new Int32Array(m.nv).map((_, i) => i);
  const racine = (i) => { while (parent[i] !== i) { parent[i] = parent[parent[i]]; i = parent[i]; } return i; };
  for (let t = 0; t < m.idx.length; t += 3) for (let j = 1; j < 3; j++) {
    const a = racine(m.idx[t]), b = racine(m.idx[t + j]); if (a !== b) parent[a] = b;
  }
  const n = new Map();
  for (let i = 0; i < m.nv; i++) { const r = racine(i), e = n.get(r) || [0, 0]; e[fixe[k][i]]++; n.set(r, e); }
  for (let i = 0; i < m.nv; i++) {
    const [libres, fixes] = n.get(racine(i));
    if (libres && fixes) { const f = fixes > libres ? 1 : 0; if (fixe[k][i] !== f) { fixe[k][i] = f; pieces++; } }
  }
});

// triangles encore étirés (un sommet fixe, un autre qui monte)
let etires = 0;
w.maillages.forEach((m, k) => {
  for (let t = 0; t < m.idx.length; t += 3) {
    const s = fixe[k][m.idx[t]] + fixe[k][m.idx[t + 1]] + fixe[k][m.idx[t + 2]];
    etires += s > 0 && s < 3;
  }
});
const avant = jeuPassages(w, R4);
let montes = 0;
w.maillages.forEach((m, k) => { for (let i = 0; i < m.nv; i++) if (!fixe[k][i]) { m.pos[i * 3 + 1] += d; montes++; } });
const apres = jeuPassages(w, R4);
console.log(`caisse relevée de ${(d * 1000).toFixed(1)} mm : ${montes} sommets montés ; ${pieces} sommets rangés avec leur pièce ; ${etires} triangles étirés`);
for (let i = 0; i < 4; i++) console.log(`  ${avant[i].cle} : jeu pneu / passage de roue ${(avant[i].jeu * 1000).toFixed(0)} → ${(apres[i].jeu * 1000).toFixed(0)} mm`);
if (etires) { console.error('des triangles seraient étirés entre la caisse et une roue : fichier non écrit'); process.exit(1); }
if (!essai) ecrireWfm(sortie, w);
