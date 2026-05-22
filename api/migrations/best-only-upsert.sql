-- =============================================================================
-- best-only-upsert.sql -- enforce one best row per (user, story, chapter)
-- =============================================================================
-- The leaderboard keeps only a player's BEST achievement per story/chapter.
-- POST /record upserts and only overwrites when the new total_score is higher,
-- which requires a unique key to conflict on. This migration first dedups any
-- existing rows (keeping the highest score, then latest end_ts), then adds the
-- unique index the upsert relies on. Safe to run more than once.
-- =============================================================================

DELETE FROM sync_records
WHERE id NOT IN (
    SELECT id FROM (
        SELECT id,
               ROW_NUMBER() OVER (
                   PARTITION BY name_user, id_story, id_chapter
                   ORDER BY total_score DESC, end_ts DESC, id DESC
               ) AS rn
        FROM sync_records
    ) WHERE rn = 1
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_sync_records_user_story_chapter
    ON sync_records (name_user, id_story, id_chapter);
