-- =============================================================================
-- sync-board.sql -- Cloudflare D1 schema for cTetris leaderboard sync
-- =============================================================================
-- Single source of truth for the global leaderboard.
-- Workers API exposes:
--   GET  /leaderboard?limit=50   → read from sync_records
--   POST /record                 → insert one row (auth via 3rd-party OTP token)
-- =============================================================================

CREATE TABLE IF NOT EXISTS sync_records (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    name_user     TEXT    NOT NULL DEFAULT '',
    total_score   INTEGER NOT NULL DEFAULT 0,
    total_seconds INTEGER NOT NULL DEFAULT 0,
    avg_speed     REAL    NOT NULL DEFAULT 0.0,
    end_ts        INTEGER NOT NULL DEFAULT 0,   -- Unix epoch seconds UTC
    id_story      INTEGER NOT NULL DEFAULT 0,
    id_chapter    INTEGER NOT NULL DEFAULT 0,
    created_at    INTEGER NOT NULL DEFAULT (unixepoch())
);

CREATE INDEX IF NOT EXISTS idx_sync_records_score
    ON sync_records (total_score DESC);

CREATE INDEX IF NOT EXISTS idx_sync_records_end_ts
    ON sync_records (end_ts DESC);

-- Seed 30 rows matching FALLBACK_BOARD_ROWS in gameConsole/app.cpp
-- so the leaderboard is never empty on a fresh deploy.
INSERT INTO sync_records (name_user, total_score, total_seconds, avg_speed, end_ts, id_story, id_chapter) VALUES
    ('ShadowWalker', 8540, 4270, 2.00, 1744449330, 1, 1),
    ('PixelKnight',  7210, 3605, 2.00, 1744453335, 1, 1),
    ('CyberGhost',   6900, 3450, 2.00, 1744456545, 1, 1),
    ('NeonViper',    6550, 3275, 2.00, 1744446612, 1, 1),
    ('IronDragon',   6120, 3060, 2.00, 1744418700, 1, 1),
    ('StormBreaker', 5890, 2945, 2.00, 1744431153, 1, 1),
    ('AlphaCoder',   5700, 2850, 2.00, 1744460410, 1, 1),
    ('MysticSage',   5430, 2715, 2.00, 1744397720, 1, 1),
    ('FrostBite',    5210, 2605, 2.00, 1744423805, 1, 1),
    ('NovaStar',     4980, 2490, 2.00, 1744432255, 1, 1),
    ('GlitchMaster', 4750, 2375, 2.00, 1744462801, 1, 1),
    ('ZenithHero',   4620, 2310, 2.00, 1744406140, 1, 1),
    ('RogueZero',    4300, 2150, 2.00, 1744421122, 1, 1),
    ('TitanPulse',   4150, 2075, 2.00, 1744434618, 1, 1),
    ('LunarSeeker',  3900, 1950, 2.00, 1744385400, 1, 1),
    ('BlazeFury',    3750, 1875, 2.00, 1744450245, 1, 1),
    ('VoidRunner',   3520, 1760, 2.00, 1744424730, 1, 1),
    ('StarLord99',   3400, 1700, 2.00, 1744414325, 1, 1),
    ('EchoWhisper',  3280, 1640, 2.00, 1744457415, 1, 1),
    ('DriftKing',    3150, 1575, 2.00, 1744432150, 1, 1),
    ('AstroBoy',     2900, 1450, 2.00, 1744399510, 1, 1),
    ('SilentBlade',  2750, 1375, 2.00, 1744445325, 1, 1),
    ('MagmaCore',    2600, 1300, 2.00, 1744453800, 1, 1),
    ('AquaMarine',   2450, 1225, 2.00, 1744460740, 1, 1),
    ('ThunderBolt',  2200, 1100, 2.00, 1744390515, 1, 1),
    ('WindWalker',   1950,  975, 2.00, 1744424435, 1, 1),
    ('NightOwl',     1800,  900, 2.00, 1744430420, 1, 1),
    ('SilverFang',   1650,  825, 2.00, 1744411055, 1, 1),
    ('GoldenEye',    1400,  700, 2.00, 1744435512, 1, 1),
    ('ProGamerVN',   1250,  625, 2.00, 1744448730, 1, 1);
