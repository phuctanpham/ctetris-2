Confirmed 8 tables. Writing the full, source-accurate SDS now.

---

### `doc/src/content/06-gameConsoleEngineering.tex` — full file

```latex
% !TEX TS-program = xelatex
% !TEX encoding = UTF-8 Unicode
\documentclass[12pt,a4paper]{article}

\IfFileExists{src/libs/body.tex}{\input{src/libs/body.tex}}{\input{../libs/body.tex}}
\IfFileExists{src/libs/header.tex}{\input{src/libs/header.tex}}{\input{../libs/header.tex}}
\IfFileExists{src/libs/footer.tex}{\input{src/libs/footer.tex}}{\input{../libs/footer.tex}}
\usepackage{docmute}
\usepackage{tikz}
\usetikzlibrary{shapes.geometric,arrows.meta,positioning,fit,backgrounds,calc}
\usepackage{enumitem}
\usepackage{tabularx}
\usepackage{amsmath}

% ═══════════════════════════════════════════════════════
% SDS Framework macros (embedded — no external file)
% Lesson: DeclareRobustCommand + texorpdfstring; no tikz-uml
% ═══════════════════════════════════════════════════════
\definecolor{pillarBlue}{RGB}{30,90,160}
\definecolor{versionGreen}{RGB}{20,130,80}
\definecolor{versionAmber}{RGB}{190,120,10}
\definecolor{versionRed}{RGB}{170,40,40}
\definecolor{boxGray}{RGB}{240,242,245}

\tikzset{
  sdsBox/.style={rectangle,rounded corners=3pt,draw=#1,fill=boxGray,
                 text width=3.0cm,align=center,minimum height=0.85cm,
                 font=\small\bfseries},
  sdsArrow/.style={-{Stealth[length=5pt]},thick,#1},
  vTag/.style={rectangle,rounded corners=2pt,fill=#1,text=white,
               font=\footnotesize\bfseries,inner sep=3pt},
  layerBox/.style={rectangle,draw=pillarBlue!50,fill=pillarBlue!8,
                   rounded corners=4pt,minimum width=9cm,
                   minimum height=0.7cm,align=center,font=\small},
}
\newcommand{\PartPillars}{%
  \vspace{4pt}%
  {\color{pillarBlue}\rule{\linewidth}{1.5pt}}\\[2pt]%
  {\large\bfseries\color{pillarBlue}PHẦN I \quad Trụ cột kỹ thuật (Technical Pillars)}\\[-2pt]%
  {\color{pillarBlue}\rule{\linewidth}{0.4pt}}\vspace{2pt}}
\newcommand{\PartVersions}{%
  \vspace{10pt}%
  {\color{versionGreen}\rule{\linewidth}{1.5pt}}\\[2pt]%
  {\large\bfseries\color{versionGreen}PHẦN II \quad Lộ trình phiên bản (Versioned Development)}\\[-2pt]%
  {\color{versionGreen}\rule{\linewidth}{0.4pt}}\vspace{2pt}}
\DeclareRobustCommand{\vbadge}[2]{%
  \tikz[baseline=-2pt]\node[vTag={#1}]{#2};~}
\newcommand{\Vone}{\texorpdfstring{\vbadge{versionGreen}{V1}}{[V1]}}
\newcommand{\Vtwo}{\texorpdfstring{\vbadge{versionAmber}{V2}}{[V2]}}
\newcommand{\Vthree}{\texorpdfstring{\vbadge{versionRed}{V3}}{[V3]}}
\newcommand{\ThreeLayerArch}[4]{%
  \begin{center}\begin{tikzpicture}[node distance=0.35cm]
    \node[layerBox,fill=pillarBlue!18,draw=pillarBlue](L1){#2};
    \node[layerBox,below=of L1](L2){#3};
    \node[layerBox,fill=versionGreen!12,draw=versionGreen!60,below=of L2](L3){#4};
    \draw[sdsArrow=pillarBlue!70](L1)--(L2);
    \draw[sdsArrow=versionGreen!70](L2)--(L3);
    \node[left=0.18cm of L1,font=\tiny\itshape,text=gray]{UI / Entry};
    \node[left=0.18cm of L2,font=\tiny\itshape,text=gray]{Logic};
    \node[left=0.18cm of L3,font=\tiny\itshape,text=gray]{Data};
  \end{tikzpicture}\end{center}}
\newenvironment{VersionTable}{%
  \vspace{4pt}\small
  \begin{tabular}{@{}c p{3cm} p{3.5cm} p{3.5cm}@{}}
  \hline
  \rowcolor{pillarBlue!15}
  \textbf{Ver}&\textbf{Mục tiêu}&\textbf{Công nghệ}&\textbf{Tích hợp}\\
  \hline}{\hline\end{tabular}\vspace{4pt}}
% ═══════════════════════════════════════════════════════

\hypersetup{pdftitle={SDS - gameConsole Module}}

\begin{document}
\providecommand{\StandaloneSectionSetup}[2]{\ApplyStandardFooter{#1}{#2}}
\StandaloneSectionSetup{06-gameConsoleEngineering.tex}{LastPageGameConsoleEngineering}
\onehalfspacing

\section{SDS --- Module \texttt{gameConsole}}

\PartPillars

% ─────────────────────────────────────────────────────────
\subsection{Trách nhiệm \& Phạm vi}

\textbf{gameConsole} là \emph{hub screen} và \textbf{DB owner} của toàn
ứng dụng: tạo schema \texttt{default.sqlite} (8 bảng, 3 nhóm), quản lý
navigation flag-based, và làm trung gian truyền \texttt{SettingsConfig}
giữa \texttt{main.cpp} và \texttt{runGameCore()}.

\noindent\textbf{Return code contract} (duy nhất giao tiếp ra ngoài):

\begin{center}\small
\begin{tabular}{@{}c p{4.0cm} p{5.5cm}@{}}
\hline
\textbf{Code} & \textbf{Điều kiện kích hoạt} &
\textbf{Hành động trong \texttt{main.cpp}} \\
\hline
\texttt{0} & QUIT click / window close &
  \texttt{break} --- thoát app \\
\texttt{1} & PLAY click &
  \texttt{runGameStory(cfg.storyId,cfg.chapterId)} $\rightarrow$
  \texttt{runGameCore(cfg)} \\
\texttt{2} & Play btn trong Stories popup &
  \texttt{runGameStory(cfg.storyId,cfg.chapterId)} $\rightarrow$
  \texttt{continue} về Console \\
\texttt{3} & DB file không tồn tại / \texttt{shared\_data} rỗng &
  \texttt{runGameStory(0,0)} (init+sync) $\rightarrow$
  \texttt{continue} \\
\hline
\end{tabular}
\end{center}

\noindent\textbf{Quyền sở hữu DB:}

\begin{center}\small
\begin{tabular}{@{}l l l@{}}
\hline
\textbf{Nhóm} & \textbf{Bảng} & \textbf{Quyền} \\
\hline
Group 1 & \texttt{default\_Records}, \texttt{default\_Stories},
          \texttt{default\_Settings} & R/W (owner) \\
Group 2 & \texttt{shared\_data}, \texttt{shared\_dialogues},
          \texttt{shared\_choices}, \texttt{shared\_meta}
        & Tạo schema; R duy nhất (gameStory ghi) \\
Group 3 & \texttt{sync\_Records} &
  Tạo schema; seed 30 rows; R cho Board popup \\
\hline
\end{tabular}
\end{center}

% ─────────────────────────────────────────────────────────
\subsection{Kiến trúc hệ thống}

\ThreeLayerArch{gameConsole}%
  {Presentation --- \texttt{drawBackground} · \texttt{draw*Lightbox}
   · \texttt{drawButton} · \texttt{drawSB}}%
  {Logic --- \texttt{AppState} (flag machine) · \texttt{SortEngine::sort}
   · \texttt{dbCheckAndUnlockStories} · event routing}%
  {Data --- \texttt{DBLayer} · SQLite \texttt{default.sqlite}
   · \texttt{SDL\_AudioStream} · nanosvg rasterizer}

\noindent\textbf{Biểu đồ thành phần:}

\begin{center}
\begin{tikzpicture}[
  comp/.style={rectangle,draw=pillarBlue,fill=pillarBlue!8,rounded corners=3pt,
               minimum width=2.7cm,minimum height=0.65cm,align=center,font=\small},
  ext/.style={rectangle,draw=gray!55,fill=gray!8,rounded corners=2pt,
              minimum width=2.3cm,minimum height=0.6cm,align=center,font=\scriptsize},
  ref/.style={rectangle,draw=versionGreen,fill=versionGreen!8,rounded corners=2pt,
              minimum width=2.3cm,minimum height=0.6cm,align=center,font=\scriptsize},
  iface/.style={-{Stealth[length=4pt]},thick},
  use/.style={-{Stealth[length=4pt]},dashed,gray!70}
]
  \node[ref,draw=versionGreen](main)   at (0,0)     {\texttt{main.cpp}};
  \node[comp](entry) at (3.8,0)                     {\texttt{runGame}\\\texttt{Console()}};
  \node[comp](app)   at (3.8,-1.6)                  {\texttt{AppState}\\(flags + cfg*)};
  \node[comp](rend)  at (0,-1.6)                    {\texttt{draw*}\\functions};
  \node[comp](db)    at (7.6,-1.6)                  {\texttt{DBLayer}\\(dbOpen..Save)};
  \node[comp](sort)  at (7.6,0)                     {\texttt{SortEngine}\\\texttt{::sort()}};
  \node[ext] (sq)    at (7.6,-3.2)                  {SQLite\\\texttt{default.sqlite}};
  \node[ext] (svg)   at (0,-3.2)                    {nanosvg\\rasterizer};
  \node[ext] (sdl)   at (3.8,-3.2)                  {SDL3 Events\\+ AudioStream};

  \draw[iface](main)--node[above,font=\tiny]{\texttt{cfg\&}}(entry);
  \draw[iface](entry)--(app);
  \draw[iface](app)--(rend)  node[above,font=\tiny,midway]{node,flags};
  \draw[iface](app)--(db)    node[above,font=\tiny,midway]{CRUD};
  \draw[iface](entry)--(sort)node[above,font=\tiny,midway]{arr,n,cmp};
  \draw[iface](db)--(sq);
  \draw[use](rend)--(svg);
  \draw[use](rend)--(sdl);
  \draw[use](app.south)to[bend right=10](sdl.north);
\end{tikzpicture}
\end{center}

\noindent\textbf{Flag-based render pipeline} ---
\texttt{AppState} không dùng ScreenStack mà dùng 5 boolean flags
kiểm soát render độc lập:

\begin{center}
\begin{tikzpicture}[
  b/.style={sdsBox=pillarBlue!60,text width=3.0cm},
  t/.style={sdsBox=versionGreen!60,text width=3.0cm},
  arr/.style={sdsArrow=pillarBlue}
]
  \node[b](p0){each frame:\\\texttt{SDL\_PollEvent}};
  \node[b,right=0.5cm of p0](p1){\texttt{drawBackground}\\(SVG lazy-init)};
  \node[b,right=0.5cm of p1](p2){\texttt{!any flag}\\$\rightarrow$ draw\\6 buttons};
  \node[b,right=0.5cm of p2](p3){\texttt{showGuide}\\$\rightarrow$\\GuideLightbox};
  \node[t,below=0.6cm of p3](p4){\texttt{showBoard}\\$\rightarrow$\\BoardLightbox};
  \node[t,below=0.6cm of p2](p5){\texttt{showSettings}\\$\rightarrow$\\SettingsLightbox};
  \node[t,below=0.6cm of p1](p6){\texttt{showStories}\\$\rightarrow$\\StoriesLightbox};
  \node[b,below=0.6cm of p0](p7){\texttt{wasmShutdown}\\$\rightarrow$\\ReloadScreen};

  \foreach \x/\y in {p0/p1,p1/p2,p2/p3}{\draw[arr](\x)--(\y);}
  \draw[arr,dashed,versionGreen](p2.south)--(p5);
  \draw[arr,dashed,versionGreen](p1.south)--(p6);
  \draw[arr,dashed,versionGreen](p0.south)--(p7);
  \draw[arr,dashed,versionGreen](p3.south)--(p4);
  \node[font=\tiny,gray] at (5.4,-1.8){flags are mutually exclusive at runtime};
\end{tikzpicture}
\end{center}

% ─────────────────────────────────────────────────────────
\subsection{Thuật toán cốt lõi}

\subsubsection{Startup Guard \& DB Init Sequence}

\begin{center}
\begin{tikzpicture}[
  proc/.style={rectangle,draw=pillarBlue,fill=pillarBlue!8,rounded corners=2pt,
               minimum width=4.8cm,minimum height=0.55cm,align=center,font=\scriptsize},
  dec/.style={diamond,draw=versionAmber!80,fill=versionAmber!8,
              aspect=3.0,align=center,font=\scriptsize},
  ret/.style={rectangle,rounded corners=3pt,draw=versionRed!60,fill=versionRed!8,
              minimum width=2.4cm,minimum height=0.5cm,align=center,font=\scriptsize},
  ok/.style={rectangle,rounded corners=3pt,draw=versionGreen!70,fill=versionGreen!10,
             minimum width=4.8cm,minimum height=0.5cm,align=center,font=\scriptsize},
  arr/.style={-{Stealth[length=4pt]},thick}
]
  \node[proc](p0) at (0,0)   {probe \texttt{SDL\_GetPrefPath+idUser.sqlite} size $> 100$\,B};
  \node[dec] (d0) at (0,-1.0){file\\exists?};
  \node[ret] (r3a)at (4.5,-1.0){return \texttt{3}\\(no DB)};
  \node[proc](p1) at (0,-2.0){WASM: \texttt{idbfs\_mount\_dir} + \texttt{idbfs\_load\_from\_idb}};
  \node[proc](p2) at (0,-3.0){\texttt{dbOpen("default")} $\rightarrow$ \texttt{sqlite3\_open}};
  \node[proc](p3) at (0,-4.0){\texttt{dbInitSchema()} --- \texttt{CREATE TABLE IF NOT EXISTS} $\times 8$};
  \node[proc](p4) at (0,-5.0){\texttt{dbSeedSharedData()} --- \texttt{SELECT COUNT(*) FROM shared\_data}};
  \node[dec] (d1) at (0,-6.0){rows\\$>0$?};
  \node[ret] (r3b)at (4.5,-6.0){return \texttt{3}\\(missing data)};
  \node[proc](p5) at (0,-7.0){\texttt{dbLoadSettings(cfgInOut)} + \texttt{applyVolumeToStream}};
  \node[proc](p6) at (0,-8.0){\texttt{dbCheckAndUnlockStories("default")}};
  \node[proc](p7) at (0,-9.0){restore \texttt{isSelected=1} story $\rightarrow$ \texttt{cfgInOut.storyId}};
  \node[ok]  (p8) at (0,-10.0){\texttt{loadBoardWithFallback()} + \texttt{applyBoardSort(SCORE\_DESC)}};

  \draw[arr](p0)--(d0);
  \draw[arr](d0)--node[above,font=\tiny]{no}(r3a);
  \draw[arr](d0)--node[left,font=\tiny]{yes}(p1);
  \draw[arr](p1)--(p2);\draw[arr](p2)--(p3);\draw[arr](p3)--(p4);\draw[arr](p4)--(d1);
  \draw[arr](d1)--node[above,font=\tiny]{no}(r3b);
  \draw[arr](d1)--node[left,font=\tiny]{yes}(p5);
  \draw[arr](p5)--(p6);\draw[arr](p6)--(p7);\draw[arr](p7)--(p8);
\end{tikzpicture}
\end{center}

\subsubsection{Smart Sort Router --- \texttt{applyBoardSort()}}

\begin{center}
\begin{tikzpicture}[
  b/.style={sdsBox=pillarBlue!60,text width=2.6cm},
  dec/.style={diamond,draw=versionAmber!80,fill=versionAmber!8,
              aspect=2.6,align=center,font=\scriptsize},
  arr/.style={-{Stealth[length=4pt]},thick}
]
  \node[b](in){BoardSortMode\\enum input};
  \node[b,right=0.5cm of in](b1){build\\comparator\\lambda};
  \node[b,right=0.5cm of b1](b2){\texttt{SortEngine}::\\\texttt{sort(arr, n,}\\cmp, CTX\_DEFAULT)};
  \node[dec,right=0.5cm of b2](d0){$n \leq 64$?};
  \node[b,above right=0.4cm and 0.5cm of d0](ins){Insertion\\Sort\\near-O(n)};
  \node[b,below right=0.4cm and 0.5cm of d0](intro){IntroSort\\(Quick+Heap\\+Insertion)};
  \node[b,right=1.2cm of d0](out){\texttt{boardScroll=0}\\log algo\\name};

  \foreach \x/\y in {in/b1,b1/b2,b2/d0}{\draw[arr](\x)--(\y);}
  \draw[arr](d0)--node[above,font=\tiny]{yes}(ins);
  \draw[arr](d0)--node[below,font=\tiny]{no}(intro);
  \draw[arr](ins.east)to[bend left=15](out.north);
  \draw[arr](intro.east)to[bend right=15](out.south);
\end{tikzpicture}
\end{center}

\noindent Board leaderboard: $n = 30$--$50$ rows $\Rightarrow$ router selects
\textbf{Insertion Sort} every call. Four modes use same router:
\texttt{SCORE\_DESC/ASC} compares \texttt{BoardEntry.score};
\texttt{TIME\_DESC/ASC} compares \texttt{BoardEntry.timeEpoch} (Unix epoch UTC,
\textbf{not} display string) for correct chronological ordering.

\subsubsection{Story Unlock Cascade --- \texttt{dbCheckAndUnlockStories()}}

\begin{center}
\begin{tikzpicture}[
  proc/.style={rectangle,draw=pillarBlue,fill=pillarBlue!8,rounded corners=2pt,
               minimum width=4.5cm,minimum height=0.55cm,align=center,font=\scriptsize},
  dec/.style={diamond,draw=versionAmber!80,fill=versionAmber!8,
              aspect=2.8,align=center,font=\scriptsize},
  arr/.style={-{Stealth[length=4pt]},thick}
]
  \node[proc](q0) at (0,0)   {\texttt{SELECT} \texttt{shared\_data} WHERE
                               \texttt{requiredStories != ''}};
  \node[proc](q1) at (0,-1.0){parse CSV \texttt{requiredStories}
                               $\rightarrow$ \texttt{parents[]}};
  \node[proc](q2) at (0,-2.0){\texttt{lookupParent}: fetch
                               \texttt{lastMaxScore}, \texttt{Speed}, \texttt{Retries}};
  \node[dec] (d0) at (0,-3.0){all parents\\satisfy child\\min thresholds?};
  \node[proc](d1) at (4.2,-3.0){skip $\rightarrow$ next\\candidate};
  \node[dec] (d2) at (0,-4.2){already\\activated?};
  \node[proc](d3) at (4.2,-4.2){skip\\(idempotent)};
  \node[proc](q3) at (0,-5.2){\texttt{INSERT \ldots ON CONFLICT DO UPDATE}
                               \texttt{isActivated=1}};
  \node[proc](q4) at (0,-6.2){\texttt{unlocked++}};
  \node[proc](q5) at (0,-7.2){after loop: \texttt{unlocked > 0}
                               $\rightarrow$ \texttt{dbSyncToPersistent()}};

  \draw[arr](q0)--(q1);\draw[arr](q1)--(q2);\draw[arr](q2)--(d0);
  \draw[arr](d0)--node[above,font=\tiny]{no}(d1);
  \draw[arr](d0)--node[left,font=\tiny]{yes}(d2);
  \draw[arr](d2)--node[above,font=\tiny]{yes}(d3);
  \draw[arr](d2)--node[left,font=\tiny]{no}(q3);
  \draw[arr](q3)--(q4);\draw[arr](q4)--(q5);
  \draw[arr,gray,dashed](d1.east)--++(0.4,0)--++(0,1.6)--
      node[right,font=\tiny]{next ch}++(-0.4,0)--(q1.east);
  \draw[arr,gray,dashed](d3.east)--++(0.4,0)--++(0,1.0)--
      node[right,font=\tiny]{next ch}++(-0.4,0)--(q1.east);
\end{tikzpicture}
\end{center}

\subsubsection{Scrollbar Interaction Model}

\noindent\textbf{\texttt{SBLayout} geometry formulas:}
\[
  \texttt{thumbH} = \max\!\Bigl(16,\; \texttt{track.h} \times
    \tfrac{\texttt{visible}}{\texttt{total}}\Bigr),
  \quad
  \texttt{thumbY} = \texttt{track.y} +
    \frac{(\texttt{track.h} - \texttt{thumbH}) \times \texttt{pos}}
         {\texttt{total} - \texttt{visible}}
\]

\noindent\textbf{Event $\rightarrow$ action mapping:}

\begin{center}\small
\begin{tabular}{@{}l l l@{}}
\hline
\textbf{SDL Event} & \textbf{Hit zone} & \textbf{Action} \\
\hline
\texttt{MOUSE\_BUTTON\_DOWN} & \texttt{upBtn} &
  \texttt{scrollPos--}; start auto-repeat (300\,ms delay, 60\,ms interval) \\
\texttt{MOUSE\_BUTTON\_DOWN} & \texttt{downBtn} &
  \texttt{scrollPos++}; start auto-repeat \\
\texttt{MOUSE\_BUTTON\_DOWN} & \texttt{thumb} &
  \texttt{dragging=true}; record \texttt{dragOffsetY = my - thumb.y} \\
\texttt{MOUSE\_BUTTON\_DOWN} & \texttt{track} (not thumb) &
  \texttt{scrollPos = (my - track.y) / track.h * (total - visible)} \\
\texttt{MOUSE\_MOTION} & \textit{(while \texttt{dragging})} &
  \texttt{ratio = (my - dragOffsetY - track.y) / (track.h - thumbH)};
  \texttt{scrollPos = ratio * (total - visible)} \\
\texttt{MOUSE\_BUTTON\_UP} & any &
  reset: \texttt{upHeld=downHeld=dragging=false} \\
\texttt{MOUSE\_WHEEL} & popup open &
  \texttt{scrollPos -= wheel.y} \\
\hline
\end{tabular}
\end{center}

\noindent\textbf{Volume slider} (Settings popup) reuses the same drag-ratio formula
on the X-axis: $\texttt{cfg.volume} = (\texttt{mx} - \texttt{VOL\_TRACK.x}
- \texttt{VOL\_THUMB\_W}/2) / (\texttt{VOL\_TRACK.w} - \texttt{VOL\_THUMB\_W})$,
clamped to $[0,1]$.

\subsubsection{Settings Serialize / Deserialize --- \texttt{dbSaveSettings} /
\texttt{dbLoadSettings}}

\noindent\textbf{Persisted keys} (10 rows in \texttt{default\_Settings}):

\begin{center}\small
\begin{tabular}{@{}l l l@{}}
\hline
\textbf{\texttt{key}} & \textbf{\texttt{value} type} &
\textbf{Field in \texttt{SettingsConfig}} \\
\hline
\texttt{"volume"}            & float string (\texttt{"\%.4f"}) & \texttt{volume} \\
\texttt{"color0"}\ldots\texttt{"color6"} & \texttt{"0"}/\texttt{"1"} &
  \texttt{colorEnabled[0..6]} \\
\texttt{"storyId"}           & int string & \texttt{storyId} \\
\texttt{"chapterId"}         & int string & \texttt{chapterId} \\
\hline
\end{tabular}
\end{center}

\noindent\textbf{NOT persisted:} \texttt{nextBlockScore}, \texttt{nextBlockSpeed},
\texttt{tableMatrix} --- these are runtime fields set from Stories popup and
passed directly to \texttt{runGameCore()} via the shared \texttt{SettingsConfig\&}.

% ─────────────────────────────────────────────────────────
\subsection{Cấu trúc dữ liệu}

\subsubsection{C++ Structs}

\begin{verbatim}
// gameConsole_layout.h — cross-module contract
struct SettingsConfig {
    float       volume          = 0.5f;
    bool        colorEnabled[7] = {true,...,true};
    int         storyId         = 0;
    int         chapterId       = 0;
    // runtime-only (not persisted in Settings):
    int         nextBlockScore  = 0;
    float       nextBlockSpeed  = 0.0f;
    std::string tableMatrix     = "";
};

// app.cpp internal
enum BoardSortMode { BOARD_SORT_SCORE_DESC=0, BOARD_SORT_SCORE_ASC,
                     BOARD_SORT_TIME_DESC, BOARD_SORT_TIME_ASC };
struct BoardEntry  { std::string user; int score;
                     std::string time;      // display "MM-DD HH:MM"
                     int64_t     timeEpoch; // Unix epoch UTC for sort };
struct AppState {
    bool           showGuide=false, showBoard=false,
                   showSettings=false, showStories=false,
                   wasmShutdown=false, isRunning=true;
    int            nextScene=0, boardScroll=0, guideScroll=0;
    int            focusIndex=0;
    BoardSortMode  boardSortMode=BOARD_SORT_SCORE_DESC;
    SBInteraction  sb;
    SettingsConfig* cfg=nullptr;   // borrowed ptr — NOT owned
    bool           draggingVolume=false;
    // Stories:
    std::vector<StoryRow> storiesCache;
    int storiesCacheChapter=-1, storiesMaxChapter=1, currentStoryChapter=1;
};
struct SvgTexture { SDL_Texture* texture=nullptr; int w=0, h=0; };
struct SBLayout   { SDL_FRect upBtn, track, downBtn, thumb; };
struct SBInteraction { bool upHeld=false, downHeld=false, dragging=false;
                       float dragOffsetY=0.0f;
                       Uint32 lastAutoScroll=0, nextAutoStart=0; };
\end{verbatim}

\subsubsection{SQLite ERD --- \texttt{default.sqlite} (8 bảng, 3 nhóm)}

\begin{center}
% Lesson: define \pk locally to avoid redefinition clash
\newcommand{\pk}[1]{\underline{#1}}
\begin{tikzpicture}[
  g1/.style={rectangle,draw=pillarBlue,fill=pillarBlue!7,rounded corners=2pt,
             align=left,font=\scriptsize,inner sep=4pt,minimum width=3.5cm},
  g2/.style={rectangle,draw=gray!55,fill=gray!6,rounded corners=2pt,
             align=left,font=\scriptsize,inner sep=4pt,minimum width=3.5cm},
  g3/.style={rectangle,draw=versionAmber,fill=versionAmber!8,rounded corners=2pt,
             align=left,font=\scriptsize,inner sep=4pt,minimum width=3.5cm},
  rel/.style={-{Stealth[length=4pt]},thick,pillarBlue!70},
  g2rel/.style={-{Stealth[length=4pt]},thick,gray!60},
  lbl/.style={font=\tiny,fill=white,inner sep=1pt}
]
  % ── Group 1 (left column) ──
  \node[g1](rec) at (0,0) {
    \textbf{default\_Records}\ \textit{\tiny(R/W)}\\[1pt]
    \pk{id}\ \texttt{INT AUTOINCREMENT}\\
    idUser\ \texttt{TEXT}\\
    startTS,\ endTS\ \texttt{INTEGER}\\
    idStory,\ idChapter\ \texttt{INT}\\
    totalScore,\ totalSeconds\\
    avgSpeed\ \texttt{REAL},\ retryNo
  };
  \node[g1,below=0.4cm of rec](sto) {
    \textbf{default\_Stories}\ \textit{\tiny(R/W)}\\[1pt]
    \pk{idUser,\ idStory,\ idChapter}\\
    isActivated,\ isSelected\ \texttt{INT}\\
    totalRetries,\ lastMaxScore\\
    lastMaxSpeed\ \texttt{REAL}
  };
  \node[g1,below=0.4cm of sto](set) {
    \textbf{default\_Settings}\ \textit{\tiny(R/W)}\\[1pt]
    \pk{key}\ \texttt{TEXT}\\
    value\ \texttt{TEXT}\\
    \textit{\tiny 10 rows: volume, color0-6,}\\
    \textit{\tiny storyId, chapterId}
  };

  % ── Group 2 (centre column) ──
  \node[g2](sd) at (5.6,0) {
    \textbf{shared\_data}\ \textit{\tiny(R gameConsole)}\\[1pt]
    \pk{idStory,\ idChapter}\\
    storyName,\ chapterName\\
    minScore,\ minSpeed,\ minRetries\\
    requiredStories\ \texttt{TEXT (CSV)}\\
    nextBlockScore,\ nextBlockSpeed\\
    tableMatrix,\ thumbnailPath
  };
  \node[g2,below=0.4cm of sd](sdlg) {
    \textbf{shared\_dialogues}\ \textit{\tiny(R gameStory)}\\[1pt]
    UNIQUE(\texttt{idStory,idChapter,nodeId})\\
    speaker,\ text,\ imageUrl\\
    bgmUrl,\ sfxUrl\\
    nextNodeId,\ hasChoices
  };
  \node[g2,below=0.4cm of sdlg](scho) {
    \textbf{shared\_choices}\ \textit{\tiny(R gameStory)}\\[1pt]
    UNIQUE(\texttt{idStory,idChapter,}\\
    \texttt{nodeId,choiceIdx})\\
    label,\ nextNodeId
  };
  \node[g2,below=0.4cm of scho](smet) {
    \textbf{shared\_meta}\ \textit{\tiny(R gameConsole)}\\[1pt]
    \pk{chapter\_id}\ \texttt{TEXT}\\
    sha,\ updated\_at\\
    media\_base\_url
  };

  % ── Group 3 (right column) ──
  \node[g3](sync) at (11.2,0) {
    \textbf{sync\_Records}\ \textit{\tiny(Cloudflare D1)}\\[1pt]
    \pk{id}\ \texttt{INT AUTOINCREMENT}\\
    nameUser\ \texttt{TEXT}\\
    totalScore,\ totalSeconds\\
    avgSpeed\ \texttt{REAL}\\
    endTS\ \texttt{INT}\\
    idStory,\ idChapter\\
    \textit{\tiny seed: 30 rows from FALLBACK}
  };

  % ── relationships ──
  \draw[rel](sd.west)to[bend right=8]
      node[lbl,pos=0.35]{1}node[lbl,pos=0.8]{N}(sto.east);
  \draw[rel](sd.west)to[bend right=20]
      node[lbl,pos=0.3]{1}node[lbl,pos=0.78]{N}(rec.east);
  \draw[g2rel](sd.south)--node[lbl,right]{1}node[lbl,pos=0.85]{N}(sdlg.north);
  \draw[g2rel](sdlg.south)--node[lbl,right]{1}node[lbl,pos=0.85]{N}(scho.north);
  \draw[g2rel,dashed](sd.south)to[bend left=10]
      node[lbl,left,pos=0.4]{logical}(smet.north);
  % sync_Records feeds Board display (no FK — data sink)
  \draw[-{Stealth[length=4pt]},thick,versionAmber!70,dashed]
      (sync.south)--++(0,-0.7)
      node[below,font=\tiny,draw=versionAmber!40,fill=versionAmber!5,
           rounded corners,inner sep=2pt]
      {Board popup: \texttt{SELECT} ORDER BY score/endTS};

  % group labels
  \node[font=\scriptsize\bfseries,pillarBlue] at (0,-5.6)
      {Group 1 --- Per-user R/W};
  \node[font=\scriptsize\bfseries,gray!60] at (5.6,-7.4)
      {Group 2 --- Shared (schema owner: gameConsole)};
  \node[font=\scriptsize\bfseries,versionAmber] at (11.2,-2.2)
      {Group 3 --- D1 sink};
\end{tikzpicture}
\end{center}

\subsubsection{Dataflow Diagram --- Console Main Loop}

\begin{center}
\begin{tikzpicture}[
  proc/.style={rectangle,draw=pillarBlue,fill=pillarBlue!8,rounded corners=2pt,
               minimum width=2.5cm,minimum height=0.6cm,align=center,font=\scriptsize},
  store/.style={cylinder,draw=gray!60,fill=gray!10,shape border rotate=90,
                minimum width=2.2cm,minimum height=0.55cm,align=center,font=\scriptsize},
  extio/.style={ellipse,draw=gray!50,fill=gray!6,minimum width=1.8cm,
                minimum height=0.5cm,align=center,font=\scriptsize},
  fl/.style={-{Stealth[length=4pt]},thick},
  dfl/.style={-{Stealth[length=4pt]},dashed,gray!60}
]
  % external actors
  \node[extio](ev)    at (0,5.5)   {SDL\\Events};
  \node[extio](main2) at (0,0)     {\texttt{main.cpp}};

  % processes
  \node[proc](router) at (3.5,5.5) {Event\\Router};
  \node[proc](flags)  at (3.5,3.8) {AppState\\flag update};
  \node[proc](dblay)  at (7.0,5.5) {DBLayer\\CRUD};
  \node[proc](sort2)  at (7.0,3.8) {SortEngine\\applyBoardSort};
  \node[proc](rend2)  at (3.5,1.8) {Screen\\Renderer};
  \node[proc](audio)  at (7.0,1.8) {AudioStream\\gain update};
  \node[extio](ret)   at (3.5,0)   {return\\0/1/2/3};

  % stores
  \node[store](sett2) at (10.5,5.5){default\\\_Settings};
  \node[store](sdat2) at (10.5,3.8){shared\_data\\(stories list)};
  \node[store](srec)  at (10.5,1.8){sync\_Records\\(board data)};

  % flows
  \draw[fl](ev)--node[above,font=\tiny]{key/mouse}(router);
  \draw[fl](router)--node[left,font=\tiny]{popup\\flags}(flags);
  \draw[fl](router)--node[above,font=\tiny]{CRUD call}(dblay);
  \draw[fl](dblay)--node[above,font=\tiny]{R/W}(sett2);
  \draw[dfl](dblay.south)--node[right,font=\tiny]{R stories}(sdat2);
  \draw[dfl](dblay.south)to[bend right=5]
      node[right,font=\tiny]{R board}(srec);
  \draw[fl](dblay.south)to[bend left=8]
      node[left,font=\tiny]{sort trigger}(sort2);
  \draw[fl](sort2)--node[right,font=\tiny]{sorted\\g\_board[]}(flags.east);
  \draw[fl](flags)--node[left,font=\tiny]{render\\state}(rend2);
  \draw[fl](flags.east)to[bend right=12]
      node[below right,font=\tiny]{vol}(audio.north);
  \draw[fl](rend2)--node[above,font=\tiny]{SDL\_RenderPresent}(ret);
  \draw[fl](ret)--node[right,font=\tiny]{nextScene}(main2);
\end{tikzpicture}
\end{center}

\subsubsection{Sequence Diagram --- PLAY path \& STORIES path}

\begin{center}
\begin{tikzpicture}[
  lhd/.style={rectangle,fill=pillarBlue!12,draw=pillarBlue,font=\scriptsize,
              inner sep=3pt,minimum width=1.8cm,align=center},
  msg/.style={-{Stealth[length=4pt]},thick},
  ret/.style={{Stealth[length=4pt]}-,thick,dashed,gray!70},
  lf/.style={draw=gray!40,dashed}
]
  \node[lhd](H1) at (0,0)    {\texttt{main.cpp}};
  \node[lhd](H2) at (3.2,0)  {\texttt{runGame}\\\texttt{Console()}};
  \node[lhd](H3) at (6.4,0)  {\texttt{DBLayer}};
  \node[lhd](H4) at (9.6,0)  {\texttt{Sort}\\\texttt{Engine}};
  \node[lhd](H5) at (12.8,0) {\texttt{Screen}\\\texttt{Renderer}};

  \foreach \x in {0,3.2,6.4,9.6,12.8}{\draw[lf](\x,-0.3)--(\x,-10.2);}

  \draw[gray!40,thick,dashed](-0.8,-5.6)--(14,-5.6);
  \node[font=\scriptsize\itshape,gray] at (-0.5,-1.0) {PLAY};
  \node[font=\scriptsize\itshape,gray] at (-0.5,-6.3) {STORIES};

  % ── PLAY path ──
  \draw[msg](0,-1.0)--(3.2,-1.0)
      node[above,font=\tiny,midway]{runGameConsole(win,ren,cfg)};
  \draw[msg](3.2,-1.6)--(6.4,-1.6)
      node[above,font=\tiny,midway]{dbOpen+Init+Seed+Load};
  \draw[ret](6.4,-2.1)--(3.2,-2.1)
      node[above,font=\tiny,midway]{cfgInOut updated};
  \draw[msg](3.2,-2.5)--(9.6,-2.5)
      node[above,font=\tiny,midway]{loadBoard+applySort(SCORE\_DESC)};
  \draw[ret](9.6,-3.0)--(3.2,-3.0)
      node[above,font=\tiny,midway]{g\_board[] sorted};
  \draw[msg](3.2,-3.4)--(12.8,-3.4)
      node[above,font=\tiny,midway]{drawBackground + drawButtons (loop)};
  \node[font=\tiny,gray] at (8,-3.9){\textit{user navigates, opens Guide/Board...}};
  \draw[msg](3.2,-4.3)--(6.4,-4.3)
      node[above,font=\tiny,midway]{dbSaveSettings(cfg) on popup close};
  \draw[msg](3.2,-4.8)--(6.4,-4.8)
      node[above,font=\tiny,midway]{user clicks PLAY};
  \draw[ret](6.4,-5.2)--(0,-5.2)
      node[above,font=\tiny,midway]{return 1, cfg.storyId/chapterId set};

  % ── STORIES path ──
  \draw[msg](0,-6.3)--(3.2,-6.3)
      node[above,font=\tiny,midway]{runGameConsole(win,ren,cfg)};
  \draw[msg](3.2,-6.9)--(3.2,-6.9);
  \draw[msg](3.2,-7.3)--(6.4,-7.3)
      node[above,font=\tiny,midway]{showStories=true\\dbLoadStories("default",ch)};
  \draw[ret](6.4,-7.8)--(3.2,-7.8)
      node[above,font=\tiny,midway]{storiesCache[] (StoryRow)};
  \draw[msg](3.2,-8.2)--(12.8,-8.2)
      node[above,font=\tiny,midway]{drawStoriesLightbox (3-state rows)};
  \node[font=\tiny,gray] at (8,-8.6){\textit{user clicks Play btn on activated row}};
  \draw[msg](3.2,-9.0)--(6.4,-9.0)
      node[above,font=\tiny,midway]{dbSelectStory("default",N,C)};
  \draw[msg](3.2,-9.5)--(3.2,-9.5);
  \node[font=\tiny,align=center] at (5.5,-9.8){\textit{cfg.storyId=N, cfg.chapterId=C,}\\
    \textit{nextBlockScore, nextBlockSpeed, tableMatrix set}};
  \draw[ret](6.4,-10.1)--(0,-10.1)
      node[above,font=\tiny,midway]{return 2, cfg updated};
\end{tikzpicture}
\end{center}

% ─────────────────────────────────────────────────────────
\subsection{Giao diện công khai}

\begin{verbatim}
// extern declared in app/main.cpp
int runGameConsole(SDL_Window*    window,
                   SDL_Renderer*  renderer,
                   SettingsConfig& cfgInOut);
// Returns: 0=quit | 1=play | 2=story-preview | 3=no-DB
// cfgInOut: READ at entry (restore saved settings);
//           WRITTEN during session (user changes volume/colors/storyId);
//           CALLER (main.cpp) passes SAME ref to runGameCore().

// gameConsole_layout.h — also included by gameCore
struct SettingsConfig { ... };   // see §2.4
\end{verbatim}

\noindent\textbf{DB init idempotency contract:}
\texttt{dbOpen("default")} + \texttt{dbInitSchema()} safe to call on
every \texttt{runGameConsole} entry including re-entries after
gameCore back-to-console; all \texttt{CREATE TABLE IF NOT EXISTS}
are no-ops when schema exists.

% ─────────────────────────────────────────────────────────
\subsection{Thư viện phụ thuộc}

\begin{tabularx}{\linewidth}{@{}l l X@{}}
\hline
\textbf{Thư viện} & \textbf{Guard} & \textbf{Vai trò} \\
\hline
SDL3              & always & Window, renderer, events, \texttt{SDL\_AudioStream} \\
sqlite3           & always & R/W \texttt{default.sqlite}; schema init; 8-table schema \\
nlohmann/json     & always & \texttt{gameConsole\_board.json} fallback parse \\
nanosvg / nanosvgrast & always & SVG background rasterize; impl in
                                  \texttt{nanosvg\_impl.cpp} \\
\texttt{gameConsole\_sort.h} & header-only & Smart Sort Engine v2.0 (7 algos) \\
Emscripten IDBFS  & \texttt{\#ifdef \_\_EMSCRIPTEN\_\_} &
  \texttt{idbfs\_mount\_dir}, \texttt{load/save\_from\_idb};
  Asyncify (\texttt{-sASYNCIFY=1}) \\
libcurl           & V3 (native) & Cloudflare D1 REST push/pull \\
emscripten\_fetch & V3 (WASM)   & Cloudflare D1 REST (WASM path) \\
\hline
\end{tabularx}

\PartVersions

\begin{VersionTable}
\Vone & Flag-based UI shell; SVG background; scrollbar; BUILD\_STANDALONE
      & SDL3, nanosvg
      & Feeds \texttt{SettingsConfig} to \texttt{runGameCore()} \\
\Vtwo & SQLite 8-table schema; SettingsConfig persist; Smart Sort;
        Stories popup; IDBFS WASM
      & sqlite3, nlohmann/json, emscripten IDBFS
      & Shared DB with gameStory (shared\_*); gameCore writes
        \texttt{default\_Records} \\
\Vthree & Cloudflare D1 leaderboard sync; OTP auth; \texttt{d1\_users}
        & libcurl / emscripten\_fetch, Cloudflare Worker
        & D1 Worker syncs \texttt{sync\_Records}; LOAD/SAVE via OTP \\
\end{VersionTable}

% ─────────────────────────────────────────────────────────
\subsection{\Vone\ --- Static UI Shell}

\textbf{Mục tiêu:} 6 nút điều hướng, SVG background lazy-init, scrollbar
tương tác, \texttt{BUILD\_STANDALONE}.

\noindent\textbf{Screen layout pixel-map (270×480):}

\begin{center}
\begin{tikzpicture}[x=0.72pt,y=0.72pt,every node/.style={font=\tiny}]
  \draw[thick](0,0) rectangle (270,480);
  % title band
  \fill[pillarBlue!15](0,440) rectangle (270,480);
  \draw[pillarBlue!40](0,440)--(270,440);
  \node at (135,460){\textbf{C T E T R I S -- GAME CONSOLE} (y=440..480)};
  % 6 buttons  y-coords inverted: display y0=130 → canvas y = 480-130-30=320
  \fill[pillarBlue!25](65,290) rectangle (205,320);
  \draw(65,290) rectangle (205,320); \node at (135,305){GUIDE (y=130..160)};
  \fill[versionGreen!25](65,250) rectangle (205,280);
  \draw(65,250) rectangle (205,280); \node at (135,265){BOARD (y=170..200)};
  \fill[versionRed!25](65,210) rectangle (205,240);
  \draw(65,210) rectangle (205,240); \node at (135,225){PLAY (y=210..240)};
  \fill[versionAmber!30](65,170) rectangle (205,200);
  \draw(65,170) rectangle (205,200); \node at (135,185){STORIES (y=250..280)};
  \fill[purple!15](65,130) rectangle (205,160);
  \draw(65,130) rectangle (205,160); \node at (135,145){SETTINGS (y=290..320)};
  \fill[gray!30](65,90) rectangle (205,120);
  \draw(65,90) rectangle (205,120); \node at (135,105){QUIT (y=330..360)};
  % hint text band
  \fill[gray!10](0,40) rectangle (270,85);
  \draw[gray!30](0,40)--(270,40);
  \node[align=center] at (135,62){hint text: TAB/WASD/ESC keys (y=380..425)};
  % SVG tetromino corners
  \fill[versionAmber!20](0,0) rectangle (35,35);
  \node at (17,17){SVG corner};
  \fill[versionAmber!20](235,0) rectangle (270,35);
  \node at (252,17){SVG corner};
  % axis labels
  \node[gray] at (258,474){y=0};
  \node[gray] at (258,6){y=480};
\end{tikzpicture}
\end{center}

\noindent\textbf{SVG background lazy-init contract:}
\texttt{g\_consoleBg.texture == nullptr} on first frame $\rightarrow$
\texttt{loadSvgTextureFromMem(renderer, GAMECONSOLE\_BG\_SVG\_DATA, 270)}
$\rightarrow$ store in \texttt{g\_consoleBg}. On \texttt{runGameConsole}
exit: \texttt{SDL\_DestroyTexture} + reset \texttt{g\_consoleBg =
\{nullptr,0,0\}} so re-entry rebuilds cleanly.

\noindent\textbf{I/O validation:}
\begin{itemize}[nosep]
  \item PLAY click $\rightarrow$ \texttt{runGameConsole} returns \texttt{1};
        \texttt{cfgInOut.volume == 0.5f} (default, no DB).
  \item ESC when no popup open $\rightarrow$ \texttt{focusIndex == 5} (QUIT);
        no popup flag is \texttt{true}.
  \item ESC when popup open $\rightarrow$ popup flag becomes \texttt{false};
        \texttt{focusIndex} unchanged.
  \item \texttt{BUILD\_STANDALONE}: \texttt{runGameConsole} called directly
        from \texttt{main()}; returns \texttt{0} on QUIT.
\end{itemize}

% ─────────────────────────────────────────────────────────
\subsection{\Vtwo\ --- SQLite Persistence + Smart Sort}

\textbf{Mục tiêu:} 8-table schema; SettingsConfig persist; 4-mode Smart Sort;
Stories popup DB-driven; IDBFS WASM flush.

\noindent\textbf{DB path formula:}
$\texttt{SDL\_GetPrefPath("uit","cTetris")} + \texttt{idUser} + \texttt{".sqlite"}$
$\Rightarrow$ \texttt{default.sqlite}.
User table names: \texttt{g\_dbCurrentUser + "\_" + suffix}
$\Rightarrow$ \texttt{default\_Records}, \texttt{default\_Stories},
\texttt{default\_Settings}.

\noindent\textbf{Settings key mapping} (10 rows, see §2.3.5):
\texttt{volume} (4 decimal places); \texttt{color0}\ldots\texttt{color6}
(\texttt{"0"}/\texttt{"1"}); \texttt{storyId}, \texttt{chapterId}.
Range clamp on load: \texttt{volume} $\in [0,1]$.
\texttt{nextBlockScore/Speed/tableMatrix} are NOT stored --- set at runtime
from Stories popup row data.

\noindent\textbf{Stories popup 3-state row classification:}
\begin{center}\small
\begin{tabular}{@{}l l l@{}}
\hline
\textbf{State} & \textbf{Condition} & \textbf{Visual} \\
\hline
LOCKED    & \texttt{isActivated == false}
          & dim bg, dim radio, no Play btn, muted text \\
ACTIVE    & \texttt{isActivated \&\& totalRetries==0 \&\& lastMaxScore==0}
          & normal row, white radio, green Play btn \\
COMPLETED & \texttt{isActivated \&\& (totalRetries>0 || lastMaxScore>0)}
          & green-tint bg, filled radio, green text \\
\hline
\end{tabular}
\end{center}

\noindent\textbf{IDBFS write protocol} (WASM only): every
\texttt{dbSyncToPersistent()} call $\Rightarrow$
\texttt{idbfs\_save\_to\_idb()} via Asyncify.
Called after: \texttt{dbSaveSettings}, \texttt{dbInsertRecord},
\texttt{dbUpsertStoryProgress}, \texttt{dbSelectStory},
\texttt{dbCheckAndUnlockStories} (only if \texttt{unlocked > 0}).
Native: \texttt{dbSyncToPersistent()} is a compile-time no-op.

\noindent\textbf{I/O validation:}
\begin{itemize}[nosep]
  \item Set \texttt{volume=0.3} in Settings $\rightarrow$ close popup
        $\rightarrow$ \texttt{SELECT value FROM default\_Settings
        WHERE key='volume'} returns \texttt{"0.3000"}.
  \item Click SCORE header in Board $\rightarrow$ sort mode cycles
        SCORE\_DESC $\rightarrow$ SCORE\_ASC; log contains algo name
        \texttt{"insertion"} ($n=30$).
  \item \texttt{colorEnabled[i]=false} for 6 colors; attempt to
        toggle last color $\rightarrow$ denied;
        \texttt{countEnabled(cfg) >= 1} guaranteed.
  \item DB probe returns \texttt{false} (file $\leq 100$\,B)
        $\rightarrow$ \texttt{runGameConsole} returns \texttt{3}
        without rendering any frame.
\end{itemize}

% ─────────────────────────────────────────────────────────
\subsection{\Vthree\ --- Cloudflare D1 + OTP Auth}

\textbf{Mục tiêu:} \texttt{sync\_Records} pushed to D1 after game-over;
global leaderboard pulled by Board popup; LOAD/SAVE via OTP
(\texttt{d1\_users}).

\noindent\textbf{Cloudflare Worker API (D1 path):}
\begin{itemize}[nosep]
  \item \textbf{Push}: \texttt{POST /api/records} JSON
        \texttt{\{nameUser, totalScore, totalSeconds, avgSpeed, endTS,
        idStory, idChapter\}} $\rightarrow$ Worker inserts into D1
        \texttt{sync\_Records}; also writes to local \texttt{sync\_Records}
        (dual-write for offline fallback).
  \item \textbf{Pull}: \texttt{GET /api/leaderboard?limit=50\&sort=score}
        $\rightarrow$ replaces \texttt{sync\_Records} local rows.
  \item \textbf{Offline}: fetch timeout / error $\rightarrow$ log
        \texttt{"[gameConsole] D1 offline, local fallback"};
        Board popup uses local \texttt{sync\_Records} or
        \texttt{FALLBACK\_BOARD\_ROWS} if empty.
\end{itemize}

\noindent\textbf{OTP auth flow (LOAD/SAVE Settings button):}
User enters email $\rightarrow$ Worker sends OTP $\rightarrow$ user enters
code $\rightarrow$ Worker validates $\rightarrow$ returns JWT $\rightarrow$
\texttt{default.sqlite} upload/download to \texttt{d1\_users}.

\noindent\textbf{I/O validation:}
\begin{itemize}[nosep]
  \item HTTP 200 from D1 pull $\rightarrow$ \texttt{SELECT COUNT(*) FROM
        sync\_Records} $\geq 1$; Board popup shows at least 1 real entry.
  \item Offline: \texttt{loadBoardWithFallback()} returns exactly
        $\texttt{sizeof(FALLBACK\_BOARD\_ROWS)/sizeof(...)=30}$ entries.
  \item WASM: close tab after game-over $\rightarrow$ reopen
        $\rightarrow$ \texttt{default\_Records} row persists (IDBFS).
\end{itemize}

\label{LastPageGameConsoleEngineering}
\end{document}
```

---

### Patch 2 — `03-gameConsoleRequirements.tex`

```latex
% ── REPLACE \subsubsection{Kiến trúc Cơ sở dữ liệu (SQLite — 3 nhóm bảng)} ──
% Remove the three-column tikzpicture group diagram + surrounding text
% Replace with:

\subsubsection{Kiến trúc Cơ sở dữ liệu (SQLite --- 3 nhóm bảng)}
\textit{ERD đầy đủ với DDL-level precision (8 bảng, tên thực
\texttt{default\_Records}/\texttt{default\_Stories}/\texttt{default\_Settings},
foreign-key relationships, 3 ownership groups) được đặc tả trong
\textbf{SDS §06-gameConsoleEngineering, mục 2.4}.}

SRS chỉ phát biểu ràng buộc schema:
\begin{itemize}[nosep]
  \item File: \texttt{SDL\_GetPrefPath("uit","cTetris")} $+$
        \texttt{idUser} $+$ \texttt{".sqlite"}; mặc định \texttt{default.sqlite}.
  \item User-prefix tables: \texttt{g\_dbCurrentUser + "\_" + suffix}
        (Group 1 --- R/W owner).
  \item \texttt{shared\_*}: tạo schema bởi gameConsole, ghi bởi gameStory
        (Group 2 --- read-only cho gameConsole).
  \item \texttt{sync\_Records}: sink từ Cloudflare D1, seed 30 rows khi init
        (Group 3).
  \item \texttt{dbInitSchema()} idempotent --- \texttt{CREATE TABLE IF NOT EXISTS}
        cho cả 8 bảng.
\end{itemize}

% ── REPLACE \subsubsection{Hợp đồng dữ liệu SettingsConfig} ─────────────────
% Remove the three-node tikzpicture (runGameConsole → SettingsConfig → runGameCore)
% Replace with:

\subsubsection{Hợp đồng dữ liệu \texttt{SettingsConfig} (Cross-module Contract)}
\textit{Component diagram và sequence diagram của \texttt{SettingsConfig}
flow (Console $\leftrightarrow$ Core $\leftrightarrow$ DBLayer) được đặc tả trong
\textbf{SDS §06-gameConsoleEngineering, mục 2.5}.}

SRS phát biểu ràng buộc contract:
\begin{itemize}[nosep]
  \item \textbf{Persisted fields} (10 rows in \texttt{default\_Settings}):
        \texttt{volume}, \texttt{color0-6}, \texttt{storyId}, \texttt{chapterId}.
  \item \textbf{Runtime-only fields} (NOT persisted):
        \texttt{nextBlockScore}, \texttt{nextBlockSpeed}, \texttt{tableMatrix} ---
        set from Stories popup, forwarded to \texttt{runGameCore()}.
  \item Single \texttt{volume} field (no separate bgm/sfx split in V2).
  \item \texttt{SettingsConfig\&} is a \textbf{borrowed pointer} inside
        \texttt{AppState::cfg} --- \texttt{runGameConsole()} does not own it.
\end{itemize}
```

---

### Patch 3 — `09-gameConsoleGuide.tex`

```latex
% ── ADD fbox BEFORE \subsection{Hình nền động -- v2} ─────────────────────────
\noindent\fbox{\begin{minipage}{0.95\linewidth}
\textit{Tham khảo kỹ thuật:} SVG lazy-init algorithm (rasterize-once,
\texttt{g\_consoleBg} reset trên re-entry, fallback solid fill),
pixel-map layout 270×480, và 6-button y-coordinate table được đặc tả
trong \textbf{SDS §06-gameConsoleEngineering, mục 3.1 (V1 screen layout)}.
\end{minipage}}
\vspace{6pt}

% ── ADD fbox BEFORE \subsection{Scrollbar tương tác} ─────────────────────────
\noindent\fbox{\begin{minipage}{0.95\linewidth}
\textit{Tham khảo kỹ thuật:} \texttt{SBLayout} geometry formulas
(thumbH, thumbY), event$\rightarrow$action mapping (drag, track-click,
auto-repeat 300\,ms/60\,ms), và volume slider X-axis drag formula
được đặc tả trong \textbf{SDS §06-gameConsoleEngineering, mục 2.3.4}.
\end{minipage}}
\vspace{6pt}

% ── ADD fbox BEFORE the sort paragraph inside \subsection{Popup Bảng xếp hạng}
% (before the line: "\noindent\textbf{Thêm v2 -- sắp xếp động:}")
\noindent\fbox{\begin{minipage}{0.95\linewidth}
\textit{Tham khảo kỹ thuật:} Smart Sort Router decision tree (7 algorithms,
CTX\_DEFAULT: $n \leq 64 \rightarrow$ Insertion, $n > 64 \rightarrow$
IntroSort), \texttt{BoardEntry.timeEpoch} vs display string for TIME sort,
và \texttt{applyBoardSort()} log contract được đặc tả trong
\textbf{SDS §06-gameConsoleEngineering, mục 2.3.2}.
\end{minipage}}
\vspace{6pt}

% ── ADD fbox BEFORE \subsection{Giới hạn còn lại sau v2} ─────────────────────
\noindent\fbox{\begin{minipage}{0.95\linewidth}
\textit{Tham khảo kỹ thuật:} Story unlock cascade algorithm
(\texttt{dbCheckAndUnlockStories} --- CSV parse, multi-parent check,
idempotent UPSERT), startup guard sequence (DB probe $\rightarrow$ return 3),
và dataflow diagram được đặc tả trong
\textbf{SDS §06-gameConsoleEngineering, mục 2.3.3 và 2.4}.
\end{minipage}}
\vspace{6pt}
```

---

**Corrections from source vs earlier draft:**

| Field | Earlier draft (wrong) | Actual source |
|---|---|---|
| `SettingsConfig` | `bgmVolume`, `sfxVolume`, `fullScreen`, `targetFPS` | Single `volume`; `colorEnabled[7]`; `nextBlockScore/Speed/tableMatrix` |
| DB filename | `default.sqlite` (hardcoded) | `{idUser}.sqlite` via `userTable()` prefix |
| Screen navigation | ScreenStack push/pop | 5 boolean flags in `AppState` |
| Table count | 9 | 8 (`3+4+1`) |
| Sort target | string timestamp | `timeEpoch` (`int64_t` Unix epoch UTC) |
| Not-persisted fields | assumed all cfg fields | `nextBlockScore`, `nextBlockSpeed`, `tableMatrix` runtime-only |
| DB guard | `dbOpen` first | Probe file size `>100B` BEFORE `dbOpen`; two `return 3` paths |