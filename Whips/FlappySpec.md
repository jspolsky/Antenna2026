# Flappy Bird - Gameplay Specification

## Display Overview

The game runs on a 24×110 LED array:
- **24 columns** (whips, numbered 0-23 from left to right)
- **110 rows** (LEDs per whip, 0 at bottom, 109 at top)
- Physical size: ~60 feet wide × 6 feet tall
- Viewed from a distance, this creates a wide, short display perfect for side-scrolling gameplay

## Anti-Aliasing / Virtual Resolution

The display is highly responsive to anti-aliasing, so the game uses a **virtual resolution** of **96×440** (4× in each dimension). All game logic runs in this higher resolution space, then downsamples to the physical 24×110 display.

### How It Works
- **Virtual grid**: 96 columns × 440 rows
- **Physical grid**: 24 columns × 110 rows
- **Mapping**: Each physical pixel represents a 4×4 block of virtual pixels
- **Rendering**: The color of each physical LED is the average of its 16 corresponding virtual pixels

### Benefits
- **Smooth motion**: Objects can move in 1/4-pixel increments, eliminating jerky movement
- **Smooth edges**: Pipe edges and the bird appear less blocky
- **Better physics**: Gravity and velocity calculations are more precise
- **Subpixel positioning**: The bird's vertical position feels continuous, not stepped

### Example
A pipe edge at virtual column 40 maps to physical column 10. As it moves left:
- Virtual 40 → Physical pixel 10 is 100% pipe color
- Virtual 39 → Physical pixel 9 is 75% pipe, 25% background (partial coverage)
- Virtual 38 → Physical pixel 9 is 50% pipe, 50% background
- ...and so on, creating smooth motion

## Command Architecture

The serial link between DOM and SUB controllers is bandwidth-constrained. Rather than sending full bitmaps (~8KB per frame), we send compact game state commands.

### cmdFlappyState Structure (~20 bytes)

```cpp
struct cmdFlappyState : cmdUnknown {
    uint8_t gameState;   // 0=ready, 1=playing, 2=gameover
    uint16_t birdY;      // 0-439 virtual Y position
    uint16_t score;
    // 3 pipes maximum
    int16_t pipe1X;      // virtual X (-32 to 96, negative = off-screen left)
    uint16_t pipe1GapY;  // gap center in virtual Y
    int16_t pipe2X;
    uint16_t pipe2GapY;
    int16_t pipe3X;
    uint16_t pipe3GapY;
};
```

### Communication Flow
1. DOM broadcasts `cmdFlappyState` to all SUBs (whip=255) each frame
2. Each SUB receives the same game state
3. Each SUB renders only its own column based on the state
4. All 24 whips stay perfectly synchronized

### Benefits
- **Minimal bandwidth**: ~20 bytes per frame vs ~8KB for full bitmap
- **Synchronized display**: All whips render from identical state
- **Distributed rendering**: Anti-aliasing math spread across 24 processors

## DOM vs SUB Responsibilities

### DOM Controller (Game Master)
- Runs the game loop at 30 FPS
- Handles button input (flap detection)
- Updates bird physics (position, velocity, gravity)
- Manages pipe spawning and movement
- Performs collision detection
- Tracks score
- Broadcasts game state to all SUBs

### SUB Controllers (Display Workers)
- Receive game state via serial
- Determine which virtual columns they represent (whip# × 4 to whip# × 4 + 3)
- Render their 110 LEDs based on game state
- Apply anti-aliasing when objects partially overlap their columns
- No game logic - purely display

## Shared Rendering Library

To ensure the Python visualizer matches the actual LED output exactly, the rendering code is written once in C++ and shared.

### Architecture
```
┌─────────────────────────────────────┐
│     FlappyRender.cpp/.dylib         │
│  renderFlappyState() → RGB buffer   │
└─────────────────────────────────────┘
         ▲                    ▲
         │                    │
    ┌────┴────┐          ┌────┴────┐
    │ SUB MCU │          │ Python  │
    │(direct) │          │(ctypes) │
    └─────────┘          └─────────┘
```

### C++ Interface
```cpp
extern "C" {
    void renderFlappyState(
        uint8_t gameState,
        uint16_t birdY,
        int16_t pipe1X, uint16_t pipe1GapY,
        int16_t pipe2X, uint16_t pipe2GapY,
        int16_t pipe3X, uint16_t pipe3GapY,
        uint8_t* rgbBuffer  // out: 24*110*3 bytes
    );
}
```

### Python Usage
```python
import ctypes
lib = ctypes.CDLL("path/to/libFlappyRender.dylib")
buffer = (ctypes.c_uint8 * (24 * 110 * 3))()
lib.renderFlappyState(state, birdY, p1x, p1gy, p2x, p2gy, p3x, p3gy, buffer)
# buffer now contains RGB data for all 24×110 LEDs
```

### Benefits
- Single source of truth for rendering
- Visualizer output matches hardware exactly
- Changes only need to be made in one place

## Game Elements

All dimensions are in **virtual coordinates** (96×440 grid). Physical display is 24×110.

### The Bird
- **Size**: 12 virtual pixels tall × 4 virtual pixels wide (renders as ~3 LEDs × 1 whip)
- **Position**: Fixed at virtual column 16 (physical column 4)
- **Color**: Bright yellow (RGB: 255, 255, 0)
- **Movement**: Vertical only, controlled by gravity and player input

### Pipes (Obstacles)
- **Structure**: Each pipe is a pair of vertical obstacles with a gap between them
  - Top pipe: Extends down from virtual row 439
  - Bottom pipe: Extends up from the ground
  - Gap: Opening between the pipes where the bird can fly through
- **Gap size**: 80-100 virtual pixels tall (~20-25 physical LEDs, adjustable for difficulty)
- **Width**: 8 virtual pixels wide (~2 physical whips for visibility)
- **Color**: Green (RGB: 0, 255, 0)
- **Movement**: Scroll from right (virtual column 95) to left (virtual column -8)
- **Spacing**: New pipe spawns when previous pipe reaches ~virtual column 60

### Background
- **Color**: Black (LEDs off) - the night sky

### Ground
- **Size**: Bottom 12 virtual pixels (~3 physical LEDs) of every column
- **Color**: Brown or dark orange (RGB: 139, 69, 19)
- **Purpose**: Visual reference and collision boundary (bird dies if it hits the ground)

## Game Mechanics

All physics values are in **virtual coordinates**.

### Physics

**Gravity**
- The bird constantly falls at an accelerating rate
- Fall speed increases over time (simulating acceleration)
- Maximum fall speed capped to prevent instant death

**Flap (Button Press)**
- Each button press gives the bird an upward velocity boost
- The boost is instant - bird immediately starts moving up
- Upward velocity decays quickly, then gravity takes over
- Multiple rapid presses can keep the bird climbing

**Suggested values (tunable, in virtual pixels):**
- Gravity: ~2.0 virtual pixels per frame²
- Flap boost: ~32 virtual pixels upward velocity
- Max fall speed: ~24 virtual pixels per frame
- Frame rate: ~30 FPS (33ms per frame)

### Pipe Generation

- Pipes spawn off-screen to the right (virtual column 96+)
- Gap position is randomized but constrained:
  - Gap center must be between virtual row 100 and virtual row 340
  - This prevents impossible gaps too close to top/bottom/ground
- Pipes move left at a constant speed (~1 virtual column per frame = smooth motion)
- Maximum 3 pipes on screen at once
- When a pipe scrolls completely off the left edge (virtual X < -8), it's removed

### Collision Detection

The bird collides (game over) when:
1. Bird touches any part of a pipe (top or bottom section)
2. Bird touches the ground (virtual Y < 12)
3. Bird goes above the ceiling (virtual Y > 439)

**Hit box**: The bird's 12 virtual pixel height is the collision zone. DOM performs all collision checks.

### Scoring

- **+1 point** each time a pipe's right edge passes the bird's column
- Score tracked by DOM controller
- **No high score feature** - intentional design to encourage player turnover

## Game States

The game integrates with the existing `LedShow::Mode` system. A new mode `flappy` is added alongside `gif`, `solid`, and `selfID`.

### 1. Attract Mode = GIF Mode (Existing)
- The normal animated GIF display serves as the attract mode
- Beautiful visuals draw people to the installation
- When a passerby discovers the button and presses it, the game begins
- **Transition**: Button press switches `LedShow::Mode` from `gif` to `flappy`

### 2. Ready State (Flappy Mode - Initial)
- Bird appears at starting position (virtual column 16, virtual Y ~220)
- Bird gently hovers/bobs to show it's alive and ready
- Ground is visible at bottom
- No pipes on screen yet
- Player has a moment to understand they're about to play
- **Transition**: Next button press starts the game

### 3. Playing State (Flappy Mode - Active)
- Full game physics active
- Pipes spawning and scrolling from right to left
- Score incrementing as pipes are passed
- Collision detection active
- **Transition**: Collision ends the game

### 4. Game Over State (Flappy Mode - End)
- Bird flashes or changes color (red) briefly
- All movement stops
- **Score display**: Large numerals scroll smoothly from right to left across all whips
  - Scrolling with anti-aliasing makes numbers readable despite 30" whip spacing
  - Numbers are tall (most of the whip height) for visibility
- After score finishes scrolling, automatically return to GIF mode (attract)
- **Transition**: Scroll completion returns to `gif` mode

## Visual Feedback

### Successful Pipe Pass
- Brief flash or color pulse when scoring
- Subtle enough not to distract, but rewarding

### Collision
- Screen flash (all whips briefly red or white)
- Bird changes color to red
- Pipes freeze in place momentarily

### Score Display (Game Over)
- Large numerals scroll smoothly right-to-left across all 24 whips
- Anti-aliased scrolling creates readable motion despite physical spacing
- Numbers fill most of the whip height for visibility from a distance

## Difficulty Progression (Optional)

As score increases:
- Pipe scroll speed increases slightly
- Gap size decreases slightly
- Pipe spacing decreases slightly

Starting easy, getting progressively harder keeps players engaged.

## Control Summary

| Input | Action |
|-------|--------|
| Button press | Flap (give bird upward velocity) |
| No input | Bird falls (gravity) |

That's it - one button, simple and intuitive!

## Technical Considerations (for implementation)

- Game loop runs on DOM controller at 30 FPS
- Each frame: update physics, check collisions, broadcast `cmdFlappyState`
- All 24 whips receive identical game state simultaneously
- Each SUB renders its column using shared `FlappyRender` library
- Shared library (.dylib) ensures Python visualizer matches hardware exactly
- Visualizer enables full development/testing without physical hardware
- Anti-aliasing calculations distributed across 24 SUB processors

## Future Enhancements (Beyond MVP)

- Sound effects via buzzer
- Two-player mode (birds at different columns)
- Different bird colors/characters
- Day/night themes
- Moving obstacles (birds, clouds)
- Power-ups (slow motion, invincibility)
