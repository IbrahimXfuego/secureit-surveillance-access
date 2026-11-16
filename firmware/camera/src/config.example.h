#pragma once
const char* WIFI_SSID = "YOUR_WIFI";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";
const char* GATEWAY_URL = "http://192.168.1.10:3000";
const char* DEVICE_KEY = "SAME_RANDOM_KEY_AS_GATEWAY";
// Confirm these against the actual relay and PIR module before connecting the load.
constexpr bool RELAY_ACTIVE_LOW = true;
constexpr int PIR_ACTIVE_LEVEL = HIGH;
