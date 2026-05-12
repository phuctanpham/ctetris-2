#!/usr/bin/env python3
"""
parse.py — Convert chapters/src/c{id}/c{id}.json into c{id}.sql.

Output SQL targets the `shared_data` table consumed by
app/src/gameConsole/app.cpp::dbSeedSharedData(). Schema mirrors
gameConsole_db.h exactly:

    INSERT OR REPLACE INTO shared_data (
        idStory, storyName, idChapter, chapterName,
        minScore, minSpeed, minRetries, requiredStories,
        nextBlockScore, nextBlockSpeed,
        tableMatrix, xmlDialogue, thumbnailPath
    ) VALUES (...);

Usage:
    python3 chapters/scripts/parse.py chapters/src/c001/c001.json

Reads:  the given JSON file.
Writes: same directory, same basename with .sql extension.

Exit codes:
    0  success
    1  validation error (schema mismatch, dangling requiredStories ref, etc.)
    2  IO error (file not found, unreadable, etc.)
"""

from __future__ import annotations
import json
import re
import sys
from pathlib import Path


# ---------- helpers ------------------------------------------------------ #

def chapter_num_from_dir(name: str) -> int:
    """c001 -> 1, c042 -> 42. Raises if format is wrong."""
    m = re.fullmatch(r"c(\d{3,})", name)
    if not m:
        raise ValueError(f"Bad chapter dir name: {name!r} (expected c###)")
    return int(m.group(1))


def sql_str(value: str | None) -> str:
    """SQLite single-quoted string with '' doubling. NULL for None."""
    if value is None:
        return "NULL"
    return "'" + str(value).replace("'", "''") + "'"


def fmt_real(v: float | int) -> str:
    """Render a REAL value the same way the hand-crafted reference SQL does:
    integers get a `.0` suffix (e.g. 1 -> '1.0'), floats keep their form."""
    f = float(v)
    if f.is_integer():
        return f"{f:.1f}"
    return repr(f)


# ---------- validation --------------------------------------------------- #

# Required fields and the python type each must be.
# `int` is also accepted where `float` is expected (JSON often has 1 vs 1.0).
REQUIRED_FIELDS: list[tuple[str, type]] = [
    ("idStory",         int),
    ("storyName",       str),
    ("idChapter",       int),
    ("chapterName",     str),
    ("minScore",        int),
    ("minSpeed",        float),
    ("minRetries",      int),
    ("requiredStories", str),
    ("nextBlockScore",  int),
    ("nextBlockSpeed",  float),
    ("tableMatrix",     str),
    ("xmlDialogue",     str),
    ("thumbnailPath",   str),
]


def check_type(value, expected: type) -> bool:
    if expected is float:
        return isinstance(value, (int, float)) and not isinstance(value, bool)
    if expected is int:
        return isinstance(value, int) and not isinstance(value, bool)
    return isinstance(value, expected)


def validate(data: dict, chapter_dir: Path) -> None:
    if not isinstance(data, dict) or list(data.keys()) != ["shared_data"]:
        raise ValueError("Root JSON must have exactly one key 'shared_data'")
    stories = data["shared_data"]
    if not isinstance(stories, list) or not stories:
        raise ValueError("'shared_data' must be a non-empty array")

    chapter_num = chapter_num_from_dir(chapter_dir.name)
    seen_ids: set[int] = set()

    # First pass: per-row field & type checks
    for i, row in enumerate(stories):
        if not isinstance(row, dict):
            raise ValueError(f"Row #{i} is not an object")
        for field, expected in REQUIRED_FIELDS:
            if field not in row:
                raise ValueError(f"Row #{i}: missing field {field!r}")
            if not check_type(row[field], expected):
                raise ValueError(
                    f"Row #{i} ({row.get('storyName','?')}): "
                    f"field {field!r} expected {expected.__name__}, "
                    f"got {type(row[field]).__name__}"
                )
        if row["idChapter"] != chapter_num:
            raise ValueError(
                f"Row idStory={row['idStory']}: idChapter={row['idChapter']} "
                f"does not match folder c{chapter_num:03d}"
            )
        if row["idStory"] in seen_ids:
            raise ValueError(f"Duplicate idStory={row['idStory']} in chapter")
        seen_ids.add(row["idStory"])
        if "/" in row["thumbnailPath"] or "\\" in row["thumbnailPath"]:
            raise ValueError(
                f"Row idStory={row['idStory']}: thumbnailPath must be a "
                f"filename only (no path separator): {row['thumbnailPath']!r}"
            )

    # Second pass: requiredStories references must resolve within this chapter
    for row in stories:
        req = row["requiredStories"].strip()
        if not req:
            continue
        for tok in req.split(","):
            tok = tok.strip()
            if not tok:
                continue
            try:
                pid = int(tok)
            except ValueError:
                raise ValueError(
                    f"Row idStory={row['idStory']}: requiredStories token "
                    f"{tok!r} is not an integer"
                )
            if pid not in seen_ids:
                raise ValueError(
                    f"Row idStory={row['idStory']}: requiredStories references "
                    f"idStory={pid} which does not exist in this chapter"
                )
            if pid == row["idStory"]:
                raise ValueError(
                    f"Row idStory={row['idStory']}: cannot require itself"
                )


# ---------- SQL emission ------------------------------------------------- #

def emit_sql(data: dict, chapter_id: str) -> str:
    lines: list[str] = []
    lines.append(f"-- =============================================================================")
    lines.append(f"-- chapters/src/{chapter_id}/{chapter_id}.sql")
    lines.append(f"-- AUTO-GENERATED by chapters/scripts/parse.py")
    lines.append(f"-- DO NOT EDIT BY HAND. Edit {chapter_id}.json instead.")
    lines.append(f"-- =============================================================================")
    lines.append("")
    lines.append("BEGIN TRANSACTION;")
    lines.append("")

    for row in data["shared_data"]:
        lines.append(
            f"INSERT OR REPLACE INTO shared_data "
            f"(idStory, storyName, idChapter, chapterName, "
            f"minScore, minSpeed, minRetries, requiredStories, "
            f"nextBlockScore, nextBlockSpeed, "
            f"tableMatrix, xmlDialogue, thumbnailPath) VALUES ("
            f"{row['idStory']}, "
            f"{sql_str(row['storyName'])}, "
            f"{row['idChapter']}, "
            f"{sql_str(row['chapterName'])}, "
            f"{row['minScore']}, "
            f"{fmt_real(row['minSpeed'])}, "
            f"{row['minRetries']}, "
            f"{sql_str(row['requiredStories'])}, "
            f"{row['nextBlockScore']}, "
            f"{fmt_real(row['nextBlockSpeed'])}, "
            f"{sql_str(row['tableMatrix'])}, "
            f"{sql_str(row['xmlDialogue'])}, "
            f"{sql_str(row['thumbnailPath'])}"
            f");"
        )

    lines.append("")
    lines.append("COMMIT;")
    lines.append("")
    return "\n".join(lines)


# ---------- main --------------------------------------------------------- #

def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(f"Usage: {argv[0]} chapters/src/c{{id}}/c{{id}}.json", file=sys.stderr)
        return 2

    json_path = Path(argv[1])
    if not json_path.is_file():
        print(f"ERROR: not a file: {json_path}", file=sys.stderr)
        return 2

    chapter_dir = json_path.parent
    chapter_id = chapter_dir.name
    if not re.fullmatch(r"c\d{3,}", chapter_id):
        print(f"ERROR: parent dir name {chapter_id!r} is not 'c###'", file=sys.stderr)
        return 2
    if json_path.name != f"{chapter_id}.json":
        print(
            f"ERROR: filename {json_path.name!r} does not match folder "
            f"{chapter_id!r}",
            file=sys.stderr,
        )
        return 2

    try:
        data = json.loads(json_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as e:
        print(f"ERROR: invalid JSON in {json_path}: {e}", file=sys.stderr)
        return 1

    try:
        validate(data, chapter_dir)
    except ValueError as e:
        print(f"ERROR: validation failed in {json_path}: {e}", file=sys.stderr)
        return 1

    sql_text = emit_sql(data, chapter_id)
    out_path = chapter_dir / f"{chapter_id}.sql"
    out_path.write_text(sql_text, encoding="utf-8")
    print(f"OK: {json_path} -> {out_path} "
          f"({len(data['shared_data'])} row(s), {len(sql_text)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
