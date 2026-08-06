# HLK-W800-KIT — wifi_con_test

WiFi connection test for the **W800** (full wireless stack populated).
Ported from the ESP-IDF `c3-classic/wifi_con_test` example.

> Build / flash instructions: see the [board README](../README.md).

## What it does

1. Scans the air and prints all found APs (SSID, RSSI, channel, auth mode).
2. Connects to the AP from `main/src/wifi_config.h` (WPA2, 30 s timeout).
3. If WPA2 fails, retries with WPA/WPA2 mixed mode (20 s timeout).
4. Prints the assigned IP from DHCP.
5. Loop: reports WiFi status (UP/DOWN + RSSI) every 15 s and blinks **LED1**
   (PB1 = red channel of the RGB LED, low-active blink) as an activity
   indicator.

## Credentials

Target AP is read from `main/src/wifi_config.h`:

```c
#define DEFAULT_AP      "HIKVISION_AE4490_2G4"
#define DEFAULT_PASSWD  "fastFAST"
```

## Verified result (real AP)

```
--- Scanning for available APs ---
  Found N AP(s)
  ...
--- Connecting to "HIKVISION_AE4490_2G4" (WPA2, 30s timeout) ---
WiFi: connected to AP
WiFi: got IP 192.168.5.10
  LINK_UP
Connected successfully
  IP: 192.168.5.10
--- WiFi status: UP ("HIKVISION_AE4490_2G4", RSSI: -62 dBm) ---
  IP: 192.168.5.10
```

DHCP-assigned address was `192.168.5.10`, RSSI ≈ -62 dBm, link stable for
the full observation window.
