-- =============================================================================
-- chapters/c001/c001.sql  --  Chapter 001: The Tetris Matrix
-- Seeded into shared_data table; idempotent via INSERT OR REPLACE.
-- 10 stories: 1 intro + 3 basic paths + 3 advance paths + 3 endings.
-- Unlock rules (key per path; others just > 0):
--   light : higher minScore       (story 2 -> 5)
--   dark  : higher minRetries     (story 3 -> 6)
--   hidden: higher minSpeed       (story 4 -> 7; basic >1, advance >10)
-- Endings require 2-3 advance routes via requiredStories CSV.
-- =============================================================================

BEGIN TRANSACTION;

-- STORY 1: Intro (always open)
INSERT OR REPLACE INTO shared_data
    (idStory, storyName, idChapter, chapterName,
     minScore, minSpeed, minRetries, requiredStories,
     nextBlockScore, nextBlockSpeed,
     tableMatrix, xmlDialogue, thumbnailPath)
VALUES
    (1, 'Falling Sky', 1, 'The Tetris Matrix',
     0, 0.0, 0, '',
     50, 1.0,
     '', '', 'THUMB_001_FALLING_SKY.png');

-- STORY 2: Basic Light  (key: minScore > 1)
INSERT OR REPLACE INTO shared_data
    (idStory, storyName, idChapter, chapterName,
     minScore, minSpeed, minRetries, requiredStories,
     nextBlockScore, nextBlockSpeed,
     tableMatrix, xmlDialogue, thumbnailPath)
VALUES
    (2, 'Holy Sanctum', 1, 'The Tetris Matrix',
     2, 0.1, 1, '1',
     100, 1.2,
     '', '', 'THUMB_002_ORDERCORP_TEMPLE.png');

-- STORY 3: Basic Dark  (key: minRetries > 1)
INSERT OR REPLACE INTO shared_data
    (idStory, storyName, idChapter, chapterName,
     minScore, minSpeed, minRetries, requiredStories,
     nextBlockScore, nextBlockSpeed,
     tableMatrix, xmlDialogue, thumbnailPath)
VALUES
    (3, 'Blood Tower', 1, 'The Tetris Matrix',
     1, 0.1, 2, '1',
     100, 1.2,
     '', '', 'THUMB_003_UNDERCITY_SLUM.png');

-- STORY 4: Basic Hidden  (key: minSpeed > 1)
INSERT OR REPLACE INTO shared_data
    (idStory, storyName, idChapter, chapterName,
     minScore, minSpeed, minRetries, requiredStories,
     nextBlockScore, nextBlockSpeed,
     tableMatrix, xmlDialogue, thumbnailPath)
VALUES
    (4, 'Quantum Leap', 1, 'The Tetris Matrix',
     1, 1.1, 1, '1',
     100, 1.2,
     '', '', 'THUMB_004_SHADOW_ALLEY.png');

-- STORY 5: Advance Light  (key: HIGHER minScore)
INSERT OR REPLACE INTO shared_data
    (idStory, storyName, idChapter, chapterName,
     minScore, minSpeed, minRetries, requiredStories,
     nextBlockScore, nextBlockSpeed,
     tableMatrix, xmlDialogue, thumbnailPath)
VALUES
    (5, 'Sanctified', 1, 'The Tetris Matrix',
     20, 0.1, 1, '2',
     200, 1.5,
     '', '', 'THUMB_005_HEAVEN_MATRIX.png');

-- STORY 6: Advance Dark  (key: HIGHER minRetries)
INSERT OR REPLACE INTO shared_data
    (idStory, storyName, idChapter, chapterName,
     minScore, minSpeed, minRetries, requiredStories,
     nextBlockScore, nextBlockSpeed,
     tableMatrix, xmlDialogue, thumbnailPath)
VALUES
    (6, 'Origin Glitch', 1, 'The Tetris Matrix',
     1, 0.1, 20, '3',
     200, 1.5,
     '', '', 'THUMB_006_GLITCH_VIRUS.png');

-- STORY 7: Advance Hidden  (key: minSpeed > 10  -- 10x basic)
INSERT OR REPLACE INTO shared_data
    (idStory, storyName, idChapter, chapterName,
     minScore, minSpeed, minRetries, requiredStories,
     nextBlockScore, nextBlockSpeed,
     tableMatrix, xmlDialogue, thumbnailPath)
VALUES
    (7, 'Algorithm Master', 1, 'The Tetris Matrix',
     1, 10.1, 1, '4',
     200, 1.5,
     '', '', 'THUMB_007_FROZEN_TIME.png');

-- STORY 8: Bad Ending  (requires light + dark advance)
INSERT OR REPLACE INTO shared_data
    (idStory, storyName, idChapter, chapterName,
     minScore, minSpeed, minRetries, requiredStories,
     nextBlockScore, nextBlockSpeed,
     tableMatrix, xmlDialogue, thumbnailPath)
VALUES
    (8, 'Dead Wall', 1, 'The Tetris Matrix',
     1, 0.1, 1, '5,6',
     0, 0.0,
     '', '', 'THUMB_008_KILL_SCREEN.png');

-- STORY 9: True Ending  (requires light + hidden advance)
INSERT OR REPLACE INTO shared_data
    (idStory, storyName, idChapter, chapterName,
     minScore, minSpeed, minRetries, requiredStories,
     nextBlockScore, nextBlockSpeed,
     tableMatrix, xmlDialogue, thumbnailPath)
VALUES
    (9, 'Endless Record', 1, 'The Tetris Matrix',
     1, 0.1, 1, '5,7',
     0, 0.0,
     '', '', 'THUMB_009_ENDLESS_LOOP.png');

-- STORY 10: Golden Ending  (requires ALL 3 advance routes)
INSERT OR REPLACE INTO shared_data
    (idStory, storyName, idChapter, chapterName,
     minScore, minSpeed, minRetries, requiredStories,
     nextBlockScore, nextBlockSpeed,
     tableMatrix, xmlDialogue, thumbnailPath)
VALUES
    (10, 'Maxout 999999', 1, 'The Tetris Matrix',
     1, 0.1, 1, '5,6,7',
     0, 0.0,
     '', '', 'THUMB_010_GOLDEN_MAXOUT.png');

COMMIT;
