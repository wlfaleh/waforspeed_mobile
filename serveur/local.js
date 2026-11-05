// WaForSpeed — relais du « Duel entre amis » en local (WFS-112, RG-07) : même logique que le Worker (src/salon.js),
// servie par Node sans bibliothèque. Pour les tests, le banc d'essai et un essai sur le réseau de la maison.
// usage : node serveur/local.js [port] [--expiration ms] [--grace ms] [--parent]
//   port 0 : port libre choisi par le système ; la ligne « RELAIS PORT <n> » est écrite sur la sortie quand il écoute.
import http from 'node:http';
import { Salon, analyserAdresse, pageInvitation } from './src/salon.js';
import { accepter } from './src/ws_mini.js';

const args = process.argv.slice(2);
const option = (nom, def) => { const i = args.indexOf(nom); return i >= 0 ? Number(args[i + 1]) : def; };
const port = args[0] && !args[0].startsWith('--') ? Number(args[0]) : 8787;
const options = { expirationMs: option('--expiration', undefined), graceMs: option('--grace', undefined) };
const salons = new Map();

const serveur = http.createServer((req, res) => {
  const url = new URL(req.url, 'http://local');
  const inv = /^\/d\/([A-Za-z0-9]{4})$/.exec(url.pathname);
  if (inv) { res.writeHead(200, { 'content-type': 'text/html; charset=utf-8' }); return res.end(pageInvitation(inv[1].toLowerCase())); }
  res.writeHead(url.pathname === '/' ? 200 : 404, { 'content-type': 'text/plain; charset=utf-8' });
  res.end(url.pathname === '/' ? 'relais WaForSpeed v1 (local)\n' : 'adresse invalide\n');
});

serveur.on('upgrade', (req, prise) => {
  prise.on('error', () => {});
  let a = null;
  try { a = analyserAdresse(new URL(req.url, 'http://local')); } catch { a = null; }
  if (!a) { prise.end('HTTP/1.1 404 Not Found\r\n\r\n'); return; }
  let s = salons.get(a.code);
  if (!s) { s = new Salon(a.code, options); salons.set(a.code, s); }
  let socket = null;
  socket = accepter(req, prise, {
    texte: (t) => s.recevoir(a.id, socket, t, Date.now()),
    fin: () => s.deconnecter(a.id, socket, Date.now()),
  });
  if (!socket) return;
  const r = s.connecter(socket, a.id, a.hote, Date.now());
  if (!r.ok) { socket.envoyer(`E|${r.raison}`); socket.fermer(); }
});

setInterval(() => {
  const t = Date.now();
  for (const [code, s] of salons) { s.tick(t); if (s.vide()) salons.delete(code); }
}, 250).unref();

serveur.listen(port, '127.0.0.1', () => {
  process.stdout.write(`RELAIS PORT ${serveur.address().port}\n`);
});
// --parent : arrêt quand le processus parent (test C++, banc) ferme l'entrée ; SIGTERM : arrêt
if (args.includes('--parent')) { process.stdin.on('end', () => process.exit(0)); process.stdin.on('error', () => process.exit(0)); process.stdin.resume(); }
process.on('SIGTERM', () => process.exit(0));
