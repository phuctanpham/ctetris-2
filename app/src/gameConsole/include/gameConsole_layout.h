#pragma once
// gameconsole-tao-giao-dien-169-00
const int CONSOLE_SCREEN_WIDTH  = 270;
const int CONSOLE_SCREEN_HEIGHT = 480;

// =============================================================================
// SettingsConfig -- contract giua gameConsole va gameCore.
// Day la "data bag" duy nhat dung de chuyen cac tuy chinh nguoi choi
// (volume, color palette, story selection) tu Console sang Core.
//
// LUU Y: KHONG add/remove/rename field giua chung vong V2 -- moi thay doi
// tai day deu pha vo signature cua runGameCore() va main.cpp tich hop.
//
// Field meanings:
//   volume          -- am luong tong [0.0f .. 1.0f], default 0.5f
//   colorEnabled[7] -- bat/tat 7 mau khoi (red, orange, pink, yellow,
//                      green, blue, purple) theo thu tu BLOCK_PALETTE.
//                      Yeu cau: it nhat 1 mau phai = true tai moi thoi
//                      diem (Core dua vao day de chon mau khoi).
//   storyId         -- id tuyen truyen dang chon (0 = default/khong story)
//   chapterId       -- id chuong cua tuyen truyen tren (0 = chuong 1)
// =============================================================================
struct SettingsConfig {
    float volume          = 0.5f;
    bool  colorEnabled[7] = { true, true, true, true, true, true, true };
    int   storyId         = 0;
    int   chapterId       = 0;
};
