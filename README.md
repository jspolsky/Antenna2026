Future Turtles 2026 Public LED Display
====

In 2024 we
* upped the number of whips to 24
* got rid of the central antenna
* improved cabling via daisy chaining

For 2026 we
* made the box at the foot of each whip smaller
* hard wired those boxes instead of using external connectors
* used two sources of power so that 16awg wires are adequate
* implemented a bird game

2026 TODO LIST:
===============
- [x] Fix 50 foot cable issues (see below)
- [x] Test game rendering with actual whips
- [x] Replace 600W power supplies with 300W
    - [x] Test temperature inside power packs
    - [x] Install new mounting plates     
- [x] Provide a way to separate left and right halves of whip control harness
    - [x] Modify setup documentation accordingly
- [ ] Implement game button
    - [x] Revise controller PCB to expose more pins (sent to JLCPCB)
    - [x] Rebuild controllers
    - [x] Cable to game button
    - [ ] Game button mounting hardware 
- [x] Finalize game code and flash it onto every MCU
- [ ] Set up a nice spare parts box


50-FOOT CABLE FIX
=================

Yes, the cable length is absolutely the problem. Here's what's happening and how to fix it.

## Why It Breaks with a Long Cable

A 50' cable introduces two nasty problems:

**1. It becomes an antenna.** The long wire efficiently picks up EMI from nearby mains wiring, motors, lighting, etc. This induced noise gets injected directly onto your input pin, causing phantom button presses or preventing real ones from registering cleanly.

**2. Capacitance loads the pullup.** Typical unshielded cable has ~30pF/foot of capacitance. At 50 feet that's ~1,500pF. The Teensy 4.1's internal pullup is a weak ~22kΩ–47kΩ. That RC combination means the pin rises very slowly when the button is released — well outside the clean logic thresholds the chip expects, causing unreliable reads.

---

## Fixes (use in combination)

**1. Add an external pullup resistor (most important)**

Replace the weak internal pullup with a strong external one. Solder a resistor directly from the signal pin to 3.3V *on the Teensy end* of the cable.

| Cable length | Recommended value |
|---|---|
| Up to ~10ft | 10kΩ (internal pullup is fine) |
| ~50ft | **1kΩ – 4.7kΩ** |

In your code, switch from `INPUT_PULLUP` to plain `INPUT` since you're now providing your own pullup.

**2. Add a bypass capacitor**

Place a **100nF ceramic capacitor** from the signal line to GND, right at the Teensy's pin header. This forms a low-pass RC filter with the series resistance of the cable, killing high-frequency noise before it reaches the pin.

```
3.3V
 |
[1kΩ–4.7kΩ]  ← external pullup
 |
 +----[Teensy Pin]
 |
[100nF]       ← bypass cap to GND
 |
GND
```

**3. Use shielded cable**

A shielded cable (e.g., mic cable, alarm wire, or screened 2-conductor) dramatically reduces the antenna effect. Connect the shield to GND at **one end only** (the Teensy end) to avoid ground loops.

**4. Improve software debouncing**

With a long cable you'll want a more generous debounce window even after the hardware fixes. A 20–50ms debounce is reasonable. If you're not already using a library, the [Bounce2](https://github.com/thomasfredericks/Bounce2) library works great on Teensy:

```cpp
#include <Bounce2.h>

Bounce button = Bounce();

void setup() {
  pinMode(BUTTON_PIN, INPUT);       // external pullup, so just INPUT
  button.attach(BUTTON_PIN);
  button.interval(25);              // 25ms debounce
}

void loop() {
  button.update();
  if (button.fell()) {              // button pressed (HIGH → LOW)
    // do the thing
  }
}
```

---

## Quick Summary

The combination of a **1–4.7kΩ external pullup** + **100nF cap to GND at the Teensy pin** will almost certainly solve this. Shielded cable and software debouncing are good additions if you're still seeing noise after that.