#!/usr/bin/env python3
"""Prepare Sonheim's Doc/Wiki pages for the separate GitHub Wiki repository.

Published pages: Home, _Sidebar, and 01–18.
Media is copied from Doc/Media/Wiki to Media/Wiki, and GIFs link to main.
This script is invoked by the one-time GitHub Actions Wiki publisher.
"""
from __future__ import annotations

import re
import shutil
from pathlib import Path
from urllib.parse import quote

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT / "Doc" / "Wiki"
DEST = ROOT / ".wiki-publish"
WIKI_URL = "https://github.com/chungheonLee0325/Sonheim/wiki"
RAW_DOC_URL = "https://raw.githubusercontent.com/chungheonLee0325/Sonheim/main/Doc"

WIKI_LINK = re.compile(r"\[\[([^\]\|\n]+)\|([^\]\n]+)\]\]")
DOC_LINK = re.compile(r"\]\((?:\./)?([a-zA-Z0-9_.-]+)\.md(#[^)]*)?\)")
CHAPTER = re.compile(r"^(0[1-9]|1[0-8])_[^.]+\.md$")


def wiki_page(target: str) -> str:
    return f"{WIKI_URL}/{quote(target.removesuffix('.md'), safe='_-')}"


def convert_markdown(original: str) -> str:
    # Wiki is a different Git repository, so source-relative paths do not work.
    text = original.replace("../Media/Wiki/", "Media/Wiki/")
    text = text.replace("../Media/", f"{RAW_DOC_URL}/Media/")
    text = text.replace("../Gifs/", f"{RAW_DOC_URL}/Gifs/")
    text = text.replace(
        "https://github.com/chungheonLee0325/Sonheim/blob/feat-ui-dex/",
        "https://github.com/chungheonLee0325/Sonheim/blob/main/",
    )

    # Only convert the pipe-delimited Wiki links: bare [[...]] is valid Mermaid.
    text = WIKI_LINK.sub(
        lambda m: f"[{m.group(1)}]({wiki_page(m.group(2))})", text
    )
    # Relative file links in the main repo become actual Wiki page URLs.
    text = DOC_LINK.sub(
        lambda m: f"]({wiki_page(m.group(1))}{m.group(2) or ''})", text
    )

    if "../Media/" in text or "../Gifs/" in text:
        raise ValueError("Unresolved repository-relative media link")
    if "/blob/feat-ui-dex/" in text:
        raise ValueError("Unresolved source branch link")
    if WIKI_LINK.search(text):
        raise ValueError("Unconverted pipe-delimited Wiki link")
    return text


def main() -> None:
    chapters = sorted(p for p in SOURCE.glob("*.md") if CHAPTER.fullmatch(p.name))
    prefixes = {p.name[:2] for p in chapters}
    expected = {f"{i:02d}" for i in range(1, 19)}
    if len(chapters) != 18 or prefixes != expected:
        raise RuntimeError(f"Expected 18 chapters; got {[p.name for p in chapters]}")

    pages = [SOURCE / "Home.md", SOURCE / "_Sidebar.md", *chapters]
    for source in pages:
        if not source.is_file():
            raise FileNotFoundError(source)

    if DEST.exists():
        shutil.rmtree(DEST)
    DEST.mkdir(parents=True)

    published = {page.stem for page in pages}
    for source in pages:
        converted = convert_markdown(source.read_text(encoding="utf-8"))
        # Verify local Wiki page links. Remote attachments and code links are exempt.
        for name in re.findall(re.escape(WIKI_URL) + r"/([a-zA-Z0-9_.-]+)", converted):
            if name not in published:
                raise ValueError(f"{source.name}: missing Wiki page target {name}")
        (DEST / source.name).write_text(converted, encoding="utf-8")

    media = ROOT / "Doc" / "Media" / "Wiki"
    if media.exists():
        shutil.copytree(media, DEST / "Media" / "Wiki")

    # Validate that every copied Markdown reference to Wiki media exists.
    for page in pages:
        rendered = (DEST / page.name).read_text(encoding="utf-8")
        for asset in re.findall(r"\]\((Media/Wiki/[^)#\s]+)(?:#[^)]*)?\)", rendered):
            filename = asset
            if not (DEST / filename).is_file():
                raise FileNotFoundError(f"{page.name}: missing {filename}")

    print(f"Prepared {len(pages)} pages (18 chapters, Home, _Sidebar).")
    print(f"Copied {sum(p.is_file() for p in (DEST / 'Media' / 'Wiki').rglob('*')) if media.exists() else 0} media files.")
    print("Converted Wiki links and source branch links; preserved Mermaid nodes.")


if __name__ == "__main__":
    main()
