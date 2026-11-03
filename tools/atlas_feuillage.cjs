// WFS-34 — usage : servir waaps/waforspeed/ sur http://127.0.0.1:8771 (python3 -m http.server 8771), puis
//   node tools/atlas_feuillage.cjs assets/world/feuillage.png
// exporte l'atlas de feuillage du jeu web (makeFoliageAtlas de js/world/life.js) en PNG, sans le redessiner
const { chromium } = require('playwright');
const fs = require('fs');
(async () => {
  const b = await chromium.launch({ args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'] });
  const p = await b.newPage();
  await p.goto('http://127.0.0.1:8771/index.html'); await p.waitForTimeout(3000);
  const url = await p.evaluate(() => {
    const t = makeFoliageAtlas(), d = t.image.data, c = document.createElement('canvas'); c.width = t.image.width; c.height = t.image.height;
    const g = c.getContext('2d'), im = g.createImageData(c.width, c.height); im.data.set(d); g.putImageData(im, 0, 0);
    return c.toDataURL('image/png');
  });
  fs.writeFileSync(process.argv[2], Buffer.from(url.split(',')[1], 'base64'));
  await b.close();
})();
