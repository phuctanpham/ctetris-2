// Service Worker -- cache-first cho PWA installable + offline support
// Version bump moi lan thay doi danh sach asset de browser tu invalidate cache
const CACHE_NAME = 'ctetris-v2';
const ASSETS = [
    './',
    'ctetris.html',
    'ctetris.js',
    'ctetris.wasm',
    'manifest.webmanifest',
    'favicon.svg',
    'favicon.png',
    'apple-touch-icon.png',
    'icon-192.png',
    'icon-512.png',
    'icon-maskable-192.png',
    'icon-maskable-512.png'
];

self.addEventListener('install', function(event) {
    event.waitUntil(
        caches.open(CACHE_NAME).then(function(cache) {
            // addAll fail toan bo neu mot file 404 -> dung add() tung file
            return Promise.all(ASSETS.map(function(url) {
                return cache.add(url).catch(function(err) {
                    console.warn('SW: skip', url, err);
                });
            }));
        })
    );
    self.skipWaiting();
});

self.addEventListener('activate', function(event) {
    event.waitUntil(
        caches.keys().then(function(keys) {
            return Promise.all(keys
                .filter(function(k) { return k !== CACHE_NAME; })
                .map(function(k) { return caches.delete(k); }));
        })
    );
    self.clients.claim();
});

self.addEventListener('fetch', function(event) {
    if (event.request.method !== 'GET') return;
    // Bo qua cross-origin requests (vd FetchContent emsdk runtime)
    if (new URL(event.request.url).origin !== location.origin) return;
    event.respondWith(
        caches.match(event.request).then(function(cached) {
            return cached || fetch(event.request).then(function(response) {
                if (response && response.status === 200) {
                    var clone = response.clone();
                    caches.open(CACHE_NAME).then(function(c) { c.put(event.request, clone); });
                }
                return response;
            }).catch(function() {
                return new Response('Offline', { status: 503 });
            });
        })
    );
});