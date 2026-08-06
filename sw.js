const CACHE_NAME = 'nano-tetris-v1';
const ASSETS_TO_CACHE = [
  '/',
  '/index.html',
  '/site.webmanifest',
  '/docs/style.css',
  '/docs/assets/images/game_boy.png',
  '/docs/assets/images/favicon-32x32.png',
  '/docs/assets/images/favicon-16x16.png',
  '/docs/assets/images/android-chrome-192x192.png',
  '/docs/assets/images/android-chrome-512x512.png',
  '/docs/assets/images/apple-touch-icon.png',
  '/docs/assets/images/favicon.ico'
];

self.addEventListener('install', event => {
  event.waitUntil(
    caches.open(CACHE_NAME).then(cache => cache.addAll(ASSETS_TO_CACHE))
  );
  self.skipWaiting();
});

self.addEventListener('activate', event => {
  event.waitUntil(
    caches.keys().then(keys => Promise.all(
      keys.filter(k => k !== CACHE_NAME).map(k => caches.delete(k))
    ))
  );
  self.clients.claim();
});

self.addEventListener('fetch', event => {
  if (event.request.method !== 'GET') return;
  event.respondWith(
    caches.match(event.request).then(resp => resp || fetch(event.request).then(fetchResp => {
      return caches.open(CACHE_NAME).then(cache => {
        try { cache.put(event.request, fetchResp.clone()); } catch (e) {}
        return fetchResp;
      });
    }))
  );
});
