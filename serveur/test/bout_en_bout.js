// WaForSpeed — test de bout en bout du relais (WFS-112, RG-07) : relais local lancé (node local.js), deux clients
// simulés qui créent et rejoignent un salon, échangent leurs présences, se reconnectent ; messages hostiles sans
// plantage ; code inconnu, salon plein, code expiré.
// usage : node serveur/test/bout_en_bout.js   (code de sortie 0 = tout est bon)
import { spawn } from 'node:child_process';
import net from 'node:net';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { connecter, trame } from '../src/ws_mini.js';

const ici = path.dirname(fileURLToPath(import.meta.url));
let echecs = 0;
const verifie = (ok, quoi) => { console.log(`${ok ? 'OK    ' : 'ÉCHEC '} ${quoi}`); if (!ok) echecs++; };
const pause = (ms) => new Promise((r) => setTimeout(r, ms));

// relais avec un code qui expire en 3 s et une place gardée 0,8 s (durées réelles : 15 min et 30 s)
const relais = spawn(process.execPath, [path.join(ici, '..', 'local.js'), '0', '--expiration', '3000', '--grace', '800'], { stdio: ['ignore', 'pipe', 'inherit'] });
const port = await new Promise((ok, ko) => {
  let s = '';
  relais.stdout.on('data', (o) => { s += o; const m = /RELAIS PORT (\d+)/.exec(s); if (m) ok(Number(m[1])); });
  relais.on('exit', () => ko(new Error('relais arrêté')));
  setTimeout(() => ko(new Error('relais muet')), 5000);
});
const ws = (code, id, hote) => connecter('127.0.0.1', port, `/salon/${code}?id=${id}&hote=${hote ? 1 : 0}&v=1`);

try {
  // 1) code inconnu : personne ne l'a créé
  const perdu = await ws('zz99', 'perdu123', false);
  verifie(await perdu.attendre((m) => m === 'E|inconnu').then(() => true, () => false), 'code inconnu → E|inconnu');

  // 2) Alice crée, Bob rejoint ; présences horodatées par le relais
  const t0 = Date.now();
  const alice = await ws('k7qx', 'alice123', true);
  await alice.attendre((m) => m.startsWith('OK|'));
  alice.envoyer('P|n=Alice;st=lobby;host=1;km=2');
  await pause(50);
  const bob = await ws('K7QX', 'bob45678', false);
  const vuAlice = await bob.attendre((m) => m.startsWith('P|alice123|'));
  verifie(/^P\|alice123\|\d+\|n=Alice;st=lobby;host=1;km=2$/.test(vuAlice), 'Bob reçoit la présence d\'Alice en arrivant');
  bob.envoyer('P|n=Bob;st=ready');
  verifie(await alice.attendre((m) => m.startsWith('P|bob45678|') && m.endsWith('n=Bob;st=ready')).then(() => true, () => false), 'Alice reçoit la présence de Bob');
  bob.envoyer('ping|123.5');
  verifie(await bob.attendre((m) => /^pong\|123\.5\|\d+$/.test(m)).then(() => true, () => false), 'ping → pong avec l\'horloge du relais');

  // 3) messages hostiles : rien ne casse, rien n'est relayé
  const avant = alice.messages.length;
  bob.envoyer('P|n=X|Y');                  // « | » interdit
  bob.envoyer('P|' + 'a'.repeat(700));     // trop long
  bob.envoyer('P|n=\u0001');               // caractère de contrôle
  bob.envoyer('ping|abc');                 // horloge invalide
  bob.envoyer('chat|salut');               // pas de chat
  bob.envoyer('\u0000ÿ');
  for (let i = 0; i < 60; i++) bob.envoyer('ping|1');   // débit dépassé (40 / s)
  await pause(150);
  verifie(alice.messages.length === avant, 'messages hostiles de Bob : rien relayé à Alice');
  const pirate = await ws('k7qx', 'pirate99', false);   // trame non masquée, opcode inconnu, longueur énorme
  pirate.brut(Buffer.from([0x81, 0x05, 0x61, 0x62, 0x63, 0x64, 0x65]));
  const pirate2 = await ws('k7qx', 'pirate98', false);
  pirate2.brut(Buffer.from([0x82, 0xff, 0, 0, 0, 1, 0, 0, 0, 0]));
  const brute = net.connect(port, '127.0.0.1'); brute.on('error', () => {});
  brute.write('GET /salon/k7qx HTTP/1.1\r\n\r\n\u0000\u0001garbage');
  await pause(200);
  verifie(relais.exitCode === null, 'relais toujours en marche après les messages hostiles');
  verifie(pirate.ferme() && pirate2.ferme(), 'trames invalides : connexion fermée par le relais');
  await pause(1000);   // fenêtre du débit passée
  bob.envoyer('P|n=Bob;st=race;d=120');
  verifie(await alice.attendre((m) => m.endsWith('st=race;d=120')).then(() => true, () => false), 'Bob joue toujours après ses messages hostiles');

  // 4) salon plein (4 joueurs) : les pirates, coupés, gardent leur place 0,8 s puis sont retirés
  const c3 = await ws('k7qx', 'carla123', false), c4 = await ws('k7qx', 'dani1234', false);
  await pause(50);
  const c5 = await ws('k7qx', 'emile123', false);
  verifie(await c5.attendre((m) => m === 'E|plein').then(() => true, () => false), '5e joueur → E|plein');
  const hote2 = await ws('k7qx', 'autrehote', true);
  verifie(await hote2.attendre((m) => m === 'E|pris' || m === 'E|plein').then(() => true, () => false), 'second créateur sur le même code → refusé');
  c3.fermer(); c4.fermer();

  // 5) reconnexion (RG-05) : Bob est coupé net, revient avec le même identifiant et retrouve Alice
  bob.couper();
  await pause(100);
  const bob2 = await ws('k7qx', 'bob45678', false);
  verifie(await bob2.attendre((m) => m.startsWith('P|alice123|')).then(() => true, () => false), 'Bob reconnecté retrouve la présence d\'Alice');
  await pause(1000);   // plus que la place gardée (0,8 s) : Bob n'a pas été retiré, il était revenu à temps
  verifie(!alice.messages.includes('B|bob45678'), 'Bob revenu à temps : jamais annoncé parti');
  verifie(alice.messages.includes('B|carla123'), 'Carla partie : annoncée partie (B|carla123)');

  // 6) code expiré (3 s ici, 15 min en vrai) : les nouveaux venus sont refusés, les joueurs déjà là continuent
  await pause(Math.max(0, t0 + 3200 - Date.now()));
  const tard = await ws('k7qx', 'tard1234', false);
  verifie(await tard.attendre((m) => m === 'E|expire').then(() => true, () => false), 'nouveau venu après l\'expiration → E|expire');
  const bob3 = await ws('k7qx', 'bob45678', false);
  verifie(await bob3.attendre((m) => m.startsWith('OK|')).then(() => true, () => false), 'reconnexion acceptée même après l\'expiration');

  // 7) départ (bye) : annoncé tout de suite ; salon vide → code libéré (inconnu)
  bob3.envoyer('bye');
  verifie(await alice.attendre((m) => m === 'B|bob45678').then(() => true, () => false), '« bye » → B|bob45678');
  alice.envoyer('bye');
  await pause(1000);
  const apres = await ws('k7qx', 'bob45678', false);
  verifie(await apres.attendre((m) => m === 'E|inconnu').then(() => true, () => false), 'salon vide → code libéré');

  // 8) page du lien d'invitation
  const page = await fetch(`http://127.0.0.1:${port}/d/K7QX`).then((r) => r.text());
  verifie(page.includes('intent://duel/k7qx#Intent;scheme=waforspeed;package=com.waforspeed.game;end'), 'lien /d/K7QX → page qui ouvre le jeu (waforspeed://duel/k7qx)');
} catch (e) {
  verifie(false, 'exception : ' + e.message);
}
relais.kill();
console.log(echecs ? `${echecs} échec(s)` : 'relais : tout est OK');
process.exit(echecs ? 1 : 0);
