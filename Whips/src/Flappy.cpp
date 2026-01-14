#include "Flappy.h"
#include "FlappyRender.h"
#include "Util.h"
#include <stdlib.h>
#include <math.h>

// Global game instance
FlappyGame flappyGame;

FlappyGame::FlappyGame()
{
    deactivate();
}

void FlappyGame::deactivate()
{
    gameState = STATE_INACTIVE;
    birdY = FLAPPY_VIRTUAL_HEIGHT / 2;
    birdVelocity = 0;
    score = 0;
    scrollX = FLAPPY_VIRTUAL_WIDTH; // Start score off-screen right

    // Deactivate all pipes
    for (int i = 0; i < 3; i++)
    {
        pipeX[i] = -100;
        pipeGapY[i] = FLAPPY_VIRTUAL_HEIGHT / 2;
    }

    pipe1Scored = pipe2Scored = pipe3Scored = false;
    gameOverFrames = 0;
    readyFrames = 0;
}

void FlappyGame::start()
{
    resetGame();
    gameState = STATE_READY;
    readyFrames = 0;
}

void FlappyGame::resetGame()
{
    birdY = FLAPPY_VIRTUAL_HEIGHT / 2;
    birdVelocity = 0;
    score = 0;
    scrollX = FLAPPY_VIRTUAL_WIDTH;

    // Deactivate all pipes
    for (int i = 0; i < 3; i++)
    {
        pipeX[i] = -100;
        pipeGapY[i] = FLAPPY_VIRTUAL_HEIGHT / 2;
    }

    pipe1Scored = pipe2Scored = pipe3Scored = false;
    gameOverFrames = 0;
}

void FlappyGame::onButtonPress()
{
    dbgprintf("Flappy button: state=%d\n", gameState);
    switch (gameState)
    {
    case STATE_INACTIVE:
        // Button press in GIF mode starts the game
        dbgprintf("  -> starting game\n");
        start();
        break;

    case STATE_READY:
        // Button press in ready state starts playing
        dbgprintf("  -> start playing\n");
        gameState = STATE_PLAYING;
        birdVelocity = FLAPPY_FLAP_VELOCITY;
        // Spawn first pipe
        spawnPipe(0);
        break;

    case STATE_PLAYING:
        // Flap!
        dbgprintf("  -> flap!\n");
        birdVelocity = FLAPPY_FLAP_VELOCITY;
        break;

    case STATE_GAMEOVER:
        // Ignore button presses during game over animation
        dbgprintf("  -> game over, ignoring\n");
        break;
    }
}

void FlappyGame::update()
{
    static uint8_t lastState = 255;
    if (gameState != lastState)
    {
        dbgprintf("Flappy update: state changed to %d\n", gameState);
        lastState = gameState;
    }

    switch (gameState)
    {
    case STATE_INACTIVE:
        // Nothing to update - GIF mode handles display
        break;

    case STATE_READY:
        updateReady();
        break;

    case STATE_PLAYING:
        updateBird();
        updatePipes();
        updateScore();
        if (checkCollision())
        {
            dbgprintf("Collision detected!\n");
            gameState = STATE_GAMEOVER;
            gameOverFrames = 0;
        }
        break;

    case STATE_GAMEOVER:
        updateGameOver();
        break;
    }
}

void FlappyGame::updateReady()
{
    readyFrames++;
    // Bird bobs up and down gently
    float bobOffset = FLAPPY_READY_BOB_AMPLITUDE *
                      sinf(2.0f * 3.14159f * readyFrames / FLAPPY_READY_BOB_PERIOD);
    birdY = (FLAPPY_VIRTUAL_HEIGHT / 2) + bobOffset;
}

void FlappyGame::updateBird()
{
    // Apply gravity
    birdVelocity -= FLAPPY_GRAVITY;

    // Clamp fall speed
    if (birdVelocity < -FLAPPY_MAX_FALL_SPEED)
    {
        birdVelocity = -FLAPPY_MAX_FALL_SPEED;
    }

    // Update position
    birdY += birdVelocity;

    // Debug output every 10 frames
    static int debugCounter = 0;
    if (++debugCounter >= 10)
    {
        debugCounter = 0;
        dbgprintf("Bird: Y=%d vel=%d\n", (int)birdY, (int)birdVelocity);
    }

    // Clamp to screen bounds
    if (birdY < 0)
    {
        birdY = 0;
    }
    if (birdY > FLAPPY_VIRTUAL_HEIGHT - 1)
    {
        birdY = FLAPPY_VIRTUAL_HEIGHT - 1;
    }
}

void FlappyGame::spawnPipe(int pipeIndex)
{
    pipeX[pipeIndex] = FLAPPY_PIPE_SPAWN_X;
    // Random gap position between min and max
    pipeGapY[pipeIndex] = FLAPPY_GAP_MIN_Y +
                          (rand() % (FLAPPY_GAP_MAX_Y - FLAPPY_GAP_MIN_Y));

    // Reset scored flag for this pipe
    switch (pipeIndex)
    {
    case 0:
        pipe1Scored = false;
        break;
    case 1:
        pipe2Scored = false;
        break;
    case 2:
        pipe3Scored = false;
        break;
    }
}

void FlappyGame::updatePipes()
{
    // Move all active pipes left
    for (int i = 0; i < 3; i++)
    {
        if (pipeX[i] > -FLAPPY_PIPE_WIDTH)
        {
            pipeX[i] -= FLAPPY_PIPE_SPEED;
        }
    }

    // Find the rightmost active pipe
    int16_t rightmostX = -1000;
    int rightmostPipe = -1;
    for (int i = 0; i < 3; i++)
    {
        if (pipeX[i] > rightmostX)
        {
            rightmostX = pipeX[i];
            rightmostPipe = i;
        }
    }

    // Spawn new pipe if needed
    if (rightmostX < FLAPPY_VIRTUAL_WIDTH - FLAPPY_PIPE_SPACING)
    {
        // Find an inactive pipe slot
        for (int i = 0; i < 3; i++)
        {
            if (pipeX[i] <= -FLAPPY_PIPE_WIDTH)
            {
                spawnPipe(i);
                break;
            }
        }
    }
}

void FlappyGame::updateScore()
{
    // Check if bird has passed each pipe
    int birdRight = FLAPPY_BIRD_X + FLAPPY_BIRD_WIDTH;

    // Pipe 0
    if (!pipe1Scored && pipeX[0] + FLAPPY_PIPE_WIDTH < birdRight && pipeX[0] > -FLAPPY_PIPE_WIDTH)
    {
        score++;
        pipe1Scored = true;
    }

    // Pipe 1
    if (!pipe2Scored && pipeX[1] + FLAPPY_PIPE_WIDTH < birdRight && pipeX[1] > -FLAPPY_PIPE_WIDTH)
    {
        score++;
        pipe2Scored = true;
    }

    // Pipe 2
    if (!pipe3Scored && pipeX[2] + FLAPPY_PIPE_WIDTH < birdRight && pipeX[2] > -FLAPPY_PIPE_WIDTH)
    {
        score++;
        pipe3Scored = true;
    }
}

bool FlappyGame::checkCollision() const
{
    // Bird bounds
    int birdLeft = FLAPPY_BIRD_X;
    int birdRight = FLAPPY_BIRD_X + FLAPPY_BIRD_WIDTH;
    int birdTop = (int)birdY + FLAPPY_BIRD_HEIGHT / 2;
    int birdBottom = (int)birdY - FLAPPY_BIRD_HEIGHT / 2;

    // Ground collision
    if (birdBottom < FLAPPY_GROUND_HEIGHT)
    {
        return true;
    }

    // Ceiling collision
    if (birdTop >= FLAPPY_VIRTUAL_HEIGHT)
    {
        return true;
    }

    // Pipe collisions
    for (int i = 0; i < 3; i++)
    {
        if (pipeX[i] <= -FLAPPY_PIPE_WIDTH)
            continue; // Inactive pipe

        int pipeLeft = pipeX[i];
        int pipeRight = pipeX[i] + FLAPPY_PIPE_WIDTH;

        // Check horizontal overlap
        if (birdRight > pipeLeft && birdLeft < pipeRight)
        {
            // Gap bounds
            int gapTop = pipeGapY[i] + FLAPPY_GAP_SIZE / 2;
            int gapBottom = pipeGapY[i] - FLAPPY_GAP_SIZE / 2;

            // Collision if bird is outside the gap
            if (birdBottom < gapBottom || birdTop > gapTop)
            {
                return true;
            }
        }
    }

    return false;
}

void FlappyGame::updateGameOver()
{
    gameOverFrames++;

    // Scroll the score across the screen (every 3rd frame for slower scroll)
    static int scrollCounter = 0;
    if (++scrollCounter >= 3)
    {
        scrollCounter = 0;
        scrollX -= FLAPPY_GAMEOVER_SCROLL_SPEED;
    }

    // Calculate how wide the score display is (must match FlappyRender.cpp)
    // Digits are 10 virtual pixels wide (5 * 2 scale), 5px spacing (half-space)
    int numDigits = 1;
    int tempScore = score;
    while (tempScore >= 10)
    {
        tempScore /= 10;
        numDigits++;
    }
    int scoreWidth = numDigits * 10 + (numDigits - 1) * 8;

    // When score has scrolled completely off left side, return to GIF mode
    if (scrollX < -scoreWidth)
    {
        deactivate();
    }
}

void FlappyGame::getState(cmdFlappyState *state) const
{
    state->gameState = gameState;
    state->birdY = (uint16_t)birdY;
    state->score = score;
    state->pipe1X = pipeX[0];
    state->pipe1GapY = pipeGapY[0];
    state->pipe2X = pipeX[1];
    state->pipe2GapY = pipeGapY[1];
    state->pipe3X = pipeX[2];
    state->pipe3GapY = pipeGapY[2];
    state->scrollX = scrollX;
}
