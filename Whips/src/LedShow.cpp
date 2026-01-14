#include <Arduino.h>
#include <PacketSerial.h>
#include <CRC16.h>
#include <CRC.h>

#include "Util.h"
#include "IR.h"
#include "LedShow.h"
#include "Commands.h"
#include "Gif.h"
#include "Flappy.h"

namespace LedShow
{
    // brightness levels 0-19
    uint8_t rgBrightness[20] = {
        1, 2, 3, 4, 6,
        8, 10, 13, 16, 21,
        26, 34, 42, 55, 68,
        81, 110, 144, 178, 255};
    uint8_t ixBrightness = 0;
    bool fWriteEEPROM = false;

    PacketSerial packetSerial;

    enum Mode
    {
        gif,
        solid,
        selfID,
        flappy
    };

    // Moved to namespace scope so onButtonPress can access it
    static Mode modeCurrent = gif;

    void setup()
    {
        dbgprintf("In LedShow.Setup\n");
        Serial1.begin(2000000);
        packetSerial.setStream(&Serial1);
        ixBrightness = EEPROM.read(0);
        if (ixBrightness > 19)
        {
            ixBrightness = 19;
            EEPROM.write(0, ixBrightness);
        }
        dbgprintf("...done\n");
    }

    void loop(IR::Op op)
    {
        static int ixGif = 1;
        static CRGB rgbSolid = CRGB::Black;
        static int gifDelay = 40;

        if (op != IR::noop)
            dbgprintf("LedShow::op is %d\n", op);

        if (op == IR::nextImageSuggested)
        {
            if (modeCurrent != gif)
            {
                op = IR::noop;
            }
        }

        switch (op)
        {
        case IR::nextImage:
        case IR::nextImageSuggested:

            modeCurrent = gif;
            {
                bool bDone = false;
                while (!bDone)
                {
                    ixGif++;

                    // try to load that GIF to see if it exists and to get its frame rate
                    if (Gif::GetGifInfo(ixGif, gifDelay))
                    {
                        dbgprintf("new gif number %d\n", ixGif);
                        bDone = true;
                    }
                    else
                    {
                        ixGif = 0;
                    }
                }
            }

            break;

        case IR::red:
            rgbSolid = CRGB::Red;
            modeCurrent = solid;
            break;

        case IR::green:
            rgbSolid = CRGB::Green;
            modeCurrent = solid;
            break;

        case IR::blue:
            rgbSolid = CRGB::Blue;
            modeCurrent = solid;
            break;

        case IR::white:
            rgbSolid = CRGB::White;
            modeCurrent = solid;
            break;

        case IR::flash:
            modeCurrent = selfID;
            break;

        case IR::brighter:
            if (ixBrightness < 19)
            {
                ixBrightness++;
                fWriteEEPROM = true;
            }
            dbgprintf("brightness %d\n", ixBrightness);
            break;

        case IR::dimmer:
            if (ixBrightness > 0)
            {
                ixBrightness--;
                fWriteEEPROM = true;
            }
            dbgprintf("brightness %d\n", ixBrightness);
            break;

        default:
            break;
        }

        switch (modeCurrent)
        {
        case gif:
            EVERY_N_MILLIS_I(timerName, 400)
            {
                timerName.setPeriod(gifDelay);

                static cmdShowGIFFrame p3(255, 0, 1);
                static uint32_t frame = 0;
                p3.frame = frame++;
                p3.iGifNumber = ixGif;
                SendPacket(&p3, packetSerial);
            }
            break;

        case solid:
            EVERY_N_MILLIS(40)
            {
                cmdSetWhipColor p4(255, rgbSolid);
                SendPacket(&p4, packetSerial);
            }
            break;

        case selfID:
            EVERY_N_MILLIS(40)
            {
                cmdSelfIdentify p5;
                SendPacket(&p5, packetSerial);
            }
            break;

        case flappy:
            EVERY_N_MILLIS(FLAPPY_FRAME_MS)
            {
                flappyGame.update();

                // If game deactivated (returned to attract mode), switch back to gif
                if (!flappyGame.isActive()) {
                    modeCurrent = gif;
                } else {
                    cmdFlappyState flappyState;
                    flappyGame.getState(&flappyState);
                    SendPacket(&flappyState, packetSerial);
                }
            }
            break;
        }

        EVERY_N_MILLIS(100)
        {
            if (ixBrightness > 0 && ixBrightness < 20)
            {
                cmdSetBrightness p2(255, rgBrightness[ixBrightness]);
                SendPacket(&p2, packetSerial);

                if (fWriteEEPROM)
                {
                    EEPROM.write(0, ixBrightness);
                    fWriteEEPROM = false;
                }
            }
        }
    }

    void onButtonPress()
    {
        // If in GIF mode, start flappy game
        if (modeCurrent == gif) {
            modeCurrent = flappy;
            flappyGame.start();
        }
        // If already in flappy mode, pass button press to game
        else if (modeCurrent == flappy) {
            flappyGame.onButtonPress();
        }
    }

}
