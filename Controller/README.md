# SEA:ME Instrument Cluster - Controller

The Controller component provides joystick interfacing for the Instrument Cluster system, allowing for user interaction with the display. It handles joystick input events in a non-blocking manner, normalizes axis values, and detects button presses.

## Features

- Non-blocking joystick event reading using `select()`
- Axis value normalization (-1.0 to 1.0 scale)
- Button state tracking
- Configurable button mapping
- Linux joystick interface integration

## Dependencies

- C++17 compatible compiler
- CMake 3.10+
- Linux joystick interface
- pthread

## Building

```bash
# From the project root
mkdir -p build && cd build
cmake ..
make
```

Or use the main build script:

```bash
./build.sh
```

## Running

```bash
# From the build/bin directory
./Controller
```

## Code Structure

- `inc/`: Header files
  - Controller class definition
  - Joystick event structures
- `src/`: Implementation files
  - Controller implementation
  - Main application logic

## Controller Class API

### `readEvent()`

**Purpose**: Reads a joystick event, processes it, and updates the axis and button data.

### Functionality:
- Uses the `select()` function to check if there are events ready to be read.
- If an event is available (indicated by `FD_ISSET`), the event is read using `read()`.
- If the reading is successful, the event type is checked:
  - **JS_EVENT_AXIS**: When a joystick axis is moved, the raw value (`_rawAxes[event.number]`) is updated, and the normalized value is calculated and stored in `_normalizedAxes`.
  - **JS_EVENT_BUTTON**: When a button is pressed or released, the button state is updated in the `_buttons` array. If the button corresponding to the "SELECT_BUTTON" is pressed (`event.value == 1`), the `_quit` variable is set to `true`, indicating that the application should quit.
- The event is processed, and the `_doEvent` variable is set to `true` to indicate that an event was read.

### `getAxis()`, `getRawAxis()`, `getButton()`

- **`getAxis(int axis)`**: Returns the normalized value of a specific axis (such as the X or Y axis) of the joystick. The normalized value is a floating-point number between -1 and 1.
- **`getRawAxis(int axis)`**: Returns the raw value of an axis (without normalization). This value can range from -32768 to 32767, depending on the device.
- **`getButton(int button)`**: Returns `true` if the specified button is pressed, otherwise returns `false`. The buttons are indexed by numbers.

### `_normalizeAxisValue(int value)`

**Purpose**: Normalizes the value of an axis to a range between -1 and 1.

### Functionality:
- Takes a raw axis value (an integer) and divides it by the maximum value (`MAX_AXIS_VALUE`), which is probably 32767 (the maximum possible value for a joystick axis).
- Returns the normalized value as a floating-point number.

## Control Index Settings

### Common Axis Mappings

| Index | Description |
|-------|-------------|
| 0 | X-axis of the left analogue (Left/Right) |
| 1 | Y-axis of the left analogue (Up/Down) |
| 2 | X-axis of the right analogue (Left/Right) |
| 3 | Y-axis of the right analogue (Up/Down) OR Triggers (L2/R2) |
| 4 | L2 trigger (left trigger) |
| 5 | R2 trigger (right trigger) |
