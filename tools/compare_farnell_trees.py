"""Inventory old Farnell files by content, without modifying either source tree."""

import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import posixpath
import re


ROOT = Path(__file__).resolve().parents[1]


def inventory(root, exclude=None):
    return {
        path.relative_to(root).as_posix(): path.read_bytes()
        for path in sorted(root.rglob("*"))
        if path.is_file() and (exclude is None or not path.resolve().is_relative_to(exclude.resolve()))
    }


def digest(data):
    return hashlib.sha256(data).hexdigest()


def canvas_signature(data):
    """Ignore only canvas window geometry/visibility, preserving all other bytes."""
    def normalize(match):
        atoms = match.group(0).split()
        if len(atoms) == 7:  # Main window: retain the font size.
            return b"#N canvas 0 0 0 0 " + atoms[-1]
        if len(atoms) == 8:  # Subpatch: retain its name, ignore visibility.
            return b"#N canvas 0 0 0 0 " + atoms[-2] + b" 0;"
        return match.group(0)

    return re.sub(rb"^#N canvas[^\r\n]*", normalize, data, flags=re.MULTILINE)


def local_references(name, data, files):
    """Find literal sibling abstractions and relative assets; not a PD resolver."""
    references = set()
    if not name.endswith(".pd"):
        return []
    records = re.split(r"(?<!\\);\s*(?:\n|$)", data.decode("utf-8"))
    for record in records:
        atoms = re.findall(r"(?:\\.|[^\s])+", record.strip())
        if len(atoms) < 5 or atoms[:2] not in (["#X", "obj"], ["#X", "msg"]):
            continue
        candidates = []
        if atoms[1] == "obj":
            candidates.append(atoms[4] + ".pd")
        candidates.extend(atom for atom in atoms[4:]
                          if Path(atom).suffix.lower() in (".pd", ".wav", ".aif", ".aiff", ".txt"))
        for candidate in candidates:
            candidate = re.sub(r"\\(.)", r"\1", candidate)
            relative = posixpath.normpath(posixpath.join(posixpath.dirname(name), candidate))
            if relative in files:
                references.add(relative)
    return sorted(references)


def compare(old, pd):
    index = defaultdict(list)
    canvas_index = defaultdict(list)
    for name, data in pd.items():
        index[digest(data)].append(name)
        if name.endswith(".pd"):
            canvas_index[digest(canvas_signature(data))].append(name)

    rows = []
    for name, data in old.items():
        sha = digest(data)
        # Check actual bytes as well as the SHA-256 lookup.
        matches = sorted(match for match in index[sha] if data == pd[match])
        if name in matches:
            category = "identical_same_path"
        elif matches:
            category = "identical_other_path"
        elif name in pd:
            category = "different_same_path"
        else:
            category = "no_matching_content_or_path"
        canvas_matches = []
        if not matches and name.endswith(".pd"):
            signature = canvas_signature(data)
            canvas_matches = sorted(match for match in canvas_index[digest(signature)]
                                    if signature == canvas_signature(pd[match]))
        rows.append({
            "old": name,
            "bytes": len(data),
            "sha256": sha,
            "category": category,
            "pd_matches": matches,
            "pd_same_path_sha256": digest(pd[name]) if name in pd else None,
            "canvas_only_pd_matches": canvas_matches,
            "local_old_references": local_references(name, data, old),
        })

    by_name = {row["old"]: row for row in rows}
    unmatched = {row["old"] for row in rows if not row["pd_matches"]}
    closure = set(unmatched)
    queue = sorted(unmatched)
    while queue:
        for reference in by_name[queue.pop()]["local_old_references"]:
            if reference not in closure:
                closure.add(reference)
                queue.append(reference)
    support = [{
        "old": name,
        "referenced_by": sorted(source for source in closure
                                if name in by_name[source]["local_old_references"]),
    } for name in sorted(closure - unmatched)]
    callers = [{
        "old": row["old"],
        "references_unmatched_old": sorted(set(row["local_old_references"]) & unmatched),
    } for row in rows if row["pd_matches"] and set(row["local_old_references"]) & unmatched]

    groups = defaultdict(list)
    for name in sorted(unmatched):
        groups[digest(old[name])].append(name)
    within_old = [names for names in groups.values() if len(names) > 1
                  and all(old[name] == old[names[0]] for name in names)]
    counts = Counter(row["category"] for row in rows)
    return {
        "method": "SHA-256 lookup followed by byte equality, across all files regardless of name or extension",
        "dependency_scope": "Literal local object names and relative asset paths in object/message boxes, transitively followed. Does not resolve external libraries, declare/search paths, clone or dynamically constructed paths; not a runnable-patch guarantee.",
        "counts": {
            "old_files": len(old),
            "pd_files": len(pd),
            **dict(sorted(counts.items())),
            "exact_duplicate_files": sum(bool(row["pd_matches"]) for row in rows),
            "exact_duplicate_bytes": sum(row["bytes"] for row in rows if row["pd_matches"]),
            "without_exact_pd_match": len(unmatched),
            "canvas_only_match_files": sum(bool(row["canvas_only_pd_matches"]) for row in rows),
            "duplicate_support_files_for_unmatched_old": len(support),
        },
        "duplicate_support_files_for_unmatched_old": support,
        "duplicate_callers_of_unmatched_old": callers,
        "identical_groups_within_unmatched_old": within_old,
        "files": rows,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--old", type=Path, default=ROOT / "farnell/pd/old")
    parser.add_argument("--pd", type=Path, default=ROOT / "farnell/pd")
    parser.add_argument("--output", type=Path, default=ROOT / "farnell/reference/old-redundancy.json")
    args = parser.parse_args()
    for source in (args.old, args.pd):
        if not source.is_dir():
            parser.error(f"Source directory does not exist: {source}")
        if args.output.resolve().is_relative_to(source.resolve()):
            parser.error("Write the inventory outside the source trees")
    report = compare(inventory(args.old), inventory(args.pd, exclude=args.old))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report["counts"], indent=2))


if __name__ == "__main__":
    main()
