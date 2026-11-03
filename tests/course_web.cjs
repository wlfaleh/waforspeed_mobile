// Référence des règles de course (WFS-12) : exécute js/game/update.js et js/game/state.js du jeu web
// (dépôt wlfaleh/waaps) sur une course solo automatique et affiche checkpoints, temps et score.
// Même scénario que tests/test_course.cpp : pas de 1/120 s, plein gaz, braquage tout-ou-rien qui garde le centre.
// usage : node tests/course_web.cjs <dossier waforspeed du jeu web> > tests/course_web_ref.txt
'use strict';
const fs = require('fs'), path = require('path'), vm = require('vm');
const web = process.argv[2] || '../waaps/waforspeed';
const ctx = { console, Math, window: {}, document: { querySelector: () => null }, matchMedia: () => ({ matches: false }),
  localStorage: { getItem: () => null, setItem() {} }, navigator: {}, setTimeout: () => 0, performance: { now: () => 0 } };
vm.createContext(ctx);
let src = ['js/core/config.js', 'js/core/utils.js', 'js/cars/catalog.js', 'js/world/track.js', 'js/world/maps.js',
  'js/game/state.js', 'js/game/traffic.js', 'js/game/update.js']
  .map((f) => fs.readFileSync(path.join(web, f), 'utf8')).join('\n').replace(/'use strict';/g, '');
// sans rendu, sans son, sans interface : seules les règles du jeu tournent
src = `function generateDecor() {}
let renderer = null; const ghosts = new Map(); const ZONE_LEN = 448;
function toast() {} function noiseHit() {} function emitSparks() {} function beep() {} function updateGhosts() {}
function sendRacePresence() {} function finishDuel() {}
const counts = []; function showCount(t) { if (t) counts.push(t); }
let ended = false; function endRace() { state = 'over'; ended = true; }
` + src;
src += `
;(function () {
  const out = [];
  for (const [mapId, carId, seed] of [['ville', 'golf', 12345], ['circuit', 'm4', 777]]) {
    MAP = mapById(mapId); buildTrack(); resetPoses(); myCar = carById(carId); PERF = carPerf(myCar);
    mode = 'solo'; resetRace('play', seed); state = 'count'; ended = false; counts.length = 0;
    for (const k in input) input[k] = false;
    const dt = 1 / 120; let lastLevel = 0, accidents = 0, n = 0;
    while (!ended && n < 120 * 400) {
      // pilote du test : plein gaz, braquage tout-ou-rien vers le centre de la route
      input.up = true; input.left = playerX > 0.08; input.right = playerX < -0.08; input.nitro = nitro > 0.5;
      const lockBefore = controlLock;
      update(dt); n++;
      if (controlLock > lockBefore + 0.3) accidents++;
      if (level !== lastLevel) { lastLevel = level; out.push(['cp', mapId, level, n, timeLeft.toFixed(4), score(), nearCount].join(' ')); }
    }
    out.push(['decompte', mapId, counts.join(',')].join(' '));
    out.push(['fin', mapId, n, level, score(), bonus, nearCount, maxKmh, Math.floor(dist), accidents].join(' '));
  }
  console.log(out.join('\\n'));
})();`;
vm.runInContext(src, ctx);
