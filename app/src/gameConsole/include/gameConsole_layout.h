#pragma once
#include <string>
// gameconsole-tao-giao-dien-169-00
const int CONSOLE_SCREEN_WIDTH  = 270;
const int CONSOLE_SCREEN_HEIGHT = 480;

struct SettingsConfig {
    float       volume            = 0.5f;
    bool        colorEnabled[7]   = { true, true, true, true, true, true, true };
    int         storyId           = 0;
    int         chapterId         = 0;
    int         nextBlockScore    = 0;
    float       nextBlockSpeed    = 0.0f;
    std::string tableMatrix       = "";
};
