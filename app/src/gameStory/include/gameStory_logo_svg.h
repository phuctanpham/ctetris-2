#pragma once
// File nay duoc sinh tu dong tu gameStory_logo.svg boi build.sh
// KHONG sua tay -- moi thay doi se bi ghi de o lan build tiep theo.
static const char* LOGO_SVG_DATA = R"SVG_RAW_DATA(
<?xml version="1.0" encoding="UTF-8"?>
<!--
    File nay la LOGO CUA GAME cTetris.
    Da duoc dieu chinh:
    1. Update mau sac theo brand moi (Pure Yellow & Pure Blue).
    2. Thu nho 75% va can giua de tranh bi cat goc (masking) tren macOS/PWA.
-->
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 320 320" width="320" height="320">
  <g transform="translate(40, 40) scale(0.75)">
    <!-- Hang 1 -->
    <rect x="0" y="0" width="100" height="100" fill="#FFFF00"/>
    <rect x="110" y="0" width="100" height="100" fill="#FFFF00"/>
    <rect x="220" y="0" width="100" height="100" fill="#0000FF"/>

    <!-- Hang 2 -->
    <rect x="0" y="110" width="100" height="100" fill="#FFFF00"/>
    <rect x="110" y="110" width="100" height="100" fill="#0000FF"/>
    <rect x="220" y="110" width="100" height="100" fill="#0000FF"/>

    <!-- Hang 3 -->
    <rect x="0" y="220" width="100" height="100" fill="#FFFF00"/>
    <rect x="110" y="220" width="100" height="100" fill="#FFFF00"/>
    <rect x="220" y="220" width="100" height="100" fill="#0000FF"/>
  </g>
</svg>
)SVG_RAW_DATA";