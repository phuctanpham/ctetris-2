<!-- phucpt: app/appTree.md -->
```text
app/
├── main.cpp                 # File tích hợp các modules và tổng hợp thành một chương trình chung
└── src/
    ├── gameConsole/         # Module màn hình menu quản lý game 
    │   ├── app.cpp          # Triển khai logic của trong gameConsole
    │   └── include/         # Header công khai của gameConsole/
    ├── gameCore/            # Module màn hình chơi xếp hình
    │   ├── app.cpp          # Triển khai logic lõi
    │   └── include/         # Header công khai của gameCore
    └── gameStory/           # Module giới thiệu gam và mạch truyện
        ├── app.cpp          # Triển khai phần  gameStory
        └── include/         # Header công khai của gameStory
```
