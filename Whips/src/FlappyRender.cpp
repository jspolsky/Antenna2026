#include "FlappyRender.h"
#include <string.h>

// 5x7 digit font for score display (each digit is 5 pixels wide, 7 tall)
// Each row is stored as 5 bits (MSB on left)
static const uint8_t digitFont[10][7] = {
    {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}, // 0
    {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}, // 1
    {0b01110, 0b10001, 0b00001, 0b00110, 0b01000, 0b10000, 0b11111}, // 2
    {0b01110, 0b10001, 0b00001, 0b00110, 0b00001, 0b10001, 0b01110}, // 3
    {0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010}, // 4
    {0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110}, // 5
    {0b01110, 0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110}, // 6
    {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000}, // 7
    {0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110}, // 8
    {0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b01110}, // 9
};

// Font dimensions
#define FONT_WIDTH 5
#define FONT_HEIGHT 7

// Helper: clamp value to range
static inline int clamp(int val, int minVal, int maxVal)
{
    if (val < minVal)
        return minVal;
    if (val > maxVal)
        return maxVal;
    return val;
}

// Helper: check if a virtual pixel is occupied by the bird
static inline bool isBirdPixel(int vx, int vy, uint16_t birdY)
{
    int birdTop = birdY + FLAPPY_BIRD_HEIGHT / 2;
    int birdBottom = birdY - FLAPPY_BIRD_HEIGHT / 2;
    int birdLeft = FLAPPY_BIRD_X;
    int birdRight = FLAPPY_BIRD_X + FLAPPY_BIRD_WIDTH;

    return (vx >= birdLeft && vx < birdRight && vy >= birdBottom && vy < birdTop);
}

// Helper: check if a virtual pixel is occupied by a pipe
static inline bool isPipePixel(int vx, int vy, int16_t pipeX, uint16_t pipeGapY)
{
    if (pipeX < -FLAPPY_PIPE_WIDTH)
        return false; // Pipe inactive/off-screen

    int pipeLeft = pipeX;
    int pipeRight = pipeX + FLAPPY_PIPE_WIDTH;

    if (vx < pipeLeft || vx >= pipeRight)
        return false;

    // Gap is centered at pipeGapY
    int gapTop = pipeGapY + FLAPPY_GAP_SIZE / 2;
    int gapBottom = pipeGapY - FLAPPY_GAP_SIZE / 2;

    // Pipe exists above and below the gap (but not in the gap)
    return (vy < gapBottom || vy >= gapTop);
}

// Helper: check if a virtual pixel is ground
static inline bool isGroundPixel(int vy)
{
    return (vy < FLAPPY_GROUND_HEIGHT);
}

// Helper: check if virtual pixel is part of score digit during gameover scroll
static inline bool isScorePixel(int vx, int vy, uint16_t score, int16_t scrollX)
{
    // Score digits scaled to be 3/4 of screen height
    // Font is 5x7 pixels
    // Y scale: 47 -> 329 virtual pixels tall (~82 physical LEDs, 3/4 of screen)
    // X scale: 2.5 -> ~12 virtual pixels wide (~3 physical pixels)
    const int SCALE_X = 3;
    const int SCALE_Y = 47;
    const int digitWidth = FONT_WIDTH * SCALE_X;                  // 10 virtual pixels
    const int digitHeight = FONT_HEIGHT * SCALE_Y;                // 329 virtual pixels
    const int digitSpacing = digitWidth / 2;                      // 5 virtual pixels (half-space between digits)
    const int scoreY = (FLAPPY_VIRTUAL_HEIGHT - digitHeight) / 2; // Center vertically

    // Convert score to digits
    int digits[5];
    int numDigits = 0;
    int tempScore = score;
    if (tempScore == 0)
    {
        digits[0] = 0;
        numDigits = 1;
    }
    else
    {
        while (tempScore > 0 && numDigits < 5)
        {
            digits[numDigits++] = tempScore % 10;
            tempScore /= 10;
        }
        // Reverse
        for (int i = 0; i < numDigits / 2; i++)
        {
            int tmp = digits[i];
            digits[i] = digits[numDigits - 1 - i];
            digits[numDigits - 1 - i] = tmp;
        }
    }

    int totalWidth = numDigits * digitWidth + (numDigits - 1) * digitSpacing;

    // Position relative to scroll
    int relX = vx - scrollX;

    if (relX < 0 || relX >= totalWidth)
        return false;
    if (vy < scoreY || vy >= scoreY + digitHeight)
        return false;

    // Which digit?
    int digitIndex = relX / (digitWidth + digitSpacing);
    if (digitIndex >= numDigits)
        return false;

    int withinDigitX = relX - digitIndex * (digitWidth + digitSpacing);
    if (withinDigitX >= digitWidth)
        return false; // In spacing

    // Map to font pixel (font is 5x7)
    int fontX = withinDigitX / SCALE_X;
    int fontY = (vy - scoreY) / SCALE_Y;

    if (fontX >= FONT_WIDTH || fontY >= FONT_HEIGHT)
        return false;

    int digit = digits[digitIndex];
    uint8_t row = digitFont[digit][FONT_HEIGHT - 1 - fontY]; // Flip Y so 0 is bottom
    return (row >> (FONT_WIDTH - 1 - fontX)) & 1;
}

// Get the color of a virtual pixel
static void getVirtualPixelColor(
    int vx, int vy,
    uint8_t gameState,
    uint16_t birdY,
    uint16_t score,
    int16_t pipe1X, uint16_t pipe1GapY,
    int16_t pipe2X, uint16_t pipe2GapY,
    int16_t pipe3X, uint16_t pipe3GapY,
    int16_t scrollX,
    uint8_t *r, uint8_t *g, uint8_t *b)
{
    // Default: sky (black)
    *r = FLAPPY_COLOR_SKY_R;
    *g = FLAPPY_COLOR_SKY_G;
    *b = FLAPPY_COLOR_SKY_B;

    // Ground (always visible)
    if (isGroundPixel(vy))
    {
        *r = FLAPPY_COLOR_GROUND_R;
        *g = FLAPPY_COLOR_GROUND_G;
        *b = FLAPPY_COLOR_GROUND_B;
        return;
    }

    // Game over: show scrolling score
    if (gameState == FLAPPY_STATE_GAMEOVER)
    {
        if (isScorePixel(vx, vy, score, scrollX))
        {
            *r = 255;
            *g = 255;
            *b = 255;
        }
        return;
    }

    // Pipes (check all 3)
    if (isPipePixel(vx, vy, pipe1X, pipe1GapY) ||
        isPipePixel(vx, vy, pipe2X, pipe2GapY) ||
        isPipePixel(vx, vy, pipe3X, pipe3GapY))
    {
        *r = FLAPPY_COLOR_PIPE_R;
        *g = FLAPPY_COLOR_PIPE_G;
        *b = FLAPPY_COLOR_PIPE_B;
        return;
    }

    // Bird (drawn on top of pipes if overlapping)
    if (isBirdPixel(vx, vy, birdY))
    {
        *r = FLAPPY_COLOR_BIRD_R;
        *g = FLAPPY_COLOR_BIRD_G;
        *b = FLAPPY_COLOR_BIRD_B;
        return;
    }
}

// Render a single physical column with anti-aliasing
void renderFlappyColumn(
    uint8_t whipIndex,
    uint8_t gameState,
    uint16_t birdY,
    uint16_t score,
    int16_t pipe1X, uint16_t pipe1GapY,
    int16_t pipe2X, uint16_t pipe2GapY,
    int16_t pipe3X, uint16_t pipe3GapY,
    int16_t scrollX,
    uint8_t *rgbBuffer)
{
    // This whip covers virtual columns [whipIndex*4, whipIndex*4 + 3]
    int vxStart = whipIndex * FLAPPY_SCALE;

    // For each physical LED in this column
    for (int led = 0; led < FLAPPY_PHYSICAL_HEIGHT; led++)
    {
        // This LED covers virtual rows [led*4, led*4 + 3]
        int vyStart = led * FLAPPY_SCALE;

        // Accumulate color from all 16 virtual pixels (4x4 block)
        int rSum = 0, gSum = 0, bSum = 0;

        for (int dvx = 0; dvx < FLAPPY_SCALE; dvx++)
        {
            for (int dvy = 0; dvy < FLAPPY_SCALE; dvy++)
            {
                int vx = vxStart + dvx;
                int vy = vyStart + dvy;

                uint8_t r, g, b;
                getVirtualPixelColor(vx, vy, gameState, birdY, score,
                                     pipe1X, pipe1GapY,
                                     pipe2X, pipe2GapY,
                                     pipe3X, pipe3GapY,
                                     scrollX, &r, &g, &b);
                rSum += r;
                gSum += g;
                bSum += b;
            }
        }

        // Average the 16 samples
        int idx = led * 3;
        rgbBuffer[idx + 0] = rSum / (FLAPPY_SCALE * FLAPPY_SCALE);
        rgbBuffer[idx + 1] = gSum / (FLAPPY_SCALE * FLAPPY_SCALE);
        rgbBuffer[idx + 2] = bSum / (FLAPPY_SCALE * FLAPPY_SCALE);
    }
}

// Render the full display (all 24 columns)
void renderFlappyState(
    uint8_t gameState,
    uint16_t birdY,
    uint16_t score,
    int16_t pipe1X, uint16_t pipe1GapY,
    int16_t pipe2X, uint16_t pipe2GapY,
    int16_t pipe3X, uint16_t pipe3GapY,
    int16_t scrollX,
    uint8_t *rgbBuffer)
{
    for (int whip = 0; whip < FLAPPY_PHYSICAL_WIDTH; whip++)
    {
        renderFlappyColumn(
            whip, gameState, birdY, score,
            pipe1X, pipe1GapY,
            pipe2X, pipe2GapY,
            pipe3X, pipe3GapY,
            scrollX,
            &rgbBuffer[whip * FLAPPY_PHYSICAL_HEIGHT * 3]);
    }
}
