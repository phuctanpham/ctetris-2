#pragma once
// =============================================================================
// gameConsole_bg_svg.h -- Background image cua man hinh Console.
//
// CHIEN LUOC: nhung SVG truc tiep o compile-time (giong gameStory_logo_svg.h
// va gameStory_corp_svg.h) thay vi load tu file system. Loi the:
//   - WASM khong can --preload-file
//   - Native khong phu thuoc working dir
//   - Re-build moi luc thay svg de capture vao binary
//
// THIET KE: subtle Tetris-themed background -- nen toi (slate #282c34)
// + decorative tetromino silhouettes mau brand (yellow/blue) o 4 goc voi
// opacity thap (0.10) de KHONG che lap nut o vung trung tam (y=160..350).
// Vien giua giu trong de title "C T E T R I S" (y=60), subtitle (y=90)
// va hint text (y=380..425) van ro net.
//
// viewBox 270x480 = ti le 9:16, khop CONSOLE_SCREEN_WIDTH/HEIGHT.
// drawBackground() trong app.cpp se rasterize SVG nay 1 lan (lazy init)
// va render lap cho moi frame.
// =============================================================================
static const char* GAMECONSOLE_BG_SVG_DATA = R"SVG_RAW_DATA(
<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 270 480" width="270" height="480">
  <!-- Nen toi slate, dong nhat voi mau drawBackground hien tai (#282c34 ~ rgb(40,44,52)) -->
  <rect width="270" height="480" fill="#282c34"/>

  <!-- Goc tren-trai: cum 3 khoi vuong yellow + blue, opacity 0.10 -->
  <g opacity="0.10">
    <rect x="10"  y="10"  width="20" height="20" fill="#FFFF00"/>
    <rect x="32"  y="10"  width="20" height="20" fill="#0000FF"/>
    <rect x="10"  y="32"  width="20" height="20" fill="#0000FF"/>
  </g>

  <!-- Goc tren-phai: hinh L-piece quay nguoc, opacity 0.10 -->
  <g opacity="0.10">
    <rect x="218" y="10"  width="20" height="20" fill="#FFFF00"/>
    <rect x="240" y="10"  width="20" height="20" fill="#FFFF00"/>
    <rect x="240" y="32"  width="20" height="20" fill="#0000FF"/>
  </g>

  <!-- Goc duoi-trai: T-piece, opacity 0.10 -->
  <g opacity="0.10">
    <rect x="10"  y="450" width="20" height="20" fill="#0000FF"/>
    <rect x="32"  y="450" width="20" height="20" fill="#FFFF00"/>
    <rect x="32"  y="428" width="20" height="20" fill="#0000FF"/>
  </g>

  <!-- Goc duoi-phai: I-piece dung doc, opacity 0.10 -->
  <g opacity="0.10">
    <rect x="240" y="406" width="20" height="20" fill="#FFFF00"/>
    <rect x="240" y="428" width="20" height="20" fill="#0000FF"/>
    <rect x="240" y="450" width="20" height="20" fill="#FFFF00"/>
  </g>

  <!-- Subtle horizontal accent line ngay duoi subtitle (y=110), opacity 0.20 -->
  <rect x="40" y="110" width="190" height="1" fill="#FFFF00" opacity="0.20"/>

  <!-- Subtle horizontal accent line ngay tren hint text (y=370), opacity 0.20 -->
  <rect x="40" y="370" width="190" height="1" fill="#0000FF" opacity="0.20"/>
</svg>
)SVG_RAW_DATA";
