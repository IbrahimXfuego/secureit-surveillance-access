# Checks and integration

## Gateway checks

Run `npm test` inside `gateway/`. Checks cover correct/wrong number selection, single-use challenges, expiry, owner filtering, duplicate rejection, device keys and HTTP routes.

## Firmware build

After creating the local configuration files:

```sh
pio run -d firmware/camera
pio run -d firmware/access
```

## Integration sequence

1. Confirm button idle HIGH / pressed LOW and one challenge per press.
2. Match the LCD number in Telegram and check the five-second unlock action.
3. Select a wrong number and check denial without servo movement.
4. Check expired and previously used challenges cannot unlock.
5. Check photos, flash, temperature, relay and motion alerts.
6. Open the livestream while requesting a still photo.
7. Check dashboard timestamps and retained history after gateway restart.
8. Reboot each board and check configured startup states.
