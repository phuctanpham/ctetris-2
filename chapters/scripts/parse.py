#!/usr/bin/env python3
"""
parse.py — Convert chapters/src/c{id}/c{id}.json into c{id}.sql.

Output SQL targets three tables consumed by the C++ game:
  shared_data      → gameConsole (story catalogue, unlock conditions)
  story_dialogues  → gameStory V2 (per-node dialogue)
  story_choices    → gameStory V2 (branching choices per node)

Schema mirrors gameConsole_db.h and gameStory_db.h exactly.

Usage:
    python3 chapters/scripts/parse.py chapters/src/c001/c001.json

Exit codes:
    0  success
    1  validation error
    2  IO error
"""

from __future__ import annotations
import json
import re
import sys
from pathlib import Path


# ---------- helpers ------------------------------------------------------ #

def chapter_num_from_dir(name: str) -> int:
    m = re.fullmatch(r"c(\d{3,})", name)
    if not m:
        raise ValueError(f"Bad chapter dir name: {name!r} (expected c###)")
    return int(m.group(1))


def sql_str(value: str | None) -> str:
    if value is None:
        return "NULL"
    return "'" + str(value).replace("'", "''") + "'"


def fmt_real(v: float | int) -> str:
    f = float(v)
    if f.is_integer():
        return f"{f:.1f}"
    return repr(f)


# ---------- validation --------------------------------------------------- #

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

DIALOGUE_REQUIRED_FIELDS: list[tuple[str, type]] = [
    ("nodeId",     int),
    ("speaker",    str),
    ("text",       str),
    ("imageUrl",   str),
    ("bgmUrl",     str),
    ("sfxUrl",     str),
    ("nextNodeId", int),
    ("choices",    list),
]

CHOICE_REQUIRED_FIELDS: list[tuple[str, type]] = [
    ("label",      str),
    ("nextNodeId", int),
]


def check_type(value, expected: type) -> bool:
    if expected is float:
        return isinstance(value, (int, float)) and not isinstance(value, bool)
    if expected is int:
        return isinstance(value, int) and not isinstance(value, bool)
    return isinstance(value, expected)


def _no_sep(val: str, field: str, story_id: int) -> None:
    if "/" in val or "\\" in val:
        raise ValueError(
            f"Row idStory={story_id}: {field} must be filename only "
            f"(no path separator): {val!r}"
        )


def validate(data: dict, chapter_dir: Path) -> None:
    if not isinstance(data, dict) or list(data.keys()) != ["shared_data"]:
        raise ValueError("Root JSON must have exactly one key 'shared_data'")
    stories = data["shared_data"]
    if not isinstance(stories, list) or not stories:
        raise ValueError("'shared_data' must be a non-empty array")

    chapter_num = chapter_num_from_dir(chapter_dir.name)
    seen_ids: set[int] = set()

    # Pass 1: shared_data field & type checks
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
                f"filename only: {row['thumbnailPath']!r}"
            )

    # Pass 2: requiredStories cross-references
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

    # Pass 3: optional dialogues[] validation
    for row in stories:
        if "dialogues" not in row:
            continue
        diags = row["dialogues"]
        if not isinstance(diags, list):
            raise ValueError(
                f"Row idStory={row['idStory']}: 'dialogues' must be an array"
            )
        seen_node_ids: set[int] = set()
        for di, node in enumerate(diags):
            if not isinstance(node, dict):
                raise ValueError(
                    f"Row idStory={row['idStory']} dialogue #{di}: not an object"
                )
            for field, expected in DIALOGUE_REQUIRED_FIELDS:
                if field not in node:
                    raise ValueError(
                        f"Row idStory={row['idStory']} dialogue #{di}: "
                        f"missing field {field!r}"
                    )
                if not check_type(node[field], expected):
                    raise ValueError(
                        f"Row idStory={row['idStory']} dialogue #{di}: "
                        f"field {field!r} expected {expected.__name__}, "
                        f"got {type(node[field]).__name__}"
                    )
            nid = node["nodeId"]
            if nid in seen_node_ids:
                raise ValueError(
                    f"Row idStory={row['idStory']}: duplicate nodeId={nid}"
                )
            seen_node_ids.add(nid)
            # Validate media filenames (no path separators)
            for mf in ("imageUrl", "bgmUrl", "sfxUrl"):
                val = node[mf]
                if val:
                    _no_sep(val, mf, row["idStory"])
            # Validate choices
            choices = node["choices"]
            for ci, choice in enumerate(choices):
                if not isinstance(choice, dict):
                    raise ValueError(
                        f"Row idStory={row['idStory']} dialogue #{di} "
                        f"choice #{ci}: not an object"
                    )
                for field, expected in CHOICE_REQUIRED_FIELDS:
                    if field not in choice:
                        raise ValueError(
                            f"Row idStory={row['idStory']} dialogue #{di} "
                            f"choice #{ci}: missing {field!r}"
                        )
                    if not check_type(choice[field], expected):
                        raise ValueError(
                            f"Row idStory={row['idStory']} dialogue #{di} "
                            f"choice #{ci}: {field!r} wrong type"
                        )

        # Pass 3b: nextNodeId cross-refs (0 = end, else must exist)
        for di, node in enumerate(diags):
            next_id = node["nextNodeId"]
            if next_id != 0 and next_id not in seen_node_ids:
                raise ValueError(
                    f"Row idStory={row['idStory']} dialogue #{di}: "
                    f"nextNodeId={next_id} not found in dialogue nodes"
                )
            for ci, choice in enumerate(node["choices"]):
                cnext = choice["nextNodeId"]
                if cnext != 0 and cnext not in seen_node_ids:
                    raise ValueError(
                        f"Row idStory={row['idStory']} dialogue #{di} "
                        f"choice #{ci}: nextNodeId={cnext} not found"
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

    # ---- shared_data ----
    lines.append("-- shared_data (story catalogue, consumed by gameConsole)")
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

    # ---- story_dialogues + story_choices ----
    has_any_dialogue = any(
        row.get("dialogues") for row in data["shared_data"]
    )
    if has_any_dialogue:
        lines.append("")
        lines.append("-- story_dialogues (consumed by gameStory V2)")
        for row in data["shared_data"]:
            for node in row.get("dialogues", []):
                has_choices = 1 if node["choices"] else 0
                lines.append(
                    f"INSERT OR REPLACE INTO story_dialogues "
                    f"(idStory, idChapter, nodeId, speaker, text, "
                    f"imageUrl, bgmUrl, sfxUrl, nextNodeId, hasChoices) VALUES ("
                    f"{row['idStory']}, {row['idChapter']}, {node['nodeId']}, "
                    f"{sql_str(node['speaker'])}, {sql_str(node['text'])}, "
                    f"{sql_str(node['imageUrl'])}, "
                    f"{sql_str(node['bgmUrl'])}, "
                    f"{sql_str(node['sfxUrl'])}, "
                    f"{node['nextNodeId']}, {has_choices}"
                    f");"
                )

        lines.append("")
        lines.append("-- story_choices (consumed by gameStory V2)")
        any_choices = False
        for row in data["shared_data"]:
            for node in row.get("dialogues", []):
                for ci, choice in enumerate(node["choices"]):
                    any_choices = True
                    lines.append(
                        f"INSERT OR REPLACE INTO story_choices "
                        f"(idStory, idChapter, nodeId, choiceIdx, label, nextNodeId) VALUES ("
                        f"{row['idStory']}, {row['idChapter']}, {node['nodeId']}, "
                        f"{ci}, "
                        f"{sql_str(choice['label'])}, "
                        f"{choice['nextNodeId']}"
                        f");"
                    )
        if not any_choices:
            lines.append("-- (no choices in this chapter)")

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
            f"ERROR: filename {json_path.name!r} does not match folder {chapter_id!r}",
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

    # Stats
    total_nodes = sum(
        len(row.get("dialogues", [])) for row in data["shared_data"]
    )
    total_choices = sum(
        len(node["choices"])
        for row in data["shared_data"]
        for node in row.get("dialogues", [])
    )
    print(
        f"OK: {json_path} -> {out_path} "
        f"({len(data['shared_data'])} stories, "
        f"{total_nodes} dialogue nodes, "
        f"{total_choices} choices, "
        f"{len(sql_text)} bytes)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))