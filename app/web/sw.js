// =============================================================================
// sw.js -- Service Worker cho cTetris PWA
// =============================================================================
// Chien luoc: CACHE-FIRST cho assets tinh (HTML, JS, WASM, SVG)
//   1. install: pre-cache tat ca core assets cua game
//   2. fetch:   neu request match cache -> serve tu cache (offline-ready)
//                neu khong -> fetch network va add vao cache
//   3. activate: don dep cache cu khi version doi
// =============================================================================

// CACHE_VERSION: doi de force update cache khi deploy phien ban moi.
// Khi sw.js thay doi (vi du tang version), browser tu dong fetch sw.js moi
// va activate -> phase activate xoa cache cu.
// v1 -> v2: fix full-height scaling + quit WASM behavior + text overflow
const CACHE_VERSION = 'ctetris-v2';

// Core assets cua game -- moi file deu can de chay offline.
const CORE_ASSETS = [
    './',
    './index.html',
    './cTetris.html',
    './cTetris.js',
    './cTetris.wasm',
    './favicon.svg',
    './manifest.webmanifest'
];

// Phase 1: install -> pre-cache core assets
self.addEventListener('install', function(event) {
    console.log('[SW] install:', CACHE_VERSION);
    event.waitUntil(
        caches.open(CACHE_VERSION).then(function(cache) {
            return Promise.all(
                CORE_ASSETS.map(function(url) {
                    return cache.add(url).catch(function(err) {
                        console.warn('[SW] Khong cache duoc:', url, err);
                    });
                })
            );
        })
    );
    // Skip waiting de SW moi active ngay, khong can cho tab dong
    self.skipWaiting();
});

// Phase 2: activate -> don dep cache cu (khac CACHE_VERSION hien tai)
self.addEventListener('activate', function(event) {
    console.log('[SW] activate:', CACHE_VERSION);
    event.waitUntil(
        caches.keys().then(function(keys) {
            return Promise.all(
                keys.map(function(key) {
                    if (key !== CACHE_VERSION) {
                        console.log('[SW] xoa cache cu:', key);
                        return caches.delete(key);
                    }
                })
            );
        })
    );
    // Claim clients de SW moi takeover ngay tab dang mo
    self.clients.claim();
});

// Phase 3: fetch -> CACHE-FIRST strategy
self.addEventListener('fetch', function(event) {
    if (event.request.method !== 'GET') return;
    var url = new URL(event.request.url);
    if (url.origin !== self.location.origin) return;

    event.respondWith(
        caches.match(event.request).then(function(cachedResponse) {
            if (cachedResponse) {
                return cachedResponse;
            }
            return fetch(event.request).then(function(networkResponse) {
                if (!networkResponse || networkResponse.status !== 200 ||
                    networkResponse.type !== 'basic') {
                    return networkResponse;
                }
                var responseToCache = networkResponse.clone();
                caches.open(CACHE_VERSION).then(function(cache) {
                    cache.put(event.request, responseToCache);
                });
                return networkResponse;
            }).catch(function() {
                console.warn('[SW] fetch fail (offline?):', event.request.url);
            });
        })
    );
});