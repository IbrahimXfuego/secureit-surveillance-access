# Setup

## 1. Configure Telegram

1. Create or reuse your project bot through Telegram's official **@BotFather**. Keep its token private.
2. Open a private chat with that bot and send `/start`.
3. Obtain your private chat ID and user ID from a Telegram update. You can call the official `getUpdates` endpoint locally using a token stored in an environment variable; never paste the token into this repository or a screenshot. In a private chat these IDs normally match, but use the actual values.
4. Run only this gateway as the bot's update consumer. Stop the old board firmware and any other poller. If the bot previously used a webhook, remove it with the official `deleteWebhook` API before polling.

Bot API reference: https://core.telegram.org/bots/api

## 2. Configure and flash boards

Use VS Code with the PlatformIO extension, or install the PlatformIO CLI. Open **each board folder** as its own project.

1. Copy `src/config.example.h` to `src/config.h` in each firmware project.
2. Set Wi-Fi SSID/password, the gateway PC's LAN address and the same long random `DEVICE_KEY` in both files.
3. Calibrate servo angles with the mechanism disconnected. The code defaults to 0° locked / 90° unlocked; these are setup values, not universal lock positions.
4. Confirm relay polarity and PIR output level. For the usual active-HIGH PIR, leave `PIR_ACTIVE_LEVEL = HIGH`.
5. Follow [wiring.md](wiring.md). Use **GPIO14 for the revised relay connection**, not the original GPIO12.

Camera build/upload, run from repository root:

```sh
pio run -d firmware/camera
pio run -d firmware/camera -t upload
pio device monitor -b 115200
```

Use a USB-to-UART adapter with 3.3 V UART signals. Power the ESP32-CAM through a suitable 5 V supply, connect UART TX to U0R and RX to U0T, and connect grounds. Connect GPIO0 to GND for uploading; remove that connection and reset for normal operation. Disconnect peripherals affecting boot if necessary. Do not join two independent 5 V supply outputs.

Access-board build/upload:

```sh
pio run -d firmware/access
pio run -d firmware/access -t upload
pio device monitor -b 115200
```

Use NodeMCU USB for programming. Avoid back-feeding USB when an external power source is attached; follow the board's power-input arrangement. Keep the servo on its own supply, with servo ground referenced to the access board.

Record the printed board IP addresses. Reserve these IPs in your router so the gateway configuration remains correct. Your PC also needs a stable LAN address.

## 3. Configure gateway

Install Node.js 22 or later. In `gateway`, create `.env` from `.env.example` and edit it:

- `BOT_TOKEN`: actual token, never committed.
- `OWNER_CHAT_ID` / `OWNER_USER_ID`: the owner’s private chat/user IDs.
- `DEVICE_KEY`: exactly the same random value as both firmware files.
- `CAMERA_URL`: camera origin, e.g. `http://192.168.1.50` (no path or port).
- `ACCESS_URL`: access-board origin, e.g. `http://192.168.1.51`.
- `PUBLIC_BASE_URL`: gateway’s reachable **LAN** URL, e.g. `http://192.168.1.10:3000`. “PUBLIC” means the link sent to the owner; it does not require Internet exposure.

Generate a shared key locally:

```sh
node -e "console.log(require('node:crypto').randomBytes(24).toString('hex'))"
```

Start the server:

```sh
cd gateway
npm test
npm start
```

No third-party Node packages are required. Allow incoming port 3000 only on the trusted private network in your PC firewall. Leave the server running while using Telegram controls. The public portfolio website and this local gateway are separate services.

## 4. Use

1. Open the dashboard LAN URL in a browser.
2. Send `/start` to the bot; use the keyboard for photos, temperature, relay and motion controls.
3. Enable motion alerts with `/motionOn`. Allow at least 30 seconds after boot for PIR warm-up; some modules need longer.
4. Press and release the access button. Read the LCD and select the matching Telegram choice within approximately 55 seconds.
5. The board accepts a valid decision while its 60-second local timeout is active. Unlock is five seconds; feedback then returns to idle.
6. A wrong number denies access. Another attempt requires another physical button press.
7. Open only one stream viewer initially. Streaming shares camera resources with still capture; available throughput depends on Wi-Fi and power quality.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| Bot silent | Gateway running; Internet access; correct owner IDs; one poller; no old webhook |
| Board unavailable | Printed IP; DHCP reservation; shared key; all devices on reachable LAN |
| LCD blank | Backpack address and contrast potentiometer; 5 V supply; level shifting |
| Button repeats | External pull-up to 3.3 V, switch to ground, firm wiring |
| Servo resets board | Independent adequately rated motor supply; common access-board/servo ground |
| Camera fails at boot | Stable power; ribbon; board model; bootstrap loading from peripherals |
| Relay inverted | Confirm module input polarity and `RELAY_ACTIVE_LOW` |
| Camera link fails from mobile data | Stream is local; connect to the same LAN or a properly configured VPN |
| DHT unavailable | Sensor wiring, pull-up and supply; allow a few seconds between readings |

Do not expose camera/API ports using router port forwarding. The local dashboard and stream do not implement public-user authentication.
