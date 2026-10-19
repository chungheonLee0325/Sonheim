#!/usr/bin/env python3
"""Convert repository Markdown into flat pages for GitHub Wiki."""
from __future__ import annotations

import argparse
import re
from pathlib import Path
from urllib.parse import quote, unquote, urlsplit

LINK = re.compile(r'(!?\[[^\]\n]*\]\()([^\s)]+)(\))')
HTML_LINK = re.compile(r'''((?:src|href)\s*=\s*["'])([^"']+)(["'])''')
IMAGE_EXT = {'.png', '.jpg', '.jpeg', '.gif', '.webp', '.svg', '.bmp', '.avif'}
VIDEO_EXT = {'.mp4', '.webm', '.mov'}
SONHEIM_PAGE = re.compile(r'^(?:0[1-9]|1[0-9]|2[01])_[^/]+\.md


def page_name(path: Path, source: Path, profile: str) -> str:
    rel = path.relative_to(source).with_suffix('')
    if profile == 'agentmcp':
        if rel.as_posix() == 'Index.ko':
            return 'Home'
        return '-'.join(rel.parts)
    return rel.name


def legacy_sources(source: Path) -> list[Path]:
    """2025-era pages preserved in the main repository, not republished individually."""
    return sorted(p for p in source.glob('*.md') if LEGACY_PAGE.fullmatch(p.name))


def sources_for(source: Path, profile: str) -> list[Path]:
    if profile == 'sonheim':
        return sorted(p for p in source.glob('*.md')
                      if SONHEIM_PAGE.fullmatch(p.name) or p.name in {'Home.md', '_Sidebar.md', 'Legacy.md'})
    return sorted(source.rglob('*.md'))


def git_url(repo: str, branch: str, path: Path, image: bool) -> str:
    quoted = quote(path.as_posix(), safe='/._-')
    if image:
        return f'https://raw.githubusercontent.com/{repo}/{branch}/{quoted}'
    return f'https://github.com/{repo}/blob/{branch}/{quoted}'


def transform_url(value: str, current: Path, repo_root: Path, names: dict[Path, str],
                  repo: str, branch: str, image_markup: bool) -> str:
    if value.startswith(('#', 'mailto:', 'data:', '//')) or re.match(r'^[a-zA-Z][\w+.-]*:', value):
        return value
    parts = urlsplit(value)
    if not parts.path:
        return value
    target = (current.parent / unquote(parts.path)).resolve()
    try:
        rel = target.relative_to(repo_root)
    except ValueError:
        raise ValueError(f'Link leaves repository: {current}: {value}') from None
    if target in names:
        result = f'https://github.com/{repo}/wiki/{quote(names[target], safe="._-")}'
    else:
        suffix = target.suffix.lower()
        image = image_markup or suffix in IMAGE_EXT or suffix in VIDEO_EXT
        result = git_url(repo, branch, rel, image)
    if parts.query:
        result += '?' + parts.query
    if parts.fragment:
        result += '#' + parts.fragment
    return result


def rewrite(body: str, current: Path, repo_root: Path, names: dict[Path, str],
            repo: str, branch: str) -> str:
    result = []
    fenced = False
    for line in body.splitlines(keepends=True):
        if re.match(r'^\s*(?:\x60{3}|~~~)', line):
            fenced = not fenced
            result.append(line)
            continue
        if fenced:
            result.append(line)
            continue

        def md_link(match: re.Match) -> str:
            image = match.group(1).startswith('!')
            target = transform_url(match.group(2), current, repo_root, names, repo, branch, image)
            return match.group(1) + target + match.group(3)

        def html_link(match: re.Match) -> str:
            image = match.group(1).lower().startswith('src')
            target = transform_url(match.group(2), current, repo_root, names, repo, branch, image)
            return match.group(1) + target + match.group(3)

        segments = re.split(r'(\x60+[^\x60\n]+\x60+)', line)
        for index in range(0, len(segments), 2):
            segments[index] = HTML_LINK.sub(html_link, LINK.sub(md_link, segments[index]))
        result.append(''.join(segments))
    return ''.join(result)


def publish(source: Path, output: Path, repo: str, branch: str, profile: str) -> int:
    source, output = source.resolve(), output.resolve()
    repo_root = Path.cwd().resolve()
    if not source.is_dir() or not source.is_relative_to(repo_root):
        raise ValueError('Source must be an existing directory inside the repository')
    paths = sources_for(source, profile)
    if not paths:
        raise ValueError(f'No Markdown pages found in {source}')
    names = {p.resolve(): page_name(p, source, profile) for p in paths}
    if len(set(names.values())) != len(names):
        raise ValueError('Wiki page filename collision')

    output.mkdir(parents=True, exist_ok=True)
    for path, name in names.items():
        (output / f'{name}.md').write_text(
            rewrite(path.read_text(encoding='utf-8-sig'), path, repo_root, names, repo, branch),
            encoding='utf-8',
        )

    if profile == 'agentmcp':
        wiki = f'https://github.com/{repo}/wiki'
        sidebar = [
            '# Agent MCP',
            f'[한국어 홈]({wiki}/Home) · [English]({wiki}/Index)',
            '', '## 사용 및 구성',
            *[f'- [{label}]({wiki}/{name})' for label, name in [
                ('Architecture', 'Architecture.ko'), ('Tools', 'Tools.ko'),
                ('Skills', 'Skills.ko'), ('Workflows', 'Workflows.ko'),
                ('AI Chat', 'AiChat.ko'), ('UI Style', 'UiStyleSystem.ko'),
                ('UI Performance', 'PerformanceInvestigation.ko'),
                ('Testing', 'Testing.ko'), ('Settings', 'Settings.ko'),
                ('Safety', 'Safety.ko')]],
            '', f'[Repository](https://github.com/{repo})',
        ]
        (output / '_Sidebar.md').write_text('\n'.join(sidebar) + '\n', encoding='utf-8')

    path = source.relative_to(repo_root).as_posix()
    (output / '_Footer.md').write_text(
        f'[Repository](https://github.com/{repo}) · '
        f'[Documentation source](https://github.com/{repo}/tree/{branch}/{path})\n',
        encoding='utf-8',
    )
    print(f'Prepared {len(names)} GitHub Wiki pages for {repo} ({profile})')
    return len(names)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--repo', required=True)
    parser.add_argument('--branch', default='main')
    parser.add_argument('--profile', choices=['sonheim', 'agentmcp'], required=True)
    args = parser.parse_args()
    publish(args.source, args.output, args.repo, args.branch, args.profile)


if __name__ == '__main__':
    main()
)
LEGACY_PAGE = re.compile(r'^\d+\.\d+_[^/]+\.md


def page_name(path: Path, source: Path, profile: str) -> str:
    rel = path.relative_to(source).with_suffix('')
    if profile == 'agentmcp':
        if rel.as_posix() == 'Index.ko':
            return 'Home'
        return '-'.join(rel.parts)
    return rel.name


def sources_for(source: Path, profile: str) -> list[Path]:
    if profile == 'sonheim':
        return sorted(p for p in source.glob('*.md')
                      if SONHEIM_PAGE.fullmatch(p.name) or p.name in {'Home.md', '_Sidebar.md'})
    return sorted(source.rglob('*.md'))


def git_url(repo: str, branch: str, path: Path, image: bool) -> str:
    quoted = quote(path.as_posix(), safe='/._-')
    if image:
        return f'https://raw.githubusercontent.com/{repo}/{branch}/{quoted}'
    return f'https://github.com/{repo}/blob/{branch}/{quoted}'


def transform_url(value: str, current: Path, repo_root: Path, names: dict[Path, str],
                  repo: str, branch: str, image_markup: bool) -> str:
    if value.startswith(('#', 'mailto:', 'data:', '//')) or re.match(r'^[a-zA-Z][\w+.-]*:', value):
        return value
    parts = urlsplit(value)
    if not parts.path:
        return value
    target = (current.parent / unquote(parts.path)).resolve()
    try:
        rel = target.relative_to(repo_root)
    except ValueError:
        raise ValueError(f'Link leaves repository: {current}: {value}') from None
    if target in names:
        result = f'https://github.com/{repo}/wiki/{quote(names[target], safe="._-")}'
    else:
        suffix = target.suffix.lower()
        image = image_markup or suffix in IMAGE_EXT or suffix in VIDEO_EXT
        result = git_url(repo, branch, rel, image)
    if parts.query:
        result += '?' + parts.query
    if parts.fragment:
        result += '#' + parts.fragment
    return result


def rewrite(body: str, current: Path, repo_root: Path, names: dict[Path, str],
            repo: str, branch: str) -> str:
    result = []
    fenced = False
    for line in body.splitlines(keepends=True):
        if re.match(r'^\s*(?:\x60{3}|~~~)', line):
            fenced = not fenced
            result.append(line)
            continue
        if fenced:
            result.append(line)
            continue

        def md_link(match: re.Match) -> str:
            image = match.group(1).startswith('!')
            target = transform_url(match.group(2), current, repo_root, names, repo, branch, image)
            return match.group(1) + target + match.group(3)

        def html_link(match: re.Match) -> str:
            image = match.group(1).lower().startswith('src')
            target = transform_url(match.group(2), current, repo_root, names, repo, branch, image)
            return match.group(1) + target + match.group(3)

        segments = re.split(r'(\x60+[^\x60\n]+\x60+)', line)
        for index in range(0, len(segments), 2):
            segments[index] = HTML_LINK.sub(html_link, LINK.sub(md_link, segments[index]))
        result.append(''.join(segments))
    return ''.join(result)


def publish(source: Path, output: Path, repo: str, branch: str, profile: str) -> int:
    source, output = source.resolve(), output.resolve()
    repo_root = Path.cwd().resolve()
    if not source.is_dir() or not source.is_relative_to(repo_root):
        raise ValueError('Source must be an existing directory inside the repository')
    paths = sources_for(source, profile)
    if not paths:
        raise ValueError(f'No Markdown pages found in {source}')
    names = {p.resolve(): page_name(p, source, profile) for p in paths}
    if len(set(names.values())) != len(names):
        raise ValueError('Wiki page filename collision')

    output.mkdir(parents=True, exist_ok=True)
    for path, name in names.items():
        (output / f'{name}.md').write_text(
            rewrite(path.read_text(encoding='utf-8-sig'), path, repo_root, names, repo, branch),
            encoding='utf-8',
        )

    if profile == 'agentmcp':
        wiki = f'https://github.com/{repo}/wiki'
        sidebar = [
            '# Agent MCP',
            f'[한국어 홈]({wiki}/Home) · [English]({wiki}/Index)',
            '', '## 사용 및 구성',
            *[f'- [{label}]({wiki}/{name})' for label, name in [
                ('Architecture', 'Architecture.ko'), ('Tools', 'Tools.ko'),
                ('Skills', 'Skills.ko'), ('Workflows', 'Workflows.ko'),
                ('AI Chat', 'AiChat.ko'), ('UI Style', 'UiStyleSystem.ko'),
                ('UI Performance', 'PerformanceInvestigation.ko'),
                ('Testing', 'Testing.ko'), ('Settings', 'Settings.ko'),
                ('Safety', 'Safety.ko')]],
            '', f'[Repository](https://github.com/{repo})',
        ]
        (output / '_Sidebar.md').write_text('\n'.join(sidebar) + '\n', encoding='utf-8')

    path = source.relative_to(repo_root).as_posix()
    (output / '_Footer.md').write_text(
        f'[Repository](https://github.com/{repo}) · '
        f'[Documentation source](https://github.com/{repo}/tree/{branch}/{path})\n',
        encoding='utf-8',
    )
    print(f'Prepared {len(names)} GitHub Wiki pages for {repo} ({profile})')
    return len(names)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--repo', required=True)
    parser.add_argument('--branch', default='main')
    parser.add_argument('--profile', choices=['sonheim', 'agentmcp'], required=True)
    args = parser.parse_args()
    publish(args.source, args.output, args.repo, args.branch, args.profile)


if __name__ == '__main__':
    main()
)


def page_name(path: Path, source: Path, profile: str) -> str:
    rel = path.relative_to(source).with_suffix('')
    if profile == 'agentmcp':
        if rel.as_posix() == 'Index.ko':
            return 'Home'
        return '-'.join(rel.parts)
    return rel.name


def sources_for(source: Path, profile: str) -> list[Path]:
    if profile == 'sonheim':
        return sorted(p for p in source.glob('*.md')
                      if SONHEIM_PAGE.fullmatch(p.name) or p.name in {'Home.md', '_Sidebar.md'})
    return sorted(source.rglob('*.md'))


def git_url(repo: str, branch: str, path: Path, image: bool) -> str:
    quoted = quote(path.as_posix(), safe='/._-')
    if image:
        return f'https://raw.githubusercontent.com/{repo}/{branch}/{quoted}'
    return f'https://github.com/{repo}/blob/{branch}/{quoted}'


def transform_url(value: str, current: Path, repo_root: Path, names: dict[Path, str],
                  repo: str, branch: str, image_markup: bool) -> str:
    if value.startswith(('#', 'mailto:', 'data:', '//')) or re.match(r'^[a-zA-Z][\w+.-]*:', value):
        return value
    parts = urlsplit(value)
    if not parts.path:
        return value
    target = (current.parent / unquote(parts.path)).resolve()
    try:
        rel = target.relative_to(repo_root)
    except ValueError:
        raise ValueError(f'Link leaves repository: {current}: {value}') from None
    if target in names:
        result = f'https://github.com/{repo}/wiki/{quote(names[target], safe="._-")}'
    else:
        suffix = target.suffix.lower()
        image = image_markup or suffix in IMAGE_EXT or suffix in VIDEO_EXT
        result = git_url(repo, branch, rel, image)
    if parts.query:
        result += '?' + parts.query
    if parts.fragment:
        result += '#' + parts.fragment
    return result


def rewrite(body: str, current: Path, repo_root: Path, names: dict[Path, str],
            repo: str, branch: str) -> str:
    result = []
    fenced = False
    for line in body.splitlines(keepends=True):
        if re.match(r'^\s*(?:\x60{3}|~~~)', line):
            fenced = not fenced
            result.append(line)
            continue
        if fenced:
            result.append(line)
            continue

        def md_link(match: re.Match) -> str:
            image = match.group(1).startswith('!')
            target = transform_url(match.group(2), current, repo_root, names, repo, branch, image)
            return match.group(1) + target + match.group(3)

        def html_link(match: re.Match) -> str:
            image = match.group(1).lower().startswith('src')
            target = transform_url(match.group(2), current, repo_root, names, repo, branch, image)
            return match.group(1) + target + match.group(3)

        segments = re.split(r'(\x60+[^\x60\n]+\x60+)', line)
        for index in range(0, len(segments), 2):
            segments[index] = HTML_LINK.sub(html_link, LINK.sub(md_link, segments[index]))
        result.append(''.join(segments))
    return ''.join(result)


def publish(source: Path, output: Path, repo: str, branch: str, profile: str) -> int:
    source, output = source.resolve(), output.resolve()
    repo_root = Path.cwd().resolve()
    if not source.is_dir() or not source.is_relative_to(repo_root):
        raise ValueError('Source must be an existing directory inside the repository')
    paths = sources_for(source, profile)
    if not paths:
        raise ValueError(f'No Markdown pages found in {source}')
    names = {p.resolve(): page_name(p, source, profile) for p in paths}
    if len(set(names.values())) != len(names):
        raise ValueError('Wiki page filename collision')

    output.mkdir(parents=True, exist_ok=True)
    for path, name in names.items():
        (output / f'{name}.md').write_text(
            rewrite(path.read_text(encoding='utf-8-sig'), path, repo_root, names, repo, branch),
            encoding='utf-8',
        )

    if profile == 'agentmcp':
        wiki = f'https://github.com/{repo}/wiki'
        sidebar = [
            '# Agent MCP',
            f'[한국어 홈]({wiki}/Home) · [English]({wiki}/Index)',
            '', '## 사용 및 구성',
            *[f'- [{label}]({wiki}/{name})' for label, name in [
                ('Architecture', 'Architecture.ko'), ('Tools', 'Tools.ko'),
                ('Skills', 'Skills.ko'), ('Workflows', 'Workflows.ko'),
                ('AI Chat', 'AiChat.ko'), ('UI Style', 'UiStyleSystem.ko'),
                ('UI Performance', 'PerformanceInvestigation.ko'),
                ('Testing', 'Testing.ko'), ('Settings', 'Settings.ko'),
                ('Safety', 'Safety.ko')]],
            '', f'[Repository](https://github.com/{repo})',
        ]
        (output / '_Sidebar.md').write_text('\n'.join(sidebar) + '\n', encoding='utf-8')

    path = source.relative_to(repo_root).as_posix()
    (output / '_Footer.md').write_text(
        f'[Repository](https://github.com/{repo}) · '
        f'[Documentation source](https://github.com/{repo}/tree/{branch}/{path})\n',
        encoding='utf-8',
    )
    print(f'Prepared {len(names)} GitHub Wiki pages for {repo} ({profile})')
    return len(names)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--repo', required=True)
    parser.add_argument('--branch', default='main')
    parser.add_argument('--profile', choices=['sonheim', 'agentmcp'], required=True)
    args = parser.parse_args()
    publish(args.source, args.output, args.repo, args.branch, args.profile)


if __name__ == '__main__':
    main()
