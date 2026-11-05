// WaForSpeed — WebSocket minimale (RFC 6455, messages texte) pour le relais local et les tests (WFS-112), sans
// bibliothèque : modules http, net et crypto de Node. Sur Cloudflare, c'est WebSocketPair qui fait ce travail.
import crypto from 'node:crypto';
import net from 'node:net';

const GUID = '258EAFA5-E914-47DA-95CA-C5AB0DC85B11';
export const TRAME_MAX = 4096;

export function cleAcceptation(cle) { return crypto.createHash('sha1').update(cle + GUID).digest('base64'); }

export function trame(opcode, donnees, masque) {
  const p = Buffer.isBuffer(donnees) ? donnees : Buffer.from(donnees, 'utf8');
  const n = p.length, tete = [0x80 | opcode];
  const m = masque ? 0x80 : 0;
  if (n < 126) tete.push(m | n);
  else if (n < 65536) tete.push(m | 126, n >> 8, n & 255);
  else { tete.push(m | 127, 0, 0, 0, 0, (n >>> 24) & 255, (n >>> 16) & 255, (n >>> 8) & 255, n & 255); }
  if (!masque) return Buffer.concat([Buffer.from(tete), p]);
  const k = crypto.randomBytes(4), q = Buffer.alloc(n);
  for (let i = 0; i < n; i++) q[i] = p[i] ^ k[i & 3];
  return Buffer.concat([Buffer.from(tete), k, q]);
}

// lecteur de trames : rappels texte(t), fermeture(), erreur(raison) ; masqueAttendu : le serveur exige le masque du client
export function lecteur(masqueAttendu, rappels) {
  let tampon = Buffer.alloc(0), morceaux = [], fini = false;
  const echec = (r) => { if (!fini) { fini = true; rappels.erreur(r); } };
  return (octets) => {
    if (fini) return;
    tampon = Buffer.concat([tampon, octets]);
    for (;;) {
      if (tampon.length < 2) return;
      const b0 = tampon[0], b1 = tampon[1], op = b0 & 15, fin = (b0 & 0x80) !== 0, masque = (b1 & 0x80) !== 0;
      if (b0 & 0x70) return echec('bits réservés');
      if (masque !== masqueAttendu) return echec('masque');
      let n = b1 & 127, i = 2;
      if (n === 126) { if (tampon.length < 4) return; n = tampon.readUInt16BE(2); i = 4; }
      else if (n === 127) { if (tampon.length < 10) return; if (tampon.readUInt32BE(2) !== 0) return echec('trop long'); n = tampon.readUInt32BE(6); i = 10; }
      if (n > TRAME_MAX) return echec('trop long');
      const k = masque ? i : -1; if (masque) i += 4;
      if (tampon.length < i + n) return;
      const p = Buffer.from(tampon.subarray(i, i + n));
      if (masque) for (let j = 0; j < n; j++) p[j] ^= tampon[k + (j & 3)];
      tampon = tampon.subarray(i + n);
      if (op === 8) { fini = true; rappels.fermeture(); return; }
      if (op === 9) { rappels.ping && rappels.ping(p); continue; }
      if (op === 10) continue;
      if (op !== 0 && op !== 1) return echec('opcode ' + op);
      morceaux.push(p);
      if (morceaux.reduce((s, x) => s + x.length, 0) > TRAME_MAX) return echec('trop long');
      if (fin) { const t = Buffer.concat(morceaux).toString('utf8'); morceaux = []; rappels.texte(t); }
    }
  };
}

// côté serveur : poignée de main d'une requête « upgrade » de http.Server, puis rappels
export function accepter(requete, prise, rappels) {
  const cle = requete.headers['sec-websocket-key'];
  if (!cle || (requete.headers['upgrade'] || '').toLowerCase() !== 'websocket') { prise.end('HTTP/1.1 400 Bad Request\r\n\r\n'); return null; }
  prise.write(`HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ${cleAcceptation(cle)}\r\n\r\n`);
  prise.setNoDelay(true);
  let ferme = false;
  const ws = {
    envoyer: (t) => { if (!ferme) prise.write(trame(1, t, false)); },
    fermer: () => { if (ferme) return; ferme = true; try { prise.end(trame(8, Buffer.from([3, 232]), false)); } catch { /* coupée */ } },
  };
  const lire = lecteur(true, {
    texte: (t) => rappels.texte(t),
    ping: (p) => { if (!ferme) prise.write(trame(10, p, false)); },
    fermeture: () => ws.fermer(),
    erreur: () => { ws.fermer(); prise.destroy(); },
  });
  prise.on('data', lire);
  const fin = () => { if (!ws.fini) { ws.fini = true; ferme = true; rappels.fin(); } };
  prise.on('close', fin); prise.on('error', fin);
  return ws;
}

// côté client (tests) : connexion à ws://hote:port/chemin ; renvoie une promesse { envoyer, fermer, couper, messages, attendre }
export function connecter(hote, port, chemin) {
  return new Promise((ok, ko) => {
    const p = net.connect(port, hote);
    const cle = crypto.randomBytes(16).toString('base64');
    const messages = [], attentes = [];
    let tete = Buffer.alloc(0), ouvert = false, ferme = false;
    const client = {
      messages, ferme: () => ferme,
      envoyer: (t) => p.write(trame(1, t, true)),
      brut: (o) => p.write(o),
      fermer: () => { try { p.end(trame(8, Buffer.alloc(0), true)); } catch { /* coupée */ } },
      couper: () => p.destroy(),
      // attend un message qui vérifie f (délai en ms)
      attendre: (f, delai = 3000) => new Promise((r, e) => {
        const m = messages.find(f); if (m) return r(m);
        const a = { f, r }; attentes.push(a);
        setTimeout(() => { const i = attentes.indexOf(a); if (i >= 0) { attentes.splice(i, 1); e(new Error('délai dépassé')); } }, delai);
      }),
    };
    const lire = lecteur(false, {
      texte: (t) => { messages.push(t); for (const a of [...attentes]) if (a.f(t)) { attentes.splice(attentes.indexOf(a), 1); a.r(t); } },
      fermeture: () => { ferme = true; p.end(); },
      erreur: () => { ferme = true; p.destroy(); },
    });
    p.on('connect', () => p.write(`GET ${chemin} HTTP/1.1\r\nHost: ${hote}:${port}\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: ${cle}\r\nSec-WebSocket-Version: 13\r\n\r\n`));
    p.on('data', (o) => {
      if (ouvert) return lire(o);
      tete = Buffer.concat([tete, o]);
      const fin = tete.indexOf('\r\n\r\n');
      if (fin < 0) return;
      const texte = tete.subarray(0, fin).toString();
      if (!texte.startsWith('HTTP/1.1 101') || !texte.includes(cleAcceptation(cle))) { p.destroy(); return ko(new Error('poignée refusée')); }
      ouvert = true; ok(client);
      if (tete.length > fin + 4) lire(tete.subarray(fin + 4));
    });
    p.on('close', () => { ferme = true; });
    p.on('error', (e) => { ferme = true; if (!ouvert) ko(e); });
  });
}
