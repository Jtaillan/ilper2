# ILPER2 — Code Architecture

## Overview

ILPER2 (version 2.5) emulates HP-IL peripheral devices on macOS/Linux. It connects to an HP calculator via a PILBox USB-serial adapter and presents virtual devices on the HP-IL loop: a display terminal, up to two mass-storage drives, and a printer.

The program runs in a terminal window managed by ncurses. Display device output is rendered in the ncurses window; printer output and scope output (HP-IL frame mnemonics) are each written to a separate log file.

---

## Source files

### `ilmain.c` — Main module

- Reads the configuration file `~/.config/ilper/ilper.conf`.
- Opens and configures the serial port (PILBox) in non-blocking mode.
- Initializes ncurses and the display window.
- Runs the main event loop (`select` on the serial port and stdin).
- Handles `SIGINT`, `SIGTERM` and `SIGWINCH` (terminal resize).
- Internal functions: `ReadPILBox()`, `SendFrame()`, `PILBox()`, `InitPILBox()`, `read_config()`, `start_emulation()`, `stop_emulation()`, `update_status()`.

### `ilper7.c` — HP-IL frame router

- `hpil_transmit(frame)`: dispatches each frame to all active virtual devices in order (display, drive 1, drive 2, printer); each device may modify the frame before it is passed to the next.
- `init_hpil()`: initializes all virtual devices at startup.

### `scope.c` — HP-IL frame logger

- `ILMnemo(frame)`: decodes the 11 bits of an HP-IL frame into a human-readable mnemonic. The 4 frame classes are handled in spec order, with all named mnemonics:
  - **DOE**: `DAB`, `DAB(SRQ)`, `END`, `END(SRQ)`
  - **CMD**: ACG group (NUL, GTL, SDC, PPD, GET, ELN, PPE, DDL, DDT), UCG group (NOP, LLO, DCL, PPU, EAR, IFC, REN, NRE, AAU, LPD), LAG (LAD, UNL), TAG (TAD, UNT), SAG (SAD)
  - **RDY**: RFC; ARG group (ETO, ETE, NRD, SDA, SST, SDI, SAI, TCT); AAG group (AAD, IAA, AEP, IEP, ZES, AES, IES, AMP, IMP)
  - **IDY**: `IDY`, `IDY(SRQ)`
- `scope_open(path)` / `scope_close()`: open and close the log file.
- `scope_log(in, out)`: logs one frame; prints `IN -> OUT` if a device modified it.
- Consecutive identical lines are collapsed into a single `(xN)` entry.

### `ilbase.c` / `ilbase.h` — HP-IL device base

All virtual devices share the same HP-IL protocol layer (AAU/AAD addressing, talker/listener state machine, SDI/SST/SAI/SDA/NRD/IFC/IDY-SRQ). This layer is implemented here.

**`hpil_dev_t` structure** (must be embedded as the first field of every device struct):
- Identity: `device_id`, `defaddr`, `aid`
- Protocol state: `adr`, `state`, `ptsdi`, `srq`
- Optional callbacks: `on_receive`, `on_send`, `on_status`, `on_clear`, `on_cmd`

Public functions:
- `hpil_dev_init(dev)`: initializes protocol state.
- `hpil_dev_process(dev, frame)`: processes one frame and returns the (possibly modified) frame.
- `hpil_dev_address(dev)`: returns the current 5-bit HP-IL address, or -1 if not yet addressed.

### `ildisp.c` — HP-IL display terminal

- Virtual screen buffer with ESC sequence handling and control code rendering.
- Selectable encoding: HP Roman-8 or ASCII (`set_display_encoding()`).
- Circular keyboard buffer (`kbuf`); `ildisp_kbuf_push()` feeds characters from `ilmain`.
- SRQ is raised when the keyboard buffer has data and no SDA transfer is active; the SRQ bit is inserted into IDY frames by `hpil_dev_process()`.
- ncurses rendering: `refresh_display()`, `set_view_size()`, `get_view_rows()`, `get_view_cols()`.
- `init_ildisplay()`: initialization; `ildisplay_address()`: current HP-IL address.

### `ildrive.c` — HP-IL mass storage

- Emulates up to two 640 KB LIF drives.
- `init_ilhdisc(file1, file2)`: initialization with disk image paths.
- `ilhdisc1_address()` / `ilhdisc2_address()`: current HP-IL addresses.
- `ilhdisc1_active()` / `ilhdisc2_active()`: whether each drive is present on the loop.

### `ilprint.c` — HP-IL printer

- Writes received data bytes to a configurable log file, rendering control codes as space and ignoring DEL.
- `init_ilprint(path)`: initialization; `close_ilprint()`: close.
- `ilprint_address()`: current HP-IL address; `ilprint_active()`: whether the printer is active.

### `roman8.c` — HP Roman-8 to UTF-8 conversion

- `roman8_table[256]`: maps all 256 Roman-8 byte values to Unicode code points.
- `roman8_to_utf8(wc, buf)`: encodes a Unicode code point to UTF-8 into `buf`.

### `ilper.h` — Global prototypes

Includes `ilbase.h` and declares the public interfaces of all modules.

---

## HP-IL frame format

An HP-IL frame is an 11-bit integer: **CCC** (3-bit class) + **DDDDDDDD** (8-bit data).

| Bits 10–8 (CCC) | Class    | Description                     |
|-----------------|----------|---------------------------------|
| `000`           | DAB      | Data byte (DOE)                 |
| `001`           | DAB(SRQ) | Data byte with SRQ (DOE)        |
| `010`           | END      | End of data (DOE)               |
| `011`           | END(SRQ) | End of data with SRQ (DOE)      |
| `100`           | CMD      | Command                         |
| `101`           | RDY      | Ready                           |
| `110`           | IDY      | Identify                        |
| `111`           | IDY(SRQ) | Identify with SRQ               |

---

## HP-IL frame flow

```
PILBox (serial) → ilmain.c
    → hpil_transmit()        [ilper7.c]
        → hpil_dev_process() [ilbase.c]  for each active device
        → scope_log()        [scope.c]
    → PILBox (serial)
```

Each frame received from the PILBox is reconstructed into 11 bits, dispatched sequentially to all active devices (each may modify it), logged by the scope, then sent back to the PILBox.

---

## SRQ mechanism

The display device raises `dev.srq` when the keyboard buffer (`kbuf`) has data and no SDA transfer is in progress. `hpil_dev_process()` then sets the SRQ bit (bit 0 of the data byte) in every IDY frame, turning `IDY` into `IDY(SRQ)`.

---

## ncurses layout

| Row  | Content           |
|------|-------------------|
| 0    | Status bar        |
| 1    | Separator         |
| 2+   | Display window    |

---

## Configuration

File: `~/.config/ilper/ilper.conf`

Format: `key = value  # optional comment`

Lines beginning with `#` are full-line comments. Inline comments are also
supported: a `#` preceded by whitespace terminates the value. Unknown keys
are errors.

---

## Build

```bash
make
make ARCH=arm64
make ARCH=x86_64
make ARCH=universal
make install
```
