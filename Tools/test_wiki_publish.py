#!/usr/bin/env python3
"""Tests for Wiki publishing links and page selection."""
import os
import tempfile
import unittest
from pathlib import Path

from wiki_publish import publish


class WikiPublishingTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.previous = Path.cwd()
        os.chdir(self.root)
        self.addCleanup(lambda: os.chdir(self.previous))

    def test_sonheim_links_and_code_fences(self):
        source = self.root / 'Doc/Wiki'
        source.mkdir(parents=True)
        (source / 'Home.md').write_text(
            '# Home\n[page](01_Test.md#overview)\n'
            '![HUD](../Media/Wiki/hud.png)\n'
            '[C++](../../Sonheim/Source/Sonheim/UI/Test.cpp)\n'
            '\x60[example](missing.md)\x60\n'
            '\x60\x60\x60md\n[keep](ignored.md)\n\x60\x60\x60\n',
            encoding='utf-8')
        (source / '01_Test.md').write_text('# Overview\n', encoding='utf-8')
        (source / '1.1_Legacy.md').write_text('# Old\n', encoding='utf-8')
        out = self.root / 'wiki'
        publish(source, out, 'example/Sonheim', 'main', 'sonheim')
        content = (out / 'Home.md').read_text(encoding='utf-8')
        self.assertIn('/wiki/01_Test#overview', content)
        self.assertIn('raw.githubusercontent.com/example/Sonheim/main/Doc/Media/Wiki/hud.png', content)
        self.assertIn('github.com/example/Sonheim/blob/main/Sonheim/Source/Sonheim/UI/Test.cpp', content)
        self.assertIn('[keep](ignored.md)', content)
        self.assertIn('[example](missing.md)', content)
        self.assertFalse((out / '1.1_Legacy.md').exists())

    def test_agentmcp_flatten_english_and_korean(self):
        source = self.root / 'Docs'
        (source / 'Samples').mkdir(parents=True)
        (source / 'Index.ko.md').write_text('# 문서\n[샘플](Samples/Trial.ko.md)\n', encoding='utf-8')
        (source / 'Index.md').write_text('[한국어](Index.ko.md)\n', encoding='utf-8')
        (source / 'Samples/Trial.ko.md').write_text('# Test\n[사진](../Images/example.png)\n', encoding='utf-8')
        out = self.root / 'wiki'
        publish(source, out, 'example/AgentMcp', 'main', 'agentmcp')
        self.assertIn('/wiki/Samples-Trial.ko', (out / 'Home.md').read_text())
        self.assertIn('/wiki/Home', (out / 'Index.md').read_text())
        self.assertIn('raw.githubusercontent.com/example/AgentMcp/main/Docs/Images/example.png',
                      (out / 'Samples-Trial.ko.md').read_text())
        self.assertTrue((out / '_Sidebar.md').exists())
        self.assertTrue((out / '_Footer.md').exists())


if __name__ == '__main__':
    unittest.main()
