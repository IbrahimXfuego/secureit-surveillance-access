# Software overview

- One Node.js Telegram poller routes controls to both boards.
- Camera menu includes still photos, flash, temperature, relay, motion controls and livestream link.
- A separate streaming server coordinates camera access with still capture.
- The access button uses an external pull-up and stable-edge debounce.
- Unlock/result timing, one-use challenges and expiry are handled in firmware.
- Owner checks and a shared device key protect command routes.
- Dashboard access history is stored on disk and appears to the left of the stream.

## Configuration

The relay uses GPIO14. Set relay polarity to match the module. Servo unlock defaults to 90°; calibrate the mechanism before attaching it. PIR input defaults active HIGH. See the example configuration headers and wiring guide.
