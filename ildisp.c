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
// ------------------------------------------------------------------------------
//
// ildisp.c   HP-IL display module
// (used in Emu41/Emu71/ILper)
//
// Based on previous personal work:
// 1986: ILPER4 video+disc, (6502 assembler)
// 1988: ILPER5 videoil module
// 1993: ILPER ported on PC (8086 assembler)
// 1997: rewriten in C and included in Emu41
// 2008: rewriten in VB for the standalone ILPER Windows version using the PILBox!
// oct 2009: changed the AID to 46 (for general 'dumb// printer)
// 2011: ported on Linux by Ch. Gottheimer
// 2026: ported to macOS and thoroughly revised by J. Taillandier
//       (used to be a printer emulation, is now a real terminal emulator)
// ------------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <ncurses.h>

#include "ilper.h"

// HP-IL device identity
#define AID 0x32  // Accessory ID = MC00701A (80-column display)
#define DEFADDR 1 // default address after AAU

// Terminal emulator dimensions
#define NBUF_ROWS 100 // max lines in display buffer
#define NBUF_COLS 256 // max columns per line

static int disp_roman8 =
    1; // 1=roman8 (default), 0=inverse video for chars >= 128
static int cursor_insert = 0;        // 0=replace, 1=insert
static int cursor_hidden = 0; // 0=displayed, 1=not displayed

void set_display_encoding(int roman8)
{
    disp_roman8 = roman8;
}

// Keyboard input buffer
#define KBUF_SIZE 16
static char kbuf[KBUF_SIZE];
static int kbuf_head;
static int kbuf_tail;
static int kbuf_count;

static void kbuf_push(char ch)
{
    if (kbuf_count < KBUF_SIZE) {
        kbuf[kbuf_tail] = ch;
        kbuf_tail = (kbuf_tail + 1) % KBUF_SIZE;
        kbuf_count++;
    }
}

static int kbuf_pop(void)
{
    if (kbuf_count == 0)
        return -1;
    int ch = (unsigned char)kbuf[kbuf_head];
    kbuf_head = (kbuf_head + 1) % KBUF_SIZE;
    kbuf_count--;
    return ch;
}

void ildisp_kbuf_push(int ch)
{
    kbuf_push((char)(ch & 0xFF));
}

// Terminal emulator state
static char vbuf[NBUF_ROWS][NBUF_COLS]; // display buffer (max dimensions)
static int dirty[NBUF_ROWS];            // 1 if buffer row needs redrawing
static int view_rows = 24;              // visible rows (set at runtime)
static int view_cols = 80;              // visible columns (set at runtime)
static int cur_col;                     // cursor column (0..view_cols-1)
static int cur_row;                     // cursor row in buffer (0..NBUF_ROWS-1)
static int win_top; // first buffer row shown on screen (0..NBUF_ROWS-view_rows)

static void mark_all_dirty(void)
{
    memset(dirty, 1, sizeof(dirty));
}

void set_view_size(int rows, int cols)
{
    view_rows = (rows >= 1 && rows <= NBUF_ROWS) ? rows : 24;
    view_cols = (cols >= 1 && cols <= NBUF_COLS) ? cols : 80;
    memset(vbuf, ' ', sizeof(vbuf));
    cur_col = 0;
    cur_row = 0;
    win_top = 0;
    mark_all_dirty();
}

int get_view_rows(void)
{
    return view_rows;
}
int get_view_cols(void)
{
    return view_cols;
}

// ******************************************
// refresh_display()
//
// redraw the visible lines of the buffer
// into the ncurses Display window
// ******************************************
void refresh_display(void)
{
    int row, col;
    unsigned char c;
    cchar_t cc;
    wchar_t wc[2];

    wc[1] = L'\0';
    for (row = 0; row < view_rows; row++) {
        if (!dirty[win_top + row])
            continue;
        dirty[win_top + row] = 0;
        for (col = 0; col < view_cols; col++) {
            attr_t attr = A_NORMAL;
            c = (unsigned char)vbuf[win_top + row][col];
            if (c >= 128) {
                if (disp_roman8)
                    wc[0] = roman8_table[c];
                else {
                    wc[0] = (wchar_t)(c & 0x7F);
                    attr = A_REVERSE;
                }
            } else if (c < 32 || c == 127) {
                wc[0] = L' ';
            } else {
                wc[0] = (wchar_t)c;
            }
            setcchar(&cc, wc, attr, 0, NULL);
            mvwadd_wch(wtv, row, col, &cc);
        }
    }
    wmove(wtv, cur_row - win_top, cur_col);
    wrefresh(wtv);
}

// ******************************************
// term_write_char(ch)
//
// write a printable character at cursor
// position and advance the cursor
// ******************************************
static void term_write_char(char ch)
{
    vbuf[cur_row][cur_col] = ch;
    dirty[cur_row] = 1;
    cur_col++;
    if (cur_col >= view_cols) {
        cur_col = 0;
        if (cur_row < NBUF_ROWS - 1)
            cur_row++;
    }
}

// ******************************************
// term_ensure_visible()
//
// adjust win_top so the cursor row is
// within the visible window
// ******************************************
static void term_ensure_visible(void)
{
    if (cur_row < win_top) {
        win_top = cur_row;
        mark_all_dirty();
    } else if (cur_row >= win_top + view_rows) {
        win_top = cur_row - view_rows + 1;
        mark_all_dirty();
    }
}

// ******************************************
// cursor_update()
//
// move the ncurses cursor to current position.
// if win_top changed (cursor left the visible
// window), fall back to a full redraw.
// ******************************************
static void cursor_update(void)
{
    int old_top = win_top;
    term_ensure_visible();
    if (win_top != old_top) {
        refresh_display();
    } else {
        wmove(wtv, cur_row - win_top, cur_col);
        wrefresh(wtv);
    }
}

// ******************************************
// DisplayStr(ch)
//
// receive one byte from HP-IL, handle ESC
// sequences, write printable chars to buffer
// ******************************************
void DisplayStr(char ch)
{
    static int fesc = 0;  // 1 = waiting for ESC command byte
    static int faddr = 0; // 1 = waiting for col byte, 2 = waiting for row byte
    int n = (int)ch & 0xFF;

    if (faddr) {
        if (faddr == 1) {
            cur_col = n % view_cols;
            faddr = 2;
        } else {
            cur_row =
                win_top + (n % view_rows); // row relative to visible window
            faddr = 0;
        }
        return;
    }

    if (fesc) {
        fesc = 0;
        switch (ch) {
        case 'A': // Cursor Up
            if (cur_row > 0)
                cur_row--;
            cursor_update();
            return;
        case 'B': // Cursor Down
            if (cur_row < NBUF_ROWS - 1)
                cur_row++;
            cursor_update();
            return;
        case 'C': // Cursor Right
            cur_col++;
            if (cur_col >= view_cols) {
                cur_col = 0;
                if (cur_row < NBUF_ROWS - 1)
                    cur_row++;
                else {
                    cur_row = 0;
                    cur_col = 0;
                } // wrap to home at last pos
            }
            cursor_update();
            return;
        case 'D': // Cursor Left
            if (cur_col > 0) {
                cur_col--;
            } else if (cur_row > 0) {
                cur_row--;
                cur_col = view_cols - 1;
            }
            cursor_update();
            return;
        case 'E': // Clear Device: clear buffer, cursor home
            memset(vbuf, ' ', sizeof(vbuf));
            cur_col = 0;
            cur_row = 0;
            win_top = 0;
            mark_all_dirty();
            break;
        case 'H': // Cursor Home
            cur_col = 0;
            cur_row = 0;
            cursor_update();
            return;
        case 'J': // Clear from cursor to end of buffer
        {
            int r, c;
            for (c = cur_col; c < view_cols; c++)
                vbuf[cur_row][c] = ' ';
            dirty[cur_row] = 1;
            for (r = cur_row + 1; r < NBUF_ROWS; r++) {
                memset(vbuf[r], ' ', NBUF_COLS);
                dirty[r] = 1;
            }
        } break;
        case 'Q': // Insert Cursor: bar cursor (DECSCUSR 6)
            cursor_insert = 1;
            printf("\033[6 q");
            fflush(stdout);
            return;
        case 'R': // Replace Cursor: block cursor (DECSCUSR 2)
            cursor_insert = 0;
            printf("\033[2 q");
            fflush(stdout);
            return;
        case 'S': // Roll Up: window moves down (text appears to scroll up)
            if (win_top < NBUF_ROWS - view_rows)
                win_top++;
            mark_all_dirty();
            break;
        case 'T': // Roll Down: window moves up (text appears to scroll down)
            if (win_top > 0)
                win_top--;
            mark_all_dirty();
            break;
        case '<': // Cursor Off
            cursor_hidden = 1;
            curs_set(0);
            return;
        case '>': // Cursor On
            cursor_hidden = 0;
            curs_set(1);
            return;
        case '%': // Cursor To Address: two bytes follow (col, row)
            faddr = 1;
            return;
        default:
            return;
        }
        term_ensure_visible();
        refresh_display();
        return;
    }

    // normal character
    switch (n) {
    case 27:
        fesc = 1;
        return; // ESC
    case 13:
        cur_col = 0;
        break; // CR
    case 10:   // LF
        if (cur_row < NBUF_ROWS - 1)
            cur_row++;
        else {
            // buffer full: scroll the whole buffer up one row
            memmove(&vbuf[0][0], &vbuf[1][0], (NBUF_ROWS - 1) * NBUF_COLS);
            memset(vbuf[NBUF_ROWS - 1], ' ', NBUF_COLS);
            mark_all_dirty();
            // cur_row stays at NBUF_ROWS-1 (now blank, ready for new content)
        }
        break;
    case 0:
        break; // NUL, ignore
    default:
        term_write_char(ch);
        break;
    }
    term_ensure_visible();
    refresh_display();
}

// ******************************************
// ClrDisplay()
//
// clear the display buffer and refresh
// ******************************************
void ClrDisplay(void)
{
    memset(vbuf, ' ', sizeof(vbuf));
    cur_col = 0;
    cur_row = 0;
    win_top = 0;
    mark_all_dirty();
    refresh_display();
}

// --------------------------------------------------------------------------
// HP-IL protocol: delegate to hpil_dev_t base via callbacks
// --------------------------------------------------------------------------

static void disp_on_receive(hpil_dev_t *dev, int frame)
{
    (void)dev;
    DisplayStr((char)(frame & 0xFF));
}

static int disp_on_send(hpil_dev_t *dev)
{
    (void)dev;
    return kbuf_pop();
}

static int disp_on_status(hpil_dev_t *dev)
{
    (void)dev;
    return 0x80; // system status byte follows
}

static int disp_on_status2(hpil_dev_t *dev)
{
    (void)dev;
    // 0abc defg:
    //   a,b (bits 6,5): RAM test — always 0 (good)
    //   c   (bit 4):    cursor mode: 0=replace, 1=insert
    //   d   (bit 3):    cursor: 0=displayed, 1=not displayed
    //   e   (bit 2):    video: 0=NTSC, 1=PAM — always 0
    //   f   (bit 1):    charset: 0=inverse, 1=roman8
    //   g   (bit 0):    width: 0=80col, 1=40col — always 0
    return (cursor_insert << 4) | (cursor_hidden << 3) | (disp_roman8 << 1);
}

static void disp_on_clear(hpil_dev_t *dev)
{
    (void)dev;
    ClrDisplay();
}

static hpil_dev_t disp_dev = {
    .device_id = "DISPLAY\r",
    .defaddr = DEFADDR,
    .aid = AID,
    .on_receive = disp_on_receive,
    .on_send = disp_on_send,
    .on_status = disp_on_status,
    .on_status2 = disp_on_status2,
    .on_clear = disp_on_clear,
    .on_cmd = NULL,
};

int ildisplay_address(void)
{
    return hpil_dev_address(&disp_dev);
}

// ******************************************
// init_ildisplay()
//
// init the virtual HPIL display device
// ******************************************
void init_ildisplay(void)
{
    hpil_dev_init(&disp_dev);
    kbuf_head = 0;
    kbuf_tail = 0;
    kbuf_count = 0;
    cursor_insert = 0;
    cursor_hidden = 0;
    cur_col = 0;
    cur_row = 0;
    win_top = 0;
    memset(vbuf, ' ', sizeof(vbuf));
    mark_all_dirty();
}

// ******************************************
// ILdisplay(frame)
//
// manage a HPIL frame for the display device
// ******************************************
int ILdisplay(int frame)
{
    // SRQ raised when kbuf has data and no SDA transfer is active (bit 0x02).
    // Suppressing on talker state (0x40) alone was wrong: HP-71 (KEYBOARD IS :DISPLAY)
    // sends IDY and waits for IDY(SRQ) before issuing SDA; it never sees IDY(SRQ) if SRQ is
    // gated on the talker bit, because the display stays talker after the first TAD.
    disp_dev.srq = (kbuf_count > 0 && !(disp_dev.state & 0x02)) ? 1 : 0;
    return hpil_dev_process(&disp_dev, frame);
}
