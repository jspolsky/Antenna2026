#include <Arduino.h>

#include "pins.h"
#include "Util.h"
#include "Led.h"
#include "LedShow.h"
#include "DipSwitch.h"
#include "SDCard.h"
#include "Gif.h"
#include "IR.h"

static bool domMode;

void setup()
{
  Util::setup();
  dbgprintf("Starting\n");
  delay(100);

  if (!(SD.begin(pinSDCardCS)))
  {
    dbgprintf("Unable to access sd card\n");
    return;
  }

  pinMode(pinGndMeansDom, INPUT_PULLUP);
  domMode = (digitalReadFast(pinGndMeansDom) == LOW);
  dbgprintf("Whip controller in %s mode\n", domMode ? "DOM" : "SUB");

  if (domMode)
  {
    LedShow::setup();
    IR::setup();
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
