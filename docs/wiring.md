# Wiring and power

Use this with [wiring.svg](wiring.svg). The diagram follows the firmware pin mapping.

## ESP8266 NodeMCU

| Connection | NodeMCU | Notes |
| --- | --- | --- |
| Button input | D5 / GPIO14 | External pull-up to **3.3 V**; switch to GND |
| Servo signal | D6 / GPIO12 | MG996R signal; separate motor supply |
| LCD SDA | D2 / GPIO4 | Through I²C level shifter if backpack is at 5 V |
| LCD SCL | D1 / GPIO5 | Through I²C level shifter if backpack is at 5 V |
| LCD address | `0x27` | Change constructor if scan finds another address |
| Board power | USB or board-approved input | Do not apply 5 V to 3V3 |

Button: `3.3 V → R_pullup → D5 → switch → GND`.

Use an external pull-up to 3.3 V; **10 kΩ is a suggested value**. No capacitor is required by the firmware. `INPUT` mode is used. A stable falling edge after 40 ms generates one press; holding the button does not repeat it. Waiting, result and unlock states ignore additional presses.

## AI-Thinker ESP32-CAM

| Peripheral | Pin | Notes |
| --- | --- | --- |
| PIR OUT | GPIO13 | Expected active HIGH; confirm model and 3.3 V-compatible output |
| PIR VCC | Sensor-rated supply | Typical HC-SR501 uses 5 V; match supply to sensor model |
| Relay control | **GPIO14** | Revised from saved GPIO12; no microSD use |
| DHT11 data | GPIO2 | Use a pull-up to 3.3 V if the sensor board does not provide one |
| DHT11 VCC | 3.3 V | Confirm the sensor/module supports this supply |
| Flash LED | GPIO4 | Already on the camera board |
| Board supply | 5V and GND | Stable regulated supply with adequate headroom |

Do not use the microSD slot with this wiring. GPIO13, GPIO14, GPIO2 and GPIO4 overlap SD functions. Use only 3.3 V logic at MCU pins. Disconnect external loads/sensors during flashing if boot strapping is affected.

Relay interface: use a 3.3 V-compatible driver/module. Some nominal 5 V relay boards pull their IN terminal up to 5 V or do not reliably accept a 3.3 V HIGH. Do not connect such an input directly to the ESP32. Confirm the exact module or add a suitable transistor/opto interface. `RELAY_ACTIVE_LOW` is a configurable default, not an assertion about the installed module.

## Separate supplies

The build uses two AC/DC converters for the electronics and a separate motor supply. Match the regulated output voltage and current capacity to each load; do not parallel converter outputs.

- Supply A powers the camera side, with appropriately regulated rails for sensors/relay.
- Supply B powers the ESP8266/LCD side, with appropriately regulated rails.
- The separate regulated servo supply powers only the motor rail.
- Connect **servo supply negative to ESP8266 ground** for the signal reference.
- Keep the two Wi-Fi-connected electronics modules independent; they need no data wire or ground connection to each other.
- Never join positive outputs of separate converters together. Converter protective earth and mains wiring are outside this diagram.
- For a 5 V LCD backpack, connect the I²C shifter LV to 3.3 V, HV to the LCD's 5 V rail and GND to the access-board ground. Ensure the correct pull-ups on both sides.
- Install useful local decoupling near the boards and servo supply; choose values from actual supply/current behaviour.

The AC load circuit is intentionally shown only as an isolated load block. Use a properly rated enclosed relay assembly and qualified mains installation. Initial development can use a low-voltage lamp.
