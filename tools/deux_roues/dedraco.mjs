// WaForSpeed — WFS-232 : décompresse un .glb Draco (KHR_draco_mesh_compression) en .glb ordinaire, lisible par convert_car.mjs.
// usage : DRACO=<dossier où « npm install draco3dgltf » a été fait> node deux_roues/dedraco.mjs entree.glb sortie.glb
// (draco3dgltf : décodeur officiel de Google, Apache-2.0, installé hors du dépôt : il ne sert qu'à cette conversion)
import { NodeIO } from '@gltf-transform/core';
import { ALL_EXTENSIONS } from '@gltf-transform/extensions';
import { createRequire } from 'module';
const [,, entree, sortie] = process.argv;
const require = createRequire((process.env.DRACO || '.') + '/');
const draco3d = require('draco3dgltf');
const io = new NodeIO().registerExtensions(ALL_EXTENSIONS).registerDependencies({ 'draco3d.decoder': await draco3d.createDecoderModule() });
const doc = await io.read(entree);
for (const e of doc.getRoot().listExtensionsUsed()) if (e.extensionName === 'KHR_draco_mesh_compression') e.dispose();
await io.write(sortie, doc);
console.log('écrit', sortie);
