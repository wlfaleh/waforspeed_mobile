// WFS-158 (RG-A, ANO-79) : plaques d'immatriculation neutres, au format français (SIV) de 520 × 110 mm, pour remplacer
// les plaques au nom de l'auteur du modèle 3D (« MYEDSU » V60, « LEXYC3D » A 45, logo violet « Unn » Range Rover Sport).
// Plaque : fond blanc, filet noir, bande bleue européenne à gauche (12 étoiles jaunes, lettre « F »), bande bleue à droite
// (numéro de département, sans logo de région), numéro inventé AA-123-AA (lettres I, O et U exclues comme au SIV).
// Chaque image reprend la place et le sens de la plaque d'origine dans sa texture (coordonnées de texture du modèle) : le
// maillage n'est pas touché par l'image. Les PNG produits sont dans le dépôt (tools/plaques/), lus par convert_car.mjs
// (WFS_TEXTURES) : la conversion ne dépend donc pas des polices de la machine.
// usage : node plaques_neutres.mjs   (réécrit tools/plaques/*.png) ; plaque(numero, dep) est aussi utilisée par
// tools/plaques_voitures.mjs (WFS-158, ST-3 : plaques des autres voitures, coordonnées de texture refaites à plat)
import sharp from 'sharp';
import fs from 'fs';
import { pathToFileURL } from 'url';

const MM = 4;                       // pixels par millimètre de la plaque dessinée (2080 × 440)
const L = 520, H = 110;             // plaque SIV (mm)
const BANDE = 42, BLEU = '#003399', JAUNE = '#FFCC00';

// texture de destination : taille de l'image d'origine, zone de la plaque (u0, v0, u1, v1), plaque retournée en hauteur
// (coordonnée v qui monte vers le haut de la plaque : V60, A 45) ; numéro et département de chaque voiture
const PLAQUES = [
  { id: 'v60', w: 1024, h: 512, zone: [0.048, 0.335, 0.938, 0.747], retourne: true, numero: 'EH-604-KB', dep: '69' },
  { id: 'a45', w: 1024, h: 256, zone: [0, 0, 1, 1], retourne: true, numero: 'FN-145-TW', dep: '13' },
  { id: 'rrsport', w: 512, h: 256, zone: [0, 0, 1, 1], retourne: false, numero: 'GR-502-DS', dep: '33' },
];

function etoiles(cx, cy) {   // 12 étoiles à cinq branches sur un cercle de 13 mm (drapeau européen)
  let s = '';
  for (let k = 0; k < 12; k++) {
    const a = (k / 12) * 2 * Math.PI, x = cx + 13 * Math.sin(a), y = cy - 13 * Math.cos(a);
    const pts = [];
    for (let i = 0; i < 10; i++) {
      const r = i % 2 ? 1.05 : 2.6, b = (i / 10) * 2 * Math.PI;
      pts.push(`${(x + r * Math.sin(b)).toFixed(2)},${(y - r * Math.cos(b)).toFixed(2)}`);
    }
    s += `<polygon points="${pts.join(' ')}" fill="${JAUNE}"/>`;
  }
  return s;
}

// fond de la plaque : blanc, filet noir, bandes bleues, étoiles (sans texte)
function svgFond() {
  return `<svg xmlns="http://www.w3.org/2000/svg" width="${L * MM}" height="${H * MM}" viewBox="0 0 ${L} ${H}">
  <rect x="0" y="0" width="${L}" height="${H}" rx="5" fill="#F7F7F5"/>
  <rect x="1.2" y="1.2" width="${L - 2.4}" height="${H - 2.4}" rx="4.2" fill="none" stroke="#111" stroke-width="2"/>
  <path d="M3,3 H${BANDE} V${H - 3} H3 Z" fill="${BLEU}"/>
  <path d="M${L - BANDE},3 H${L - 3} V${H - 3} H${L - BANDE} Z" fill="${BLEU}"/>
  ${etoiles(BANDE / 2 + 1.5, 36)}
</svg>`;
}

// un texte rendu seul, recadré au plus juste, puis mis dans sa boîte (mm) sans déformation, centré : la taille des
// caractères ne dépend pas des mesures de la police (hauteur des capitales fixée par la boîte)
async function texte(t, couleur, boite, serrage) {
  const svg = `<svg xmlns="http://www.w3.org/2000/svg" width="${t.length * 200}" height="400"><text x="20" y="300" font-family="DejaVu Sans Condensed" font-weight="bold" font-size="300" fill="${couleur}">${t}</text></svg>`;
  const brut = await sharp(Buffer.from(svg)).trim().png().toBuffer();
  const m = await sharp(brut).metadata();
  const [x, y, w, h] = boite.map((v) => Math.round(v * MM));
  // police resserrée (serrage < 1) comme celle des plaques, puis hauteur de la boîte
  const largeur = Math.min(w, Math.round((m.width * serrage * h) / m.height));
  const img = await sharp(brut).resize(largeur, h, { fit: 'fill' }).png().toBuffer();
  return { input: img, left: x + Math.round((w - largeur) / 2), top: y };
}

export async function plaque(numero, dep) {
  const fond = await sharp(Buffer.from(svgFond())).png().toBuffer();
  return sharp(fond).composite([
    await texte('F', '#FFFFFF', [8, 74, BANDE - 10, 22], 1),                         // pays : 22 mm, sous les étoiles
    await texte(dep, '#FFFFFF', [L - BANDE + 3, 74, BANDE - 9, 22], 1),              // département : 22 mm
    await texte(numero, '#111111', [BANDE + 12, 17.5, L - 2 * BANDE - 24, 75], 0.78), // numéro : 75 mm de haut
  ]).png().toBuffer();
}

// ligne de commande seulement (pas à l'import)
if (import.meta.url === pathToFileURL(process.argv[1]).href) {
  fs.mkdirSync(new URL('./plaques/', import.meta.url), { recursive: true });
  for (const p of PLAQUES) {
    const [u0, v0, u1, v1] = p.zone;
    const x0 = Math.round(u0 * p.w), y0 = Math.round(v0 * p.h), w = Math.round(u1 * p.w) - x0, h = Math.round(v1 * p.h) - y0;
    let image = sharp(await plaque(p.numero, p.dep)).resize(w, h, { fit: 'fill' });
    if (p.retourne) image = image.flip();
    const img = await sharp({ create: { width: p.w, height: p.h, channels: 3, background: '#F7F7F5' } })
      .composite([{ input: await image.png().toBuffer(), left: x0, top: y0 }]).png({ compressionLevel: 9 }).toBuffer();
    fs.writeFileSync(new URL(`./plaques/${p.id}.png`, import.meta.url), img);
    console.log(`plaques/${p.id}.png`, `${p.w}×${p.h}`, `zone ${w}×${h} en (${x0}, ${y0})`, p.retourne ? 'retournée' : '', p.numero);
  }
  // carte de normales plate (la plaque d'origine de la V60 avait ses lettres en relief) : même taille que l'image d'origine
  const plat = await sharp({ create: { width: 1024, height: 512, channels: 3, background: { r: 128, g: 128, b: 255 } } }).png({ compressionLevel: 9 }).toBuffer();
  fs.writeFileSync(new URL('./plaques/normale-plate.png', import.meta.url), plat);
  console.log('plaques/normale-plate.png 1024×512');
}
