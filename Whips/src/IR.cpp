#include <Arduino.h>

#define DECODE_NEC
#define DECODE_DISTANCE_WIDTH
#define EXCLUDE_UNIVERSAL_PROTOCOLS
#define EXCLUDE_EXOTIC_PROTOCOLS

#include <IRremote.hpp>

#include "Util.h"
#include "IR.h"
#include "pins.h"

namespace IR
{
    void setup()
    {
        IrReceiver.begin(pinIR);
        dbgprintf("IrReceiver ready\n");
    }

    Op loop()
    {
        if (IrReceiver.decode())
        {
            Serial.print(F("Decoded protocol: "));
            Serial.print(getProtocolString(IrReceiver.decodedIRData.protocol));
            Serial.print(F(", decoded raw data: "));
            Serial.print(IrReceiver.decodedIRData.decodedRawData, HEX);
            Serial.print(F(", decoded address: "));
            Serial.print(IrReceiver.decodedIRData.address, HEX);
            Serial.print(F(", decoded command: "));
            Serial.println(IrReceiver.decodedIRData.command, HEX);
            IrReceiver.resume();

            if (IrReceiver.decodedIRData.decodedRawData)
            {
                switch (IrReceiver.decodedIRData.command)
                {
                case 0x41:
                    // The ⏯️ button - go to the next image
                    return nextImage;

                case 0x5C:
                    return brighter;

                case 0x5D:
                    return dimmer;

                case 0x58:
                    return red;

                case 0x59:
                    return green;

                case 0x45:
                    return blue;

                case 0x44:
                    return white;

                case 0xB:
                    return flash;

                case 0x14:
                    return bottomUp;

                case 0x10:
                    return bottomDown;

                case 0x16:
                    return topUp;

                case 0x12:
                    return topDown;

                default:
                    break;
                }
            }
        }
        return noop;
    }
}