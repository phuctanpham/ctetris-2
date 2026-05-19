// =============================================================================
// app/src/ctetris_debug.h  --  HTTP debug logging for cTetris
// =============================================================================
// Activated at compile-time with -DCTETRIS_DEBUG=1 (set via DEBUG=true in
// app/.env, picked up by CMakeLists.txt debug patch).
//
// Native (.exe / .app):  writes timestamped lines to  ctetris.log  in CWD.
// WASM  (.wasm)       :  writes via SDL_Log  ->  browser console.error
//                        (filter "[HTTP_DEBUG]" in DevTools Console).
//
// Usage in any .cpp:
//   #include "ctetris_debug.h"
//
//   CTDBG_REQ ("GET",  url);
//   std::string body = doRequest(url);
//   CTDBG_RES ("GET",  url, httpStatus, body.size());
//   CTDBG_BODY(body);          // first 500 chars; truncated if longer
//   CTDBG_ERR ("curl failed");
// =============================================================================
#pragma once

#ifdef CTETRIS_DEBUG
// ─────────────────────────────────────────────────────────────────────────────
// ENABLED
// ─────────────────────────────────────────────────────────────────────────────
#include <string>
#include <cstdarg>
#include <cstdio>
#include <ctime>

#ifdef __EMSCRIPTEN__
//
// ── WASM path ──────────────────────────────────────────────────────────────
// SDL_Log() is mapped to Module.printErr inside shell.html, which calls
// console.error.  Prefix "[HTTP_DEBUG]" makes filtering easy in DevTools.
//
#include <SDL3/SDL.h>

static inline void _ctdbg_emit(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    SDL_vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    SDL_Log("[HTTP_DEBUG] %s", buf);
}

#else
//
// ── Native path (.exe / .app) ───────────────────────────────────────────────
// Appends to ctetris.log in the current working directory.
// File is created/cleared once per process by CTDBG_INIT() (called in
// main.cpp).  All subsequent macro calls append without reopening.
//
#include <mutex>

namespace _ctdbg_internal {
    inline std::mutex& getMutex() {
        static std::mutex m;
        return m;
    }
}

static inline void _ctdbg_emit(const char* fmt, ...) {
    std::lock_guard<std::mutex> lk(_ctdbg_internal::getMutex());
    FILE* f = std::fopen("ctetris.log", "a");
    if (!f) return;

    // HH:MM:SS timestamp
    std::time_t t = std::time(nullptr);
    char ts[12];
    std::strftime(ts, sizeof(ts), "%H:%M:%S", std::localtime(&t));
    std::fprintf(f, "[%s][HTTP_DEBUG] ", ts);

    char buf[2048];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    std::fprintf(f, "%s\n", buf);
    std::fclose(f);
}

#endif  // __EMSCRIPTEN__

// ─── Public macros ─────────────────────────────────────────────────────────

// Call once at app startup (main.cpp) to stamp the log header.
// On WASM this is a no-op (SDL_Log always appends to the JS console).
#ifndef __EMSCRIPTEN__
#define CTDBG_INIT() \
    do { \
        std::lock_guard<std::mutex> _lk(_ctdbg_internal::getMutex()); \
        FILE* _f = std::fopen("ctetris.log", "w"); \
        if (_f) { \
            std::time_t _t = std::time(nullptr); \
            char _ts[32]; \
            std::strftime(_ts, sizeof(_ts), "%Y-%m-%d %H:%M:%S", \
                          std::localtime(&_t)); \
            std::fprintf(_f, \
                "=== cTetris HTTP Debug Log ===\nStarted: %s\n\n", _ts); \
            std::fclose(_f); \
        } \
    } while(0)
#else
#define CTDBG_INIT() (void)0
#endif

// Log an outgoing HTTP request.
#define CTDBG_REQ(method, url) \
    _ctdbg_emit(">> REQ  %-4s  %s", (method), (url))

// Log the response status + body size.
#define CTDBG_RES(method, url, httpStatus, bodyLen) \
    _ctdbg_emit("<< RES  %-4s  %s  ->  HTTP %d  (%zu bytes)", \
                (method), (url), (int)(httpStatus), (std::size_t)(bodyLen))

// Log the first 500 chars of the response body (useful for JSON debugging).
#define CTDBG_BODY(bodyStr) \
    do { \
        std::string _s = (bodyStr); \
        bool _trunc = (_s.size() > 500); \
        if (_trunc) _s = _s.substr(0, 500); \
        _ctdbg_emit("   BODY [%s]%s", \
                    _s.c_str(), _trunc ? " ...(truncated)" : ""); \
    } while(0)

// Log a curl / fetch error string.
#define CTDBG_ERR(msg) \
    _ctdbg_emit("!! ERR  %s", (msg))

// Log arbitrary key-value context (headers, tokens, etc.)
#define CTDBG_INFO(key, val) \
    _ctdbg_emit("   INFO  %s = %s", (key), (val))

#else
// ─────────────────────────────────────────────────────────────────────────────
// DISABLED (CTETRIS_DEBUG not defined) — all macros compile to nothing
// ─────────────────────────────────────────────────────────────────────────────
#define CTDBG_INIT()                              (void)0
#define CTDBG_REQ(method, url)                    (void)0
#define CTDBG_RES(method, url, httpStatus, bodyLen) (void)0
#define CTDBG_BODY(bodyStr)                       (void)0
#define CTDBG_ERR(msg)                            (void)0
#define CTDBG_INFO(key, val)                      (void)0

#endif  // CTETRIS_DEBUG
