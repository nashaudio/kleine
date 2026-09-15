"""Rebuild the coverage and primitive inventories from local sources and curated metadata.

Uses only the standard library. Does not read or publish the private book PDF.
Edit farnell/catalogue.json for figure mappings and implementation status.
"""

from collections import Counter, defaultdict
import hashlib
from html import escape, unescape
import json
from pathlib import Path
import re
from urllib.parse import quote


ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "farnell"
SITE = "https://mitp-content-server.mit.edu/books/content/sectbyfn/books_pres_0/8375/designing_sound.zip/"
ALIASES = dict(zip(
    "t f i b s r s~ r~ del sel".split(),
    "trigger float int bang send receive send~ receive~ delay select".split()))
FOLDERS = {
    "ABOUTPD": 10, "ABSTRACTIONS": 12, "BASICS": 10, "PD-COMMON": 14,
    "SHAPING": 13, "SIGNALS": 11, "WAVEFORMS": 13, "TECHNIQUE": 20,
    "PEDESTRIANS": 24, "PHONETONES": 25, "DTMF": 26, "ALARMS": 27,
    "POLICE": 28, "BELL": 29, "BOUNCINGBALL": 30, "ROLLING": 31,
    "CREAKING": 32, "MRBOINGY": 33, "FIRE": 34, "BUBBLES": 35,
    "WATER": 35, "RUNNINGWATER": 36, "POUREDLIQUIDS": 37, "RAIN": 38,
    "ELECTRICITY": 39, "THUNDER": 40, "WIND": 41, "SWITCHES": 42,
    "CLOCK": 43, "MOTORS": 44, "ENGINES": 45, "FAN": 46,
    "JET": 47, "JETENGINE": 47, "HELICOPTER": 48, "FOOTSTEPS": 49,
    "INSECTS": 50, "BIRDS": 51, "MAMMALS": 52, "GUNS": 53,
    "EXPLOSIONS": 54, "ROCKETLAUNCHER": 55, "SCIFI": 56, "R2D2": 57,
    "REDALERT": 58, "REVERB": 14,
}
ARITHMETIC = set("+ - * / % mod pow sqrt cos sin exp log abs min max clip wrap == != > < >= <= && || +~ -~ *~ /~ max~ min~ clip~ sqrt~ abs~".split())
CONTROL = set("trigger float int bang send receive delay select inlet outlet pack unpack loadbang random metro swap moses change route spigot until line pipe symbol timer makefilename print".split())
GUI = set("hsl vsl bng tgl nbx cnv hradio vradio vu".split())
HOST = set("inlet~ outlet~ dac~ adc~ send~ receive~ throw~ catch~ block~ switch~ bang~ samplerate~".split())
BUILTINS = ARITHMETIC | CONTROL | GUI | HOST | set("osc~ hip~ lop~ bp~ noise~ vcf~ phasor~ cos~ sig~ vline~ line~ delwrite~ delread~ vd~ wrap~ env~ tabwrite~ tabwrite tabread tabread~ tabread4~ tabosc4~ tabplay~ tabsend~ rfft~ expr expr~ fexpr~ table list soundfiler textfile openpanel writesf~ notein noteout makenote stripnote ctlin poly mtof dbtorms snapshot~ threshold~ rzero~ rpole~ biquad~ samphold~ exp~ log~ q8_sqrt~".split())
# Bundled Vanilla abstractions are dependencies, not C primitives.
BUNDLED = {"hilbert~", "rev3~"}
PORTS = {
    "osc~": ("Tested port", "pd::osc", "Phase, negative frequency and FM fixtures; exact decoded samples in tested cases."),
    "hip~": ("Tested port", "pd::hip", "Impulse / zero cutoff / high cutoff / legacy normalisation fixtures."),
    "noise~": ("Tested port", "pd::noise", "Explicit-seed sample parity; default global seed order remains an integration concern."),
    "lop~": ("Tested port", "pd::lop", "100 Hz impulse at two rates; broader modulation/reset coverage remains."),
    "bp~": ("Tested port", "pd::bpf", "Two frequency/Q fixtures; keep existing name until reviewed."),
    "vcf~": ("Present, unvalidated", "pd::vcf", "Audit real/imaginary outlets, Q, frequency modulation and coefficient refresh; K-005."),
    "vline~": ("Limited model helper", "TelephoneBell::Decay", "Only immediate attack plus linear decay is tested; no queued/delayed ramp contract. K-007."),
    "delread~": ("Limited model helper", "TelephoneBell::Delay / Police::Environment", "Fixed taps and routing delays tested; arbitrary delay/control/reset semantics remain. K-008."),
    "delwrite~": ("Limited model helper", "TelephoneBell::Delay / Police::Environment", "Police feedback impulse is sample-identical at both rates; no general named-buffer port. K-008."),
    "phasor~": ("Tested port", "pd::phasor", "Positive/negative frequency, initial phase and wrap at 48/44.1 kHz; preserve 64-sample phase maintenance."),
    "cos~": ("Tested port", "pd::cos", "Negative/positive cycle lookup; exact fixture samples and alarm waveshaping comparisons."),
    "wrap~": ("Tested port", "pd::wrap", "Signed phase ramp including negative integers; finite-input fixture scope."),
    "line~": ("Tested port", "pd::line", "64-sample grid; short ramp, retarget, stop, immediate set at both rates. No arbitrary block size contract."),
    "env~": ("Tested port", "pd::env", "Default 1024-point Hann/512-sample hop only; detector states match. Configurable windows remain outside this port's scope."),
    "pow~": ("Native subset validated", "Police::LogOsc / std::pow", "Both police inlet conventions compared with PD; other domains and legacy numeric approximations remain."),
    "vd~": ("Not ported", "candidate: Klang Delay", "PD interpolation, minimum delay and block ordering must be checked. K-008."),
    "sig~": ("Native candidate", "signal / param", "Preserve PD block quantisation where audible; pulse trial uses host event rounding."),
    "sqrt~": ("Native candidate", "Klang/C++ square root", "PD 0.55 changed this algorithm; test pre-0.55 compatibility separately (installed unops-tilde help)."),
    "q8_sqrt~": ("Legacy compatibility gap", "—", "Now an alias of sqrt~ in PD 0.55.2; older fast approximation needs separate reference."),
}
# Registered primitive ports must not be reported as missing external objects.
BUILTINS.update(PORTS)

# Muted dark fills keep local SVGs comfortable in the editor's dark preview.
# Not-started stays neutral/off-white in its label, with a subdued empty track.
VISUALS = {
    "complete": ("Complete", "#243a2d", "#9dcba9", "#477b57", 82),
    "current": ("Current", "#3b3323", "#d0b376", "#9a7a3e", 75),
    "problem": ("Problem", "#3e292c", "#d49a9f", "#97545c", 80),
    "not-started": ("Not started", "#2b2e33", "#c4c8cf", "#36393f", 100),
    "none": ("—", "#24272c", "#808792", "#515761", 28),
}
VISUAL_BORDER = "#454b54"
PROGRESS_BACKGROUND = "#24272c"
PROGRESS_TEXT = "#c4c8cf"
STATUS_LEGEND = " · ".join(f"![{value[0]}](status/{role}.svg)" for role, value in VISUALS.items())


def svg_file(name, body, title, width, height):
    path = BASE / "status" / (name + ".svg")
    path.parent.mkdir(exist_ok=True)
    path.write_text(
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
        f'viewBox="0 0 {width} {height}" role="img" aria-label="{escape(title, quote=True)}">\n'
        f'<title>{escape(title)}</title>\n{body}\n</svg>\n', encoding="utf-8")
    return "status/" + path.name


def status_badges():
    for role, (label, background, foreground, fill, width) in VISUALS.items():
        svg_file(role,
                 f'<rect x="0.5" y="0.5" width="{width-1}" height="19" rx="4" '
                 f'fill="{background}" stroke="{VISUAL_BORDER}"/>'
                 f'<text x="{width/2}" y="14" text-anchor="middle" '
                 f'font-family="Arial, sans-serif" font-size="11" fill="{foreground}">{label}</text>',
                 "No work to do" if role == "none" else label, width, 20)


def badge(role, detail=""):
    label = VISUALS[role][0]
    return f"![{label}](status/{role}.svg)" + (f" {detail}" if detail and detail.lower() != label.lower() else "")


def patch_role(row):
    if row["missing"] or row.get("problem"):
        return "problem"
    state = row["status"].lower()
    if any(word in state for word in ("broken", "problem", "blocked", "failed")):
        return "problem"
    if state in ("trial validated", "complete", "completed"):
        return "complete"
    if state in ("not started", "not ported"):
        return "not-started"
    if state in ("not applicable", "no work"):
        return "none"
    return "current"


def progress(roles):
    """Completion is green-only; other roles colour the unfinished portion."""
    counts = Counter(roles)
    order = ("complete", "current", "problem", "not-started")
    total = sum(counts[role] for role in order)
    if not total:
        return badge("none")
    done = counts["complete"]
    percent = f"{100 * done / total:.1f}".removesuffix(".0")
    label = f"{percent}% ({done}/{total})"
    title = f"{label} complete; {counts['current']} current, {counts['problem']} problematic, {counts['not-started']} not started"
    segments, offset = [], 0.0
    for role in order:
        width = 110 * counts[role] / total
        if width:
            segments.append(f'<rect x="{offset:.3f}" y="3" width="{width:.3f}" height="14" fill="{VISUALS[role][3]}"/>')
        offset += width
    body = (f'<rect width="238" height="20" rx="4" fill="{PROGRESS_BACKGROUND}"/>'
            + ''.join(segments)
            + f'<rect x="0.5" y="3.5" width="109" height="13" rx="2" fill="none" stroke="{VISUAL_BORDER}"/>'
            + f'<text x="119" y="14" font-family="Arial, sans-serif" font-size="11" fill="{PROGRESS_TEXT}">{label}</text>')
    name = "progress-" + "-".join(str(counts[role]) for role in order)
    path = svg_file(name, body, title, 238, 20)
    return f"![{title}]({path})"


def group_status(roles):
    counts = Counter(roles)
    total = sum(counts.values()) - counts["none"]
    if not total:
        return badge("none")
    if counts["problem"]:
        return badge("problem", f"{counts['problem']} source/status issue(s)")
    if counts["complete"] == total:
        return badge("complete")
    if counts["complete"] or counts["current"]:
        return badge("current", "partly complete / awaiting comparison")
    return badge("not-started")


def primitive_role(name):
    status = PORTS.get(name, ("",))[0]
    if status == "Tested port":
        return "complete"
    if status in ("Present, unvalidated", "Limited model helper", "Native subset validated"):
        return "current"
    if name in GUI:
        return "none"
    return "not-started"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def link(label, path):
    return f"[{label}]({quote(path, safe='/#~:')})"


def cell(value):
    return str(value).replace("|", "&#124;").replace("\n", " ")


def table(headers, rows):
    return ["| " + " | ".join(headers) + " |", "| " + " | ".join("---" for _ in headers) + " |"] + [
        "| " + " | ".join(cell(x) for x in row) + " |" for row in rows]


def records(data):
    return [re.findall(r"(?:\\.|[^\s])+", r.strip())
            for r in re.split(r"(?<!\\);\s*(?:\n|$)", data.decode("utf-8")) if r.strip()]


def object_names(data):
    return [a[4] for a in records(data) if len(a) >= 5 and a[:2] == ["#X", "obj"]]


def load_sources():
    return {p.relative_to(BASE).as_posix(): p.read_bytes()
            for folder in ("pd", "zip") for p in sorted((BASE / folder).rglob("*.pd"))
            if "old" not in p.relative_to(BASE / folder).parts}


def website():
    pages = {}
    for path in sorted((BASE / "zip").glob("*.html")):
        if path.name == "index.html":
            continue
        body = path.read_text(encoding="iso-8859-1").split('<div id="rightcolumn">')[1].split("<!-- End Right Column -->")[0]
        chapter = int(re.search(r"\d+", path.stem)[0]) + (23 if path.stem.startswith("practical") else 0)
        heading, patches, audio = "", {}, []
        for match in re.finditer(r'<h3>(.*?)</h3>|href="([^"<>]+\.(?:pd|wav))"', body, re.S):
            if match[1]:
                heading = unescape(re.sub("<[^>]*>", "", match[1])).strip()
            elif match[2].endswith(".pd"):
                patches.setdefault(match[2], []).append(heading)
            else:
                audio.append(match[2])
        pages[chapter] = {"page": path.name, "patches": patches, "audio": audio}
    return pages


def make_rows(meta, sources, web):
    rows, covered = [], set()
    chapters = {c["number"]: c for c in meta["chapters"]}
    bulk = {p: b for p, b in sources.items() if p.startswith("pd/")}
    hashes = defaultdict(list)
    for path, data in bulk.items():
        hashes[sha(data)].append(path)
    for number, page in web.items():
        chapter = chapters[number]
        for path, headings in page["patches"].items():
            source = "zip/" + path
            spec = meta["website_patches"][path]
            matches = list(hashes[sha(sources[source])]) if source in sources else []
            # Same name in the corresponding chapter may be a different revision.
            variants = [p for p in bulk if Path(p).name == Path(path).name
                        and FOLDERS.get(p.split('/')[1]) == number and p not in matches]
            if path == "ch11/basics1.pd":
                variants = ["pd/ABOUTPD/midi-monosynth.pd"]
            if path == "ch13/basics1.pd":
                variants = ["pd/SHAPING/shaping-antiphase.pd"]
            covered.update(matches + variants)
            rows.append({"id": chapter["id"] + "/" + Path(path).stem,
                         "chapter": number, "label": Path(path).name,
                         "description": " / ".join(headings),
                         "sources": ([source] if source in sources else []) + matches + variants,
                         "web_path": path, "missing": source not in sources,
                         "identical": matches, "variants": variants, **spec})
    for path in sorted(set(bulk) - covered):
        override = meta.get("bulk_patches", {}).get(path, {})
        number = override.get("chapter", FOLDERS.get(path.split('/')[1], 0))
        prefix = chapters[number]["id"] if number else "extra"
        rows.append({"id": prefix + "/bulk/" + path[3:-3], "chapter": number,
                     "label": path[3:], "description": "Bulk supplement / variant",
                     "sources": [path], "web_path": None, "missing": False,
                     "identical": [], "variants": [], "figures": [], "mapping": "unmapped",
                     "status": "not started", **override})
    for spec in meta.get("book_patches", []):
        chapter = chapters[spec["chapter"]]
        rows.append({"id": chapter["id"] + "/" + spec["name"],
                     "label": spec["name"], "sources": [], "web_path": None,
                     "missing": True, "book_only": True, "identical": [], "variants": [], **spec})
    return rows


def dependencies(sources):
    stems = defaultdict(list)
    for path in sources:
        stems[Path(path).stem].append(path)
    graph, missing = {}, {}
    for path, data in sources.items():
        local, unresolved = set(), set()
        for name in object_names(data):
            canonical = ALIASES.get(name, name)
            if canonical in BUILTINS or canonical in BUNDLED:
                continue
            candidate = (Path(path).parent / (name + ".pd")).as_posix()
            if candidate in sources:
                local.add(candidate)
            else:
                unresolved.add(name)
        graph[path], missing[path] = sorted(local), sorted(unresolved)
    return graph, missing, stems


def coverage(meta, sources, web, rows, graph, missing):
    chapters = {c["number"]: c for c in meta["chapters"]}
    by_chapter = defaultdict(list)
    source_ids = {}
    for row in rows:
        by_chapter[row["chapter"]].append(row)
        for source in row["sources"]:
            source_ids.setdefault(source, row["id"])
    anchor = lambda value: value.replace("/", "-").replace("~", "-tilde").replace("_", "-").lower()
    refs = lambda figures: "; ".join(f"{f} p. {meta['figure_pages'][f]}" for f in figures)
    mapping_counts = Counter(row["mapping"] for row in rows)
    lines = ["# Designing Sound coverage", "",
             "Snapshot: " + meta["snapshot"] + ". Full coverage is the target; this is an inventory and work queue, not a claim that the patches have all been run.", "",
             "Companion records: [PD primitive status](PD-PRIMITIVES.md) and [Klang review backlog](KLANG-REVIEW.md). [Curated metadata](catalogue.json) and [generator](../tools/catalogue_farnell.py) keep IDs, figure references and status separate from the rendered tables.", "",
             "## Overview", "",
             f"All **58 book chapters**, **35 website practicals**, **11 website teaching/technique chapters**, **300 distinct website patch links** (298 local files; two missing links), **434 bulk patches**, and **35 practical WAVs** are accounted for. The two source trees contain {len(set(map(sha, sources.values())))} byte-distinct patch contents. The parked `pd/old` archive is excluded from active counts and remains available for recovery.", "",
             f"There are {len(rows)} patch work items, including bulk supplements and {sum(r.get('book_only', False) for r in rows)} book patch diagrams awaiting sources. **Artificial Sounds (24–28) is trial validated: 37/37 items**, including documented reference variants and reconstructed matchers for two bulk demos with missing external abstractions. Chapter 29 has five website items covered. Two chapter 45 items have existing Chris Nash ports awaiting this comparison workflow. See the [Artificial Sounds evidence and limits](audio/artificial-sounds.md).", "",
             f"Mapping gaps: **{mapping_counts['unmapped']} patches need a figure reference**; **{sum(r.get('book_only', False) for r in rows)} inspected patch diagrams need a PD source**. See [mapping gaps](#mapping-gaps). Ordinary diagrams, plots and photographs do not require patches. The {mapping_counts['website-only']} explicitly website-only examples are exempt from book-figure matching.", "",
             "`Trial validated` means the retained 48 kHz fixtures and relevant controls were compared; it does not certify every historical revision, GUI action, sample rate, or parameter. Bulk variants remain work even when the main model exists. See [trial evidence](audio/README.md).", "",
             STATUS_LEGEND, "",
             "Amber includes partial implementations and work awaiting comparison/review. Red marks known source/status problems. White is unstarted work; grey means no work in this table's scope. Percentages count equally weighted completed items only, not estimated effort. Green is limited to the documented validation scope; amber and red segments do not earn partial completion credit.", "",
             "**Overall:** " + progress(patch_role(r) for r in rows), "",
             "| Series | Chapters | Patch work items | Status | Completion |",
             "| --- | --- | --- | --- | --- |"]
    for series in dict.fromkeys(c["series"] for c in chapters.values()):
        nums = [n for n, c in chapters.items() if c["series"] == series]
        group = [r for r in rows if r["chapter"] in nums]
        roles = [patch_role(r) for r in group]
        lines.append(f"| {series} | {nums[0]}–{nums[-1]} | {len(group) if group else badge('none')} | {group_status(roles)} | {progress(roles)} |")
    if by_chapter[0]:
        roles = [patch_role(r) for r in by_chapter[0]]
        lines.append(f"| Unassigned bulk supplements | — | {len(roles)} | {group_status(roles)} | {progress(roles)} |")
    lines += ["", "## IDs, mappings and maintenance", "",
              "Use stable chapter/practical IDs such as `25-phonetones`, with patch IDs such as `25-phonetones/pulsedial`. Bulk-only entries add `/bulk/<original path>` to avoid filename collisions. Figures are references, not IDs: a patch can span multiple figures or have no numbered figure. Dependencies link to patch IDs; the primitive table covers built-in nodes separately.", "",
              "Page numbers are **printed book pages**. In the supplied private PDF, add 25 for the PDF page number (printed p. 377 = PDF page 402). Chapter spans run to the next chapter and can include intervening part/series dividers. Only bibliographic mappings are retained here; the PDF, text and page images stay local.", "",
              "Mapping labels: **caption** = book figure caption/topic and website description were cross-referenced, not a claim of graph identity; **topic** = a looser candidate or variant association requiring inspection; **diagram** = the pictured patch was inspected; **unmapped** = a patch needs a justified figure reference (or an explicit outside-book exception); **website-only** = explicitly described as absent from the book. Mapping problems are shown in the book-reference column separately from implementation status. A figure needs source-recovery work only when it depicts a patch; other illustrations do not create work items.", "",
              "Source labels: `web` is the local companion download; `bulk =` is byte-identical; `bulk variant` is a same-name/candidate revision, not a proven equivalent. Supplemental folder-to-chapter assignments are provisional. Planned Klang paths are shown as code, not links; nested components can share a practical's `.k` file. Existing source paths are links.", "",
              "During implementation update the metadata with figures, controls, dependencies, source choice, Klang path/component, and evidence. Use statuses `not started`, `in progress`, `implemented; comparison pending`, `trial validated`, or `adapted`; a `problem` field records a known fault separately. Book patch diagrams with missing sources belong in `book_patches`. Record a render recipe and comparison link before moving to trial validated. Regenerate with `python tools/catalogue_farnell.py`; refresh the review backlog at series completion or when requested.", "",
              "## Suggested order", "",
              "Artificial Sounds (24–28) was selected as the extended process trial and is now covered. Select the next practical milestone with Chris; **41-wind** and **38-rain** remain the early ambient priorities, with bubbles (35) useful for rain. Bring Techniques and the Pure Data teaching section in as needed. Birds (51), footsteps (49), and engine calibration/adaptation remain queued; full coverage is still the target.", "",
              "Useful cross-practical prerequisites: rain 38 draws on bubbles 35 and can combine with wind 41; clocks 43 develop switch 42 techniques (book p. 493); machine resonances recur through 44–48. These are conceptual links, distinct from the literal abstraction dependencies below.", "",
              "## Mapping gaps", "",
              "The audit runs from patch to figure. Find a reference for each unassigned patch, or explicitly establish that it is a companion-only supplement. In the other direction, investigate only figures depicting patches whose sources have not been located; no blanket list of unmatched illustrations is needed.", "",
              "### Patch diagrams needing a source", ""]
    lines += table(["Work item", "Figure / printed page", "Source status", "Next step"], [
        [link(r["id"], "#" + anchor(r["id"])), refs(r["figures"]),
         badge("problem", "PD source not located"), r.get("note", "Locate source or reconstruct a separate reference.")]
        for r in rows if r.get("book_only")])
    lines += ["", f"<details><summary>{mapping_counts['unmapped']} patches needing figure references</summary>", ""]
    lines += table(["Patch", "Chapter pages", "Mapping status / lead"], [
        [link(r["id"], "#" + anchor(r["id"])),
         f"{chapters[r['chapter']]['page_start']}–{chapters[r['chapter']]['page_end']}" if r["chapter"] else "Unassigned",
         badge("problem", r.get("note", "Find the figure, or confirm an outside-book supplement."))]
        for r in rows if r["mapping"] == "unmapped"])
    lines += ["", "</details>", "", "## Chapter index", ""]
    summary = []
    for number, c in chapters.items():
        count = len(by_chapter[number])
        roles = [patch_role(r) for r in by_chapter[number]]
        summary.append([link(c["id"] + " — " + c["title"], "#" + c["id"]),
                        f"{c['page_start']}–{c['page_end']}", f"P{number-23:02}" if number >= 24 else "—",
                        count if count else badge("none"), group_status(roles), progress(roles)])
    lines += table(["Chapter / ID", "Printed pages", "Website practical", "Patch items", "Status", "Completion"], summary)
    for number in list(chapters) + ([0] if by_chapter[0] else []):
        c = chapters.get(number, {"id": "extra", "title": "Unassigned bulk supplements", "klang_file": "TBD", "page_start": "?", "page_end": "?"})
        lines += ["", f'<a id="{c["id"]}"></a>', f"## {c['id']} — {c['title']}", ""]
        if number in web:
            page = web[number]
            audio = ", ".join(link(Path(p).name, "zip/" + p) for p in page["audio"]) or "none supplied"
            lines += [f"Website: {link('local page', 'zip/' + page['page'])} / {link('publisher page', SITE + page['page'])}. Audio: {audio}. The recording demonstrates the practical as a whole, not necessarily every patch or preset.", ""]
        if not by_chapter[number]:
            lines += [badge("none"), ""]
        entries = []
        for row in by_chapter[number]:
            source_links = []
            if row["web_path"]:
                source_links.append("web link missing" if row["missing"] else link("web", "zip/" + row["web_path"]))
            source_links += [link("bulk = " + p[3:], p) for p in row["identical"]]
            source_links += [link("bulk variant " + p[3:], p) for p in row["variants"]]
            if not row["web_path"]:
                source_links = [link(p[3:], p) for p in row["sources"]] or ["PD source not located"]
            # Reference-specific dependencies are unioned and labelled conservatively.
            deps, unresolved = set(), set()
            for path in row["sources"]:
                deps.update(graph[path]); unresolved.update(missing[path])
            dep_labels = []
            for dep in sorted(deps):
                ident = source_ids.get(dep)
                dep_labels.append(link(ident, "#" + anchor(ident)) if ident else link(dep, dep))
            dep_labels += [f"`{name}` ?" for name in sorted(unresolved)]
            filename = row.get("klang_file", c["klang_file"])
            if number <= 22 and (row["web_path"] or row.get("book_only")):
                stem = Path(row["web_path"]).stem if row["web_path"] else row["name"]
                filename = str(Path(c["klang_file"]).with_name(stem + ".k")).replace('\\', '/')
            if row["status"].startswith("existing port"):
                filename = "../sounds/Motors.h"
            klang = link(filename, filename) if (BASE / filename).is_file() else f"`{filename}` (planned)"
            if row.get("component"):
                klang += " — `" + row["component"] + "`"
            book = refs(row["figures"]) + f" ({row['mapping']})" if row["figures"] else badge("none", "Website-only") if row["mapping"] == "website-only" else badge("problem", f"Figure reference missing; chapter pp. {c['page_start']}–{c['page_end']}")
            note = row.get("note", "")
            if row.get("evidence"):
                note += " " + link("Comparison evidence", row["evidence"])
            if number == 45 and row["status"].startswith("existing port"):
                note += " " + ("ToyBoatEngine" if "toy_boat" in row["id"] else "FourStrokeEngine") + "; planned .k extraction pending."
            problem = ("PD source not located" if row.get("book_only") else
                       "Web download missing" if row["missing"] else row.get("problem", ""))
            entries.append([f'<a id="{anchor(row["id"])}"></a>`{row["id"]}`',
                            row["description"] + ": " + "; ".join(source_links), book,
                            "; ".join(dep_labels) or "—", klang,
                            badge(patch_role(row), (problem + "; " if problem else "") + row["status"]), note or "—"])
        if entries:
            lines += table(["Patch ID", "Patch / sources", "Book figure / page", "Local dependencies / unresolved objects", "Klang implementation", "Status", "Notes"], entries) + [""]
    lines += ["## Dependency and source limits", "",
              "Dependencies are a static union over the row's source revisions. The scanner follows literal local abstraction names; it does not expand each abstraction instance into the runtime graph, evaluate `$1` object names, or resolve user search paths/external libraries. `?` marks an unresolved local object, external or dynamic name, not automatically a missing Vanilla primitive. Supporting WAV/table filenames must also be checked when making a render wrapper. See the primitive inventory's unresolved-object table for candidate locations.", "",
              "Two broken mirror links are `ch11/basics1.pd` and `ch13/basics1.pd`; their HTML pages still provide context/embedded code. `ch12/wavetablesynth2.pd` and `p08/rolling1.pd` are each linked twice under different headings. Do not silently treat repeated links as proof that both intended examples are supplied.", "",
              "The [parked old archive](pd/old/) and [earlier redundancy findings](reference/old-redundancy.md) are recovery resources. Their files do not inflate the active inventory. Third-party Pd help examples mentioned in the book (for example fig. 17.3) need explicit provenance if brought into this repository."]
    return "\n".join(lines) + "\n"


def primitives(meta, sources, rows, graph, missing, stems):
    web = {p: b for p, b in sources.items() if p.startswith("zip/")}
    unique = {}
    for path, data in sources.items():
        unique.setdefault(sha(data), (path, data))
    def counts(items):
        nodes, patches = Counter(), Counter()
        for path, data in items:
            names = Counter(ALIASES.get(n, n) for n in object_names(data))
            nodes.update(names); patches.update(names.keys())
        return nodes, patches
    wn, wp = counts(web.items()); an, ap = counts(unique.values())
    raw_names = set(an)
    abstraction_names = set(stems) - BUILTINS
    builtin_names = sorted(raw_names & BUILTINS, key=lambda n: (-wp[n], -ap[n], n))
    lines = ["# PD primitive coverage and usage", "", "Snapshot: " + meta["snapshot"] + ". See [model coverage](COVERAGE.md), [Klang review backlog](KLANG-REVIEW.md), and [isolated comparison evidence](../tests/pd/README.md).", "",
             "## Overview", "",
             f"The active inventory contains **{len(sources)} patch files**: {len(web)} website files and {len(sources)-len(web)} bulk files, representing **{len(unique)} byte-distinct contents**. The parked `pd/old` archive is excluded. There are **{len(builtin_names)} distinct normalised Vanilla node names used**, plus bundled abstractions and non-Vanilla/local/dynamic objects listed separately.", "",
             "Eleven dedicated structs exist in [include/klang/pd.h](../include/klang/pd.h). **Ten have isolated comparison fixtures; `vcf` remains unvalidated.** New ports are `phasor`, `cos`, `wrap`, `line` and the default-window `env`. The bell's limited decay helper also has a fixture, but is not a full `vline~` port. All 40 retained cases passed across 48/44.1 kHz on PD 0.55.2; this describes those fixtures, not universal parity. [Results](../tests/pd/results.json) and [source revisions](../tests/pd/sources.json) preserve the evidence.", "",
             STATUS_LEGEND, "",
             "**Node coverage:** " + progress(primitive_role(name) for name in builtin_names), "",
             "Green means validated within the retained fixture scope; amber means an existing implementation/helper still needs work. White includes native/control candidates whose PD semantics have not yet been checked. Grey GUI rows need no dedicated primitive port and are excluded from the percentage; model control translation still applies. Each remaining node counts once, regardless of usage frequency. Red in the dependency table marks unresolved source requirements, not a failed audio test.", "",
             "## Counting rules", "",
             "`Web patches` counts files containing the node at least once; `Web nodes` counts saved object boxes in all 298 website patches. `All patches/nodes` counts one representative of each exact-content hash across both active trees. Changed revisions remain distinct. Aliases (`t/trigger`, `f/float`, `i/int`, `b/bang`, `s/send`, `r/receive`, signal send/receive, `del/delay`, `sel/select`) are combined. Embedded subpatch contents are counted once as saved; an abstraction called ten times is not expanded ten times. These are source usage counts, not runtime instances or CPU cost.", "",
             "Message boxes, comments, array data and GUI atom boxes are not primitives. Named GUI objects such as `hsl` are counted but separated from DSP work. Local abstractions and bundled `hilbert~`/`rev3~` are not misclassified as primitive ports. Whole-graph scheduling, summing and inlet semantics still need preserving when replacing PD glue with native code.", "",
             "## Next port work", "",
             "Prioritise `vcf~` verification, queued `vline~` ramps and general delays (`vd~`, `delread~`, `delwrite~`), then `samphold~`, `rzero~` and further noise/envelope work as required by the next practical. The Artificial Sounds trial now covers `phasor~`, `cos~`, `wrap~`, block-64 `line~` and default `env~`. High counts alone do not justify porting every control object: arithmetic, lists and GUI logic often translate more clearly into Klang/C++. Police fixtures distinguish the two `pow~` inlet conventions; wider numeric-domain compatibility remains open.", "",
             "## Vanilla node inventory", ""]
    entries = []
    for name in builtin_names:
        if name in PORTS:
            status, implementation, next_step = PORTS[name]
        elif name in GUI:
            status, implementation, next_step = "Host/UI translation", "Controls / native state", "No DSP port; preserve defaults, ranges and event behaviour."
        elif name in ARITHMETIC:
            status, implementation, next_step = "Native candidate", "Klang/C++ arithmetic", "Verify PD edge cases (domain, clipping, division/modulo); native availability is not tested parity."
        elif name in CONTROL:
            status, implementation, next_step = "Native/control translation", "param / state / event code", "Preserve hot/cold inlet ordering and scheduling; no complete PD control runtime."
        elif name in HOST:
            status, implementation, next_step = "Host/scheduling gap", "Kleine / model wiring", "Check block delays, fan-in, feedback, events and graph enable/disable; K-009."
        else:
            status, implementation, next_step = "Not ported", "—", "Add isolated fixture when first required; assess existing Klang equivalents."
        entries.append([f"`{name}`", wp[name], wn[name], ap[name], an[name], badge(primitive_role(name), status),
                        implementation, next_step])
    lines += table(["PD node", "Web patches", "Web nodes", "All patches", "All nodes", "Port status", "Klang path/type", "Remaining work"], entries)
    lines += ["", "## Local abstractions, bundled helpers and unresolved objects", "",
              "These names are not counted as missing Vanilla primitive ports. A name can exist somewhere in the collection but still be absent from a caller's local directory. Candidate files are leads, not an automatic search-path fix; the coverage table records literal local dependencies. External identity is provisional unless verified by its source/package.", ""]
    external = []
    for name in sorted(raw_names - BUILTINS, key=lambda n: (-wp[n], -ap[n], n)):
        candidates = stems.get(name, [])
        unresolved_callers = [p for p in sources if name in missing[p]]
        kind = "Vanilla bundled abstraction" if name in BUNDLED else "Dynamic object name" if "$" in name else "Local abstraction" if name in abstraction_names else "External / unresolved"
        locations = ", ".join(link(p, p) for p in candidates[:4])
        if len(candidates) > 4:
            locations += f" (+{len(candidates)-4} copies)"
        if unresolved_callers:
            locations += ("; " if locations else "") + f"not local in {len(unresolved_callers)} files; e.g. " + link(unresolved_callers[0], unresolved_callers[0])
        role = "problem" if kind == "External / unresolved" or (unresolved_callers and kind == "Local abstraction") else "none"
        external.append([f"`{name}`", wp[name], wn[name], ap[name], an[name], badge(role, kind), locations or "Resolve from Pd distribution / original package"])
    lines += table(["Object", "Web patches", "Web nodes", "All patches", "All nodes", "Kind", "Source / resolution work"], external)
    lines += ["", "## Maintenance", "",
              "Rebuild both inventories with `python tools/catalogue_farnell.py` after changing source collections. Update the port registry in that script only when implementation and evidence warrant the new status. Add isolated patches under `tests/pd` for each new/changed primitive; record level/residual, rate, block size, relevant parameter events and source revision. Preserve explicit compatibility variants. Aggregate Klang implementation issues in the review backlog at the end of a practical series or when requested.", "",
              "Vanilla classification uses the installed PD 0.55.2 reference help and local source evidence, with legacy `q8_sqrt~` retained as a compatibility item. A headless no-preferences creation probe confirmed that `init` and `>~` are unavailable in this Vanilla installation; they remain unresolved/external dependencies. The installed `unops-tilde-help.pd` documents the sqrt algorithm change in 0.55 and the current q8 aliases. This is a working dependency inventory, not a complete list of all Vanilla objects. No primitive code was changed by this inventory."]
    return re.sub(r"\bK-(\d{3})\b", lambda m: link(m[0], "KLANG-REVIEW.md#k-" + m[1]), "\n".join(lines)) + "\n"


def main():
    status_badges()
    meta = json.loads((BASE / "catalogue.json").read_text(encoding="utf-8"))
    sources, web = load_sources(), website()
    rows = make_rows(meta, sources, web)
    graph, missing, stems = dependencies(sources)
    ids = [r["id"] for r in rows]
    assert len(ids) == len(set(ids)), "Duplicate patch IDs"
    assert set(sources) == {p for row in rows for p in row["sources"]}, "Uncatalogued source file"
    for row in rows:
        assert all(f in meta["figure_pages"] for f in row["figures"]), row["id"]
    (BASE / "COVERAGE.md").write_text(coverage(meta, sources, web, rows, graph, missing), encoding="utf-8")
    (BASE / "PD-PRIMITIVES.md").write_text(primitives(meta, sources, rows, graph, missing, stems), encoding="utf-8")
    print(f"Catalogued {len(sources)} active PD files in {len(rows)} work items; all source paths accounted for.")


if __name__ == "__main__":
    main()
