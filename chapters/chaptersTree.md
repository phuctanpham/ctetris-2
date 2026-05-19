# Chapters source tree

This file shows the source tree for the `chapters` directory and the naming/structure patterns used.

chapters/
├─ chaptersTree.md
├─ prompts/
│  ├─ json.md
│  ├─ c001.md
│  └─ c002.md (etc.)
├─ src/
│  ├─ c001/
│  │  ├─ c001.json
│  │  └─ media/
│  │     ├─ BGM_001_FALLING_SKY.mp3
│  │     ├─ BGM_002_ORDERCORP_TEMPLE.mp3
│  │     ├─ BGM_003_UNDERCITY_SLUM.mp3
│  │     ├─ BGM_004_SHADOW_ALLEY.mp3
│  │     ├─ BGM_005_HEAVEN_MATRIX.mp3
│  │     ├─ BGM_006_GLITCH_VIRUS.mp3
│  │     ├─ BGM_007_FROZEN_TIME.mp3
│  │     ├─ BGM_008_KILL_SCREEN.mp3
│  │     ├─ BGM_009_ENDLESS_LOOP.mp3
│  │     ├─ GM_010_GOLDEN_MAXOUT.mp3
│  │     ├─ SFX_001_BLOCK_DROP.mp3
│  │     ├─ SFX_002_HOLY_CHIME.mp3
│  │     ├─ SFX_003_ORC_LAUGH.mp3
│  │     ├─ SFX_004_GLITCH_WARP.mp3
│  │     ├─ SFX_005_ASCEND_BURST.mp3
│  │     ├─ SFX_006_VIRUS_CRASH.mp3
│  │     ├─ SFX_007_TIME_FREEZE.mp3
│  │     ├─ SFX_008_DEAD_STATIC.mp3
│  │     ├─ SFX_009_FLOW_PULSE.mp3
│  │     ├─ SFX_010_MAXOUT_FANFARE.mp3
│  │     ├─ THUMB_001_FALLING_SKY.png
│  │     ├─ THUMB_002_ORDERCORP_TEMPLE.png
│  │     ├─ THUMB_003_UNDERCITY_SLUM.png
│  │     ├─ THUMB_004_SHADOW_ALLEY.png
│  │     ├─ THUMB_005_HEAVEN_MATRIX.png
│  │     ├─ THUMB_006_GLITCH_VIRUS.png
│  │     ├─ THUMB_007_FROZEN_TIME.png
│  │     ├─ THUMB_008_KILL_SCREEN.png
│  │     ├─ THUMB_009_ENDLESS_LOOP.png
│  │     └─ THUMB_010_GOLDEN_MAXOUT.png
│  ├─ c002/
│  │  └─ (same pattern as `c001`)
│  └─ (more chapterID-driven folders will be added over time)
└─ (other support files)

Patterns
- Chapter markdown files in `prompts/` follow: `c{chapterID}.md` (example: `c001.md`, `c002.md`, ...)
- Chapter data files live under `src/{chapterID}/` and follow: `c{chapterID}.json` (example: `src/c001/c001.json`)
- Each chapter folder may include a `media/` subfolder for assets.

Notes
- New chapters are added by creating the `c{chapterID}.md` in `prompts/`, and a matching `src/c{chapterID}/` folder containing `c{chapterID}.json` and `media/` as needed.
- The `etc.` indicators above mean the same patterns will continue for subsequent chapters.

