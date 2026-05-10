# cTetris Build Summary - Console Window Hidden

## ✅ Build Hoàn Tất Thành Công

- **Executable**: `D:\projects\ctetris-2\app\build\desktop\windows\cTetris.exe` (604 KB)
- **SDL3 Runtime**: `SDL3.dll` (đã copy vào output folder)
- **Build Date**: 11/05/2026

## 🎯 Tính Năng Đã Thêm

### 1. Console Window Ẩn Trên Windows ✅

**Phương pháp sử dụng**: Linker flags MSVC
```cmake
# CMakeLists.txt - Lines 95-101
target_link_options(cTetris PRIVATE 
    /SUBSYSTEM:WINDOWS 
    /ENTRY:mainCRTStartup
)
```

**Kết quả:**
- Khi chạy `cTetris.exe`, **chỉ hiển thị cửa sổ GUI S
- Console window hoàn toàn bị ẩn
- Chỉ có tác dụng trên Windows (macOS/Linẽ ignore)

### 2. File Logging System ✅

**Header-only Logger Class**: `src/logger.h`

**Tính năng:**
- ✅ Ghi log vào file `game.log` (cùng thư mục với .exe)
- ✅ Singleton pattern - sử dụng từ bất kỳ file nào
- ✅ Thread-safe với mutex
- ✅ Printf-style formatting
- ✅ Auto-timestamp: `HH:MM:SS.mmm`
- ✅ Multiple log levels: `log()`, `logAudio()`, `logEvent()`, `logError()`

**File đã sửa:**
- `src/logger.h` - Thư viện logging (mới)
- `CMakeLists.txt` - Thêm `src` vào include dirs, Windows linker flags
- `main.cpp` - Tích hợp logger, ghi các sự kiện chính

## 📝 Cách Sử Dụng Logger

### Trong main.cpp (đã được setup):
```cpp
#include "logger.h"

int main(int argc, char* argv[]) {
    Logger& logger = Logger::getInstance();
    logger.log("=== cTetris Application Started ===");
    
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        logger.logError("Cannot initialize SDL3: %s", SDL_GetError());
        return -1;
    }
    
    logger.log("SDL3 initialized successfully (VIDEO | AUDIO)");
    logger.log("Window created: 270x480");
    logger.logEvent("STORY", "Starting game story sequence");
    // ... rest of app
}
```

### Thêm logging vào game modules:

#### gameConsole/app.cpp:
```cpp
#include "logger.h"

int runGameConsole(...) {
    Logger& logger = Logger::getInstance();
    logger.logEvent("CONSOLE", "Menu displayed");
    logger.logEvent("CONSOLE", "Player selected: %s", choice.c_str());
    return nextState;
}
```

#### gameCore/app.cpp (khi cần ghi audio logs):
```cpp
#include "logger.h"

void playBGM(const std::string& track, float volume) {
    Logger& logger = Logger::getInstance();
    logger.logAudio("Playing: %s (volume: %.2f)", track.c_str(), volume);
}
```

## 📂 File Output

**Vị trí log file**: Cùng thư mục với executable
```
D:\projects\ctetris-2\app\build\desktop\windows\
    cTetris.exe
    SDL3.dll
    game.log          ← Log file (tạo khi chạy app)
```

**Format của game.log:**
```
=== cTetris Game Log ===
Started at: 14:30:45.123
14:30:45.125 === cTetris Application Started ===
14:30:45.130 SDL3 initialized successfully (VIDEO | AUDIO)
14:30:45.145 Window created: 270x480
14:30:45.200 [STORY] Starting game story sequence
14:30:46.500 [CONSOLE] Entering main game loop
14:30:47.000 [AUDIO] BGM audio stream playing at volume: 0.50
14:31:00.300 [CONSOLE] Player selected story: 0
```

## 📚 Logger API Reference

### Hàm chính:

```cpp
// General logging
logger.log("Message: %s", value);

// Audio/BGM logging (có [AUDIO] prefix)
logger.logAudio("BGM playing: %s", trackName);

// Game event logging (với module name)
logger.logEvent("CONSOLE", "Player action");
logger.logEvent("CORE", "Score: %d", score);
logger.logEvent("STORY", "Narrative started");

// Error logging (có [ERROR] prefix)
logger.logError("Failed to load: %s", filename);

// Flush to disk immediately
logger.flush();

// Close log file (auto-called on shutdown)
logger.close();
```

## 🛠️ Build Details

### CMake Changes:
- ✅ Thêm `WIN32` flag vào `add_executable()` (ẩn console subsystem)
- ✅ Thêm linker flags `/SUBSYSTEM:WINDOWS` và `/ENTRY:mainCRTStartup`
- ✅ Thêm đường dẫn Windows SDK: `C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/`
- ✅ Thêm `src` vào `target_include_directories()` để include logger.h

### Dependencies:
- SDL3.dll - Tự động copy vào build output
- Windows SDK - Được cấu hình tự động qua CMake

## 🎮 Cách Chạy

1. **Build project** (đã hoàn tất):
   ```powershell
   cd D:\projects\ctetris-2\app
   .\build.ps1 native
   ```

2. **Run executable**:
   ```
   D:\projects\ctetris-2\app\build\desktop\windows\cTetris.exe
   ```
   - Cửa sổ GUI hiển thị
   - Console hoàn toàn ẩn
   - File `game.log` được tạo

3. **Kiểm tra logs**:
   ```
   cat D:\projects\ctetris-2\app\build\desktop\windows\game.log
   ```

## 📋 Checklist Hoàn Tất

- ✅ Console window ẩn (Windows linker flags)
- ✅ Logger header file tạo (src/logger.h)
- ✅ Logger tích hợp vào main.cpp
- ✅ CMakeLists.txt cập nhật với linker flags
- ✅ Executable build thành công (cTetris.exe)
- ✅ SDL3.dll tự động copy vào output
- ✅ Logger hỗ trợ printf-style formatting
- ✅ Logger thread-safe
- ✅ Documentation (LOGGER_USAGE.md + BUILD_SUMMARY.md)

## 🔧 Troubleshooting

### Nếu console vẫn hiển thị:
- Đảm bảo rebuild sau khi sửa CMakeLists.txt
- Kiểm tra linker flags trong build output (tìm `/SUBSYSTEM:WINDOWS`)

### Nếu game.log không được tạo:
- Kiểm tra quyền write trong thư mục output
- Đảm bảo `#include "logger.h"` có trong files
- Kiểm tra dòng `Logger& logger = Logger::getInstance();` có được gọi

### Nếu build fail:
- Đảm bảo Windows SDK cài đặt tại: `C:\Program Files (x86)\Windows Kits\10`
- Run `.\build.ps1 clean` rồi rebuild
- Kiểm tra Visual Studio 2022 Build Tools đã cài đặt

## 📌 Notes

- Logger là **header-only** - có thể include từ bất kỳ .cpp file nào
- Tự động tạo `game.log` khi `getInstance()` được gọi lần đầu
- Tự động đóng file khi ứng dụng thoát (trong destructor)
- Support UTF-8 encoding
- Format timestamp: `HH:MM:SS.mmm` (millisecond precision)

---

**Build Date**: 11/05/2026  
**Status**: ✅ READY FOR DEPLOYMENT
