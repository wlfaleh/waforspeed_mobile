/* WaForSpeed — Réglages propres au mobile (web app iOS / Android et appli Android).
   - service worker : le jeu s'ouvre hors ligne une fois chargé (web app installée) ;
   - plein écran + horizontal forcé au premier appui (Android, navigateur) ;
   - message « Tourne ton téléphone » et pause si on passe à la verticale en course ;
   - écran qui reste allumé pendant la course (Wake Lock) ;
   - pas de zoom, de loupe ni de menu contextuel au toucher (iOS). */
'use strict';

const isNativeApp = !!(window.Capacitor && window.Capacitor.isNativePlatform && window.Capacitor.isNativePlatform());
const isStandalone = matchMedia('(display-mode: standalone), (display-mode: fullscreen)').matches || navigator.standalone === true;
document.documentElement.classList.toggle('is-app', isNativeApp || isStandalone);
document.documentElement.classList.toggle('is-touch', isTouch);

/* ---------- Service worker (pas dans l'appli : les fichiers y sont déjà en local) ---------- */
if ('serviceWorker' in navigator && !isNativeApp && (location.protocol === 'https:' || location.hostname === 'localhost')) {
  addEventListener('load', () => navigator.serviceWorker.register('sw.js').catch((e) => console.warn('Service worker non installé', e)));
}

/* ---------- Plein écran + horizontal au premier appui (Android) ---------- */
function goFullscreen() {
  if (!isTouch || isNativeApp || isStandalone || document.fullscreenElement) return;
  const el = document.documentElement;
  if (!el.requestFullscreen) return;   // iPhone : pas d'API plein écran, la web app installée s'ouvre déjà en plein écran
  el.requestFullscreen({ navigationUI: 'hide' })
    .then(() => screen.orientation && screen.orientation.lock && screen.orientation.lock('landscape'))
    .catch(() => {});
}
document.addEventListener('click', (e) => { if (e.target.closest('#startBtn, #duelBtn, #againBtn, #rematchBtn')) goFullscreen(); }, true);

/* ---------- Course : orientation et écran allumé ---------- */
const rotateEl = $('#rotate');
const portrait = matchMedia('(orientation: portrait)');
const racing = () => state === 'play' || state === 'count' || state === 'pause';
let wakeLock = null;
function keepAwake(on) {
  if (!('wakeLock' in navigator)) return;
  if (on && !wakeLock && !document.hidden) {
    wakeLock = 'pending';
    navigator.wakeLock.request('screen').then((l) => {
      wakeLock = l; l.addEventListener('release', () => { if (wakeLock === l) wakeLock = null; });
    }).catch(() => { wakeLock = null; });
  } else if (!on && wakeLock && wakeLock !== 'pending') { wakeLock.release().catch(() => {}); wakeLock = null; }
}
function checkMobile() {
  const r = racing();
  const turn = isTouch && portrait.matches && r;
  if (turn && rotateEl.hidden && (state === 'play' || state === 'count')) pauseGame();
  rotateEl.hidden = !turn;
  keepAwake(r && state !== 'pause');
}
portrait.addEventListener ? portrait.addEventListener('change', checkMobile) : portrait.addListener(checkMobile);
document.addEventListener('visibilitychange', () => { if (!document.hidden) wakeLock = null; checkMobile(); });
setInterval(checkMobile, 400);

/* ---------- Pas de zoom au pincement ni de menu au toucher long (iOS Safari ignore user-scalable=no ;
   le double appui est déjà bloqué par touch-action: none) ---------- */
for (const ev of ['gesturestart', 'gesturechange']) document.addEventListener(ev, (e) => e.preventDefault(), { passive: false });
document.addEventListener('contextmenu', (e) => { if (isTouch) e.preventDefault(); });

/* ---------- Bouton « retour » d'Android dans l'appli : pause plutôt que quitter ---------- */
if (isNativeApp && window.Capacitor.Plugins && window.Capacitor.Plugins.App) {
  window.Capacitor.Plugins.App.addListener('backButton', () => {
    if (state === 'play' || state === 'count') pauseGame();
    else if (!screens.pause.hidden) resumeGame();
    else window.Capacitor.Plugins.App.exitApp();
  });
}
