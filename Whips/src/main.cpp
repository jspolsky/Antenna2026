#include <Arduino.h>

#include "pins.h"
#include "Util.h"
#include "Led.h"
#include "LedShow.h"
#include "DipSwitch.h"
#include "SDCard.h"
#include "Gif.h"
#include "IR.h"
#include "Button.h"

static bool domMode;

void setup()
{
  Util::setup();
  pinMode(pinGndMeansDom, INPUT_PULLUP); // this takes a millisecond to pull up the pin; it MUST be before the delay() before you can start reading the pin

  dbgprintf("Starting\n");
  delay(100);

  if (!(SD.begin(pinSDCardCS)))
  {
    dbgprintf("Unable to access sd card\n");
    return;
  }

  domMode = (digitalReadFast(pinGndMeansDom) == LOW);
  dbgprintf("Whip controller in %s mode\n", domMode ? "DOM" : "SUB");

  if (domMode)
  {
    LedShow::setup();
    IR::setup();
    Button::setup();
  }
  else
  {
    DipSwitch::setup();
    Led::setup();
  }
  Gif::setup();
}

void loop()
{
  if (domMode)
  {
    IR::Op op = IR::loop();
    LedShow::loop(op);
    Button::loop();

    EVERY_N_SECONDS(20)
    {
      LedShow::loop(IR::nextImageSuggested);
    }
  }
  else
  {
    DipSwitch::loop();
    Led::loop();
  }
}
