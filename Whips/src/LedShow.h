#pragma once

/*
 * LedShow is the DOM-side code that drives a show or animation
 * or chase or something pretty on the whips
 */

#include <WS2812Serial.h>
#define USE_WS2812SERIAL
#include <FastLED.h>
#include <EEPROM.h>
#include "IR.h"

namespace LedShow
{
    void setup();
    void loop(IR::Op op);
    void onButtonPress();
}
