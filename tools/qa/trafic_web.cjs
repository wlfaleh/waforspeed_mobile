// Référence du trafic : exécute js/game/traffic.js du jeu web (dépôt wlfaleh/waaps) dans le même scénario
// que tests/test_traffic.cpp et affiche l'état des voitures. Sert à (re)générer les valeurs attendues du test.
// usage : node tools/qa/trafic_web.cjs <dossier waforspeed du jeu web>
'use strict';
const fs = require('fs'), path = require('path'), vm = require('vm');
const web = process.argv[2] || '../waaps/waforspeed';
const ctx = { console, Math, window: {}, document: { querySelector: () => null }, matchMedia: () => ({ matches: false }),
  localStorage: { getItem: () => null, setItem() {} }, navigator: {} };
vm.createContext(ctx);
let src = ['js/core/config.js', 'js/core/utils.js', 'js/cars/catalog.js', 'js/world/track.js', 'js/world/maps.js', 'js/game/state.js', 'js/game/traffic.js']
  .map((f) => fs.readFileSync(path.join(web, f), 'utf8')).join('\n').replace(/'use strict';/g, '');
src = 'function generateDecor() {}\n' + src;   // décor inutile ici (n'influence pas le trafic)
// state.js appelle addCars via resetRace seulement : on fait le scénario nous-mêmes
src += `
;(function () {
  const out = [];
  for (const [mapId, seed] of [['ville', 12345], ['circuit', 777]]) {
    MAP = mapById(mapId); buildTrack();
    cars = []; trafficSeed = seed; state = 'play'; myCar = carById('golf');
    position = 0; playerX = 0; speed = MAX_SPEED * 0.4;
    addCars(MAP.traffic.n);
    const snap = (tag) => { for (let i = 0; i < cars.length; i += 5) { const c = cars[i];
      out.push([mapId, tag, i, c.type, c.color, c.lane, c.target, c.z.toFixed(3), c.offset.toFixed(6), c.speed.toFixed(3)].join(' ')); } };
    snap(0);
    for (let s = 1; s <= 1200; s++) { stepTraffic(T_STEP); position = wrap(position + speed * T_STEP); if (s % 600 === 0) snap(s); }
  }
  console.log(out.join('\\n'));
})();`;
vm.runInContext(src, ctx);
