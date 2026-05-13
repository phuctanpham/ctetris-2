#pragma once
// =============================================================================
// gameStory_db.h — SQLite read layer for story dialogue data
// =============================================================================
// Data contract for gameStory V2 dialogue engine (Phase C/D in taskStory.md).
// Implementation lives in app/src/gameStory/app.cpp.
//
// gameStory opens the SAME sqlite file as gameConsole (default.sqlite) but
// uses a separate sqlite3* handle and only reads — never writes — the
// story_dialogues and story_choices tables.
//
// Tables (created by gameConsole::dbInitSchema, seeded by dbSeedSharedData):
//   story_dialogues  : per-node dialogue rows (idStory, idChapter, nodeId, ...)
//   story_choices    : branching choice rows  (idStory, idChapter, nodeId, ...)
// =============================================================================

#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// DialogueNode — one frame of the dialogue state machine.
// Loaded in bulk for a (storyId, chapterId) pair by storyDbLoadDialogue().
// ---------------------------------------------------------------------------
struct DialogueNode {
    int         nodeId     = 0;
    std::string speaker;        // character name shown in dialogue box header
    std::string text;           // dialogue line (max ~200 chars for display box)
    std::string imageUrl;       // filename only (e.g. "THUMB_001_FALLING_SKY.png")
    std::string bgmUrl;         // filename only, empty = no change
    std::string sfxUrl;         // filename only, empty = no sfx
    int         nextNodeId = 0; // 0 = end of dialogue; else advance to that nodeId
    bool        hasChoices = false; // true = load choices from story_choices
};

// ---------------------------------------------------------------------------
// DialogueChoice — one option in a branching node.
// Loaded on demand for a specific (storyId, chapterId, nodeId) triple.
// ---------------------------------------------------------------------------
struct DialogueChoice {
    int         choiceIdx  = 0; // 0-based index (display order)
    std::string label;          // button text shown to player
    int         nextNodeId = 0; // node to jump to when this choice is selected
};

// ---------------------------------------------------------------------------
// API — implementations in app/src/gameStory/app.cpp
// ---------------------------------------------------------------------------

// Open (or reuse) the sqlite DB at SDL_GetPrefPath("uit","cTetris")+"default.sqlite".
// Returns true on success. Safe to call multiple times — no-op if already open.
bool storyDbOpen();

// Close the sqlite handle. Safe to call when not open.
void storyDbClose();

// Load all dialogue nodes for (idStory, idChapter) ordered by nodeId.
// Returns empty vector on error or if no rows exist.
std::vector<DialogueNode> storyDbLoadDialogue(int idStory, int idChapter);

// Load all choices for a specific (idStory, idChapter, nodeId) triple,
// ordered by choiceIdx. Returns empty vector if node has no choices.
std::vector<DialogueChoice> storyDbLoadChoices(int idStory, int idChapter,
                                                int nodeId);