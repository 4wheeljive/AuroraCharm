# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

AuroraCharm is a PlatformIO-based ESP32 project for controlling LED matrix displays via Bluetooth Low Energy (BLE). The project creates animated light patterns on a 6x10 LED matrix using the FastLED library and provides a web-based control interface.

## Build and Development Commands

### PlatformIO Commands
```bash
# Build the project
pio run

# Upload firmware to device
pio run --target upload

# Monitor serial output
pio run --target monitor

# Clean build files
pio run --target clean

# Build and upload in one command
pio run --target upload --target monitor
```

### Device Configuration
- **Target Board**: Seeed XIAO ESP32S3
- **Framework**: Arduino
- **Platform**: ESP32 (custom platform from GitHub)
- **Filesystem**: LittleFS

## Architecture Overview

### Core Components

**Main Application** (`src/main.cpp`)
- ESP32 setup and main loop
- LED matrix control using FastLED
- Deep sleep functionality with button wake-up
- Settings persistence using ESP32 Preferences
- Integration of BLE control and animation programs

**BLE Control System** (`src/bleControl.h`)
- Bluetooth Low Energy server implementation
- JSON-based parameter management using ArduinoJson
- Web interface communication protocols
- Preset save/load system using LittleFS
- Dynamic visualizer parameter mapping

**LED Matrix Mapping** (`src/matrixMap_6x10.h`)
- Multiple mapping configurations (progressive, serpentine)
- Support for different LED strip orientations
- Predefined mapping arrays for 6x10 matrix (60 LEDs total)

**Animation Programs** (`src/programs/`)
- Modular animation system with separate namespaces
- `rainbow.hpp` - Rainbow color effects
- `waves.hpp` - Wave-based animations
- Each program has corresponding detail implementation files

**Web Interface**
- `index.html` - Web-based control panel UI
- `script.js` - BLE Web API integration
- `style.css` - UI styling

### Key Architecture Patterns

**Namespace Organization**: Animation programs use separate namespaces (rainbow, waves) with init/run function patterns

**Parameter Management**: X-macro system in `bleControl.h` for automatic parameter serialization/deserialization

**Mapping System**: Flexible LED addressing through multiple mapping modes supporting different physical layouts

**BLE Communication**: JSON-based bidirectional communication with web interface using standardized characteristics

## Hardware Configuration

- **LED Data Pin**: D2 (configurable via DATA_PIN_1)
- **Wake-up Button**: GPIO 4
- **LED Matrix**: 6x10 WS2812B LEDs (60 total)
- **Power Management**: ESP32 deep sleep with external wake-up

## Development Notes

### Dependencies
- FastLED library (from GitHub)
- ArduinoJson v7.4.2
- ESP32 Arduino framework with PSRAM support

### Memory Management
- Uses PROGMEM for palette and mapping data
- LittleFS for preset file storage
- JSON documents for BLE communication

### Parameter System
The parameter management uses X-macros to automatically generate serialization code. When adding new parameters:
1. Add to PARAMETER_TABLE in `bleControl.h`
2. Add corresponding UI elements in web interface
3. Handle in appropriate process functions

### Visualizer 
1. The "visualizer" is a conceptual framework for referencing and working with either (1) a program-mode combination (e.g., waves-palette) or (2) a modeless program (e.g., rainbow).

### Animation Development
New animations should follow the pattern:
1. Create header file in `src/programs/`
2. Implement in separate detail header
3. Use namespace organization
4. Provide init() and run() functions
5. Add to main program switch statement


THIS EXPLANATION JUST ADDED: 
**Framework Core:**
- **src/main.cpp**: Main program loop, hardware setup, program switching, global mappings
- **src/bleControl.h**: BLE server, characteristic handling, preset management, cVariable parameters
- **src/matrixMap_6x10.h**: LED coordinate mapping (60 LEDs, single pin)  
- **platformio.ini**: Build configuration, dependencies, board settings
- **index.html**: Complete web-based control interface with Web Bluetooth API

**LED Program Modules:** *(Each program has .hpp interface + _detail.hpp implementation)
- **src/programs/rainbow.hpp** + **src/programs/rainbow_detail.hpp**: Simple rainbow matrix animation  
- **src/programs/waves.hpp** + **src/programs/waves_detail.hpp**: Pride wave algorithm with palette rotation
- other programs added periodically

**Program Organization Notes:**
- Each program uses the circular dependency pattern: main.cpp → program.hpp → program_detail.hpp → bleControl.h
- Detail files contain all implementation and can access cVariable parameters for future real-time control
- Programs with XY coordinate mapping use function pointer pattern for main.cpp XY function access