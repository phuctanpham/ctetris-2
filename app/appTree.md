<!-- phucpt: app/appTree.md -->
```text
app/
├── main.cpp                 # File tích hợp các modules và tổng hợp thành một chương trình chung
└── src/
    ├── gameConsole/         # Module quản lý các khối giao diện và logic tương tác của màn hình cấu hình game
    │   ├── app.cpp          # File tổng hợp các codeblocks của gameConsole
    │   └── include/         # Các Headers công khai của gameConsole/
    ├── gameCore/            # Module quản lý các khối giao diện và logic tương tác của màn hình chơi game
    │   ├── app.cpp          # File tổng hợp các codeblocks của gameCore
    │   └── include/         # Các Headers công khai của gameCore
    └── gameStory/           # Module quản lý các khối giao diện và logic tương tác của màn hình giới thiệu game
        ├── app.cpp          # File tổng hợp các codeblocks của gameStory
        └── include/         # Các Headers công khai của gameStory
└── CMakeLists.txt
└── build.ps1
└── build.sh
```
