# BLE Scan Qt

A Qt-based application for scanning Bluetooth Low Energy (BLE) devices.

## Build

1. Ensure you have a recent Qt (Qt 6 recommended) and a C++ toolchain installed.
2. Clone the repository.
3. Open the project in Qt Creator **or** build from the command line:

```bash
qmake
make
```

(If the project uses CMake instead of qmake, use `cmake -S . -B build && cmake --build build`.)

## Run

- From Qt Creator, run the target directly.
- From the command line, run the built binary (e.g., in the build output directory).

## License

This project is licensed under the GNU AGPLv3. See the LICENSE file for details.
