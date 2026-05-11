# cTetris Logger - Hướng Dẫn Sử Dụng

## Tổng Quan

File `src/logger.h` cung cấp một logging system singleton để ghi các sự kiện game vào file `game.log` thay vì dùng `std::cout` hay `SDL_Log`.

**Lợi ích:**
- ✅ Ghi log vào file `game.log` (output folder cùng thư mục với .exe)
- ✅ Thread-safe với mutex
- ✅ Hỗ trợ printf-style formatting
- ✅ Tự động thêm timestamp cho mỗi log entry
- ✅ Singleton pattern - có thể gọi từ bất kỳ file nào

## Cách Sử Dụng

### 1. Include Header

```cpp
#include "logger.h"
```

### 2. Lấy Instance (Singleton)

```cpp
Logger& logger = Logger::getInstance();
```

### 3. Ghi Log

#### General Logging
```cpp
logger.log("Message: %s", "hello");
logger.log("Value: %d", 42);
```

#### Audio/BGM Logging
```cpp
logger.logAudio("BGM audio stream playing at volume: %.2f", volume);
logger.logAudio("Sound effect triggered: jump");
```

#### Game Event Logging (với module name)
```cpp
logger.logEvent("CONSOLE", "Player selected story: %d", storyId);
logger.logEvent("CORE", "Game Over - Score: %d", score);
logger.logEvent("STORY", "Starting narrative sequence");
```

#### Error Logging
```cpp
logger.logError("Failed to load asset: %s", filename);
logger.logError("SDL initialization failed: %s", SDL_GetError());
```

### 4. Flush & Close

```cpp
logger.flush();  // Đẩy buffer vào disk ngay lập tức
logger.close();  // Đóng file log
```

## Output Format

File `game.log` sẽ trông như thế này:

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
14:31:02.000 [CORE] Starting game session
14:31:15.500 [CORE] Game Over - Score: 1250
14:31:16.000 [CONSOLE] Returning to console
14:31:30.200 === cTetris Application Shutdown ===
```

## Integration vào Game Modules

### Trong gameConsole/app.cpp
```cpp
#include "logger.h"

int runGameConsole(...) {
    Logger& logger = Logger::getInstance();
    logger.logEvent("CONSOLE", "Menu displayed");
    logger.logEvent("CONSOLE", "Player selected: %s", choice.c_str());
    return nextState;
}
```

### Trong gameCore/app.cpp
```cpp
#include "logger.h"

int runGameCore(...) {
    Logger& logger = Logger::getInstance();
    logger.logEvent("CORE", "Game started");
    logger.logEvent("CORE", "Current score: %d", score);
    logger.logEvent("CORE", "Game Over - Final score: %d", finalScore);
    return result;
}
```

### Với Audio/BGM (gameStory hoặc gameConsole)
```cpp
#include "logger.h"

void playBGM(const std::string& trackName, float volume) {
    Logger& logger = Logger::getInstance();
    logger.logAudio("Playing: %s (volume: %.2f)", trackName.c_str(), volume);
    // ... implement audio playback
}
```

## Cấu Trúc Chi Tiết

### Hàm log()
```cpp
void log(const char* fmt, ...);
```
- Dùng cho thông tin chung
- Định dạng: `[timestamp] message`

### Hàm logAudio()
```cpp
void logAudio(const char* fmt, ...);
```
- Dùng cho audio/BGM events
- Định dạng: `[timestamp] [AUDIO] message`

### Hàm logEvent()
```cpp
void logEvent(const char* module, const char* fmt, ...);
```
- `module`: "CONSOLE", "CORE", "STORY", v.v.
- Định dạng: `[timestamp] [module] message`

### Hàm logError()
```cpp
void logError(const char* fmt, ...);
```
- Dùng cho error messages
- Định dạng: `[timestamp] [ERROR] message`

## Thread Safety

Logger là **thread-safe** nhờ `std::mutex`. Bạn có thể gọi từ bất kỳ thread nào:

```cpp
// Thread 1
logger.logEvent("AUDIO", "Stream 1 playing");

// Thread 2 (cùng lúc)
logger.logEvent("CORE", "Game logic update");

// Cả hai sẽ được ghi an toàn vào file
```

## File Output

- **Vị trí**: Cùng thư mục với executable (.exe)
- **Tên file**: `game.log`
- **Encoding**: UTF-8
- **Mode**: Ghi (truncate nếu file đã tồn tại)

## Notes

1. Logger được khởi tạo lần đầu khi `getInstance()` được gọi
2. Constructor tự động tạo/truncate `game.log`
3. Destructor tự động đóng file
4. Không cần gọi `close()` - diễn ra tự động
5. `flush()` có thể dùng khi cần ghi ngay vào disk (hữu ích trước khi crash)

## Ví Dụ Đầy Đủ

```cpp
#include "logger.h"
#include <iostream>

int main() {
    Logger& logger = Logger::getInstance();
    
    logger.log("Game initialization started");
    
    int score = 1500;
    float volume = 0.75f;
    
    logger.logEvent("INIT", "Loading assets");
    logger.logAudio("BGM playing at volume: %.2f", volume);
    logger.log("Score initialized: %d", score);
    logger.logEvent("CORE", "Game loop started");
    
    // Simulate some gameplay
    score += 100;
    logger.logEvent("CORE", "Score updated: %d", score);
    
    logger.logEvent("CORE", "Game ended with score: %d", score);
    logger.log("Game shutdown");
    
    logger.close();
    return 0;
}
```

Output trong `game.log`:
```
=== cTetris Game Log ===
Started at: 14:35:20.450
14:35:20.451 Game initialization started
14:35:20.452 [INIT] Loading assets
14:35:20.453 [AUDIO] BGM playing at volume: 0.75
14:35:20.454 Score initialized: 1500
14:35:20.455 [CORE] Game loop started
14:35:20.500 [CORE] Score updated: 1600
14:35:21.200 [CORE] Game ended with score: 1600
14:35:21.201 Game shutdown
```
