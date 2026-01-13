#include <Arduino.h>
#include <FastLED.h>
#include <OneButton.h>

#include "Util.h"
#include "Button.h"
#include "pins.h"

namespace Button
{
    OneButton button(pinButton);
    bool fButtonReady = false; // prevent the very first button release (from power up) from doing anything

    static void handleClick()
    {
        dbgprintf("click!\n");
    }

    void setup()
    {
        pinMode(pinButton, INPUT_PULLUP);
    }

    void loop()
    {

        if (!fButtonReady)
        {
            if (digitalReadFast(pinButton) == HIGH)
            {
                // button finally released from startup

                button.attachPress(handleClick);
                fButtonReady = true;
            }
        }
        else
        {
            button.tick();
        }
    }
}