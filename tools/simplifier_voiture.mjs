// WaForSpeed — WFS-158 (RG-D) : allège un modèle de voiture glTF trop lourd pour le téléphone avant tools/convert_car.mjs
// (Model 3 « Tesla 2018 Model 3 » d'Ameer Studio : 684 000 triangles, 23 Mo une fois converti, pour un budget de 8 Mo et du
// même ordre que l'ancien modèle, 135 000 triangles). Soudure des sommets identiques, puis simplification meshoptimizer à
// erreur bornée, matériau par matériau : la carrosserie (peinture) garde une erreur 4 fois plus petite que le reste (aucune
// facette : la surface reste à moins de ERREUR_PEINTURE de l'originale), l'habitacle et les roues sont allégés davantage ;
// bords et coutures d'uv verrouillés (aucun trou, aucune texture déplacée). Les normales d'origine sont gardées.
// usage : node simplifier_voiture.mjs entree.gltf sortie.glb <regex_peinture> [erreur, défaut 0.0008 de la taille du modèle]
import { NodeIO } from '@gltf-transform/core';
import { ALL_EXTENSIONS } from '@gltf-transform/extensions';
import { weld, simplifyPrimitive, dedup, prune } from '@gltf-transform/functions';
import { MeshoptSimplifier, MeshoptDecoder } from 'meshoptimizer';

const [,, entree, sortie, peintureArg, erreurArg] = process.argv;
const ERREUR = parseFloat(erreurArg || '0.0008'), ERREUR_PEINTURE = ERREUR / 4;
const peinture = new RegExp(peintureArg || '^primary$');
await MeshoptSimplifier.ready; await MeshoptDecoder.ready;
const io = new NodeIO().registerExtensions(ALL_EXTENSIONS).registerDependencies({ 'meshopt.decoder': MeshoptDecoder });
const doc = await io.read(entree);
const compte = () => doc.getRoot().listMeshes().reduce((s, m) => s + m.listPrimitives().reduce((t, p) => t + (p.getIndices() ? p.getIndices().getCount() : p.getAttribute('POSITION').getCount()) / 3, 0), 0);
const avant = compte();
await doc.transform(weld(), dedup());
const parMat = new Map();
for (const mesh of doc.getRoot().listMeshes()) for (const prim of mesh.listPrimitives()) {
  if (prim.getMode() !== 4) continue;
  const nom = (prim.getMaterial() && prim.getMaterial().getName()) || '';
  const n0 = (prim.getIndices() ? prim.getIndices().getCount() : prim.getAttribute('POSITION').getCount()) / 3;
  if (n0 < 200) continue;   // petites pièces : intactes
  simplifyPrimitive(prim, { simplifier: MeshoptSimplifier, ratio: 0, error: peinture.test(nom) ? ERREUR_PEINTURE : ERREUR, lockBorder: true });
  const n1 = (prim.getIndices() ? prim.getIndices().getCount() : 0) / 3;
  const e = parMat.get(nom) || [0, 0]; e[0] += n0; e[1] += n1; parMat.set(nom, e);
}
await doc.transform(prune());
await io.write(sortie, doc);
const apres = compte();
console.log(`${Math.round(avant)} → ${Math.round(apres)} triangles (${(100 * apres / avant).toFixed(0)} %) ; erreur ${ERREUR} (peinture ${ERREUR_PEINTURE})`);
for (const [nom, [a, b]] of [...parMat].sort((x, y) => y[1][0] - x[1][0]).slice(0, 12)) console.log(`  ${nom} : ${a} → ${b}`);
