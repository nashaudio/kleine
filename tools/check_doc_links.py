"""Report missing local Markdown link targets under the repository."""
from pathlib import Path
import re
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[1]
LINK = re.compile(r"!?\[[^\]]*\]\((?P<target>[^)]+)\)", re.DOTALL)

failures = []
for document in [ROOT / "README.md", ROOT / "AGENTS.md", *(ROOT / "docs").rglob("*.md")]:
    text = document.read_text(encoding="utf-8")
    fenced = False
    visible = []
    for line in text.splitlines(keepends=True):
        if line.lstrip().startswith("```"):
            fenced = not fenced
            visible.append("\n" if line.endswith("\n") else "")
            continue
        visible.append(("\n" if line.endswith("\n") else "") if fenced else line)
    visible_text = "".join(visible)
    for match in LINK.finditer(visible_text):
        target = match.group("target").strip()
        if target.startswith(("http://", "https://", "mailto:", "#", "/")):
            continue
        if target.startswith("<") and target.endswith(">"):
            target = target[1:-1]
        target = target.split("#", 1)[0].strip()
        if not target:
            continue
        path = (document.parent / unquote(target)).resolve()
        if not path.exists():
            number = visible_text.count("\n", 0, match.start()) + 1
            failures.append((document.relative_to(ROOT), number, target))

for document, line, target in failures:
    print(f"{document}:{line}: {target}")
raise SystemExit(bool(failures))
