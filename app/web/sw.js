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
const CACHE_VERSION = 'ctetris-v1';

// Core assets cua game -- moi file deu can de chay offline.
// Path tuong doi voi scope cua SW (./ tu manifest).
const CORE_ASSETS = [
    './',
    './index.html',         // GitHub Pages se serve cTetris.html duoi ten nay
    './cTetris.html',       // backup neu access truc tiep
    './cTetris.js',         // Emscripten loader
    './cTetris.wasm',       // WebAssembly module chinh
    './favicon.svg',        // logo
    './manifest.webmanifest'
];

// Phase 1: install -> pre-cache core assets
self.addEventListener('install', function(event) {
    console.log('[SW] install:', CACHE_VERSION);
    event.waitUntil(
        caches.open(CACHE_VERSION).then(function(cache) {
            // addAll fail toan bo neu MOT trong cac asset fail.
            // Dung addAll cho core assets de dam bao tinh nhat quan.
            // Neu asset thieu (vd dev mode khong co index.html) thi log warn.
            return Promise.all(
                CORE_ASSETS.map(function(url) {
                    return cache.add(url).catch(function(err) {
                        console.warn('[SW] Khong cache duoc:', url, err);
                    });
                })
            );
        })
    );
    // Skip waiting de SW moi active ngay khong can cho tab dong
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
    // Chi handle GET request cua cung domain (same-origin)
    if (event.request.method !== 'GET') return;
    var url = new URL(event.request.url);
    if (url.origin !== self.location.origin) return;

    event.respondWith(
        caches.match(event.request).then(function(cachedResponse) {
            // Hit cache -> serve ngay (offline-ready)
            if (cachedResponse) {
                return cachedResponse;
            }
            // Miss cache -> fetch network, add vao cache de offline lan sau
            return fetch(event.request).then(function(networkResponse) {
                // Chi cache response thanh cong (status 200, basic type)
                if (!networkResponse || networkResponse.status !== 200 ||
                    networkResponse.type !== 'basic') {
                    return networkResponse;
                }
                // Clone vi response stream chi doc duoc 1 lan
                var responseToCache = networkResponse.clone();
                caches.open(CACHE_VERSION).then(function(cache) {
                    cache.put(event.request, responseToCache);
                });
                return networkResponse;
            }).catch(function() {
                // Network fail va khong co cache -> tra ve undefined
                // (browser hien default offline page)
                console.warn('[SW] fetch fail (offline?):', event.request.url);
            });
        })
    );
});