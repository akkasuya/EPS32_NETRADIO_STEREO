# ESP32 Net Radio

Simple internet radio for ESP32 + MAX98357 I2S amplifier, with a web UI.

## Hardware

- ESP32 dev board
- MAX98357 I2S amp
  - LRC  -> GPIO22
  - BCLK -> GPIO19
  - DIN  -> GPIO23
- Wi-Fi: SSID `DRBOM`

## Features

- Streams internet radio (MP3)
- 13 Thai/international stations
- Web UI at `http://netradio.local` (mDNS) or device IP
  - station list (click to play)
  - play/pause
  - volume slider
- Serial control: `n` = next, `p` = previous (115200 baud)

## Build & Flash (PlatformIO)

```bash
pio run
pio run -t upload
pio device monitor
```

## Libraries

- ESP32-audioI2S (schreibfaul1 / esphome fork)
- WebServer, ESPmDNS (built into ESP32 Arduino core)

## Notes

- MAX98357 `SD` (shutdown) pin may need to be tied to VCC to enable the amp.
- Some streams use plain HTTP; the library handles both HTTP and HTTPS.