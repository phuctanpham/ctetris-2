<!-- phucpt: app/appTree.md -->
```text
app/
├── main.cpp                 # File tích hợp các modules và tổng hợp thành một chương trình chung
├── CMakeLists.txt
├── build.ps1
├── build.sh
├── appTree.md
├── brandkit/                # Thư mục chứa các file nhận diện thương hiệu (logo, icon, ...)
│   ├── favicon.svg
│   ├── logo.icns
│   ├── logo.ico
│   ├── logo.svg
│   └── template.svg
├── chapters/                # Thư mục chứa các chapter; mỗi chapter nên có thumbnail cho từng story và file SQL để import dữ liệu
│   └── c001/
│       ├── c001.sql         # File SQL import chapter mẫu
│       ├── THUMB_001_FALLING_SKY.png
│       ├── THUMB_002_ORDERCORP_TEMPLE.png
│       ├── THUMB_003_UNDERCITY_SLUM.png
│       ├── THUMB_004_SHADOW_ALLEY.png
│       ├── THUMB_005_HEAVEN_MATRIX.png
│       ├── THUMB_006_GLITCH_VIRUS.png
│       ├── THUMB_007_FROZEN_TIME.png
│       ├── THUMB_008_KILL_SCREEN.png
│       ├── THUMB_009_ENDLESS_LOOP.png
│       └── THUMB_010_GOLDEN_MAXOUT.png
├── build/                   # Thư mục build trung gian
│   └── wasm/
│       └── macos/
│           ├── CMakeCache.txt
│           ├── CMakeFiles/
│           ├── Makefile
│           ├── cTetris.html
│           ├── cTetris.js
│           ├── cTetris.wasm
│           ├── cmake_install.cmake
│           └── favicon.svg
├── libs/                    # Thư viện/phụ trợ bên ngoài
│   └── macos/
│       └── downloads/
│           ├── SDL-3.4.4/
│           │   ├── Android.mk
│           │   ├── ...
│           │   ├── android-project/
│           │   ├── build-scripts/
│           │   ├── build-wasm/
│           │   ├── cmake/
│           │   ├── docs/
│           │   ├── examples/
│           │   ├── include/
│           │   ├── src/
│           │   ├── test/
│           │   ├── VisualC/
│           │   ├── VisualC-GDK/
│           │   ├── Xcode/
│           │   └── wayland-protocols/
│           └── sdl3-wasm-3.4.4/
│               ├── include/
│               ├── lib/
│               └── share/
├── src/                     # Source code các module chính
│   ├── gameConsole/         # Module quản lý các khối giao diện và logic tương tác của màn hình cấu hình game
│   │   ├── app.cpp
│   │   ├── gameConsole_board.json
│   │   ├── task.md
│   │   └── include/
│   │       ├── .gitkeep
│   │       └── gameConsole_layout.h
│   ├── gameCore/            # Module quản lý các khối giao diện và logic tương tác của màn hình chơi game
│   │   ├── app.cpp
│   │   ├── task.md
│   │   └── include/
│   │       ├── .gitkeep
│   │       └── gameCore_layout.h
│   └── gameStory/           # Module quản lý các khối giao diện và logic tương tác của màn hình giới thiệu game
│       ├── app.cpp
│       ├── task.md
│       └── include/
│           ├── gameStory_corp_svg.h
│           ├── gameStory_layout.h
│           ├── gameStory_logo_svg.h
│           ├── nanosvg.h
│           └── nanosvgrast.h
└── web/                     # Thư mục chứa các file web (manifest, shell, service worker)
    ├── manifest.webmanifest
    ├── shell.html
    └── sw.js
```
