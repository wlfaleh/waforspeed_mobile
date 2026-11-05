// WaForSpeed — relais du « Duel entre amis » sur Cloudflare Workers (WFS-112). Logique du salon : salon.js.
// Une Durable Object par code de salon (SALONS.idFromName(code)) : tous les joueurs d'un même code arrivent sur la même
// instance, qui garde les présences en mémoire (rien n'est écrit sur disque, aucune donnée personnelle).
//   GET /                → « relais WaForSpeed » (vérification du déploiement)
//   GET /d/<code>        → page du lien d'invitation (ouvre le jeu dans le salon)
//   GET /salon/<code>?…  → WebSocket du salon (Upgrade: websocket)
import { Salon, analyserAdresse, pageInvitation } from './salon.js';

export default {
  async fetch(requete, env) {
    const url = new URL(requete.url);
    const inv = /^\/d\/([A-Za-z0-9]{4})$/.exec(url.pathname);
    if (inv) return new Response(pageInvitation(inv[1].toLowerCase()), { headers: { 'content-type': 'text/html; charset=utf-8' } });
    if (url.pathname === '/') return new Response('relais WaForSpeed v1\n', { headers: { 'content-type': 'text/plain; charset=utf-8' } });
    const a = analyserAdresse(url);
    if (!a) return new Response('adresse invalide\n', { status: 404 });
    if (requete.headers.get('Upgrade') !== 'websocket') return new Response('WebSocket attendue\n', { status: 426 });
    return env.SALONS.get(env.SALONS.idFromName(a.code)).fetch(requete);
  },
};

export class SalonDurable {
  constructor(etat, env) {
    this.etat = etat; this.env = env;
    this.salon = null; this.minuteur = null;
  }

  async fetch(requete) {
    const a = analyserAdresse(new URL(requete.url));
    if (!a) return new Response('adresse invalide\n', { status: 404 });
    if (!this.salon) this.salon = new Salon(a.code);
    const [client, serveur] = Object.values(new WebSocketPair());
    serveur.accept();
    const socket = {
      envoyer: (t) => { try { serveur.send(t); } catch { /* fermée */ } },
      fermer: () => { try { serveur.close(1000, 'fin'); } catch { /* déjà fermée */ } },
    };
    const r = this.salon.connecter(socket, a.id, a.hote, Date.now());
    if (!r.ok) {
      socket.envoyer(`E|${r.raison}`);
      socket.fermer();
      return new Response(null, { status: 101, webSocket: client });
    }
    serveur.addEventListener('message', (e) => this.salon.recevoir(a.id, socket, typeof e.data === 'string' ? e.data : '', Date.now()));
    const fin = () => this.salon.deconnecter(a.id, socket, Date.now());
    serveur.addEventListener('close', fin);
    serveur.addEventListener('error', fin);
    if (!this.minuteur) {
      this.minuteur = setInterval(() => {
        this.salon.tick(Date.now());
        if (this.salon.vide()) { clearInterval(this.minuteur); this.minuteur = null; }
      }, 2000);
    }
    return new Response(null, { status: 101, webSocket: client });
  }
}
