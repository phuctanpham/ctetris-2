<!-- phucpt: doc/docTree.md -->
```text
doc/
├── main.tex            # File build báo cáo đầy đủ
├── assets/             # Hình ảnh, logo SVG
└── src/
    ├── cover.tex       # File build trang bìa độc lập
    ├── content.tex     # File build Tổng hợp các tài liệu từ src/content/*.tex
    ├── appendix.tex    # File build Tổng hợp các tài liệu từ src/appendix/*.tex
    ├── libs/           # Các thư viện định dạng bố cục dùng chung
    │   ├── body.tex
    │   ├── header.tex
    │   ├── footer.tex
    │   └── index.tex
    ├── content/        # Thư mục chứa các tài liệu chính
    └── appendix/       # Thư mục chứa các phụ lục
```

Không biên dịch trực tiếp các tệp trong `src/libs/`. Đây là các khối dùng chung cho mọi tài liệu.