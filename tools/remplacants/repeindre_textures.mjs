// WaForSpeed — WFS-397 (RG-05) : logos imprimés dans les textures d'un remplaçant, repeints AVANT la conversion, sur une copie
// des textures de la source (le dossier d'origine n'est jamais modifié). Zones de textures.json : disque [u, v, r] ou rectangle
// [u0, v0, u1, v1] (uv de 0 à 1, v vers le bas de l'image), peints de la couleur médiane d'un anneau de 3 % autour de la zone.
// usage : node repeindre_textures.mjs <id> <dossier de la source> <dossier copie>   (la copie reçoit scene.gltf, .bin, textures/)
import fs from 'fs';
import path from 'path';
import sharp from 'sharp';

const [,, id, source, copie] = process.argv;
const ICI = path.dirname(new URL(import.meta.url).pathname);
const liste = JSON.parse(fs.readFileSync(path.join(ICI, 'textures.json'), 'utf8'))[id] || [];
fs.cpSync(source, copie, { recursive: true });
for (const t of liste) {
  const f = path.join(copie, t.fichier);
  const { data, info } = await sharp(f).removeAlpha().raw().toBuffer({ resolveWithObject: true });
  const W = info.width, H = info.height, C = info.channels;
  for (const z of t.zones) {
    const dans = z.disque ? (u, v, m) => Math.hypot(u - z.disque[0], v - z.disque[1]) <= z.disque[2] + m
                          : (u, v, m) => u >= z.rect[0] - m && u <= z.rect[2] + m && v >= z.rect[1] - m && v <= z.rect[3] + m;
    const anneau = [[], [], []];
    for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) {
      const u = (x + 0.5) / W, v = (y + 0.5) / H;
      if (dans(u, v, 0.015) && !dans(u, v, 0.005)) for (let c = 0; c < 3; c++) anneau[c].push(data[(y * W + x) * C + c]);
    }
    const med = anneau.map((a) => a.sort((p, q) => p - q)[a.length >> 1]);
    let n = 0;
    for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) if (dans((x + 0.5) / W, (y + 0.5) / H, 0)) { for (let c = 0; c < 3; c++) data[(y * W + x) * C + c] = med[c]; n++; }
    console.log(`  ${t.fichier} : ${z.nom} repeint (${n} pixels, couleur ${med.join(', ')})`);
  }
  const img = sharp(data, { raw: { width: W, height: H, channels: C } });
  await (f.endsWith('.png') ? img.png() : img.jpeg({ quality: 92 })).toFile(f + '.tmp');
  fs.renameSync(f + '.tmp', f);
}
