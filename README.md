# BlockDrop

A ncurses color-matching falling blocks puzzle game.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

## Description

**BlockDrop** is a terminal-based puzzle game that puts a unique spin on the falling blocks genre. Instead of clearing complete lines, players must match colors between adjacent blocks to clear them from the board. This creates a dynamic gameplay experience with chain reactions and strategic positioning.

### Key Features

- **12x18 Game Grid**: A spacious playing field for strategic block placement
- **5 Unique Piece Types**: 2-block and 3-block linear pieces, 4-block square and L-shaped pieces, and a 5-block T-shaped piece
- **5 Vibrant Colors**: Red, Yellow, Light Blue, Pink, and White
- **Color-Matching Gameplay**: Clear blocks by matching colors with neighboring pieces (not corners)
- **Chain Reactions**: Blocks fall individually after clearing, potentially triggering more matches
- **Progressive Difficulty**: Game speed increases by 30% for every 500 points
- **High Score Tracking**: Your best score is saved between sessions
- **Blink Effect**: Matching blocks blink before clearing (0.5 seconds)

## Gameplay

### Objective

Score points by clearing blocks through color matching. The game ends when a new piece cannot spawn (when the spawn area is blocked).

### Game Area

The game is played on a **12x18 grid** - 12 columns wide by 18 rows high.

When a piece lands, the game checks each block against its neighbors (up, down, left, right):
- **Match occurs when**: Same color AND different piece ID
- The flood-fill algorithm finds all connected matching blocks
- Matching blocks blink 3 times over 0.5 seconds before clearing
- Each block cleared earns 25 points

### Chain Reactions

After blocks clear, individual block gravity is applied:
1. Each column is scanned from bottom to top
2. Floating blocks fall down to fill empty spaces
3. If a falling block lands on a matching color (different piece), it triggers another clear
4. Chain reactions continue until no more matches occur

This mechanic rewards strategic play - setting up chains can significantly boost your score!

## Installation

### Dependencies

- `gcc` or compatible C compiler
- `ncurses` library and development headers
- `make`

On Debian/Ubuntu:
```bash
sudo apt-get install build-essential libncurses5-dev
```

On Fedora/RHEL:
```bash
sudo dnf install gcc ncurses-devel make
```

On Arch Linux:
```bash
sudo pacman -S gcc ncurses make
```

### Building

Clone or download the source code, then run:

```bash
make
```

### Installing System-Wide

To install:

```bash
sudo make install
```

### Uninstalling

```bash
sudo make uninstall
```

## Controls

| Key | Action |
|-----|--------|
| ← (Left Arrow) | Move piece left |
| → (Right Arrow) | Move piece right |
| ↓ (Down Arrow) | Move piece down faster |
| ↑ (Up Arrow) | *Restricted - no action* |
| Space | Rotate piece clockwise (90°) |
| Enter | Start / Pause game |
| Esc | First press: Pause and show exit warning<br>Second press: Exit game |

## Scoring System

- **25 points** per block cleared
- **Speed increase**: 30% faster for every 500 points
- **High score**: Automatically saved to `~/.blockdrop_highscore`

## Pieces

| Type | Size | Shape | Rotatable |
|------|------|-------|-----------|
| Linear 2 | 2 blocks |    Yes    |
| Linear 3 | 3 blocks |    Yes    |
| Square   | 4 blocks |    No     |
| L-Shape  | 4 blocks |    Yes    |
| T-Shape  | 5 blocks |    Yes    |



### Terminal Size

The game requires a terminal of at least **80 columns × 24 rows**. If the display appears garbled, try resizing your terminal window.

### Architecture

- **Grid**: 12×18 2D array of Block structs
- **Block**: Contains color ID (0-7) and piece ID (unique per piece)
- **Fall Timer**: Uses `clock_gettime()` for millisecond precision
- **Input**: Non-blocking ncurses input at ~60 FPS
- **Memory**: Dynamic allocation only for flood-fill search

### Algorithms

1. **Flood Fill**: Recursive 4-directional search for color matching
2. **Individual Gravity**: Column-by-column bottom-up scan
3. **Wall Kick**: Automatic adjustment when rotation hits boundaries


