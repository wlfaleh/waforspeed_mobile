// WaForSpeed — WFS-192 (RG-01, RG-06 à RG-11) : voitures sans marque. Retouche directement assets/cars/<id>.wfm d'après
// tools/logos.json : zones de texture repeintes (logos imprimés : couleur de la pièce autour), pièces de logo retirées
// (triangles du badge : la carrosserie derrière est fermée, RG-10), puis régénère le .lod de la voiture (tools/alleger_voiture.mjs,
// sans le modifier) si elle a perdu des triangles.
//
// usage (depuis tools/) : node effacer_logos.mjs [--verifier] [id …]      (sans id : toutes les voitures de logos.json)
//   --verifier : contrôle seulement (aucune écriture) : dit pour chaque voiture « à faire », « déjà faite » ou l'écart trouvé.
//
// Règles (regles-de-gestion/WFS-192.md, décision du PO du 2026-11-08) :
//  - rien d'autre ne change : nombre et ordre des matériaux et des textures gardés (src/render/correctifs_voitures.h les désigne
//    par rang) ; une texture sans logo reste identique octet pour octet ; une texture retouchée est réenregistrée dans son format
//    (JPEG qualité 88 comme tools/convert_car.mjs, ou PNG si elle a de la transparence), à la même taille ; sommets, section 'TAN1',
//    plaques, échelle et roues gardés ; seuls les triangles des pièces de logo sont retirés ; un maillage vidé est retiré du fichier
//    (avec ses tangentes) : il n'est plus dessiné (aucun appel de dessin) ; son matériau reste à son rang ;
//  - rejouable : chaque retouche est contrôlée avant (empreinte SHA-256 de la texture, nombre de triangles du maillage, boîte de la
//    pièce) : déjà faite → rien ; ni avant ni après → arrêt sans rien écrire, avec la raison ; une 2e passe ne change aucun octet ;
//  - ne lit que le dépôt (pas les sources du jeu web) ; ne touche jamais assets/cars/deux_roues/ ni assets/cars/remplacants/.
//
// logos.json : { "<id>": { "materiaux": <nombre>, "textures": [ { "rang": n, "sha256": "<16 hex>", "sha256Apres": "<16 hex>",
//   "role": "couleur" | "normale" | "metal", "zones": [ { "rect": [u0, v0, u1, v1] } | { "disque": [cu, cv, r] }, … ],
//   "couleur": "auto" | "#rrggbb", "materiau": k (matériau qui l'utilise, pour l'albédo), "comparer": [ { "materiau": a, "voisin": b } ] } ], "pieces": [ { "materiau": k, "triangles": [debut, nombre], "trianglesAvant": t,
//   "boite": [x0, y0, z0, x1, y1, z1], "nom": "…" } ], "materiauxRetouches": [ { "rang", "avant", "apres", "voisin" } ] (RG-12) } } ; zone : { "rect" | "disque" | "anneau": [cu, cv, r0, r1] (flanc de pneu),
//   "couleur" (propre à la zone), "reference" (surface voisine dans un atlas : zone ou [u0, v0, u1, v1]), "transparent": true (alpha 0) } ; le maillage d'une pièce est celui de son matériau (un seul par matériau) ; uv de 0 à 1, v vers le bas de l'image ; « auto » : couleur médiane d'un
//   anneau de 3 % autour de la zone (la pièce voisine) ; normale : (128, 128, 255) plat.
import fs from 'fs';
import crypto from 'crypto';
import path from 'path';
import { execFileSync } from 'child_process';
import sharp from 'sharp';

const ICI = path.dirname(new URL(import.meta.url).pathname);
const RACINE = path.join(ICI, '..');
const args = process.argv.slice(2);
const verifier = args.includes('--verifier');
const choix = args.filter((a) => !a.startsWith('--'));
const LOGOS = JSON.parse(fs.readFileSync(path.join(ICI, 'logos.json'), 'utf8'));
const empreinte = (b) => crypto.createHash('sha256').update(b).digest('hex').slice(0, 16);

// ---------- lecture / écriture d'un .wfm (format : src/render/model_loader.cpp, mesh.h) ----------
function lire(b) {
  let o = 0;
  const u32 = () => { const v = b.readUInt32LE(o); o += 4; return v; };
  if (b.toString('latin1', 0, 4) !== 'WFM1') throw new Error('format inconnu');
  o = 4;
  const version = u32(), nT = u32(), nM = u32(), nMe = u32();
  const boite = b.subarray(o, o + 24); o += 24;
  const textures = [];
  for (let i = 0; i < nT; i++) { const n = u32(); textures.push(Buffer.from(b.subarray(o, o + n))); o += n; }
  const materiaux = b.subarray(o, o + 56 * nM); o += 56 * nM;
  const maillages = [];
  for (let i = 0; i < nMe; i++) {
    const mat = u32(), nv = u32(), ni = u32();
    const v = b.subarray(o, o + nv * 32); o += nv * 32;
    const ix = new Uint32Array(b.buffer.slice(b.byteOffset + o, b.byteOffset + o + ni * 4)); o += ni * 4;
    maillages.push({ mat, nv, v, ix });
  }
  let tan = null;   // WFS-149 : section 'TAN1' (version 2) : nv × 4 octets par maillage, dans l'ordre
  if (version >= 2 && o + 4 <= b.length && b.toString('latin1', o, o + 4) === 'TAN1') {
    let p = o + 4; tan = [];
    for (const m of maillages) { tan.push(b.subarray(p, p + m.nv * 4)); p += m.nv * 4; }
    if (p !== b.length) throw new Error(`section TAN1 de ${b.length - o} octets inattendue`);
  } else if (o !== b.length) throw new Error(`${b.length - o} octets inattendus après les maillages`);
  return { version, boite, textures, nM, materiaux, maillages, tan };
}
function ecrire(w) {
  const parts = [Buffer.from('WFM1')];
  const u32 = (...a) => parts.push(Buffer.from(new Uint32Array(a).buffer));
  u32(w.version, w.textures.length, w.nM, w.maillages.length); parts.push(Buffer.from(w.boite));
  for (const t of w.textures) { u32(t.length); parts.push(t); }
  parts.push(Buffer.from(w.materiaux));
  for (const m of w.maillages) { u32(m.mat, m.nv, m.ix.length); parts.push(Buffer.from(m.v)); parts.push(Buffer.from(m.ix.buffer, m.ix.byteOffset, m.ix.byteLength)); }
  if (w.tan) { parts.push(Buffer.from('TAN1')); for (const t of w.tan) parts.push(Buffer.from(t)); }
  return Buffer.concat(parts);
}
function boiteTriangles(m, debut, nombre) {
  const lo = [1e9, 1e9, 1e9], hi = [-1e9, -1e9, -1e9];
  const f = new Float32Array(m.v.buffer.slice(m.v.byteOffset, m.v.byteOffset + m.v.byteLength));
  for (let t = debut; t < debut + nombre; t++) for (let k = 0; k < 3; k++) {
    const s = m.ix[t * 3 + k];
    for (let a = 0; a < 3; a++) { lo[a] = Math.min(lo[a], f[s * 8 + a]); hi[a] = Math.max(hi[a], f[s * 8 + a]); }
  }
  return [...lo, ...hi];
}

// ---------- mesure des écarts de couleur (charte DA-realisme § 3 : ΔE2000 sur l'albédo) ----------
// albédo = couleur de base du matériau (linéaire, glTF) × texel décodé sRGB → linéaire ; puis XYZ (D65) → L*a*b*.
const lin = (c) => { c /= 255; return c <= 0.04045 ? c / 12.92 : ((c + 0.055) / 1.055) ** 2.4; };
const LIN = Float64Array.from({ length: 256 }, (_, i) => lin(i));
function versLab(r, g, b, f = [1, 1, 1]) {
  const R = LIN[r] * f[0], G = LIN[g] * f[1], B = LIN[b] * f[2];
  const X = (0.4124 * R + 0.3576 * G + 0.1805 * B) / 0.95047, Y = 0.2126 * R + 0.7152 * G + 0.0722 * B, Z = (0.0193 * R + 0.1192 * G + 0.9505 * B) / 1.08883;
  const h = (t) => (t > 216 / 24389 ? Math.cbrt(t) : (24389 / 27 * t + 16) / 116);
  const fx = h(X), fy = h(Y), fz = h(Z);
  return [116 * fy - 16, 500 * (fx - fy), 200 * (fy - fz)];
}
function deltaE2000([L1, a1, b1], [L2, a2, b2]) {   // CIEDE2000 (Sharma, Wu, Dalal 2005), kL = kC = kH = 1
  const rad = Math.PI / 180, C1 = Math.hypot(a1, b1), C2 = Math.hypot(a2, b2), Cm = (C1 + C2) / 2;
  const G = 0.5 * (1 - Math.sqrt(Cm ** 7 / (Cm ** 7 + 25 ** 7)));
  const A1 = a1 * (1 + G), A2 = a2 * (1 + G), c1 = Math.hypot(A1, b1), c2 = Math.hypot(A2, b2);
  const t1 = (Math.atan2(b1, A1) / rad + 360) % 360, t2 = (Math.atan2(b2, A2) / rad + 360) % 360;
  const dL = L2 - L1, dC = c2 - c1;
  let dh = c1 * c2 === 0 ? 0 : t2 - t1; if (dh > 180) dh -= 360; else if (dh < -180) dh += 360;
  const dH = 2 * Math.sqrt(c1 * c2) * Math.sin(dh / 2 * rad);
  const Lm = (L1 + L2) / 2, cm = (c1 + c2) / 2;
  let hm = t1 + t2; if (c1 * c2 !== 0) hm = Math.abs(t1 - t2) <= 180 ? hm / 2 : (hm + (hm < 360 ? 360 : -360)) / 2;
  const T = 1 - 0.17 * Math.cos((hm - 30) * rad) + 0.24 * Math.cos(2 * hm * rad) + 0.32 * Math.cos((3 * hm + 6) * rad) - 0.2 * Math.cos((4 * hm - 63) * rad);
  const SL = 1 + 0.015 * (Lm - 50) ** 2 / Math.sqrt(20 + (Lm - 50) ** 2), SC = 1 + 0.045 * cm, SH = 1 + 0.015 * cm * T;
  const RT = -2 * Math.sqrt(cm ** 7 / (cm ** 7 + 25 ** 7)) * Math.sin(60 * Math.exp(-(((hm - 275) / 25) ** 2)) * rad);
  return Math.sqrt((dL / SL) ** 2 + (dC / SC) ** 2 + (dH / SH) ** 2 + RT * (dC / SC) * (dH / SH));
}
const enc = (l) => Math.round(255 * Math.min(1, l <= 0.0031308 ? 12.92 * l : 1.055 * l ** (1 / 2.4) - 0.055));   // linéaire → sRGB (IEC 61966-2-1)
const luminance = (r, g, b) => 0.2126 * LIN[r] + 0.7152 * LIN[g] + 0.0722 * LIN[b];
// Référence d'une surface (conseil de l'expert réalisme 3D) : moyenne tronquée 10-90 % (rangée par luminance), en linéaire,
// rendue en sRGB 0-255 : c'est ce que le téléphone voit de loin (il moyenne les texels).
function moyenneTronquee(px, indices) {
  const tri = Array.from(indices).sort((i, j) => luminance(px[i], px[i + 1], px[i + 2]) - luminance(px[j], px[j + 1], px[j + 2]));
  const d = Math.floor(tri.length * 0.1), f = Math.max(d + 1, Math.ceil(tri.length * 0.9)), s = [0, 0, 0];
  for (let k = d; k < f; k++) for (let c = 0; c < 3; c++) s[c] += LIN[px[tri[k] + c]];
  return s.map((v) => enc(v / (f - d)));
}
const quantile = (a, q) => { const s = Float64Array.from(a).sort(); return s.length ? s[Math.min(s.length - 1, Math.floor(q * s.length))] : NaN; };
const couleurMateriau = (w, k) => [...new Float32Array(w.materiaux.buffer.slice(w.materiaux.byteOffset + 56 * k, w.materiaux.byteOffset + 56 * k + 12))];
const textureCouleur = (w, k) => new Int32Array(w.materiaux.buffer.slice(w.materiaux.byteOffset + 56 * k + 40, w.materiaux.byteOffset + 56 * k + 44))[0];
async function pixels(blob) {
  const { data, info } = await sharp(blob).ensureAlpha().raw().toBuffer({ resolveWithObject: true });
  return { W: info.width, H: info.height, px: new Uint8Array(data.buffer, data.byteOffset, data.length) };
}
// zone (rect / disque en uv, v vers le bas) : le pixel (x, y) est-il dedans, élargie de « marge » texels ?
const dansZone = (W, H, x, y, z, marge) => {
  const u = (x + 0.5) / W, v = (y + 0.5) / H, mu = marge / W, mv = marge / H;
  if (z.rect) { const [a, b, c, d] = z.rect; return u >= a - mu && u <= c + mu && v >= b - mv && v <= d + mv; }
  if (z.anneau) { const [cu, cv, r0, r1] = z.anneau, d = Math.hypot(u - cu, v - cv), m = Math.max(mu, mv); return d >= r0 - m && d <= r1 + m; }   // flanc de pneu
  const [cu, cv, r] = z.disque; return Math.hypot(u - cu, v - cv) <= r + Math.max(mu, mv);
};
const BORD = 2;                 // la zone repeinte déborde de 2 texels (aucun liseré du logo ne reste)
const ANNEAU = (W) => Math.max(8, Math.round(0.03 * W));   // anneau de référence : 3 % de la texture au-delà du bord
// texels (indices × 4) d'une bande [de, a[ texels autour de la zone, limitée à la boîte de la zone élargie
function bande(W, H, z, de, a) {
  const out = [];
  for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) if (dansZone(W, H, x, y, z, a) && !dansZone(W, H, x, y, z, de)) out.push((y * W + x) * 4);
  return out;
}
// texels de référence d'une zone : « reference » (zone, ou [u0, v0, u1, v1]) = la surface de la même pièce dans un atlas, hors de la
// zone ; sinon l'anneau de 3 % autour (qui, dans un atlas, mêle d'autres pièces)
const referenceZone = (W, H, z) => z.reference
  ? dedans(W, H, Array.isArray(z.reference) ? { rect: z.reference } : z.reference, 0).filter((i) => { const q = i / 4, x = q % W; return !dansZone(W, H, x, (q - x) / W, z, BORD); })
  : bande(W, H, z, BORD, BORD + ANNEAU(W));
const couleurZone = (t, z) => z.couleur ?? t.couleur ?? 'auto';
const dedans = (W, H, z, marge) => { const out = []; for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) if (dansZone(W, H, x, y, z, marge)) out.push((y * W + x) * 4); return out; };
// grain de l'anneau : écart type de L* une fois retirée la moyenne locale 9 × 9 (uni suffisant si σ ≤ 1,5)
function grain(P, idx) {
  const { W, H, px } = P, L = new Float64Array(W * H);
  for (let i = 0; i < W * H; i++) L[i] = versLab(px[i * 4], px[i * 4 + 1], px[i * 4 + 2])[0];
  const S = new Float64Array((W + 1) * (H + 1));   // image intégrale
  for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) S[(y + 1) * (W + 1) + x + 1] = L[y * W + x] + S[y * (W + 1) + x + 1] + S[(y + 1) * (W + 1) + x] - S[y * (W + 1) + x];
  const r = [];
  for (const i4 of idx) {
    const i = i4 / 4, x = i % W, y = (i - x) / W, x0 = Math.max(0, x - 4), x1 = Math.min(W, x + 5), y0 = Math.max(0, y - 4), y1 = Math.min(H, y + 5);
    const m = (S[y1 * (W + 1) + x1] - S[y0 * (W + 1) + x1] - S[y1 * (W + 1) + x0] + S[y0 * (W + 1) + x0]) / ((x1 - x0) * (y1 - y0));
    r.push(L[i] - m);
  }
  const moy = r.reduce((s, v) => s + v, 0) / r.length;
  return Math.sqrt(r.reduce((s, v) => s + (v - moy) ** 2, 0) / r.length);
}
// Écarts après retouche, sur les textures décodées (la texture réencodée est celle que lit le jeu) :
// (1) chaque zone (moyenne tronquée de ses texels) ↔ l'anneau autour dans la texture d'origine (ΔE2000 ≤ 2), grain de l'anneau (σ),
//     liseré : écart maximal de la bande de 4 texels juste autour ;
// (2) RG-07 (a) : ΔE2000 pixel à pixel origine ↔ réencodée, hors des zones et d'une bande de 8 texels : moyenne (≤ 0,5) et 99e centile (≤ 3) ;
// (3) « comparer » : [{ "materiau": a, "voisin": b }] : albédo de la zone vue par le matériau a (sa couleur de base × la zone) ↔ albédo du
//     matériau b (sa couleur de base × moyenne tronquée de sa texture de couleur) : pour une zone sans anneau (texture entière).
// L'albédo de (1) et (2) est pris avec la couleur de base du matériau « materiau » de la texture.
async function mesurer(w, t, avant, apres) {
  const A = await pixels(avant), B = await pixels(apres), { W, H } = A, f = couleurMateriau(w, t.materiau ?? 0);
  const sorties = [], alertes = [];
  for (const [n, z] of t.zones.entries()) {
    const nom = `zone ${n + 1}`;
    if (z.transparent) { sorties.push(`${nom} : transparente (décalque retiré)`); continue; }
    const zb = moyenneTronquee(B.px, dedans(W, H, z, BORD)), an = referenceZone(W, H, z);
    if (an.length) {
      const d = deltaE2000(versLab(...zb, f), versLab(...moyenneTronquee(A.px, an), f)), g = grain(A, an);
      const bord = bande(W, H, z, BORD, BORD + 4), lz = versLab(...zb, f);
      let lis = 0; for (const i of bord) lis = Math.max(lis, deltaE2000(lz, versLab(B.px[i], B.px[i + 1], B.px[i + 2], f)));
      // une couleur donnée sans « reference » a été mesurée à la main sur la pièce voisine (notes de logos.json) : l'anneau, qui dans un
      // atlas ou sur un flanc de pneu mêle d'autres pièces ou des lettres, n'est alors qu'une information
      const juge = couleurZone(t, z) === 'auto' || z.reference;
      sorties.push(`${nom} ↔ ${z.reference ? 'référence' : 'anneau'} ΔE2000 ${d.toFixed(2)}${juge ? '' : ' (pour information : couleur mesurée sur la pièce voisine)'} (grain σ ${g.toFixed(2)}, liseré max ${lis.toFixed(1)})`);
      if (juge && d > 2) alertes.push(`${nom} : ΔE2000 ${d.toFixed(2)} > 2`);
      if (juge && g > 1.5) alertes.push(`${nom} : grain σ ${g.toFixed(2)} > 1,5 (l'uni ferait une tache lisse)`);
    }
    for (const c of t.comparer || []) {
      const tv = textureCouleur(w, c.voisin);
      let rgb = [255, 255, 255];
      if (tv >= 0) { const V = await pixels(w.textures[tv]); rgb = moyenneTronquee(V.px, Array.from({ length: V.W * V.H }, (_, i) => i * 4)); }
      const d = deltaE2000(versLab(...zb, couleurMateriau(w, c.materiau)), versLab(...rgb, couleurMateriau(w, c.voisin)));
      sorties.push(`${nom} (matériau ${c.materiau}) ↔ matériau ${c.voisin} ΔE2000 ${d.toFixed(2)}`);
      if (d > 2) alertes.push(`${nom} (matériau ${c.materiau}) ↔ ${c.voisin} : ΔE2000 ${d.toFixed(2)} > 2`);
    }
  }
  const ecarts = [];   // RG-07 (a) : pertes du réencodage hors des zones (tous les pixels)
  for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) {
    if (t.zones.some((z) => dansZone(W, H, x, y, z, BORD + 8))) continue;
    const i = (y * W + x) * 4;
    ecarts.push(deltaE2000(versLab(A.px[i], A.px[i + 1], A.px[i + 2], f), versLab(B.px[i], B.px[i + 1], B.px[i + 2], f)));
  }
  if (ecarts.length) {
    const moy = ecarts.reduce((s, v) => s + v, 0) / ecarts.length, p99 = quantile(ecarts, 0.99);
    sorties.push(`hors zones ΔE2000 moyen ${moy.toFixed(3)}, 99e centile ${p99.toFixed(2)} (${ecarts.length} pixels)`);
    if (moy > 0.5 || p99 > 3) alertes.push(`hors zones : moyenne ${moy.toFixed(3)} / 99e centile ${p99.toFixed(2)} (≤ 0,5 / ≤ 3 attendus)`);
  } else sorties.push('hors zones : aucun pixel (texture entière)');
  return { texte: sorties.join(', '), alertes };
}

// ---------- retouche d'une texture ----------
// couleur « auto » : moyenne tronquée de l'anneau autour de la zone (pièce voisine), seulement s'il est assez grand (≥ 200 texels)
// et homogène (écart interquartile de L* ≤ 3, médiane ↔ moyenne tronquée ΔE2000 ≤ 1) ; sinon arrêt : la couleur est à donner dans logos.json.
async function retoucher(blob, t) {
  const img = sharp(blob), meta = await img.metadata();
  const { data, info } = await img.ensureAlpha().raw().toBuffer({ resolveWithObject: true });
  const W = info.width, H = info.height, px = new Uint8Array(data.buffer, data.byteOffset, data.length);
  for (const [n, z] of t.zones.entries()) {
    let couleur; const cz = couleurZone(t, z), auto = cz === 'auto' && t.role !== 'normale';
    if (cz !== 'auto') { const h = parseInt(cz.slice(1), 16); couleur = [h >> 16 & 255, h >> 8 & 255, h & 255, 255]; }   // normale : normale moyenne du fond
    else if (t.role === 'normale') couleur = [128, 128, 255, 255];
    else {
      const an = referenceZone(W, H, z);
      if (an.length < 200) throw new Error(`texture ${t.rang}, zone ${n + 1} : référence de ${an.length} texels (≥ 200) : couleur à donner`);
      const Ls = an.map((i) => versLab(px[i], px[i + 1], px[i + 2])[0]), iqr = quantile(Ls, 0.75) - quantile(Ls, 0.25);
      const mt = moyenneTronquee(px, an), med = [0, 1, 2].map((c) => Math.round(quantile(an.map((i) => px[i + c]), 0.5)));
      const dm = deltaE2000(versLab(...mt), versLab(...med));
      if (iqr > 3 || dm > 1) throw new Error(`texture ${t.rang}, zone ${n + 1} : référence hétérogène (écart interquartile L* ${iqr.toFixed(1)}, médiane ↔ moyenne ΔE2000 ${dm.toFixed(2)}) : couleur à donner`);
      couleur = [...mt, Math.round(quantile(an.map((i) => px[i + 3]), 0.5))];   // alpha aussi : lettres opaques sur un fond transparent
    }
    // « transparent » (décalque d'un matériau à masque alpha) : alpha 0, le décalque disparaît ; « alpha » de la texture : valeur imposée
    if (z.transparent && !meta.hasAlpha) throw new Error(`texture ${t.rang}, zone ${n + 1} : « transparent » sans canal alpha`);
    const alpha = (i) => (!meta.hasAlpha ? 255 : z.transparent ? 0 : t.alpha !== undefined ? t.alpha : auto ? couleur[3] : px[i + 3]);
    for (const i of dedans(W, H, z, BORD)) { px[i + 3] = alpha(i); for (let c = 0; c < 3; c++) px[i + c] = couleur[c]; }
  }
  const s = sharp(Buffer.from(px.buffer, px.byteOffset, px.length), { raw: { width: W, height: H, channels: 4 } });
  const png = blob[0] === 0x89;
  return png ? (meta.hasAlpha ? s.png({ compressionLevel: 9 }) : s.removeAlpha().png({ compressionLevel: 9 })).toBuffer()
             : s.removeAlpha().jpeg({ quality: 88 }).toBuffer();
}

// ---------- une voiture ----------
async function traiter(id) {
  if (!/^[a-z0-9]+$/.test(id) || !LOGOS[id]) throw new Error(`voiture « ${id} » absente de logos.json`);
  const L = LOGOS[id], fichier = path.join(RACINE, 'assets/cars', `${id}.wfm`);
  const avant = fs.readFileSync(fichier), w = lire(avant);
  // WFS-193 : tools/retoucher_formes.mjs ajoute ses matériaux (grilles génériques) à la FIN, après ceux-ci : rang des matériaux inchangé
  if (w.nM < L.materiaux) throw new Error(`${w.nM} matériaux, ${L.materiaux} attendus`);
  const journal = [];
  // contrôles d'abord (rien n'est écrit si une seule retouche ne correspond pas)
  const texAFaire = [];
  for (const t of L.textures || []) {
    if (t.rang >= w.textures.length) throw new Error(`texture ${t.rang} absente`);
    const e = empreinte(w.textures[t.rang]);
    if (e === t.sha256Apres) { journal.push(`texture ${t.rang} : déjà faite`); continue; }
    if (e !== t.sha256) throw new Error(`texture ${t.rang} : empreinte ${e}, ${t.sha256} attendue (ou ${t.sha256Apres} après)`);
    texAFaire.push(t);
  }
  const parMateriau = new Map();
  for (const p of L.pieces || []) {
    if (!parMateriau.has(p.materiau)) parMateriau.set(p.materiau, []);
    parMateriau.get(p.materiau).push(p);
  }
  const mailAFaire = [];
  for (const [k, ps] of parMateriau) {
    const lesquels = w.maillages.map((m, i) => (m.mat === k ? i : -1)).filter((i) => i >= 0);
    if (lesquels.length > 1) throw new Error(`matériau ${k} : ${lesquels.length} maillages (un seul attendu)`);
    if (ps.some((p) => p.trianglesAvant !== ps[0].trianglesAvant)) throw new Error(`matériau ${k} : trianglesAvant différents`);
    const retires = ps.reduce((s, p) => s + p.triangles[1], 0);
    if (!lesquels.length) {   // maillage vidé et retiré à une passe précédente
      if (retires === ps[0].trianglesAvant) { journal.push(`matériau ${k} : déjà fait (maillage retiré)`); continue; }
      throw new Error(`matériau ${k} : aucun maillage`);
    }
    const mi = lesquels[0], m = w.maillages[mi], tri = m.ix.length / 3;
    if (tri === ps[0].trianglesAvant - retires) { journal.push(`matériau ${k} : déjà fait`); continue; }
    if (tri !== ps[0].trianglesAvant) throw new Error(`matériau ${k} : ${tri} triangles, ${ps[0].trianglesAvant} attendus`);
    for (const p of ps) {
      const b = boiteTriangles(m, p.triangles[0], p.triangles[1]);
      if (b.some((x, k) => Math.abs(x - p.boite[k]) > 0.01)) throw new Error(`pièce « ${p.nom} » : boîte ${b.map((x) => x.toFixed(3))}, ${p.boite} attendue`);
    }
    mailAFaire.push([mi, ps]);
  }
  // RG-12 (décision du PO du 2026-11-08) : matériau qui ne sert qu'à un logo et dont la texture est partagée avec une autre pièce
  // (M4 : centre du volant et caches des roues) : sa texture de couleur est retirée (-1) et sa couleur de base devient celle de la pièce
  // voisine ; métal, rugosité, autres textures, nombre et ordre des matériaux inchangés. « avant » / « apres » : { couleur: [r, g, b], tC }.
  const lireMat = (k) => ({ couleur: couleurMateriau(w, k), tC: textureCouleur(w, k) });
  const egalMat = (a, b) => a.tC === b.tC && a.couleur.every((x, i) => Math.abs(x - b.couleur[i]) < 1e-6);
  const matAFaire = [];
  for (const m of L.materiauxRetouches || []) {
    if (m.rang >= w.nM) throw new Error(`matériau ${m.rang} absent`);
    const c = lireMat(m.rang);
    if (egalMat(c, m.apres)) { journal.push(`matériau ${m.rang} (couleur) : déjà fait`); continue; }
    if (!egalMat(c, m.avant)) throw new Error(`matériau ${m.rang} : couleur ${c.couleur.map((x) => x.toFixed(4))}, texture ${c.tC} ; attendu ${JSON.stringify(m.avant)} (ou après)`);
    matAFaire.push(m);
  }
  if (!texAFaire.length && !mailAFaire.length && !matAFaire.length) return { id, change: false, journal: [...journal, 'rien à faire'] };
  if (verifier) return { id, change: false, journal: [...journal, `à faire : ${texAFaire.length} texture(s), ${mailAFaire.length} maillage(s), ${matAFaire.length} matériau(x)`] };
  if (matAFaire.length) {
    w.materiaux = Buffer.from(w.materiaux);   // copie : le tampon lu reste l'original
    for (const m of matAFaire) {
      const o = 56 * m.rang;
      m.apres.couleur.forEach((x, i) => w.materiaux.writeFloatLE(x, o + 4 * i));
      w.materiaux.writeInt32LE(m.apres.tC, o + 40);
      const v = m.voisin === undefined ? '' : textureCouleur(w, m.voisin) >= 0 ? ` ; matériau ${m.voisin} texturé : écart non mesuré`
        : ` ; albédo ↔ matériau ${m.voisin} ΔE2000 ${deltaE2000(versLab(255, 255, 255, couleurMateriau(w, m.rang)), versLab(255, 255, 255, couleurMateriau(w, m.voisin))).toFixed(2)}`;
      journal.push(`matériau ${m.rang} : texture de couleur ${m.avant.tC} → ${m.apres.tC}, couleur de base ${m.avant.couleur.map((x) => x.toFixed(3))} → ${m.apres.couleur.map((x) => x.toFixed(3))}${v}`);
    }
  }
  for (const t of texAFaire) {
    const nouveau = await retoucher(w.textures[t.rang], t);
    const m = t.role === 'couleur' ? await mesurer(w, t, w.textures[t.rang], nouveau) : null;
    journal.push(`texture ${t.rang} (${t.role}) : ${t.zones.length} zone(s) repeinte(s), ${w.textures[t.rang].length} → ${nouveau.length} octets, empreinte ${empreinte(nouveau)}${m ? ` ; ${m.texte}` : ''}`);
    for (const a of m?.alertes || []) journal.push(`⚠ ${a}`);
    w.textures[t.rang] = nouveau;
  }
  const vides = [];
  for (const [mi, ps] of mailAFaire) {
    const m = w.maillages[mi], garde = new Uint8Array(m.ix.length / 3).fill(1);
    for (const p of ps) for (let t = p.triangles[0]; t < p.triangles[0] + p.triangles[1]; t++) garde[t] = 0;
    const ix = []; for (let t = 0; t < garde.length; t++) if (garde[t]) ix.push(m.ix[t * 3], m.ix[t * 3 + 1], m.ix[t * 3 + 2]);
    journal.push(`maillage ${mi} (matériau ${m.mat}) : ${ps.map((p) => p.nom).join(', ')} retiré(s), ${garde.length} → ${ix.length / 3} triangles`);
    m.ix = new Uint32Array(ix);
    if (!ix.length) vides.push(mi);
  }
  // maillage vidé : retiré du fichier avec ses tangentes (plus d'appel de dessin) ; son matériau reste à son rang
  for (const mi of vides.sort((a, b) => b - a)) { w.maillages.splice(mi, 1); if (w.tan) w.tan.splice(mi, 1); journal.push(`maillage ${mi} vide : retiré du fichier`); }
  fs.writeFileSync(fichier, ecrire(w));
  if (mailAFaire.length) {   // RG-07 : .lod régénéré seulement si la voiture perd des triangles
    execFileSync('node', [path.join(ICI, 'alleger_voiture.mjs'), fichier, path.join(RACINE, 'assets/cars', `${id}.lod`)], { stdio: 'inherit' });
    journal.push('.lod régénéré (alleger_voiture.mjs)');
  }
  journal.push(`${id}.wfm : ${avant.length} → ${fs.statSync(fichier).size} octets, empreinte ${empreinte(fs.readFileSync(fichier))}`);
  return { id, change: true, journal };
}

let erreurs = 0;
for (const id of choix.length ? choix : Object.keys(LOGOS)) {
  try { const r = await traiter(id); console.log(`${id} : ${r.journal.join(' ; ')}`); }
  catch (e) { erreurs++; console.error(`${id} : ARRÊT, rien n'est écrit : ${e.message}`); }
}
process.exit(erreurs ? 1 : 0);
