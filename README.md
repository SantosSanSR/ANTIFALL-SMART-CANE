# AFSC — Anti-Fall Smart Cane

An ESP32-based assistive technology prototype exploring multisensor
event detection, local feedback, and remote notifications through Telegram.

Developed in the academic context of Bionic Engineering at
Universidad Popular Autónoma del Estado de Puebla (UPAEP), Puebla, Mexico.

**Project lead:** Santiago Alejandro Santos Rosas
**Status:** Academic prototype — validation pending

## Overview

AFSC integrates motion, impact, optical pulse sensing, and passive
infrared motion detection into a walking cane prototype.

The firmware combines sensor-trigger information using a state machine
that escalates from local audible and visual feedback to remote
Telegram notifications. A user acknowledgement button supports
alert cancellation.

The project explores embedded systems, physiological instrumentation,
assistive technology, and systems-engineering documentation.

Despite the project name, AFSC does not physically prevent falls.
It investigates the detection of possible events and notification
of a designated contact.

## Main features

- Acceleration and orientation monitoring using an MPU6050.
- Impact detection using three KY-031 modules.
- Experimental optical pulse estimation using a MAX30102.
- PIR-based motion/inactivity triggering.
- Multisensor trigger aggregation and escalating alert states.
- OLED display showing system state, trigger count, and sensor information.
- Audible feedback through a buzzer.
- Visual feedback through an LED indicator.
- Wi-Fi connectivity and Telegram notifications.
- User acknowledgement/cancellation button.
- Serial diagnostics and sensor initialization checks.

## Hardware

| Component | Quantity | Role |
|---|---:|---|
| ESP32-WROOM-32 development board | 1 | Processing and Wi-Fi communication |
| MPU6050 | 1 | Acceleration and orientation sensing |
| MAX30102 | 1 | Experimental optical pulse sensing |
| KY-031 | 3 | Impact-trigger inputs |
| HW-456 / SR505 mini PIR | 1 | Motion-trigger input |
| SSD1306 OLED, 128 × 64 | 1 | System feedback |
| Buzzer | 1 | Audible feedback |
| LED indicator | 1 | Visual feedback |
| Push button | 1 | User acknowledgement |
| Battery and power circuitry | As required | Portable power |
| Cane frame and wiring | As required | Mechanical integration |

Verify the electrical specifications of the actual modules before assembly.
The GPIO table below is a firmware mapping, not a complete wiring guide.

## Firmware GPIO mapping

| Signal | ESP32 GPIO |
|---|---:|
| Impact sensor 1 | 27 |
| Impact sensor 2 | 26 |
| Impact sensor 3 | 25 |
| Buzzer | 14 |
| Acknowledgement button | 2 |
| PIR input | 33 |
| LED indicator | 13 |
| I²C SDA | 21 |
| I²C SCL | 22 |

The button wiring and active logic level must be checked before use:
the design report contains inconsistent descriptions of the pull-up
configuration and pressed state.

## System architecture

```text
MPU6050 ────────────────┐
3 × KY-031 ────────────┤
MAX30102 ──────────────┼── ESP32 ── State machine ── OLED / LED / Buzzer
PIR motion input ──────┤                  │
Acknowledgement button┘                  └── Wi-Fi ── Telegram
```

### Trigger aggregation

The firmware aggregates four trigger categories:

1. Impact.
2. Motion/orientation-based possible fall event.
3. Optical pulse signal availability.
4. PIR-based absence of detected motion.

The three impact modules contribute to one impact category;
they are not counted as three independent alert categories.

| Active trigger categories | Firmware state |
|---:|---|
| 0 | `NORMAL` |
| 1 | `ALERT_1` |
| 2 | `ALERT_2` |
| 3 | `ALERT_3` |
| 4 | `CRITICO` |

Additional states include `CALIBRATING` and `CANCELLED`.

A high trigger count is a prototype classification rule.
It does not establish that a person has fallen or is experiencing
a medical emergency.

## Software dependencies

The firmware shown in the design report uses:

- Arduino framework with ESP32 board support.
- `Wire`
- `WiFi`
- `WiFiClientSecure`
- `UniversalTelegramBot`
- `MPU6050_tockn`
- `Adafruit_GFX`
- `Adafruit_SSD1306`
- `MAX30105` and `heartRate.h` from the SparkFun MAX3010x library.
- ESP32 system utilities through `esp_system.h`.

Dependency and board-package versions should be recorded after
a successful build. Third-party dependencies retain their own licenses.

## Getting started

1. Install Arduino IDE and ESP32 board support.
2. Install the required libraries and their dependencies.
3. Open the project firmware.
4. Review the wiring and verify module voltage compatibility.
5. Configure Wi-Fi and Telegram credentials locally.
6. Select the correct ESP32 board and serial port.
7. Compile and upload the firmware.
8. Open the Serial Monitor at 115200 baud.
9. Keep the device stationary during initialization and calibration.
10. Perform controlled bench tests before any demonstration.

Do not test falls by asking a person to fall.

### Credentials

Never commit real Wi-Fi passwords, Telegram bot tokens,
or personal chat identifiers.

For publication, replace embedded credentials with placeholders.
A recommended improvement is to move credentials into a local
`secrets.h` file and publish only `secrets.example.h`.

Creating these files alone does not change the firmware:
the sketch must be updated to include and use the local configuration.

## Validation status

The design report documents the architecture, firmware, and a test plan.
It does not establish quantified clinical or real-world performance.

Planned evaluation includes:

- Individual sensor checks.
- Controlled motion and impact simulations.
- State-machine transition testing.
- Acknowledgement-button testing.
- Telegram delivery and failure handling.
- Response-time measurement.
- Power consumption and battery-runtime testing.
- Mechanical integration checks.
- Repeatability and false-alert assessment.

Battery autonomy, detection accuracy, environmental resistance,
and notification latency must be treated as targets until measured
and supported by test records.

## Known limitations and review items

- Cane motion is not equivalent to body motion; dropping the cane
  may resemble a fall event.
- PIR activity is not a direct measurement of the user's mobility
  or consciousness.
- Optical pulse estimates depend on contact and signal quality.
- Loss of optical signal is not evidence of a cardiac emergency.
- The firmware does not demonstrate validated abnormal-heart-rate
  detection.
- Remote notifications depend on Wi-Fi, internet access, and Telegram.
- Blocking delays and network requests require latency evaluation.
- The report's firmware uses `client.setInsecure()`, disabling TLS
  certificate verification; this must be reviewed before deployment.
- The inactivity threshold differs between the report's test plan
  and the firmware.
- Button polarity and cancellation behavior require verification.
- An ultrasonic obstacle sensor is mentioned as a requirement,
  but is not implemented in the firmware shown in the report.
- No clinical validation, IP rating, or certified emergency-response
  performance is claimed.

## Documentation and demonstration

The project follows a PDR/CDR-style documentation process covering:

- Requirements.
- System architecture.
- Hardware selection.
- Firmware design.
- Integration.
- Test planning.
- Safety considerations.

Prototype demonstration:
[Watch the circuit demonstration](https://youtu.be/Oy1licVQWoQ)

## Safety notice

This is an academic and research prototype, not a clinically
validated medical device.

It must not be relied upon for diagnosis, treatment, fall prevention,
patient monitoring, or emergency response. It does not replace
a caregiver, certified assistive equipment, or emergency services.

## Authorship and acknowledgement

Project lead: Santiago Alejandro Santos Rosas.

When referencing this work, credit Santiago Alejandro Santos Rosas,
identify AFSC — Anti-Fall Smart Cane, and link to this repository.

Suggested acknowledgement:

> Based on AFSC — Anti-Fall Smart Cane, led by Santiago Alejandro
> Santos Rosas. Adaptations are the responsibility of their authors
> and do not imply endorsement by the original project lead.

Contributors and supervisors must also be credited for their actual
contributions. Project leadership does not, by itself, establish
exclusive ownership of every repository asset.

## Licensing

Original firmware is provided under the PolyForm Noncommercial
License 1.0.0. See `LICENSE`.

Original report text, photographs, and explanatory documentation
are provided under Creative Commons Attribution-NonCommercial
4.0 International. See `LICENSE-DOCUMENTATION.md`.

Third-party software and materials retain their original licenses.
Hardware manufacturing and patent rights are not granted by the
documentation license.

Commercial permissions outside these licenses require a separate
written agreement from the relevant rights holder(s).

These are source-available, noncommercial terms; this project is not
described as open source.
