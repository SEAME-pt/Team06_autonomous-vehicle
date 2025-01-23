# CLUSTER DISPLAY

## General Context
This is a program writen in Qt which receives information from a socket using ZeroMQ and displays it on screen. The information displayed includes:
- **Speed (in Km/h).**
- **Battery level.**
- **Current time**

---

## System Structure
### 1. QML
- The actual display is done using QtQuick, an application written in QML (a language similar to JavaScript). All logic which needs no external values is done there, such as the time label (which is updated using a qml function) and the animations.
- Currently the animations consist in lateral bars which increase and decrease depending of the values received.
- To receive values from the outside, QML accesses values exposed by C++ classes.

### 2. C++
- Currently, each external value displayed has its own c++ class. This allows a different update rate for each value, reducing the amount of unnecessary data displayed and sent. This works great for the current situation, where only a small amount of information is necessary to be shown to the user.
- The class exposes the property to be shown to QML, which updates it on screen automatically, due to the signal that is emmited when the property changes.
- The value is received through ZeroMQ, a library which handles sockets at low level.

### 3. ZeroMQ
- Each C++ class inherits from a ZmqSubscriber class, which handles the communication with the "outside world".
- Currently, the communication only works one way. The pattern used is the "Publisher-Subscriber" pattern, where a publisher (in this case, the middleware) publishes a value and a subscriber receives it. This pattern will have to be revised in case Qt ever needs to communicate to the Middleware.

---

### Dependencies
- Linux
- Qt 5.9 required, with Qt.Quick library for QML support
- ZeroMQ library
