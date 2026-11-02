/* WaForSpeed — Service worker : le jeu s'ouvre hors ligne une fois chargé.
   - fichiers du jeu (page, scripts, bibliothèques, polices) : mis en cache à l'installation,
     servis depuis le cache puis rafraîchis en arrière-plan ;
   - modèles 3D des voitures (plusieurs Mo) : mis en cache la première fois qu'on les charge.
   Change VERSION à chaque mise à jour pour vider l'ancien cache. */
'use strict';

const VERSION = 'v1';
const SHELL = 'wfs-shell-' + VERSION;
const MODELS = 'wfs-models-' + VERSION;
importScripts('sw-files.js');   // liste SHELL_FILES, générée par « npm run vendor »

self.addEventListener('install', (e) => {
  e.waitUntil(caches.open(SHELL).then((c) => c.addAll(self.SHELL_FILES)).then(() => self.skipWaiting()));
});
self.addEventListener('activate', (e) => {
  e.waitUntil(caches.keys()
    .then((keys) => Promise.all(keys.filter((k) => k.startsWith('wfs-') && k !== SHELL && k !== MODELS).map((k) => caches.delete(k))))
    .then(() => self.clients.claim()));
});
self.addEventListener('fetch', (e) => {
  const req = e.request;
  if (req.method !== 'GET') return;
  const url = new URL(req.url);
  if (url.origin !== location.origin) return;   // PeerJS (duel) et le reste : réseau direct
  if (url.pathname.includes('/models/')) {
    // modèle 3D : cache d'abord, sinon réseau puis mise en cache
    e.respondWith(caches.open(MODELS).then((c) => c.match(req).then((hit) => hit || fetch(req).then((res) => {
      if (res.ok) c.put(req, res.clone());
      return res;
    }))));
    return;
  }
  // fichiers du jeu : cache tout de suite, mise à jour en arrière-plan
  e.respondWith(caches.open(SHELL).then((c) => c.match(req, { ignoreSearch: true }).then((hit) => {
    const net = fetch(req).then((res) => { if (res.ok) c.put(req, res.clone()); return res; }).catch(() => hit);
    return hit || net;
  })));
});
