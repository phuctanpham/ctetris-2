// =============================================================================
// app/src/ctetris_debug.h  --  HTTP debug logging for cTetris
// =============================================================================
// Activated at compile-time with -DCTETRIS_DEBUG=1 (set via DEBUG=true in
// app/.env, picked up by CMakeLists.txt).
//
// Native (.exe / .app):  writes timestamped lines to  ctetris.log  in CWD.
// WASM  (.wasm)       :  writes via SDL_Log  ->  browser console.error
//                        (filter "[HTTP_DEBUG]" in DevTools Console).
//
// CTDBG_INIT() writes a device/OS header once at startup:
//
//   Native ctetris.log example:
//     === cTetris Debug Log ===
//     Started   : 2026-05-19 16:07:03
//
//     [SYSINFO] Platform  : macOS
//     [SYSINFO] OS Version: 15.3.2 (Darwin 24.3.0)
//     [SYSINFO] CPU Arch  : arm64
//     [SYSINFO] CPU Cores : 10  |  RAM: 16384 MB
//     [SYSINFO] SDL3 Ver  : 3.2.18
//     [SYSINFO] Build     : native  DEBUG=1
//
//     --- HTTP Requests ---
//     [16:07:03][HTTP_DEBUG] >> REQ  GET   https://gist...
//
//   WASM browser console example:
//     [HTTP_DEBUG] === cTetris Debug Log ===
//     [HTTP_DEBUG] [SYSINFO] Platform  : Web (Emscripten / WASM)
//     [HTTP_DEBUG] [SYSINFO] UserAgent : Mozilla/5.0 (Macintosh...
//     [HTTP_DEBUG] [SYSINFO] Screen    : 1512 x 982  devicePixelRatio=2.00
//     [HTTP_DEBUG] [SYSINFO] Network   : online=yes  effectiveType=4g
//     [HTTP_DEBUG] [SYSINFO] Build     : wasm  DEBUG=1
//
// Usage in any .cpp:
//   #include "ctetris_debug.h"
//   CTDBG_REQ ("GET",  url);
//   CTDBG_RES ("GET",  url, httpStatus, body.size());
//   CTDBG_BODY(body);
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
#include <cstring>
#include <SDL3/SDL.h>

// ── Platform-specific OS version headers ─────────────────────────────────────
#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#elif defined(__APPLE__)
#  include <sys/utsname.h>
#  include <sys/sysctl.h>
#elif defined(__linux__) && !defined(__EMSCRIPTEN__)
#  include <sys/utsname.h>
#endif

// =============================================================================
#ifdef __EMSCRIPTEN__
// ── WASM path ─────────────────────────────────────────────────────────────────
#include <emscripten.h>

static inline void _ctdbg_emit(const char* fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    SDL_vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    SDL_Log("[HTTP_DEBUG] %s", buf);
}

static inline void _ctdbg_write_sysinfo_wasm() {
    int cpus = SDL_GetNumLogicalCPUCores();
    int ram  = SDL_GetSystemRAM();

    _ctdbg_emit("[SYSINFO] Platform  : Web (Emscripten / WASM)");
    _ctdbg_emit("[SYSINFO] CPU Cores : %d  |  RAM: %d MB", cpus, ram);

    // SDL3 version
    {
        int v = SDL_GetVersion();
        _ctdbg_emit("[SYSINFO] SDL3 Ver  : %d.%d.%d",
                    SDL_VERSIONNUM_MAJOR(v),
                    SDL_VERSIONNUM_MINOR(v),
                    SDL_VERSIONNUM_MICRO(v));
    }

    // navigator.userAgent
    char ua[512] = "(unavailable)";
    EM_ASM({
        var s = (typeof navigator !== 'undefined' && navigator.userAgent)
                ? navigator.userAgent : "(unknown)";
        stringToUTF8(s, $0, $1);
    }, ua, (int)sizeof(ua));
    _ctdbg_emit("[SYSINFO] UserAgent : %s", ua);

    // navigator.platform
    char plat[128] = "(unavailable)";
    EM_ASM({
        var s = (typeof navigator !== 'undefined' && navigator.platform)
                ? navigator.platform : "(unknown)";
        stringToUTF8(s, $0, $1);
    }, plat, (int)sizeof(plat));
    _ctdbg_emit("[SYSINFO] NavPlatform: %s", plat);

    // Screen resolution + devicePixelRatio (×100 to avoid float in EM_ASM_INT)
    int sw = EM_ASM_INT({ return (typeof screen !== 'undefined') ? (screen.width  || 0) : 0; });
    int sh = EM_ASM_INT({ return (typeof screen !== 'undefined') ? (screen.height || 0) : 0; });
    int dp = EM_ASM_INT({
        return Math.round(
            ((typeof window !== 'undefined' && window.devicePixelRatio)
             ? window.devicePixelRatio : 1) * 100);
    });
    _ctdbg_emit("[SYSINFO] Screen    : %d x %d  devicePixelRatio=%.2f",
                sw, sh, (float)dp / 100.0f);

    // Network / connection type
    int online = EM_ASM_INT({
        return (typeof navigator !== 'undefined' && navigator.onLine) ? 1 : 0;
    });
    char conn[64] = "unknown";
    EM_ASM({
        var c = (typeof navigator !== 'undefined' && navigator.connection)
                ? (navigator.connection.effectiveType || "unknown") : "unknown";
        stringToUTF8(c, $0, $1);
    }, conn, (int)sizeof(conn));
    _ctdbg_emit("[SYSINFO] Network   : online=%s  effectiveType=%s",
                online ? "yes" : "no", conn);

    _ctdbg_emit("[SYSINFO] Build     : wasm  DEBUG=1");
}

#define CTDBG_INIT() \
    do { \
        { \
            char _ts[32]; \
            std::time_t _t = std::time(nullptr); \
            std::strftime(_ts, sizeof(_ts), "%Y-%m-%d %H:%M:%S", \
                          std::localtime(&_t)); \
            _ctdbg_emit("=== cTetris Debug Log ==="); \
            _ctdbg_emit("Started   : %s", _ts); \
        } \
        _ctdbg_write_sysinfo_wasm(); \
        _ctdbg_emit("--- HTTP Requests ---"); \
    } while(0)

// =============================================================================
#else
// ── Native path (.exe / .app) ─────────────────────────────────────────────────
#include <mutex>

namespace _ctdbg_internal {

    inline std::mutex& getMutex() {
        static std::mutex m;
        return m;
    }

    inline void getOsVersion(char* buf, int bufLen) {
#if defined(_WIN32)
        // RtlGetVersion works on all Windows 10/11 versions
        // (GetVersionEx is deprecated and lies about version).
        typedef LONG(WINAPI* RtlGetVer_t)(OSVERSIONINFOEXW*);
        OSVERSIONINFOEXW vi = {};
        vi.dwOSVersionInfoSize = sizeof(vi);
        HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        auto pfn = ntdll
            ? reinterpret_cast<RtlGetVer_t>(
                  GetProcAddress(ntdll, "RtlGetVersion"))
            : nullptr;
        if (pfn && pfn(&vi) == 0) {
            std::snprintf(buf, bufLen,
                "Windows %lu.%lu Build %lu",
                vi.dwMajorVersion, vi.dwMinorVersion, vi.dwBuildNumber);
        } else {
            std::snprintf(buf, bufLen, "Windows (version unknown)");
        }
        // Append edition from registry (optional)
        HKEY hKey = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            wchar_t edition[128] = {};
            DWORD sz = sizeof(edition);
            if (RegQueryValueExW(hKey, L"EditionID",
                    nullptr, nullptr,
                    reinterpret_cast<LPBYTE>(edition), &sz) == ERROR_SUCCESS) {
                char ed8[128] = {};
                WideCharToMultiByte(CP_UTF8, 0, edition, -1,
                                    ed8, sizeof(ed8), nullptr, nullptr);
                int used = (int)std::strlen(buf);
                std::snprintf(buf + used, bufLen - used, " (%s)", ed8);
            }
            RegCloseKey(hKey);
        }

#elif defined(__APPLE__)
        // kern.osproductversion → "15.3.2", uname release → "24.3.0"
        char prodver[64] = "(unknown)";
        std::size_t pvLen = sizeof(prodver);
        sysctlbyname("kern.osproductversion", prodver, &pvLen, nullptr, 0);
        struct utsname u {};
        uname(&u);
        std::snprintf(buf, bufLen, "%s (Darwin %s)", prodver, u.release);

#elif defined(__linux__)
        // Prefer /etc/os-release PRETTY_NAME
        char prettyName[128] = "";
        FILE* f = std::fopen("/etc/os-release", "r");
        if (f) {
            char line[256];
            while (std::fgets(line, sizeof(line), f)) {
                if (std::strncmp(line, "PRETTY_NAME=", 12) == 0) {
                    char* v = line + 12;
                    if (*v == '"') v++;
                    int vl = (int)std::strlen(v);
                    while (vl > 0 && (v[vl-1] == '\n' || v[vl-1] == '"'
                                   || v[vl-1] == '\r'))
                        v[--vl] = '\0';
                    std::snprintf(prettyName, sizeof(prettyName), "%s", v);
                    break;
                }
            }
            std::fclose(f);
        }
        struct utsname u {};
        uname(&u);
        if (prettyName[0] != '\0') {
            std::snprintf(buf, bufLen, "%s (kernel %s)", prettyName, u.release);
        } else {
            std::snprintf(buf, bufLen, "%s %s", u.sysname, u.release);
        }

#else
        std::snprintf(buf, bufLen, "Unknown OS");
#endif
    }

    inline const char* getCpuArch() {
#if defined(__aarch64__) || defined(_M_ARM64)
        return "arm64";
#elif defined(__arm__) || defined(_M_ARM)
        return "armv7";
#elif defined(__x86_64__) || defined(_M_X64)
        return "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
        return "x86";
#elif defined(__riscv)
        return "riscv";
#else
        return "unknown";
#endif
    }

}  // namespace _ctdbg_internal

// Low-level formatter used inside CTDBG_INIT (file is already open)
static inline void _ctdbg_writef(FILE* f, const char* fmt, ...) {
    char buf[2048];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    std::fprintf(f, "%s\n", buf);
}

// Timestamped emit used by all CTDBG_* macros
static inline void _ctdbg_emit(const char* fmt, ...) {
    std::lock_guard<std::mutex> lk(_ctdbg_internal::getMutex());
    FILE* f = std::fopen("ctetris.log", "a");
    if (!f) return;
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

#define CTDBG_INIT() \
    do { \
        std::lock_guard<std::mutex> _lk(_ctdbg_internal::getMutex()); \
        FILE* _f = std::fopen("ctetris.log", "w"); \
        if (!_f) break; \
        /* Timestamp */ \
        std::time_t _now = std::time(nullptr); \
        char _ts[32]; \
        std::strftime(_ts, sizeof(_ts), "%Y-%m-%d %H:%M:%S", \
                      std::localtime(&_now)); \
        _ctdbg_writef(_f, "=== cTetris Debug Log ==="); \
        _ctdbg_writef(_f, "Started   : %s", _ts); \
        _ctdbg_writef(_f, ""); \
        /* OS / Platform */ \
        _ctdbg_writef(_f, "[SYSINFO] Platform  : %s", SDL_GetPlatform()); \
        { char _osver[256] = ""; \
          _ctdbg_internal::getOsVersion(_osver, (int)sizeof(_osver)); \
          _ctdbg_writef(_f, "[SYSINFO] OS Version: %s", _osver); } \
        _ctdbg_writef(_f, "[SYSINFO] CPU Arch  : %s", \
                      _ctdbg_internal::getCpuArch()); \
        /* Hardware */ \
        _ctdbg_writef(_f, "[SYSINFO] CPU Cores : %d  |  RAM: %d MB", \
                      SDL_GetNumLogicalCPUCores(), SDL_GetSystemRAM()); \
        /* SDL3 runtime version */ \
        { int _v = SDL_GetVersion(); \
          _ctdbg_writef(_f, "[SYSINFO] SDL3 Ver  : %d.%d.%d", \
                        SDL_VERSIONNUM_MAJOR(_v), \
                        SDL_VERSIONNUM_MINOR(_v), \
                        SDL_VERSIONNUM_MICRO(_v)); } \
        /* Build tag */ \
        _ctdbg_writef(_f, "[SYSINFO] Build     : native  DEBUG=1"); \
        _ctdbg_writef(_f, ""); \
        _ctdbg_writef(_f, "--- HTTP Requests ---"); \
        std::fclose(_f); \
    } while(0)

#endif  // __EMSCRIPTEN__

// ─── Public HTTP logging macros (same on both platforms) ──────────────────────

#define CTDBG_REQ(method, url) \
    _ctdbg_emit(">> REQ  %-4s  %s", (method), (url))

#define CTDBG_RES(method, url, httpStatus, bodyLen) \
    _ctdbg_emit("<< RES  %-4s  %s  ->  HTTP %d  (%zu bytes)", \
                (method), (url), (int)(httpStatus), (std::size_t)(bodyLen))

#define CTDBG_BODY(bodyStr) \
    do { \
        std::string _s = (bodyStr); \
        bool _trunc = (_s.size() > 500); \
        if (_trunc) _s = _s.substr(0, 500); \
        _ctdbg_emit("   BODY [%s]%s", \
                    _s.c_str(), _trunc ? " ...(truncated)" : ""); \
    } while(0)

#define CTDBG_ERR(msg) \
    _ctdbg_emit("!! ERR  %s", (msg))

#define CTDBG_INFO(key, val) \
    _ctdbg_emit("   INFO  %s = %s", (key), (val))

#else
// ─────────────────────────────────────────────────────────────────────────────
// DISABLED — all macros are zero-cost no-ops
// ─────────────────────────────────────────────────────────────────────────────
#define CTDBG_INIT()                                (void)0
#define CTDBG_REQ(method, url)                      (void)0
#define CTDBG_RES(method, url, httpStatus, bodyLen) (void)0
#define CTDBG_BODY(bodyStr)                         (void)0
#define CTDBG_ERR(msg)                              (void)0
#define CTDBG_INFO(key, val)                        (void)0

#endif  // CTETRIS_DEBUG
