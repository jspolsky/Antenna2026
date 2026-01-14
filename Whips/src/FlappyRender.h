#pragma once

#include <stdint.h>

// Virtual resolution (4x physical)
#define FLAPPY_VIRTUAL_WIDTH 96
#define FLAPPY_VIRTUAL_HEIGHT 440

// Physical resolution
#define FLAPPY_PHYSICAL_WIDTH 24
#define FLAPPY_PHYSICAL_HEIGHT 110

// Scale factor
#define FLAPPY_SCALE 4

// Game element sizes (in virtual pixels)
#define FLAPPY_BIRD_WIDTH 4
#define FLAPPY_BIRD_HEIGHT 12
#define FLAPPY_BIRD_X 16          // Fixed horizontal position
#define FLAPPY_PIPE_WIDTH 8
#define FLAPPY_PIPE_HITBOX_MARGIN 2  // Shrink pipe collision hitbox by this much on each side
#define FLAPPY_GAP_SIZE 88        // Gap height in virtual pixels
#define FLAPPY_GROUND_HEIGHT 12

// Colors (RGB)
#define FLAPPY_COLOR_BIRD_R 255
#define FLAPPY_COLOR_BIRD_G 255
#define FLAPPY_COLOR_BIRD_B 0

#define FLAPPY_COLOR_PIPE_R 0
#define FLAPPY_COLOR_PIPE_G 200
#define FLAPPY_COLOR_PIPE_B 0

#define FLAPPY_COLOR_GROUND_R 139
#define FLAPPY_COLOR_GROUND_G 69
#define FLAPPY_COLOR_GROUND_B 19

#define FLAPPY_COLOR_SKY_R 0
#define FLAPPY_COLOR_SKY_G 0
#define FLAPPY_COLOR_SKY_B 0

// Game states
#define FLAPPY_STATE_READY 0
#define FLAPPY_STATE_PLAYING 1
#define FLAPPY_STATE_GAMEOVER 2

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Render the Flappy Bird game state to an RGB buffer.
 *
 * @param gameState  Current game state (0=ready, 1=playing, 2=gameover)
 * @param birdY      Bird Y position in virtual coordinates (0-439)
 * @param score      Current score (used for gameover display)
 * @param pipe1X     Pipe 1 X position (virtual), negative = inactive
 * @param pipe1GapY  Pipe 1 gap center Y (virtual)
 * @param pipe2X     Pipe 2 X position
 * @param pipe2GapY  Pipe 2 gap center Y
 * @param pipe3X     Pipe 3 X position
 * @param pipe3GapY  Pipe 3 gap center Y
 * @param scrollX    Scroll position for gameover score display (virtual pixels)
 * @param rgbBuffer  Output buffer, 24*110*3 bytes (row-major: [whip][led][rgb])
 */
void renderFlappyState(
    uint8_t gameState,
    uint16_t birdY,
    uint16_t score,
    int16_t pipe1X, uint16_t pipe1GapY,
    int16_t pipe2X, uint16_t pipe2GapY,
    int16_t pipe3X, uint16_t pipe3GapY,
    int16_t scrollX,
    uint8_t* rgbBuffer
);

/**
 * Render a single physical column (whip) from game state.
 * Used by SUB controllers to render just their column.
 *
 * @param whipIndex  Which whip (0-23)
 * @param gameState  Current game state
 * @param birdY      Bird Y position in virtual coordinates
 * @param score      Current score
 * @param pipe1X     Pipe 1 X position (virtual)
 * @param pipe1GapY  Pipe 1 gap center Y
 * @param pipe2X     Pipe 2 X position
 * @param pipe2GapY  Pipe 2 gap center Y
 * @param pipe3X     Pipe 3 X position
 * @param pipe3GapY  Pipe 3 gap center Y
 * @param scrollX    Scroll position for gameover score display
 * @param rgbBuffer  Output buffer, 110*3 bytes for this whip
 */
void renderFlappyColumn(
    uint8_t whipIndex,
    uint8_t gameState,
    uint16_t birdY,
    uint16_t score,
    int16_t pipe1X, uint16_t pipe1GapY,
    int16_t pipe2X, uint16_t pipe2GapY,
    int16_t pipe3X, uint16_t pipe3GapY,
    int16_t scrollX,
    uint8_t* rgbBuffer
);

#ifdef __cplusplus
}
#endif
