# Controller Class

The `Controller` class provides an interface to interact with a joystick in Linux, capturing events from the axes and buttons. It allows the application to know whether the joystick is connected, if an event has occurred, and provides functions to access the values of the axes and buttons. Joystick events are read non-blocking using `select()`, which allows the application to continue running while waiting for joystick input.

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


# Control Index Settings
## Index (getAxis(n)) Common Movement
### 0 X-axis of the left analogue (Left/Right)
### 1 Left analogue Y-axis (Up/Down)
### 2 X-axis of the right analogue (Left/Right)
### 3 Y-axis of the right analogue (Up/Down) OR Triggers (L2/R2)
### 4 L2 trigger (left trigger)
### 5 R2 trigger (right trigger)
