// ------------------------------------------------------------------------------
// ILPER 1.35 for Linux
// Copyright (c) 2008-2009  J-F Garnier
// Copyright (c) 2011-2012  Ch. Gottheimer
// Copyright (c) 2026       J. Taillandier
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// ----------------------------------------------------------------------------------
//
// ilmain.c   ILPER main module
//
// Based on previous personal work:
// 1986: ILPER4 (6502 assembler)
// 1988: ILPER5 modular version (6502 assembler)
// 1993: ILPER6 ported on PC (8086 assembler)
// 1997: rewriten in C and included in Emu41
// 2008: rewriten in VB for the standalone ILPER Windows version using the PILBox!
// 2009: released as free software
// 2011: ported on Linux by Ch. Gottheimer with NCURSES
// 2026: ported to macOS and thoroughly revised by J. Taillandier
//       (added a new display terminal, Roman-8 compatible)
// ---------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <limits.h>
#include <errno.h>
#include <ncurses.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>

#include "ilper.h"

const char ilper_vers[] = "2.5";

#define CON 0x496
#define COFF 0x497
#define COFI 0x495
#define TDIS 0x494

char lif_file1[_POSIX_PATH_MAX];
char lif_file2[_POSIX_PATH_MAX];
static char strpo[_POSIX_PATH_MAX];
static speed_t ilbaudrate = 0; // 0 = not set
static char config_path[_POSIX_PATH_MAX];
static char strsc[_POSIX_PATH_MAX];
char strpr[_POSIX_PATH_MAX];
static int ilfd = -1;
static int lasth = 0;
static int have_colors = 0;
static int keyboard_on = -1; // 1 = COFI (IDY forwarded), 0 = COFF, -1 = not set

WINDOW *wtv;
static int x, y;

static volatile sig_atomic_t quit_flag = 0;
static volatile sig_atomic_t resize_flag = 0;
static void update_status(void);

// ******************************************
// on_sigint()
//
// SIGINT handler: request clean exit
// ******************************************
static void on_sigint(int sig)
{
    (void)sig;
    quit_flag = 1;
}

// ******************************************
// on_sigwinch()
//
// SIGWINCH handler: request terminal resize
// ******************************************
static void on_sigwinch(int sig)
{
    (void)sig;
    resize_flag = 1;
}

// ******************************************
// handle_resize()
//
// reinitialize ncurses and wtv after a
// terminal resize; ignored if too small
// ******************************************
static void handle_resize(void)
{
    endwin();
    refresh();
    getmaxyx(stdscr, y, x);

    if (y < 25 || x < 80) {
        mvprintw(0, 0, "Terminal too small (%dx%d), minimum 25x80", y, x);
        refresh();
        return;
    }

    delwin(wtv);
    mvhline(1, 0, ACS_HLINE, x);
    set_view_size(y - 2, x);
    wtv = newwin(get_view_rows(), get_view_cols(), 2, 0);
    scrollok(wtv, 0);
    keypad(wtv, TRUE);
    nodelay(wtv, TRUE);
    move(0, 0);
    clrtoeol();
    update_status();
    refresh_display();
}

// ******************************************
// ReadPILBox(&byt)
//
// try to read one byte from PILBox
// return  1: byte read into *byt
// return  0: timeout (no data within 50 ms)
// return -1: I/O error — caller must reconnect
// ******************************************
static int ReadPILBox(char *byt)
{
    struct timeval tv = {0, 50000}; // 50ms
    fd_set pilset;
    int rc;

    FD_ZERO(&pilset);
    FD_SET(ilfd, &pilset);
    rc = select(ilfd + 1, &pilset, NULL, NULL, &tv);
    if (rc == 1) {
        if (read(ilfd, byt, 1) != 1)
            return -1; // I/O error on the port
        return 1;
    }
    *byt = 0;
    return 0; // timeout or EINTR (signal): not a disconnect
}

// ******************************************
// SendFrame(frame)
//
// Encode an 11-bit HP-IL frame into the PILBox 2-byte serial protocol:
//   high byte (bit 7=0): 0b001x_xxx0  frame bits [10:7] in bits [4:1]
//                                      (frame type + data MSB)
//   low  byte (bit 7=1): 0b1xxx_xxxx  frame bits  [6:0] in bits [6:0]
// The high byte is suppressed when identical to the previous frame (lasth).
// Returns 0 on success, -1 on I/O error.
// ******************************************
static int SendFrame(int frame)
{
    unsigned char hbyt = ((frame >> 6) & 0x1E) | 0x20;
    unsigned char lbyt = (frame & 0x7F) | 0x80;

    if (hbyt != lasth) {
        lasth = hbyt;
        unsigned char buf[2] = {hbyt, lbyt};
        return (write(ilfd, buf, 2) == 2) ? 0 : -1;
    }
    return (write(ilfd, &lbyt, 1) == 1) ? 0 : -1;
}

// ******************************************
// PILBox(byt)
//
// manage the PILBox
// (based on Emu41 ilext2 module)
// called at each received byte.
// return  0: ok
// return -1: I/O error — caller must reconnect
// ******************************************
static int PILBox(int byt)
{
    int frame;

    if ((byt & 0xC0) == 0) {
        if (byt & 0x20) {
            lasth = (int)byt & 0xFF;
            if (write(ilfd, "\r", 1) != 1) // acknowledge high byte
                return -1;
        }
    } else {
        if (byt & 0x80)
            frame = ((lasth & 0x1E) << 6) + (byt & 0x7F);
        else
            frame = ((lasth & 0x1F) << 6) + (byt & 0x3F);

        frame = hpil_transmit(frame);
        return SendFrame(frame);
    }
    return 0;
}

// ******************************************
// InitPILBox(cmd)
//
// init the PIL Box
// ******************************************
static int InitPILBox(int cmd)
{
    char byt;

    lasth = 0;
    if (SendFrame(cmd) != 0)
        return -1;
    if (1 == ReadPILBox(&byt))
        if ((byt & 0x3F) == (cmd & 0x3F))
            return 0;

    return -1;
}

// ******************************************
// build_config_path()
//
// build path to ~/.config/ilper/ilper.conf
// respecting XDG_CONFIG_HOME if set
// ******************************************
static void build_config_path(void)
{
    const char *xdg = getenv("XDG_CONFIG_HOME");
    const char *home;

    if (xdg && *xdg)
        snprintf(config_path, sizeof(config_path), "%s/ilper/ilper.conf", xdg);
    else {
        home = getenv("HOME");
        if (!home || !*home)
            home = "/tmp";
        snprintf(config_path, sizeof(config_path),
                 "%s/.config/ilper/ilper.conf", home);
    }
}

// ******************************************
// resolve_path(val, buf, size)
//
// resolve a config file path:
//   ~/foo  -> $HOME/foo
//   foo    -> $HOME/foo  (relative: anchored to HOME)
//   /foo   -> /foo       (absolute: used as-is)
// ******************************************
static void resolve_path(const char *val, char *buf, size_t size)
{
    const char *home = getenv("HOME");
    if (!home || !*home)
        home = "/tmp";

    if (val[0] == '~' && (val[1] == '/' || val[1] == '\0'))
        snprintf(buf, size, "%s%s", home, val + 1);
    else if (val[0] != '/')
        snprintf(buf, size, "%s/%s", home, val);
    else
        snprintf(buf, size, "%s", val);
}

// ******************************************
// read_config()
//
// parse ilper.conf: key=value lines,
// '#' comments ignored. Keys: device,
// LIF_file, baudrate (115200 or 230400),
// scope (path to scope log file).
// Missing file or keys keep defaults.
// ******************************************
static void read_config(void)
{
    FILE *f = fopen(config_path, "r");
    if (!f) {
        fprintf(stderr, "ilper: cannot open config file %s: %s\n", config_path,
                strerror(errno));
        exit(1);
    }

    int seen_device = 0, seen_lif1 = 0, seen_lif2 = 0;
    int seen_baudrate = 0, seen_scope = 0, seen_printer = 0, seen_keyboard = 0;
    int seen_dispenc = 0;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = '\0';

        char *p = line;
        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == '#' || *p == '\0')
            continue;

        char *eq = strchr(p, '=');
        if (!eq)
            continue;
        *eq = '\0';

        char *key = p;
        char *e = eq - 1;
        while (e >= key && (*e == ' ' || *e == '\t'))
            *e-- = '\0';

        char *val = eq + 1;
        while (*val == ' ' || *val == '\t')
            val++;

        // strip inline comment: # at start of val or preceded by whitespace
        for (char *c = val; *c; c++) {
            if (*c == '#' && (c == val || c[-1] == ' ' || c[-1] == '\t')) {
                *c = '\0';
                break;
            }
        }
        // strip trailing whitespace from val
        char *ve = val + strlen(val);
        while (ve > val && (ve[-1] == ' ' || ve[-1] == '\t'))
            *--ve = '\0';

        if (strcmp(key, "device") == 0) {
            if (seen_device++) {
                fprintf(stderr, "ilper: duplicate key: device\n");
                fclose(f);
                exit(1);
            }
            resolve_path(val, strpo, sizeof(strpo));
        } else if (strcmp(key, "LIF_file1") == 0) {
            if (seen_lif1++) {
                fprintf(stderr, "ilper: duplicate key: LIF_file1\n");
                fclose(f);
                exit(1);
            }
            resolve_path(val, lif_file1, sizeof(lif_file1));
        } else if (strcmp(key, "LIF_file2") == 0) {
            if (seen_lif2++) {
                fprintf(stderr, "ilper: duplicate key: LIF_file2\n");
                fclose(f);
                exit(1);
            }
            resolve_path(val, lif_file2, sizeof(lif_file2));
        } else if (strcmp(key, "scope") == 0) {
            if (seen_scope++) {
                fprintf(stderr, "ilper: duplicate key: scope\n");
                fclose(f);
                exit(1);
            }
            resolve_path(val, strsc, sizeof(strsc));
        } else if (strcmp(key, "printer") == 0) {
            if (seen_printer++) {
                fprintf(stderr, "ilper: duplicate key: printer\n");
                fclose(f);
                exit(1);
            }
            resolve_path(val, strpr, sizeof(strpr));
        } else if (strcmp(key, "baudrate") == 0) {
            if (seen_baudrate++) {
                fprintf(stderr, "ilper: duplicate key: baudrate\n");
                fclose(f);
                exit(1);
            }
            int baud = atoi(val);
            if (baud == 115200)
                ilbaudrate = B115200;
            else if (baud == 230400)
                ilbaudrate = B230400;
            else {
                fprintf(stderr,
                        "ilper: invalid baudrate: %s (use 115200 or 230400)\n",
                        val);
                fclose(f);
                exit(1);
            }
        } else if (strcmp(key, "keyboard") == 0) {
            if (seen_keyboard++) {
                fprintf(stderr, "ilper: duplicate key: keyboard\n");
                fclose(f);
                exit(1);
            }
            if (strcmp(val, "on") == 0)
                keyboard_on = 1;
            else if (strcmp(val, "off") == 0)
                keyboard_on = 0;
            else {
                fprintf(
                    stderr,
                    "ilper: invalid keyboard value: %s (use 'on' or 'off')\n",
                    val);
                fclose(f);
                exit(1);
            }
        } else if (strcmp(key, "display_encoding") == 0) {
            if (seen_dispenc++) {
                fprintf(stderr, "ilper: duplicate key: display_encoding\n");
                fclose(f);
                exit(1);
            }
            if (strcmp(val, "roman8") == 0)
                set_display_encoding(1);
            else if (strcmp(val, "inverse") == 0)
                set_display_encoding(0);
            else {
                fprintf(stderr,
                        "ilper: invalid display_encoding value: %s (use "
                        "'roman8' or 'inverse')\n",
                        val);
                fclose(f);
                exit(1);
            }
        } else {
            fprintf(stderr, "ilper: unknown config key: %s\n", key);
            fclose(f);
            exit(1);
        }
    }
    fclose(f);
}

// ******************************************
// update_status()
//
// refresh the status indicators on row 0
// ******************************************
static void update_status(void)
{
    int addr;

    move(0, 0);
    clrtoeol();

    addr = ildisplay_address();
    if (addr >= 0)
        mvprintw(0, 0, "[DISPLAY]:%-2d", addr);
    else
        mvprintw(0, 0, "             ");

    addr = (ilhdisc1_active() && ilhdisc1_address() >= 0) ? ilhdisc1_address()
                                                          : -1;
    if (addr >= 0)
        mvprintw(0, 14, "[DRIVE1]:%-2d  ", addr);
    else
        mvprintw(0, 14, "             ");

    addr = (ilhdisc2_active() && ilhdisc2_address() >= 0) ? ilhdisc2_address()
                                                          : -1;
    if (addr >= 0)
        mvprintw(0, 28, "[DRIVE2]:%-2d  ", addr);
    else
        mvprintw(0, 28, "             ");

    addr =
        (ilprint_active() && ilprint_address() >= 0) ? ilprint_address() : -1;
    if (addr >= 0)
        mvprintw(0, 42, "[PRINTER]:%-2d", addr);
    else
        mvprintw(0, 42, "             ");

    if (ilscope_active()) {
        if (have_colors)
            attron(COLOR_PAIR(2));
        mvprintw(0, x - 9, "[SCOPE]  ");
        if (have_colors)
            attroff(COLOR_PAIR(2));
    } else {
        mvprintw(0, x - 9, "         ");
    }
    wnoutrefresh(stdscr);
    wnoutrefresh(wtv); // cursor ends up in the display window
    doupdate();
}

// ******************************************
// start_emulation()
//
// open serial port, configure it, init
// PILBox and start HP-IL emulation.
// Returns 0 on success, -1 on failure.
// ******************************************
static int start_emulation(void)
{
    struct termios tp;

    ilfd = open(strpo, O_RDWR | O_NOCTTY);
    if (ilfd == -1)
        return -1;

    if (flock(ilfd, LOCK_EX | LOCK_NB) == -1) {
        close(ilfd);
        ilfd = -1;
        endwin();
        fprintf(stderr, "ilper: another instance is already running on %s\n",
                strpo);
        exit(1);
    }

    memset(&tp, 0, sizeof(tp));
    tcgetattr(ilfd, &tp);

    tp.c_cflag |= (CLOCAL | CREAD);
    cfsetospeed(&tp, ilbaudrate);
    cfsetispeed(&tp, ilbaudrate);
    tp.c_cflag &= ~CSIZE;
    tp.c_cflag |= CS8;
    tp.c_cflag &= ~PARENB;
    tp.c_cflag &= ~CSTOPB;
    tp.c_cflag &= ~CRTSCTS;
    tp.c_iflag &= ~(IXON | IXOFF | IXANY | ICRNL | INLCR | IGNCR);
    tp.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tp.c_oflag &= ~OPOST;
    tp.c_cc[VMIN] = 0;
    tp.c_cc[VTIME] = 10; // 1 second
    tcsetattr(ilfd, TCSAFLUSH, &tp);

    if (InitPILBox(keyboard_on ? COFI : COFF) != 0) {
        close(ilfd);
        ilfd = -1;
        return -1;
    }

    werase(wtv);
    wrefresh(wtv);
    init_hpil();
    refresh_display();
    update_status();
    return 0;
}

// ******************************************
// stop_emulation()
//
// send TDIS to PILBox and close serial port
// ******************************************
static void stop_emulation(void)
{
    if (ilfd == -1)
        return;
    InitPILBox(TDIS); // best-effort: may fail if port already gone
    close(ilfd);
    ilfd = -1;
}

// ******************************************
// main()
//
// ilper main
// ******************************************
int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    lif_file1[0] = '\0';
    lif_file2[0] = '\0';
    strpo[0] = '\0';
    strsc[0] = '\0';
    strpr[0] = '\0';

    build_config_path();
    read_config();

    if (lif_file1[0] == '\0' && lif_file2[0] != '\0') {
        fprintf(stderr, "ilper: LIF_file2 set without LIF_file1 in %s\n",
                config_path);
        exit(1);
    }
    if (strpo[0] == '\0') {
        fprintf(stderr, "ilper: 'device' not set in %s\n", config_path);
        exit(1);
    }
    if (ilbaudrate == 0) {
        fprintf(stderr, "ilper: 'baudrate' not set in %s\n", config_path);
        exit(1);
    }
    if (keyboard_on < 0) {
        fprintf(stderr, "ilper: 'keyboard' not set in %s\n", config_path);
        exit(1);
    }

    // Validate LIF files before ncurses takes the screen
    if (lif_file1[0] != '\0' && access(lif_file1, R_OK | W_OK) != 0) {
        fprintf(stderr, "ilper: LIF_file1 %s: %s\n", lif_file1,
                strerror(errno));
        exit(1);
    }
    if (lif_file2[0] != '\0' && access(lif_file2, R_OK | W_OK) != 0) {
        fprintf(stderr, "ilper: LIF_file2 %s: %s\n", lif_file2,
                strerror(errno));
        exit(1);
    }

    signal(SIGINT, on_sigint);
    signal(SIGTERM, on_sigint);
    signal(SIGWINCH, on_sigwinch);

    setlocale(LC_ALL, "");
    if (!initscr()) {
        fprintf(stderr, "ilper: cannot initialize terminal\n");
        exit(1);
    }
    getmaxyx(stdscr, y, x);
    if ((25 > y) || (80 > x)) {
        endwin();
        fprintf(stderr,
                "Sorry, I need 25 lines and 80 columns to run."
                " %d lines, %d columns\n",
                y, x);
        exit(1);
    }
    if (has_colors()) {
        have_colors = 1;
        start_color();
        use_default_colors();
        init_pair(1, -1, -1);
        init_pair(2, COLOR_RED, -1);
    }

    noecho();
    cbreak();
    refresh();

    // Row 1: separator line; display window starts at row 2
    mvhline(1, 0, ACS_HLINE, x);
    set_view_size(y - 2, x);
    wtv = newwin(get_view_rows(), get_view_cols(), 2, 0);
    scrollok(wtv, 0);
    keypad(wtv, TRUE);
    nodelay(wtv, TRUE);
    wrefresh(wtv);

    scope_open(strsc);

    if (start_emulation() != 0) {
        endwin();
        fprintf(stderr, "ilper: cannot open PILBox on %s\n", strpo);
        exit(1);
    }

    while (!quit_flag) {
        if (resize_flag) {
            resize_flag = 0;
            handle_resize();
        }

        if (keyboard_on) {
            int key = wgetch(wtv);
            if (key != ERR && key != 3) {
                if (key == KEY_UP) {
                    ildisp_kbuf_push(27);
                    ildisp_kbuf_push('A');
                } else if (key == KEY_DOWN) {
                    ildisp_kbuf_push(27);
                    ildisp_kbuf_push('B');
                } else if (key == KEY_RIGHT) {
                    ildisp_kbuf_push(27);
                    ildisp_kbuf_push('C');
                } else if (key == KEY_LEFT) {
                    ildisp_kbuf_push(27);
                    ildisp_kbuf_push('D');
                } else if (key == '\n' || key == KEY_ENTER) {
                    ildisp_kbuf_push(0x0D);
                } else if (key == 27) {
                    ildisp_kbuf_push(27);
                    ildisp_kbuf_push(27);
                } else if (key == KEY_BACKSPACE || key == 127) {
                    ildisp_kbuf_push(27);
                    ildisp_kbuf_push('i');
                } else if (key < 256) {
                    ildisp_kbuf_push(key);
                }
            }
        }

        char byt;
        int rc = ReadPILBox(&byt);
        if (rc == -1) {
            quit_flag = 1;
        } else if (rc == 1) {
            if (PILBox((int)byt & 0xFF) == -1)
                quit_flag = 1;
            else
                update_status();
        }
    }

    stop_emulation();
    scope_close();
    close_ilprint();

    printf("\033[0 q"); // restore default cursor shape
    fflush(stdout);
    erase();
    refresh();
    endwin();

    return 0;
}
