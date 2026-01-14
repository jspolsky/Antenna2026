#pragma once

#include <stdint.h>

// FastLED is needed before Commands.h for CRGB type
#include <WS2812Serial.h>
#define USE_WS2812SERIAL
#include <FastLED.h>

#include "Commands.h"

// Physics constants (all in virtual coordinates, per frame at 30 FPS)
#define FLAPPY_GRAVITY 1.0f           // Acceleration per frame
#define FLAPPY_FLAP_VELOCITY 10.0f    // Upward velocity on flap
#define FLAPPY_MAX_FALL_SPEED 12.0f   // Terminal velocity
#define FLAPPY_PIPE_SPEED 1           // Pixels per frame pipes move left

// Pipe spawning
#define FLAPPY_PIPE_SPAWN_X 100       // Spawn off-screen right
#define FLAPPY_PIPE_SPACING 48        // Horizontal distance between pipes
#define FLAPPY_GAP_MIN_Y 100          // Minimum gap center Y
#define FLAPPY_GAP_MAX_Y 340          // Maximum gap center Y

// Timing
#define FLAPPY_FRAME_MS 33            // ~30 FPS
#define FLAPPY_GAMEOVER_SCROLL_SPEED 2 // Score scroll speed
#define FLAPPY_READY_BOB_AMPLITUDE 8  // Bird bobbing amplitude in ready state
#define FLAPPY_READY_BOB_PERIOD 60    // Frames per bob cycle

class FlappyGame {
public:
    FlappyGame();

    // Call this every frame (~30 FPS)
    void update();

    // Call when button is pressed
    void onButtonPress();

    // Get current game state for broadcasting
    void getState(cmdFlappyState* state) const;

    // Check if game is active (not in attract/GIF mode)
    bool isActive() const { return gameState != STATE_INACTIVE; }

    // Start the game (transition from GIF attract mode)
    void start();

    // Reset to inactive state (return to GIF mode)
    void deactivate();

    // Game states
    static const uint8_t STATE_INACTIVE = 255;  // GIF/attract mode
    static const uint8_t STATE_READY = 0;
    static const uint8_t STATE_PLAYING = 1;
    static const uint8_t STATE_GAMEOVER = 2;

private:
    void resetGame();
    void spawnPipe(int pipeIndex);
    bool checkCollision() const;
    void updatePipes();
    void updateBird();
    void updateScore();
    void updateGameOver();
    void updateReady();

    // Game state
    uint8_t gameState;

    // Bird state
    float birdY;          // Virtual Y position
    float birdVelocity;   // Vertical velocity (positive = up)

    // Score
    uint16_t score;
    bool pipe1Scored;     // Track if we've scored for each pipe
    bool pipe2Scored;
    bool pipe3Scored;

    // Pipes (3 max)
    int16_t pipeX[3];     // X positions (negative = off-screen/inactive)
    uint16_t pipeGapY[3]; // Gap center Y positions

    // Game over animation
    int16_t scrollX;      // Score scroll position
    uint16_t gameOverFrames; // Frames since game over

    // Ready state animation
    uint16_t readyFrames; // Frames since entering ready state
};

// Global game instance
extern FlappyGame flappyGame;
