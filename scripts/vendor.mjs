// Copie les bibliothèques (three r128, PeerJS) et les polices dans www/lib,
// pour que le jeu marche hors ligne et dans l'appli Android (aucun CDN nécessaire).
// usage : npm run vendor
import fs from 'fs';
import path from 'path';

const nm = 'node_modules';
const out = 'www/lib';
const files = {
  'three.min.js': 'three/build/three.min.js',
  'CopyShader.js': 'three/examples/js/shaders/CopyShader.js',
  'LuminosityHighPassShader.js': 'three/examples/js/shaders/LuminosityHighPassShader.js',
  'EffectComposer.js': 'three/examples/js/postprocessing/EffectComposer.js',
  'RenderPass.js': 'three/examples/js/postprocessing/RenderPass.js',
  'ShaderPass.js': 'three/examples/js/postprocessing/ShaderPass.js',
  'UnrealBloomPass.js': 'three/examples/js/postprocessing/UnrealBloomPass.js',
  'GLTFLoader.js': 'three/examples/js/loaders/GLTFLoader.js',
  'peerjs.min.js': 'peerjs/dist/peerjs.min.js',
};
const fonts = [
  ['rajdhani', 600], ['rajdhani', 700],
  ['orbitron', 700], ['orbitron', 800], ['orbitron', 900],
  ['inter', 400], ['inter', 500], ['inter', 600],
];

fs.mkdirSync(path.join(out, 'fonts'), { recursive: true });
for (const [dst, src] of Object.entries(files)) fs.copyFileSync(path.join(nm, src), path.join(out, dst));

const family = { rajdhani: 'Rajdhani', orbitron: 'Orbitron', inter: 'Inter' };
let css = '/* Polices intégrées (générées par scripts/vendor.mjs, licence SIL OFL 1.1). */\n';
for (const [f, w] of fonts) {
  const name = `${f}-latin-${w}-normal.woff2`;
  fs.copyFileSync(path.join(nm, '@fontsource', f, 'files', name), path.join(out, 'fonts', name));
  css += `@font-face { font-family: '${family[f]}'; font-style: normal; font-weight: ${w}; font-display: swap; src: url(fonts/${name}) format('woff2'); }\n`;
}
fs.writeFileSync(path.join(out, 'fonts.css'), css);
console.log('Bibliothèques et polices copiées dans', out);

// liste des fichiers que le service worker met en cache à l'installation (hors modèles 3D)
const list = ['./', 'index.html', 'manifest.webmanifest'];
const walk = (d) => { for (const f of fs.readdirSync(path.join('www', d)).sort()) {
  const rel = d + '/' + f;
  if (fs.statSync(path.join('www', rel)).isDirectory()) walk(rel); else list.push(rel);
} };
['css', 'js', 'lib', 'icons'].forEach(walk);
fs.writeFileSync('www/sw-files.js', '/* Généré par scripts/vendor.mjs — ne pas modifier à la main. */\nself.SHELL_FILES = ' + JSON.stringify(list, null, 1) + ';\n');
console.log(list.length, 'fichiers listés pour le mode hors ligne (www/sw-files.js)');
