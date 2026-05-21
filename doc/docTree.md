<!-- phucpt: doc/docTree.md -->
```text
doc/
├── docTree.md
├── main.tex            # File build báo cáo đầy đủ
├── assets/             # Hình ảnh, logo SVG
└── src/
    ├── appendix/       # Thư mục chứa các phụ lục
    │   ├── 01-teamContract.pdf
    │   ├── 01-teamContract.tex
    │   ├── 02-teamContractValidation.tex
    │   ├── 03-meetingMinute.tex
    │   ├── 04-teamConflictSolving.tex
    │   ├── 05-cvPhucpt.pdf
    │   ├── 06-cvTruongtm.pdf
    │   ├── 07-cvKiennt.pdf
    │   ├── 08-cvThaidg.pdf
    │   ├── 09-cvVinnt.pdf
    │   └── 09-cvVint.pdf
    ├── appendix.tex    # File build Tổng hợp các tài liệu từ src/appendix/*.tex
    ├── content/        # Thư mục chứa các tài liệu chính
    │   ├── 00-Appendix.tex
    │   ├── 00-References.tex
    │   ├── 01-Preface.tex
    │   ├── 02-gameStoryRequirements.tex
    │   ├── 03-gameConsoleRequirements.tex
    │   ├── 04-gameCoreRequirements.tex
    │   ├── 05-gameStoryEngineering.tex
    │   ├── 06-gameConsoleEngineering.tex
    │   ├── 07-gameCoreEngineering.tex
    │   ├── 08-gameStoryGuide.tex
    │   ├── 09-gameConsoleGuide.tex
    │   ├── 10-gameCoreGuide.tex
    │   ├── 11-testStrategy.tex
    │   ├── 12-testList.tex
    │   ├── 13-productMilestones.tex
    │   ├── 14-projectGanttChart.tex
    │   ├── 15-technologyAdoption.tex
    │   ├── 16-toolsetList.tex
    │   └── 17-skillsetList.tex
    ├── content.tex     # File build Tổng hợp các tài liệu từ src/content/*.tex
    ├── cover.tex       # File build trang bìa độc lập
    ├── libs/           # Các thư viện định dạng bố cục dùng chung
    │   ├── body.tex
    │   ├── header.tex
    │   ├── footer.tex
    │   └── index.tex
```

Không biên dịch trực tiếp các tệp trong `src/libs/`. Đây là các khối dùng chung cho mọi tài liệu.