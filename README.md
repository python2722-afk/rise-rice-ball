# Rise Rice Ball Firmware

Firmware for **Rise Rice Ball (XJWB03)**, an ESP-IDF-based smart companion device with voice interaction, cloud connectivity, posture detection, and expressive hardware feedback.

## Features

- Real-time voice interaction and audio streaming
- Wi-Fi provisioning and cloud connectivity
- Course schedule, alarm, pomodoro, and media playback modules
- Camera-based posture detection with ESP-DL
- WS2812 LED ring effects and vibration feedback
- Servo, MPU6050, and peripheral control
- Emotion states driven by AI response tags and keyword matching

## Project Structure

The application source is located in `firmware/apps/main`:

```text
firmware/apps/main/
├── agent/           Voice agent and audio transport
├── cloud/           Cloud connection, provisioning, and time sync
├── lj_components/   Alarm, course, playback, pomodoro, and IoT control
├── peripheral/      Camera, LEDs, vibration, servo, and sensors
├── pose/            Posture detection and ESP-DL integration
├── voice/           Audio initialization and startup sound
├── CMakeLists.txt
└── main.c
```

## Requirements

- ESP-IDF 5.3 or later
- The matching `qumios-dev` platform SDK
- Component dependencies declared in `firmware/apps/main/idf_component.yml`

The build configuration looks for `qumios-dev` relative to the application directory. You can also set `ADF_PATH` to the directory containing `qumios-dev`.

## Configuration

Create a local cloud configuration file before building:

```bash
cp firmware/apps/main/cloud/ljyd_cloud_credentials.example.h \
   firmware/apps/main/cloud/ljyd_cloud_credentials.h
```

Then update the copied file with your product configuration:

```c
#define CONFIG_XJIANG_CLOUD_PID    YOUR_PRODUCT_ID
#define CONFIG_XJIANG_CLOUD_SECERT "YOUR_PRODUCT_SECRET"
```

The local credentials file is excluded by `.gitignore`.

## Build

Add `firmware/apps/main` to the matching ESP-IDF SDK workspace, configure the target board and dependencies, then build with:

```bash
idf.py build
```

Flash and monitor the device with:

```bash
idf.py flash monitor
```
