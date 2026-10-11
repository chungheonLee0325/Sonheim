# Sonheim UI style — v2 (Palworld-like)

The look every new screen and notice follows: the notices (headline, feed, title), the tab menu, the monster book, the quest
journal and dialog, the HUD tracker. Reference frames are AI-made Palworld-style mockups the user supplied, kept outside the
repository (`A:\A_Workspace\UE\Reference\Palworld\Palworld_Fullscreen_UI_References_10`, 01–10); nothing of them is shipped.
The curated dungeon's HUD and result card still use the v1 tokens at the end of this file until they move over.

Widgets are built by scripts with the kit in `Scripts/UI/ui_kit.py`, which holds the values below; this file says what they mean.

## Principles

- **The kind of message picks the shape.** A big moment (level up, first capture, boss down) takes the centre alone; progress
  (quest accepted, updated, done; crafting; warnings) stacks on the left; a place name crosses the top. Never one card for all.
- **Light on the world, not boxes on it.** Bands fade out at their ends, panels are translucent navy, lines are thin and bright.
  No rounded "card" with a grey outline; corners are square or cut, framed by small bright brackets.
- **Colour says what it is.** Each role below has one colour; text stays white, with the colour on the icon, the band and the title.
- **Big, bold Korean.** Titles are heavy; body text is plain and short. Every text has a soft dark shadow so it reads on any sky.

## Colour roles (sRGB hex; linear values are in the kit)

| Role | Hex | Used for |
|---|---|---|
| Panel | #0A1626 at 82–90 % | Screen panels, list rows, the tracker |
| Panel raised | #13263D at 90 % | Rows, tiles, inputs on a panel |
| Line | #FFFFFF at 12 % | Dividers, row outlines |
| Text | #FFFFFF | Titles, values |
| Muted | #9FB3C8 | Captions, inactive tabs, notes |
| Cyan (accent) | #2CC8FF | Selection, level up, capture, info, tabs, quest accepted |
| Gold | #FFC63A | Boss, rewards, tracker header, counts, ready to report |
| Purple | #9B4DFF | Alpha / guardian moments |
| Green | #33D17A | Done, completed quest, checks |
| Orange | #FF9F1C | Updated, caution |
| Red | #FF4B4B | Danger, failure, damage |

## Type

| Use | Font | Size (1080p units) |
|---|---|---|
| Headline title (`LvUP!`, `BOSS ELIMINATED`, `최초로 포획했다!`) | F_BlackHanSans | 56–72, italic skew for LvUP only |
| Region title | F_BlackHanSans, letter spacing 600 | 52 |
| Screen title, big name (`꼬꼬닭`) | F_Pretendard ExtraBold | 34–44 |
| Section header (`파트너 스킬`, `목표 목록`) | F_Pretendard Bold | 18–20 |
| Body, row titles | F_Pretendard SemiBold | 15–17 |
| Captions, counts, keys | F_Pretendard Medium | 12–14 |

All text: shadow offset (0, 2), black at 60 %.

## Shapes and kit

- **Band** — a horizontal strip whose colour fades to nothing at both ends (headline) or toward the right (feed). Drawn by
  `M_UI_Band` (a UI material with Color, Fade and Edge parameters), with a 1 unit bright edge line top and bottom.
- **Emblem** — a diamond (rotated square) with a glyph, over a headline or at a region title's centre line.
- **Frame** — a screen panel: Panel fill, square corners, and short cyan L-brackets (18 × 18, 2 thick) at the four corners.
- **Section header** — glyph in a small diamond, bold title, then a full-width line in the section's colour; a coloured
  2 × 18 bar at the left for list group headers.
- **Row** — Panel raised, 1 unit Line outline; selected: 2 unit cyan outline and a soft cyan glow behind.
- **Tab bar** — dark strip across the top; tabs are equal cells with glyph and label; the active tab is filled cyan with a
  small diamond under it; Q and E key caps at the ends.
- **Key cap** — white rounded square, dark letter (`F`, `J`, `Esc`).
- **Check** — square box, 2 unit Muted outline; done is cyan filled with a white tick.
- **Quest mark** — over a quest giver's head while they have something for the player: a gold diamond with a dark `!` (a quest
  to give) or `?` (one to report).

## Notices

| Slot | Where | Holds | Moves |
|---|---|---|---|
| Headline | centre (42 % down), one at a time, clear of the title above | emblem, title, subtitle, up to 4 lines (stat +N, item ×N), a footer line | scales from 0.92 and fades in 0.25 s; a light sweep runs along the band; holds 3.5 s; fades 0.3 s |
| Feed | left, below the party list, stacks 3 | round icon, coloured title, name line, detail line, close | slides in from the left 0.25 s; holds 5 s; the oldest leaves first |
| Title | top centre (12 % down) | the region's name between two thin lines, small subtitle under, over a rounded background blur and a dark band fading at its ends, so it reads on sky and cave alike | grows in from 1.06 and fades in 0.45 s; holds 2.5 s |

Headline colours: cyan for level up and first capture, gold for a boss down, purple for the guardian's first defeat.
Feed colours: cyan accepted, orange updated, green completed, cyan crafting done, gold for a caution, red for danger.

## Screens

- Tab menu across the top for 인벤토리 · 보유 팰 · 도감 · 퀘스트 · 지도; screens without content show their tab dimmed.
- Monster book (ref 08): numbered rows with element badge and portrait at the right on the left; the 3D model on a ring in the
  middle with No., name and element above and the description under; records and rewards in sections on the right.
- Quest journal (ref 09): filter tabs (진행 중 · 완료 · 전체) and grouped rows with a thumbnail on the left; a banner header with
  title, place and description, then the objective list with checks and counts, then rewards and the track button.
- Dialog (ref 10): the speaker's name over a dark text box at the bottom centre, with bracket corners and a pink-cyan name
  underline; choices as a column of rows at the right, the focused one in cyan.

## v1 tokens (curated dungeon HUD and result card)

Surface #03080F at 82/92 %, chip white at 6/10 %, line white at 10 %, text white, muted (0.55, 0.64, 0.75), accent (0.05, 0.6,
1.0), gold (1.0, 0.7, 0.2), boss red (0.85, 0.16, 0.1); rounded boxes of radius 14 (cards), 10 (tiles), 6 (badges); Roboto.
Icons from `Art/Requests/dungeon_ui.json`: flat white glyphs tinted by the widget.
