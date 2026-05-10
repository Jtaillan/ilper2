# ilper2 — HP-IL Peripherals Emulator for macOS and Linux

ilper2 is built on the foundation provided by ilper 1.35 (Christophe
Gottheimer, 2011), which was itself derived from the original program by
Jean-François Garnier. The code has been extensively reworked, with the
primary goal of providing a video terminal interface supporting at least an
80×24 screen. The printer and mass-storage device have been preserved.
Configuration is now done exclusively through a dedicated file.

Tested with HP-41C/CV/CX (+ HP82160A module) and HP-71B.


## Requirements

- A PILBox adapter (Jean-François Garnier or Karel Koci)
- macOS 10.14+ or Linux
- ncurses (included on macOS; `libncursesw5-dev` on Debian/Ubuntu)
- A terminal emulator at least 80 columns wide and 25 lines tall
  (Ghostty, iTerm2, Terminal.app, xterm, …)
- On macOS: the AppleUSBFTDI driver (built-in since 10.14) or the
  third-party FTDI driver
- On Linux: the `ftdi_sio` kernel module (loaded automatically when the
  PILBox is plugged in); the user must be in the `dialout` group


## Build

```
make                     build for the native architecture
make ARCH=arm64          Apple Silicon only
make ARCH=x86_64         Intel only
make ARCH=universal      fat binary (arm64 + x86_64)

make install             install to /usr/local/bin and /usr/local/share/man/man1
```


## Configuration

ilper2 requires a configuration file. Create it at:

```
~/.config/ilper/ilper.conf
```

Example:

```
# ilper configuration
device    = /dev/cu.usbserial-XXXXXXXX   # macOS
baudrate  = 115200
keyboard  = on
LIF_file1 = ~/HP71/HDRIVE1.DAT
LIF_file2 = ~/HP71/HDRIVE2.DAT
printer   = ~/HP71/printer.log
scope     = ~/HP71/scope.log
```

Keys:

| Key                | Mandatory | Description |
|--------------------|-----------|-------------|
| `device`           | yes | Serial port of the PILBox. |
| `baudrate`         | yes | 115200 or 230400. |
| `keyboard`         | yes | `on`: forward Mac keyboard to the HP calculator. `off`: disable. |
| `display_encoding` | no  | `roman8` (default): HP Roman-8 → Unicode. `inverse`: low 7 bits in reverse video. |
| `LIF_file1`        | no  | Path to the first LIF disk image. If absent, no drive is emulated. |
| `LIF_file2`        | no  | Path to the second LIF disk image. Requires `LIF_file1`. |
| `printer`          | no  | Path to the printer log file. If absent, printer output is discarded. |
| `scope`            | no  | Path to the HP-IL frame log file. If absent, logging is disabled. |

Paths may be absolute (`/path/to/file`), relative to `$HOME` (just a filename
or subpath), or use `~` as a shorthand for `$HOME`.

Unknown keys in the configuration file are treated as errors.


## Usage

```
ilper
```

The program uses the full terminal window, which can be resized. To exit, press **Ctrl-C**.

Only one instance of ilper may run on the same serial port at a time.


## Screen Layout

```
+--[DISPLAY]:1  [DRIVE1]:2  [DRIVE2]:3  [PRINTER]:4 ----- [SCOPE]--+
+------------------------------------------------------------------+
|                                                                  |
|   (display terminal — output from the HP calculator)             |
|                                                                  |
```

- **Row 0:** Status bar. Shows the HP-IL address of each active peripheral,
  and `[SCOPE]` when frame logging is active.
- **Row 1:** Separator line.
- **Rows 2+:** Display terminal window, sized to the terminal. Adapts automatically
  when the terminal is resized (SIGWINCH). The scroll buffer holds 100 lines.

The display uses the terminal's native colors (no forced colors).


## HP-IL Addresses

Devices are assigned addresses in two ways:

- After an **AAU** (Auto-Address Unconfigure) message, devices take their default fixed addresses: display=1, drive1=2, drive2=3, printer=4 (only for devices present in ilper.conf).
- After **AAD** (Auto-Address), addresses are assigned in physical loop order.

Extended addressing is not supported.

## ESC Sequences

The HP calculator can send ESC sequences to control the display terminal:

| Sequence | Effect |
|----------|--------|
| `ESC A`  | Cursor up |
| `ESC B`  | Cursor down |
| `ESC C`  | Cursor right |
| `ESC D`  | Cursor left |
| `ESC E`  | Clear device (clear buffer, cursor home) |
| `ESC H`  | Cursor home |
| `ESC J`  | Clear from cursor to end of buffer |
| `ESC Q`  | Insert mode — bar cursor |
| `ESC R`  | Replace mode — block cursor |
| `ESC S`  | Roll up (scroll window down) |
| `ESC T`  | Roll down (scroll window up) |
| `ESC <`  | Cursor off |
| `ESC >`  | Cursor on |
| `ESC %`  | Cursor to address (two bytes follow: column, row) |

`ESC Q` and `ESC R` require a terminal emulator that supports DECSCUSR (Ghostty, iTerm2, most xterm-compatible terminals).


## Keyboard Forwarding

When `keyboard = on`, keystrokes typed on the Mac are forwarded to the HP
calculator via the HP-IL keyboard buffer. This feature requires an HP-71B.

On the HP-71B, use `KEYBOARD IS :DISPLAY` to designate the display terminal
as the keyboard source, and `ESCAPE` to enable translation of the received
ESC sequences into HP-71 key codes.

| Key        | Sent to HP calculator |
|------------|-----------------------|
| Arrow keys | ESC A / ESC B / ESC C / ESC D |
| Return     | CR (0x0D) |
| Escape     | ESC ESC |
| Backspace  | ESC i |
| Other keys | sent as-is |


## LIF Disk Image

The LIF disk image is a flat binary file emulating a 640 KB floppy
(80 tracks, 2 sides, 16 sectors of 256 bytes = 2560 sectors).

If the file does not exist or is not readable and writable, ilper exits with an error message.

Valid LIF images can also be created with the `lifutils lifinit` command
(see [lifutils](https://github.com/bug400/lifutils)).
A new image can be formatted from the HP calculator using the standard HP-IL mass-storage commands.


## Authors

- Original ilper for Windows: Jean-François Garnier, 2008–2009
- Linux port: Christophe Gottheimer, 2011
- macOS port, full revision and extension: Janick Taillandier, 2026
