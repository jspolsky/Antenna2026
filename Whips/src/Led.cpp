#include <Arduino.h>
#include <PacketSerial.h>
#include <CRC16.h>
#include <CRC.h>

#include "pins.h"
#include "Util.h"
#include "Led.h"
#include "Commands.h"
#include "DipSwitch.h"
#include "Gif.h"
#include "FlappyRender.h"

namespace Led
{

    CRGB leds[NUM_LEDS];
    PacketSerial packetSerial;
    uint8_t brightness = 32;

    void setup()
    {
        Serial1.begin(2000000);
        packetSerial.setStream(&Serial1);
        packetSerial.setPacketHandler(&onPacketReceived);

        FastLED.addLeds<WS2812SERIAL, pinLEDStrip, BGR>(leds, NUM_LEDS);
        FastLED.setBrightness(brightness);
        FastLED.showColor(CRGB::DarkOrange);

        pinMode(pinLEDRxIndicator, OUTPUT);
    }

    void loop()
    {
        packetSerial.update();

        EVERY_N_MILLIS(200)
        {
            digitalWriteFast(pinLEDRxIndicator, LOW);
        }
    }

    void onPacketReceived(const uint8_t *buffer, size_t size)
    {
        if (size < sizeof(cmdUnknown))
        {
            // impossible packet doesn't even have room for checksum and command
            return;
        }

        cmdUnknown *punk = (cmdUnknown *)buffer;

        uint16_t checksum = punk->checksum;
        punk->checksum = 0; // when packet checksum was calculated, these 2 bytes were 0

        if (checksum != calcCRC16(buffer, size))
        {
            // packet garbled
            dbgprintf("garbled packet. Size was %d\n", size);
            return;
        }

        digitalWriteFast(pinLEDRxIndicator, HIGH);

        if (punk->whip != DipSwitch::getWhipNumber() && punk->whip != 255)
        {
            // not a message for us
            return;
        }

        switch (punk->chCommand)
        {
        case 'c':
        {
            cmdSetWhipColor *pSetWhipColor = (cmdSetWhipColor *)buffer;
            FastLED.showColor(pSetWhipColor->rgb);
        }
        break;

        case 'b':
        {

            cmdSetBrightness *pSetBrightness = (cmdSetBrightness *)buffer;
            if (brightness != pSetBrightness->brightness)
            {
                brightness = pSetBrightness->brightness;
                FastLED.setBrightness(128);
                int ixBrightness = map(brightness, 0, 255, 0, NUM_LEDS);
                for (int i = 0; i < ixBrightness; i++)
                {
                    leds[i] = CRGB::White;
                }
                for (int i = ixBrightness; i < NUM_LEDS; i++)
                {
                    leds[i] = CRGB::Black;
                }
                FastLED.show();
                FastLED.delay(200);
                FastLED.setBrightness(pSetBrightness->brightness);
            }
        }
        break;

        case 'g':
        {
            if (DipSwitch::getWhipNumber() <= 23)
            {
                static uint16_t iGifLoaded = 0;

                cmdShowGIFFrame *pShowGIFFrame = (cmdShowGIFFrame *)buffer;

                if (iGifLoaded != pShowGIFFrame->iGifNumber)
                {
                    Gif::LoadGif(iGifLoaded = pShowGIFFrame->iGifNumber);
                    dbgprintf("Loading gif number %d\n", pShowGIFFrame->iGifNumber);
                }
                Gif::GetFrame(pShowGIFFrame->frame, leds);
                FastLED.show();
            }
            else
            {
                FastLED.showColor(CRGB::Red);
            }
            break;
        }

        case 'i':
        {
            uint8_t whip = DipSwitch::getWhipNumber();

            for (int i = 0; i < NUM_LEDS; i++)
                leds[i] = CRGB::Black;

            for (int i = 0; i < 5; i++)
            {
                for (int led = i * 20; led < (i + 1) * 20 - 3; led++)
                {
                    if (whip & 0x01)
                        leds[led] = CRGB::White;
                }
                for (int led = (i + 1) * 20 - 3; led < (i + 1) * 20; led++)
                {
                    leds[led] = CRGB::Red;
                }
                whip >>= 1;
            }

            FastLED.show();
            break;
        }

        case 'f':
        {
            // Flappy Bird game state - render this whip's column
            cmdFlappyState *pFlappy = (cmdFlappyState *)buffer;
            uint8_t whipNum = DipSwitch::getWhipNumber();

            if (whipNum < FLAPPY_PHYSICAL_WIDTH)
            {
                // Buffer for RGB data (110 LEDs * 3 bytes)
                uint8_t rgbBuffer[FLAPPY_PHYSICAL_HEIGHT * 3];

                // Render just this whip's column
                renderFlappyColumn(
                    whipNum,
                    pFlappy->gameState,
                    pFlappy->birdY,
                    pFlappy->score,
                    pFlappy->pipe1X, pFlappy->pipe1GapY,
                    pFlappy->pipe2X, pFlappy->pipe2GapY,
                    pFlappy->pipe3X, pFlappy->pipe3GapY,
                    pFlappy->scrollX,
                    rgbBuffer);

                // Copy RGB buffer to FastLED array
                for (int i = 0; i < FLAPPY_PHYSICAL_HEIGHT; i++)
                {
                    leds[i].r = rgbBuffer[i * 3];
                    leds[i].g = rgbBuffer[i * 3 + 1];
                    leds[i].b = rgbBuffer[i * 3 + 2];
                }

                FastLED.show();
            }
            break;
        }
        }
    }
}
