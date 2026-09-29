# Dungeon UI style — 잊혀진 지하 유적

The curated dungeon's screens (HUD, result card, toast) as built by `Scripts/CuratedDungeon/author_styled_hud.py`. The style frames
are captures of those screens in play: `style_frame_hud.png` (guard room, wave A), `style_frame_result.png` (a won run) and
`style_frame_boss.png` (the guardian in phase 2). The token values live in that script and are written into the Widget Blueprints.

## Palette (linear RGBA)

| Token | Value | Use |
|---|---|---|
| Surface 82 / 92 | 0.012, 0.03, 0.06 at 0.82 / 0.92 | Card and panel fill, dark navy, translucent |
| Chip / Chip strong | white at 0.06 / 0.10 | Tiles, badges, reward slots |
| Line | white at 0.10 | 1 unit card outline, dividers |
| Text | white | Titles, values |
| Muted text | 0.55, 0.64, 0.75 | Notes, labels, arrows |
| Accent | 0.05, 0.6, 1.0 | Current step, kind badge, reward slot rim (at 0.45) |
| Gold | 1.0, 0.7, 0.2 | Time left, optional goals, counts, leader, rewards |
| Boss red | 0.85, 0.16, 0.1 | Boss health, phase 2, downed, failure |

## Kit

- Cards: rounded boxes, radius 14, 1 unit outline in Line (the optional card in Gold), padding 20 x 16. Drawn by the engine.
- Badges and tiles: rounded boxes, radius 6 (badges) and 10 (tiles, reward slots), Chip fills. Drawn by the engine.
- Bars: flat fills on a Track (black at 0.45); boss health 12 high, break gauge 4 high in Gold.
- Toast: a diamond (a small rotated box) holding a symbol, then category, title and detail.
- Type: Roboto, Bold for titles and values, Regular for the rest; 11 to 40 units.
- Nothing but the icons is textured. The icons (below, request `Art/Requests/dungeon_ui.json`) replaced the text glyphs
  (○ ● › ! ↗ ★) and the badge words (대장, 쓰러짐); a glyph still shows where an icon is missing.

## Icons

Icons replace those glyphs and badge words, and add a symbol where a line reads faster with one. They are one set:

- Flat, single-color white glyphs on a transparent background. The widget tints them with the tokens above, so one icon serves
  every state (Accent while current, Gold for optional, Boss red for danger, Muted when inactive).
- A bold silhouette that reads at 20 units: a stroke about a twelfth of the icon's width at the delivered size, rounded ends and
  joins, no hairlines, no inner detail smaller than the stroke.
- No gradients, outlines, drop shadows, glow, perspective or text inside the image; the widget adds shadow and color.
- Centered in the square with the requested padding, optically balanced (a round shape slightly larger than a square one).
- The dungeon is an old ruin guarded by an electric beast: stone and rune shapes for places, a jagged lightning spark only for the
  guardian's own icons.
- Emblems (the guardian, the outcome) follow the same rules at a larger size, with at most one level of inner detail.
