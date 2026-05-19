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
│  │     └─ (images, audio, other assets)
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

