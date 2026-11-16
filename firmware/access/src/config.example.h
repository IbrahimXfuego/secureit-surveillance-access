#pragma once
const char* WIFI_SSID = "YOUR_WIFI";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";
const char* GATEWAY_URL = "http://192.168.1.10:3000";
const char* DEVICE_KEY = "SAME_RANDOM_KEY_AS_GATEWAY";
constexpr int LOCKED_ANGLE = 0;
constexpr int UNLOCKED_ANGLE = 90; // Calibrate without attaching the lock mechanism.
constexpr unsigned long UNLOCK_MS = 5000;
constexpr unsigned long CHALLENGE_MS = 60000;
constexpr unsigned long DEBOUNCE_MS = 40;
