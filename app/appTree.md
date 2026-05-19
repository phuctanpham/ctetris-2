<!-- phucpt: app/appTree.md -->
```text
app/
├── appTree.md
├── main.cpp                 # File tích hợp các modules và tổng hợp thành một chương trình chung
├── CMakeLists.txt
├── build.ps1
├── build.sh
├── brandkit/                # Thư mục chứa các file nhận diện thương hiệu (logo, icon, ...)
│   ├── logo.icns
│   ├── logo.ico
│   ├── logo.svg
│   └── template.svg
├── src/                     # Source code các module chính
│   ├── logger.h
│   ├── gameConsole/         # Module quản lý các khối giao diện và logic tương tác của màn hình cấu hình game
│   │   ├── app.cpp
│   │   └── include/
│   │       ├── gameConsole_bg_svg.h
│   │       ├── gameConsole_db.h
│   │       ├── gameConsole_layout.h
│   │       └── gameConsole_sort.h
│   ├── gameCore/            # Module quản lý các khối giao diện và logic tương tác của màn hình chơi game
│   │   ├── app.cpp
│   │   └── include/
│   │       └── gameCore_layout.h
│   ├── gameStory/           # Module quản lý các khối giao diện và logic tương tác của màn hình giới thiệu game
│   │   ├── app.cpp
│   │   └── include/
│   │       ├── gameStory_corp_svg.h
│   │       ├── gameStory_db.h
│   │       ├── gameStory_layout.h
│   │       └── gameStory_logo_svg.h
│   └── shared/
│       └── nanosvg_impl.cpp
└── web/                     # Thư mục chứa các file web (manifest, shell, service worker)
    ├── manifest.webmanifest
    ├── shell.html
    └── sw.js
```
