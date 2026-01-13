#include <Arduino.h>

#include "Util.h"
#include "DipSwitch.h"
#include "pins.h"

uint8_t whip;

namespace DipSwitch
{
    void setup()
    {
        pinMode(pinDip16, INPUT_PULLUP);
        pinMode(pinDip8, INPUT_PULLUP);
        pinMode(pinDip4, INPUT_PULLUP);
        pinMode(pinDip2, INPUT_PULLUP);
        pinMode(pinDip1, INPUT_PULLUP);

        readWhipNumber();
    }

    void loop()
    {
        EVERY_N_MILLIS(1000)
        {
            readWhipNumber();
        }
    }

    uint8_t getWhipNumber()
    {
        return whip;
    }

    void readWhipNumber()
    {
        whip = (digitalReadFast(pinDip16) == HIGH ? 0 : 16) +
               (digitalReadFast(pinDip8) == HIGH ? 0 : 8) +
               (digitalReadFast(pinDip4) == HIGH ? 0 : 4) +
               (digitalReadFast(pinDip2) == HIGH ? 0 : 2) +
               (digitalReadFast(pinDip1) == HIGH ? 0 : 1);
    }
}
