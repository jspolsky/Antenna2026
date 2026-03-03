#pragma once

/*
 * IR receiver, used only on DOM for misc. controls
 */

#include <WS2812Serial.h>
#define USE_WS2812SERIAL
#include <FastLED.h>

namespace IR
{
    enum Op
    {
        noop,
        nextImage,
        nextImageSuggested,
        red,
        green,
        blue,
        white,
        flash,
        brighter,
        dimmer,
        bottomUp,
        bottomDown,
        topUp,
        topDown,
    };

    void setup();
    Op loop();
}
