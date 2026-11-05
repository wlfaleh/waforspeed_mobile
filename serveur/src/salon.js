// WaForSpeed — relais du « Duel entre amis » (WFS-112) : logique d'un salon, sans dépendance (Cloudflare Workers et Node).
// Un salon = un code de 4 caractères. Le créateur (hote=1) ouvre le salon ; ses amis le rejoignent avec le code ou le lien.
// Le relais ne comprend pas le jeu : il garde la dernière présence de chaque joueur (texte « clé=valeur;… » de
// src/game/duel.cpp) et la renvoie aux autres, horodatée par son horloge (horloge commune des joueurs).
//
// Protocole (messages texte d'une WebSocket ouverte sur /salon/<code>?id=<id>&hote=0|1&v=1) :
//   client → relais : « P|<présence> » · « ping|<horloge du client> » · « bye »
//   relais → client : « OK|<horloge>|<fin de validité du code> » · « P|<id>|<horloge>|<présence> » · « B|<id> » (parti)
//                     « pong|<horloge du client>|<horloge du relais> » · « E|<raison> » puis fermeture
//   raisons : inconnu (pas de salon à ce code), expire (code de plus de 15 min), plein (4 joueurs), pris (le code a
//   déjà un créateur), invalide (adresse mal formée)
// Règles (regles-de-gestion/WFS-112.md) : RG-05 reconnexion (un joueur coupé garde sa place 30 s), RG-06 codes qui
// expirent (15 min après la création, pour les nouveaux venus), données jamais crues (tailles, débit, caractères
// bornés ; un message hostile est ignoré), pas de donnée personnelle (aucune adresse IP gardée), pas de chat.

export const LIMITES = {
  JOUEURS: 4,                    // RG-03 : jusqu'à 4 joueurs
  MSG_MAX: 600,                  // présence ≤ 512 caractères (duel::MSG_MAX) + en-tête
  EXPIRATION_MS: 15 * 60 * 1000, // RG-06 : code valable 15 min
  GRACE_MS: 30 * 1000,           // RG-05 : place gardée 30 s après une coupure
  DEBIT_MAX: 40,                 // messages par seconde et par joueur (au-delà : ignorés)
};

const CODE = /^[a-z0-9]{4}$/;
const ID = /^[a-z0-9]{6,20}$/;

// /salon/<code>?id=…&hote=… → { code, id, hote } ou null
export function analyserAdresse(url) {
  const m = /^\/salon\/([A-Za-z0-9]{4})$/.exec(url.pathname || '');
  if (!m) return null;
  const code = m[1].toLowerCase(), id = url.searchParams.get('id') || '';
  if (!CODE.test(code) || !ID.test(id)) return null;
  return { code, id, hote: url.searchParams.get('hote') === '1' };
}

// présence reçue : texte imprimable (UTF-8 accepté pour le pseudo), sans retour à la ligne ni « | »
function presenceValide(p) {
  if (typeof p !== 'string' || p.length > 512) return false;
  for (let i = 0; i < p.length; i++) {
    const c = p.charCodeAt(i);
    if (c < 0x20 || c === 0x7c || c === 0x7f) return false;
  }
  return true;
}

export class Salon {
  // options : { expirationMs, graceMs } (tests : durées raccourcies)
  constructor(code, options = {}) {
    this.code = code;
    this.expirationMs = options.expirationMs ?? LIMITES.EXPIRATION_MS;
    this.graceMs = options.graceMs ?? LIMITES.GRACE_MS;
    this.cree = 0;                 // 0 : pas de salon ouvert à ce code
    this.joueurs = new Map();      // id → { socket, presence, maj, hote, coupeDepuis, fenetre, n }
    this.ignores = 0;              // messages refusés (journal du relais local, tests)
  }

  ouvert() { return this.cree > 0; }
  vide() { return this.joueurs.size === 0; }

  // socket : { envoyer(texte), fermer() } ; renvoie { ok: true } ou { ok: false, raison }
  connecter(socket, id, hote, maintenant) {
    const deja = this.joueurs.get(id);
    if (deja) {   // RG-05 : reconnexion (même identifiant) — acceptée même après l'expiration du code
      if (deja.socket && deja.socket !== socket) { try { deja.socket.fermer(); } catch { /* déjà fermée */ } }
      deja.socket = socket; deja.coupeDepuis = 0;
    } else {
      if (!this.ouvert()) {
        if (!hote) return { ok: false, raison: 'inconnu' };
        this.cree = maintenant;
      } else {
        if (maintenant - this.cree > this.expirationMs) return { ok: false, raison: 'expire' };
        if (this.joueurs.size >= LIMITES.JOUEURS) return { ok: false, raison: 'plein' };
        if (hote) for (const j of this.joueurs.values()) if (j.hote) return { ok: false, raison: 'pris' };
      }
      this.joueurs.set(id, { socket, presence: '', maj: 0, hote, coupeDepuis: 0, fenetre: maintenant, n: 0 });
    }
    socket.envoyer(`OK|${maintenant}|${this.cree + this.expirationMs}`);
    for (const [autre, j] of this.joueurs) if (autre !== id && j.presence) socket.envoyer(`P|${autre}|${j.maj}|${j.presence}`);
    return { ok: true };
  }

  // message d'un joueur (ignoré s'il vient d'une ancienne connexion, s'il est trop long, trop fréquent ou mal formé)
  recevoir(id, socket, texte, maintenant) {
    const j = this.joueurs.get(id);
    if (!j || j.socket !== socket) { this.ignores++; return; }
    if (maintenant - j.fenetre >= 1000) { j.fenetre = maintenant; j.n = 0; }
    if (++j.n > LIMITES.DEBIT_MAX || typeof texte !== 'string' || texte.length > LIMITES.MSG_MAX) { this.ignores++; return; }
    if (texte.startsWith('P|')) {
      const p = texte.slice(2);
      if (!presenceValide(p)) { this.ignores++; return; }
      j.presence = p; j.maj = maintenant;
      this.diffuser(id, `P|${id}|${maintenant}|${p}`);
    } else if (texte.startsWith('ping|')) {
      const c = texte.slice(5);
      if (!/^[0-9.]{1,24}$/.test(c)) { this.ignores++; return; }
      socket.envoyer(`pong|${c}|${maintenant}`);
    } else if (texte === 'bye') {
      this.retirer(id);
    } else this.ignores++;
  }

  // connexion fermée : la place est gardée 30 s (RG-05), sauf si une nouvelle connexion l'a déjà reprise
  deconnecter(id, socket, maintenant) {
    const j = this.joueurs.get(id);
    if (!j || j.socket !== socket) return;
    j.socket = null; j.coupeDepuis = maintenant;
  }

  // à appeler régulièrement : retire les joueurs coupés depuis plus de 30 s ; salon vide → code libéré
  tick(maintenant) {
    for (const [id, j] of [...this.joueurs]) if (!j.socket && maintenant - j.coupeDepuis > this.graceMs) this.retirer(id);
  }

  retirer(id) {
    const j = this.joueurs.get(id);
    if (!j) return;
    this.joueurs.delete(id);
    if (j.socket) { try { j.socket.fermer(); } catch { /* déjà fermée */ } }
    this.diffuser(id, `B|${id}`);
    if (this.vide()) this.cree = 0;
  }

  diffuser(de, texte) {
    for (const [id, j] of this.joueurs) if (id !== de && j.socket) { try { j.socket.envoyer(texte); } catch { /* coupée */ } }
  }
}

// page du lien d'invitation (https://<relais>/d/<code>) : ouvre le jeu dans le salon (lien profond waforspeed://duel/<code>)
export function pageInvitation(code) {
  const c = CODE.test(code) ? code : '';
  const appli = `intent://duel/${c}#Intent;scheme=waforspeed;package=com.waforspeed.game;end`;
  return `<!doctype html><html lang="fr"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>WaForSpeed — Duel entre amis</title>
<style>body{margin:0;min-height:100vh;display:flex;align-items:center;justify-content:center;background:#0A0B10;color:#F2F4F8;font-family:system-ui,sans-serif;text-align:center}
.c{font-size:64px;letter-spacing:.18em;color:#FF6A00;font-weight:800}a{display:inline-block;margin-top:24px;padding:16px 32px;background:#FF6A00;color:#0A0B10;font-weight:800;text-decoration:none;text-transform:uppercase;letter-spacing:.12em}
p{color:#8A90A6}</style></head><body><main><p>DUEL ENTRE AMIS</p><div class="c">${c.toUpperCase()}</div>
<a href="${appli}">Ouvrir WaForSpeed</a><p>Ou ouvre le jeu : Duel entre amis → Rejoindre avec le code.</p></main>
<script>if(${c ? 'true' : 'false'})location.href=${JSON.stringify(appli)};</script></body></html>`;
}
