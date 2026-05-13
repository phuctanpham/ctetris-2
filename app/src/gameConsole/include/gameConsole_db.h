#pragma once
// =============================================================================
// gameConsole_db.h -- SQLite wrapper for Stories / Records / Catalogue
// =============================================================================
// Data contract for Task 2.6 (Phase F). Implementation lives in app.cpp.
// Schema mirrors task.md spec:
//   idUser_Records  : per-session game log (one row per game-over)
//   idUser_Stories  : per-user progress overlay (isActivated, isSelected, ...)
//   shared_data     : static story catalogue (seeded V2, synced V3 from MongoDB)
// =============================================================================

#include <string>
#include <vector>
#include <cstdint>
#include "gameConsole_layout.h"   // SettingsConfig (for dbSaveSettings/dbLoadSettings)

// ---------------------------------------------------------------------------
// Story row -- one record from shared_data joined with idUser_Stories overlay.
// loadStories(idChapter) returns rows where shared_data.idChapter = idChapter.
// ---------------------------------------------------------------------------
struct StoryRow {
    // shared_data (catalogue)
    int         idStory       = 0;
    std::string storyName;
    int         idChapter     = 0;
    std::string chapterName;
    int         minScore      = 0;
    float       minSpeed      = 0.0f;
    int         minRetries    = 0;
    std::string requiredStories;   // CSV of idStory prerequisites
    int         nextBlockScore = 0;
    float       nextBlockSpeed = 0.0f;
    std::string tableMatrix;       // serialized init board for gameCore
    std::string xmlDialogue;       // dialogue script for gameStory
    std::string thumbnailPath;     // SVG path or asset key

    // idUser_Stories (user overlay -- joined at load)
    bool        isActivated   = false;
    bool        isSelected    = false;
    int         totalRetries  = 0;
    int         lastMaxScore  = 0;
    float       lastMaxSpeed  = 0.0f;
};

// ---------------------------------------------------------------------------
// Game session record -- written on game-over from gameCore
// ---------------------------------------------------------------------------
struct GameRecord {
    std::string idUser;
    int64_t     startTS      = 0;   // Unix epoch ms
    int64_t     endTS        = 0;
    int         idStory      = 0;
    int         idChapter    = 0;
    int         totalScore   = 0;
    int         totalSeconds = 0;
    float       avgSpeed     = 0.0f;
    int         retryNo      = 0;
};

// ---------------------------------------------------------------------------
// Board record -- one leaderboard row loaded from idUser_Records.
// Used by Board popup (V2: reads from DB instead of gameConsole_board.json).
// ---------------------------------------------------------------------------
struct BoardRecord {
    std::string idUser;
    int         totalScore   = 0;
    int         totalSeconds = 0;
    float       avgSpeed     = 0.0f;
    int64_t     endTS        = 0;   // Unix epoch ms; used for time-based sort
};

// ---------------------------------------------------------------------------
// API -- prototypes only; implementation in src/gameConsole/app.cpp.
// All functions return bool for success/failure where applicable.
// ---------------------------------------------------------------------------

// Open or create DB at SDL_GetPrefPath(...) + "<idUser>.sqlite".
// Calling twice without dbClose() is a no-op (returns true if already open).
bool dbOpen(const char* idUser);

// Close current DB handle. Safe to call when no DB is open.
void dbClose();

// Create tables if they don't exist. Idempotent.
bool dbInitSchema();

// Seed shared_data with hardcoded catalogue on first run.
// Guarded by SELECT COUNT(*) FROM shared_data so re-runs are no-ops.
bool dbSeedSharedData();

// Load stories for a given chapter, joined with user progress overlay.
// Returns empty vector if no rows or on error.
std::vector<StoryRow> dbLoadStories(const char* idUser, int idChapter);

// Insert-or-update user progress for one (idUser, idStory, idChapter) tuple.
bool dbUpsertStoryProgress(const char* idUser,
                           int idStory,
                           int idChapter,
                           bool isActivated,
                           bool isSelected);

// Append one game-session record. Caller fills GameRecord before calling.
bool dbInsertRecord(const GameRecord& rec);

// Return MAX(idChapter) WHERE isActivated=1 for this user.
// Used by chapter navigator "n/x" display. Returns 0 if no rows.
int dbMaxActivatedChapter(const char* idUser);

// Apply requiredStories rule: for each shared_data row, if all prerequisite
// idStory values are completed in idUser_Stories, set isActivated=1.
// Returns number of rows newly unlocked.
int dbCheckAndUnlockStories(const char* idUser);

// ---------------------------------------------------------------------------
// Settings persistence (Issue 2.2 / 2.3 gameConsole V2)
// Table: user_settings (key TEXT PRIMARY KEY, value TEXT)
// Created by dbInitSchema() alongside the other three tables.
// ---------------------------------------------------------------------------

// Persist current SettingsConfig to user_settings table (INSERT OR REPLACE).
// Called when Settings popup is closed. WASM: caller must syncfs after this.
bool dbSaveSettings(const SettingsConfig& cfg);

// Load SettingsConfig from user_settings table into cfg.
// Returns false if no settings row exists yet — caller keeps struct defaults.
bool dbLoadSettings(SettingsConfig& cfg);

// ---------------------------------------------------------------------------
// Leaderboard from DB (Issue 2.7 gameConsole V2)
// Replaces gameConsole_board.json as the Board popup data source.
// Falls back to gameConsole_board.json if idUser_Records is empty.
// ---------------------------------------------------------------------------

// Load up to `limit` game records ordered by totalScore DESC.
// Returns empty vector on error or when table has no rows.
std::vector<BoardRecord> dbLoadRecords(const char* idUser, int limit = 50);