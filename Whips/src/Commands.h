#pragma once

#include <PacketSerial.h>

//
// a bunch of data structures sent over the serial wire
// from DOM to SUB with instructions to do things to the lights
//

// Here is a utility function that sends any command
// as a packet over the PacketSerial driver.
template <typename T>
void SendPacket(T *cmd, PacketSerial &packetSerial)
{
    cmd->checksum = 0;
    cmd->checksum = calcCRC16((uint8_t *)cmd, sizeof(T));
    packetSerial.send((uint8_t *)cmd, sizeof(T));
    visualize((uint8_t *)cmd, sizeof(T));
}

// minimize byte size to maximize the throughput
#pragma pack(push, 1)

// Every command has this header:
struct cmdUnknown
{
    cmdUnknown(char chCommand, uint8_t whip) : checksum(0),
                                               chCommand(chCommand),
                                               whip(whip) {}

    uint16_t checksum; // CRC16 checksum - will be filled in by SendPacket right before sending
    char chCommand;    // command. Use 'c' for cmdSetWhipColor, for example
    uint8_t whip;      // which whip should respond. 0 - 23 or 255 for all whips
};

/* Set an entire whip to the same color */
struct cmdSetWhipColor : cmdUnknown
{
    cmdSetWhipColor(uint8_t whip, CRGB rgb) : cmdUnknown('c', whip),
                                              rgb(rgb)
    {
    }

    CRGB rgb; // the color
};

/* Show a single frame from a GIF */
struct cmdShowGIFFrame : cmdUnknown
{
    cmdShowGIFFrame(uint8_t whip, uint32_t frame, uint16_t iGifNumber) : cmdUnknown('g', whip),
                                                                         frame(frame),
                                                                         iGifNumber(iGifNumber)
    {
    }

    uint32_t frame;      // frame will be increasing forever because DOM doesn't know how many
                         // frames are in the animation. Sub mods it by the number of frames in the animation.
    uint16_t iGifNumber; // We will look for a file named %03d.gif to display
};

/* Set Brightness */
struct cmdSetBrightness : cmdUnknown
{
    cmdSetBrightness(uint8_t whip, uint8_t brightness) : cmdUnknown('b', whip),
                                                         brightness(brightness)
    {
    }

    uint8_t brightness; // 0 (off) to 255 (blinding)
};

/* ID yourself */
struct cmdSelfIdentify : cmdUnknown
{
    cmdSelfIdentify() : cmdUnknown('i', 255)
    {
    }
};

/* Flappy Bird game state - broadcast to all whips each frame */
struct cmdFlappyState : cmdUnknown
{
    // Game state: 0=ready, 1=playing, 2=gameover
    static const uint8_t STATE_READY = 0;
    static const uint8_t STATE_PLAYING = 1;
    static const uint8_t STATE_GAMEOVER = 2;

    cmdFlappyState() : cmdUnknown('f', 255),  // Always broadcast to all whips
                       gameState(STATE_READY),
                       birdY(220),
                       score(0),
                       pipe1X(-100), pipe1GapY(0),
                       pipe2X(-100), pipe2GapY(0),
                       pipe3X(-100), pipe3GapY(0),
                       scrollX(96)
    {
    }

    uint8_t gameState;    // STATE_READY, STATE_PLAYING, or STATE_GAMEOVER
    uint16_t birdY;       // Bird vertical position in virtual coords (0-439)
    uint16_t score;       // Current score

    // Up to 3 pipes. X < 0 means pipe is off-screen left or inactive.
    // All coordinates in virtual space (96 wide x 440 tall)
    int16_t pipe1X;       // Pipe 1 X position (left edge), negative = inactive/off-screen
    uint16_t pipe1GapY;   // Pipe 1 gap center Y position
    int16_t pipe2X;
    uint16_t pipe2GapY;
    int16_t pipe3X;
    uint16_t pipe3GapY;

    int16_t scrollX;      // Scroll position for gameover score display (virtual pixels)
};

#pragma pack(pop)
