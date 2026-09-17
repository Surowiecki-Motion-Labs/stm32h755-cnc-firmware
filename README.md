# STM32H755 CNC Firmware

Firmware for a prototype CNC controller based on the dual-core STM32H755 microcontroller.
The project separates high-level command handling from real-time motion execution:

- Cortex-M7 handles USB CDC communication, text commands, G-code parsing, program buffering, modal machine state and IPC command generation.
- Cortex-M4 executes motion commands, generates STEP/DIR/ENA signals, updates axis state and publishes real-time status.

## Main Features

- Dual-core STM32H755 architecture.
- USB CDC text protocol visible as a virtual COM port.
- G-code parser for `G0`, `G1`, `G28`, `G90`, `G91`, `M3`, `M4`, `M5`.
- Program buffer for streamed machining programs.
- Shared-memory IPC between Cortex-M7 and Cortex-M4.
- STEP/DIR/ENA generation for three stepper axes.
- Timer-based pulse generation using TIM1 and TIM2.
- Axis configuration through `config.h`, `config_cm7.h` and `config_cm4.h`.
- Real-time status reporting for HMI integration.

## Project Structure

```text
CM7/
  app/              high-level application flow
  gcode/            G-code parser
  machine_state/    modal machine state
  motion_cmd/       conversion from parsed G-code to IPC commands
  program_manager/  ring buffer for program lines
  ipc/              Cortex-M7 side of shared-memory IPC
  logger/           diagnostic message buffer
  USB_DEVICE/       USB CDC device layer

CM4/
  app/              command dispatch and runtime loop
  axis/             axis state and unit conversion
  homing/           logical homing
  ipc/              Cortex-M4 side of shared-memory IPC
  motion/           motion execution and STEP/DIR generator
  rt_status/        real-time status model
  safety/           alarm and E-stop state model
  spindle/          logical spindle state
```

## Configuration

The main mechanical parameters are defined in `config.h`:

```c
#define X_DRIVER_PULSES_PER_REV 6400
#define X_TRAVEL_UM_PER_REV     5000
```

With the current setup, one motor revolution equals 6400 driver pulses and 5000 um of linear travel.
This gives 1280 pulses per millimeter.

CM4-specific output logic and timing are configured in `config_cm4.h`, including STEP pulse width, enable polarity and DIR polarity.

## Communication Protocol

The firmware accepts simple ASCII commands over USB CDC:

```text
GC:G1 X10000 F1000;
ADD:G1 X10000 F1000;
RUN;
PAUSE;
RESUME;
STOP;
CLEAR;
STATUS;
RTSTATUS;
FOVR:100;
SOVR:100;
```

`GC` executes a single G-code line immediately.
`ADD` stores a line in the program buffer.
`RUN` starts buffered program execution.
`STATUS` returns program buffer state.
`RTSTATUS` returns real-time machine state.

## Motion Generation

The Cortex-M4 core generates motion from already prepared IPC segments.
TIM1 defines the step timing and TIM2 controls the STEP pulse width.
For multi-axis moves, the step generator selects the major axis and distributes steps on the remaining axes using a Bresenham-like interpolation method.

## Build

The project is intended to be opened and built with STM32CubeIDE.

Use `cnc_controler_h755.ioc` to inspect CubeMX configuration.
Both Cortex-M7 and Cortex-M4 targets must be built and flashed.

## Related Repository

This firmware is designed to work with the Qt desktop HMI repository:

```text
qt-cnc-hmi
```

## Status

Prototype / engineering project version.
The firmware is suitable as a base for further development of safety inputs, motion planning and hardware integration.
