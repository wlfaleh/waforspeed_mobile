// Référence du tracé des cartes (WFS-14) : exécute js/world/track.js du jeu web (dépôt wlfaleh/waaps) pour plage,
// campagne et circuit, et écrit chaque segment (courbure, altitudes). Sert à (re)générer tests/cartes_web_ref.txt.
// usage : node tests/cartes_web.cjs <dossier waforspeed du jeu web> > tests/cartes_web_ref.txt
'use strict';
const fs = require('fs'), path = require('path'), vm = require('vm');
const web = process.argv[2] || '../waaps/waforspeed';
const ctx = { console, Math, window: {}, document: { querySelector: () => null }, matchMedia: () => ({ matches: false }),
  localStorage: { getItem: () => null, setItem() {} }, navigator: {} };
vm.createContext(ctx);
let src = ['js/core/config.js', 'js/core/utils.js', 'js/world/track.js', 'js/world/maps.js']
  .map((f) => fs.readFileSync(path.join(web, f), 'utf8')).join('\n').replace(/'use strict';/g, '');
src = 'function generateDecor() {}\n' + src + `
;(function () {
  const out = [];
  for (const id of ['plage', 'campagne', 'circuit']) {
    MAP = mapById(id); buildTrack();
    out.push(id + ' ' + segments.length);
    for (const s of segments) out.push(s.curve.toPrecision(17) + ' ' + s.p1.world.y.toPrecision(17) + ' ' + s.p2.world.y.toPrecision(17));
  }
  console.log(out.join('\\n'));
})();`;
vm.runInContext(src, ctx);
