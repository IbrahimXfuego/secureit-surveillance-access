# Project record

## System overview

- Hackathon surveillance and access project with ESP32-CAM and ESP8266.
- One Telegram bot shared by both functions.
- AI-Thinker-style ESP32-CAM, MG996R servo, common PIR sensor.
- External button pull-up and debounce.
- Two AC/DC converters for electronics, separate motor supply.
- AC bulb switched through a relay.
- LCD number challenge, three choices, servo actuation and granted/denied feedback.
- Camera options, motion photos and dashboard access logs/livestream.

## Components

| Item | Quantity | Details |
| --- | --- | --- |
| AI-Thinker ESP32-CAM | 1 | Camera and flash LED |
| ESP8266 NodeMCU | 1 | Confirm exact carrier if different |
| MG996R servo | 1 | Independent regulated power |
| 16×2 I²C LCD | 1 | Default address `0x27` |
| PIR sensor | 1 | Motion detection input |
| DHT11 | 1 | Temperature/humidity sensor |
| Relay module | 1 | Confirm input polarity, logic compatibility and ratings |
| Push button | 1 | Normally open, to ground |
| Pull-up resistor | 1 | 10 kΩ suggested |
| I²C level shifter | 1 if needed | For a 5 V LCD backpack; suggested interface |
| Regulated power supplies | As required | Record voltage/current labels from actual adapters |
| USB-to-UART adapter | 1 | ESP32-CAM programming, 3.3 V UART logic |
| Lamp and enclosed relay assembly | 1 | AC load, relay-switched |

## Portfolio description

**SecureIT — Telegram-Based Surveillance and Access Control**

A hackathon IoT system combining ESP32-CAM surveillance, ESP8266 access control and a web dashboard.

- Built a button-triggered LCD number challenge with Telegram selection and MG996R lock actuation.
- Integrated camera capture, PIR-triggered photo alerts and relay-based light control.
- Combined local camera streaming with timestamped access-granted/access-denied history.
- Organized firmware and dashboard into a documented project with wiring, configuration and setup instructions.

Add project photos and demo links under `docs/media/`.
